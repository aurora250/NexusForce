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
#include <NeForce/core/system/system_event.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windef.h>
#    include <WinBase.h>
#    include <consoleapi2.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/system/pipe.hpp>
#    include <cstdlib>
#    include <unistd.h>
#    include <fcntl.h>
#    include <sys/select.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    system_event* g_notify_event = nullptr;

    system_signal::event map_win_event(unsigned long event_id) {
        switch (event_id) {
            case CTRL_C_EVENT:
                return system_signal::event::INTERRUPT;
            case CTRL_BREAK_EVENT:
                return system_signal::event::CTRL_BREAK;
            case CTRL_CLOSE_EVENT:
                return system_signal::event::CLOSE;
            case CTRL_LOGOFF_EVENT:
                return system_signal::event::LOGOFF;
            case CTRL_SHUTDOWN_EVENT:
                return system_signal::event::SHUTDOWN;
            default:
                return static_cast<system_signal::event>(event_id);
        }
    }

    ::BOOL __stdcall windows_handler(unsigned long event_id) {
        if (g_notify_event != nullptr) {
            system_signal::instance().send_signal(map_win_event(event_id));
        }
        return TRUE;
    }

#else
    pipe& get_signal_pipe() {
        static pipe p(false);
        return p;
    }

    pipe* g_signal_pipe = nullptr;

    void posix_handler(const int sig) {
        const int saved_errno = errno;
        if (g_signal_pipe != nullptr) {
            ::write(g_signal_pipe->native_write_handle(), &sig, sizeof(sig));
        }
        errno = saved_errno;
    }

#endif

    thread_local auto g_current_signal =
#ifdef NEFORCE_PLATFORM_WINDOWS
            static_cast<system_signal::event>(CTRL_C_EVENT);
#else
            static_cast<system_signal::event>(SIGTERM);
#endif

    thread_local void* g_signal_context = nullptr;

    void send_platform_signal(const system_signal::event e) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        const int native = system_signal::to_native(e);
        if (native >= 0) {
            ::GenerateConsoleCtrlEvent(static_cast<::DWORD>(native), 0);
        }
#else
        const int native = system_signal::to_native(e);
        if (native > 0) {
            ::raise(native);
        }
#endif
    }
} // namespace


struct system_signal::impl {
    atomic<bool> running_{false};
    atomic<bool> force_exit_{false};
    atomic<int> force_exit_timeout_{5000};
    atomic<int> guard_count_{0};

    mutex mutex_;
    condition_variable cv_;

    flat_unordered_map<event, signal_handler> handlers_;
    vector<pending_signal> pending_signals_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    system_event notify_event_{false, system_event::type::auto_reset};
#else
    struct ::sigaction old_actions_[64]{};
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

void system_signal::initialize() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    g_notify_event = &impl_->notify_event_;
    ::SetConsoleCtrlHandler(windows_handler, TRUE);
    impl_->handlers_[event::interrupt] = nullptr;
    impl_->handlers_[event::ctrl_break] = nullptr;
    impl_->handlers_[event::close] = nullptr;
    impl_->handlers_[event::logoff] = nullptr;
    impl_->handlers_[event::shutdown] = nullptr;
#else
    g_signal_pipe = &get_signal_pipe();
    const int read_fd = g_signal_pipe->native_read_handle();
    const int rflags = ::fcntl(read_fd, F_GETFL, 0);
    ::fcntl(read_fd, F_SETFL, rflags | O_NONBLOCK);

    const int write_fd = g_signal_pipe->native_write_handle();
    const int wflags = ::fcntl(write_fd, F_GETFL, 0);
    ::fcntl(write_fd, F_SETFL, wflags | O_NONBLOCK);

