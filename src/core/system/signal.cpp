#include <NeForce/core/algorithm/remove.hpp>
#include <NeForce/core/exception/terminate.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/signal.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windef.h>
#    include <WinBase.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cstdlib>
#    include <cstring>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::BOOL __stdcall windows_handler(const native_size_type event) {
        signal_manager& manager = signal_manager::instance();
        manager.send_signal(static_cast<signal_event>(event));
        return TRUE;
    }
#else
    void posix_handler(const int sig) {
        signal_manager& manager = signal_manager::instance();
        manager.send_signal(static_cast<signal_event>(sig));
    }

    bool is_valid_posix_signal(const int sig) { return sig > 0 && sig < 64; }
    bool is_windows_simulated_event(signal_event event) {
        const int value = static_cast<int>(event);
        return value >= 1000 && value < 2000;
    }

    const unordered_map<signal_event, int>& windows_to_posix_map() {
        static unordered_map<signal_event, int> signal_map{{signal_event::CTRL_BREAK, SIGTERM},
                                                           {signal_event::CLOSE, SIGHUP},
                                                           {signal_event::LOGOFF, SIGTERM},
                                                           {signal_event::SHUTDOWN, SIGTERM}};
        return signal_map;
    };

#endif

    thread_local auto g_current_signal =
#ifdef NEFORCE_PLATFORM_WINDOWS
            static_cast<signal_event>(CTRL_C_EVENT);
#else
            static_cast<signal_event>(SIGTERM);
#endif

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local void* signal_context = nullptr;
} // namespace


signal_manager::signal_manager() { initialize_platform(); }

signal_manager::~signal_manager() {
    stop_monitoring();
    cleanup_platform();
}

void signal_manager::initialize_platform() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCtrlHandler(windows_handler, TRUE);

    handlers_[signal_event::INTERRUPT] = nullptr;
    handlers_[signal_event::CTRL_BREAK] = nullptr;
    handlers_[signal_event::CLOSE] = nullptr;
    handlers_[signal_event::LOGOFF] = nullptr;
    handlers_[signal_event::SHUTDOWN] = nullptr;

    registered_windows_events_ = {CTRL_C_EVENT, CTRL_BREAK_EVENT, CTRL_CLOSE_EVENT, CTRL_LOGOFF_EVENT,
                                  CTRL_SHUTDOWN_EVENT};
#else
    struct ::sigaction sa;
    sa.sa_handler = posix_handler;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    const int posix_signals[] = {SIGINT, SIGTERM, SIGABRT, SIGILL, SIGFPE,  SIGSEGV,
                                 SIGBUS, SIGPIPE, SIGALRM, SIGHUP, SIGUSR1, SIGUSR2};

    for (const int sig: posix_signals) {
        ::sigaction(sig, &sa, &old_actions_[sig]);
    }

    ::sigevent sev{};
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = [](::sigval val) { signal_manager::instance().send_signal(signal_event::TIMEOUT); };
    sev.sigev_notify_attributes = nullptr;
    ::timer_create(CLOCK_REALTIME, &sev, &alarm_timer_);
#endif
}

void signal_manager::cleanup_platform() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCtrlHandler(windows_handler, FALSE);
#else
    for (int sig = 1; sig < 64; ++sig) {
        if (old_actions_[sig].sa_handler != SIG_DFL && old_actions_[sig].sa_handler != SIG_IGN &&
            old_actions_[sig].sa_handler != nullptr) {
            ::sigaction(sig, &old_actions_[sig], nullptr);
        }
    }

    if (alarm_timer_ != nullptr) {
        ::timer_delete(alarm_timer_);
    }
#endif
}

void signal_manager::register_handler(const signal_event event, signal_handler handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }

    lock<mutex> lock(mutex_);

    if (!is_platform_signal(event)) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        printcln(color::yellow(), "Registering custom event: ", static_cast<::DWORD>(event));
#else
        if (is_windows_simulated_event(event)) {
            printcln(color::yellow(), "Registering Windows simulated event: ", static_cast<int>(event));
        } else {
            printcln(color::yellow(), "Registering custom event: ", static_cast<int>(event));
        }
#endif
    }

    handlers_[event] = move(handler);
}

