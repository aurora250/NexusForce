#ifndef MSTL_CORE_ASYNC_CALL_ONCE_HPP__
#define MSTL_CORE_ASYNC_CALL_ONCE_HPP__
#include "mutex.hpp"
#include "atomic.hpp"
MSTL_BEGIN_NAMESPACE__

class once_flag {
public:
    once_flag() noexcept : state_(false) {}

    once_flag(const once_flag&) = delete;
    once_flag& operator=(const once_flag&) = delete;
    once_flag(once_flag&&) = delete;
    once_flag& operator=(once_flag&&) = delete;

private:
    template<typename Callable, typename... Args>
    friend void call_once(once_flag& flag, Callable&& func, Args&&... args);

    atomic_bool state_;
    mutex mtx_;
};

template <typename Callable, typename... Args>
void call_once(once_flag& flag, Callable&& func, Args&&... args) {
    if (flag.state_.load(memory_order_acquire)) {
        return;
    }
    lock_guard<mutex> lock(flag.mtx_);
    if (!flag.state_.load(memory_order_relaxed)) {
        _MSTL forward<Callable>(func)(_MSTL forward<Args>(args)...);
        flag.state_.store(true, memory_order_release);
    }
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_CALL_ONCE_HPP__