    struct ::sigaction sa;
    memory_zero(&sa);
    sa.sa_handler = posix_handler;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    constexpr int sigs[] = {SIGINT, SIGTERM, SIGABRT, SIGILL, SIGFPE,  SIGSEGV,
                            SIGBUS, SIGPIPE, SIGALRM, SIGHUP, SIGUSR1, SIGUSR2};
    for (const int sig: sigs) {
        ::sigaction(sig, &sa, &impl_->old_actions_[sig]);
    }
#endif
}

void system_signal::cleanup() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCtrlHandler(windows_handler, FALSE);
    g_notify_event = nullptr;
#else
    for (int sig = 1; sig < 64; ++sig) {
        if (impl_->old_actions_[sig].sa_handler != nullptr || impl_->old_actions_[sig].sa_flags != 0) {
            ::sigaction(sig, &impl_->old_actions_[sig], nullptr);
        }
    }
#endif
}

void system_signal::register_handler(const event event, signal_handler handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(impl_->mutex_);
    impl_->handlers_[event] = move(handler);
}

void system_signal::register_handlers(const vector<event>& events, const signal_handler& handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(impl_->mutex_);
    for (auto e: events) {
        impl_->handlers_[e] = handler;
    }
}

void system_signal::remove_handler(const event event) {
    lock<mutex> lk(impl_->mutex_);
    impl_->handlers_.erase(event);
}

system_signal::event system_signal::wait_for_signal(const int timeout_ms) {
    return wait_for_signal_internal(timeout_ms).event;
}

void system_signal::send_signal(const event event, void* context) {
    lock<mutex> lk(impl_->mutex_);
    send_signal_nolock(event, context);
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

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetThreadPriority(impl_->signal_thread_.native_handle(), THREAD_PRIORITY_HIGHEST);
#else
    ::sched_param param;
    param.sched_priority = ::sched_get_priority_max(SCHED_FIFO);
    ::pthread_setschedparam(impl_->signal_thread_.native_handle(), SCHED_FIFO, &param);
#endif
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
    for (auto it = impl_->handlers_.begin(); it != impl_->handlers_.end();) {
        if (it->first >= event::custom_1) {
            it = impl_->handlers_.erase(it);
        } else {
            ++it;
        }
    }
}

bool system_signal::is_running() const { return impl_->running_; }

void system_signal::signal_thread_func() {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sched_param param{};
    param.sched_priority = 10;
    ::pthread_setschedparam(::pthread_self(), SCHED_FIFO, &param);
#endif

    while (impl_->running_) {
        bool has_signals = false;

#ifdef NEFORCE_PLATFORM_WINDOWS
        impl_->notify_event_.wait(50);
        if (!impl_->running_) {
            break;
        }

        while (impl_->running_) {
            const signal_result result = wait_for_signal_internal(0);
            if (result.event == event::timeout) {
                break;
            }
            has_signals = true;
            process_signal(result.event, result.context);
            if (result.event == event::force_exit) {
                return;
            }
        }
#else
        pipe& sig_pipe = get_signal_pipe();
        const int read_fd = sig_pipe.native_read_handle();
        ::fd_set rfds{};
        FD_ZERO(&rfds);
        FD_SET(read_fd, &rfds);
        ::timeval tv{};
        tv.tv_sec = 1;
        tv.tv_usec = 0;
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
                        const signal_result result = wait_for_signal_internal(0);
                        if (result.event == event::timeout) {
                            break;
                        }
                        has_signals = true;
                        process_signal(result.event, result.context);
                        if (result.event == event::force_exit) {
                            return;
                        }
                    }
                } else if (sig >= 0) {
                    const auto ev = static_cast<system_signal::event>(sig);
                    has_signals = true;
                    process_signal(ev, nullptr);
                    if (ev == system_signal::event::force_exit) {
                        return;
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
            signal_handler h;
            {
                lock<mutex> lk(impl_->mutex_);
                auto it = impl_->handlers_.find(event::timeout);
                if (it != impl_->handlers_.end() && it->second) {
                    h = it->second;
                }
            }
            if (h) {
                h(event::timeout, nullptr);
            }
        }
    }
}

