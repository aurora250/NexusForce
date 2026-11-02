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
#include "NeForce/core/string/string_view.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/utility/optional.hpp"
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
 *
 * @note Windows 仅支持向进程发送 terminate/kill
 */
class NEFORCE_API system_signal {
public:
    /**
     * @enum signal
     * @brief 可发送系统信号
     */
    enum class signal : int32_t {
        terminate,       ///< 请求终止
        kill,            ///< 强制终止
        interrupt,       ///< 中断
        quit,            ///< 退出
        hangup,          ///< 挂断
        stop,            ///< 暂停
        resume,          ///< 继续
        user1,           ///< 用户信号 1
        user2,           ///< 用户信号 2
        abort,           ///< 中止
        illegal_instr,   ///< 非法指令
        floating_point,  ///< 浮点异常
        segment_fault,   ///< 段错误
        bus_error,       ///< 总线错误
        pipe_broken,     ///< 管道破裂
        alarm,           ///< 定时器
        trap,            ///< 调试陷阱
        child,           ///< 子进程状态变化
        terminal_stop,   ///< 终端暂停
        terminal_input,  ///< 后台读终端
        terminal_output, ///< 后台写终端
        urgent,          ///< 紧急数据
        cpu_limit,       ///< CPU 时间限制
        file_limit,      ///< 文件大小限制
        virtual_alarm,   ///< 虚拟定时器
        profiling,       ///< 性能分析定时器
        window_change,   ///< 窗口大小变化
        io,              ///< 异步 I/O
        power,           ///< 电源故障
        bad_syscall      ///< 错误系统调用
    };

    /**
     * @enum event
     * @brief 事件枚举
     */
    enum class event : int32_t {
        ctrl_break = 1000, ///< Ctrl+Break
        close = 1001,      ///< 关闭控制台窗口
        logoff = 1002,     ///< 用户注销
        shutdown = 1003,   ///< 系统关机
        timeout = 2000,    ///< 超时事件
        custom_1 = 2001,   ///< 自定义事件 1
        custom_2 = 2002,   ///< 自定义事件 2
        force_exit = 9999  ///< 强制退出事件
    };

    /**
     * @struct notification
     * @brief 接收侧通知
     *
     * 携带系统信号或本地事件。
     */
    struct notification {
        /**
         * @enum kind
         * @brief 通知种类
         */
        enum class kind {
            signal, ///< 系统信号
            event   ///< 本地事件
        };

    private:
        kind kind_{kind::signal};          ///< 通知种类
        signal signal_{signal::terminate}; ///< 系统信号值
        event event_{event::timeout};      ///< 本地事件值

    public:
        /**
         * @brief 默认构造
         */
        constexpr notification() noexcept = default;

        /**
         * @brief 从系统信号构造通知
         * @param value 系统信号
         */
        constexpr notification(const signal value) noexcept :
        signal_(value) {}

        /**
         * @brief 从本地事件构造通知
         * @param value 本地事件
         */
        constexpr notification(const event value) noexcept :
        kind_(kind::event),
        event_(value) {}

        /**
         * @brief 从系统信号创建通知
         * @param value 系统信号
         * @return 信号通知
         */
        NEFORCE_NODISCARD static constexpr notification from_signal(const signal value) noexcept {
            return notification{value};
        }

        /**
         * @brief 从本地事件创建通知
         * @param value 本地事件
         * @return 事件通知
         */
        NEFORCE_NODISCARD static constexpr notification from_event(const event value) noexcept {
            return notification{value};
        }

        /**
         * @brief 是否为系统信号通知
         * @return 是返回 true
         */
        NEFORCE_NODISCARD constexpr bool is_signal() const noexcept { return kind_ == kind::signal; }

        /**
         * @brief 是否为本地事件通知
         * @return 是返回 true
         */
        NEFORCE_NODISCARD constexpr bool is_event() const noexcept { return kind_ == kind::event; }

        /**
         * @brief 获取系统信号值
         * @return 系统信号；仅当 is_signal() 为 true 时有意义
         */
        NEFORCE_NODISCARD constexpr signal signal_value() const noexcept { return signal_; }

        /**
         * @brief 获取本地事件值
         * @return 本地事件；仅当 is_event() 为 true 时有意义
         */
        NEFORCE_NODISCARD constexpr event event_value() const noexcept { return event_; }

        friend constexpr bool operator==(const notification&, const notification&) noexcept = default;
    };

    /**
     * @brief 通知处理函数类型
     * @param notification 收到的通知
     * @param context 上下文指针
     * @return true 继续运行；false 请求退出
     */
    using notification_handler = function<bool(notification, void*)>;

private:
    /**
     * @struct notification_result
     * @brief 通知等待结果
     */
    struct notification_result {
        bool has_value{false};  ///< 是否收到通知
        notification value;     ///< 通知
        void* context{nullptr}; ///< 上下文数据
    };

    /**
     * @struct pending_notification
     * @brief 待处理通知
     */
    struct pending_notification {
        notification value;                 ///< 通知
        void* context;                      ///< 上下文数据
        steady_clock::time_point timestamp; ///< 时间戳

        pending_notification(const notification value, void* context, const steady_clock::time_point timestamp) :
        value{value},
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

    void install_signal_handler(int native_signal);
    void restore_signal_handler(int native_signal);
    void restore_all_signal_handlers();

