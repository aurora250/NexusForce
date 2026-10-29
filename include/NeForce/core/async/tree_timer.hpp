#ifndef NEFORCE_CORE_ASYNC_TREE_TIMER_HPP__
#define NEFORCE_CORE_ASYNC_TREE_TIMER_HPP__

/**
 * @file tree_timer.hpp
 * @brief 基于红黑树的定时调度器
 *
 * 提供带独立调度线程的定时任务服务与单任务定时器句柄。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/packaged_task.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/map.hpp"
#include "NeForce/core/container/set.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Timer 定时器
 * @brief 定时器容器
 * @{
 */

/**
 * @class tree_timer_scheduler
 * @brief 基于红黑树的定时任务调度器
 *
 * 管理所有定时任务的调度和执行。
 * 使用独立的线程运行调度循环，基于时间点对任务进行排序，并在任务到期时执行回调函数。
 */
class NEFORCE_API tree_timer_scheduler {
public:
    using clock_type = steady_clock;           ///< 时钟类型
    using time_point = clock_type::time_point; ///< 时间点类型
    using duration = clock_type::duration;     ///< 时长类型
    using token = size_t;                      ///< 任务标识符类型
    using handler_type = function<void()>;     ///< 回调函数类型

private:
    /**
     * @struct node
     * @brief 定时任务节点
     */
    struct node {
        time_point expire;    ///< 到期时间
        token id;             ///< 任务 ID
        handler_type handler; ///< 回调函数

        node(time_point exp, const token tid, handler_type&& h) :
        expire(exp),
        id(tid),
        handler(move(h)) {}

        node(const node&) = default;
        node& operator=(const node&) = default;
        node(node&&) = default;
        node& operator=(node&&) = default;

        ~node() = default;

        /**
         * @brief 比较操作符，按到期时间和 ID 排序
         */
        bool operator<(const node& other) const noexcept {
            if (expire < other.expire) {
                return true;
            }
            if (expire > other.expire) {
                return false;
            }
            return id < other.id;
        }
    };

    set<node> nodes_;                                   ///< 按时间排序的任务集合
    map<token, typename set<node>::iterator> node_map_; ///< ID 到迭代器的映射
    map<token, shared_ptr<atomic<bool>>> cancel_flags_; ///< ID 到取消标志的映射
    map<token, shared_ptr<promise<void>>> promises_;    ///< ID 到 promise 的映射

    thread thread_;                           ///< 调度线程
    duration spin_window_{microseconds(100)}; ///< 到期前自旋守时窗口
    mutable mutex mutex_;                     ///< 互斥锁
    condition_variable cv_;                   ///< 条件变量
    token next_id_{1};                        ///< 下一个可用的任务 ID
    atomic<bool> stopped_{false};             ///< 停止标志

    friend class thread_pool;

private:
    void run();

public:
    tree_timer_scheduler();
    ~tree_timer_scheduler();

    /**
     * @brief 停止调度线程并等待其结束
     */
    void stop();

    tree_timer_scheduler(const tree_timer_scheduler&) = delete;
    tree_timer_scheduler& operator=(const tree_timer_scheduler&) = delete;
    tree_timer_scheduler(tree_timer_scheduler&&) = delete;
    tree_timer_scheduler& operator=(tree_timer_scheduler&&) = delete;

    /**
     * @brief 设置到期前自旋守时窗口
     * @param window 窗口长度，设为 0 表示关闭
     */
    void set_spin_window(duration window) noexcept { spin_window_ = window; }

    /**
     * @brief 查询到期前自旋守时窗口
     * @return 当前窗口长度
     */
    NEFORCE_NODISCARD duration spin_window() const noexcept { return spin_window_; }

    /**
     * @brief 添加定时任务
     * @param expire 任务到期时间点
     * @param handler 任务回调函数
     * @return 任务标识符
     */
    token add_task(time_point expire, handler_type&& handler);

    /**
     * @brief 取消定时任务
     * @param id 任务标识符
     * @return 是否成功取消
     */
    bool cancel(token id);

    /**
     * @brief 取消所有定时任务
     */
    void cancel_all();

