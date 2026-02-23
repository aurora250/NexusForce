#ifndef MSTL_CORE_SYSTEM_SIGNAL_HPP__
#define MSTL_CORE_SYSTEM_SIGNAL_HPP__
#include "../async/atomic.hpp"
#include "../async/condition_variable.hpp"
#include "../async/thread.hpp"
#include "../functional/function.hpp"
#include "../container/unordered_map.hpp"
#include "../container/vector.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <consoleapi.h>
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <csignal>
#endif
MSTL_BEGIN_NAMESPACE__

enum class SIGNAL_EVENT {
#ifdef MSTL_PLATFORM_WINDOWS__
    INTERRUPT      = CTRL_C_EVENT,
    CTRL_BREAK     = CTRL_BREAK_EVENT,
    CLOSE          = CTRL_CLOSE_EVENT,
    LOGOFF         = CTRL_LOGOFF_EVENT,
    SHUTDOWN       = CTRL_SHUTDOWN_EVENT,

    TERMINATE      = 1000,
    ABORT          = 1001,
    ILLEGAL_INSTR  = 1002,
    FLOATING_POINT = 1003,
    SEGMENT_FAULT  = 1004,
    BUS_ERROR      = 1005,
    PIPE_BROKEN    = 1006,
    ALARM          = 1007,
    HANGUP         = 1008,
    USER1          = 1009,
    USER2          = 1010,

    TIMEOUT        = 2000,
    CUSTOM_1       = 2001,
    CUSTOM_2       = 2002,
    FORCE_EXIT     = 9999
#else
    INTERRUPT      = SIGINT,
    TERMINATE      = SIGTERM,
    ABORT          = SIGABRT,
    ILLEGAL_INSTR  = SIGILL,
    FLOATING_POINT = SIGFPE,
    SEGMENT_FAULT  = SIGSEGV,
    BUS_ERROR      = SIGBUS,
    PIPE_BROKEN    = SIGPIPE,
    ALARM          = SIGALRM,
    HANGUP         = SIGHUP,
    USER1          = SIGUSR1,
    USER2          = SIGUSR2,

    CTRL_BREAK     = 1000,
    CLOSE          = 1001,
    LOGOFF         = 1002,
    SHUTDOWN       = 1003,

    TIMEOUT        = SIGALRM,
    CUSTOM_1       = 2000,
    CUSTOM_2       = 2001,
    FORCE_EXIT     = 9999
#endif
};

template <>
struct hash<SIGNAL_EVENT> {
    size_t operator ()(const SIGNAL_EVENT e) const {
        return hash<int32_t>()(static_cast<int32_t>(e));
    }
};


class MSTL_API signal_manager {
public:
    using signal_handler = function<bool(SIGNAL_EVENT, void*)>;

private:
    atomic_bool running_{false};
    atomic_bool force_exit_{false};
    atomic_int force_exit_timeout_{5000};

    mutex mutex_;
    condition_variable cv_;

    unordered_map<SIGNAL_EVENT, signal_handler> handlers_;

    struct pending_signal {
        SIGNAL_EVENT event;
        void* context;
        steady_clock::time_point timestamp;

        pending_signal(const SIGNAL_EVENT event, void* context, steady_clock::time_point timestamp)
        : event{event}, context{context}, timestamp{timestamp} {}
    };
    vector<pending_signal> pending_signals_;

#ifdef MSTL_PLATFORM_WINDOWS__
    vector<::DWORD> registered_windows_events_;
#else
    struct ::sigaction old_actions_[64];
    ::timer_t alarm_timer_{nullptr};

    static unordered_map<SIGNAL_EVENT, int> windows_to_posix_map_;
#endif

    thread signal_thread_;
    thread timeout_thread_;

private:
    void initialize_platform();
    void cleanup_platform() const;

    void signal_thread_func();
    void timeout_monitor_thread();

    void process_signal(SIGNAL_EVENT event, void* context = nullptr);

    signal_manager();

public:
    signal_manager(const signal_manager&) = delete;
    signal_manager& operator =(const signal_manager&) = delete;
    signal_manager(const signal_manager&&) = delete;
    signal_manager& operator =(const signal_manager&&) = delete;
    ~signal_manager();

    static signal_manager& instance() {
        static signal_manager instance;
        return instance;
    }

    void register_handler(SIGNAL_EVENT event, signal_handler handler);
    void register_handlers(const vector<SIGNAL_EVENT>& events, signal_handler handler);
    void remove_handler(SIGNAL_EVENT event);

    SIGNAL_EVENT wait_for_signal(int timeout_ms = -1);

    void send_signal(SIGNAL_EVENT event, void* context = nullptr);
    void set_force_exit_timeout(int timeout_ms);

    void start_monitoring();
    void stop_monitoring();

    bool is_running() const;

    bool block_signals(const vector<SIGNAL_EVENT>& signals_to_block) const;
    bool unblock_signals(const vector<SIGNAL_EVENT>& signals_to_unblock) const;

    static bool is_platform_signal(SIGNAL_EVENT event) {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::DWORD value = static_cast<::DWORD>(event);
        return value <= CTRL_SHUTDOWN_EVENT;
#else
        int value = static_cast<int>(event);
        return value > 0 && value < 64 && value != SIGALRM;
#endif
    }
};


class signal_guard {
public:
    signal_guard() {
        signal_manager::instance().start_monitoring();
    }
    ~signal_guard() {
        signal_manager::instance().stop_monitoring();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_SYSTEM_SIGNAL_HPP__
