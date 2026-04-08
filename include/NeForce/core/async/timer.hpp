#ifndef NEFORCE_CORE_ASYNC_TIMER_HPP__
#define NEFORCE_CORE_ASYNC_TIMER_HPP__

/**
 * @file timer.hpp
 * @brief 异步定时器
 *
 * 此文件提供了异步定时器功能，支持单次定时、重复定时和定时任务调度。
 * 使用独立的调度线程管理所有定时任务，支持任务的取消和重新调度。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/map.hpp"
#include "NeForce/core/container/set.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncTimer 定时调度器
 * @brief 异步定时器工具
 * @{
 */

/**
 * @class timer_scheduler
 * @brief 定时任务调度器
 * @tparam Clock 时钟类型
 *
 * 管理所有定时任务的调度和执行。使用独立的线程运行调度循环，
 * 基于时间点对任务进行排序，并在任务到期时执行回调函数。
 */
template <typename Clock>
class timer_scheduler {
public:
    using clock_type = Clock;                           ///< 时钟类型
    using time_point = typename clock_type::time_point; ///< 时间点类型
    using duration = typename clock_type::duration;     ///< 时长类型
    using token = size_t;                               ///< 任务标识符类型
    using handler_type = function<void()>;              ///< 回调函数类型

private:
    /**
     * @struct node
     * @brief 定时任务节点
     *
     * 包含任务的到期时间、唯一标识符和回调函数。
     */
    struct node {
        time_point expire;    ///< 到期时间
        token id;             ///< 任务ID
        handler_type handler; ///< 回调函数

        node(time_point exp, const token tid, handler_type&& h) :
        expire(exp),
        id(tid),
        handler(move(h)) {}

        node(const node&) = default;
        node& operator=(const node&) = default;
        node(node&&) = default;
        node& operator=(node&&) = default;

