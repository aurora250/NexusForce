#include <NeForce/core/algorithm/remove.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/signal.hpp>
#include <NeForce/core/async/atomic.hpp>
#include <NeForce/core/async/condition_variable.hpp>
#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/async/thread.hpp>
#include <NeForce/core/container/flat_unordered_map.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/async/waitable_event.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windef.h>
#    include <WinBase.h>
#    include <consoleapi2.h>
#    include <processthreadsapi.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/system/pipe.hpp>
#    include <csignal>
#    include <cstdlib>
#    include <unistd.h>
#    include <fcntl.h>
#    include <sys/select.h>
#    include <sys/syscall.h>
#    include <sys/types.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    waitable_event* g_notify_event = nullptr;

    system_signal::notification map_win_notification(unsigned long event_id) {
        switch (event_id) {
            case CTRL_C_EVENT:
                return system_signal::notification::from_signal(system_signal::signal::interrupt);
            case CTRL_BREAK_EVENT:
                return system_signal::notification::from_event(system_signal::event::ctrl_break);
            case CTRL_CLOSE_EVENT:
                return system_signal::notification::from_event(system_signal::event::close);
            case CTRL_LOGOFF_EVENT:
                return system_signal::notification::from_event(system_signal::event::logoff);
            case CTRL_SHUTDOWN_EVENT:
                return system_signal::notification::from_event(system_signal::event::shutdown);
            default:
                return system_signal::notification::from_event(system_signal::event::custom_1);
        }
    }

    ::BOOL WINAPI windows_handler(unsigned long event_id) {
        if (g_notify_event != nullptr) {
            const auto value = map_win_notification(event_id);
            if (value.is_signal()) {
                system_signal::instance().enqueue_signal(value.signal_value());
            } else {
                system_signal::instance().enqueue_event(value.event_value());
            }
        }
        return TRUE;
    }

#else
    pipe& get_signal_pipe() {
        static pipe p(false);
        return p;
    }

    pipe* g_signal_pipe = nullptr;
    struct ::sigaction* g_previous_actions = nullptr;
    volatile sig_atomic_t g_chain_previous[64]{};
    volatile sig_atomic_t g_chain_enabled = 1;

    void posix_handler(const int sig, ::siginfo_t* info, void* context) {
        const int saved_errno = errno;
        if (sig > 0 && sig < 64 && g_chain_previous[sig] == 1) {
            errno = saved_errno;
            return;
        }
        if (sig > 0 && sig < 64 && g_chain_previous[sig] == 2 && g_chain_enabled != 0 &&
            g_previous_actions != nullptr) {
            const auto& previous = g_previous_actions[sig];
            if ((previous.sa_flags & SA_SIGINFO) != 0 && previous.sa_sigaction != nullptr) {
                previous.sa_sigaction(sig, info, context);
            } else if (previous.sa_handler != nullptr && previous.sa_handler != SIG_DFL &&
                       previous.sa_handler != SIG_IGN) {
                previous.sa_handler(sig);
            }
        }
        if (g_signal_pipe != nullptr) {
            ::write(g_signal_pipe->native_write_handle(), &sig, sizeof(sig));
        }
        errno = saved_errno;
    }

#endif

} // namespace


struct system_signal::impl {
    atomic<bool> running_{false};
    atomic<bool> force_exit_{false};
    atomic<int> force_exit_timeout_{5000};
    atomic<int> guard_count_{0};

    mutex mutex_;
    condition_variable cv_;

    flat_unordered_map<signal, notification_handler> notification_handlers_;
    flat_unordered_map<event, notification_handler> event_handlers_;
    vector<pending_notification> pending_notifications_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    waitable_event notify_event_{false, waitable_event::type::auto_reset};
#else
    struct ::sigaction old_actions_[64]{};
    bool installed_[64]{};
#endif

    thread signal_thread_;
    thread timeout_thread_;
};

system_signal::system_signal() :
impl_(make_unique<impl>()) {
    initialize();
}

system_signal::~system_signal() {
    stop_monitoring();
    cleanup();
}

system_signal& system_signal::instance() {
    static system_signal instance;
    return instance;
}

