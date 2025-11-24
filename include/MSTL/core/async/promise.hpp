#ifndef MSTL_CORE_ASYNC_PROMISE_HPP__
#define MSTL_CORE_ASYNC_PROMISE_HPP__
#include "future.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Result>
class promise {
    static_assert(!is_array<Result>{}, "result type must not be an array");
    static_assert(!is_function<Result>{}, "result type must not be a function");
    static_assert(is_destructible<Result>{}, "result type must be destructible");

public:
    typedef __future_base::state_base state_type;
    typedef __future_base::basic_result<Result> result_type;
    typedef __future_base::Ptr<result_type> ptr_type;

private:
    shared_ptr<state_type> future_ptr;
    ptr_type storage;

    template <typename T, typename U>
    friend struct __future_base::state_base::setter;

public:
    promise()
        : future_ptr(_MSTL make_shared<state_type>())
        , storage(new result_type())
    { }

    promise(promise&& other) noexcept
        : future_ptr(_MSTL move(other.future_ptr))
        , storage(_MSTL move(other.storage))
    { }

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

    future<Result> get_future() {
        return future<Result>(future_ptr);
    }

    void set_value(const Result& value) {
        state().set_result(state_type::create_setter(this, value));
    }

    void set_value(Result&& value) {
        state().set_result(state_type::create_setter(this, _MSTL move(value)));
    }

    void set_exception(exception_ptr exception) {
        state().set_result(state_type::create_setter(exception, this));
    }

    void set_value_at_thread_exit(const Result& value) {
        state().set_delayed_result(state_type::create_setter(this, value), future_ptr);
    }

    void set_value_at_thread_exit(Result&& value) {
        state().set_delayed_result(
            state_type::create_setter(this, _MSTL move(value)), future_ptr);
    }

    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }

private:
    state_type& state() {
        __future_base::state_base::check(future_ptr);
        return *future_ptr;
    }
};

template<typename Result>
inline void swap(promise<Result>& left, promise<Result>& right) noexcept
{
    left.swap(right);
}

template <typename Result>
class promise<Result&> {
public:
    typedef __future_base::state_base state_type;
    typedef __future_base::basic_result<Result&> result_type;
    typedef __future_base::Ptr<result_type> ptr_type;

private:
    shared_ptr<state_type> future_ptr;
    ptr_type storage;

    template <typename T, typename U>
    friend struct __future_base::state_base::setter;

public:
    promise()
        : future_ptr(_MSTL make_shared<state_type>())
        , storage(new result_type())
    { }

    promise(promise&& other) noexcept
        : future_ptr(_MSTL move(other.future_ptr))
        , storage(_MSTL move(other.storage))
    { }

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

    future<Result&> get_future() {
        return future<Result&>(future_ptr);
    }

    void set_value(Result& value) {
        state().set_result(state_type::create_setter(this, value));
    }

    void set_exception(exception_ptr exception) {
        state().set_result(state_type::create_setter(exception, this));
    }

    void set_value_at_thread_exit(Result& value) {
        state().set_delayed_result(state_type::create_setter(this, value), future_ptr);
    }

    void set_exception_at_thread_exit(exception_ptr exception) {
        state().set_delayed_result(state_type::create_setter(exception, this), future_ptr);
    }

private:
    state_type& state() {
        __future_base::state_base::check(future_ptr);
        return *future_ptr;
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

public:
    promise()
    : future_ptr(_MSTL make_shared<state_type>())
    , storage(new result_type()) {}

    promise(promise&& other) noexcept
        : future_ptr(_MSTL move(other.future_ptr))
        , storage(_MSTL move(other.storage))
    { }

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

    future<void> get_future() {
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

private:
    state_type& state() {
        __future_base::state_base::check(future_ptr);
        return *future_ptr;
    }
};


template <typename PtrType, typename Function, typename Result>
struct __future_base::task_setter {
    PtrType operator()() const {
        try {
            (*result_ptr)->set((*function_ptr)());
        } catch(...) {
            (*result_ptr)->error_ptr = current_exception();
        }
        return _MSTL move(*result_ptr);
    }
    PtrType* result_ptr;
    Function* function_ptr;
};

template <typename PtrType, typename Function>
struct __future_base::task_setter<PtrType, Function, void> {
    PtrType operator()() const {
        try {
            (*function_ptr)();
        } catch(...) {
            (*result_ptr)->error_ptr = current_exception();
        }
        return _MSTL move(*result_ptr);
    }
    PtrType* result_ptr;
    Function* function_ptr;
};

template <typename Result, typename... Args>
struct __future_base::task_state_base<Result(Args...)>
    : __future_base::state_base {
    typedef Result result_type;

    template<typename Alloc>
    task_state_base(const Alloc& alloc)
        : result_storage(allocate_result<Result>(alloc))
    { }

    virtual void run(Args&&... args) = 0;

    virtual void run_delayed(Args&&... args, weak_ptr<state_base> self) = 0;

    virtual shared_ptr<task_state_base> reset() = 0;

    typedef __future_base::Ptr<basic_result<Result>> PtrType;
    PtrType result_storage;
};

template <typename Function, typename Alloc, typename Result, typename... Args>
struct __future_base::task_state<Function, Alloc, Result(Args...)> final
    : __future_base::task_state_base<Result(Args...)> {
    template<typename Function2>
    task_state(Function2&& function, const Alloc& alloc)
        : task_state_base<Result(Args...)>(alloc)
        , implementation(_MSTL forward<Function2>(function), alloc)
    {}

private:
    virtual void run(Args&&... args) {
        auto bound_function = [&]() -> Result {
            return _MSTL invoke_r<Result>(implementation.function_ptr,
                                         _MSTL forward<Args>(args)...);
        };
        this->set_result(create_task_setter(this->result_storage, bound_function));
    }

    virtual void run_delayed(Args&&... args, weak_ptr<state_base> self) {
        auto bound_function = [&]() -> Result {
            return _MSTL invoke_r<Result>(implementation.function_ptr,
                                         _MSTL forward<Args>(args)...);
        };
        this->set_delayed_result(create_task_setter(this->result_storage, bound_function),
                               _MSTL move(self));
    }

    virtual shared_ptr<task_state_base<Result(Args...)>>
    reset();

    struct Implementation : Alloc {
        template<typename Function2>
        Implementation(Function2&& function, const Alloc& alloc)
            : Alloc(alloc), function_ptr(_MSTL forward<Function2>(function)) { }
        Function function_ptr;
    } implementation;
};

template <typename Signature, typename Function, typename Alloc = _MSTL allocator<int>>
static shared_ptr<__future_base::task_state_base<Signature>>
create_task_state(Function&& function, const Alloc& alloc = Alloc()) {
    typedef typename decay<Function>::type Function2;
    typedef __future_base::task_state<Function2, Alloc, Signature> State;
    return _MSTL allocate_shared<State>(alloc, _MSTL forward<Function>(function), alloc);
}

template <typename Function, typename Alloc, typename Result, typename... Args>
shared_ptr<__future_base::task_state_base<Result(Args...)>>
__future_base::task_state<Function, Alloc, Result(Args...)>::reset() {
    return create_task_state<Result(Args...)>(
        _MSTL move(implementation.function_ptr), static_cast<Alloc&>(implementation));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_PROMISE_HPP__
