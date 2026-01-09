#ifndef MSTL_CORE_ASYNC_SEMAPHORE_HPP__
#define MSTL_CORE_ASYNC_SEMAPHORE_HPP__
#include "atomic_timed_wait.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

struct atomic_semaphore {
private:
    alignas(_INNER PLATFORM_WAIT_ALIGN)
    _INNER platform_wait_t counter_;

public:
    static constexpr ptrdiff_t int_max = numeric_limits<int>::max();
    
    explicit atomic_semaphore(const _INNER platform_wait_t count) noexcept
        : counter_(count) {
        MSTL_CONSTEXPR_ASSERT(count >= 0 && count <= int_max);
    }

    atomic_semaphore(const atomic_semaphore&) = delete;
    atomic_semaphore& operator =(const atomic_semaphore&) = delete;

    static MSTL_ALWAYS_INLINE bool
    do_try_acquire(_INNER platform_wait_t* counter) noexcept {
        auto old_value = _INNER load(counter, memory_order::acquire);
        if (old_value == 0) {
            return false;
        }
        return _INNER compare_exchange_strong(counter, &old_value, old_value - 1,
            memory_order::acquire, memory_order::relaxed);
    }

    MSTL_ALWAYS_INLINE void acquire() noexcept {
        auto const pred = [this] { 
            return do_try_acquire(&this->counter_); 
        };
        atomic_wait_address(&counter_, pred);
    }

    bool try_acquire() noexcept {
        auto const pred = [this] { 
            return do_try_acquire(&this->counter_); 
        };
        return _INNER atomic_spin(pred);
    }

    template <typename Clock, typename Dur>
    MSTL_ALWAYS_INLINE bool try_acquire_until(const time_point<Clock, Dur>& timeout) noexcept {
        auto const pred = [this] { 
            return do_try_acquire(&this->counter_); 
        };
        return _MSTL atomic_wait_address_until(&counter_, pred, timeout);
    }

    template <typename Rep, typename Period>
    MSTL_ALWAYS_INLINE bool try_acquire_for(const duration<Rep, Period>& relative) noexcept {
        auto const pred = [this] { 
            return do_try_acquire(&this->counter_); 
        };
        return _MSTL atomic_wait_address_for(&counter_, pred, relative);
    }

    MSTL_ALWAYS_INLINE void release(const ptrdiff_t update) noexcept {
        if (0 < _INNER fetch_add(&counter_, update, memory_order_release)) {
            return;
        }
        if (update > 1) {
            _MSTL atomic_notify_address(&counter_, true);
        } else {
            _MSTL atomic_notify_address(&counter_, true);
        }
    }
};

MSTL_END_INNER__

template <ptrdiff_t LeastMaxValue = _INNER atomic_semaphore::int_max>
class counting_semaphore {
    static_assert(LeastMaxValue >= 0, "LeastMaxValue should be upper than zero.");
    static_assert(LeastMaxValue <= _INNER atomic_semaphore::int_max, "LeastMaxValue should be less than max value of ptrdiff_t.");
    _INNER atomic_semaphore sem_;

public:
    explicit counting_semaphore(const ptrdiff_t desired) noexcept
    : sem_(desired) {}

    ~counting_semaphore() = default;

    counting_semaphore(const counting_semaphore&) = delete;
    counting_semaphore& operator =(const counting_semaphore&) = delete;

    static constexpr ptrdiff_t max() noexcept { 
        return LeastMaxValue; 
    }

    void release(const ptrdiff_t update = 1) noexcept(noexcept(sem_.release(1))) {
        sem_.release(update); 
    }

    void acquire() noexcept(noexcept(sem_.acquire())) {
        sem_.acquire(); 
    }

    bool try_acquire() noexcept(noexcept(sem_.try_acquire())) {
        return sem_.try_acquire(); 
    }

    template <typename Rep, typename Period>
    bool try_acquire_for(const duration<Rep, Period>& relative) {
        return sem_.try_acquire_for(relative); 
    }

    template <typename Clock, typename Dur>
    bool try_acquire_until(const time_point<Clock, Dur>& timeout) {
        return sem_.try_acquire_until(timeout); 
    }
};

using binary_semaphore = counting_semaphore<1>;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_SEMAPHORE_HPP__
