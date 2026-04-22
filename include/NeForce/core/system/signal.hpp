#ifndef NEFORCE_CORE_SYSTEM_SIGNAL_HPP__
#define NEFORCE_CORE_SYSTEM_SIGNAL_HPP__

/**
 * @file signal.hpp
 * @brief 跨平台信号处理系统
 *
 * 此文件提供了统一的跨平台信号处理接口。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#ifdef NEFORCE_COMPILER_MSVC
#    include <consoleapi.h>
#endif
#ifdef NEFORCE_COMPILER_MINGW
#    include <windef.h>
#    include <wingdi.h>
#    include <wincon.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <csignal>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SystemSignal 系统信号
 * @brief 系统信号处理
 * @{
 */

/**
 * @enum signal_event
 * @brief 信号事件枚举
 *
 * 统一的信号事件定义，抽象了Windows控制台事件和POSIX信号。
 * 包含系统信号和自定义事件。
 */
enum class signal_event {
#ifdef NEFORCE_PLATFORM_WINDOWS
    INTERRUPT = CTRL_C_EVENT,       ///< Ctrl+C中断信号
    CTRL_BREAK = CTRL_BREAK_EVENT,  ///< Ctrl+Break信号
    CLOSE = CTRL_CLOSE_EVENT,       ///< 关闭控制台窗口
    LOGOFF = CTRL_LOGOFF_EVENT,     ///< 用户注销
    SHUTDOWN = CTRL_SHUTDOWN_EVENT, ///< 系统关机

    TERMINATE = 1000,      ///< 终止信号（模拟SIGTERM）
    ABORT = 1001,          ///< 中止信号（模拟SIGABRT）
    ILLEGAL_INSTR = 1002,  ///< 非法指令（模拟SIGILL）
    FLOATING_POINT = 1003, ///< 浮点异常（模拟SIGFPE）
    SEGMENT_FAULT = 1004,  ///< 段错误（模拟SIGSEGV）
    BUS_ERROR = 1005,      ///< 总线错误（模拟SIGBUS）
    PIPE_BROKEN = 1006,    ///< 管道破裂（模拟SIGPIPE）
    ALARM = 1007,          ///< 定时器信号（模拟SIGALRM）
    HANGUP = 1008,         ///< 挂起信号（模拟SIGHUP）
    USER1 = 1009,          ///< 用户自定义信号1（模拟SIGUSR1）
    USER2 = 1010,          ///< 用户自定义信号2（模拟SIGUSR2）

    TIMEOUT = 2000,   ///< 超时事件
    CUSTOM_1 = 2001,  ///< 自定义事件1
    CUSTOM_2 = 2002,  ///< 自定义事件2
    FORCE_EXIT = 9999 ///< 强制退出信号
#else
    INTERRUPT = SIGINT,      ///< Ctrl+C中断信号
    TERMINATE = SIGTERM,     ///< 终止信号
    ABORT = SIGABRT,         ///< 中止信号
    ILLEGAL_INSTR = SIGILL,  ///< 非法指令
    FLOATING_POINT = SIGFPE, ///< 浮点异常
    SEGMENT_FAULT = SIGSEGV, ///< 段错误
    BUS_ERROR = SIGBUS,      ///< 总线错误
    PIPE_BROKEN = SIGPIPE,   ///< 管道破裂
    ALARM = SIGALRM,         ///< 定时器信号
    HANGUP = SIGHUP,         ///< 挂起信号
    USER1 = SIGUSR1,         ///< 用户自定义信号1
    USER2 = SIGUSR2,         ///< 用户自定义信号2

    CTRL_BREAK = 1000, ///< Ctrl+Break（模拟）
    CLOSE = 1001,      ///< 关闭事件（模拟）
    LOGOFF = 1002,     ///< 注销事件（模拟）
    SHUTDOWN = 1003,   ///< 关机事件（模拟）

    TIMEOUT = 2000,   ///< 超时事件
    CUSTOM_1 = 2001,  ///< 自定义事件1
    CUSTOM_2 = 2002,  ///< 自定义事件2
    FORCE_EXIT = 9999 ///< 强制退出信号
#endif
};


/**
 * @class signal_manager
 * @brief 信号管理器
 *
 * 统一管理跨平台的信号处理，提供异步信号分发机制。
 *
 * 特性：
 * - 单例模式
 * - 线程安全的信号注册和分发
 * - 支持信号队列
 * - 支持超时信号
 * - 支持自定义事件
 * - 信号守卫
 */
class NEFORCE_API signal_manager {
public:
    /**
     * @brief 信号处理函数类型
     *
     * 返回true表示继续运行，false表示退出
     */
    using signal_handler = function<bool(signal_event, void*)>;

private:
    /**
     * @struct signal_result
     * @brief 信号等待结果
     */
    struct signal_result {
        signal_event event; ///< 信号事件
        void* context;      ///< 上下文数据
    };

    /**
     * @struct pending_signal
     * @brief 待处理信号
     */
    struct pending_signal {
        signal_event event;                 ///< 信号事件
        void* context;                      ///< 上下文数据
        steady_clock::time_point timestamp; ///< 时间戳