    void process_notification(const notification& value, void* context);
    void trigger_force_exit();
    notification_result wait_for_notification_internal(int timeout_ms = -1);
    void enqueue_notification_nolock(const notification& value, void* context);

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
     * @brief 查询当前平台是否支持向进程发送指定信号
     * @param sig 发送侧系统信号
     * @return 支持返回 true
     */
    NEFORCE_NODISCARD static bool can_send(signal sig) noexcept;

    /**
     * @brief 设置是否链式调用订阅前已有的信号处理器
     * @param enabled 是否启用
     * @note 默认启用；仅对订阅前安装的自定义 POSIX handler 生效
     */
    static void set_chain_existing_handlers(bool enabled) noexcept;

    /**
     * @brief 查询是否启用信号处理器链式调用
     * @return 是否启用
     */
    NEFORCE_NODISCARD static bool chain_existing_handlers() noexcept;

    /**
     * @brief 向目标进程发送发送侧系统信号
     * @param process_id 目标进程 ID
     * @param sig 发送侧系统信号
     * @return 发送成功返回 true
     */
    static bool send_to_process(int process_id, signal sig) noexcept;

    /**
     * @brief 获取发送侧信号名称
     * @param sig 发送侧系统信号
     * @return 不含 SIG 前缀的名称，如 "TERM"
     */
    NEFORCE_NODISCARD static string_view signal_name(signal sig) noexcept;

    /**
     * @brief 获取发送侧信号的原生编号
     * @param sig 发送侧系统信号
     * @return POSIX 原生信号编号
     */
    NEFORCE_NODISCARD static int signal_number(signal sig) noexcept;

    /**
     * @brief 从原生编号创建发送侧信号
     * @param native_signal 原生信号编号
     * @return 可发送信号；未知编号返回空
     */
    NEFORCE_NODISCARD static optional<signal> signal_from_number(int native_signal) noexcept;

    /**
     * @brief 向目标进程发送平台原生信号
     * @param process_id 目标进程 ID
     * @param native_signal 原生信号编号
     * @return 发送成功返回 true
     */
    static bool send_native(int process_id, int native_signal) noexcept;

    /**
     * @brief 订阅系统信号
     * @param sig 系统信号
     * @param handler 处理函数
     * @throws system_exception handler 为空时抛出
     */
    void subscribe(signal sig, notification_handler handler);

    /**
     * @brief 批量订阅系统信号
     * @param signals 系统信号列表
     * @param handler 处理函数
     */
    void subscribe(const vector<signal>& signals, const notification_handler& handler);

    /**
     * @brief 订阅本地事件
     * @param event 本地事件
     * @param handler 处理函数
     * @throws system_exception handler 为空时抛出
     */
    void subscribe(event event, notification_handler handler);

    /**
     * @brief 批量订阅本地事件
     * @param events 本地事件列表
     * @param handler 处理函数
     */
    void subscribe(const vector<event>& events, const notification_handler& handler);

    /**
     * @brief 取消系统信号订阅
     * @param sig 系统信号
     */
    void unsubscribe(signal sig);

    /**
     * @brief 取消本地事件订阅
     * @param event 本地事件
     */
    void unsubscribe(event event);

    /**
     * @brief 等待接收侧通知
     * @param timeout_ms 超时时间（毫秒），-1 表示无限等待
     * @return 收到的通知；超时返回空
     */
    NEFORCE_NODISCARD optional<notification> wait_for_notification(int timeout_ms = -1);

    /**
     * @brief 将系统信号通知投递到当前进程队列
     * @param sig 系统信号
     * @param context 上下文数据（可选）
     * @note 不向操作系统发送真实信号
     */
    void enqueue_signal(signal sig, void* context = nullptr);

    /**
     * @brief 将本地事件投递到当前进程队列
     * @param event 本地事件
     * @param context 上下文数据（可选）
     */
    void enqueue_event(event event, void* context = nullptr);

    /**
     * @brief 设置强制退出超时时间
     * @param timeout_ms 超时时间（毫秒）
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
     * @return 正在运行返回 true
     */
    NEFORCE_NODISCARD bool is_running() const;

    /**
     * @brief 阻塞发送侧信号
     * @param signals 要阻塞的发送侧信号集合
     * @return 是否成功
     * @note 仅 Linux 支持
     */
    static bool block_signals(const vector<signal>& signals);

    /**
     * @brief 解除阻塞发送侧信号
     * @param signals 要解除阻塞的发送侧信号集合
     * @return 是否成功
     * @note 仅 Linux 支持
     */
    static bool unblock_signals(const vector<signal>& signals);

    /**
     * @brief 判断通知是否对应平台原生信号
     * @param value 通知
     * @return 是否为平台原生信号
     */
    NEFORCE_NODISCARD static bool is_native_notification(const notification& value) noexcept;

    /**
     * @brief 从平台原生信号创建通知
     * @param native_signal 原生信号编号
     * @return 系统信号或控制台事件通知；未知编号返回空
     */
    NEFORCE_NODISCARD static optional<notification> notification_from_native(int native_signal) noexcept;

    /**
     * @brief 将通知转换为平台原生信号
     * @param value 通知
     * @return 平台原生编号；本地事件返回 -1
     */
    NEFORCE_NODISCARD static int notification_to_native(const notification& value) noexcept;

    /**
     * @brief 获取本地事件名称
     * @param event 本地事件
     * @return 事件名称，如 "TIMEOUT"
     */
    NEFORCE_NODISCARD static string_view event_name(event event) noexcept;

    /**
     * @brief 获取通知名称
     * @param value 通知
     * @return 系统信号名称或本地事件名称
     */
    NEFORCE_NODISCARD static string_view notification_name(const notification& value) noexcept;
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
