#include <NeForce/core/algorithm/remove.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/signal.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windef.h>
#    include <WinBase.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
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

    system_signal_manager::event map_win_event(unsigned long event_id) {
        switch (event_id) {
            case CTRL_C_EVENT:
                return system_signal_manager::event::INTERRUPT;
            case CTRL_BREAK_EVENT:
                return system_signal_manager::event::CTRL_BREAK;
            case CTRL_CLOSE_EVENT:
                return system_signal_manager::event::CLOSE;
            case CTRL_LOGOFF_EVENT:
                return system_signal_manager::event::LOGOFF;
            case CTRL_SHUTDOWN_EVENT:
                return system_signal_manager::event::SHUTDOWN;
            default:
                return static_cast<system_signal_manager::event>(event_id);
        }
    }

    ::BOOL __stdcall windows_handler(unsigned long event_id) {
        if (g_notify_event != nullptr) {
            system_signal_manager::instance().send_signal(map_win_event(event_id));
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
            static_cast<system_signal_manager::event>(CTRL_C_EVENT);
#else
            static_cast<system_signal_manager::event>(SIGTERM);
#endif

    thread_local void* g_signal_context = nullptr;
} // namespace


system_signal_manager::system_signal_manager()
#ifdef NEFORCE_PLATFORM_WINDOWS
:
notify_event_(false, system_event::type::auto_reset)
#endif
{
    initialize();
}

system_signal_manager::~system_signal_manager() {
    stop_monitoring();
    cleanup();
}

void system_signal_manager::initialize() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    g_notify_event = &notify_event_;
    ::SetConsoleCtrlHandler(windows_handler, TRUE);
    handlers_[event::INTERRUPT] = nullptr;
    handlers_[event::CTRL_BREAK] = nullptr;
    handlers_[event::CLOSE] = nullptr;
    handlers_[event::LOGOFF] = nullptr;
    handlers_[event::SHUTDOWN] = nullptr;
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

    const int sigs[] = {SIGINT, SIGTERM, SIGABRT, SIGILL, SIGFPE,  SIGSEGV,
                        SIGBUS, SIGPIPE, SIGALRM, SIGHUP, SIGUSR1, SIGUSR2};
    for (const int sig: sigs) {
        ::sigaction(sig, &sa, &old_actions_[sig]);
    }
#endif
}

void system_signal_manager::cleanup() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCtrlHandler(windows_handler, FALSE);
    g_notify_event = nullptr;
#else
    for (int sig = 1; sig < 64; ++sig) {
        if (old_actions_[sig].sa_handler != nullptr || old_actions_[sig].sa_flags != 0) {
            ::sigaction(sig, &old_actions_[sig], nullptr);
        }
    }
#endif
}

void system_signal_manager::register_handler(const event event, signal_handler handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(mutex_);
    handlers_[event] = move(handler);
}

void system_signal_manager::register_handlers(const vector<event>& events, const signal_handler& handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }
    lock<mutex> lk(mutex_);
    for (auto e: events) {
        handlers_[e] = handler;
    }
}

void system_signal_manager::remove_handler(const event event) {
    lock<mutex> lk(mutex_);
    handlers_.erase(event);
}

system_signal_manager::event system_signal_manager::wait_for_signal(const int timeout_ms) {
    return wait_for_signal_internal(timeout_ms).event;
}

void system_signal_manager::send_signal(const event event, void* context) {
    lock<mutex> lk(mutex_);
    send_signal_nolock(event, context);
}

void system_signal_manager::set_force_exit_timeout(const int timeout_ms) { force_exit_timeout_ = timeout_ms; }

void system_signal_manager::start_monitoring() {
    if (running_) {
        return;
    }

    if (signal_thread_.joinable()) {
        signal_thread_.join();
    }
    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }

    running_ = true;
    force_exit_ = false;

    signal_thread_ = thread(&system_signal_manager::signal_thread_func, this);
    timeout_thread_ = thread(&system_signal_manager::timeout_monitor_thread, this);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetThreadPriority(signal_thread_.native_handle(), THREAD_PRIORITY_HIGHEST);
#else
    struct sched_param param;
    param.sched_priority = ::sched_get_priority_max(SCHED_FIFO);
    ::pthread_setschedparam(signal_thread_.native_handle(), SCHED_FIFO, &param);
#endif
}

