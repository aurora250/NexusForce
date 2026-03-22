#ifndef NEFORCE_CORE_ASYNC_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_HPP__

/**
 * @file thread.hpp
 * @brief 线程支持
 *
 * 此文件提供了线程类和相关操作。
 */

#include "NeForce/core/functional/apply.hpp"
#include "NeForce/core/exception/terminate.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/async/this_thread.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct thread_exception
 * @extends system_exception
 * @brief 线程操作异常
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(thread_exception, system_exception, "Thread Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @class thread
 * @brief 线程类
 */
class NEFORCE_API thread {
public:
    /**
     * @struct id
     * @brief 线程唯一标识符类
     */
    struct id : ihashable<id> {
    private:
        /**
         * @brief 系统线程标识符类型
         */
        using native_id_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
            ::DWORD;
#else
            ::pthread_t;
#endif

        native_id_type id_{};  ///< 系统线程标识符

        friend class thread;

    public:
        /**
         * @brief 默认构造函数
         */
        id() noexcept = default;

        /**
         * @brief 从原生ID构造
         * @param id 原生线程ID
         */
        explicit id(const native_id_type id) noexcept : id_(id) {}

        /**
         * @brief 获取原生线程ID
         * @return 原生线程ID
         */
        NEFORCE_NODISCARD native_id_type native_handle() const noexcept { return id_; }

        /**
         * @brief 计算哈希值
         * @return 哈希值
         */
        NEFORCE_NODISCARD size_t to_hash() const noexcept {
            return _NEFORCE FNV_hash(reinterpret_cast<const byte_t*>(&id_), sizeof(id));
        }

        /**
         * @brief 等于比较运算符
         * @param rhs 右操作数
         * @return 两个线程ID是否相等
         */
        NEFORCE_NODISCARD bool operator ==(const id& rhs) const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
            return id_ == rhs.id_;
#else
            return ::pthread_equal(id_, rhs.id_) != 0;
#endif
        }

        /**
         * @brief 不等于比较运算符
         * @param rhs 右操作数
         * @return 两个线程ID是否不相等
         */
        NEFORCE_NODISCARD bool operator !=(const id& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

private:
    /**
     * @enum state
     * @brief 线程状态枚举
     */
    enum state {
        NOT_A_THREAD, ///< 不是线程
        CREATED,      ///< 已创建
        JOINED,       ///< 已被等待
        DETACHED      ///< 已分离
    };

    /**
     * @struct data_base
     * @brief 线程数据基类
     *
     * 抽象基类，用于存储线程执行所需的函数和数据。
     */
    struct data_base {
        virtual ~data_base() = default;
        virtual void run() = 0;
    };

    /**
     * @struct thread_data
     * @brief 线程数据具体实现
     * @tparam Callable 可调用对象类型
     */
    template <typename Callable>
    struct thread_data final : data_base {
        Callable func_;  ///< 要执行的可调用对象

        /**
         * @brief 构造函数
         * @tparam F 可调用对象类型
         * @param f 可调用对象
         */
        template <typename F>
        explicit thread_data(F&& f) : func_(_NEFORCE forward<F>(f)) {}

        /**
         * @brief 执行可调用对象
         */
        void run() override { func_(); }
    };

public:
    /**
     * @brief 系统线程句柄类型
     */
    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::HANDLE;
#else
        ::pthread_t;
#endif

private:
    native_handle_type handle_{}; ///< 线程句柄
    id id_{};                     ///< 线程ID
    state state_ = NOT_A_THREAD;  ///< 线程状态

#ifdef NEFORCE_PLATFORM_WINDOWS
    static unsigned int __stdcall
#else
    static void*
#endif
    thread_entry(void* arg) {
        const unique_ptr<data_base> data(static_cast<data_base*>(arg));
        try {
            data->run();
        } catch (...) {
            terminate();
        }
#ifdef NEFORCE_PLATFORM_WINDOWS
        return 0;
#else
        return nullptr;
#endif
    }

    /**
     * @brief 启动线程实现
     * @param args 线程数据指针
     * @throw thread_exception 如果线程创建失败
     * @note 线程的执行目标报错将导致进程终止
     */
    void start_thread_impl(void* args);

    /**
     * @brief 启动线程
     * @tparam F 可调用对象类型
     * @param f 要执行的可调用对象
     * @throw thread_exception 如果线程创建失败
     * @note 线程的执行目标报错将导致进程终止
     */
    template <typename F>
    void start_thread(F&& f) {
        auto data = _NEFORCE make_unique<thread_data<decay_t<F>>>(_NEFORCE forward<F>(f));
        this->start_thread_impl(data.get());
        data.release();
        state_ = CREATED;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个不表示任何线程的thread对象。
     */
    thread() noexcept = default;

    /**
     * @brief 从可调用对象构造线程
     * @tparam F 可调用对象类型
     * @tparam Args 参数类型
     * @param f 要执行的可调用对象
     * @param args 传递给可调用对象的参数
     * @throw thread_exception 如果线程创建失败
     *
     * 创建一个新线程，并在线程中执行带参数的可调用对象。
     *
     * @note 线程的执行目标报错将导致进程终止
     */
    template <typename F, typename... Args, typename = enable_if_t<!is_same_v<decay_t<F>, thread>>>
    explicit thread(F&& f, Args&&... args) {
        auto func = [func = _NEFORCE move(f), args = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...)]() mutable {
            return _NEFORCE apply(_NEFORCE move(func), _NEFORCE move(args));
        };
        thread::start_thread(_NEFORCE move(func));
    }

    /**
     * @note 禁止复制构造
     */
    thread(const thread&) = delete;

    /**
     * @note 禁止复制赋值
     */
    thread& operator =(const thread&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的线程对象
     */
    thread(thread&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的线程对象
     * @return 当前对象的引用
     */
    thread& operator =(thread&& other) noexcept;

    /**
     * @brief 析构函数
     * @note 如果线程处于可被等待状态则终止进程。
     */
    ~thread();

    /**
     * @brief 获取线程标识符
     * @return 线程标识符
     */
    NEFORCE_NODISCARD id get_id() const noexcept {
        return id_;
    }

    /**
     * @brief 获取原生句柄
     * @return 平台特定的线程句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept {
        return handle_;
    }

    /**
     * @brief 检查线程是否可被等待
     * @return 线程是否处于可被等待状态
     *
     * 线程在创建后、被等待结束或分离前是可被等待的。
     */
    NEFORCE_NODISCARD bool joinable() const noexcept {
        return state_ == CREATED;
    }

    /**
     * @brief 等待线程结束
     * @throw thread_exception 如果线程不可被等待或等待失败
     *
     * 阻塞当前线程，直到目标线程执行完毕。
     */
    void join();

    /**
     * @brief 分离线程
     * @throw thread_exception 如果线程不可被等待或分离失败
     *
     * 使线程在后台独立运行，线程结束后自动释放资源。
     */
    void detach();

    /**
     * @brief 交换两个线程对象
     * @param other 要交换的线程对象
     */
    void swap(thread& other) noexcept {
        _NEFORCE swap(handle_, other.handle_);
        _NEFORCE swap(id_, other.id_);
        _NEFORCE swap(state_, other.state_);
    }
};

/** @} */ // Thread

NEFORCE_BEGIN_THIS_THREAD__

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @brief 获取当前线程标识符
 * @return 当前线程的标识符
 */
NEFORCE_ALWAYS_INLINE_INLINE thread::id id() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return thread::id(::GetCurrentThreadId());
#else
    return thread::id(::pthread_self());
#endif
}

/** @} */ // Thread

NEFORCE_END_THIS_THREAD__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_HPP__
