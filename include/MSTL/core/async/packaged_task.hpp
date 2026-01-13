#ifndef MSTL_CORE_ASYNC_PACKAGED_TASK_HPP__
#define MSTL_CORE_ASYNC_PACKAGED_TASK_HPP__
#include "promise.hpp"
#include "thread.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Res, typename... Args>
class packaged_task<Res(Args...)> {
    using StateType = __future_base::task_state_base<Res(Args...)>;

    shared_ptr<StateType> state_ptr;

    template <typename Func, typename U = remove_cvref_t<Func>>
    using not_same_t = typename enable_if<!is_same<packaged_task, U>::value>::type;

public:
    packaged_task() noexcept {}

    template <typename Func, typename = not_same_t<Func>>
    explicit packaged_task(Func&& function)
    : state_ptr(create_task_state<Res(Args...)>(_MSTL forward<Func>(function)))
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

    future<Res> get_future() {
        return future<Res>(state_ptr);
    }

    void operator ()(Args... args) {
        __future_base::state_base::check(state_ptr);
        state_ptr->run(_MSTL forward<Args>(args)...);
    }

    void make_ready_at_thread_exit(Args... args) {
        __future_base::state_base::check(state_ptr);
        state_ptr->run_delayed(_MSTL forward<Args>(args)..., state_ptr);
    }

    void reset() {
        __future_base::state_base::check(state_ptr);
        packaged_task temp;
        temp.state_ptr = state_ptr;
        state_ptr = state_ptr->reset();
    }
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Res, typename... Args>
packaged_task(Res(*)(Args...)) -> packaged_task<Res(Args...)>;

template <typename Func, typename Sign = typename
    _INNER __function_guide_helper<decltype(&Func::operator ())>::type>
packaged_task(Func) -> packaged_task<Sign>;
#endif

template <typename Res, typename... Args>
void swap(
    packaged_task<Res(Args...)>& left,
    packaged_task<Res(Args...)>& right) noexcept {
    left.swap(right);
}


template <typename BoundFunc, typename Res>
class __future_base::deferred_state final
    : public __future_base::state_base {
private:
    using PtrType = __future_base::Ptr<basic_result<Res>>;

    PtrType result_storage;
    BoundFunc function;

    void complete_async() override {
        state_base::set_result(create_task_setter(result_storage, function), true);
    }
    bool is_deferred_future() const override { return true; }

public:
    template <typename... Args>
    explicit deferred_state(Args&&... args)
    : result_storage(new basic_result<Res>()),
    function(_MSTL forward<Args>(args)...) {}

};

class __future_base::async_state_common
    : public __future_base::state_base {
protected:
    _MSTL thread thread;
    _MSTL once_flag once_flag;

    ~async_state_common() override = default;

    void complete_async() override { join(); }
    void join() { _MSTL call_once(once_flag, &_MSTL thread::join, &thread); }
};

template <typename Func, typename Res>
class __future_base::async_state_impl final
    : public __future_base::async_state_common {
private:
    using PtrType = __future_base::Ptr<basic_result<Res>>;

    PtrType result_storage;
    Func function;

    void run() {
        try {
            state_base::set_result(__future_base::create_task_setter(result_storage, function));
        }
        catch (...) {
            if (static_cast<bool>(result_storage))
                state_base::break_promise(_MSTL move(result_storage));
            throw;
        }
    }

public:
    template <typename... Args>
    explicit async_state_impl(Args&&... args)
    : result_storage(new basic_result<Res>()), function(_MSTL forward<Args>(args)...) {
        thread = _MSTL thread{&async_state_impl::run, this};
    }
    ~async_state_impl() override {
        if (thread.joinable()) {
            thread.join();
        }
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_PACKAGED_TASK_HPP__
