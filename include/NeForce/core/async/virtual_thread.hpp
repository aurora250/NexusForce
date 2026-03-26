#ifndef NEFORCE_CORE_ASYNC_VIRTUAL_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_VIRTUAL_THREAD_HPP__

/**
 * @file virtual_thread.hpp
 * @brief 虚拟线程实现
 *
 * 此文件提供了虚拟线程的实现，
 * 支持轻量级并发编程，提供任务调度和协作式多任务支持。
 */

#ifdef NEFORCE_STANDARD_20
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/coroutine.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Coroutine 协程
 * @brief 协程和虚拟线程相关功能
 * @{
 */

/**
 * @struct virtual_thread_task
 * @brief 虚拟线程任务
 *
 * 表示一个协程任务，包含协程句柄和异常处理。
 */
struct virtual_thread_task {
    /**
     * @struct promise_type
     * @brief 协程承诺类型
     *
     * 定义协程的行为和状态管理。
     */
    struct promise_type {
        exception_ptr exception_;  ///< 异常存储

        /**
         * @brief 获取返回对象
         * @return 虚拟线程任务对象
         */
        virtual_thread_task get_return_object() {
            return virtual_thread_task{
                coroutine_handle<promise_type>::from_promise(*this)
            };
        }

        /**
         * @brief 初始挂起点
         * @return 立即恢复的挂起器
         */
        suspend_never initial_suspend() {
            return suspend_never{};
        }

        /**
         * @brief 最终挂起点
         * @return 总是挂起的挂起器
         */
        suspend_always final_suspend() noexcept {
            return suspend_always{};
        }

        /**
         * @brief 返回void
         */
        void return_void() {}

        /**
         * @brief 未处理异常处理
         *
         * 捕获协程中未处理的异常。
         */
        void unhandled_exception() {
            exception_ = _NEFORCE current_exception();
        }
    };

    coroutine_handle<promise_type> handle_;  ///< 协程句柄

    /**
     * @brief 构造函数
     * @param h 协程句柄
     */
    virtual_thread_task(coroutine_handle<promise_type> h)
    : handle_(h) {}

    /**
     * @brief 析构函数
     *
     * 如果协程未完成，销毁协程资源。
     */
    ~virtual_thread_task() {
        if (handle_ && !handle_.done()) {
            handle_.destroy();
        }
    }

    virtual_thread_task(const virtual_thread_task&) = delete;  ///< 禁止拷贝构造
    virtual_thread_task& operator =(const virtual_thread_task&) = delete;  ///< 禁止拷贝赋值

    /**
     * @brief 移动构造函数
     * @param other 要移动的虚拟线程任务
     */
    virtual_thread_task(virtual_thread_task&& other) noexcept
    : handle_(_NEFORCE exchange(other.handle_, nullptr)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的虚拟线程任务
     * @return 当前对象的引用
     */
    virtual_thread_task& operator =(virtual_thread_task&& other) noexcept {
        if (this != &other) {
            if (handle_ && !handle_.done()) {
                handle_.destroy();
            }
            handle_ = _NEFORCE exchange(other.handle_, nullptr);
        }
        return *this;
    }
};

/**
 * @class virtual_thread_scheduler
 * @brief 虚拟线程调度器
 *
 * 管理协程任务的调度和执行，全局统一调度。
 */
class virtual_thread_scheduler {
private:
    queue<coroutine_handle<>> task_queue_;  ///< 任务队列
    vector<thread> workers_;  ///< 工作线程集合
    mutex mutex_;  ///< 队列保护互斥锁
    condition_variable cv_;  ///< 条件变量
    atomic<bool> shutdown_;  ///< 关闭标志

    /**
     * @brief 默认构造函数
     */
    virtual_thread_scheduler()
    : shutdown_(false) {}

    /**
     * @brief 工作线程循环
     *
     * 从任务队列获取协程任务并执行。
     */
    void worker_loop() {
        while (true) {
            coroutine_handle<> handle;

            {
                smart_lock<mutex> lock(mutex_);
                cv_.wait(lock, [this] {
                    return shutdown_ || !task_queue_.empty();
                });

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
            }
        }
    }

public:
    /**
     * @brief 获取调度器实例
     * @return 调度器单例引用
     */
    static virtual_thread_scheduler& get_instance() {
        static virtual_thread_scheduler instance;
        return instance;
    }

    /**
     * @brief 调度协程任务
     * @param handle 协程句柄
     *
     * 将协程任务加入任务队列，唤醒工作线程执行。
     */
    void schedule(coroutine_handle<> handle) {
        {
            lock<mutex> lock(mutex_);
            task_queue_.push(handle);
        }
        cv_.notify_one();
    }

    /**
     * @brief 启动工作线程
     * @param num_threads 工作线程数量
     *
     * 创建指定数量的工作线程处理协程任务。
     */
    void start_workers(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    /**
     * @brief 关闭调度器
     *
     * 通知所有工作线程退出，并等待线程结束。
     */
    void shutdown() {
        {
            lock<mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    /**
     * @brief 析构函数
     *
     * 自动关闭调度器。
     */
    ~virtual_thread_scheduler() {
        shutdown();
    }
};

/**
 * @struct virtual_thread_awaiter
 * @brief 虚拟线程等待器
 *
 * 用于协程挂起和调度的等待器类型。
 */
struct virtual_thread_awaiter {
    coroutine_handle<> handle_;  ///< 协程句柄

    /**
     * @brief 检查是否准备就绪
     * @return 总是返回false，表示需要挂起
     */
    bool await_ready() const noexcept {
        return false;
    }

    /**
     * @brief 挂起协程
     * @param handle 协程句柄
     *
     * 将协程提交给调度器执行。
     */
    void await_suspend(coroutine_handle<> handle) {
        virtual_thread_scheduler::get_instance().schedule(handle);
    }

    /**
     * @brief 恢复协程
     */
    void await_resume() const noexcept {}
};


/**
 * @class virtual_thread
 * @brief 虚拟线程类
 *
 * 提供虚拟线程的创建、调度和管理功能。
 * 支持协作式多任务和轻量级并发。
 */
class virtual_thread {
private:
    optional<virtual_thread_task> task_;  ///< 关联的任务对象

    /**
     * @brief 创建任务
     * @tparam Func 可调用类型
     * @param func 要执行的函数
     * @return 虚拟线程任务
     *
     * 将普通函数包装为协程任务。
     */
    template<typename Func>
    static virtual_thread_task create_task(Func func) {
        co_await suspend_never{};
        func();
    }

public:
    virtual_thread() = default;  ///< 默认构造函数
    virtual_thread(const virtual_thread&) = delete;  ///< 禁止拷贝构造
    virtual_thread& operator =(const virtual_thread&) = delete;  ///< 禁止拷贝赋值
    virtual_thread(virtual_thread&& other) noexcept = default;  ///< 移动构造函数
    virtual_thread& operator =(virtual_thread&& other) noexcept = default;  ///< 移动赋值运算符

    /**
     * @brief 启动虚拟线程
     * @tparam Func 可调用类型
     * @param func 要执行的函数
     * @return 虚拟线程对象
     *
     * 创建并启动新的虚拟线程执行指定函数。
     */
    template <typename Func>
    static virtual_thread start(Func&& func) {
        virtual_thread vt;
        vt.task_ = virtual_thread::create_task(_NEFORCE forward<Func>(func));
        return vt;
    }

    /**
     * @brief 让出执行权
     * @return 等待器对象
     *
     * 挂起当前协程，将执行权交给调度器。
     */
    static virtual_thread_awaiter yield() {
        return virtual_thread_awaiter{};
    }

    /**
     * @brief 睡眠指定毫秒数
     * @param ms 毫秒数
     * @return 虚拟线程任务
     *
     * 挂起当前协程指定时间，协程式睡眠。
     */
    static virtual_thread_task sleep(const int64_t ms) {
        co_await yield();
        this_thread::sleep_for(milliseconds(ms));
    }

    /**
     * @brief 初始化调度器
     * @param num_threads 工作线程数量
     *
     * 启动指定数量的工作线程处理协程任务。
     */
    static void initialize(size_t num_threads) {
        virtual_thread_scheduler::get_instance().start_workers(num_threads);
    }

    /**
     * @brief 关闭调度器
     *
     * 关闭所有工作线程，清理资源。
     */
    static void shutdown() {
        virtual_thread_scheduler::get_instance().shutdown();
    }
};

/** @} */ // Coroutine

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_ASYNC_VIRTUAL_THREAD_HPP__