        /**
         * @brief 比较操作符，按到期时间和ID排序
         */
        bool operator<(const node& other) const {
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
    map<token, typename set<node>::iterator> node_map_; ///< ID到迭代器的映射

    thread thread_;         ///< 调度线程
    mutable mutex mutex_;   ///< 互斥锁
    condition_variable cv_; ///< 条件变量
    token next_id_;         ///< 下一个可用的任务ID
    atomic<bool> stopped_;  ///< 停止标志

    friend class thread_pool;

private:
    /**
     * @brief 调度线程的主循环
     *
     * 循环执行：
     * 1. 等待直到有任务到达或停止信号
     * 2. 执行所有到期的任务
     * 3. 等待下一个任务的到期时间
     */
    void run() {
        while (!stopped_.load()) {
            unique_lock<mutex> lock(mutex_);

            if (nodes_.empty()) {
                cv_.wait(lock, [this] { return stopped_.load() || !nodes_.empty(); });
                if (stopped_.load()) {
                    break;
                }
            }

            time_point now = clock_type::now();
            while (!nodes_.empty() && nodes_.begin()->expire <= now) {
                auto it = nodes_.begin();
                node current_node = *it;
                nodes_.erase(it);
                node_map_.erase(current_node.id);

                lock.unlock_quiet();
                if (!stopped_.load()) {
                    current_node.handler();
                }
                lock.lock_quiet();
                now = clock_type::now();
            }

            if (!nodes_.empty()) {
                time_point next_expire = nodes_.begin()->expire;
                cv_.wait_until(lock, next_expire);
            }
        }
    }

public:
    /**
     * @brief 构造函数，启动调度线程
     */
    timer_scheduler() :
    next_id_(0),
    stopped_(false) {
        thread_ = thread(&timer_scheduler::run, this);
    }

    /**
     * @brief 析构函数，停止调度线程并等待其结束
     */
    ~timer_scheduler() {
        stopped_.store(true);
        cv_.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    timer_scheduler(const timer_scheduler&) = delete;
    timer_scheduler& operator=(const timer_scheduler&) = delete;
    timer_scheduler(timer_scheduler&&) = default;
    timer_scheduler& operator=(timer_scheduler&&) = default;

    /**
     * @brief 添加定时任务
     * @param expire 任务到期时间点
     * @param handler 任务回调函数
     * @return 任务标识符，可用于取消任务
     *
     * 如果新任务的到期时间早于当前最早的任务，会唤醒调度线程。
     */
    token add_task(time_point expire, handler_type&& handler) {
        unique_lock<mutex> lock(mutex_);
        token id = next_id_++;

        const bool is_earliest = nodes_.empty() || expire < nodes_.begin()->expire;

        node new_node(expire, id, _NEFORCE move(handler));
        auto result = nodes_.insert(new_node);
        node_map_[id] = result.first;

        lock.unlock_quiet();

        if (is_earliest) {
            cv_.notify_one();
        }

        return id;
    }

    /**
     * @brief 取消定时任务
     * @param id 任务标识符
     * @return 是否成功取消
     *
     * 如果取消的是当前最早的任务，会唤醒调度线程重新计算等待时间。
     */
    bool cancel(token id) {
        unique_lock<mutex> lock(mutex_);
        auto it_map = node_map_.find(id);
        if (it_map == node_map_.end()) {
            return false;
        }

        const bool is_earliest = (it_map->second == nodes_.begin());
        nodes_.erase(it_map->second);
        node_map_.erase(it_map);

        lock.unlock_quiet();

        if (is_earliest) {
            cv_.notify_one();
        }

        return true;
    }

    /**
     * @brief 取消所有定时任务
     */
    void cancel_all() {
        unique_lock<mutex> lock(mutex_);
        nodes_.clear();
        node_map_.clear();
        lock.unlock_quiet();
        cv_.notify_one();
    }

    /**
     * @brief 获取当前待处理的任务数量
     * @return 任务数量
     */
    NEFORCE_NODISCARD size_t size() const {
        lock<mutex> lock(mutex_);
        return nodes_.size();
    }
};

/**
 * @class basic_timer
 * @brief 基本定时器
 * @tparam Clock 时钟类型
 *
 * 封装一个定时任务，提供简单的设置和等待接口。
 * 支持一次性定时和取消操作。
 */
template <typename Clock>
class basic_timer {
public:
    using clock_type = Clock;                                           ///< 时钟类型
    using time_point = typename clock_type::time_point;                 ///< 时间点类型
    using duration = typename clock_type::duration;                     ///< 时长类型
    using token = typename timer_scheduler<Clock>::token;               ///< 任务标识符类型
    using handler_type = typename timer_scheduler<Clock>::handler_type; ///< 回调函数类型

private:
    timer_scheduler<Clock> scheduler_{};    ///< 共享的调度器
    token task_id_ = 0;                     ///< 当前任务的ID
    time_point expire_ = clock_type::now(); ///< 到期时间点

public:
    basic_timer() = default;

    /**
     * @brief 析构函数，自动取消未完成的任务
     */
    ~basic_timer() { cancel(); }

    basic_timer(const basic_timer&) = delete;
    basic_timer& operator=(const basic_timer&) = delete;

    /**
     * @brief 移动构造函数
     */
    basic_timer(basic_timer&& other) noexcept :
    scheduler_(other.scheduler_),
    task_id_(other.task_id_),
    expire_(other.expire_) {
        other.task_id_ = 0;
    }

    /**
     * @brief 移动赋值运算符
     */
    basic_timer& operator=(basic_timer&& other) noexcept {
        if (this != &other) {
            cancel();
            task_id_ = other.task_id_;
            expire_ = other.expire_;
            other.task_id_ = 0;
        }
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
     * @brief 检查定时器是否活跃（有待执行的任务）
     * @return 是否活跃
     */
    NEFORCE_NODISCARD bool is_active() const { return task_id_ != 0; }

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
        task_id_ = scheduler_.add_task(expire_, handler_type(_NEFORCE forward<WaitHandler>(handler)));
    }

    /**
     * @brief 取消定时任务
     *
     * 如果任务尚未执行，会从调度器中移除。
     */
    void cancel() {
        if (task_id_ != 0) {
            scheduler_.cancel(task_id_);
            task_id_ = 0;
        }
    }
};

/**
 * @brief 基于稳定时钟的定时器
 */
using steady_timer = basic_timer<steady_clock>;

/**
 * @brief 基于系统时钟的定时器
 */
using system_timer = basic_timer<system_clock>;

/** @} */ // AsyncTimer

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_TIMER_HPP__
