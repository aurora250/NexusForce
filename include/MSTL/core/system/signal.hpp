#ifndef MSTL_CORE_SYSTEM_SIGNAL_HPP__
#define MSTL_CORE_SYSTEM_SIGNAL_HPP__
#include "../async/atomic.hpp"
#include "../async/condition_variable.hpp"
#include "../async/thread.hpp"
#include "../functional/function.hpp"
#include "../container/unordered_map.hpp"
#include "../container/vector.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <csignal>
#endif
MSTL_BEGIN_NAMESPACE__

enum class SIGNAL_EVENT : int32_t {
    // Universal
    INTERRUPT      = 1,
    TERMINATE      = 2,
    ABORT          = 3,

    // POSIX
    ILLEGAL_INSTR  = 4,    // SIGILL
    FLOATING_POINT = 5,    // SIGFPE
    SEGMENT_FAULT  = 6,    // SIGSEGV
    BUS_ERROR      = 7,    // SIGBUS
    PIPE_BROKEN    = 8,    // SIGPIPE
    ALARM          = 9,    // SIGALRM
    TERM           = 10,   // SIGTERM
    HANGUP         = 11,   // SIGHUP
    USER1          = 12,   // SIGUSR1
    USER2          = 13,   // SIGUSR2

    // Windows
    CTRL_BREAK     = 14,
    CLOSE          = 15,
    LOGOFF         = 16,
    SHUTDOWN       = 17,

    //
    TIMEOUT        = 100,
    CUSTOM_1       = 101,
    CUSTOM_2       = 102,
    FORCE_EXIT     = 999
};

template <>
struct hash<SIGNAL_EVENT> {
    size_t operator ()(const SIGNAL_EVENT e) const {
        return hash<int32_t>()(static_cast<int32_t>(e));
    }
};


class signal_manager {
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
    vector<DWORD> registered_windows_events_;
#else
    struct ::sigaction old_actions_[64];
    ::timer_t alarm_timer_{nullptr};
#endif

    thread signal_thread_;
    thread timeout_thread_;

    static MSTL_THREAD_LOCAL SIGNAL_EVENT current_signal_;
    static MSTL_THREAD_LOCAL void* signal_context_;

private:
    void initialize_platform();
    void cleanup_platform() const;

#ifdef MSTL_PLATFORM_WINDOWS__
    static BOOL WINAPI windows_handler(DWORD event);
    DWORD convert_to_windows_event(SIGNAL_EVENT event) const;
    SIGNAL_EVENT convert_from_windows_event(DWORD event) const;
#else
    static void posix_handler(int sig);
    int convert_to_posix_signal(SIGNAL_EVENT event) const;
    SIGNAL_EVENT convert_from_posix_signal(int sig) const;
    static void alarm_handler(int sig);
#endif

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

    static signal_manager& instance();

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
