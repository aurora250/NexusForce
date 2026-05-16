#ifndef NEFORCE_CORE_ASYNC_PACKAGED_TASK_HPP__
#define NEFORCE_CORE_ASYNC_PACKAGED_TASK_HPP__

/**
 * @file packaged_task.hpp
 * @brief NeForce 异步任务包装器
 *
 * 此文件提供了packaged_task的实现，用于将可调用对象包装为异步任务，
 * 并与future关联，便于异步执行和结果获取。
 */

#include "NeForce/core/async/promise.hpp"
#include "NeForce/core/async/thread.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Async 异步调用
 * @brief 异步调用组件
 * @{
 */

/**
 * @brief 异步任务包装类模板
 * @tparam Res 结果类型
 * @tparam Args 参数类型
 *
 * 包装可调用对象，提供异步执行能力，并将执行结果或异常传递给关联的future。
 */
template <typename Res, typename... Args>
class packaged_task<Res(Args...)> {
    using StateType = inner::__future_base::task_state_base<Res(Args...)>; ///< 状态类型

    shared_ptr<StateType> state_ptr; ///< 任务状态共享指针

public:
    /**
     * @brief 默认构造函数
     *
     * 创建空的packaged_task对象，不关联任何任务。
     */
    packaged_task() noexcept = default;

    /**
     * @brief 构造函数
     * @tparam Func 可调用类型
     * @param function 要包装的可调用对象
     *
     * 创建包装指定函数的packaged_task对象。
     * 函数签名必须与模板参数Res Args...兼容。
     */
    template <typename Func, typename = enable_if_t<!is_same_v<packaged_task, remove_cvref_t<Func>>>>
    explicit packaged_task(Func&& function) :
    state_ptr(inner::create_task_state<Res(Args...)>(_NEFORCE forward<Func>(function))) {}

    /**
     * @brief 析构函数
     *
     * 如果存在关联的future且任务未执行，会自动设置broken_promise异常。
     */
    ~packaged_task() {
        if (static_cast<bool>(state_ptr) && !state_ptr.unique()) {
            state_ptr->break_promise(_NEFORCE move(state_ptr->result_storage));
        }
    }

    packaged_task(const packaged_task&) = delete;            ///< 禁止拷贝构造
    packaged_task& operator=(const packaged_task&) = delete; ///< 禁止拷贝赋值

    /**
     * @brief 移动构造函数
     * @param other 要移动的packaged_task对象
     */
    packaged_task(packaged_task&& other) noexcept { this->swap(other); }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的packaged_task对象
     * @return 当前对象的引用
     */
    packaged_task& operator=(packaged_task&& other) noexcept {
        packaged_task(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 交换两个packaged_task对象
     * @param other 要交换的packaged_task对象
     */
    void swap(packaged_task& other) noexcept { state_ptr.swap(other.state_ptr); }

    /**
     * @brief 检查任务是否有效
     * @return 是否关联了有效的可调用对象
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return static_cast<bool>(state_ptr); }

    /**
     * @brief 获取关联的future对象
     * @return future对象
     * @throw future_exception 如果future已被获取
     */
    future<Res> get_future() { return future<Res>(state_ptr); }

    /**
     * @brief 执行任务
     * @param args 任务参数
     * @throw future_exception 如果任务无效
     *
     * 同步执行包装的任务，结果或异常会传递给关联的future。
     */
    void operator()(Args... args) {
        inner::__future_base::state_base::check(state_ptr);
        state_ptr->run(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 在线程退出时标记任务完成
     * @param args 任务参数
     * @throw future_exception 如果任务无效
     *
     * 执行任务但延迟结果设置，直到当前线程退出时才将结果传递给future。
     * 适用于需要保证某些资源在结果设置前有效的场景。
     */
    void make_ready_at_thread_exit(Args... args) {
        inner::__future_base::state_base::check(state_ptr);
        state_ptr->run_delayed(_NEFORCE forward<Args>(args)..., state_ptr);
    }

    /**
     * @brief 重置任务
     * @throw future_exception 如果任务无效
     *
     * 重置任务状态，可以重新关联新的future。
     * 原有任务函数保持不变，但会创建新的共享状态。
     */
    void reset() {
        inner::__future_base::state_base::check(state_ptr);
        packaged_task temp;
        temp.state_ptr = state_ptr;
        state_ptr = state_ptr->reset();
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Res, typename... Args>
packaged_task(Res (*)(Args...)) -> packaged_task<Res(Args...)>;

template <typename Func, typename Sign = typename inner::__function_guide_helper<decltype(&Func::operator())>::type>
packaged_task(Func) -> packaged_task<Sign>;
#endif


/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 延迟执行状态类
 * @tparam BoundFunc 绑定函数类型
 * @tparam Res 结果类型
 *
 * 用于实现延迟执行的异步任务状态管理。
 */
template <typename BoundFunc, typename Res>
class __future_base::deferred_state final : public __future_base::state_base {
private:
    using PtrType = __future_base::Ptr<basic_result<Res>>;

    PtrType result_storage;
    BoundFunc function;

    void complete_async() override { state_base::set_result(create_task_setter(result_storage, function), true); }

    NEFORCE_NODISCARD bool is_deferred_future() const override { return true; }

public:
    template <typename... Args>
    explicit deferred_state(Args&&... args) :
    result_storage(new basic_result<Res>()),
    function(_NEFORCE forward<Args>(args)...) {}
};

/**
 * @brief 异步状态基类
 *
 * 异步执行任务的公共基类，管理线程生命周期。
 */
class __future_base::async_state_common : public __future_base::state_base {
protected:
    _NEFORCE thread thread;
    once_flag flag;

public:
    ~async_state_common() override = default;

protected:
    void complete_async() override { join(); }

    void join() { _NEFORCE call_once(flag, &_NEFORCE thread::join, &thread); }
};

/**
 * @brief 异步状态实现类
 * @tparam Func 函数类型
 * @tparam Res 结果类型
 *
 * 具体实现异步任务执行的类，管理函数执行和异常处理。
 */
template <typename Func, typename Res>
class __future_base::async_state_impl final : public __future_base::async_state_common {
private:
    using PtrType = __future_base::Ptr<basic_result<Res>>;

    PtrType result_storage;
    Func function;

    void run() {
        try {
            state_base::set_result(__future_base::create_task_setter(result_storage, function));
        } catch (...) {
            if (static_cast<bool>(result_storage)) {
                state_base::break_promise(_NEFORCE move(result_storage));
            }
            throw;
        }
    }

public:
    template <typename... Args>
    explicit async_state_impl(Args&&... args) :
    result_storage(new basic_result<Res>()),
    function(_NEFORCE forward<Args>(args)...) {
        thread = _NEFORCE thread{&async_state_impl::run, this};
    }

    ~async_state_impl() override {
        if (thread.joinable()) {
            thread.join();
        }
    }
};

NEFORCE_END_INNER__
/// @endcond

/** @} */ // Async

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_PACKAGED_TASK_HPP__
