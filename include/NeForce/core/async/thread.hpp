#ifndef NEFORCE_CORE_ASYNC_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_HPP__

/**
 * @file thread.hpp
 * @brief 线程管理类
 *
 * 此文件提供了跨平台的线程管理功能，
 * 包括线程创建、等待、分离、线程标识符、线程名称设置等操作。
 */

#include "NeForce/core/async/this_thread.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/functional/apply.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct thread_exception
 * @brief 线程操作异常
 */
struct thread_exception final : system_exception {
    explicit thread_exception(const char* info = "Thread Operation Failed.", const char* type = static_type,
                              const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit thread_exception(const exception& e) :
    system_exception(e) {}

    ~thread_exception() override = default;

    static constexpr auto static_type = "thread_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @class thread
 * @brief 线程类
 *
 * 提供跨平台的线程管理功能，支持线程创建、等待、分离、ID获取等操作。
 * 线程对象可移动但不可复制。
 */
class NEFORCE_API thread {
public:
    /**
     * @struct id
     * @brief 线程唯一标识符类
     *
     * 用于标识线程，支持哈希和比较操作。
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

        native_id_type id_{}; ///< 系统线程标识符

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
        explicit id(const native_id_type id) noexcept :
        id_(id) {}

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
        NEFORCE_NODISCARD bool operator==(const id& rhs) const noexcept {
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
        NEFORCE_NODISCARD bool operator!=(const id& rhs) const noexcept { return !(*this == rhs); }
    };

    /**
     * @struct hook
     * @brief 线程生命周期钩子
     *
     * 提供线程生命周期事件的回调机制，用于监控线程的创建和销毁。
     */
    struct NEFORCE_API hook {
        /**
         * @enum point
         * @brief 钩子触发点枚举
         */
        enum class point {
            before_create, ///< 线程创建前
            after_create,  ///< 线程创建后
            thread_start,  ///< 线程函数开始执行
            thread_end,    ///< 线程函数结束
            before_destroy ///< 线程对象销毁前
        };

        using callback_t = void (*)(point point, id thread_id); ///< 钩子回调函数类型

        /**
         * @brief 添加钩子回调
         * @param hook 回调函数指针
         */
        static void add_hook(callback_t hook);

        /**
         * @brief 移除钩子回调
         * @param hook 要移除的回调函数指针
         */
        static void remove_hook(callback_t hook);

        /**
         * @brief 调用钩子回调
         * @param point 触发点
         * @param thread_id 线程ID
         */
        static void invoke(point point, id thread_id);
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
        Callable func_;

        template <typename F>
        explicit thread_data(F&& f) :
        func_(_NEFORCE forward<F>(f)) {}

        void run() override { func_(); }
    };

    /**
     * @struct thread_startup_args
     * @brief 线程启动参数
     *
     * 传递给线程入口函数的参数结构。
     */
    struct thread_startup_args {
        unique_ptr<data_base> data; ///< 线程执行数据
        id thread_id;               ///< 线程ID
    };

    /**
     * @struct thread_monitor
     * @brief 线程监控器
     *
     * 在线程函数开始和结束时自动调用钩子并更新跟踪计数。
     */
    struct NEFORCE_API thread_monitor {
    private:
        id thread_id_; ///< 线程ID

    public:
        /**
         * @brief 构造函数
         * @param thread_id 线程ID
         */
        explicit thread_monitor(id thread_id);

        /**
         * @brief 析构函数
         */
        ~thread_monitor() noexcept;
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
    static unsigned int __stdcall thread_entry(void* arg);
#else
    static void* thread_entry(void* arg);
#endif

    void start_thread_impl(thread_startup_args* args);

    template <typename F>
    void start_thread(F&& f) {
        auto data = _NEFORCE make_unique<thread_data<decay_t<F>>>(_NEFORCE forward<F>(f));
        this->start_thread_impl(new thread_startup_args{_NEFORCE move(data), id_});
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

    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

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
    thread& operator=(thread&& other) noexcept;

    /**
     * @brief 析构函数
     * @note 如果线程处于可被等待状态则终止进程。
     */
    ~thread();

    /**
     * @brief 获取线程标识符
     * @return 线程标识符
     */
    NEFORCE_NODISCARD id get_id() const noexcept { return id_; }

    /**
     * @brief 获取原生句柄
     * @return 平台特定的线程句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept { return handle_; }

    /**
     * @brief 检查线程是否可被等待
     * @return 线程是否处于可被等待状态
     *
     * 线程在创建后、被等待结束或分离前是可被等待的。
     */
    NEFORCE_NODISCARD bool joinable() const noexcept { return state_ == CREATED; }

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
     * @brief 设置线程名称
     * @param name 线程名称
     * @return 是否设置成功
     * @throw thread_exception 如果线程不可被等待
     */
    bool set_name(const char* name);

    /**
     * @brief 获取线程名称
     * @param buffer 存储名称的缓冲区
     * @param size 缓冲区大小
     * @return 是否获取成功
     */
    bool name(char* buffer, size_t size) const;

    /**
     * @brief 交换两个线程对象
     * @param other 要交换的线程对象
     */
    void swap(thread& other) noexcept;

    /**
     * @brief 设置指定线程的名称
     * @param handle 线程句柄
     * @param name 线程名称
     * @return 是否设置成功
     */
    static bool set_name(native_handle_type handle, const char* name);

    /**
     * @brief 获取指定线程的名称
     * @param handle 线程句柄
     * @param buffer 存储名称的缓冲区
     * @param size 缓冲区大小
     * @return 是否获取成功
     */
    static bool name(native_handle_type handle, char* buffer, size_t size);
};

/** @} */ // Thread

/** @} */ // AsyncComponents

NEFORCE_BEGIN_THIS_THREAD__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

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

/**
 * @brief 获取当前线程句柄
 * @return 当前线程的句柄
 */
NEFORCE_ALWAYS_INLINE_INLINE thread::native_handle_type handle() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetCurrentThread();
#else
    return ::pthread_self();
#endif
}

/**
 * @brief 获取当前线程名称
 * @param buffer 存储名称的缓冲区
 * @param size 缓冲区大小
 * @return 是否获取成功
 */
NEFORCE_ALWAYS_INLINE_INLINE bool name(char* buffer, size_t size) {
    return thread::name(this_thread::handle(), buffer, size);
}

/**
 * @brief 设置当前线程名称
 * @param name 线程名称
 * @return 是否设置成功
 */
NEFORCE_ALWAYS_INLINE_INLINE bool set_name(const char* name) { return thread::set_name(this_thread::handle(), name); }

/** @} */ // Thread

/** @} */ // AsyncComponents

NEFORCE_END_THIS_THREAD__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_HPP__
