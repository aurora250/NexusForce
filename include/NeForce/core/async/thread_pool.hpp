#ifndef NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__

/**
 * @file thread_pool.hpp
 * @brief 线程池实现
 *
 * 此文件提供了高性能线程池的实现，支持任务提交、优先级调度、任务窃取、
 * 延迟任务和周期性任务等功能。线程池支持固定模式和缓存模式两种运行方式。
 */

#include "NeForce/core/async/packaged_task.hpp"
#include "NeForce/core/async/timer.hpp"
#include "NeForce/core/container/priority_queue.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ThreadPool 线程池
 * @brief 高性能线程池的实现
 * @{
 */

/// @cond
NEFORCE_BEGIN_INNER__

class NEFORCE_API manual_thread {
public:
    using id_type = uint32_t;

private:
    using thread_func = function<void(id_type)>;

    thread_func func_;
    id_type thread_id_;

public:
    explicit manual_thread(thread_func&& func) noexcept;
    ~manual_thread() = default;

    NEFORCE_NODISCARD id_type id() const noexcept { return thread_id_; }
    void start();
};

NEFORCE_END_INNER__
/// @endcond


/**
 * @struct task_group
 * @brief 任务组
 *
 * 用于跟踪一组任务的执行状态，可以等待组内所有任务完成。
 * 支持任务的嵌套分组。
 */
struct task_group {
    task_group() = default;
    ~task_group() = default;

    atomic<size_t> running_count{0}; ///< 正在运行的任务计数

    /**
     * @brief 增加运行计数
     */
    void increment() noexcept { running_count.fetch_add(1, memory_order_relaxed); }

    /**
     * @brief 减少运行计数
     *
     * 当计数变为0时，通知所有等待的线程。
     */
    void decrement() noexcept {
        if (running_count.fetch_sub(1, memory_order_release) == 1) {
            running_count.notify_all();
        }
    }

    /**
     * @brief 等待组内所有任务完成
     */
    void wait() const noexcept {
        size_t count = running_count.load(memory_order_acquire);
        while (count != 0) {
            running_count.wait(count);
            count = running_count.load(memory_order_acquire);
        }
    }
};

/**
 * @class local_queue
 * @brief 线程本地任务队列
 *
 * 每个工作线程的本地任务队列，支持无锁的任务推送、弹出和任务窃取。
 * 采用环形缓冲区实现，支持多种窃取策略。
 */
class NEFORCE_API local_queue {
public:
    /**
     * @enum steal_strategy
     * @brief 任务窃取策略
     */
    enum class steal_strategy : uint8_t {
        half,        ///< 窃取一半任务
        fixed_batch, ///< 窃取固定数量的任务
        single,      ///< 每次只窃取一个任务
        adaptive     ///< 自适应策略
    };

    static constexpr size_t queue_size = 256; ///< 队列容量

private:
    static steal_strategy steal_strategy_; ///< 全局窃取策略
    static uint32_t fixed_batch_size_;     ///< 固定批次大小

    array<function<void()>, queue_size> tasks_{}; ///< 任务数组
    atomic<uint64_t> head_{0};                    ///< 队列头指针
    atomic<uint32_t> tail_{0};                    ///< 队列尾指针

private:
    constexpr static size_t mask_ = queue_size - 1; ///< 掩码，用于环形索引计算

    NEFORCE_NODISCARD static uint64_t pack(const uint32_t steal, const uint32_t local_head) noexcept {
        return static_cast<uint64_t>(steal) << 32 | static_cast<uint64_t>(local_head);
    }

    NEFORCE_NODISCARD static pair<uint32_t, uint32_t> unpack(const uint64_t head) noexcept {
        return {static_cast<uint32_t>(head >> 32), static_cast<uint32_t>(head)};
    }

    uint32_t be_stolen_by_impl(local_queue& dst, uint32_t dst_tail);

public:
    local_queue() = default;
    ~local_queue() = default;
    local_queue(const local_queue&) = delete;
    local_queue& operator=(const local_queue&) = delete;
    local_queue(local_queue&& other) noexcept;
    local_queue& operator=(local_queue&& other) noexcept;

    /**
     * @brief 获取队列容量
     * @return 队列最大容量
     */
    NEFORCE_NODISCARD size_t capacity() const noexcept { return tasks_.size(); }

    /**
     * @brief 检查队列是否为空
     * @return 队列为空返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0u; }

    /**
     * @brief 获取剩余容量
     * @return 队列剩余可用空间
     */
    NEFORCE_NODISCARD size_t remain_size() const noexcept {
        const auto tail = tail_.load(memory_order_acquire);
        const auto head = head_.load(memory_order_acquire);
        const auto steal = unpack(head).first;
        const size_t used = static_cast<size_t>(tail - steal);
        const size_t remain = capacity() - used;
        return remain;
    }

    /**
     * @brief 获取队列当前大小
     * @return 队列中的任务数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept {
        const auto tail = tail_.load(memory_order_acquire);
        const auto head = head_.load(memory_order_acquire);
        const auto local_head = unpack(head).second;
        return static_cast<size_t>(tail - local_head);
    }

    /**
     * @brief 设置窃取策略
     * @param strategy 窃取策略
     * @param batch_size 批次大小（仅对fixed_batch策略有效）
     */
    static void set_steal_strategy(const steal_strategy strategy, const uint32_t batch_size = 4) {
        steal_strategy_ = strategy;
        fixed_batch_size_ = batch_size;
    }

    /**
     * @brief 推送任务到队列尾部
     * @param task 要推送的任务
     */
    void push_back(function<void()> task) {
        const uint32_t tail = tail_.load(memory_order_relaxed);
        tasks_[tail & mask_] = move(task);
        tail_.store(tail + 1, memory_order_release);
    }

    /**
     * @brief 从队列头部弹出任务
     * @return 弹出的任务，队列为空时返回none
     */
    optional<function<void()>> try_pop();

    /**
     * @brief 被其他队列窃取任务
     * @param dst_queue 目标队列
     * @return 窃取到的任务，如果窃取失败返回none
     */
    optional<function<void()>> be_stolen_by(local_queue& dst_queue);
};

/**
 * @struct worker_context
 * @brief 工作线程上下文
 *
 * 存储每个工作线程的本地状态信息。
 */
struct NEFORCE_API worker_context {
    using id_type = inner::manual_thread::id_type; ///< 线程ID类型

    local_queue queue{};               ///< 本地任务队列
    id_type id{0};                     ///< 线程ID
    atomic<bool> is_stealing{false};   ///< 是否正在执行窃取操作
    size_t consecutive_idle_count = 0; ///< 连续空闲次数

    worker_context() = default;
    worker_context(const worker_context&) = delete;
    worker_context& operator=(const worker_context&) = delete;
    worker_context(worker_context&& other) noexcept;
    worker_context& operator=(worker_context&& other) noexcept;
};


/**
 * @struct task_info
 * @brief 任务信息
 *
 * 存储任务的元数据，包括状态、时间戳、错误信息等。
 */
struct task_info {
    /**
     * @enum status
     * @brief 任务状态枚举
     */
    enum class status {
        pending,   ///< 等待执行
        running,   ///< 正在执行
        completed, ///< 已完成
        failed     ///< 执行失败
    };

    enum class priority_type : uint32_t {}; ///< 优先级类型

    const uint64_t id;                                 ///< 任务ID
    atomic<status> status{status::pending};            ///< 任务状态
    timestamp submit_time{timestamp::now()};           ///< 提交时间
    timestamp start_time{0};                           ///< 开始执行时间
    timestamp finish_time{0};                          ///< 完成时间
    inner::manual_thread::id_type worker_thread_id{0}; ///< 执行任务的线程ID
    string error{};                                    ///< 错误信息
    priority_type priority;                            ///< 任务优先级

    /**
     * @brief 构造函数
     * @param task_id 任务ID
     * @param priority 任务优先级
     */
    explicit task_info(const uint64_t task_id, const priority_type priority) :
    id(task_id),
    priority(priority) {}

    /**
     * @brief 检查任务是否已完成
     * @return 任务已完成返回true
     */
    NEFORCE_NODISCARD bool is_finished() const noexcept {
        const auto s = status.load(memory_order_acquire);
        return s == status::completed || s == status::failed;
    }

    /**
     * @brief 获取任务执行时间
     * @return 执行时间（微秒），未开始或未完成返回-1
     */
    NEFORCE_NODISCARD int64_t exec_time() const noexcept {
        if (start_time.value() == 0 || finish_time.value() == 0) {
            return -1;
        }
        return finish_time.value() - start_time.value();
    }
};

/**
 * @struct submit_result
 * @brief 任务提交结果
 * @tparam T 任务返回值类型
 *
 * 包含任务的future和任务信息，用于跟踪任务执行状态。
 */
template <typename T>
struct submit_result {
    _NEFORCE future<T> future;                ///< 任务的future
    shared_ptr<_NEFORCE task_info> task_info; ///< 任务信息

    /**
     * @brief 检查提交是否有效
     * @return 有效返回true
     */
    NEFORCE_NODISCARD explicit operator bool() const noexcept { return future.valid() && task_info; }
};


/**
 * @class thread_pool
 * @brief 线程池类
 *
 * 高性能线程池实现，支持：
 * - 固定模式和缓存模式
 * - 任务优先级调度
 * - 工作窃取算法
 * - 延迟任务和周期性任务
 * - 任务组跟踪
 */
class NEFORCE_API thread_pool {
public:
    /**
     * @enum pool_mode
     * @brief 线程池运行模式
     */
    enum class pool_mode : uint8_t {
        fixed, ///< 固定线程数模式
        cached ///< 缓存模式，可动态增减线程
    };

    /**
     * @struct periodic_task_state
     * @brief 周期性任务状态
     */
    struct periodic_task_state {
        atomic<bool> cancelled{false}; ///< 是否已取消
    };

    /**
     * @struct pool_statistics
     * @brief 线程池统计信息
     */
    struct NEFORCE_API pool_statistics : istringify<pool_statistics> {
        size_t total_threads;   ///< 总线程数
        size_t idle_threads;    ///< 空闲线程数
        size_t busy_threads;    ///< 忙碌线程数
        size_t queue_size;      ///< 全局队列大小
        size_t total_submitted; ///< 总提交任务数
        size_t total_stolen;    ///< 总窃取任务数
        size_t total_completed; ///< 总完成任务数

        /**
         * @brief 转换为字符串
         * @return 格式化的统计信息字符串
         */
        NEFORCE_NODISCARD string to_string() const;
    };

    using steal_strategy = local_queue::steal_strategy;     ///< 窃取策略类型别名
    using id_type = inner::manual_thread::id_type;          ///< 线程ID类型别名
    using periodic_token = shared_ptr<periodic_task_state>; ///< 周期性任务令牌
    using priority_type = task_info::priority_type;         ///< 优先级类型别名

    static constexpr size_t task_max_threshhold = numeric_traits<int32_t>::max(); ///< 最大任务队列阈值
    static constexpr size_t max_idle_seconds = 60;                                ///< 最大空闲秒数
    static const size_t max_threshhold;                                           ///< 最大线程数阈值

private:
    using task_type = function<void()>; ///< 任务类型

    /**
     * @struct priority_task
     * @brief 带优先级的任务包装
     */
    struct priority_task {
        task_type task;             ///< 任务函数
        priority_type priority;     ///< 优先级
        shared_ptr<task_info> info; ///< 任务信息

        priority_task(task_type t, const priority_type p, shared_ptr<task_info> info) noexcept :
        task(move(t)),
        priority(p),
        info(_NEFORCE move(info)) {}

        bool operator<(const priority_task& other) const noexcept { return priority < other.priority; }
    };

    unordered_map<id_type, unique_ptr<inner::manual_thread>> threads_map_; ///< 线程映射
    unordered_map<id_type, worker_context> worker_contexts_;               ///< 工作线程上下文映射
    vector<atomic<worker_context*>> worker_contexts_ptr_;                  ///< 工作线程上下文指针数组
    mutex worker_contexts_mtx_;                                            ///< 工作线程上下文互斥锁

    timer_scheduler<steady_clock> timer_{}; ///< 定时器调度器

    id_type init_thread_size_{0};              ///< 初始线程数
    size_t thread_threshhold_{max_threshhold}; ///< 线程数阈值

    priority_queue<priority_task> task_queue_{};  ///< 全局优先级任务队列
    atomic<uint32_t> task_size_{0};               ///< 全局队列大小
    atomic<uint32_t> idle_thread_size_{0};        ///< 空闲线程数
    size_t task_threshhold_{task_max_threshhold}; ///< 任务队列阈值

    mutex task_queue_mtx_{};         ///< 任务队列互斥锁
    condition_variable not_full_{};  ///< 队列非满条件变量
    condition_variable not_empty_{}; ///< 队列非空条件变量
    condition_variable exit_cond_{}; ///< 退出条件变量

    atomic<pool_mode> pool_mode_{pool_mode::fixed}; ///< 线程池模式
    atomic<bool> is_running_{false};                ///< 是否正在运行

    atomic<size_t> total_submitted_tasks_{0}; ///< 总提交任务计数
    atomic<size_t> total_completed_tasks_{0}; ///< 总完成任务计数
    atomic<size_t> total_stolen_tasks_{0};    ///< 总窃取任务计数
    atomic<size_t> steal_worker_count_{0};    ///< 正在窃取的工作线程数
    atomic<uint64_t> next_task_id_{0};        ///< 下一个任务ID

private:
    uint64_t generate_task_id() { return next_task_id_.fetch_add(1, memory_order_relaxed); }

    void thread_function(id_type thread_id);
    optional<task_type> try_steal_task(worker_context& ctx);

    pool_statistics statistics_unsafe() const;

public:
    /**
     * @brief 默认构造函数
     */
    thread_pool();

    /**
     * @brief 析构函数
     */
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    thread_pool(thread_pool&&) = default;
    thread_pool& operator=(thread_pool&&) = default;

    /**
     * @brief 设置线程池模式
     * @param mode 新模式
     * @return 设置成功返回true（线程池未运行时）
     */
    bool set_mode(pool_mode mode) noexcept;

    /**
     * @brief 设置窃取策略
     * @param strategy 窃取策略
     * @param steal_batch 批次大小
     * @return 设置成功返回true（线程池未运行时）
     */
    bool set_steal_mode(steal_strategy strategy, uint32_t steal_batch = 4) noexcept;

    /**
     * @brief 设置任务队列阈值
     * @param threshhold 新阈值
     * @return 设置成功返回true（线程池未运行时）
     */
    bool set_task_threshhold(size_t threshhold) noexcept;

    /**
     * @brief 设置线程数阈值
     * @param threshhold 新阈值
     * @return 设置成功返回true（线程池未运行时且处于缓存模式）
     */
    bool set_thread_threshhold(size_t threshhold) noexcept;

    /**
     * @brief 获取最大线程数
     * @return 系统支持的最大线程数
     */
    NEFORCE_NODISCARD static size_t max_thread_size() noexcept { return max_threshhold; }

    /**
     * @brief 检查线程池是否正在运行
     * @return 正在运行返回true
     */
    NEFORCE_NODISCARD bool running() const noexcept { return is_running_; }

    /**
     * @brief 获取线程池模式
     * @return 当前模式
     */
    NEFORCE_NODISCARD pool_mode mode() const noexcept { return pool_mode_; }

    /**
     * @brief 获取线程池统计信息
     * @return 统计信息
     */
    NEFORCE_NODISCARD pool_statistics statistics() const;

    /**
     * @brief 启动线程池
     * @param init_thread_size 初始线程数，默认为3
     * @return 启动成功返回true
     */
    bool start(size_t init_thread_size = 3);

    /**
     * @brief 停止线程池
     * @return 停止前的统计信息
     */
    pool_statistics stop();

    /**
     * @brief 提交任务
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param priority 任务优先级
     * @param func 可调用对象
     * @param args 参数
     * @return 包含future和任务信息的结果
     */
    template <typename Func, typename... Args>
    submit_result<invoke_result_t<Func, Args...>> submit_task(priority_type priority, Func&& func, Args&&... args);

    /**
     * @brief 提交任务（使用默认优先级0）
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param func 可调用对象
     * @param args 参数
     * @return 包含future和任务信息的结果
     */
    template <typename Func, typename... Args>
    submit_result<invoke_result_t<Func, Args...>> submit_task(Func&& func, Args&&... args) {
        return this->submit_task(static_cast<priority_type>(0), _NEFORCE forward<Func>(func),
                                 _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 提交延迟任务
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param delay_ms 延迟时间（毫秒）
     * @param priority 任务优先级
     * @param func 可调用对象
     * @param args 参数
     * @return 包含future和任务信息的结果
     */
    template <typename Func, typename... Args>
    submit_result<invoke_result_t<Func, Args...>> submit_after(int64_t delay_ms, priority_type priority, Func&& func,
                                                               Args&&... args);

    /**
     * @brief 提交延迟任务（使用默认优先级0）
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param delay_ms 延迟时间（毫秒）
     * @param func 可调用对象
     * @param args 参数
     * @return 包含future和任务信息的结果
     */
    template <typename Func, typename... Args>
    submit_result<invoke_result_t<Func, Args...>> submit_after(int64_t delay_ms, Func&& func, Args&&... args) {
        return this->submit_after(delay_ms, static_cast<priority_type>(0), _NEFORCE forward<Func>(func),
                                  _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 提交周期性任务
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param interval_ms 执行间隔（毫秒）
     * @param priority 任务优先级
     * @param func 可调用对象
     * @param args 参数
     * @return 周期性任务令牌，用于取消任务
     */
    template <typename Func, typename... Args>
    periodic_token submit_every(int64_t interval_ms, priority_type priority, Func&& func, Args&&... args);

    /**
     * @brief 提交周期性任务（使用默认优先级0）
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param interval_ms 执行间隔（毫秒）
     * @param func 可调用对象
     * @param args 参数
     * @return 周期性任务令牌，用于取消任务
     */
    template <typename Func, typename... Args>
    periodic_token submit_every(int64_t interval_ms, Func&& func, Args&&... args) {
        return this->submit_every(interval_ms, static_cast<priority_type>(0), _NEFORCE forward<Func>(func),
                                  _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 取消周期性任务
     * @param token 周期性任务令牌
     */
    static void cancel_periodic_task(const periodic_token& token) {
        if (token) {
            token->cancelled.store(true);
        }
    }

    /**
     * @brief 等待多个future完成
     * @tparam Types future结果类型
     * @param futures 要等待的future
     * @return 包含所有结果的元组
     */
    template <typename... Types>
    static tuple<future_result_t<Types>...> wait(future<Types>&&... futures) {
        return _NEFORCE make_tuple(_NEFORCE get(futures)...);
    }
};


/**
 * @brief 获取当前线程的工作线程上下文
 * @return 工作线程上下文指针
 */
NEFORCE_API worker_context*& get_worker_context() noexcept;

/**
 * @brief 获取当前线程的任务组
 * @return 任务组共享指针
 */
NEFORCE_API shared_ptr<task_group>& get_current_task_group() noexcept;

/// @cond

template <typename Func, typename... Args>
submit_result<invoke_result_t<Func, Args...>> thread_pool::submit_task(const priority_type priority, Func&& func,
                                                                       Args&&... args) {
    static_assert(is_invocable_v<Func, Args...>, "Func must be invocable with Args");

    using Result = invoke_result_t<Func, Args...>;

    auto info = make_shared<task_info>(generate_task_id(), priority);

    const auto current_group = get_current_task_group();
    if (current_group) {
        current_group->increment();
    }

    auto task = _NEFORCE make_shared<packaged_task<Result()>>(
            [func = _NEFORCE forward<Func>(func), args = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...),
             group = current_group, info]() mutable -> Result {
                struct context_guard {
                    shared_ptr<task_info> info;
                    shared_ptr<task_group> group_inner;
                    shared_ptr<task_group> prev_group_inner;

                    explicit context_guard(shared_ptr<task_info> i, shared_ptr<task_group> g) :
                    info(move(i)),
                    group_inner(move(g)) {
                        info->status.store(task_info::status::running, memory_order_release);
                        info->start_time = timestamp::now();
                        info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;

                        prev_group_inner = get_current_task_group();
                        get_current_task_group() = group_inner;
                    }

                    ~context_guard() noexcept {
                        try {
                            info->finish_time = timestamp::now();
                            auto expected = task_info::status::running;
                            info->status.compare_exchange_strong(expected, task_info::status::completed,
                                                                 memory_order_release);

                            get_current_task_group() = prev_group_inner;
                            if (group_inner) {
                                group_inner->decrement();
                            }
                        } catch (...) {
                            /* ignore */
                        }
                    }
                };

                context_guard guard(info, group);
                try {
                    return _NEFORCE apply(func, args);
                } catch (const exception& e) {
                    info->status.store(task_info::status::failed, memory_order_release);
                    info->error = e.what();
                    throw;
                } catch (...) {
                    info->status.store(task_info::status::failed, memory_order_release);
                    info->error = "Unknown exception";
                    throw;
                }
            });

    future<Result> res = task->get_future();
    task_type job([task] { (*task)(); });

    if (static_cast<uint32_t>(priority) > 0) {
        unique_lock<mutex> lock(task_queue_mtx_);

        if (!not_full_.wait_for(lock, seconds(1), [&]() -> bool { return task_queue_.size() < task_threshhold_; })) {
            info->status.store(task_info::status::failed, memory_order_release);
            info->error = "Task queue is full";

            auto dummy_task = _NEFORCE make_shared<packaged_task<Result()>>([]() -> Result { return Result(); });
            (*dummy_task)();
            return submit_result<Result>{dummy_task->get_future(), info};
        }

        task_queue_.emplace(move(job), priority, info);
        ++task_size_;
        ++total_submitted_tasks_;
        not_empty_.notify_one();

    } else {
        auto* ctx = get_worker_context();

        if (ctx != nullptr && ctx->queue.remain_size() > 0) {
            ctx->queue.push_back(move(job));
            ++total_submitted_tasks_;
        } else {
            unique_lock<mutex> lock(task_queue_mtx_);
            if (!not_full_.wait_for(lock, seconds(1),
                                    [&]() -> bool { return task_queue_.size() < task_threshhold_; })) {
                info->status.store(task_info::status::failed, memory_order_release);
                info->error = "Task queue is full";

                auto dummy_task = _NEFORCE make_shared<packaged_task<Result()>>([]() -> Result { return Result(); });
                (*dummy_task)();
                return submit_result<Result>{dummy_task->get_future(), info};
            }

            task_queue_.emplace(move(job), static_cast<priority_type>(0), info);
            ++task_size_;
            ++total_submitted_tasks_;
            not_empty_.notify_one();
        }
    }

    if (pool_mode_.load() == pool_mode::cached && task_size_.load() > idle_thread_size_) {

        inner::manual_thread* t_ptr = nullptr;
        id_type thread_id = 0;

        {
            unique_lock<mutex> lock(task_queue_mtx_);
            if (threads_map_.size() < thread_threshhold_) {
                auto ptr =
                        _NEFORCE make_unique<inner::manual_thread>([this](const id_type id) { thread_function(id); });

                thread_id = ptr->id();
                t_ptr = ptr.get();
                threads_map_.emplace(thread_id, move(ptr));
            }
        }

        if (t_ptr != nullptr) {
            {
                lock<mutex> ctx_lock(worker_contexts_mtx_);
                if (thread_id >= worker_contexts_ptr_.size()) {
                    worker_contexts_ptr_.reserve(thread_id + 1);
                    for (size_t i = worker_contexts_ptr_.size(); i <= thread_id; i++) {
                        atomic<worker_context*> tmp;
                        tmp.store(nullptr, memory_order_relaxed);
                        worker_contexts_ptr_.emplace_back(move(tmp));
                    }
                }
            }

            t_ptr->start();
        }
    }

    return submit_result<Result>{move(res), move(info)};
}

template <typename Func, typename... Args>
submit_result<invoke_result_t<Func, Args...>>
thread_pool::submit_after(const int64_t delay_ms, const priority_type priority, Func&& func, Args&&... args) {
    static_assert(is_invocable_v<Func, Args...>, "Func must be invocable with Args");

    using Result = invoke_result_t<Func, Args...>;

    auto info = make_shared<task_info>(generate_task_id(), priority);

    auto task = _NEFORCE make_shared<packaged_task<Result()>>(
            [func = _NEFORCE forward<Func>(func), tup = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...),
             info]() mutable {
                struct context_guard {
                    shared_ptr<task_info> info;

                    explicit context_guard(shared_ptr<task_info> i) :
                    info(move(i)) {
                        info->status.store(task_info::status::running, memory_order_release);
                        info->start_time = timestamp::now();
                        info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;
                    }

                    ~context_guard() noexcept {
                        info->finish_time = timestamp::now();
                        auto expected = task_info::status::running;
                        info->status.compare_exchange_strong(expected, task_info::status::completed,
                                                             memory_order_release);
                    }
                };

                context_guard guard(info);

                try {
                    return _NEFORCE apply(func, tup);
                } catch (const exception& e) {
                    info->status.store(task_info::status::failed, memory_order_release);
                    info->error = e.what();
                    throw;
                }
            });

    future<Result> res = task->get_future();

    auto expire_time = steady_clock::now() + milliseconds(delay_ms);
    timer_.add_task(expire_time, [this, task = _NEFORCE move(task), priority]() mutable {
        this->submit_task(priority, [task]() { (*task)(); });
    });

    return submit_result<Result>{_NEFORCE move(res), info};
}

template <typename Func, typename... Args>
thread_pool::periodic_token thread_pool::submit_every(int64_t interval_ms, const priority_type priority, Func&& func,
                                                      Args&&... args) {
    auto state = make_shared<periodic_task_state>();
    auto task = _NEFORCE make_shared<function<void()>>(
            [func = _NEFORCE forward<Func>(func),
             tup = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...)]() mutable { _NEFORCE apply(func, tup); });
    auto handler_ptr = _NEFORCE make_shared<task_type>();
    *handler_ptr = [this, state, task, interval_ms, priority, handler_ptr]() {
        if (state->cancelled.load()) {
            return;
        }

        this->submit_task(priority, [task]() { (*task)(); });

        if (state->cancelled.load()) {
            return;
        }
        auto next_time = steady_clock::now() + milliseconds(interval_ms);
        timer_.add_task(next_time, [handler_ptr]() { (*handler_ptr)(); });
    };

    auto first_time = steady_clock::now() + milliseconds(interval_ms);
    timer_.add_task(first_time, [handler_ptr]() { (*handler_ptr)(); });
    return state;
}

/// @endcond

/** @} */ // ThreadPool

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
