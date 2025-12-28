#ifndef MSTL_CORE_ASYNC_LATCH_HPP__
#define MSTL_CORE_ASYNC_LATCH_HPP__
#include "atomic_base.hpp"
MSTL_BEGIN_NAMESPACE__

class latch {
private:
    alignas(alignof(_INNER platform_wait_t))
    _INNER platform_wait_t counter_;

public:
    static constexpr ptrdiff_t max() noexcept {
        return numeric_limits<_INNER platform_wait_t>::max();
    }

    constexpr explicit latch(const ptrdiff_t expected) noexcept
    : counter_(expected) {}

    ~latch() = default;
    latch(const latch&) = delete;
    latch& operator =(const latch&) = delete;

    MSTL_ALWAYS_INLINE void count_down(const ptrdiff_t update = 1) {
        auto const old_value =
            _INNER fetch_sub(&counter_, update, memory_order::release);
        if (old_value == update)
            _INNER notify_all(&counter_);
    }

    MSTL_ALWAYS_INLINE bool try_wait() const noexcept {
        return _INNER load(&counter_, memory_order::acquire) == 0;
    }

    MSTL_ALWAYS_INLINE void wait() const noexcept {
        auto const predicate = [this] { return this->try_wait(); };
        _MSTL atomic_wait_address(&counter_, predicate);
    }

    MSTL_ALWAYS_INLINE void arrive_and_wait(const ptrdiff_t update = 1) noexcept {
        count_down(update);
        wait();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_LATCH_HPP__
