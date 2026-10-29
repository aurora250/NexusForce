#ifndef NEFORCE_CORE_ASYNC_IO_CONTEXT_HPP__
#define NEFORCE_CORE_ASYNC_IO_CONTEXT_HPP__

/**
 * @file io_context.hpp
 * @brief 统一异步操作核心
 *
 * io_context 是 NeForce 异步架构的枢纽——合并 I/O 多路复用、定时器管理和工作投递
 * 于单一执行上下文中。所有异步 socket、定时器、文件 I/O 操作均以 io_context 为调度目标。
 *
 * 核心能力：
 *  - post() / dispatch() 任意工作投递
 *  - 定时器（min-heap，在 run() 循环中到期触发，无需独立线程）
 *  - fd 事件监控（epoll / IOCP）
 *  - 多线程 run_pool() 并发执行
 *  - executor 抽象 + work 守卫
 */

#include "NeForce/core/async/heap_timer.hpp"
#include "NeForce/core/async/lock_free_queue.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/flat_unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/exception/error_code.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup IOContext IO上下文
 * @brief 统一异步操作核心
 * @{
 */

/// @brief epoll 事件标志
enum class epoll_event : uint32_t {
    none = 0,
    in = 0x001,      ///< 监控可读事件
    out = 0x004,     ///< 监控可写事件
    et = 0x80000000, ///< 边沿触发模式
};

constexpr epoll_event operator|(epoll_event a, epoll_event b) noexcept {
    return static_cast<epoll_event>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr epoll_event operator&(epoll_event a, epoll_event b) noexcept {
    return static_cast<epoll_event>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}


/**
 * @class io_context
 * @brief 统一异步操作核心
 *
 * 合并 I/O 多路复用（epoll/IOCP）、定时器管理和工作投递。
 * 所有异步操作以此为调度目标，替代分散的 event_loop 和 thread_pool 实例。
 *
 * 使用示例：
 * @code
 * io_context ctx;
 * ctx.post([] { println("async work"); });
 * ctx.schedule_timer(1000, [] { println("1 second later"); });
 * ctx.run(); // 驱动事件循环
 * @endcode
 *
 * @note 单线程 run() 时所有 handler 串行执行，无需加锁。多线程 run_pool() 时需 strand。
 * @warning 跨线程 post() 是线程安全的；add_fd/remove_fd 必须在 run() 所在线程调用。
 */
class NEFORCE_API io_context {
public:
    /// @brief 通用 handler 类型
    using handler_type = function<void()>;
    /// @brief fd 事件回调，参数为 (fd, events, error_code)
    using fd_callback = function<void(int fd, epoll_event events, error_code ec)>;
    /// @brief 定时器到期回调
    using timer_callback = function<void()>;

    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
            uintptr_t; ///< 平台原生句柄类型
#else
            int; ///< 平台原生句柄类型
#endif

    class executor;
    class work;

    /**
     * @brief 构造 io_context
     */
    io_context();

    /**
     * @brief 析构
     *
     * 自动 stop 并释放资源
     */
    ~io_context();

    io_context(const io_context&) = delete;
    io_context& operator=(const io_context&) = delete;

    /**
     * @brief 获取默认执行器
     */
    executor get_executor() noexcept;

    /**
     * @brief 将 handler 投递到队列末尾
     * @param handler 要执行的回调
     */
    void post(handler_type handler);

    /**
     * @brief 若当前在 io_context 线程上则立即执行 handler，否则 post
     * @param handler 要执行的回调
     */
    void dispatch(handler_type handler);

    /**
     * @brief 登记一项未完成的异步操作
     * @note 必须与 work_finished() 严格配对，且每次配对只对应一项操作。
     */
    void work_started() noexcept;

    /**
     * @brief 标记一项异步操作已完成
     * @note 必须与 work_started() 严格配对，且每次配对只对应一项操作。
     */
    void work_finished() noexcept;

    /**
     * @brief 单线程阻塞驱动事件循环
     * @return 执行的 handler 数量
     *
     * 循环直到 stop() 被调用且无待处理工作。
     * 若需防止提前退出，使用 io_context::work。
     */
    size_t run();

    /**
     * @brief 多线程并发驱动事件循环
     * @param n 线程数
     *
     * 启动 n 个线程同时调用 run()。
     */
    void run_pool(size_t n);

    /**
     * @brief 等待并执行一个就绪的 handler
     * @param timeout_ms 最大等待时间（ms），-1 表示无限等待
     * @return 执行的 handler 数量（0 或 1）
     * @note 依赖外部事件而尚未登记工作的场景，应使用 io_context::work 守卫或先调用 work_started()
     */
    size_t run_one(int timeout_ms = -1);

    /**
     * @brief 执行所有已就绪的 handler 后立即返回
     * @return 执行的 handler 数量
     */
    size_t poll();

    /**
     * @brief 非阻塞单次轮询
     * @return 执行的 handler 数量
     */
    size_t poll_one() { return run_one(0); }

    /**
     * @brief 停止事件循环
     *
     * 设置停止标志并唤醒 run()。正在执行的 handler 会完成，不再启动新的迭代。
     */
    void stop();

    /**
     * @brief 检查是否已停止
     */
    bool stopped() const noexcept { return stopped_.load(memory_order_acquire); }

    /**
     * @brief 重置停止标志，允许再次 run()
     */
    void restart() { stopped_.store(false, memory_order_release); }

    /**
     * @brief 调度一次性定时器
     * @param delay_ms 延迟毫秒数
     * @param handler 到期时执行的回调（在 run() 线程中执行）
     * @return 定时器 ID，可用于 cancel_timer()
     * @note 回调会被拷贝、需要重新排期时应安装新闭包
     */
    size_t schedule_timer(uint64_t delay_ms, timer_callback handler);

    /**
     * @brief 取消待执行的定时器
     * @param timer_id schedule_timer() 返回的 ID
     * @return 找到并取消返回 true
     */
    bool cancel_timer(size_t timer_id);

    /**
     * @brief 注册 fd 进行事件监控
     * @param fd 要监控的文件描述符
     * @param events epoll_event 事件
     * @param cb 事件就绪时的回调
     * @param edge_triggered 是否使用边缘触发模式，默认 true
     */
    void add_fd(native_handle_type fd, epoll_event events, fd_callback cb, bool edge_triggered = true);

    /**
     * @brief 修改已注册 fd 的事件掩码
     * @param fd 已注册的文件描述符
     * @param events 新的事件掩码
     * @param edge_triggered 是否使用边缘触发模式，默认 true
     */
    void mod_fd(native_handle_type fd, epoll_event events, bool edge_triggered = true);

    /**
     * @brief 取消 fd 的事件监控
     * @param fd 已注册的文件描述符
     */
    void remove_fd(native_handle_type fd);

private:
    friend class file_async;

    using file_completion_cb = function<void(error_code, size_t, void* overlapped)>;

    /**
     * @brief fd 注册信息
     */
    struct fd_info {
        native_handle_type fd;
        epoll_event events;
        fd_callback callback;
    };

    atomic<int> running_{0};             ///< 正在运行的线程数
    atomic<bool> stopped_{false};        ///< 停止标志
    atomic<size_t> outstanding_work_{0}; ///< 待处理工作数
    atomic<size_t> registered_fds_{0};   ///<  add_fd() 注册的 fd 数量

    flat_unordered_map<native_handle_type, fd_info> fd_map_;       ///< fd 到注册信息映射
    mutable mutex fd_mutex_;                                       ///< fd_map_ 锁
    heap_timer timers_;                                            ///< 定时器最小堆
    flat_unordered_map<uint64_t, timer_callback> timer_callbacks_; ///< 定时器 ID 到回调

    mutable mutex timer_mutex_;                    ///< timer_callbacks_ 锁
    lock_free_queue<handler_type> external_queue_; ///< 跨线程 post 的 handler 队列
    atomic<size_t> external_queue_count_{0};       ///< post 计数器

#ifdef NEFORCE_PLATFORM_WINDOWS
    void* iocp_handle_;                   ///< IOCP 完成端口句柄
    void* wake_event_;                    ///< monitor 线程唤醒事件
    thread monitor_thread_;               ///< WSA 监控线程
    atomic<bool> monitor_running_{false}; ///< 监控线程运行标志

    flat_unordered_map<native_handle_type, void*> fd_events_; ///< fd 到 WSA 事件句柄
    vector<void*> pending_close_;                             ///< WSAEVENT 句柄

    flat_unordered_map<uintptr_t, file_completion_cb> file_completions_; ///< 文件 I/O 完成回调

    void monitor_loop();

    void register_file_completion(uintptr_t key, file_completion_cb cb);
    void unregister_file_completion(uintptr_t key);

#elif defined(NEFORCE_PLATFORM_FREEBSD)
    // TODO: using neforce::pipe instead
    int wake_fd_;       ///< 唤醒管道读端
    int wake_write_fd_; ///< 唤醒管道写端
#else
    int epoll_fd_; ///< epoll 文件描述符
    int wake_fd_;  ///< 用于唤醒的 eventfd
#endif

    vector<thread> pool_threads_; ///< 工作线程列表
    mutex pool_mutex_;            ///< 保护线程管理的互斥锁

    NEFORCE_NODISCARD bool has_outstanding_work() const noexcept;
    NEFORCE_NODISCARD bool has_pending_work() const noexcept;

    uint64_t next_timer_deadline();
    bool fire_expired_timer();
    void process_timers(size_t max_count = numeric_traits<size_t>::max());

    void wake() noexcept;
    size_t drain_handlers(size_t max_count = 256);
};

/**
 * @class io_context::executor
 * @brief io_context 的执行器句柄
 *
 * 轻量级可拷贝句柄，引用关联的 io_context。
 * 满足 Executor 概念，可被 strand 和 co_spawn 使用。
 */
class io_context::executor {
public:
    executor(const executor& other) noexcept = default;
    executor& operator=(const executor& other) noexcept = default;

    /**
     * @brief 在关联的 io_context 上投递 handler
     */
    void execute(handler_type handler) const { ctx_->post(move(handler)); }

    /**
     * @brief 获取关联的 io_context
     */
    NEFORCE_NODISCARD io_context& context() const noexcept { return *ctx_; }

    /**
     * @brief 相等比较 — 同一 io_context 的执行器相等
     */
    bool operator==(const executor& other) const noexcept { return ctx_ == other.ctx_; }

    /**
     * @brief 不等比较
     */
    bool operator!=(const executor& other) const noexcept { return ctx_ != other.ctx_; }

private:
    friend class io_context;

    executor(io_context& ctx) noexcept :
    ctx_(&ctx) {}

    io_context* ctx_;
};

/**
 * @class io_context::work
 * @brief 阻止 io_context::run() 提前退出的守卫
 *
 * 构造时增加工作计数，析构时减少。
 * 只要有 work 对象存活或未完成的操作，run() 就不会返回。
 *
 * @note 典型用法：在启动 run() 之前创建 work 对象，
 *       在想要停止时销毁 work 对象并调用 stop()。
 */
class io_context::work {
private:
    io_context* ctx_;

public:
    /**
     * @brief 构造函数 — 增加 io_context 的工作计数
     */
    explicit work(io_context& ctx) :
    ctx_(&ctx) {
        ctx_->outstanding_work_.fetch_add(1, memory_order_relaxed);
    }

    /**
     * @brief 析构函数 — 减少工作计数
     */
    ~work() { ctx_->outstanding_work_.fetch_sub(1, memory_order_relaxed); }

    work(const work&) = delete;
    work& operator=(const work&) = delete;
};

/** @} */ // IOContext

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_IO_CONTEXT_HPP__
