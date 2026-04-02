#ifndef NEFORCE_CORE_ASYNC_PROMISE_HPP__
#define NEFORCE_CORE_ASYNC_PROMISE_HPP__

/**
 * @file promise.hpp
 * @brief 异步结果生产者
 *
 * 此文件提供了promise的实现作为结果的生产者。
 */

#include "NeForce/core/async/future.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Async 异步行为
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @class promise
 * @brief Promise类模板
 * @tparam Res 结果类型
 *
 * 表示一个异步计算的结果提供者，与future配对使用。
 * 可以将计算结果或异常设置到promise中，关联的future可以获取这些结果。
 *
 * @note 结果类型不能是数组、函数，必须是可析构类型
 */
template <typename Res> class promise {
    static_assert(!is_array_v<Res>, "result type must not be an array");
    static_assert(!is_function_v<Res>, "result type must not be a function");
    static_assert(is_destructible_v<Res>, "result type must be destructible");

public:
    using state_type = inner::__future_base::state_base;         ///< 状态类型
    using result_type = inner::__future_base::basic_result<Res>; ///< 结果类型
    using ptr_type = inner::__future_base::Ptr<result_type>;     ///< 结果指针类型

private:
    shared_ptr<state_type> future_ptr; ///< 共享状态指针
    ptr_type storage;                  ///< 结果存储

    template <typename T, typename U> friend struct inner::__future_base::state_base::setter;

    /**
     * @brief 获取内部状态引用
     * @return 状态引用
     * @throw future_exception 如果状态无效
     */
    NEFORCE_NODISCARD state_type& state() const {
        inner::__future_base::state_base::check(future_ptr);
        return *future_ptr;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 创建新的promise对象，初始化共享状态和结果存储。
     */
    promise() :
    future_ptr(_NEFORCE make_shared<state_type>()),
    storage(new result_type()) {}

    /**
     * @brief 移动构造函数
     * @param other 要移动的promise对象
     */
    promise(promise&& other) noexcept :
    future_ptr(_NEFORCE move(other.future_ptr)),
    storage(_NEFORCE move(other.storage)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的promise对象
     * @return 当前对象的引用
     */
    promise& operator=(promise&& other) noexcept {
        promise(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    promise(const promise&) = delete;            ///< 禁止拷贝构造
    promise& operator=(const promise&) = delete; ///< 禁止拷贝赋值

    /**
     * @brief 析构函数
     *
     * 如果存在关联的future且promise未被设置结果，会自动设置broken_promise异常。
     */
    ~promise() {
        if (static_cast<bool>(future_ptr) && !future_ptr.unique()) {
            future_ptr->break_promise(_NEFORCE move(storage));
        }
    }

    /**
     * @brief 交换两个promise对象
     * @param other 要交换的promise对象
     */
    void swap(promise& other) noexcept {
        future_ptr.swap(other.future_ptr);
        storage.swap(other.storage);
    }

    /**
     * @brief 获取关联的future对象
     * @return future对象
     * @throw future_exception 如果future已被获取
     */
    future<Res> get_future() { return future<Res>(future_ptr); }

    /**
     * @brief 设置结果值
     * @param value 要设置的结果值
     * @throw future_exception 如果结果已被设置
     */
    void set_value(Res&& value) { state().set_result(state_type::create_setter(this, _NEFORCE forward<Res>(value))); }

    /**
     * @brief 设置异常
     * @param exception 异常指针
     * @throw future_exception 如果结果已被设置
     */
    void set_exception(exception_ptr exception) { state().set_result(state_type::create_setter(exception, this)); }

    /**
     * @brief 在线程退出时设置结果值
     * @param value 要设置的结果值
     * @throw future_exception 如果结果已被设置
     * @note 结果会在当前线程退出时设置，适用于需要保证某些资源在结果设置前有效的场景
     */
    void set_value_at_thread_exit(Res&& value) {
        state().set_delayed_result(state_type::create_setter(this, _NEFORCE forward<Res>(value)), future_ptr);
    }

    /**
     * @brief 在线程退出时设置异常
     * @param exception 异常指针
     * @throw future_exception 如果结果已被设置
     */
    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }
};

/**
 * @brief 引用类型的promise特化
 * @tparam Res 引用类型
 */
template <typename Res> class promise<Res&> {
public:
    using state_type = inner::__future_base::state_base;          ///< 状态类型
    using result_type = inner::__future_base::basic_result<Res&>; ///< 结果类型
    using ptr_type = inner::__future_base::Ptr<result_type>;      ///< 结果指针类型

private:
    shared_ptr<state_type> future_ptr; ///< 共享状态指针
    ptr_type storage;                  ///< 结果存储

    template <typename T, typename U> friend struct inner::__future_base::state_base::setter;

    NEFORCE_NODISCARD state_type& state() const {
        inner::__future_base::state_base::check(future_ptr);
        return *future_ptr;
    }

public:
    /**
     * @brief 默认构造函数
     */
    promise() :
    future_ptr(_NEFORCE make_shared<state_type>()),
    storage(new result_type()) {}

    /**
     * @brief 移动构造函数
     */
    promise(promise&& other) noexcept :
    future_ptr(_NEFORCE move(other.future_ptr)),
    storage(_NEFORCE move(other.storage)) {}

    /**
     * @brief 移动赋值运算符
     */
    promise& operator=(promise&& other) noexcept {
        promise(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    promise(const promise&) = delete;            ///< 禁止拷贝构造
    promise& operator=(const promise&) = delete; ///< 禁止拷贝赋值

    /**
     * @brief 析构函数
     */
    ~promise() {
        if (static_cast<bool>(future_ptr) && !future_ptr.unique()) {
            future_ptr->break_promise(_NEFORCE move(storage));
        }
    }

    /**
     * @brief 交换两个promise对象
     */
    void swap(promise& other) noexcept {
        future_ptr.swap(other.future_ptr);
        storage.swap(other.storage);
    }

    /**
     * @brief 获取关联的future对象
     * @return future对象
     */
    future<Res&> get_future() { return future<Res&>(future_ptr); }

    /**
     * @brief 设置结果引用
     * @param value 要引用的对象
     * @throw future_exception 如果结果已被设置
     */
    void set_value(Res& value) { state().set_result(state_type::create_setter(this, value)); }

    /**
     * @brief 设置异常
     * @param exception 异常指针
     * @throw future_exception 如果结果已被设置
     */
    void set_exception(exception_ptr exception) { state().set_result(state_type::create_setter(exception, this)); }

    /**
     * @brief 在线程退出时设置结果引用
     * @param value 要引用的对象
     * @throw future_exception 如果结果已被设置
     */
    void set_value_at_thread_exit(Res& value) {
        state().set_delayed_result(state_type::create_setter(this, value), future_ptr);
    }

    /**
     * @brief 在线程退出时设置异常
     * @param exception 异常指针
     * @throw future_exception 如果结果已被设置
     */
    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }
};

/**
 * @brief void类型的promise特化
 */
template <> class promise<void> {
public:
    using state_type = inner::__future_base::state_base;          ///< 状态类型
    using result_type = inner::__future_base::basic_result<void>; ///< 结果类型
    using ptr_type = inner::__future_base::Ptr<result_type>;      ///< 结果指针类型

private:
    shared_ptr<state_type> future_ptr; ///< 共享状态指针
    ptr_type storage;                  ///< 结果存储

    template <typename T, typename U> friend struct inner::__future_base::state_base::setter;

    NEFORCE_NODISCARD state_type& state() const {
        inner::__future_base::state_base::check(future_ptr);
        return *future_ptr;
    }

public:
    /**
     * @brief 默认构造函数
     */
    promise() :
    future_ptr(_NEFORCE make_shared<state_type>()),
    storage(new result_type()) {}

    /**
     * @brief 移动构造函数
     */
    promise(promise&& other) noexcept :
    future_ptr(_NEFORCE move(other.future_ptr)),
    storage(_NEFORCE move(other.storage)) {}

    /**
     * @brief 移动赋值运算符
     */
    promise& operator=(promise&& other) noexcept {
        promise(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    promise(const promise&) = delete;            ///< 禁止拷贝构造
    promise& operator=(const promise&) = delete; ///< 禁止拷贝赋值

    /**
     * @brief 析构函数
     */
    ~promise() {
        if (static_cast<bool>(future_ptr) && !future_ptr.unique()) {
            future_ptr->break_promise(_NEFORCE move(storage));
        }
    }

    /**
     * @brief 交换两个promise对象
     */
    void swap(promise& other) noexcept {
        future_ptr.swap(other.future_ptr);
        storage.swap(other.storage);
    }

    /**
     * @brief 获取关联的future对象
     * @return future对象
     */
    NEFORCE_NODISCARD future<void> get_future() const { return future<void>(future_ptr); }

    /**
     * @brief 设置void结果
     * @throw future_exception 如果结果已被设置
     */
    void set_value() { state().set_result(state_type::create_setter(this)); }

    /**
     * @brief 设置异常
     * @param exception 异常指针
     * @throw future_exception 如果结果已被设置
     */
    void set_exception(exception_ptr exception) { state().set_result(state_type::create_setter(exception, this)); }

    /**
     * @brief 在线程退出时设置void结果
     * @throw future_exception 如果结果已被设置
     */
    void set_value_at_thread_exit() { state().set_delayed_result(state_type::create_setter(this), future_ptr); }

    /**
     * @brief 在线程退出时设置异常
     * @param exception 异常指针
     * @throw future_exception 如果结果已被设置
     */
    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 任务设置器
 * @tparam PtrT 结果指针类型
 * @tparam Func 可调用类型
 *
 * 用于执行任务并捕获异常，将结果设置到promise中。
 */
template <typename PtrT, typename Func, typename> struct __future_base::task_setter {
    PtrT* result_ptr;   ///< 结果指针
    Func* function_ptr; ///< 可调用对象指针

    /**
     * @brief 执行设置操作
     * @return 结果指针
     * @note 自动捕获异常并存储到结果中
     */
    PtrT operator()() const noexcept {
        try {
            (*result_ptr)->set((*function_ptr)());
        } catch (...) {
            (*result_ptr)->error_ptr = _NEFORCE current_exception();
        }
        return _NEFORCE move(*result_ptr);
    }
};

/**
 * @brief 任务设置器（void结果）
 * @tparam PtrT 结果指针类型
 * @tparam Func 可调用类型
 */
template <typename PtrT, typename Func> struct __future_base::task_setter<PtrT, Func, void> {
    PtrT* result_ptr;   ///< 结果指针
    Func* function_ptr; ///< 可调用对象指针

    PtrT operator()() const noexcept {
        try {
            (*function_ptr)();
        } catch (...) {
            (*result_ptr)->error_ptr = current_exception();
        }
        return _NEFORCE move(*result_ptr);
    }
};

/**
 * @brief 任务状态基类
 * @tparam Res 结果类型
 * @tparam Args 参数类型
 *
 * 用于packaged_task的任务执行状态管理。
 */
template <typename Res, typename... Args>
class __future_base::task_state_base<Res(Args...)> : public __future_base::state_base {
public:
    using result_type = Res;                               ///< 结果类型
    using PtrType = __future_base::Ptr<basic_result<Res>>; ///< 结果指针类型

    PtrType result_storage; ///< 结果存储

    /**
     * @brief 构造函数
     * @tparam Alloc 分配器类型
     * @param alloc 分配器实例
     */
    template <typename Alloc>
    task_state_base(const Alloc& alloc) :
    result_storage(allocate_result<Res>(alloc)) {}

    /**
     * @brief 执行任务
     * @param args 任务参数
     * @note 纯虚函数，由具体实现类完成
     */
    virtual void run(Args&&... args) = 0;

    /**
     * @brief 延迟执行任务
     * @param args 任务参数
     * @param self 自身弱引用
     * @note 纯虚函数，由具体实现类完成
     */
    virtual void run_delayed(Args&&... args, weak_ptr<state_base> self) = 0;

    /**
     * @brief 重置任务状态
     * @return 新的任务状态共享指针
     * @note 纯虚函数，用于packaged_task的重置操作
     */
    virtual shared_ptr<task_state_base> reset() = 0;
};

/**
 * @brief 具体任务状态实现类
 * @tparam Func 可调用类型
 * @tparam Alloc 分配器类型
 * @tparam Res 结果类型
 * @tparam Args 参数类型
 *
 * 管理具体的任务函数和分配器。
 */
template <typename Func, typename Alloc, typename Res, typename... Args>
class __future_base::task_state<Func, Alloc, Res(Args...)> final : public __future_base::task_state_base<Res(Args...)> {
public:
    /**
     * @brief 构造函数
     * @tparam Func2 可调用类型
     * @param func 要包装的函数
     * @param alloc 分配器实例
     */
    template <typename Func2>
    task_state(Func2&& func, const Alloc& alloc) :
    task_state_base<Res(Args...)>(alloc),
    impl(_NEFORCE forward<Func2>(func), alloc) {}

private:
    void run(Args&&... args) override {
        auto bound_func = [&]() -> Res {
            return _NEFORCE invoke_r<Res>(impl.function_ptr, _NEFORCE forward<Args>(args)...);
        };
        state_base::set_result(__future_base::create_task_setter(this->result_storage, bound_func));
    }

    void run_delayed(Args&&... args, weak_ptr<state_base> self) override {
        auto bound_function = [&]() -> Res {
            return _NEFORCE invoke_r<Res>(impl.function_ptr, _NEFORCE forward<Args>(args)...);
        };
        state_base::set_delayed_result(__future_base::create_task_setter(this->result_storage, bound_function),
                                       _NEFORCE move(self));
    }

    shared_ptr<task_state_base<Res(Args...)>> reset() override;

    /**
     * @brief 实现结构体
     *
     * 组合分配器和函数对象。
     */
    struct Impl : Alloc {
        Func function_ptr;

        template <typename Func2>
        Impl(Func2&& func, const Alloc& alloc) :
        Alloc(alloc),
        function_ptr(_NEFORCE forward<Func2>(func)) {}
    } impl;
};

/**
 * @brief 创建任务状态
 * @tparam Sign 函数签名类型
 * @tparam Func 可调用类型
 * @tparam Alloc 分配器类型
 * @param func 要包装的函数
 * @param alloc 分配器实例
 * @return 任务状态共享指针
 */
template <typename Sign, typename Func, typename Alloc = _NEFORCE allocator<int>>
static shared_ptr<__future_base::task_state_base<Sign>> create_task_state(Func&& func, const Alloc& alloc = Alloc()) {
    using State = __future_base::task_state<decay_t<Func>, Alloc, Sign>;
    return _NEFORCE allocate_shared<State>(alloc, _NEFORCE forward<Func>(func), alloc);
}

/**
 * @brief 重置任务状态
 * @return 新的任务状态共享指针
 *
 * 创建新的任务状态，移动原有函数对象。
 */
template <typename Func, typename Alloc, typename Res, typename... Args>
shared_ptr<__future_base::task_state_base<Res(Args...)>> __future_base::task_state<Func, Alloc, Res(Args...)>::reset() {
    return inner::create_task_state<Res(Args...)>(_NEFORCE move(impl.function_ptr), static_cast<Alloc&>(impl));
}

NEFORCE_END_INNER__
/// @endcond

/** @} */ // Async

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_PROMISE_HPP__