void system_signal::add_guard_ref() {
    if (impl_->guard_count_++ == 0) {
        start_monitoring();
    }
}

void system_signal::remove_guard_ref() noexcept {
    if (--impl_->guard_count_ == 0) {
        stop_monitoring();
    }
}

void system_signal::install_signal_handler(const int native_signal) {
#ifndef NEFORCE_PLATFORM_WINDOWS
    if (native_signal <= 0 || native_signal >= 64 || native_signal == SIGKILL || native_signal == SIGSTOP) {
        return;
    }
    if (impl_->installed_[native_signal]) {
        return;
    }
    struct ::sigaction action;
    memory_zero(&action);
    action.sa_sigaction = posix_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESTART;
    if (::sigaction(native_signal, &action, &impl_->old_actions_[native_signal]) == 0) {
        impl_->installed_[native_signal] = true;
        const auto& previous = impl_->old_actions_[native_signal];
        if ((previous.sa_flags & SA_SIGINFO) != 0 && previous.sa_sigaction != nullptr) {
            g_chain_previous[native_signal] = 2;
        } else if (previous.sa_handler == SIG_IGN) {
            g_chain_previous[native_signal] = 1;
        } else if (previous.sa_handler != nullptr && previous.sa_handler != SIG_DFL) {
            g_chain_previous[native_signal] = 2;
        } else {
            g_chain_previous[native_signal] = 0;
        }
    }
#else
    static_cast<void>(native_signal);
#endif
}

void system_signal::restore_signal_handler(const int native_signal) {
#ifndef NEFORCE_PLATFORM_WINDOWS
    if (native_signal > 0 && native_signal < 64 && impl_->installed_[native_signal]) {
        g_chain_previous[native_signal] = 0;
        ::sigaction(native_signal, &impl_->old_actions_[native_signal], nullptr);
        impl_->installed_[native_signal] = false;
    }
#else
    static_cast<void>(native_signal);
#endif
}

void system_signal::restore_all_signal_handlers() {
#ifndef NEFORCE_PLATFORM_WINDOWS
    for (int sig = 1; sig < 64; ++sig) {
        if (impl_->installed_[sig]) {
            g_chain_previous[sig] = 0;
            ::sigaction(sig, &impl_->old_actions_[sig], nullptr);
            impl_->installed_[sig] = false;
        }
    }
#endif
}

void system_signal::initialize() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    g_notify_event = &impl_->notify_event_;
    ::SetConsoleCtrlHandler(windows_handler, TRUE);
#else
    g_signal_pipe = &get_signal_pipe();
    g_previous_actions = impl_->old_actions_;
    const int read_fd = g_signal_pipe->native_read_handle();
    const int rflags = ::fcntl(read_fd, F_GETFL, 0);
    ::fcntl(read_fd, F_SETFL, rflags | O_NONBLOCK);

    const int write_fd = g_signal_pipe->native_write_handle();
    const int wflags = ::fcntl(write_fd, F_GETFL, 0);
    ::fcntl(write_fd, F_SETFL, wflags | O_NONBLOCK);

#endif
}

void system_signal::cleanup() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCtrlHandler(windows_handler, FALSE);
    g_notify_event = nullptr;
#else
    restore_all_signal_handlers();
#endif
}

void system_signal::subscribe(const signal sig, notification_handler handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(impl_->mutex_);
    impl_->notification_handlers_[sig] = move(handler);
    install_signal_handler(signal_number(sig));
}

void system_signal::subscribe(const vector<signal>& signals, const notification_handler& handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(impl_->mutex_);
    for (const auto sig: signals) {
        impl_->notification_handlers_[sig] = handler;
        install_signal_handler(signal_number(sig));
    }
}

void system_signal::subscribe(const event event, notification_handler handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(impl_->mutex_);
    impl_->event_handlers_[event] = move(handler);
}

void system_signal::subscribe(const vector<event>& events, const notification_handler& handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(impl_->mutex_);
    for (const auto event: events) {
        impl_->event_handlers_[event] = handler;
    }
}

