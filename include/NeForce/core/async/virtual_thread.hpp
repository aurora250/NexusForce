#ifndef NEFORCE_CORE_ASYNC_VIRTUAL_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_VIRTUAL_THREAD_HPP__

/**
 * @file virtual_thread.hpp
 * @brief 虚拟线程实现
 *
 * 核心能力:
 *  - virtual_thread_task<T>  带返回值的异步任务
 *  - co_await 协程等待支持
 *  - 协程调度器自动管理
 *  - yield / sleep 协作式调度
 *  - get_result() 阻塞获取结果
 */

#include "NeForce/core/async/coroutine.hpp"
#ifdef NEFORCE_STANDARD_20
#    include "NeForce/core/async/atomic.hpp"
#    include "NeForce/core/async/condition_variable.hpp"
#    include "NeForce/core/async/mutex.hpp"
#    include "NeForce/core/async/thread.hpp"
#    include "NeForce/core/container/priority_queue.hpp"
#    include "NeForce/core/container/queue.hpp"
#    include "NeForce/core/container/vector.hpp"
#    include "NeForce/core/exception/exception_ptr.hpp"
#    include "NeForce/core/functional/functor.hpp"
#    include "NeForce/core/memory/aligned_buffer.hpp"
#    include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup VirtualThread 虚拟线程
 * @brief 虚拟线程相关功能
 * @{
 */

/**
 * @brief 异步任务主模板
 * @tparam T 任务返回值类型，默认为 void
 */
template <typename T = void>
struct virtual_thread_task;

/**
 * @brief 判断类型是否为 virtual_thread_task 特化
 * @tparam T 待检查的类型
 */
template <typename T>
struct is_virtual_thread_task : false_type {};

/// @cond INTERNAL
template <typename T>
struct is_virtual_thread_task<virtual_thread_task<T>> : true_type {};
/// @endcond

/**
 * @brief is_virtual_thread_task 的便捷变量模板
 */
template <typename T>
constexpr bool is_virtual_thread_task_v = is_virtual_thread_task<T>::value;


NEFORCE_BEGIN_INNER__

struct task_shared_state_base {
    /// @brief 帧所有权标记
    /// @note 任何把帧交给第三方的挂起点都必须置位，否则拥有者可能销毁一个仍会被恢复的帧
    atomic<bool> detached_{false};
};

template <typename T>
struct task_shared_state : task_shared_state_base {
    aligned_buffer<T> result_buffer_;
    bool has_value_{false};
    exception_ptr exception_{nullptr};

    /// 等待此任务的协程句柄
    coroutine_handle<> continuation_{nullptr};
    atomic<bool> completed_{false};
    mutex mtx_;
    condition_variable cv_;
    atomic<unsigned long> ref_count_{1};

    void add_ref() noexcept { ref_count_.fetch_add(1, memory_order_relaxed); }

    void release() noexcept {
        if (ref_count_.fetch_sub(1, memory_order_acq_rel) == 1) {
            if (has_value_) {
                result_buffer_.ptr()->~T();
            }
            delete this;
        }
    }

    static task_shared_state* create() { return new task_shared_state(); }
};

template <>
struct task_shared_state<void> : task_shared_state_base {
    exception_ptr exception_{nullptr};

    /// 等待此任务的协程句柄
    coroutine_handle<> continuation_{nullptr};
    atomic<bool> completed_{false};
    mutex mtx_;
    condition_variable cv_;
    atomic<unsigned long> ref_count_{1};

    void add_ref() noexcept { ref_count_.fetch_add(1, memory_order_relaxed); }

    void release() noexcept {
        if (ref_count_.fetch_sub(1, memory_order_acq_rel) == 1) {
            delete this;
        }
    }

    static task_shared_state* create() { return new task_shared_state(); }
};

NEFORCE_END_INNER__


/**
 * @class virtual_thread_scheduler
 * @brief 虚拟线程调度器
 */
class virtual_thread_scheduler {
private:
    /**
     * @struct timer_entry
     * @brief 定时器条目
     */
    struct timer_entry {
        steady_clock::time_point deadline; ///< 绝对到期时间
        uint64_t sequence;                 ///< 入队序号
        coroutine_handle<> handle;         ///< 需要恢复的协程

        NEFORCE_NODISCARD bool operator>(const timer_entry& other) const noexcept {
            if (deadline != other.deadline) {
                return deadline > other.deadline;
            }
            return sequence > other.sequence;
        }
    };

    queue<coroutine_handle<>> task_queue_;                                          ///< 协程任务队列
    priority_queue<timer_entry, vector<timer_entry>, greater<timer_entry>> timers_; ///< 定时器最小堆
    uint64_t next_timer_sequence_{0};                                               ///< 定时器序号计数器
    vector<thread> workers_;                                                        ///< 工作线程池
    mutex mutex_;                                                                   ///< 保护任务队列与定时器堆的互斥锁
    condition_variable cv_;                                                         ///< 任务/定时器通知条件变量
    atomic<bool> shutdown_{false};                                                  ///< 关闭标志

    void pop_expired_timers_locked() {
        const auto now = steady_clock::now();
        while (!timers_.empty() && timers_.top().deadline <= now) {
            task_queue_.push(timers_.top().handle);
            timers_.pop();
        }
    }

public:
    /**
     * @brief 获取调度器单例实例
     * @return 调度器引用
     */
    static virtual_thread_scheduler& get_instance() {
        static virtual_thread_scheduler instance;
        return instance;
    }

    /**
     * @brief 将协程加入调度队列
     * @param handle 协程句柄
     */
    void schedule(coroutine_handle<> handle) {
        {
            lock<mutex> lock(mutex_);
            task_queue_.push(handle);
        }
        cv_.notify_one();
    }

    /**
     * @brief 将协程延迟指定毫秒后加入调度队列
     * @param handle 协程句柄
     * @param delay_ms 延迟毫秒数，非正数表示立即调度
     */
    void schedule_after(coroutine_handle<> handle, const int64_t delay_ms) {
        if (delay_ms <= 0) {
            schedule(handle);
            return;
        }
        {
            lock<mutex> lock(mutex_);
            timers_.push(timer_entry{steady_clock::now() + milliseconds(delay_ms), next_timer_sequence_++, handle});
        }
        cv_.notify_all();
    }

    /**
     * @brief 启动指定数量的工作线程
     * @param num_threads 工作线程数量
     */
    void start_workers(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    /**
     * @brief 关闭调度器
     *
     * 设置关闭标志，唤醒所有工作线程并等待它们退出。
     */
    void shutdown() {
        {
            lock<mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();

        for (auto& worker: workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    /**
     * @brief 析构函数，自动调用 shutdown()
     */
    ~virtual_thread_scheduler() { shutdown(); }

private:
    /**
     * @brief 工作线程主循环
     *
     * 循环等待任务队列中的协程，取出并恢复执行。
     */
    void worker_loop() {
        while (true) {
            coroutine_handle<> handle;

            {
                unique_lock<mutex> lock(mutex_);
                for (;;) {
                    pop_expired_timers_locked();

                    if (shutdown_ && task_queue_.empty() && timers_.empty()) {
                        return;
                    }

                    if (!task_queue_.empty()) {
                        handle = task_queue_.front();
                        task_queue_.pop();
                        break;
                    }

                    if (timers_.empty()) {
                        cv_.wait(lock, [this] { return shutdown_ || !task_queue_.empty() || !timers_.empty(); });
                        continue;
                    }

                    const auto deadline = timers_.top().deadline;
                    cv_.wait_until(lock, deadline, [this, deadline] {
                        return shutdown_ || !task_queue_.empty() || timers_.empty() ||
                               timers_.top().deadline != deadline;
                    });
                }
            }

            if (handle) {
                handle.resume();
            }
        }
    }
};


NEFORCE_BEGIN_INNER__

struct yield_tag {};

struct sleep_tag {
    int64_t ms_;
};

/**
 * @brief 判断某个 promise 是否属于 virtual_thread_task 协程
 * @tparam Promise 待检查的 promise 类型
 */
template <typename Promise, typename = void>
struct is_virtual_thread_promise : false_type {};

/// @cond
template <typename Promise>
struct is_virtual_thread_promise<Promise, void_t<decltype(Promise::is_virtual_thread_task_promise)>>
: bool_constant<Promise::is_virtual_thread_task_promise> {};
/// @endcond

/**
 * @brief is_virtual_thread_promise 的便捷变量模板
 */
template <typename Promise>
constexpr bool is_virtual_thread_promise_v = is_virtual_thread_promise<Promise>::value;

/**
 * @brief 把等待方的协程帧标记为已交付他人调度
 * @tparam Promise 等待方的 promise 类型
 * @param caller 等待方的协程句柄
 *
 * 仅当等待方是 virtual_thread_task 协程时置位其共享状态的 detached 标记
 */
template <typename Promise>
void detach_continuation_frame(coroutine_handle<Promise> caller) noexcept {
    if constexpr (is_virtual_thread_promise_v<Promise>) {
        caller.promise().shared_state_->detached_.store(true, memory_order_release);
    }
}

NEFORCE_END_INNER__


/**
 * @brief virtual_thread_task 的 void 特化版本
 *
 * 表示一个返回 void 的异步任务，支持 co_await 等待和 get_result() 同步获取结果。
 * 任务结果（或异常）存储在引用计数的堆对象中，确保协程帧销毁后仍可安全访问。
 */
template <>
struct virtual_thread_task<void> {
    /**
     * @brief 协程 promise_type，管理任务生命周期与状态
     */
    struct promise_type {
        /// @brief 标记本 promise 属于 virtual_thread_task
        static constexpr bool is_virtual_thread_task_promise = true;

        inner::task_shared_state<void>* shared_state_{inner::task_shared_state<void>::create()};

        /**
         * @brief 创建返回给调用者的任务对象
         * @return 关联此协程的 virtual_thread_task
         */
        virtual_thread_task get_return_object() {
            auto task = virtual_thread_task{coroutine_handle<promise_type>::from_promise(*this)};
            return task;
        }

        /**
         * @brief 初始挂起点
         */
        suspend_never initial_suspend() noexcept { return {}; }

        /**
         * @brief 最终挂起点
         *
         * 任务完成时标记 completed_、通知等待者，并将 continuation 送入调度器。
         */
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }

                bool await_suspend(coroutine_handle<promise_type> h) noexcept {
                    auto& p = h.promise();
                    auto* state = p.shared_state_;
                    coroutine_handle<> cont{nullptr};

                    {
                        lock<mutex> lock(state->mtx_);
                        state->completed_.store(true, memory_order_release);
                        state->cv_.notify_all();

                        cont = state->continuation_;
                        state->continuation_ = nullptr;
                    }

                    if (cont) {
                        try {
                            virtual_thread_scheduler::get_instance().schedule(cont);
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (const exception& e) {
                            NEFORCE_REPORT_EXCEPTION(e);
                        }
                    }
                    return false;
                }

                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }

        /**
         * @brief 处理 co_await yield
         * @return yield 等待器
         */
        auto await_transform(inner::yield_tag /*unused*/) {
            struct yield_awaiter {
                inner::task_shared_state<void>* shared_state_;

                NEFORCE_NODISCARD bool await_ready() const noexcept { return false; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->detached_.store(true, memory_order_release);
                    virtual_thread_scheduler::get_instance().schedule(handle);
                }

                void await_resume() const noexcept {}
            };
            return yield_awaiter{shared_state_};
        }

        /**
         * @brief 处理 co_await sleep
         * @param tag 包含休眠时长的标记
         * @return sleep 等待器
         */
        auto await_transform(inner::sleep_tag tag) {
            struct sleep_awaiter_impl {
                inner::task_shared_state<void>* shared_state_;
                int64_t ms_;

                NEFORCE_NODISCARD bool await_ready() const noexcept { return ms_ <= 0; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->detached_.store(true, memory_order_release);
                    virtual_thread_scheduler::get_instance().schedule_after(handle, ms_);
                }

                void await_resume() const noexcept {}
            };
            return sleep_awaiter_impl{shared_state_, tag.ms_};
        }

        /**
         * @brief 通用 await_transform
         */
        template <typename Awaiter>
        decltype(auto) await_transform(Awaiter&& a) {
            return _NEFORCE forward<Awaiter>(a);
        }

        /**
         * @brief co_return 无返回值
         */
        void return_void() {}

        /**
         * @brief 未处理异常的捕获入口
         */
        void unhandled_exception() { shared_state_->exception_ = _NEFORCE current_exception(); }

        /**
         * @brief 析构时释放共享状态的引用
         */
        ~promise_type() {
            if (shared_state_ != nullptr) {
                shared_state_->release();
            }
        }
    };

    coroutine_handle<promise_type> handle_{nullptr};        ///< 协程句柄
    inner::task_shared_state<void>* shared_state_{nullptr}; ///< 共享状态指针

    /**
     * @brief 默认构造，创建空任务
     */
    virtual_thread_task() = default;

    /**
     * @brief 从协程句柄构造任务
     * @param h 协程句柄
     */
    explicit virtual_thread_task(coroutine_handle<promise_type> h) :
    handle_(h) {
        if (handle_) {
            shared_state_ = handle_.promise().shared_state_;
            if (shared_state_ != nullptr) {
                shared_state_->add_ref();
            }
        }
    }

    /**
     * @brief 析构函数
     *
     * 释放共享状态的引用。
     */
    ~virtual_thread_task() {
        if (handle_) {
            const bool completed = shared_state_ != nullptr && shared_state_->completed_.load(memory_order_acquire);
            const bool detached = shared_state_ != nullptr && shared_state_->detached_.load(memory_order_acquire);
            if (!completed && !detached) {
                handle_.destroy();
            }
        }
        if (shared_state_ != nullptr) {
            shared_state_->release();
        }
    }

    virtual_thread_task(const virtual_thread_task&) = delete;
    virtual_thread_task& operator=(const virtual_thread_task&) = delete;

    /**
     * @brief 移动构造函数
     */
    virtual_thread_task(virtual_thread_task&& other) noexcept :
    handle_(_NEFORCE exchange(other.handle_, nullptr)),
    shared_state_(_NEFORCE exchange(other.shared_state_, nullptr)) {}

    /**
     * @brief 移动赋值运算符
     */
    virtual_thread_task& operator=(virtual_thread_task&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }
        if (handle_) {
            const bool completed = shared_state_ != nullptr && shared_state_->completed_.load(memory_order_acquire);
            const bool detached = shared_state_ != nullptr && shared_state_->detached_.load(memory_order_acquire);
            if (!completed && !detached) {
                handle_.destroy();
            }
        }
        if (shared_state_ != nullptr) {
            shared_state_->release();
        }
        handle_ = _NEFORCE exchange(other.handle_, nullptr);
        shared_state_ = _NEFORCE exchange(other.shared_state_, nullptr);
        return *this;
    }

    /**
     * @class awaiter
     * @brief co_await 使用的等待器
     */
    class awaiter {
    private:
        inner::task_shared_state<void>* state_{nullptr};

        void release() noexcept {
            if (state_ != nullptr) {
                auto* state = _NEFORCE exchange(state_, nullptr);
                state->release();
            }
        }

    public:
        explicit awaiter(inner::task_shared_state<void>* state) noexcept :
        state_(state) {
            if (state_ != nullptr) {
                state_->add_ref();
            }
        }

        ~awaiter() { release(); }

        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;

        awaiter(awaiter&& other) noexcept :
        state_(_NEFORCE exchange(other.state_, nullptr)) {}

        awaiter& operator=(awaiter&& other) noexcept {
            if (addressof(other) != this) {
                release();
                state_ = _NEFORCE exchange(other.state_, nullptr);
            }
            return *this;
        }

        /**
         * @brief co_await 就绪检查
         * @return 任务是否已完成
         */
        NEFORCE_NODISCARD bool await_ready() const noexcept {
            return state_ == nullptr || state_->completed_.load(memory_order_acquire);
        }

        /**
         * @brief co_await 挂起时注册 continuation
         * @tparam Promise 等待方协程的 promise 类型
         * @param caller 等待此任务的协程句柄
         * @return true 需要挂起，false 已可继续
         */
        template <typename Promise>
        bool await_suspend(coroutine_handle<Promise> caller) noexcept {
            if (state_ == nullptr) {
                return false;
            }
            lock<mutex> lock(state_->mtx_);
            if (state_->completed_.load(memory_order_acquire)) {
                return false;
            }
            inner::detach_continuation_frame(caller);
            state_->continuation_ = caller;
            return true;
        }

        /**
         * @brief co_await 恢复时检查异常
         */
        void await_resume() const {
            if (state_ != nullptr && state_->exception_) {
                rethrow_exception(state_->exception_);
            }
        }
    };

    /**
     * @brief 获取 co_await 等待器
     * @return 持有共享状态引用的等待器
     */
    NEFORCE_NODISCARD awaiter operator co_await() const& noexcept { return awaiter{shared_state_}; }

    /**
     * @brief 获取 co_await 等待器
     * @return 持有共享状态引用的等待器
     */
    NEFORCE_NODISCARD awaiter operator co_await() && noexcept { return awaiter{shared_state_}; }

    /**
     * @brief 阻塞等待任务完成并获取结果
     *
     * 若任务未完成，阻塞当前线程直到任务完成。
     * 若任务抛出异常，在此重新抛出。
     */
    void get_result() {
        if (!shared_state_->completed_.load(memory_order_acquire)) {
            unique_lock<mutex> lock(shared_state_->mtx_);
            shared_state_->cv_.wait(lock, [this] { return shared_state_->completed_.load(memory_order_acquire); });
        }
        if (shared_state_->exception_) {
            rethrow_exception(shared_state_->exception_);
        }
    }

    /**
     * @brief 检查任务是否已完成
     */
    NEFORCE_NODISCARD bool is_done() const noexcept {
        return shared_state_ != nullptr && shared_state_->completed_.load(memory_order_acquire);
    }

    /**
     * @brief 检查任务是否关联有效共享状态
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return shared_state_ != nullptr; }
};


/**
 * @brief virtual_thread_task 的类型化版本
 * @tparam T 任务返回值类型
 *
 * 表示一个返回 T 类型值的异步任务。
 * 提供带返回值的 get_result() 与 co_await 等待。
 */
template <typename T>
struct virtual_thread_task {
    /**
     * @brief 协程 promise_type，管理任务生命周期与返回值存储
     */
    struct promise_type {
        /// @brief 标记本 promise 属于 virtual_thread_task
        static constexpr bool is_virtual_thread_task_promise = true;

        inner::task_shared_state<T>* shared_state_{inner::task_shared_state<T>::create()};

        /**
         * @brief 创建返回给调用者的任务对象
         */
        virtual_thread_task get_return_object() {
            auto task = virtual_thread_task{coroutine_handle<promise_type>::from_promise(*this)};
            return task;
        }

        /**
         * @brief 初始挂起点 — 不暂停
         */
        suspend_never initial_suspend() noexcept { return {}; }

        /**
         * @brief 最终挂起点
         *
         * 标记完成、通知等待者、调度 continuation。
         */
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }

                bool await_suspend(coroutine_handle<promise_type> h) noexcept {
                    auto& p = h.promise();
                    auto* state = p.shared_state_;
                    coroutine_handle<> cont{nullptr};

                    {
                        lock<mutex> lock(state->mtx_);
                        state->completed_.store(true, memory_order_release);
                        state->cv_.notify_all();

                        cont = state->continuation_;
                        state->continuation_ = nullptr;
                    }

                    if (cont) {
                        try {
                            virtual_thread_scheduler::get_instance().schedule(cont);
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (const exception& e) {
                            NEFORCE_REPORT_EXCEPTION(e);
                        }
                    }
                    return false;
                }

                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }

        /**
         * @brief 处理 co_await yield
         */
        auto await_transform(inner::yield_tag /*unused*/) {
            struct yield_awaiter {
                inner::task_shared_state<T>* shared_state_;

                NEFORCE_NODISCARD bool await_ready() const noexcept { return false; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->detached_.store(true, memory_order_release);
                    virtual_thread_scheduler::get_instance().schedule(handle);
                }

                void await_resume() const noexcept {}
            };
            return yield_awaiter{shared_state_};
        }

        /**
         * @brief 处理 co_await sleep
         * @param tag 包含休眠时长的标记
         */
        auto await_transform(inner::sleep_tag tag) {
            struct sleep_awaiter_impl {
                inner::task_shared_state<T>* shared_state_;
                int64_t ms_;

                NEFORCE_NODISCARD bool await_ready() const noexcept { return ms_ <= 0; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->detached_.store(true, memory_order_release);
                    virtual_thread_scheduler::get_instance().schedule_after(handle, ms_);
                }

                void await_resume() const noexcept {}
            };
            return sleep_awaiter_impl{shared_state_, tag.ms_};
        }

        /**
         * @brief co_return 值，拷贝存储
         */
        void return_value(const T& value) {
            ::new (shared_state_->result_buffer_.addr()) T(value);
            shared_state_->has_value_ = true;
        }

        /**
         * @brief co_return 值，移动存储
         */
        void return_value(T&& value) {
            ::new (shared_state_->result_buffer_.addr()) T(move(value));
            shared_state_->has_value_ = true;
        }

        /**
         * @brief 通用 await_transform，透传自定义等待器
         */
        template <typename Awaiter>
        decltype(auto) await_transform(Awaiter&& a) {
            return _NEFORCE forward<Awaiter>(a);
        }

        /**
         * @brief 未处理异常的捕获入口
         */
        void unhandled_exception() { shared_state_->exception_ = _NEFORCE current_exception(); }

        /**
         * @brief 析构时释放共享状态的引用
         */
        ~promise_type() {
            if (shared_state_) {
                shared_state_->release();
            }
        }
    };

    coroutine_handle<promise_type> handle_{nullptr};     ///< 协程句柄
    inner::task_shared_state<T>* shared_state_{nullptr}; ///< 共享状态指针

    /**
     * @brief 默认构造，创建空任务
     */
    virtual_thread_task() = default;

    /**
     * @brief 从协程句柄构造任务
     */
    explicit virtual_thread_task(coroutine_handle<promise_type> h) :
    handle_(h) {
        if (handle_) {
            shared_state_ = handle_.promise().shared_state_;
            if (shared_state_) {
                shared_state_->add_ref();
            }
        }
    }

    /**
     * @brief 析构函数 — 清理未调度的帧，释放共享状态引用
     */
    ~virtual_thread_task() {
        if (handle_) {
            const bool completed = shared_state_ && shared_state_->completed_.load(memory_order_acquire);
            const bool detached = shared_state_ && shared_state_->detached_.load(memory_order_acquire);
            if (!completed && !detached) {
                handle_.destroy();
            }
        }
        if (shared_state_) {
            shared_state_->release();
        }
    }

    virtual_thread_task(const virtual_thread_task&) = delete;
    virtual_thread_task& operator=(const virtual_thread_task&) = delete;

    /**
     * @brief 移动构造函数
     */
    virtual_thread_task(virtual_thread_task&& other) noexcept :
    handle_(_NEFORCE exchange(other.handle_, nullptr)),
    shared_state_(_NEFORCE exchange(other.shared_state_, nullptr)) {}

    /**
     * @brief 移动赋值运算符
     */
    virtual_thread_task& operator=(virtual_thread_task&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }
        if (handle_) {
            const bool completed = shared_state_ && shared_state_->completed_.load(memory_order_acquire);
            const bool detached = shared_state_ && shared_state_->detached_.load(memory_order_acquire);
            if (!completed && !detached) {
                handle_.destroy();
            }
        }
        if (shared_state_) {
            shared_state_->release();
        }
        handle_ = _NEFORCE exchange(other.handle_, nullptr);
        shared_state_ = _NEFORCE exchange(other.shared_state_, nullptr);
        return *this;
    }

    /**
     * @class awaiter
     * @brief co_await 使用的等待器
     */
    class awaiter {
    private:
        inner::task_shared_state<T>* state_{nullptr};

        void release() noexcept {
            if (state_ != nullptr) {
                auto* state = _NEFORCE exchange(state_, nullptr);
                state->release();
            }
        }

    public:
        explicit awaiter(inner::task_shared_state<T>* state) noexcept :
        state_(state) {
            if (state_ != nullptr) {
                state_->add_ref();
            }
        }

        ~awaiter() { release(); }

        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;

        awaiter(awaiter&& other) noexcept :
        state_(_NEFORCE exchange(other.state_, nullptr)) {}

        awaiter& operator=(awaiter&& other) noexcept {
            if (addressof(other) != this) {
                release();
                state_ = _NEFORCE exchange(other.state_, nullptr);
            }
            return *this;
        }

        /**
         * @brief co_await 就绪检查
         * @return 任务是否已完成
         */
        NEFORCE_NODISCARD bool await_ready() const noexcept {
            return state_ == nullptr || state_->completed_.load(memory_order_acquire);
        }

        /**
         * @brief co_await 挂起时注册 continuation
         * @tparam Promise 等待方协程的 promise 类型
         * @param caller 等待此任务的协程句柄
         * @return true 需要挂起，false 已可继续
         */
        template <typename Promise>
        bool await_suspend(coroutine_handle<Promise> caller) noexcept {
            if (state_ == nullptr) {
                return false;
            }
            lock<mutex> lock(state_->mtx_);
            if (state_->completed_.load(memory_order_acquire)) {
                return false;
            }
            inner::detach_continuation_frame(caller);
            state_->continuation_ = caller;
            return true;
        }

        /**
         * @brief co_await 恢复时返回结果或抛出异常
         * @return 任务的返回值
         */
        T await_resume() const {
            if (state_ == nullptr) {
                return T{};
            }
            if (state_->exception_) {
                rethrow_exception(state_->exception_);
            }
            return move(*state_->result_buffer_.ptr());
        }
    };

    /**
     * @brief 获取 co_await 等待器
     * @return 持有共享状态引用的等待器
     */
    NEFORCE_NODISCARD awaiter operator co_await() const& noexcept { return awaiter{shared_state_}; }

    /**
     * @brief 获取 co_await 等待器
     * @return 持有共享状态引用的等待器
     */
    NEFORCE_NODISCARD awaiter operator co_await() && noexcept { return awaiter{shared_state_}; }

    /**
     * @brief 阻塞获取任务结果
     *
     * 若任务未完成则阻塞当前线程。任务异常会在此重新抛出。
     * @return 任务的返回值
     */
    T get_result() {
        if (!shared_state_->completed_.load(memory_order_acquire)) {
            unique_lock<mutex> lock(shared_state_->mtx_);
            shared_state_->cv_.wait(lock, [this] { return shared_state_->completed_.load(memory_order_acquire); });
        }
        if (shared_state_->exception_) {
            rethrow_exception(shared_state_->exception_);
        }
        return move(*shared_state_->result_buffer_.ptr());
    }

    /**
     * @brief 检查任务是否已完成
     */
    NEFORCE_NODISCARD bool is_done() const noexcept {
        return shared_state_ && shared_state_->completed_.load(memory_order_acquire);
    }

    /**
     * @brief 检查任务是否关联有效共享状态
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return shared_state_ != nullptr; }
};

/**
 * @class virtual_thread
 * @brief 虚拟线程用户门面类
 *
 * 提供启动异步任务、yield、sleep 等操作的静态接口。
 * 不可实例化（构造/析构为 private，仅提供静态方法）。
 */
class virtual_thread {
private:
    /**
     * @brief 将普通函数包装为协程任务
     * @tparam Func 可调用类型
     * @param func 要包装的函数
     * @return virtual_thread_task<void>
     */
    template <typename Func>
    static virtual_thread_task<void> create_task(Func func) {
        func();
        co_return;
    }

public:
    /**
     * @brief 启动异步任务
     * @tparam Func 可调用类型，返回 virtual_thread_task<T> 或普通值
     * @param func 要执行的可调用对象
     * @return 若 func 返回 virtual_thread_task<T> 则直接返回，否则包装为 virtual_thread_task<void>
     */
    template <typename Func>
    static auto start(Func&& func) {
        using result_type = invoke_result_t<decay_t<Func>>;
        if constexpr (is_virtual_thread_task_v<result_type>) {
            static_assert(!is_rvalue_reference_v<Func&&> || is_empty_v<decay_t<Func>>,
                          "virtual_thread::start() was given a temporary coroutine callable that captures state. "
                          "A coroutine frame reaches its captures through `this`, so the closure must outlive the "
                          "coroutine: bind the lambda to a named local and pass that lvalue instead.");
            return _NEFORCE forward<Func>(func)();
        } else {
            return create_task(_NEFORCE forward<Func>(func));
        }
    }

    /**
     * @brief 创建 yield 标记，用于 co_await 让出执行权
     */
    static inner::yield_tag yield() { return inner::yield_tag{}; }

    /**
     * @brief 创建 sleep 标记，用于 co_await 休眠
     * @param ms 休眠时长（毫秒）
     */
    static inner::sleep_tag sleep(const int64_t ms) { return inner::sleep_tag{ms}; }

    /**
     * @brief 初始化调度器并启动工作线程
     * @param num_threads 工作线程数量
     */
    static void initialize(size_t num_threads) { virtual_thread_scheduler::get_instance().start_workers(num_threads); }

    /**
     * @brief 关闭调度器
     */
    static void shutdown() { virtual_thread_scheduler::get_instance().shutdown(); }
};

/** @} */ // VirtualThread

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_ASYNC_VIRTUAL_THREAD_HPP__