void signal_manager::register_handlers(const vector<signal_event>& events, const signal_handler& handler) {
    if (!handler) {
        NEFORCE_THROW_EXCEPTION(system_exception("Signal handler cannot be null"));
    }

    lock<mutex> lock(mutex_);
    for (auto event: events) {
        handlers_[event] = handler;
    }
}

void signal_manager::remove_handler(const signal_event event) {
    lock<mutex> lock(mutex_);
    handlers_.erase(event);
}

signal_event signal_manager::wait_for_signal(const int timeout_ms) {
    return wait_for_signal_internal(timeout_ms).event;
}

void signal_manager::send_signal(const signal_event event, void* context) {
    lock<mutex> lock(mutex_);
    send_signal_nolock(event, context);
}

void signal_manager::set_force_exit_timeout(const int timeout_ms) { force_exit_timeout_ = timeout_ms; }

void signal_manager::start_monitoring() {
    if (running_) {
        return;
    }
    running_ = true;
    force_exit_ = false;

    signal_thread_.start(&signal_manager::signal_thread_func, this);
    timeout_thread_.start(&signal_manager::timeout_monitor_thread, this);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetThreadPriority(signal_thread_.native_handle(), THREAD_PRIORITY_HIGHEST);
#else
    ::sched_param param;
    param.sched_priority = ::sched_get_priority_max(SCHED_FIFO);
    ::pthread_setschedparam(signal_thread_.native_handle(), SCHED_FIFO, &param);
#endif
}

void signal_manager::stop_monitoring() {
    if (!running_) {
        return;
    }

    running_ = false;
    cv_.notify_all();

    if (signal_thread_.joinable()) {
        signal_thread_.join();
    }

    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }
}

bool signal_manager::is_running() const { return running_; }

void signal_manager::signal_thread_func() {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sched_param param;
    param.sched_priority = 10;
    ::pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif

    while (running_) {
        const signal_result result = wait_for_signal_internal(100);

        if (result.event == signal_event::TIMEOUT) {
            signal_handler timeout_handler;
            {
                lock<mutex> lock(mutex_);
                const auto it = handlers_.find(signal_event::TIMEOUT);
                if (it != handlers_.end() && it->second) {
                    timeout_handler = it->second;
                }
            }
            if (timeout_handler) {
                timeout_handler(signal_event::TIMEOUT, nullptr);
            }
            continue;
        }

        process_signal(result.event, result.context);

        if (result.event == signal_event::FORCE_EXIT) {
            break;
        }
    }
}