void system_signal::unsubscribe(const signal sig) {
    bool restore = false;
    {
        lock<mutex> lk(impl_->mutex_);
        impl_->notification_handlers_.erase(sig);
        restore = impl_->notification_handlers_.find(sig) == impl_->notification_handlers_.end();
    }
    if (restore) {
        restore_signal_handler(signal_number(sig));
    }
}

void system_signal::unsubscribe(const event event) {
    lock<mutex> lk(impl_->mutex_);
    impl_->event_handlers_.erase(event);
}

optional<system_signal::notification> system_signal::wait_for_notification(const int timeout_ms) {
    const auto result = wait_for_notification_internal(timeout_ms);
    if (!result.has_value) {
        return none;
    }
    return result.value;
}

void system_signal::enqueue_signal(const signal sig, void* context) {
    lock<mutex> lk(impl_->mutex_);
    enqueue_notification_nolock(notification::from_signal(sig), context);
}

void system_signal::enqueue_event(const event event, void* context) {
    lock<mutex> lk(impl_->mutex_);
    enqueue_notification_nolock(notification::from_event(event), context);
}

void system_signal::set_force_exit_timeout(const int timeout_ms) { impl_->force_exit_timeout_ = timeout_ms; }

void system_signal::start_monitoring() {
    if (impl_->running_) {
        return;
    }

    if (impl_->signal_thread_.joinable()) {
        impl_->signal_thread_.join();
    }
    if (impl_->timeout_thread_.joinable()) {
        impl_->timeout_thread_.join();
    }

    impl_->running_ = true;
    impl_->force_exit_ = false;

    impl_->signal_thread_ = thread(&system_signal::signal_thread_func, this);
    impl_->timeout_thread_ = thread(&system_signal::timeout_monitor_thread, this);
}

