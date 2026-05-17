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

#ifdef NEFORCE_STANDARD_20
#    include "NeForce/core/async/atomic.hpp"
#    include "NeForce/core/async/condition_variable.hpp"
#    include "NeForce/core/async/coroutine.hpp"
#    include "NeForce/core/async/mutex.hpp"
#    include "NeForce/core/container/queue.hpp"
#    include "NeForce/core/memory/aligned_buffer.hpp"
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
    atomic<bool> scheduled_{false};
};

template <typename T>
struct task_shared_state : task_shared_state_base {
    aligned_buffer<T> result_buffer_;
    bool has_value_{false};
    exception_ptr exception_{nullptr};

    atomic<coroutine_handle<>> continuation_{nullptr};
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

    atomic<coroutine_handle<>> continuation_{nullptr};
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
 *
 * 管理协程任务的调度和执行，全局统一调度。
 * 内部维护一个任务队列和一组工作线程，工作线程从队列中取出协程并执行。
 */
class virtual_thread_scheduler {
private:
    queue<coroutine_handle<>> task_queue_; ///< 协程任务队列
    vector<thread> workers_;               ///< 工作线程池
    mutex mutex_;                          ///< 保护任务队列的互斥锁
    condition_variable cv_;                ///< 任务通知条件变量
    atomic<bool> shutdown_{false};         ///< 关闭标志

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
     * 若协程执行完毕则销毁帧，否则等待下次调度。
     */
    void worker_loop() {
        while (true) {
            coroutine_handle<> handle;

            {
                unique_lock<mutex> lock(mutex_);
                cv_.wait(lock, [this] { return shutdown_ || !task_queue_.empty(); });

                if (shutdown_ && task_queue_.empty()) {
                    return;
                }

                if (!task_queue_.empty()) {
                    handle = task_queue_.front();
                    task_queue_.pop();
                }
            }

            if (handle) {
                handle.resume();
                if (handle.done()) {
                    handle.destroy();
                }
            }
        }
    }
};


NEFORCE_BEGIN_INNER__

struct yield_tag {};

struct sleep_tag {
    int64_t ms_;
};

void mark_continuation_scheduled(coroutine_handle<> cont);

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
         * 返回 void 而非 coroutine_handle，避免对称转移导致的 use-after-free。
         */
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }

                void await_suspend(coroutine_handle<promise_type> h) noexcept {
                    auto& p = h.promise();
                    auto* state = p.shared_state_;

                    state->completed_.store(true, memory_order_release);
                    state->cv_.notify_all();

                    auto cont = state->continuation_.exchange(nullptr, memory_order_acq_rel);
                    if (cont) {
                        inner::mark_continuation_scheduled(cont);
                        virtual_thread_scheduler::get_instance().schedule(cont);
                    }
                }

                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }

        /**
         * @brief 处理 co_await yield
         * @return yield 等待器
         */
        auto await_transform(inner::yield_tag) {
            struct yield_awaiter {
                inner::task_shared_state<void>* shared_state_;

                bool await_ready() const noexcept { return false; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->scheduled_.store(true, memory_order_release);
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

                bool await_ready() const noexcept { return ms_ <= 0; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->scheduled_.store(true, memory_order_release);
                    thread([handle, ms = ms_] {
                        this_thread::sleep_for(milliseconds(ms));
                        virtual_thread_scheduler::get_instance().schedule(handle);
                    }).detach();
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
            if (shared_state_) {
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
            if (shared_state_) {
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
            bool was_scheduled = shared_state_ ? shared_state_->scheduled_.load(memory_order_acquire) : false;
            if (!was_scheduled) {
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
            bool was_scheduled = shared_state_ ? shared_state_->scheduled_.load(memory_order_acquire) : false;
            if (!was_scheduled) {
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
     * @brief co_await 就绪检查
     * @return 任务是否已完成
     */
    bool await_ready() noexcept { return shared_state_ && shared_state_->completed_.load(memory_order_acquire); }

    /**
     * @brief co_await 挂起时注册 continuation
     * @param caller 等待此任务的协程句柄
     * @return true 需要挂起，false 已可继续
     */
    bool await_suspend(coroutine_handle<> caller) noexcept {
        shared_state_->continuation_.store(caller, memory_order_release);

        if (shared_state_->completed_.load(memory_order_acquire)) {
            if (shared_state_->continuation_.exchange(nullptr, memory_order_acq_rel)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief co_await 恢复时检查异常
     */
    void await_resume() {
        if (shared_state_->exception_) {
            rethrow_exception(shared_state_->exception_);
        }
    }

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
    bool is_done() const noexcept { return shared_state_ && shared_state_->completed_.load(memory_order_acquire); }

    /**
     * @brief 检查任务是否关联有效共享状态
     */
    bool valid() const noexcept { return shared_state_ != nullptr; }
};


NEFORCE_BEGIN_INNER__

inline void mark_continuation_scheduled(coroutine_handle<> cont) {
    auto vh = coroutine_handle<virtual_thread_task<void>::promise_type>::from_address(cont.address());
    vh.promise().shared_state_->scheduled_.store(true, memory_order_release);
}

NEFORCE_END_INNER__


/**
 * @brief virtual_thread_task 的类型化版本
 * @tparam T 任务返回值类型
 *
 * 表示一个返回 T 类型值的异步任务。
 * 提供带返回值的 get_result() 和 await_resume()。
 */
template <typename T>
struct virtual_thread_task {
    /**
     * @brief 协程 promise_type，管理任务生命周期与返回值存储
     */
    struct promise_type {
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

                void await_suspend(coroutine_handle<promise_type> h) noexcept {
                    auto& p = h.promise();
                    auto* state = p.shared_state_;

                    state->completed_.store(true, memory_order_release);
                    state->cv_.notify_all();

                    auto cont = state->continuation_.exchange(nullptr, memory_order_acq_rel);
                    if (cont) {
                        inner::mark_continuation_scheduled(cont);
                        virtual_thread_scheduler::get_instance().schedule(cont);
                    }
                }

                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }

        /**
         * @brief 处理 co_await yield
         */
        auto await_transform(inner::yield_tag) {
            struct yield_awaiter {
                inner::task_shared_state<T>* shared_state_;

                bool await_ready() const noexcept { return false; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->scheduled_.store(true, memory_order_release);
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

                bool await_ready() const noexcept { return ms_ <= 0; }

                void await_suspend(coroutine_handle<> handle) const {
                    shared_state_->scheduled_.store(true, memory_order_release);
                    thread([handle, ms = ms_] {
                        this_thread::sleep_for(milliseconds(ms));
                        virtual_thread_scheduler::get_instance().schedule(handle);
                    }).detach();
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
            bool was_scheduled = shared_state_ ? shared_state_->scheduled_.load(memory_order_acquire) : false;
            if (!was_scheduled) {
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
            bool was_scheduled = shared_state_ ? shared_state_->scheduled_.load(memory_order_acquire) : false;
            if (!was_scheduled) {
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
     * @brief co_await 就绪检查
     * @return 任务是否已完成
     */
    bool await_ready() noexcept { return shared_state_ && shared_state_->completed_.load(memory_order_acquire); }

    /**
     * @brief co_await 挂起时注册 continuation
     */
    bool await_suspend(coroutine_handle<> caller) noexcept {
        shared_state_->continuation_.store(caller, memory_order_release);

        if (shared_state_->completed_.load(memory_order_acquire)) {
            if (shared_state_->continuation_.exchange(nullptr, memory_order_acq_rel)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief co_await 恢复时返回结果或抛出异常
     * @return 任务的返回值
     */
    T await_resume() {
        if (shared_state_->exception_) {
            rethrow_exception(shared_state_->exception_);
        }
        return move(*shared_state_->result_buffer_.ptr());
    }

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
    bool is_done() const noexcept { return shared_state_ && shared_state_->completed_.load(memory_order_acquire); }

    /**
     * @brief 检查任务是否关联有效共享状态
     */
    bool valid() const noexcept { return shared_state_ != nullptr; }
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
