#ifndef MSTL_CORE_ASYNC_PROMISE_HPP__
#define MSTL_CORE_ASYNC_PROMISE_HPP__
#include "future.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Res>
class promise {
    static_assert(!is_array_v<Res>, "result type must not be an array");
    static_assert(!is_function_v<Res>, "result type must not be a function");
    static_assert(is_destructible_v<Res>, "result type must be destructible");

public:
    typedef __future_base::state_base state_type;
    typedef __future_base::basic_result<Res> result_type;
    typedef __future_base::Ptr<result_type> ptr_type;

private:
    shared_ptr<state_type> future_ptr;
    ptr_type storage;

    template <typename T, typename U>
    friend struct __future_base::state_base::setter;

    MSTL_NODISCARD state_type& state() const {
        __future_base::state_base::check(future_ptr);
        return *future_ptr;
    }

public:
    promise()
    : future_ptr(_MSTL make_shared<state_type>()) , storage(new result_type()) {}

    promise(promise&& other) noexcept
    : future_ptr(_MSTL move(other.future_ptr)) , storage(_MSTL move(other.storage)) {}

    promise(const promise&) = delete;

    ~promise() {
        if (static_cast<bool>(future_ptr) && !future_ptr.unique()) {
            future_ptr->break_promise(_MSTL move(storage));
        }
    }

    promise& operator=(promise&& other) noexcept {
        promise(_MSTL move(other)).swap(*this);
        return *this;
    }

    promise& operator=(const promise&) = delete;

    void swap(promise& other) noexcept {
        future_ptr.swap(other.future_ptr);
        storage.swap(other.storage);
    }

    future<Res> get_future() {
        return future<Res>(future_ptr);
    }

    void set_value(const Res& value) {
        state().set_result(state_type::create_setter(this, value));
    }

    void set_value(Res&& value) {
        state().set_result(state_type::create_setter(this, _MSTL move(value)));
    }

    void set_exception(exception_ptr exception) {
        state().set_result(state_type::create_setter(exception, this));
    }

    void set_value_at_thread_exit(const Res& value) {
        state().set_delayed_result(state_type::create_setter(this, value), future_ptr);
    }

    void set_value_at_thread_exit(Res&& value) {
        state().set_delayed_result(
            state_type::create_setter(this, _MSTL move(value)), future_ptr);
    }

    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }
};

template <typename Res>
void swap(promise<Res>& left, promise<Res>& right) noexcept {
    left.swap(right);
}


template <typename Res>
class promise<Res&> {
public:
    typedef __future_base::state_base state_type;
    typedef __future_base::basic_result<Res&> result_type;
    typedef __future_base::Ptr<result_type> ptr_type;

private:
    shared_ptr<state_type> future_ptr;
    ptr_type storage;

    template <typename T, typename U>
    friend struct __future_base::state_base::setter;

    MSTL_NODISCARD state_type& state() const {
        __future_base::state_base::check(future_ptr);
        return *future_ptr;
    }

public:
    promise()
    : future_ptr(_MSTL make_shared<state_type>()), storage(new result_type()) {}

    promise(promise&& other) noexcept
    : future_ptr(_MSTL move(other.future_ptr)), storage(_MSTL move(other.storage)) {}

    promise(const promise&) = delete;

    ~promise() {
        if (static_cast<bool>(future_ptr) && !future_ptr.unique()) {
            future_ptr->break_promise(_MSTL move(storage));
        }
    }

    promise& operator=(promise&& other) noexcept {
        promise(_MSTL move(other)).swap(*this);
        return *this;
    }

    promise& operator=(const promise&) = delete;

    void swap(promise& other) noexcept {
        future_ptr.swap(other.future_ptr);
        storage.swap(other.storage);
    }

    future<Res&> get_future() {
        return future<Res&>(future_ptr);
    }

    void set_value(Res& value) {
        state().set_result(state_type::create_setter(this, value));
    }

    void set_exception(exception_ptr exception) {
        state().set_result(state_type::create_setter(exception, this));
    }

    void set_value_at_thread_exit(Res& value) {
        state().set_delayed_result(state_type::create_setter(this, value), future_ptr);
    }

    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }
};

template <>
class promise<void> {
public:
    typedef __future_base::state_base state_type;
    typedef __future_base::basic_result<void> result_type;
    typedef __future_base::Ptr<result_type> ptr_type;

private:
    shared_ptr<state_type> future_ptr;
    ptr_type storage;

    template <typename T, typename U>
    friend struct __future_base::state_base::setter;

    MSTL_NODISCARD state_type& state() const {
        __future_base::state_base::check(future_ptr);
        return *future_ptr;
    }

public:
    promise()
    : future_ptr(_MSTL make_shared<state_type>()), storage(new result_type()) {}