    /**
     * @brief 获取当前待处理的任务数量
     * @return 任务数量
     */
    NEFORCE_NODISCARD size_t size() const;

    /**
     * @brief 检查任务是否仍在等待或执行中
     * @param id 任务标识符
     * @return 任务是否未完成
     */
    NEFORCE_NODISCARD bool is_pending(token id) const;
};

/**
 * @class tree_timer
 * @brief 红黑树定时器句柄
 *
 * 提供简单的设置和等待接口。
 * 每个句柄持有自己的调度器实例，适合数量很少的长生命周期定时器。
 * 支持一次性定时和取消操作。
 */
class tree_timer {
public:
    using clock_type = tree_timer_scheduler::clock_type;     ///< 时钟类型
    using time_point = tree_timer_scheduler::time_point;     ///< 时间点类型
    using duration = tree_timer_scheduler::duration;         ///< 时长类型
    using token = tree_timer_scheduler::token;               ///< 任务标识符类型
    using handler_type = tree_timer_scheduler::handler_type; ///< 回调函数类型

private:
    shared_ptr<tree_timer_scheduler> scheduler_; ///< 共享的调度器
    token task_id_{0};                           ///< 当前任务的ID
    time_point expire_{clock_type::now()};       ///< 到期时间点

public:
    tree_timer() :
    scheduler_(make_shared<tree_timer_scheduler>()) {}

    ~tree_timer() {
        if (scheduler_) {
            cancel();
        }
    }

    tree_timer(const tree_timer&) = delete;
    tree_timer& operator=(const tree_timer&) = delete;

    /**
     * @brief 移动构造函数
     */
    tree_timer(tree_timer&& other) noexcept :
    scheduler_(move(other.scheduler_)),
    task_id_(other.task_id_),
    expire_(other.expire_) {
        other.task_id_ = 0;
    }

    /**
     * @brief 移动赋值运算符
     */
    tree_timer& operator=(tree_timer&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }

        cancel();
        scheduler_ = move(other.scheduler_);
        task_id_ = other.task_id_;
        expire_ = other.expire_;
        other.task_id_ = 0;

        return *this;
    }

    /**
     * @brief 设置绝对到期时间
     * @param expiry_time 到期时间点
     *
     * 如果之前有未完成的任务，会自动取消。
     */
    void expires_at(const time_point& expiry_time) {
        cancel();
        expire_ = expiry_time;
    }

    /**
     * @brief 设置相对到期时间
     * @param expiry_duration 从当前时间开始的时长
     *
     * 如果之前有未完成的任务，会自动取消。
     */
    void expires_after(const duration& expiry_duration) {
        cancel();
        expire_ = clock_type::now() + expiry_duration;
    }

    /**
     * @brief 设置从当前时间开始的毫秒数
     * @param ms 毫秒数
     */
    void expires_from_now(const int64_t ms) { expires_after(milliseconds(ms)); }

    /**
     * @brief 获取到期时间点
     * @return 到期时间点
     */
    NEFORCE_NODISCARD time_point expiry() const { return expire_; }

    /**
     * @brief 检查定时器是否活跃
     * @return 是否活跃
     */
    NEFORCE_NODISCARD bool is_active() const { return task_id_ != 0 && scheduler_ && scheduler_->is_pending(task_id_); }

    /**
     * @brief 异步等待定时器到期
     * @tparam WaitHandler 回调函数类型
     * @param handler 到期时执行的回调函数
     *
     * 如果之前有未完成的任务，会自动取消。
     * 回调函数会在调度线程中执行，不应包含耗时操作。
     */
    template <typename WaitHandler>
    void async_wait(WaitHandler&& handler) {
        cancel();
        task_id_ = scheduler_->add_task(expire_, handler_type(_NEFORCE forward<WaitHandler>(handler)));
    }

    /**
     * @brief 取消定时任务
     */
    void cancel() {
        if (scheduler_ && task_id_ != 0) {
            scheduler_->cancel(task_id_);
            task_id_ = 0;
        }
    }
};

/** @} */ // Timer

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_TREE_TIMER_HPP__