void system_signal::stop_monitoring() noexcept {
    try {
        if (impl_->guard_count_.load() > 0) {
            return;
        }

        if (!impl_->running_) {
            if (impl_->signal_thread_.joinable()) {
                impl_->signal_thread_.join();
            }
            if (impl_->timeout_thread_.joinable()) {
                impl_->timeout_thread_.join();
            }
            return;
        }

        impl_->force_exit_ = true;
        impl_->running_ = false;

#ifdef NEFORCE_PLATFORM_WINDOWS
        impl_->notify_event_.set();
#else
        if (g_signal_pipe != nullptr) {
            constexpr int wakeup = -1;
            ::write(g_signal_pipe->native_write_handle(), &wakeup, sizeof(wakeup));
        }
#endif
        impl_->cv_.notify_all();

        if (impl_->signal_thread_.joinable()) {
            impl_->signal_thread_.join();
        }
        if (impl_->timeout_thread_.joinable()) {
            impl_->timeout_thread_.join();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const exception& e) {
        NEFORCE_REPORT_EXCEPTION(e);
    }
}

void system_signal::reset_force() {
    stop_monitoring();
    impl_->guard_count_ = 0;
    lock<mutex> lk(impl_->mutex_);
    for (auto it = impl_->event_handlers_.begin(); it != impl_->event_handlers_.end();) {
        if (it->first == event::custom_1 || it->first == event::custom_2 || it->first == event::force_exit) {
            it = impl_->event_handlers_.erase(it);
        } else {
            ++it;
        }
    }
}

bool system_signal::is_running() const { return impl_->running_; }

void system_signal::signal_thread_func() {
    while (impl_->running_) {
        bool has_signals = false;

#ifdef NEFORCE_PLATFORM_WINDOWS
        impl_->notify_event_.wait(50);
        if (!impl_->running_) {
            break;
        }

        while (impl_->running_) {
            const notification_result result = wait_for_notification_internal(0);
            if (!result.has_value) {
                break;
            }
            has_signals = true;
            process_notification(result.value, result.context);
            if (result.value.is_event() && result.value.event_value() == event::force_exit) {
                return;
            }
        }
#else
        const int read_fd = g_signal_pipe->native_read_handle();
        ::fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(read_fd, &rfds);
        ::timeval tv{0, 50000};

        int ready = ::select(read_fd + 1, &rfds, nullptr, nullptr, &tv);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (!impl_->running_) {
            break;
        }

        while (impl_->running_ && ready > 0 && FD_ISSET(read_fd, &rfds)) {
            int sig = 0;
            const ssize_t n = ::read(read_fd, &sig, sizeof(sig));

            if (n == sizeof(sig)) {
                if (sig == -2) {
                    while (impl_->running_) {
                        const notification_result result = wait_for_notification_internal(0);
                        if (!result.has_value) {
                            break;
                        }
                        has_signals = true;
                        process_notification(result.value, result.context);
                        if (result.value.is_event() && result.value.event_value() == event::force_exit) {
                            return;
                        }
                    }
                } else if (sig > 0) {
                    const auto value = signal_from_number(sig);
                    if (value.has_value()) {
                        has_signals = true;
                        process_notification(notification::from_signal(*value), nullptr);
                    }
                }
            }

            FD_ZERO(&rfds);
            FD_SET(read_fd, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            ready = ::select(read_fd + 1, &rfds, nullptr, nullptr, &tv);
        }
#endif

        if (!has_signals && impl_->running_) {
            notification_handler h;
            {
                lock<mutex> lk(impl_->mutex_);
                auto it = impl_->event_handlers_.find(event::timeout);
                if (it != impl_->event_handlers_.end() && it->second) {
                    h = it->second;
                }
            }
            if (h) {
                h(notification::from_event(event::timeout), nullptr);
            }
        }
    }
}

void system_signal::timeout_monitor_thread() {
    while (impl_->running_ && !impl_->force_exit_) {
        this_thread::sleep_for(milliseconds(100));
        lock<mutex> lk(impl_->mutex_);
        if (impl_->pending_notifications_.empty()) {
            continue;
        }

        int timeout = impl_->force_exit_timeout_.load();
        auto now = steady_clock::now();

        auto it = remove_if(impl_->pending_notifications_.begin(), impl_->pending_notifications_.end(),
                            [timeout, now](const pending_notification& ps) -> bool {
                                const auto age = time_cast<milliseconds>(now - ps.timestamp).count();
                                return age > timeout;
                            });
        if (it != impl_->pending_notifications_.end()) {
            printcln(color::yellow(), "Removing ", impl_->pending_notifications_.end() - it, " stale signal(s)");
            impl_->pending_notifications_.erase(it, impl_->pending_notifications_.end());
            impl_->cv_.notify_all();
        }
    }
}

void system_signal::process_notification(const notification& value, void* context) {
    notification_handler h;
    {
        lock<mutex> lk(impl_->mutex_);
        if (value.is_signal()) {
            const auto it = impl_->notification_handlers_.find(value.signal_value());
            if (it != impl_->notification_handlers_.end()) {
                h = it->second;
            }
        } else {
            const auto it = impl_->event_handlers_.find(value.event_value());
            if (it != impl_->event_handlers_.end()) {
                h = it->second;
            }
        }
    }

    if (h) {
        const bool should_exit = !h(value, context);
        if (should_exit && value.is_event() && value.event_value() == event::force_exit) {
            terminate();
        }
        return;
    }

    if (value.is_signal()) {
        switch (value.signal_value()) {
            case signal::interrupt:
            case signal::terminate:
            case signal::hangup: {
                printcln(color::blue(), "Received termination signal, exiting...");
                impl_->running_ = false;
                break;
            }
            case signal::segment_fault:
            case signal::illegal_instr:
            case signal::floating_point:
            case signal::bus_error: {
#ifdef NEFORCE_PLATFORM_WINDOWS
                printcln(color::red(), "Critical error signal received: ", signal_name(value.signal_value()));
                trigger_force_exit();
#else
                printcln(color::red(), "Critical error detected, aborting: ", signal_name(value.signal_value()));
                abort();
#endif
            }
            case signal::abort: {
                printcln(color::red(), "Abort signal received.");
#ifdef NEFORCE_PLATFORM_WINDOWS
                trigger_force_exit();
#else
                abort();
#endif
            }
            case signal::pipe_broken:
            case signal::alarm:
            case signal::user1:
            case signal::user2:
                break;
            default:
                break;
        }
        return;
    }

    switch (value.event_value()) {
        case event::ctrl_break:
        case event::close:
        case event::logoff:
        case event::shutdown:
            printcln(color::blue(), "Received termination event, exiting...");
            impl_->running_ = false;
            break;
        case event::timeout:
            printcln(color::yellow(), "Timeout event received.");
            break;
        case event::custom_1:
        case event::custom_2:
            printcln(color::cyan(), "Custom event received: ", static_cast<int>(value.event_value()));
            break;
        case event::force_exit:
            printcln(color::red(), "Force exit triggered.");
            impl_->force_exit_ = true;
            terminate();
            break;
    }
}

void system_signal::trigger_force_exit() {
    bool expected = false;
    if (impl_->force_exit_.compare_exchange_strong(expected, true)) {
        impl_->running_ = false;
        impl_->cv_.notify_all();
#ifdef NEFORCE_PLATFORM_WINDOWS
        impl_->notify_event_.set();
#endif
    }
}

system_signal::notification_result system_signal::wait_for_notification_internal(const int timeout_ms) {
    unique_lock<mutex> lk(impl_->mutex_);
    if (timeout_ms >= 0) {
        const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
        if (!impl_->cv_.wait_until(lk, deadline,
                                   [this]() { return !impl_->pending_notifications_.empty() || !impl_->running_; })) {
            return notification_result{};
        }
    } else {
        impl_->cv_.wait(lk, [this]() { return !impl_->pending_notifications_.empty() || !impl_->running_; });
    }

    if (!impl_->pending_notifications_.empty()) {
        const pending_notification ps = impl_->pending_notifications_.front();
        impl_->pending_notifications_.erase(impl_->pending_notifications_.begin());
        return notification_result{true, ps.value, ps.context};
    }
    return notification_result{};
}

optional<system_signal::notification> system_signal::notification_from_native(const int native_signal) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    switch (native_signal) {
        case CTRL_C_EVENT:
            return notification::from_signal(signal::interrupt);
        case CTRL_BREAK_EVENT:
            return notification::from_event(event::ctrl_break);
        case CTRL_CLOSE_EVENT:
            return notification::from_event(event::close);
        case CTRL_LOGOFF_EVENT:
            return notification::from_event(event::logoff);
        case CTRL_SHUTDOWN_EVENT:
            return notification::from_event(event::shutdown);
        default: {
            const auto value = signal_from_number(native_signal);
            if (value.has_value()) {
                return notification::from_signal(*value);
            }
            return none;
        }
    }
#else
    const auto value = signal_from_number(native_signal);
    if (value.has_value()) {
        return notification::from_signal(*value);
    }
    return none;
#endif
}

int system_signal::notification_to_native(const notification& value) noexcept {
    if (value.is_signal()) {
        return signal_number(value.signal_value());
    }
    switch (value.event_value()) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        case event::ctrl_break:
            return CTRL_BREAK_EVENT;
        case event::close:
            return CTRL_CLOSE_EVENT;
        case event::logoff:
            return CTRL_LOGOFF_EVENT;
        case event::shutdown:
            return CTRL_SHUTDOWN_EVENT;
#else
        case event::ctrl_break:
        case event::close:
        case event::logoff:
        case event::shutdown:
#endif
        case event::timeout:
        case event::custom_1:
        case event::custom_2:
        case event::force_exit:
        default:
            return -1;
    }
}

