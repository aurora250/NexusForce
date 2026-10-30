#ifndef NEFORCE_CORE_SYSTEM_SIGNAL_HPP__
#define NEFORCE_CORE_SYSTEM_SIGNAL_HPP__

/**
 * @file signal.hpp
 * @brief 跨平台信号处理系统
 *
 * 提供线程安全、异步信号安全的统一信号管理接口。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/clocks.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <consoleapi.h>
#    ifdef NEFORCE_COMPILER_MINGW
#        include <windef.h>
#        include <wingdi.h>
#        include <wincon.h>
#    endif
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <csignal>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SystemSignal 系统信号
 * @brief 系统信号处理
 * @{
 */

/**
 * @class system_signal
 * @brief 系统信号
 *
 * 统一管理操作系统的信号处理，提供安全的分发机制。
 *
 * 特性：
 * - 异步信号安全
 * - 线程安全
 * - 信号队列与超时清理
 * - 自定义事件与强制退出
 */
class NEFORCE_API system_signal {
public:
    /**
     * @enum event
     * @brief 信号事件枚举
     *
     * 统一的信号事件定义。
     */
    enum class event {
#ifdef NEFORCE_PLATFORM_WINDOWS
        interrupt = CTRL_C_EVENT,       ///< Ctrl+C中断信号
        ctrl_break = CTRL_BREAK_EVENT,  ///< Ctrl+Break信号
        close = CTRL_CLOSE_EVENT,       ///< 关闭控制台窗口
        logoff = CTRL_LOGOFF_EVENT,     ///< 用户注销
        shutdown = CTRL_SHUTDOWN_EVENT, ///< 系统关机

        terminate = 1000,      ///< 终止信号（模拟SIGTERM）
        abort = 1001,          ///< 中止信号（模拟SIGABRT）
        illegal_instr = 1002,  ///< 非法指令（模拟SIGILL）
        floating_point = 1003, ///< 浮点异常（模拟SIGFPE）
        segment_fault = 1004,  ///< 段错误（模拟SIGSEGV）
        bus_error = 1005,      ///< 总线错误（模拟SIGBUS）
        pipe_broken = 1006,    ///< 管道破裂（模拟SIGPIPE）
        alarm = 1007,          ///< 定时器信号（模拟SIGALRM）
        hangup = 1008,         ///< 挂起信号（模拟SIGHUP）
        user1 = 1009,          ///< 用户自定义信号1（模拟SIGUSR1）
        user2 = 1010,          ///< 用户自定义信号2（模拟SIGUSR2）

        timeout = 2000,   ///< 超时事件
        custom_1 = 2001,  ///< 自定义事件1
        custom_2 = 2002,  ///< 自定义事件2
        force_exit = 9999 ///< 强制退出信号
#else
        interrupt = SIGINT,      ///< Ctrl+C中断信号
        terminate = SIGTERM,     ///< 终止信号
        abort = SIGABRT,         ///< 中止信号
        illegal_instr = SIGILL,  ///< 非法指令
        floating_point = SIGFPE, ///< 浮点异常
        segment_fault = SIGSEGV, ///< 段错误
        bus_error = SIGBUS,      ///< 总线错误
        pipe_broken = SIGPIPE,   ///< 管道破裂
        alarm = SIGALRM,         ///< 定时器信号
        hangup = SIGHUP,         ///< 挂起信号
        user1 = SIGUSR1,         ///< 用户自定义信号1
        user2 = SIGUSR2,         ///< 用户自定义信号2

        ctrl_break = 1000, ///< Ctrl+Break（模拟）
        close = 1001,      ///< 关闭事件（模拟）
        logoff = 1002,     ///< 注销事件（模拟）
        shutdown = 1003,   ///< 关机事件（模拟）

        timeout = 2000,   ///< 超时事件
        custom_1 = 2001,  ///< 自定义事件1
        custom_2 = 2002,  ///< 自定义事件2
        force_exit = 9999 ///< 强制退出信号
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
        system_signal::event event; ///< 信号事件
        void* context;              ///< 上下文数据
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

    struct impl;
    unique_ptr<impl> impl_;

    friend class signal_guard;

    system_signal();

    void add_guard_ref();
    void remove_guard_ref() noexcept;

    void initialize();
    void cleanup();

    void signal_thread_func();
    void timeout_monitor_thread();

    void process_signal(event event, void* context);
    void trigger_force_exit();
    signal_result wait_for_signal_internal(int timeout_ms = -1);
    void send_signal_nolock(event event, void* context);

public:
    system_signal(const system_signal&) = delete;
    system_signal& operator=(const system_signal&) = delete;
    system_signal(const system_signal&&) = delete;
    system_signal& operator=(const system_signal&&) = delete;

    ~system_signal();

    /**
     * @brief 获取单例实例
     * @return 信号管理器实例引用
     */
    static system_signal& instance();

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
     * @brief 阻塞指定信号
     * @param signals_to_block 要阻塞的信号集合
     * @return 是否成功
     * @note 仅Linux支持
     */
    static bool block_signals(const vector<event>& signals_to_block);

    /**
     * @brief 解除阻塞指定信号
     * @param signals_to_unblock 要解除的信号集合
     * @return 是否成功
     * @note 仅Linux支持
     */
    static bool unblock_signals(const vector<event>& signals_to_unblock);

    /**
     * @brief 判断事件是否为平台原生信号
     * @param event 事件
     * @return 是否平台原生信号
     */
    NEFORCE_NODISCARD static bool is_platform_signal(event event) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return static_cast<::DWORD>(event) <= CTRL_SHUTDOWN_EVENT;
#else
        const int v = static_cast<int>(event);
        return v > 0 && v < 64;
#endif
    }

    /**
     * @brief 将平台原生信号转换为统一事件
     * @param native_signal 平台原生信号值
     * @return 对应的统一事件，无法映射时返回原值强转
     */
    NEFORCE_NODISCARD static event from_native(int native_signal) noexcept;

    /**
     * @brief 将统一事件转换为平台原生信号
     * @param e 统一事件
     * @return 平台原生信号值，模拟事件返回 -1
     */
    NEFORCE_NODISCARD static int to_native(event e) noexcept;
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
    signal_guard() { system_signal::instance().add_guard_ref(); }

    /**
     * @brief 析构函数
     *
     * 停止信号监控。
     */
    ~signal_guard() { system_signal::instance().remove_guard_ref(); }

    signal_guard(const signal_guard&) = delete;
    signal_guard& operator=(const signal_guard&) = delete;
    signal_guard(signal_guard&&) = delete;
    signal_guard& operator=(signal_guard&&) = delete;
};

/** @} */ // SystemSignal

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_SIGNAL_HPP__
