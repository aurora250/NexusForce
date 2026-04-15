#ifndef NEFORCE_CORE_ASYNC_SCOPED_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_SCOPED_THREAD_HPP__

/**
 * @file scoped_thread.hpp
 * @brief 作用域线程
 *
 * 此文件提供了作用域线程的实现，支持自动停止和清理。
 */

#include "NeForce/core/async/stop_token.hpp"
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
 * @class scoped_thread
 * @brief 作用域线程类
 *
 * 提供自动停止和清理功能，在析构时会自动请求停止并等待线程结束，避免资源泄漏。
 *
 * @note 支持stop_token参数自动传递
 */
class scoped_thread {
public:
    using id = thread::id;                                 ///< 线程ID类型
    using native_handle_type = thread::native_handle_type; ///< 原生句柄类型

private:
    template <typename Callable, typename Object, typename... Args>
    static constexpr bool pmf_expects_stop_token =
            conjunction_v<is_member_function_pointer<remove_reference_t<Callable>>,
                          is_invocable<Callable, Object, stop_token, Args...>>;

    stop_source stop_source_{none};
    thread thread_{};

    template <typename Callable, typename Object, typename... Args,
              enable_if_t<pmf_expects_stop_token<Callable, Args...>, int> = 0>
    static thread create(stop_source& source, Callable func, Object&& object, Args&&... args) {
        return thread{func, _NEFORCE forward<Object>(object), source.get_token(), _NEFORCE forward<Args>(args)...};
    }

    template <typename Callable, typename... Args,
              enable_if_t<!pmf_expects_stop_token<Callable, Args...> &&
                                  is_invocable_v<decay_t<Callable>, stop_token, decay_t<Args>...>,
                          int> = 0>
    static thread create(stop_source& source, Callable func, Args&&... args) {
        return thread{_NEFORCE forward<Callable>(func), source.get_token(), _NEFORCE forward<Args>(args)...};
    }

    template <typename Callable, typename... Args,
              enable_if_t<!pmf_expects_stop_token<Callable, Args...> &&
                                  !is_invocable_v<decay_t<Callable>, stop_token, decay_t<Args>...>,
                          int> = 0>
    static thread create(stop_source&, Callable func, Args&&... args) {
        static_assert(is_invocable_v<decay_t<Callable>, decay_t<Args>...>,
                      "jthread arguments must be invocable after conversion to rvalues");
        return thread{_NEFORCE forward<Callable>(func), _NEFORCE forward<Args>(args)...};
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个不表示任何线程的scoped_thread对象。
     */
    scoped_thread() noexcept = default;

    /**
     * @brief 构造函数
     * @tparam Callable 可调用类型
     * @tparam Args 参数类型
     * @param func 要在线程中执行的函数
     * @param args 传递给函数的参数
     *
     * 创建新线程并开始执行。如果函数接受stop_token参数，会自动传递。
     */
    template <typename Callable, typename... Args,
              typename = enable_if_t<!is_same_v<remove_cvref_t<Callable>, scoped_thread>>>
    explicit scoped_thread(Callable&& func, Args&&... args) :
    thread_{this->create(stop_source_, _NEFORCE forward<Callable>(func), _NEFORCE forward<Args>(args)...)} {}

    scoped_thread(const scoped_thread&) = delete;            ///< 禁止拷贝构造
    scoped_thread& operator=(const scoped_thread&) = delete; ///< 禁止拷贝赋值

    /**
     * @brief 移动构造函数
     * @param other 要移动的scoped_thread
     */
    scoped_thread(scoped_thread&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的scoped_thread
     * @return 当前对象的引用
     */
    scoped_thread& operator=(scoped_thread&& other) noexcept {
        scoped_thread(move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 如果线程可被等待结束，自动请求停止并等待线程结束。
     */
    ~scoped_thread() {
        if (joinable()) {
            request_stop();
            join();
        }
    }

    /**
     * @brief 交换两个scoped_thread对象
     * @param other 要交换的scoped_thread
     */
    void swap(scoped_thread& other) noexcept {
        _NEFORCE swap(stop_source_, other.stop_source_);
        _NEFORCE swap(thread_, other.thread_);
    }

    /**
     * @brief 检查线程是否可被等待
     * @return 是否可被等待
     */
    NEFORCE_NODISCARD bool joinable() const noexcept { return thread_.joinable(); }

    /**
     * @brief 等待线程结束
     *
     * 阻塞当前线程，直到目标线程执行完成。
     */
    void join() { thread_.join(); }

    /**
     * @brief 分离线程
     *
     * 允许线程独立执行，不再与其关联。
     * 分离后不能再加入或请求停止。
     */
    void detach() { thread_.detach(); }

    /**
     * @brief 获取线程ID
     * @return 线程ID
     */
    NEFORCE_NODISCARD id get_id() const noexcept { return thread_.get_id(); }

    /**
     * @brief 获取原生线程句柄
     * @return 原生线程句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const { return thread_.native_handle(); }

    /**
     * @brief 获取停止源
     * @return 关联的stop_source
     */
    NEFORCE_NODISCARD stop_source get_stop_source() noexcept { return stop_source_; }

    /**
     * @brief 获取停止令牌
     * @return 关联的stop_token
     */
    NEFORCE_NODISCARD stop_token get_stop_token() const noexcept { return stop_source_.get_token(); }

    /**
     * @brief 请求线程停止
     * @return 是否成功请求停止
     *
     * 设置停止标志，线程可以通过检查stop_token来响应停止请求。
     */
    bool request_stop() noexcept { return stop_source_.request_stop(); }
};

/** @} */ // Thread

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_SCOPED_THREAD_HPP__