void system_signal::timeout_monitor_thread() {
    while (impl_->running_ && !impl_->force_exit_) {
        this_thread::sleep_for(milliseconds(100));
        lock<mutex> lk(impl_->mutex_);
        if (impl_->pending_signals_.empty()) {
            continue;
        }

        int timeout = impl_->force_exit_timeout_.load();
        auto now = steady_clock::now();

        auto it = remove_if(impl_->pending_signals_.begin(), impl_->pending_signals_.end(),
                            [timeout, now](const pending_signal& ps) -> bool {
                                const auto age = time_cast<milliseconds>(now - ps.timestamp).count();
                                return age > timeout;
                            });
        if (it != impl_->pending_signals_.end()) {
            printcln(color::yellow(), "Removing ", impl_->pending_signals_.end() - it, " stale signal(s)");
            impl_->pending_signals_.erase(it, impl_->pending_signals_.end());
            impl_->cv_.notify_all();
        }
    }
}

void system_signal::process_signal(const event event, void* context) {
    signal_handler h;
    {
        lock<mutex> lk(impl_->mutex_);
        const auto it = impl_->handlers_.find(event);
        if (it != impl_->handlers_.end()) {
            h = it->second;
        }
    }

    if (h) {
        g_current_signal = event;
        g_signal_context = context;
        const bool should_exit = !h(event, context);
        g_current_signal =
#ifdef NEFORCE_PLATFORM_WINDOWS
                static_cast<system_signal::event>(CTRL_C_EVENT);
#else
                static_cast<system_signal::event>(SIGTERM);
#endif
        g_signal_context = nullptr;
        if (should_exit && event == event::force_exit) {
            terminate();
        }
        return;
    }

    switch (event) {
        case event::interrupt:
        case event::terminate:
        case event::hangup:
        case event::ctrl_break:
        case event::close:
        case event::logoff:
        case event::shutdown: {
            printcln(color::blue(), "Received termination signal, exiting...");
            impl_->running_ = false;
            break;
        }
        case event::segment_fault:
        case event::illegal_instr:
        case event::floating_point:
        case event::bus_error: {
#ifdef NEFORCE_PLATFORM_WINDOWS
            printcln(color::red(), "Critical error signal received: ", static_cast<int>(event));
            trigger_force_exit();
#else
            printcln(color::red(), "Critical error detected, aborting: ", static_cast<int>(event));
            abort();
#endif
        }
        case event::abort: {
            printcln(color::red(), "Abort signal received.");
#ifdef NEFORCE_PLATFORM_WINDOWS
            trigger_force_exit();
#else
            abort();
#endif
        }
        case event::pipe_broken: {
            printcln(color::yellow(), "Broken pipe signal received, ignoring.");
            break;
        }
        case event::alarm: {
            printcln(color::yellow(), "Alarm signal received.");
            break;
        }
        case event::user1: {
            printcln(color::cyan(), "User signal 1 received.");
            break;
        }
        case event::user2: {
            printcln(color::cyan(), "User signal 2 received.");
            break;
        }
        case event::timeout: {
            printcln(color::yellow(), "Timeout signal received.");
            break;
        }
        case event::custom_1:
        case event::custom_2: {
            printcln(color::cyan(), "Custom event received: ", static_cast<int>(event));
            break;
        }
        case event::force_exit: {
            printcln(color::red(), "Force exit triggered.");
            impl_->force_exit_ = true;
            terminate();
        }
        default: {
            printcln(color::yellow(), "Unhandled signal: ", static_cast<int>(event));
            break;
        }
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

system_signal::signal_result system_signal::wait_for_signal_internal(const int timeout_ms) {
    unique_lock<mutex> lk(impl_->mutex_);
    if (timeout_ms >= 0) {
        const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
        if (!impl_->cv_.wait_until(lk, deadline,
                                   [this]() { return !impl_->pending_signals_.empty() || !impl_->running_; })) {
            return signal_result{event::timeout, nullptr};
        }
    } else {
        impl_->cv_.wait(lk, [this]() { return !impl_->pending_signals_.empty() || !impl_->running_; });
    }

    if (!impl_->pending_signals_.empty()) {
        const pending_signal ps = impl_->pending_signals_.front();
        impl_->pending_signals_.erase(impl_->pending_signals_.begin());
        return signal_result{ps.signal_event, ps.context};
    }
    return signal_result{event::timeout, nullptr};
}

system_signal::event system_signal::from_native(int native_signal) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    switch (native_signal) {
        case CTRL_C_EVENT:
            return event::interrupt;
        case CTRL_BREAK_EVENT:
            return event::ctrl_break;
        case CTRL_CLOSE_EVENT:
            return event::close;
        case CTRL_LOGOFF_EVENT:
            return event::logoff;
        case CTRL_SHUTDOWN_EVENT:
            return event::shutdown;
        default:
            return static_cast<event>(native_signal);
    }
#else
    switch (native_signal) {
        case SIGINT:
            return event::interrupt;
        case SIGTERM:
            return event::terminate;
        case SIGABRT:
            return event::abort;
        case SIGILL:
            return event::illegal_instr;
        case SIGFPE:
            return event::floating_point;
        case SIGSEGV:
            return event::segment_fault;
        case SIGBUS:
            return event::bus_error;
        case SIGPIPE:
            return event::pipe_broken;
        case SIGALRM:
            return event::alarm;
        case SIGHUP:
            return event::hangup;
        case SIGUSR1:
            return event::user1;
        case SIGUSR2:
            return event::user2;
        default:
            return static_cast<event>(native_signal);
    }
#endif
}

int system_signal::to_native(const event e) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    switch (e) {
        case event::interrupt:
            return CTRL_C_EVENT;
        case event::ctrl_break:
            return CTRL_BREAK_EVENT;
        case event::close:
            return CTRL_CLOSE_EVENT;
        case event::logoff:
            return CTRL_LOGOFF_EVENT;
        case event::shutdown:
            return CTRL_SHUTDOWN_EVENT;
        default:
            return -1;
    }
#else
    switch (e) {
        case event::interrupt:
            return SIGINT;
        case event::terminate:
            return SIGTERM;
        case event::abort:
            return SIGABRT;
        case event::illegal_instr:
            return SIGILL;
        case event::floating_point:
            return SIGFPE;
        case event::segment_fault:
            return SIGSEGV;
        case event::bus_error:
            return SIGBUS;
        case event::pipe_broken:
            return SIGPIPE;
        case event::alarm:
            return SIGALRM;
        case event::hangup:
            return SIGHUP;
        case event::user1:
            return SIGUSR1;
        case event::user2:
            return SIGUSR2;
        default:
            return -1;
    }
#endif
}

void system_signal::send_signal_nolock(event event, void* context) {
    send_platform_signal(event);

    impl_->pending_signals_.emplace_back(event, context, steady_clock::now());
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

bool system_signal::block_signals(const vector<event>& signals_to_block) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    ::sigemptyset(&mask);
    for (auto ev: signals_to_block) {
        const int sig = static_cast<int>(ev);
        if (sig > 0 && sig < NSIG) {
            ::sigaddset(&mask, sig);
        }
    }
    return ::pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0;
#else
    (void) signals_to_block;
    return true;
#endif
}

bool system_signal::unblock_signals(const vector<event>& signals_to_unblock) {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    ::sigemptyset(&mask);
    for (auto ev: signals_to_unblock) {
        const int sig = static_cast<int>(ev);
        if (sig > 0 && sig < NSIG) {
            ::sigaddset(&mask, sig);
        }
    }
    return ::pthread_sigmask(SIG_UNBLOCK, &mask, nullptr) == 0;
#else
    (void) signals_to_unblock;
    return true;
#endif
}

NEFORCE_END_NAMESPACE__
