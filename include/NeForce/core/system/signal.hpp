#ifndef NEFORCE_CORE_SYSTEM_SIGNAL_HPP__
#define NEFORCE_CORE_SYSTEM_SIGNAL_HPP__

/**
 * @file signal.hpp
 * @brief 跨平台信号处理系统
 *
 * 提供线程安全、异步信号安全的统一信号管理接口。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include "NeForce/core/system/system_event.hpp"
#    include <consoleapi.h>
#    ifdef NEFORCE_COMPILER_MINGW
#        include <windef.h>
#        include <wingdi.h>
#        include <wincon.h>
#    endif
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
 * @class system_signal_manager
 * @brief 信号管理器
 *
 * 统一管理跨平台的信号处理，提供异步信号安全的分发机制。
 *
 * 特性：
 * - 异步信号安全
 * - 线程安全
 * - 信号队列与超时清理
 * - 自定义事件与强制退出
 */
class NEFORCE_API system_signal_manager {
public:
    /**
     * @enum event
     * @brief 信号事件枚举
     *
     * 统一的信号事件定义。
     */
    enum class event {
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
     * @brief 信号处理函数类型
     * @param event 收到的信号事件
     * @param context 上下文指针
     * @return true 继续运行；false 请求退出
     */
    using signal_handler = function<bool(event, void*)>;

private:
    /**
     * @struct signal_result
     * @brief 信号等待结果
     */
    struct signal_result {
        system_signal_manager::event event; ///< 信号事件
        void* context;                      ///< 上下文数据
    };

    /**
     * @struct pending_signal
     * @brief 待处理信号
     */
    struct pending_signal {
        event signal_event;                 ///< 信号事件
        void* context;                      ///< 上下文数据
        steady_clock::time_point timestamp; ///< 时间戳

        pending_signal(const event event, void* context, const steady_clock::time_point timestamp) :
        signal_event{event},
        context{context},
        timestamp{timestamp} {}
    };

    atomic<bool> running_{false};          ///< 运行标志
    atomic<bool> force_exit_{false};       ///< 强制退出标志
    atomic<int> force_exit_timeout_{5000}; ///< 强制退出超时（毫秒）

    atomic<int> guard_count_{0}; // 作用域管理器的引用计数

    mutex mutex_;           ///< 保护共享数据的互斥锁
    condition_variable cv_; ///< 信号等待条件变量

    unordered_map<event, signal_handler> handlers_; ///< 信号处理函数映射
    vector<pending_signal> pending_signals_;        ///< 待处理信号队列

#ifdef NEFORCE_PLATFORM_WINDOWS
    system_event notify_event_; ///< 事件对象
#else
    struct ::sigaction old_actions_[64]{}; ///< 原始的信号处理行为
#endif

    thread signal_thread_;  ///< 信号处理线程
    thread timeout_thread_; ///< 超时监控线程

    friend class signal_guard;

    system_signal_manager();

    void add_guard_ref() {
        if (guard_count_++ == 0) {
            start_monitoring();
        }
    }

    void remove_guard_ref() noexcept {
        if (--guard_count_ == 0) {
            stop_monitoring();
        }
    }

    void initialize();
    void cleanup();

    void signal_thread_func();
    void timeout_monitor_thread();

    void process_signal(event event, void* context);
    void trigger_force_exit();
    signal_result wait_for_signal_internal(int timeout_ms = -1);
    void send_signal_nolock(event event, void* context);

public:
    system_signal_manager(const system_signal_manager&) = delete;
    system_signal_manager& operator=(const system_signal_manager&) = delete;
    system_signal_manager(const system_signal_manager&&) = delete;
    system_signal_manager& operator=(const system_signal_manager&&) = delete;

    ~system_signal_manager();

    /**
     * @brief 获取单例实例
     * @return 信号管理器实例引用
     */
    static system_signal_manager& instance() {
        static system_signal_manager instance;
        return instance;
    }

    /**
     * @brief 注册信号处理函数
     * @param event 信号事件
     * @param handler 处理函数
     * @throws system_exception handler为空时抛出
     */
    void register_handler(event event, signal_handler handler);

    /**
     * @brief 批量注册信号处理函数
     * @param events 信号事件列表
     * @param handler 处理函数
     */
    void register_handlers(const vector<event>& events, const signal_handler& handler);

    /**
     * @brief 移除信号处理函数
     * @param event 信号事件
     */
    void remove_handler(event event);

    /**
     * @brief 等待信号
     * @param timeout_ms 超时时间（毫秒），-1表示无限等待
     * @return 收到的信号事件
     *
     * 阻塞当前线程直到收到信号或超时。
     */
    event wait_for_signal(int timeout_ms = -1);

    /**
     * @brief 发送信号
     * @param event 信号事件
     * @param context 上下文数据（可选）
     */
    void send_signal(event event, void* context = nullptr);

    /**
     * @brief 设置强制退出超时时间
     * @param timeout_ms 超时时间（毫秒）
     *
     * 超过此时间未处理的信号将被丢弃。
     */
    void set_force_exit_timeout(int timeout_ms);

    /**
     * @brief 启动信号监控
     */
    void start_monitoring();

    /**
     * @brief 停止信号监控
     */
    void stop_monitoring() noexcept;

    /**
     * @brief 强制重置信号管理器状态
     * @warning 生产环境中不应使用此方法。
     */
    void reset_force();

    /**
     * @brief 检查是否正在运行
     * @return 正在运行返回true
     */
    NEFORCE_NODISCARD bool is_running() const;

    /**
     * @brief 阻塞指定信号（仅Linux）
     * @param signals_to_block 要阻塞的信号集合
     * @return 是否成功
     */
    bool block_signals(const vector<event>& signals_to_block) const;

    /**
     * @brief 解除阻塞指定信号（仅Linux）
     * @param signals_to_unblock 要解除的信号集合
     * @return 是否成功
     */
    bool unblock_signals(const vector<event>& signals_to_unblock) const;

    /**
     * @brief 判断事件是否为平台原生信号
     * @param event 事件
     * @return 是否平台原生信号
     */
    NEFORCE_NODISCARD static bool is_platform_signal(event event) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return static_cast<::DWORD>(event) <= CTRL_SHUTDOWN_EVENT;
#else
        int v = static_cast<int>(event);
        return v > 0 && v < 64;
#endif
    }
};

/**
 * @class signal_guard
 * @brief 信号守卫
 *
 * 构造时自动启动监控，析构时自动停止，保障资源安全释放。
 */
class signal_guard {
public:
    /**
     * @brief 构造函数
     *
     * 启动信号监控。
     */
    signal_guard() { system_signal_manager::instance().add_guard_ref(); }

    /**
     * @brief 析构函数
     *
     * 停止信号监控。
     */
    ~signal_guard() { system_signal_manager::instance().remove_guard_ref(); }

    signal_guard(const signal_guard&) = delete;
    signal_guard& operator=(const signal_guard&) = delete;
    signal_guard(signal_guard&&) = delete;
    signal_guard& operator=(signal_guard&&) = delete;
};

/** @} */ // SystemSignal

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_SIGNAL_HPP__
