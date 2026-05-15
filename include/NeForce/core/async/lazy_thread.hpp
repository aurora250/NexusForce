#ifndef NEFORCE_CORE_ASYNC_LAZY_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_LAZY_THREAD_HPP__

/**
 * @file lazy_thread.hpp
 * @brief 延迟启动线程实现
 *
 * 此文件提供了延迟启动线程的实现。
 */

#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
 * @class lazy_thread
 * @brief 延迟启动线程类
 *
 * 允许创建线程对象时仅存储可调用对象，而不立即创建和执行线程。
 * 线程的实际创建由start()方法触发。
 *
 * 使用场景：
 * - 需要将线程作为对象存储，但执行时机未定
 * - 需要在不同作用域间传递线程任务
 * - 需要在某些条件满足后才启动线程
 * - 避免不必要的线程创建开销
 *
 * @note lazy_thread在析构时会自动join而不是terminate。
 */
class NEFORCE_API lazy_thread {
public:
    using id = thread::id;                                 ///< 线程ID类型
    using native_handle_type = thread::native_handle_type; ///< 原生句柄类型

private:
    function<void()> func_; ///< 存储的可调用对象
    thread thread_;         ///< 实际线程对象

public:
    /**
     * @brief 默认构造函数
     */
    lazy_thread() noexcept = default;

    /**
     * @brief 构造函数
     * @tparam F 可调用对象类型
     * @tparam Args 参数类型
     * @param f 要执行的可调用对象
     * @param args 传递给可调用对象的参数
     *
     * 创建延迟线程对象，存储可调用对象和参数，但不立即启动线程。
     */
    template <typename F, typename... Args>
    explicit lazy_thread(F&& f, Args&&... args) {
        func_ = [func = _NEFORCE forward<F>(f), args = _NEFORCE make_tuple(_NEFORCE forward<Args>(args)...)]() mutable {
            _NEFORCE apply(_NEFORCE move(func), _NEFORCE move(args));
        };
    }

    lazy_thread(const lazy_thread&) = delete;
    lazy_thread& operator=(const lazy_thread&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的延迟线程对象
     *
     * 转移任务和线程的所有权，other变为空状态。
     */
    lazy_thread(lazy_thread&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的延迟线程对象
     * @return 自身引用
     *
     * 如果当前对象已有线程且可连接，会等待其完成后再转移所有权。
     */
    lazy_thread& operator=(lazy_thread&& other) noexcept;

    /**
     * @brief 析构函数
     *
     * 如果线程已启动且仍可连接，会等待其完成。
     */
    ~lazy_thread();

    /**
     * @brief 启动线程
     * @throws thread_exception 没有可调用对象或线程已启动时抛出
     *
     * 创建实际线程并执行存储的任务。
     * 只能调用一次，调用后线程立即开始执行。
     */
    void start();

    /**
     * @brief 检查线程是否可被等待
     * @return 线程已启动且尚未被等待或分离时返回true
     */
    NEFORCE_NODISCARD bool joinable() const noexcept { return thread_.joinable(); }

    /**
     * @brief 等待线程结束
     * @throw thread_exception 如果线程不可被等待或等待失败
     *
     * 阻塞当前线程，直到目标线程执行完毕。
     */
    void join() { thread_.join(); }

    /**
     * @brief 分离线程
     * @throw thread_exception 如果线程不可被等待或分离失败
     *
     * 使线程在后台独立运行，线程结束后自动释放资源。
     */
    void detach() { thread_.detach(); }

    /**
     * @brief 获取线程标识符
     * @return 线程标识符
     */
    NEFORCE_NODISCARD thread::id get_id() const noexcept { return thread_.get_id(); }

    /**
     * @brief 交换两个延迟线程对象
     * @param other 要交换的对象
     */
    void swap(lazy_thread& other) noexcept {
        _NEFORCE swap(func_, other.func_);
        _NEFORCE swap(thread_, other.thread_);
    }
};

/** @} */ // Thread

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_LAZY_THREAD_HPP__
