#ifndef MSTL_CORE_ASYNC_PACKAGED_TASK_HPP__
#define MSTL_CORE_ASYNC_PACKAGED_TASK_HPP__
#include "promise.hpp"
#include "thread.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Result, typename... ArgTypes>
class packaged_task<Result(ArgTypes...)> {
    typedef __future_base::task_state_base<Result(ArgTypes...)> StateType;
    shared_ptr<StateType> state_ptr;

    template<typename Function, typename Function2 = remove_cvref_t<Function>>
    using NotSameType = typename enable_if<!is_same<packaged_task, Function2>::value>::type;

public:
    packaged_task() noexcept {}

    template <typename Function, typename = NotSameType<Function>>
    explicit packaged_task(Function&& function)
    : state_ptr(create_task_state<Result(ArgTypes...)>(_MSTL forward<Function>(function)))
    {}

    ~packaged_task() {
        if (static_cast<bool>(state_ptr) && !state_ptr.unique())
            state_ptr->break_promise(_MSTL move(state_ptr->result_storage));
    }

    packaged_task(const packaged_task&) = delete;
    packaged_task& operator=(const packaged_task&) = delete;

    packaged_task(packaged_task&& other) noexcept {
        this->swap(other);
    }

    packaged_task& operator=(packaged_task&& other) noexcept {
        packaged_task(_MSTL move(other)).swap(*this);
        return *this;
    }

    void swap(packaged_task& other) noexcept {
        state_ptr.swap(other.state_ptr);
    }

    bool valid() const noexcept {
        return static_cast<bool>(state_ptr);
    }

    future<Result> get_future() {
        return future<Result>(state_ptr);
    }

    void operator()(ArgTypes... args) {
        __future_base::state_base::check(state_ptr);
        state_ptr->run(_MSTL forward<ArgTypes>(args)...);
    }

    void make_ready_at_thread_exit(ArgTypes... args) {
        __future_base::state_base::check(state_ptr);
        state_ptr->run_delayed(_MSTL forward<ArgTypes>(args)..., state_ptr);
    }

    void reset() {
        __future_base::state_base::check(state_ptr);
        packaged_task temp;
        temp.state_ptr = state_ptr;
        state_ptr = state_ptr->reset();
    }
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Result, typename... ArgTypes>
packaged_task(Result(*)(ArgTypes...)) -> packaged_task<Result(ArgTypes...)>;

template <typename Function, typename Signature = typename
    _INNER __function_guide_helper<decltype(&Function::operator())>::type>
packaged_task(Function) -> packaged_task<Signature>;
#endif

template <typename Result, typename... ArgTypes>
void swap(
    packaged_task<Result(ArgTypes...)>& left,
    packaged_task<Result(ArgTypes...)>& right) noexcept {
    left.swap(right);
}


template <typename BoundFunction, typename Result>
class __future_base::deferred_state final
    : public __future_base::state_base {
public:
    template <typename... Args>
    explicit deferred_state(Args&&... args)
    : result_storage(new basic_result<Result>()),
    function(_MSTL forward<Args>(args)...) {}

private:
    typedef __future_base::Ptr<basic_result<Result>> PtrType;
    PtrType result_storage;
    BoundFunction function;

    void complete_async() override {
        set_result(create_task_setter(result_storage, function), true);
    }

    bool is_deferred_future() const override { return true; }
};

class __future_base::async_state_common
    : public __future_base::state_base {
protected:
    ~async_state_common() override = default;

    void complete_async() override { join(); }

    void join() { _MSTL call_once(once_flag, &_MSTL thread::join, &thread); }

    _MSTL thread thread;
    _MSTL once_flag once_flag;
};

template <typename Func, typename Res>
class __future_base::async_state_impl final
    : public __future_base::async_state_common {
public:
    template <typename... Args>
    explicit async_state_impl(Args&&... args)
    : result_storage(new basic_result<Res>()),
    function(_MSTL forward<Args>(args)...) {
        thread = _MSTL thread{&async_state_impl::run, this};
    }

    ~async_state_impl() override {
        if (thread.joinable()) {
            thread.join();
        }
    }

private:
    void run() {
        try {
            this->set_result(create_task_setter(result_storage, function));
        }
        catch (...) {
            if (static_cast<bool>(result_storage))
                this->break_promise(_MSTL move(result_storage));
            throw;
        }
    }

    typedef __future_base::Ptr<basic_result<Res>> PtrType;
    PtrType result_storage;
    Func function;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_PACKAGED_TASK_HPP__