void signal_manager::timeout_monitor_thread() {
    while (running_ && !force_exit_) {
        this_thread::sleep_for(milliseconds(100));

        {
            lock<mutex> lock(mutex_);

            if (pending_signals_.empty()) {
                continue;
            }

            const int timeout = force_exit_timeout_.load();
            const auto now = steady_clock::now();

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
}

void signal_manager::process_signal(signal_event event, void* context) {
    signal_handler handler;

    {
        lock<mutex> lock(mutex_);
        const auto it = handlers_.find(event);
        if (it != handlers_.end()) {
            handler = it->second;
        }
    }

    if (handler) {
        g_current_signal = event;
        signal_context = context;

        const bool should_exit = !handler(move(event), move(context));

        g_current_signal =
#ifdef NEFORCE_PLATFORM_WINDOWS
                static_cast<signal_event>(CTRL_C_EVENT);
#else
                static_cast<signal_event>(SIGTERM);
#endif
        signal_context = nullptr;

        if (should_exit && event == signal_event::FORCE_EXIT) {
            terminate();
        }
        return;
    }

    switch (event) {
        case signal_event::INTERRUPT:
        case signal_event::TERMINATE:
        case signal_event::HANGUP:
#ifdef NEFORCE_PLATFORM_WINDOWS
        case signal_event::CTRL_BREAK:
        case signal_event::CLOSE:
        case signal_event::LOGOFF:
        case signal_event::SHUTDOWN:
#else
        case signal_event::CTRL_BREAK:
        case signal_event::CLOSE:
        case signal_event::LOGOFF:
        case signal_event::SHUTDOWN:
#endif
        {
            printcln(color::blue(), "Received termination signal, exiting...");
            running_ = false;
            break;
        }
        case signal_event::SEGMENT_FAULT:
        case signal_event::ILLEGAL_INSTR:
        case signal_event::FLOATING_POINT:
        case signal_event::BUS_ERROR: {
#ifdef NEFORCE_PLATFORM_WINDOWS
            printcln(color::red(), "Critical error signal received: ", static_cast<int>(event));
            send_signal(signal_event::FORCE_EXIT);
#else
            printcln(color::red(), "Critical error detected, aborting: ", static_cast<int>(event));
            ::abort();
#endif
            break;
        }
        case signal_event::ABORT: {
            printcln(color::red(), "Abort signal received.");
#ifdef NEFORCE_PLATFORM_WINDOWS
            send_signal(signal_event::FORCE_EXIT);
#else
            ::abort();
#endif
            break;
        }
        case signal_event::PIPE_BROKEN: {
            printcln(color::yellow(), "Broken pipe signal received, ignoring.");
            break;
        }
        case signal_event::ALARM: {
            printcln(color::yellow(), "Alarm signal received.");
            break;
        }
        case signal_event::USER1: {
            printcln(color::cyan(), "User signal 1 received.");
            break;
        }
        case signal_event::USER2: {
            printcln(color::cyan(), "User signal 2 received.");
            break;
        }
        case signal_event::TIMEOUT: {
            printcln(color::yellow(), "Timeout signal received.");
            break;
        }
        case signal_event::CUSTOM_1:
        case signal_event::CUSTOM_2: {
            printcln(color::cyan(), "Custom event received: ", static_cast<int>(event));
            break;
        }
        case signal_event::FORCE_EXIT: {
            printcln(color::red(), "Force exit triggered.");
            terminate();
        }
        default: {
            printcln(color::yellow(), "Unhandled signal: ", static_cast<int>(event));
            break;
        }
    }
}

signal_manager::signal_result signal_manager::wait_for_signal_internal(const int timeout_ms) {
    unique_lock<mutex> lock(mutex_);

    if (timeout_ms >= 0) {
        const auto timeout_time = steady_clock::now() + milliseconds(timeout_ms);
        if (!cv_.wait_until(lock, timeout_time, [this]() { return !pending_signals_.empty() || !running_; })) {
            return signal_result{signal_event::TIMEOUT, nullptr};
        }
    } else {
        cv_.wait(lock, [this]() { return !pending_signals_.empty() || !running_; });
    }

    if (!pending_signals_.empty()) {
        const pending_signal ps = pending_signals_.front();
        pending_signals_.erase(pending_signals_.begin());
        return signal_result{ps.event, ps.context};
    }

    return signal_result{signal_event::TIMEOUT, nullptr};
}

void signal_manager::send_signal_nolock(signal_event event, void* context) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    pending_signals_.emplace_back(event, context, steady_clock::now());
    printcln(color::yellow(), "Signal sent: ", static_cast<::DWORD>(event));
#else
    const int sig_value = static_cast<int>(event);

    if (is_valid_posix_signal(sig_value)) {
        pending_signals_.emplace_back(event, context, steady_clock::now());
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        printcln(color::yellow(), "POSIX signal sent: ", sig_value, " (", ::strsignal(sig_value), ")");
    } else if (is_windows_simulated_event(event)) {
        const auto it = windows_to_posix_map().find(event);
        if (it != windows_to_posix_map().end()) {
            printcln(color::yellow(), "Windows simulated event: ", sig_value, " -> POSIX ", it->second);
        }
        pending_signals_.emplace_back(event, context, steady_clock::now());
    } else {
        pending_signals_.emplace_back(event, context, steady_clock::now());
        printcln(color::yellow(), "Custom event sent: ", sig_value);
    }
#endif
    cv_.notify_all();
}

bool signal_manager::block_signals(const vector<signal_event>& signals_to_block) const {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    ::sigemptyset(&mask);

    for (const auto event: signals_to_block) {
        const int sig = static_cast<int>(event);
        if (sig > 0) {
            ::sigaddset(&mask, sig);
        }
    }

    return ::pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0;
#endif
    return true;
}

bool signal_manager::unblock_signals(const vector<signal_event>& signals_to_unblock) const {
#ifdef NEFORCE_PLATFORM_LINUX
    ::sigset_t mask;
    ::sigemptyset(&mask);

    for (const auto event: signals_to_unblock) {
        const int sig = static_cast<int>(event);
        if (sig > 0) {
            ::sigaddset(&mask, sig);
        }
    }

    return ::pthread_sigmask(SIG_UNBLOCK, &mask, nullptr) == 0;
#endif
    return true;
}

NEFORCE_END_NAMESPACE__