bool system_signal::is_native_notification(const notification& value) noexcept {
    return notification_to_native(value) > 0;
}

string_view system_signal::event_name(const event e) noexcept {
    switch (e) {
        case event::ctrl_break:
            return "BREAK";
        case event::close:
            return "CLOSE";
        case event::logoff:
            return "LOGOFF";
        case event::shutdown:
            return "SHUTDOWN";
        case event::timeout:
            return "TIMEOUT";
        case event::custom_1:
            return "CUSTOM1";
        case event::custom_2:
            return "CUSTOM2";
        case event::force_exit:
            return "FORCE";
        default:
            return "UNKNOWN";
    }
}

string_view system_signal::notification_name(const notification& value) noexcept {
    return value.is_signal() ? signal_name(value.signal_value()) : event_name(value.event_value());
}

bool system_signal::send_native(const int process_id, const int native_signal) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (native_signal != 9 && native_signal != 15) {
        return false;
    }
    const ::HANDLE handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<::DWORD>(process_id));
    if (handle == nullptr) {
        return false;
    }
    const bool sent = ::TerminateProcess(handle, 1) != 0;
    ::CloseHandle(handle);
    return sent;
#else
    if (process_id <= 0 || native_signal <= 0) {
        return false;
    }
#    ifdef SYS_pidfd_open
    const int descriptor = static_cast<int>(::syscall(SYS_pidfd_open, process_id, 0));
    if (descriptor >= 0) {
        const bool sent = ::syscall(SYS_pidfd_send_signal, descriptor, native_signal, nullptr, 0) == 0;
        ::close(descriptor);
        if (sent) {
            return true;
        }
    }
