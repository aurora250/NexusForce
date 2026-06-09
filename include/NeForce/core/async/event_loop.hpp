#ifndef NEFORCE_CORE_ASYNC_EVENT_LOOP_HPP__
#define NEFORCE_CORE_ASYNC_EVENT_LOOP_HPP__

/**
 * @file event_loop.hpp
 * @brief 单线程 I/O 事件循环
 *
 * Reactor 模式事件循环，监控文件描述符和定时器。
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/// @brief 监控可读事件
NEFORCE_INLINE17 constexpr uint32_t epoll_in = 0x001;
/// @brief 监控可写事件
NEFORCE_INLINE17 constexpr uint32_t epoll_out = 0x004;
/// @brief 边沿触发模式
NEFORCE_INLINE17 constexpr uint32_t epoll_et = 0x80000000;

/**
 * @class event_loop
 * @brief 单线程 I/O 事件循环
 *
 * 监控已注册的文件描述符的可读/可写事件，并在事件发生时调度回调。
 * 定时器以最小堆管理，到期时自动触发。
 *
 * @note 所有回调在事件循环线程中执行，长耗时回调会阻塞 I/O 分发。
 * @warning 不支持从其他线程注册 fd；通过 wake() + 外部队列实现跨线程操作。
 */
class NEFORCE_API event_loop {
public:
    /// @brief fd 事件回调
    using fd_callback = function<void(int fd, uint32_t events)>;
    /// @brief 定时器到期回调
    using timer_callback = function<void()>;

private:
    struct timer_entry {
        size_t id;
        uint64_t deadline_ms;
        timer_callback callback;
        bool operator>(const timer_entry& other) const { return deadline_ms > other.deadline_ms; }
    };
    struct fd_info {
        int fd;
        uint32_t events;
        fd_callback callback;
    };

    /// @brief 事件循环是否正在运行
    bool running_ = false;
    /// @brief 定时器 ID 计数器（单调递增）
    size_t next_timer_id_ = 1;

    /// @brief fd → 注册信息的映射
    unordered_map<int, fd_info> fd_map_;
    /// @brief 定时器最小堆（最早到期者位于堆顶）
    vector<timer_entry> timer_heap_;
    /// @brief 保护 timer_heap_ 的互斥锁
    mutable mutex timer_mutex_;

    void process_timers();
    uint64_t next_timer_deadline() const;
    void wake();

#ifdef NEFORCE_PLATFORM_WINDOWS
    /// @brief IOCP 完成端口句柄
    void* iocp_handle_;
    /// @brief 用于唤醒监控线程的手动重置事件
    void* wake_event_;
    /// @brief fd → WSA 事件句柄的映射
    unordered_map<int, void*> fd_events_;
    /// @brief 后台 WSAWaitForMultipleEvents 监控线程
    thread monitor_thread_;
    /// @brief 保护 fd_events_ 的互斥锁
    mutex fd_mutex_;
    /// @brief 监控线程是否正在运行
    bool monitor_running_ = false;

    void monitor_loop();
#else
    /// @brief epoll 文件描述符
    int epoll_fd_;
    /// @brief 用于唤醒的事件通知 fd
    int wake_fd_;
#endif

public:
    /**
     * @brief 构造事件循环并初始化操作系统后端
     */
    event_loop();

    /**
     * @brief 停止事件循环并释放操作系统资源
     */
    ~event_loop();

    event_loop(const event_loop&) = delete;
    event_loop& operator=(const event_loop&) = delete;

    /**
     * @brief 注册文件描述符进行监控
     * @param fd 要监控的文件描述符
     * @param events epoll_in / epoll_out 的事件掩码
     * @param cb fd 上有事件发生时的回调
     * @note fd 必须在 remove_fd() 调用前保持有效
     */
    void add_fd(int fd, uint32_t events, fd_callback cb);

    /**
     * @brief 修改已注册 fd 的事件掩码
     * @param fd 已注册的文件描述符
     * @param events 新的事件掩码（覆盖旧值）
     */
    void mod_fd(int fd, uint32_t events);

    /**
     * @brief 取消文件描述符的监控
     * @param fd 已注册的文件描述符
     */
    void remove_fd(int fd);

    /**
     * @brief 调度一次性定时器
     * @param delay_ms 以当前时间为基准的延迟毫秒数
     * @param cb 定时器到期时的回调
     * @return 定时器 ID，可用于 cancel_timer()
     */
    size_t schedule_timer(uint64_t delay_ms, timer_callback cb);

    /**
     * @brief 取消待执行的定时器
     * @param timer_id schedule_timer() 返回的定时器 ID
     * @return 找到并取消返回 true，已触发返回 false
     */
    bool cancel_timer(size_t timer_id);

    /**
     * @brief 运行事件循环（阻塞直到 stop() 被调用）
     * @note 此方法会无限期阻塞调用线程
     */
    void run();

    /**
     * @brief 执行事件循环的单次迭代
     * @param timeout_ms 等待事件的最大阻塞时间（-1 表示无超时）
     */
    void run_once(int timeout_ms = 0);

    /**
     * @brief 停止事件循环
     * @note 线程安全，可从任意线程调用
     */
    void stop();

    /// @brief 检查事件循环当前是否正在运行
    bool is_running() const noexcept { return running_; }
};

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_EVENT_LOOP_HPP__