        pending_signal(const signal_event event, void* context, steady_clock::time_point timestamp) :
        event{event},
        context{context},
        timestamp{timestamp} {}
    };

    atomic<bool> running_{false};          ///< 运行标志
    atomic<bool> force_exit_{false};       ///< 强制退出标志
    atomic<int> force_exit_timeout_{5000}; ///< 强制退出超时（毫秒）

    mutex mutex_;           ///< 保护共享数据的互斥锁
    condition_variable cv_; ///< 信号等待条件变量

    unordered_map<signal_event, signal_handler> handlers_; ///< 信号处理函数映射
    vector<pending_signal> pending_signals_;               ///< 待处理信号队列

#ifdef NEFORCE_PLATFORM_WINDOWS
    vector<::DWORD> registered_windows_events_; ///< 已注册的Windows事件
#else
    struct ::sigaction old_actions_[64]{}; ///< 保存原有信号处理器
    ::timer_t alarm_timer_{nullptr};       ///< 定时器
#endif

    thread signal_thread_;  ///< 信号处理线程
    thread timeout_thread_; ///< 超时监控线程

private:
    void initialize_platform();
    void cleanup_platform() const;

    void signal_thread_func();
    void timeout_monitor_thread();

    void process_signal(signal_event event, void* context = nullptr);

    signal_result wait_for_signal_internal(int timeout_ms = -1);
    void send_signal_nolock(signal_event event, void* context = nullptr);

    signal_manager();

public:
    signal_manager(const signal_manager&) = delete;
    signal_manager& operator=(const signal_manager&) = delete;
    signal_manager(const signal_manager&&) = delete;
    signal_manager& operator=(const signal_manager&&) = delete;

    /**
     * @brief 析构函数
     */
    ~signal_manager();

    /**
     * @brief 获取单例实例
     * @return 信号管理器实例引用
     */
    static signal_manager& instance() {
        static signal_manager instance;
        return instance;
    }

    /**
     * @brief 注册信号处理函数
     * @param event 信号事件
     * @param handler 处理函数
     * @throws system_exception handler为空时抛出
     */
    void register_handler(signal_event event, signal_handler handler);

    /**
     * @brief 批量注册信号处理函数
     * @param events 信号事件列表
     * @param handler 处理函数
     */
    void register_handlers(const vector<signal_event>& events, const signal_handler& handler);

    /**
     * @brief 移除信号处理函数
     * @param event 信号事件
     */
    void remove_handler(signal_event event);

    /**
     * @brief 等待信号
     * @param timeout_ms 超时时间（毫秒），-1表示无限等待
     * @return 收到的信号事件
     *
     * 阻塞当前线程直到收到信号或超时。
     */
    signal_event wait_for_signal(int timeout_ms = -1);

    /**
     * @brief 发送信号
     * @param event 信号事件
     * @param context 上下文数据（可选）
     */
    void send_signal(signal_event event, void* context = nullptr);

    /**
     * @brief 设置强制退出超时时间
     * @param timeout_ms 超时时间（毫秒）
     *
     * 超过此时间未处理的信号将被丢弃。
     */
    void set_force_exit_timeout(int timeout_ms);

    /**
     * @brief 启动信号监控
     *
     * 启动信号处理线程和超时监控线程。
     */
    void start_monitoring();

    /**
     * @brief 停止信号监控
     *
     * 停止所有监控线程。
     */
    void stop_monitoring();

    /**
     * @brief 检查是否正在运行
     * @return 正在运行返回true
     */
    NEFORCE_NODISCARD bool is_running() const;

    /**
     * @brief 阻塞信号
     * @param signals_to_block 要阻塞的信号列表
     * @return 操作成功返回true（仅Linux有效）
     */
    bool block_signals(const vector<signal_event>& signals_to_block) const;

    /**
     * @brief 解除信号阻塞
     * @param signals_to_unblock 要解除阻塞的信号列表
     * @return 操作成功返回true（仅Linux有效）
     */
    bool unblock_signals(const vector<signal_event>& signals_to_unblock) const;

    /**
     * @brief 检查是否为平台原生信号
     * @param event 信号事件
     * @return 是平台原生信号返回true
     */
    NEFORCE_NODISCARD static bool is_platform_signal(signal_event event) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return static_cast<::DWORD>(event) <= CTRL_SHUTDOWN_EVENT;
#else
        const int value = static_cast<int>(event);
        return value > 0 && value < 64 && value != SIGALRM;
#endif
    }
};


/**
 * @class signal_guard
 * @brief 信号守卫
 *
 * 在构造时启动信号监控，析构时停止信号监控。
 */
class signal_guard {
public:
    /**
     * @brief 构造函数
     *
     * 启动信号监控。
     */
    signal_guard() { signal_manager::instance().start_monitoring(); }

    /**
     * @brief 析构函数
     *
     * 停止信号监控。
     */
    ~signal_guard() { signal_manager::instance().stop_monitoring(); }
};

/** @} */ // SystemSignal

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_SIGNAL_HPP__