void system_signal_manager::stop_monitoring() noexcept {
    try {
        if (guard_count_.load() > 0) {
            return;
        }

        if (!running_) {
            if (signal_thread_.joinable()) {
                signal_thread_.join();
            }
            if (timeout_thread_.joinable()) {
                timeout_thread_.join();
            }
            return;
        }

        force_exit_ = true;
        running_ = false;

#ifdef NEFORCE_PLATFORM_WINDOWS
        notify_event_.set();
#else
        if (g_signal_pipe != nullptr) {
            constexpr int wakeup = -1;
            ::write(g_signal_pipe->native_write_handle(), &wakeup, sizeof(wakeup));
        }
#endif
        cv_.notify_all();

        if (signal_thread_.joinable()) {
            signal_thread_.join();
        }
        if (timeout_thread_.joinable()) {
            timeout_thread_.join();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void system_signal_manager::reset_force() {
    stop_monitoring();
    guard_count_ = 0;
    lock<mutex> lk(mutex_);
    for (auto it = handlers_.begin(); it != handlers_.end();) {
        if (it->first >= event::CUSTOM_1) {
            it = handlers_.erase(it);
        } else {
            ++it;
        }
    }
}

bool system_signal_manager::is_running() const { return running_; }

void system_signal_manager::signal_thread_func() {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sched_param param{};
    param.sched_priority = 10;
    ::pthread_setschedparam(::pthread_self(), SCHED_FIFO, &param);
#endif

    while (running_) {
        bool has_signals = false;

#ifdef NEFORCE_PLATFORM_WINDOWS
        notify_event_.wait(50);
        if (!running_) {
            break;
        }

        while (running_) {
            const signal_result result = wait_for_signal_internal(0);
            if (result.event == event::TIMEOUT) {
                break;
            }
            has_signals = true;
            process_signal(result.event, result.context);
            if (result.event == event::FORCE_EXIT) {
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
        if (!running_) {
            break;
        }

        while (running_ && ready > 0 && FD_ISSET(read_fd, &rfds)) {
            int sig = 0;
            const ssize_t n = ::read(read_fd, &sig, sizeof(sig));

            if (n == sizeof(sig)) {
                if (sig == -2) {
                    while (running_) {
                        const signal_result result = wait_for_signal_internal(0);
                        if (result.event == event::TIMEOUT) {
                            break;
                        }
                        has_signals = true;
                        process_signal(result.event, result.context);
                        if (result.event == event::FORCE_EXIT) {
                            return;
                        }
                    }
                } else if (sig >= 0) {
                    const auto ev = static_cast<system_signal_manager::event>(sig);
                    has_signals = true;
                    process_signal(ev, nullptr);
                    if (ev == system_signal_manager::event::FORCE_EXIT) {
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

        if (!has_signals && running_) {
            signal_handler h;
            {
                lock<mutex> lk(mutex_);
                auto it = handlers_.find(event::TIMEOUT);
                if (it != handlers_.end() && it->second) {
                    h = it->second;
                }
            }
            if (h) {
                h(event::TIMEOUT, nullptr);
            }
        }
    }
}

void system_signal_manager::timeout_monitor_thread() {
    while (running_ && !force_exit_) {
        this_thread::sleep_for(milliseconds(100));
        lock<mutex> lk(mutex_);
        if (pending_signals_.empty()) {
            continue;
        }

        int timeout = force_exit_timeout_.load();
        auto now = steady_clock::now();

        auto it = remove_if(pending_signals_.begin(), pending_signals_.end(),
                            [timeout, now](const pending_signal& ps) -> bool {
                                const auto age = time_cast<milliseconds>(now - ps.timestamp).count();
                                return age > timeout;
                            });
        if (it != pending_signals_.end()) {
            printcln(color::yellow(), "Removing ", pending_signals_.end() - it, " stale signal(s)");
            pending_signals_.erase(it, pending_signals_.end());
            cv_.notify_all();
        }
    }
}

void system_signal_manager::process_signal(const event event, void* context) {
    signal_handler h;
    {
        lock<mutex> lk(mutex_);
        const auto it = handlers_.find(event);
        if (it != handlers_.end()) {
            h = it->second;
        }
    }

    if (h) {
        g_current_signal = event;
        g_signal_context = context;
        const bool should_exit = !h(event, context);
        g_current_signal =
#ifdef NEFORCE_PLATFORM_WINDOWS
                static_cast<system_signal_manager::event>(CTRL_C_EVENT);
#else
                static_cast<system_signal_manager::event>(SIGTERM);
#endif
        g_signal_context = nullptr;
        if (should_exit && event == event::FORCE_EXIT) {
            terminate();
        }
        return;
    }

    switch (event) {
        case event::INTERRUPT:
        case event::TERMINATE:
        case event::HANGUP:
        case event::CTRL_BREAK:
        case event::CLOSE:
        case event::LOGOFF:
        case event::SHUTDOWN: {
            printcln(color::blue(), "Received termination signal, exiting...");
            running_ = false;
            break;
        }
        case event::SEGMENT_FAULT:
        case event::ILLEGAL_INSTR:
        case event::FLOATING_POINT:
        case event::BUS_ERROR: {
#ifdef NEFORCE_PLATFORM_WINDOWS
            printcln(color::red(), "Critical error signal received: ", static_cast<int>(event));
            trigger_force_exit();
#else
            printcln(color::red(), "Critical error detected, aborting: ", static_cast<int>(event));
            ::abort();
#endif
            break;
        }
        case event::ABORT: {
            printcln(color::red(), "Abort signal received.");
#ifdef NEFORCE_PLATFORM_WINDOWS
            trigger_force_exit();
#else
            ::abort();
#endif
            break;
        }
        case event::PIPE_BROKEN: {
            printcln(color::yellow(), "Broken pipe signal received, ignoring.");
            break;
        }
        case event::ALARM: {
            printcln(color::yellow(), "Alarm signal received.");
            break;
        }
        case event::USER1: {
            printcln(color::cyan(), "User signal 1 received.");
            break;
        }
        case event::USER2: {
            printcln(color::cyan(), "User signal 2 received.");
            break;
        }
        case event::TIMEOUT: {
            printcln(color::yellow(), "Timeout signal received.");
            break;
        }
        case event::CUSTOM_1:
        case event::CUSTOM_2: {
            printcln(color::cyan(), "Custom event received: ", static_cast<int>(event));
            break;
        }
        case event::FORCE_EXIT: {
            printcln(color::red(), "Force exit triggered.");
            force_exit_ = true;
            terminate();
        }
        default: {
            printcln(color::yellow(), "Unhandled signal: ", static_cast<int>(event));
            break;
        }
    }
}

void system_signal_manager::trigger_force_exit() {
    bool expected = false;
    if (force_exit_.compare_exchange_strong(expected, true)) {
        running_ = false;
        cv_.notify_all();
#ifdef NEFORCE_PLATFORM_WINDOWS
        notify_event_.set();
#endif
    }
}

system_signal_manager::signal_result system_signal_manager::wait_for_signal_internal(const int timeout_ms) {
    unique_lock<mutex> lk(mutex_);
    if (timeout_ms >= 0) {
        const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
        if (!cv_.wait_until(lk, deadline, [this]() { return !pending_signals_.empty() || !running_; })) {
            return signal_result{event::TIMEOUT, nullptr};
        }
    } else {
        cv_.wait(lk, [this]() { return !pending_signals_.empty() || !running_; });
    }

    if (!pending_signals_.empty()) {
        const pending_signal ps = pending_signals_.front();
        pending_signals_.erase(pending_signals_.begin());
        return signal_result{ps.signal_event, ps.context};
    }
    return signal_result{event::TIMEOUT, nullptr};
}

void system_signal_manager::send_signal_nolock(event event, void* context) {
    pending_signals_.emplace_back(event, context, steady_clock::now());
    cv_.notify_all();
#ifdef NEFORCE_PLATFORM_WINDOWS
    notify_event_.set();
#else
    constexpr int wakeup = -2;
    if (g_signal_pipe != nullptr) {
        ::write(g_signal_pipe->native_write_handle(), &wakeup, sizeof(wakeup));
    }
#endif
}

bool system_signal_manager::block_signals(const vector<event>& signals_to_block) const {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    ::sigemptyset(&mask);
    for (auto ev: signals_to_block) {
        const int sig = static_cast<int>(ev);
        if (sig > 0) {
            ::sigaddset(&mask, sig);
        }
    }
    return ::pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0;
#else
    (void) signals_to_block;
    return true;
#endif
}

bool system_signal_manager::unblock_signals(const vector<event>& signals_to_unblock) const {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    ::sigemptyset(&mask);
    for (auto ev: signals_to_unblock) {
        const int sig = static_cast<int>(ev);
        if (sig > 0) {
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
