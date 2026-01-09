#include <MSTL/core/system/signal.hpp>
#include <MSTL/core/algorithm/remove.hpp>
#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <cstring>
#endif
MSTL_BEGIN_NAMESPACE__

static thread_local SIGNAL_EVENT current_signal =
#ifdef MSTL_PLATFORM_WINDOWS__
    static_cast<SIGNAL_EVENT>(CTRL_C_EVENT);
#else
    static_cast<SIGNAL_EVENT>(SIGTERM);
#endif

static thread_local void* signal_context = nullptr;

#ifdef MSTL_PLATFORM_LINUX__
unordered_map<SIGNAL_EVENT, int> signal_manager::windows_to_posix_map_ = {
    {SIGNAL_EVENT::CTRL_BREAK, SIGTERM},
    {SIGNAL_EVENT::CLOSE,      SIGHUP},
    {SIGNAL_EVENT::LOGOFF,     SIGTERM},
    {SIGNAL_EVENT::SHUTDOWN,   SIGTERM}
};

static bool is_valid_posix_signal(const int sig) {
    return sig > 0 && sig < 64;
}

static bool is_windows_simulated_event(SIGNAL_EVENT event) {
    const int value = static_cast<int>(event);
    return value >= 1000 && value < 2000;
}
#endif

signal_manager::signal_manager() {
    initialize_platform();
}

signal_manager::~signal_manager() {
    stop_monitoring();
    cleanup_platform();
}

void signal_manager::initialize_platform() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetConsoleCtrlHandler(windows_handler, TRUE);

    handlers_[SIGNAL_EVENT::INTERRUPT]  = nullptr;
    handlers_[SIGNAL_EVENT::CTRL_BREAK] = nullptr;
    handlers_[SIGNAL_EVENT::CLOSE]      = nullptr;
    handlers_[SIGNAL_EVENT::LOGOFF]     = nullptr;
    handlers_[SIGNAL_EVENT::SHUTDOWN]   = nullptr;

    registered_windows_events_ = {
        CTRL_C_EVENT,
        CTRL_BREAK_EVENT,
        CTRL_CLOSE_EVENT,
        CTRL_LOGOFF_EVENT,
        CTRL_SHUTDOWN_EVENT
    };
#else
    struct ::sigaction sa_alarm;
    sa_alarm.sa_handler = signal_manager::alarm_handler;
    ::sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = SA_RESTART;
    ::sigaction(SIGALRM, &sa_alarm, nullptr);

    struct ::sigaction sa;
    sa.sa_handler = signal_manager::posix_handler;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    const int posix_signals[] = {
        SIGINT, SIGTERM, SIGABRT, SIGILL, SIGFPE,
        SIGSEGV, SIGBUS, SIGPIPE, SIGHUP, SIGUSR1, SIGUSR2
    };

    for (const int sig : posix_signals) {
        ::sigaction(sig, &sa, &old_actions_[sig]);
    }

    ::sigevent sev{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    ::timer_create(CLOCK_REALTIME, &sev, &alarm_timer_);
#endif
}

void signal_manager::cleanup_platform() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetConsoleCtrlHandler(windows_handler, FALSE);
#else
    for (int sig = 1; sig < 64; ++sig) {
        if (old_actions_[sig].sa_handler != SIG_DFL &&
            old_actions_[sig].sa_handler != SIG_IGN &&
            old_actions_[sig].sa_handler != nullptr) {
            ::sigaction(sig, &old_actions_[sig], nullptr);
        }
    }
    
    if (alarm_timer_ != nullptr) {
        ::timer_delete(alarm_timer_);
    }
#endif
}

