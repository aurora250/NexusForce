#ifndef NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__

/**
 * @file thread_pool.hpp
 * @brief 线程池实现
 *
 * 此文件提供了高性能线程池的实现，支持任务提交、优先级调度、任务窃取、
 * 延迟任务和周期性任务等功能。线程池支持固定模式和缓存模式两种运行方式。
 */

#include "NeForce/core/async/lazy_thread.hpp"
#include "NeForce/core/async/lock_free_queue.hpp"
#include "NeForce/core/async/timer.hpp"
#include "NeForce/core/container/priority_queue.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/memory/weak_ptr.hpp"
#include "NeForce/core/system/sysinfo.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup ThreadPool 线程池
 * @brief 高性能线程池的实现
 * @{
 */

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
    steal_strategy steal_strategy_{steal_strategy::adaptive}; ///< 窃取策略
    uint32_t fixed_batch_size_{4};                            ///< 固定批次大小

    array<function<void()>, queue_size> tasks_; ///< 任务数组
    atomic<uint64_t> head_{0};                  ///< 队列头指针
    atomic<uint32_t> tail_{0};                  ///< 队列尾指针

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
    NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0U; }

    /**
     * @brief 获取剩余容量
     * @return 队列剩余可用空间
     */
    NEFORCE_NODISCARD size_t remain_size() const noexcept {
        const auto tail = tail_.load(memory_order_acquire);
        const auto head = head_.load(memory_order_acquire);
        const auto steal = unpack(head).first;
        const auto used = static_cast<size_t>(tail - steal);
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
    void set_steal_strategy(const steal_strategy strategy, const uint32_t batch_size = 4) {
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
    using id_type = uint32_t; ///< 线程ID类型

    local_queue queue;                 ///< 本地任务队列
    id_type id{0};                     ///< 线程ID
    atomic<bool> is_stealing{false};   ///< 是否正在执行窃取操作
    size_t consecutive_idle_count = 0; ///< 连续空闲次数
    uint32_t cpu_core{0};              ///< 绑定的 CPU 核心编号
    uint32_t numa_node{0};             ///< 所属 NUMA 节点编号

    /**
     * @brief 已注册标记
     * @warning 窃取者必须先读取该标记，为 false 时不得访问本上下文的其它字段
     */
    atomic<bool> attached{false};

    /**
     * @brief 本地队列长度发布值
     * @note 由所有者维护，窃取者据此选择目标，避免触碰非候选者的缓存行
     */
    atomic<uint32_t> published_size{0};

    /**
     * @brief 退出请求标记
     * @note 缓存模式收缩线程时由伸缩线程设置
     */
    atomic<bool> retire{false};

    atomic<size_t> completed{0}; ///< 本工作线程完成的任务数
    atomic<size_t> stolen{0};    ///< 本工作线程成功窃取的任务数
    atomic<size_t> failed{0};    ///< 本工作线程执行失败的任务数

    /**
     * @brief 是否已发布窃取唤醒信号
     * @note 仅由上下文所有者读写
     */
    bool steal_signalled{false};

    /**
     * @brief 停驻前的自旋预算
     * @note 取到任务时倍增、确无任务可做时减半，使繁忙期不产生唤醒系统调用、空闲时迅速停驻
     */
    size_t spin_budget{64};

    bool idle_counted{false}; ///< 是否已计入"空闲轮询中"的计数

    worker_context() = default;
    worker_context(const worker_context&) = delete;
    worker_context& operator=(const worker_context&) = delete;
    worker_context(worker_context&& other) noexcept;
    worker_context& operator=(worker_context&& other) noexcept;

    /**
     * @brief 重置为可复用状态
     * @warning 仅可在确认没有窃取者持有该上下文时调用
     */
    void reset() noexcept;
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

    enum class priority_type : uint32_t {
    }; ///< 优先级类型

    const uint64_t id;                       ///< 任务ID
    atomic<status> status{status::pending};  ///< 任务状态
    timestamp submit_time{timestamp::now()}; ///< 提交时间
    timestamp start_time{0};                 ///< 开始执行时间
    timestamp finish_time{0};                ///< 完成时间
    uint32_t worker_thread_id{0};            ///< 执行任务的线程ID
    string error;                            ///< 错误信息
    priority_type priority;                  ///< 任务优先级

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
    using id_type = uint32_t;                               ///< 线程ID类型别名
    using periodic_token = shared_ptr<periodic_task_state>; ///< 周期性任务令牌
    using priority_type = task_info::priority_type;         ///< 优先级类型别名

    static constexpr size_t task_max_threshhold = numeric_traits<int32_t>::max(); ///< 最大任务队列阈值
    static constexpr size_t max_idle_seconds = 60;                                ///< 最大空闲秒数

    static size_t max_thread_threshhold() noexcept;

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

    struct thread_pool_id_generator {
        static NEFORCE_API uint32_t get_new_id() noexcept;
        static NEFORCE_API void reset_id() noexcept;
    };

    /**
     * @struct worker_slot
     * @brief 工作线程槽位
     */
    struct worker_slot {
        unique_ptr<lazy_thread> thread;     ///< 线程对象
        unique_ptr<worker_context> context; ///< 上下文
        id_type id{0};                      ///< 线程ID
        bool used{false};                   ///< 槽位是否已占用
    };


    unique_ptr<timer_scheduler<steady_clock>> timer_;     ///< 定时器调度器（随 start 重建）
    unique_ptr<lock_free_queue<task_type>> global_queue_; ///< 全局无锁任务队列
    unique_ptr<lazy_thread> scaler_thread_;               ///< 缓存模式伸缩线程

    size_t thread_threshhold_;                    ///< 线程数阈值
    size_t task_threshhold_{task_max_threshhold}; ///< 任务队列阈值

    /**
     * @brief 上一次存活巡检观察到的完成计数
     * @note 仅由定时器线程访问
     */
    size_t liveness_completed_{0};

    int64_t warmup_window_us_{200}; ///< 定时任务预热窗口

    /**
     * @brief 定时任务预热截止时刻
     * @note 非零表示已指派一个工作线程在到期前保持自旋，避免定时任务遭遇冷唤醒
     */
    atomic<int64_t> warm_deadline_ns_{0};

    atomic<size_t> total_submitted_tasks_{0}; ///< 总提交任务计数
    atomic<uint64_t> next_task_id_{0};        ///< 下一个任务ID
    uint64_t allowed_cpus_{0};                ///< 进程允许的 CPU 掩码

    const vector<sysinfo::numa_node_info>* numa_nodes_{nullptr}; ///< NUMA 节点信息指针

    vector<unique_ptr<worker_slot>> worker_slots_; ///< 槽位表
    vector<atomic<worker_context*>> worker_index_; ///< 上下文索引
    priority_queue<priority_task> priority_queue_; ///< 高优先级任务队列

    mutable mutex workers_mtx_; ///< 保护槽位表结构变更
    mutex priority_mtx_;        ///< 优先级队列互斥锁

    id_type init_thread_size_{0};        ///< 初始线程数
    uint32_t configured_steal_batch_{4}; ///< 配置的窃取批次大小

    /**
     * @brief 优先级队列待处理任务数
     * @note 工作线程据此以一次原子读取代每次加锁探测优先级队列
     */
    atomic<uint32_t> priority_pending_{0};

    atomic<int32_t> warm_owner_{-1}; ///< 预热线程编号

    /**
     * @brief 待处理任务总数
     * @note 工作线程停驻时以该计数所在缓存行为参照做脏检查
     */
    atomic<uint32_t> pending_tasks_{0};

    atomic<uint32_t> wake_word_{0};          ///< 单调递增的唤醒字
    atomic<uint32_t> attached_workers_{0};   ///< 已注册工作线程数
    atomic<uint32_t> parked_workers_{0};     ///< 停驻工作线程数
    atomic<uint32_t> idle_workers_{0};       ///< 空闲但仍在校验任务的线程数
    atomic<uint32_t> steal_worker_count_{0}; ///< 正在窃取的工作线程数

    steal_strategy configured_steal_strategy_{steal_strategy::adaptive}; ///< 配置的窃取策略
    atomic<pool_mode> pool_mode_{pool_mode::fixed};                      ///< 线程池模式
    atomic<bool> is_running_{false};                                     ///< 是否正在运行
    atomic<bool> scaler_stop_{false};                                    ///< 伸缩线程退出标记
    atomic<bool> affinity_enabled_{false};                               ///< 是否绑定工作线程到 CPU
    atomic<bool> task_tracking_enabled_{true};                           ///< 是否为每个任务分配并记录任务信息

private:
    uint64_t generate_task_id() { return next_task_id_.fetch_add(1, memory_order_relaxed); }

    void thread_function(size_t slot_index);
    optional<task_type> try_steal_task(worker_context& ctx);
    optional<task_type> try_take_priority();
    void publish_local_size(worker_context& ctx);
    void wake_workers(uint32_t backlog) noexcept;
    NEFORCE_NODISCARD shared_ptr<task_info> make_task_info(priority_type priority);
    void prewarm_dispatch(int64_t deadline_ns);
    void liveness_tick();
    bool dispatch_task(task_type&& job, priority_type priority, const shared_ptr<task_info>& info);
    NEFORCE_NODISCARD bool hold_warm_spin(worker_context& self);
    void worker_cleanup(size_t slot_index);
    void scaler_function();
    size_t acquire_slot();
    bool spawn_worker();
    void apply_affinity(worker_context& ctx, size_t slot_index);
    void reset_worker_index(size_t capacity);
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

    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

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
     * @brief 设置定时任务预热窗口
     * @param window_us 预热窗口，0 表示关闭
     * @return 设置成功返回true（线程池未运行时）
     */
    bool set_warmup_window(int64_t window_us) noexcept;

    /**
     * @brief 查询定时任务预热窗口
     * @return 预热窗口（微秒）
     */
    NEFORCE_NODISCARD int64_t warmup_window() const noexcept { return warmup_window_us_; }

    /**
     * @brief 设置任务信息跟踪开关
     * @param enable 是否启用
     */
    void set_task_tracking(bool enable) noexcept;

    /**
     * @brief 查询任务信息跟踪开关
     * @return 已启用返回true
     */
    NEFORCE_NODISCARD bool task_tracking() const noexcept { return task_tracking_enabled_; }

    /**
     * @brief 设置工作线程 CPU 绑定开关
     * @param enable 是否启用
     * @return 设置成功返回true（线程池未运行时）
     * @note 启用后工作线程会绑定到进程允许 CPU 集合内的不同核心，避免被调度器迁移
     */
    bool set_cpu_affinity(bool enable) noexcept;

    /**
     * @brief 查询工作线程 CPU 绑定开关
     * @return 已启用返回true
     */
    NEFORCE_NODISCARD bool cpu_affinity() const noexcept { return affinity_enabled_; }

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
    bool set_thread_threshhold(size_t threshhold);

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
     * @brief 投递任务
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param priority 任务优先级
     * @param func 可调用对象
     * @param args 参数
     * @note 适用于 fire-and-forget 场景
     * @warning 任务抛出的异常会计入工作线程的失败计数而不传递
     */
    template <typename Func, typename... Args>
    void post_task(priority_type priority, Func&& func, Args&&... args);

    /**
     * @brief 投递任务（使用默认优先级0）
     * @tparam Func 可调用对象类型
     * @tparam Args 参数类型
     * @param func 可调用对象
     * @param args 参数
     */
    template <typename Func, typename... Args>
    void post_task(Func&& func, Args&&... args) {
        this->post_task(static_cast<priority_type>(0), _NEFORCE forward<Func>(func), _NEFORCE forward<Args>(args)...);
    }

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

    auto info = make_task_info(priority);
    const bool tracked = task_tracking_enabled_.load(memory_order_relaxed);

    const auto current_group = get_current_task_group();
    if (current_group) {
        current_group->increment();
    }

    auto task = _NEFORCE make_shared<packaged_task<Result()>>(
            [func = _NEFORCE forward<Func>(func), args = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...),
             group = current_group, info, tracked]() mutable -> Result {
                struct context_guard {
                    shared_ptr<task_info> info;
                    shared_ptr<task_group> group_inner;
                    shared_ptr<task_group> prev_group_inner;
                    bool tracked;

                    context_guard(shared_ptr<task_info> i, shared_ptr<task_group> g, const bool track) :
                    info(move(i)),
                    group_inner(move(g)),
                    tracked(track) {
                        if (tracked) {
                            info->status.store(task_info::status::running, memory_order_release);
                            info->start_time = timestamp::now();
                            info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;
                        }

                        prev_group_inner = get_current_task_group();
                        get_current_task_group() = group_inner;
                    }

                    ~context_guard() noexcept {
                        try {
                            if (tracked) {
                                info->finish_time = timestamp::now();
                                auto expected = task_info::status::running;
                                info->status.compare_exchange_strong(expected, task_info::status::completed,
                                                                     memory_order_release);
                            }

                            get_current_task_group() = prev_group_inner;
                            if (group_inner) {
                                group_inner->decrement();
                            }
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (...) {
                            /* ignore */
                        }
                    }
                };

                context_guard guard(info, group, tracked);
                try {
                    return _NEFORCE apply(func, args);
                } catch (const exception& e) {
                    if (tracked) {
                        info->status.store(task_info::status::failed, memory_order_release);
                        info->error = e.what();
                    }
                    throw;
                } catch (...) {
                    if (tracked) {
                        info->status.store(task_info::status::failed, memory_order_release);
                        info->error = "Unknown exception";
                    }
                    throw;
                }
            });

    auto res = task->get_future();
    task_type job([task] { (*task)(); });

    if (!dispatch_task(_NEFORCE move(job), priority, info) && tracked) {
        info->status.store(task_info::status::failed, memory_order_release);
        info->error = global_queue_ == nullptr ? "Thread pool is not running" : "Task queue is full";
    }

    return submit_result<Result>{_NEFORCE move(res), _NEFORCE move(info)};
}

template <typename Func, typename... Args>
void thread_pool::post_task(priority_type priority, Func&& func, Args&&... args) {
    static_assert(is_invocable_v<Func, Args...>, "Func must be invocable with Args");

    task_type job([func = _NEFORCE forward<Func>(func), args = _NEFORCE make_tuple(_NEFORCE forward<Args>(
                                                                args)...)]() mutable { _NEFORCE apply(func, args); });

    static const shared_ptr<task_info> untracked;
    (void) dispatch_task(_NEFORCE move(job), priority, untracked);
}

template <typename Func, typename... Args>
submit_result<invoke_result_t<Func, Args...>>
thread_pool::submit_after(const int64_t delay_ms, const priority_type priority, Func&& func, Args&&... args) {
    static_assert(is_invocable_v<Func, Args...>, "Func must be invocable with Args");

    using Result = invoke_result_t<Func, Args...>;

    auto info = make_task_info(priority);
    const bool tracked = task_tracking_enabled_.load(memory_order_relaxed);

    auto task = _NEFORCE make_shared<packaged_task<Result()>>(
            [func = _NEFORCE forward<Func>(func), tup = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...), info,
             tracked]() mutable {
                struct context_guard {
                    shared_ptr<task_info> info;
                    bool tracked;

                    context_guard(shared_ptr<task_info> i, const bool track) :
                    info(move(i)),
                    tracked(track) {
                        if (tracked) {
                            info->status.store(task_info::status::running, memory_order_release);
                            info->start_time = timestamp::now();
                            info->worker_thread_id = get_worker_context() ? get_worker_context()->id : 0;
                        }
                    }

                    ~context_guard() noexcept {
                        if (tracked) {
                            info->finish_time = timestamp::now();
                            auto expected = task_info::status::running;
                            info->status.compare_exchange_strong(expected, task_info::status::completed,
                                                                 memory_order_release);
                        }
                    }
                };

                context_guard guard(info, tracked);

                try {
                    return _NEFORCE apply(func, tup);
                } catch (const exception& e) {
                    if (tracked) {
                        info->status.store(task_info::status::failed, memory_order_release);
                        info->error = e.what();
                    }
                    throw;
                }
            });

    auto res = task->get_future();

    const auto expire_time = steady_clock::now() + milliseconds(delay_ms);
    const auto window_us = warmup_window_us_;
    if (window_us > 0 && timer_) {
        const auto deadline_ns = time_cast<nanoseconds>(expire_time - steady_clock::time_point{}).count();
        const auto prewarm_time = expire_time - microseconds(window_us);
        if (prewarm_time > steady_clock::now()) {
            timer_->add_task(prewarm_time, [this, deadline_ns] { prewarm_dispatch(deadline_ns); });
        }
    }
    timer_->add_task(expire_time, [this, task = _NEFORCE move(task), priority]() mutable {
        this->submit_task(priority, [task]() { (*task)(); });
    });

    return submit_result<Result>{_NEFORCE move(res), _NEFORCE move(info)};
}

