#include <MSTL/core/system/signal.hpp>
#include <MSTL/core/algorithm/erase.hpp>
#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_LINUX__

#endif
MSTL_BEGIN_NAMESPACE__

MSTL_THREAD_LOCAL SIGNAL_EVENT signal_manager::current_signal_ = SIGNAL_EVENT::TERMINATE;
MSTL_THREAD_LOCAL void* signal_manager::signal_context_ = nullptr;

signal_manager& signal_manager::instance() {
    static signal_manager instance;
    return instance;
}

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
#else
    struct ::sigaction sa_alarm;
    sa_alarm.sa_handler = signal_manager::alarm_handler;
    ::sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = SA_RESTART;
    ::sigaction(SIGALRM, &sa_alarm, nullptr);

    ::sigevent sev;
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
    handlers_[event] = move(handler);
    
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::DWORD win_event = convert_to_windows_event(event);
    if (win_event != 0) {
        registered_windows_events_.push_back(win_event);
    }
#endif
}

void signal_manager::register_handlers(
    const vector<SIGNAL_EVENT>& events, signal_handler handler) {
    if (!handler) {
        throw_exception(system_exception("Signal handler cannot be null"));
    }

    lock_guard<mutex> lock(mutex_);
    for (auto event : events) {
        handlers_[event] = move(handler);
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
    pending_signals_.emplace_back(event, context, steady_clock::now());
    printcln(color::yellow(), "Signal sent: ", static_cast<int>(event));
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
    param.sched_priority = ::sched_get_priority_max(SCHED_FIFO);
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
        if (!handler(move(event), move(context)) && event == SIGNAL_EVENT::FORCE_EXIT) {
            terminate();
        }
    } else {
        switch (event) {
            case SIGNAL_EVENT::INTERRUPT:
            case SIGNAL_EVENT::TERMINATE: {
                printcln(color::blue(), "Received termination signal, exiting...");
                running_ = false;
                break;
            }
            case SIGNAL_EVENT::SEGMENT_FAULT:
            case SIGNAL_EVENT::ILLEGAL_INSTR: {
                printcln(color::red(), "Critical error detected!");
                std::abort();
                break;
            }
            case SIGNAL_EVENT::FORCE_EXIT: {
                terminate();
                break;
            }
            default: {
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
        const int sig = convert_to_posix_signal(event);
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
        const int sig = convert_to_posix_signal(event);
        if (sig > 0) {
            ::sigaddset(&mask, sig);
        }
    }
    
    return ::pthread_sigmask(SIG_UNBLOCK, &mask, nullptr) == 0;
#endif
    return true;
}

#ifdef MSTL_PLATFORM_WINDOWS__

BOOL WINAPI signal_manager::windows_handler(const DWORD event) {
    signal_manager& manager = instance();
    manager.send_signal(manager.convert_from_windows_event(event));
    return TRUE;
}

DWORD signal_manager::convert_to_windows_event(const SIGNAL_EVENT event)const {
    switch (event) {
        case SIGNAL_EVENT::INTERRUPT:    return CTRL_C_EVENT;
        case SIGNAL_EVENT::CTRL_BREAK:   return CTRL_BREAK_EVENT;
        case SIGNAL_EVENT::CLOSE:        return CTRL_CLOSE_EVENT;
        case SIGNAL_EVENT::LOGOFF:       return CTRL_LOGOFF_EVENT;
        case SIGNAL_EVENT::SHUTDOWN:     return CTRL_SHUTDOWN_EVENT;
        default: return 0;
    }
}

SIGNAL_EVENT signal_manager::convert_from_windows_event(const DWORD event) const {
    switch (event) {
        case CTRL_C_EVENT:        return SIGNAL_EVENT::INTERRUPT;
        case CTRL_BREAK_EVENT:    return SIGNAL_EVENT::CTRL_BREAK;
        case CTRL_CLOSE_EVENT:    return SIGNAL_EVENT::CLOSE;
        case CTRL_LOGOFF_EVENT:   return SIGNAL_EVENT::LOGOFF;
        case CTRL_SHUTDOWN_EVENT: return SIGNAL_EVENT::SHUTDOWN;
        default:                  return SIGNAL_EVENT::TERMINATE;
    }
}

#else

void signal_manager::posix_handler(const int sig) {
    signal_manager& manager = instance();
    const SIGNAL_EVENT event = manager.convert_from_posix_signal(sig);
    manager.send_signal(event);
}

int signal_manager::convert_to_posix_signal(const SIGNAL_EVENT event) const {
    switch (event) {
        case SIGNAL_EVENT::INTERRUPT:      return SIGINT;
        case SIGNAL_EVENT::TERMINATE:      return SIGTERM;
        case SIGNAL_EVENT::ABORT:          return SIGABRT;
        case SIGNAL_EVENT::ILLEGAL_INSTR:  return SIGILL;
        case SIGNAL_EVENT::FLOATING_POINT: return SIGFPE;
        case SIGNAL_EVENT::SEGMENT_FAULT:  return SIGSEGV;
        case SIGNAL_EVENT::BUS_ERROR:      return SIGBUS;
        case SIGNAL_EVENT::PIPE_BROKEN:    return SIGPIPE;
        case SIGNAL_EVENT::ALARM:          return SIGALRM;
        case SIGNAL_EVENT::HANGUP:         return SIGHUP;
        case SIGNAL_EVENT::USER1:          return SIGUSR1;
        case SIGNAL_EVENT::USER2:          return SIGUSR2;
        default: return -1;
    }
}

SIGNAL_EVENT signal_manager::convert_from_posix_signal(const int sig) const {
    switch (sig) {
        case SIGINT:    return SIGNAL_EVENT::INTERRUPT;
        case SIGTERM:   return SIGNAL_EVENT::TERMINATE;
        case SIGABRT:   return SIGNAL_EVENT::ABORT;
        case SIGILL:    return SIGNAL_EVENT::ILLEGAL_INSTR;
        case SIGFPE:    return SIGNAL_EVENT::FLOATING_POINT;
        case SIGSEGV:   return SIGNAL_EVENT::SEGMENT_FAULT;
        case SIGBUS:    return SIGNAL_EVENT::BUS_ERROR;
        case SIGPIPE:   return SIGNAL_EVENT::PIPE_BROKEN;
        case SIGALRM:   return SIGNAL_EVENT::ALARM;
        case SIGHUP:    return SIGNAL_EVENT::HANGUP;
        case SIGUSR1:   return SIGNAL_EVENT::USER1;
        case SIGUSR2:   return SIGNAL_EVENT::USER2;
        default:        return SIGNAL_EVENT::TERMINATE;
    }
}

void signal_manager::alarm_handler(int sig) {
    instance().send_signal(SIGNAL_EVENT::TIMEOUT);
}

#endif

MSTL_END_NAMESPACE__