void signal_manager::register_handler(const SIGNAL_EVENT event, signal_handler handler) {
    if (!handler) {
        throw_exception(system_exception("Signal handler cannot be null"));
    }

    lock_guard<mutex> lock(mutex_);

    if (!is_platform_signal(event)) {
#ifdef MSTL_PLATFORM_WINDOWS__
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

void signal_manager::register_handlers(
    const vector<SIGNAL_EVENT>& events, signal_handler handler) {
    if (!handler) {
        throw_exception(system_exception("Signal handler cannot be null"));
    }

    lock_guard<mutex> lock(mutex_);
    for (auto event : events) {
        handlers_[event] = handler;
    }
}

void signal_manager::remove_handler(const SIGNAL_EVENT event) {
    lock_guard<mutex> lock(mutex_);
    handlers_.erase(event);
}

SIGNAL_EVENT signal_manager::wait_for_signal(const int timeout_ms) {
    unique_lock<mutex> lock(mutex_);
    
    if (timeout_ms >= 0) {
        const auto timeout_time = steady_clock::now() + milliseconds(timeout_ms);
        if (!cv_.wait_until(lock, timeout_time, [this]() {
            return !pending_signals_.empty();
        })) {
            return SIGNAL_EVENT::TIMEOUT;
        }
    } else {
        cv_.wait(lock, [this]() {
            return !pending_signals_.empty();
        });
    }
    
    if (!pending_signals_.empty()) {
        const SIGNAL_EVENT event = pending_signals_.front().event;
        pending_signals_.erase(pending_signals_.begin());
        return event;
    }
    
    return SIGNAL_EVENT::TIMEOUT;
}

void signal_manager::send_signal(SIGNAL_EVENT event, void* context) {
    lock_guard<mutex> lock(mutex_);

#ifdef MSTL_PLATFORM_WINDOWS__
    if (static_cast<DWORD>(event) == CTRL_C_EVENT ||
        static_cast<DWORD>(event) == CTRL_BREAK_EVENT) {
        pending_signals_.emplace_back(event, context, steady_clock::now());
        printcln(color::yellow(), "Signal sent: ", static_cast<DWORD>(event));
        } else {
            pending_signals_.emplace_back(event, context, steady_clock::now());
            printcln(color::yellow(), "Signal sent: ", static_cast<DWORD>(event));
        }
#else
    const int sig_value = static_cast<int>(event);
    if (is_valid_posix_signal(sig_value)) {
        pending_signals_.emplace_back(event, context, steady_clock::now());
        printcln(color::yellow(), "POSIX signal sent: ", sig_value,
            " (", ::strsignal(sig_value), ")");
    } else if (is_windows_simulated_event(event)) {
        const auto it = windows_to_posix_map_.find(event);
        if (it != windows_to_posix_map_.end()) {
            pending_signals_.emplace_back(
                static_cast<SIGNAL_EVENT>(it->second),
                context,
                steady_clock::now()
            );
            printcln(color::yellow(), "Windows simulated event sent: ",
                sig_value, " -> POSIX ", it->second);
        } else {
            pending_signals_.emplace_back(event, context, steady_clock::now());
            printcln(color::yellow(), "Custom event sent: ", sig_value);
        }
    } else {
        pending_signals_.emplace_back(event, context, steady_clock::now());
        printcln(color::yellow(), "Custom event sent: ", sig_value);
    }
#endif

    cv_.notify_all();
}

void signal_manager::set_force_exit_timeout(const int timeout_ms) {
    force_exit_timeout_ = timeout_ms;
}

void signal_manager::start_monitoring() {
    if (running_) return;
    running_ = true;
    force_exit_ = false;

    signal_thread_ = thread(&signal_manager::signal_thread_func, this);
    timeout_thread_ = thread(&signal_manager::timeout_monitor_thread, this);
    
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetThreadPriority(signal_thread_.native_handle(), THREAD_PRIORITY_HIGHEST);
#else
    ::sched_param param;
    param.sched_priority = ::sched_get_priority_max(SCHED_FIFO);
    ::pthread_setschedparam(signal_thread_.native_handle(), SCHED_FIFO, &param);
#endif
}

void signal_manager::stop_monitoring() {
    if (!running_) return;
    
    running_ = false;
    cv_.notify_all();
    
    if (signal_thread_.joinable()) {
        signal_thread_.join();
    }
    
    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }
}

bool signal_manager::is_running() const {
    return running_;
}

void signal_manager::signal_thread_func() {
#ifdef MSTL_PLATFORM_LINUX__
    ::sched_param param;
    param.sched_priority = 10;
    ::pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif

    while (running_) {
        const SIGNAL_EVENT event = wait_for_signal(100);
        if (event == SIGNAL_EVENT::TIMEOUT) {
            continue;
        }
        void* context = nullptr;
        {
            lock_guard<mutex> lock(mutex_);
            if (!pending_signals_.empty()) {
                context = pending_signals_.front().context;
            }
        }
        process_signal(event, context);
        if (event == SIGNAL_EVENT::FORCE_EXIT) {
            break;
        }
    }
}

void signal_manager::timeout_monitor_thread() {
    const auto start_time = steady_clock::now();

    while (running_ && !force_exit_) {
        this_thread::sleep_for(milliseconds(100));

        {
            lock_guard<mutex> lock(mutex_);
            if (pending_signals_.empty()) {
                continue;
            }

            int timeout = force_exit_timeout_.load();
            auto now = steady_clock::now();
            const auto elapsed = duration_cast<milliseconds>(
                now - start_time).count();

            if (elapsed > timeout) {
                send_signal(SIGNAL_EVENT::FORCE_EXIT);
                break;
            }

            auto it = remove_if(pending_signals_.begin(), pending_signals_.end(),
                [timeout, now](const pending_signal& ps) -> bool {
                    const auto signal_age = duration_cast<milliseconds>(
                        now - ps.timestamp).count();
                    return signal_age > timeout;
                });

            if (it != pending_signals_.end()) {
                pending_signals_.erase(it, pending_signals_.end());
            }
        }
    }
}

void signal_manager::process_signal(SIGNAL_EVENT event, void* context) {
    signal_handler handler;
    
    {
        lock_guard<mutex> lock(mutex_);
        const auto it = handlers_.find(event);
        if (it != handlers_.end()) {
            handler = it->second;
        }
    }
    
    if (handler) {
        current_signal = event;
        signal_context = context;

        const bool should_exit = !handler(move(event), move(context));

        current_signal =
#ifdef MSTL_PLATFORM_WINDOWS__
            static_cast<SIGNAL_EVENT>(CTRL_C_EVENT);
#else
            static_cast<SIGNAL_EVENT>(SIGTERM);
#endif
        signal_context = nullptr;

        if (should_exit && event == SIGNAL_EVENT::FORCE_EXIT) {
            terminate();
        }
    } else {
        switch (event) {
#ifdef MSTL_PLATFORM_WINDOWS__
            case SIGNAL_EVENT::INTERRUPT:
            case SIGNAL_EVENT::CTRL_BREAK:
            case SIGNAL_EVENT::CLOSE:
            case SIGNAL_EVENT::LOGOFF:
            case SIGNAL_EVENT::SHUTDOWN: {
#else
            case SIGNAL_EVENT::INTERRUPT:
            case SIGNAL_EVENT::TERMINATE:
            case SIGNAL_EVENT::HANGUP: {
#endif
                printcln(color::blue(), "Received termination signal, exiting...");
                running_ = false;
                break;
            }
#ifdef MSTL_PLATFORM_WINDOWS__
            case SIGNAL_EVENT::SEGMENT_FAULT:
            case SIGNAL_EVENT::ILLEGAL_INSTR: {
                printcln(color::red(), "Simulated critical error!");
                send_signal(SIGNAL_EVENT::FORCE_EXIT);
                break;
            }
#else
            case SIGNAL_EVENT::SEGMENT_FAULT:
            case SIGNAL_EVENT::ILLEGAL_INSTR:
            case SIGNAL_EVENT::FLOATING_POINT:
            case SIGNAL_EVENT::BUS_ERROR: {
                printcln(color::red(), "Critical error detected!");
                std::abort();
                break;
            }
#endif
            case SIGNAL_EVENT::FORCE_EXIT: {
                terminate();
            }
            default: {
                printcln(color::yellow(), "Unhandled signal: ", static_cast<int>(event));
                break;
            }
        }
    }
}

bool signal_manager::block_signals(const vector<SIGNAL_EVENT>& signals_to_block) const {
#ifdef MSTL_PLATFORM_LINUX__
    ::sigset_t mask;
    ::sigemptyset(&mask);
    
    for (const auto event : signals_to_block) {
        const int sig = static_cast<int>(event);
        if (sig > 0) {
            ::sigaddset(&mask, sig);
        }
    }
    
    return ::pthread_sigmask(SIG_BLOCK, &mask, nullptr) == 0;
#endif
    return true;
}

bool signal_manager::unblock_signals(const vector<SIGNAL_EVENT>& signals_to_unblock) const {
#ifdef MSTL_PLATFORM_LINUX__
    ::sigset_t mask;
    ::sigemptyset(&mask);
    
    for (const auto event : signals_to_unblock) {
        const int sig = static_cast<int>(event);
        if (sig > 0) {
            ::sigaddset(&mask, sig);
        }
    }
    
    return ::pthread_sigmask(SIG_UNBLOCK, &mask, nullptr) == 0;
#endif
    return true;
}

#ifdef MSTL_PLATFORM_WINDOWS__

::BOOL WINAPI signal_manager::windows_handler(const ::DWORD event) {
    signal_manager& manager = instance();
    manager.send_signal(static_cast<SIGNAL_EVENT>(event));
    return TRUE;
}

#else

void signal_manager::posix_handler(const int sig) {
    signal_manager &manager = instance();
    manager.send_signal(static_cast<SIGNAL_EVENT>(sig));
}

void signal_manager::alarm_handler(int sig) {
    instance().send_signal(SIGNAL_EVENT::TIMEOUT);
}

#endif

MSTL_END_NAMESPACE__