#    endif
    return ::kill(process_id, native_signal) == 0;
#endif
}

void system_signal::set_chain_existing_handlers(const bool enabled) noexcept { g_chain_enabled = enabled ? 1 : 0; }

bool system_signal::chain_existing_handlers() noexcept { return g_chain_enabled != 0; }

bool system_signal::can_send(const signal sig) noexcept { return signal_number(sig) > 0; }

bool system_signal::send_to_process(const int process_id, const signal sig) noexcept {
    if (!can_send(sig)) {
        return false;
    }
    return send_native(process_id, signal_number(sig));
}

string_view system_signal::signal_name(const signal sig) noexcept {
    switch (sig) {
        case signal::interrupt:
            return "INT";
        case signal::terminate:
            return "TERM";
        case signal::abort:
            return "ABRT";
        case signal::illegal_instr:
            return "ILL";
        case signal::floating_point:
            return "FPE";
        case signal::segment_fault:
            return "SEGV";
        case signal::bus_error:
            return "BUS";
        case signal::pipe_broken:
            return "PIPE";
        case signal::alarm:
            return "ALRM";
        case signal::hangup:
            return "HUP";
        case signal::user1:
            return "USR1";
        case signal::user2:
            return "USR2";
        case signal::kill:
            return "KILL";
        case signal::quit:
            return "QUIT";
        case signal::trap:
            return "TRAP";
        case signal::child:
            return "CHLD";
        case signal::resume:
            return "CONT";
        case signal::stop:
            return "STOP";
        case signal::terminal_stop:
            return "TSTP";
        case signal::terminal_input:
            return "TTIN";
        case signal::terminal_output:
            return "TTOU";
        case signal::urgent:
            return "URG";
        case signal::cpu_limit:
            return "XCPU";
        case signal::file_limit:
            return "XFSZ";
        case signal::virtual_alarm:
            return "VTALRM";
        case signal::profiling:
            return "PROF";
        case signal::window_change:
            return "WINCH";
        case signal::io:
            return "IO";
        case signal::power:
            return "PWR";
        case signal::bad_syscall:
            return "SYS";
        default:
            return "UNKNOWN";
    }
}

int system_signal::signal_number(const signal sig) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (sig == signal::terminate) {
        return 15;
    }
    if (sig == signal::kill) {
        return 9;
    }
    return 0;
#else
    switch (sig) {
        case signal::interrupt:
            return SIGINT;
        case signal::terminate:
            return SIGTERM;
        case signal::abort:
            return SIGABRT;
        case signal::illegal_instr:
            return SIGILL;
        case signal::floating_point:
            return SIGFPE;
        case signal::segment_fault:
            return SIGSEGV;
        case signal::bus_error:
            return SIGBUS;
        case signal::pipe_broken:
            return SIGPIPE;
        case signal::alarm:
            return SIGALRM;
        case signal::hangup:
            return SIGHUP;
        case signal::user1:
            return SIGUSR1;
        case signal::user2:
            return SIGUSR2;
        case signal::kill:
            return SIGKILL;
        case signal::quit:
            return SIGQUIT;
        case signal::trap:
            return SIGTRAP;
        case signal::child:
            return SIGCHLD;
        case signal::resume:
            return SIGCONT;
        case signal::stop:
            return SIGSTOP;
        case signal::terminal_stop:
            return SIGTSTP;
        case signal::terminal_input:
            return SIGTTIN;
        case signal::terminal_output:
            return SIGTTOU;
        case signal::urgent:
            return SIGURG;
        case signal::cpu_limit:
            return SIGXCPU;
        case signal::file_limit:
            return SIGXFSZ;
        case signal::virtual_alarm:
            return SIGVTALRM;
        case signal::profiling:
            return SIGPROF;
        case signal::window_change:
            return SIGWINCH;
        case signal::io:
            return SIGIO;
#    ifdef SIGPWR
        case signal::power:
            return SIGPWR;
#    endif
#    ifdef SIGSYS
        case signal::bad_syscall:
            return SIGSYS;
#    endif
        default:
            return 0;
    }