template <typename Func, typename... Args>
thread_pool::periodic_token thread_pool::submit_every(int64_t interval_ms, const priority_type priority, Func&& func,
                                                      Args&&... args) {
    auto state = make_shared<periodic_task_state>();
    auto task = _NEFORCE make_shared<function<void()>>(
            [func = _NEFORCE forward<Func>(func),
             tup = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...)]() mutable { _NEFORCE apply(func, tup); });
    auto handler_ptr = _NEFORCE make_shared<task_type>();
    weak_ptr<task_type> weak_handler(handler_ptr);
    *handler_ptr = [this, state, task, interval_ms, priority, weak_handler]() {
        if (state->cancelled.load()) {
            return;
        }

        this->submit_task(priority, [task]() { (*task)(); });

        if (state->cancelled.load()) {
            return;
        }
        if (auto locked = weak_handler.lock()) {
            auto next_time = steady_clock::now() + milliseconds(interval_ms);
            timer_->add_task(next_time, [locked]() { (*locked)(); });
        }
    };

    auto first_time = steady_clock::now() + milliseconds(interval_ms);
    timer_->add_task(first_time, [handler_ptr]() { (*handler_ptr)(); });
    return state;
}

/// @endcond

/** @} */ // ThreadPool

/**
 * @addtogroup Executor 执行器
 * @{
 */

/**
 * @struct thread_pool_executor
 * @brief thread_pool 的轻量执行器适配器
 *
 * 将 thread_pool 的 submit_task() 适配为 executor 接口。
 */
struct thread_pool_executor {
    thread_pool* pool;

    /**
     * @brief 提交 handler 到线程池
     * @param handler 要执行的 handler
     */
    void execute(function<void()> handler) { pool->submit_task(move(handler)); }

    /**
     * @brief 检查当前线程是否为线程池 worker
     * @return 在 worker 线程中返回 true
     */
    NEFORCE_NODISCARD bool running_in_this_thread() const noexcept { return get_worker_context() != nullptr; }
};

/** @} */ // Executor

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_POOL_HPP__