    promise(promise&& other) noexcept
    : future_ptr(_MSTL move(other.future_ptr)), storage(_MSTL move(other.storage)) {}

    promise(const promise&) = delete;

    ~promise() {
        if (static_cast<bool>(future_ptr) && !future_ptr.unique()) {
            future_ptr->break_promise(_MSTL move(storage));
        }
    }

    promise& operator=(promise&& other) noexcept {
        promise(_MSTL move(other)).swap(*this);
        return *this;
    }

    promise& operator=(const promise&) = delete;

    void swap(promise& other) noexcept {
        future_ptr.swap(other.future_ptr);
        storage.swap(other.storage);
    }

    MSTL_NODISCARD future<void> get_future() const {
        return future<void>(future_ptr);
    }

    void set_value() {
        state().set_result(state_type::create_setter(this));
    }

    void set_exception(exception_ptr exception) {
        state().set_result(state_type::create_setter(exception, this));
    }

    void set_value_at_thread_exit() {
        state().set_delayed_result(state_type::create_setter(this), future_ptr);
    }

    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }
};


template <typename PtrT, typename Func, typename>
struct __future_base::task_setter {
    PtrT* result_ptr;
    Func* function_ptr;

    PtrT operator()() const {
        try {
            (*result_ptr)->set((*function_ptr)());
        } catch(...) {
            (*result_ptr)->error_ptr = current_exception();
        }
        return _MSTL move(*result_ptr);
    }
};

template <typename PtrT, typename Func>
struct __future_base::task_setter<PtrT, Func, void> {
    PtrT* result_ptr;
    Func* function_ptr;

    PtrT operator()() const {
        try {
            (*function_ptr)();
        } catch(...) {
            (*result_ptr)->error_ptr = current_exception();
        }
        return _MSTL move(*result_ptr);
    }
};

template <typename Res, typename... Args>
class __future_base::task_state_base<Res(Args...)> : public __future_base::state_base {
public:
    typedef Res result_type;
    typedef __future_base::Ptr<basic_result<Res>> PtrType;
    PtrType result_storage;

    template <typename Alloc>
    task_state_base(const Alloc& alloc)
    : result_storage(allocate_result<Res>(alloc)) {}

    virtual void run(Args&&... args) = 0;

    virtual void run_delayed(Args&&... args, weak_ptr<state_base> self) = 0;

    virtual shared_ptr<task_state_base> reset() = 0;
};

template <typename Func, typename Alloc, typename Res, typename... Args>
class __future_base::task_state<Func, Alloc, Res(Args...)> final
    : public __future_base::task_state_base<Res(Args...)> {
public:
    template <typename Func2>
    task_state(Func2&& func, const Alloc& alloc)
    : task_state_base<Res(Args...)>(alloc)
    , impl(_MSTL forward<Func2>(func), alloc) {}

private:
    void run(Args&&... args) override {
        auto bound_func = [&]() -> Res {
            return _MSTL invoke_r<Res>(impl.function_ptr, _MSTL forward<Args>(args)...);
        };
        this->set_result(create_task_setter(this->result_storage, bound_func));
    }

    void run_delayed(Args&&... args, weak_ptr<state_base> self) override {
        auto bound_function = [&]() -> Res {
            return _MSTL invoke_r<Res>(impl.function_ptr, _MSTL forward<Args>(args)...);
        };
        this->set_delayed_result(create_task_setter(this->result_storage, bound_function), _MSTL move(self));
    }

    shared_ptr<task_state_base<Res(Args...)>>
    reset() override;

    struct Impl : Alloc {
        Func function_ptr;

        template <typename Func2>
        Impl(Func2&& func, const Alloc& alloc)
        : Alloc(alloc), function_ptr(_MSTL forward<Func2>(func)) {}
    } impl;
};

template <typename Sign, typename Func, typename Alloc = _MSTL allocator<int>>
static shared_ptr<__future_base::task_state_base<Sign>>
create_task_state(Func&& func, const Alloc& alloc = Alloc()) {
    typedef typename decay<Func>::type Function2;
    typedef __future_base::task_state<Function2, Alloc, Sign> State;
    return _MSTL allocate_shared<State>(alloc, _MSTL forward<Func>(func), alloc);
}

template <typename Func, typename Alloc, typename Res, typename... Args>
shared_ptr<__future_base::task_state_base<Res(Args...)>>
__future_base::task_state<Func, Alloc, Res(Args...)>::reset() {
    return create_task_state<Res(Args...)>(_MSTL move(impl.function_ptr), static_cast<Alloc&>(impl));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_PROMISE_HPP__