#endif
}

optional<system_signal::signal> system_signal::signal_from_number(const int native_signal) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (native_signal == 15) {
        return signal::terminate;
    }
    if (native_signal == 9) {
        return signal::kill;
    }
    return none;
#else
    switch (native_signal) {
        case SIGINT:
            return signal::interrupt;
        case SIGTERM:
            return signal::terminate;
        case SIGABRT:
            return signal::abort;
        case SIGILL:
            return signal::illegal_instr;
        case SIGFPE:
            return signal::floating_point;
        case SIGSEGV:
            return signal::segment_fault;
        case SIGBUS:
            return signal::bus_error;
        case SIGPIPE:
            return signal::pipe_broken;
        case SIGALRM:
            return signal::alarm;
        case SIGHUP:
            return signal::hangup;
        case SIGUSR1:
            return signal::user1;
        case SIGUSR2:
            return signal::user2;
        case SIGKILL:
            return signal::kill;
        case SIGQUIT:
            return signal::quit;
        case SIGTRAP:
            return signal::trap;
        case SIGCHLD:
            return signal::child;
        case SIGCONT:
            return signal::resume;
        case SIGSTOP:
            return signal::stop;
        case SIGTSTP:
            return signal::terminal_stop;
        case SIGTTIN:
            return signal::terminal_input;
        case SIGTTOU:
            return signal::terminal_output;
        case SIGURG:
            return signal::urgent;
        case SIGXCPU:
            return signal::cpu_limit;
        case SIGXFSZ:
            return signal::file_limit;
        case SIGVTALRM:
            return signal::virtual_alarm;
        case SIGPROF:
            return signal::profiling;
        case SIGWINCH:
            return signal::window_change;
        case SIGIO:
            return signal::io;
#    ifdef SIGPWR
        case SIGPWR:
            return signal::power;
#    endif
#    ifdef SIGSYS
        case SIGSYS:
            return signal::bad_syscall;
#    endif
        default:
            return none;
    }
#endif
}

void system_signal::enqueue_notification_nolock(const notification& value, void* context) {
    impl_->pending_notifications_.emplace_back(value, context, steady_clock::now());
    impl_->cv_.notify_all();
#ifdef NEFORCE_PLATFORM_WINDOWS
    impl_->notify_event_.set();
#else
    constexpr int wakeup = -2;
    if (g_signal_pipe != nullptr) {
        ::write(g_signal_pipe->native_write_handle(), &wakeup, sizeof(wakeup));
    }
#endif
}

bool system_signal::block_signals(const vector<signal>& signals_to_block) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    sigemptyset(&mask);
    for (auto ev: signals_to_block) {
        const int sig = signal_number(ev);
        if (sig > 0 && sig < NSIG) {
            sigaddset(&mask, sig);
        }
    }
    return ::pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0;
#else
    (void) signals_to_block;
    return true;
#endif
}

bool system_signal::unblock_signals(const vector<signal>& signals_to_unblock) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    sigemptyset(&mask);
    for (auto ev: signals_to_unblock) {
        const int sig = signal_number(ev);
        if (sig > 0 && sig < NSIG) {
            sigaddset(&mask, sig);
        }
    }
    return ::pthread_sigmask(SIG_UNBLOCK, &mask, nullptr) == 0;
#else
    (void) signals_to_unblock;
    return true;
#endif
}

NEFORCE_END_NAMESPACE__
