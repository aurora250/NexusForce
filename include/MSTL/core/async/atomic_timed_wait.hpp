#ifndef MSTL_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
#include "atomic_wait.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include "atomic_futex_base.hpp"
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

using wait_clock_t = chrono::steady_clock;

template <typename Clock, typename Dur>
wait_clock_t::time_point to_wait_clock(const chrono::time_point<Clock, Dur>& time_point) noexcept {
    const typename Clock::time_point clock_entry = Clock::now();
    const wait_clock_t::time_point wait_entry = wait_clock_t::now();
    const auto delta = time_point - clock_entry;
    return wait_entry + chrono::ceil<wait_clock_t::duration>(delta);
}

template <typename Dur>
wait_clock_t::time_point to_wait_clock(const chrono::time_point<wait_clock_t, Dur>& time_point) noexcept {
    return chrono::ceil<wait_clock_t::duration>(time_point);
}

template <typename Dur>
bool platform_wait_until_impl(const platform_wait_t* addr, platform_wait_t old,
    const chrono::time_point<wait_clock_t, Dur>& timeout) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    atomic_futex_base futex;
    const auto tp = wait_clock_t::time_point(
        chrono::duration_cast<wait_clock_t::duration>(timeout.time_since_epoch()));

    const auto sec = chrono::duration_cast<chrono::seconds>(tp.time_since_epoch());
    const auto ns = chrono::duration_cast<chrono::nanoseconds>(tp.time_since_epoch() - sec);

    const auto futex_addr = reinterpret_cast<unsigned*>(const_cast<platform_wait_t*>(addr));
    const unsigned expected_value = static_cast<unsigned>(old);
    bool result = false;
    const bool has_timeout = (timeout != wait_clock_t::time_point::max());

    if (has_timeout) {
        const auto now = wait_clock_t::now();
        if (timeout <= now) {
            errno = ETIMEDOUT;
            return false;
        }

        result = futex.futex_wait_until_steady(
            futex_addr, expected_value,
            true,
            chrono::seconds(sec.count()),
            chrono::nanoseconds(ns.count()));
    } else {
        result = futex.futex_wait_until_steady(
            futex_addr, expected_value,
            false,
            chrono::seconds(0),
            chrono::nanoseconds(0));
    }

    if (!result && has_timeout) {
        const auto now = wait_clock_t::now();
        if (now >= timeout) {
            errno = ETIMEDOUT;
            return false;
        }
        errno = EAGAIN;
    }
    return result;
#else
    auto seconds = chrono::time_point_cast<chrono::seconds>(timeout);
    auto nanoseconds = chrono::duration_cast<chrono::nanoseconds>(timeout - seconds);

    std::timespec rt = {
        static_cast<std::time_t>(seconds.time_since_epoch().count()),
        static_cast<long>(nanoseconds.count())
    };

    const auto error = ::syscall(SYS_futex, addr,
        static_cast<int>(futex_wait_flags::wait_bitset_private),
        old, &rt, nullptr,
        static_cast<int>(futex_wait_flags::bitset_match_any));
    
    if (error) {
        if (errno == ETIMEDOUT) {
            return false;
        } else if (errno != EINTR && errno != EAGAIN) {
            throw_exception(system_exception());
        }
    }
    return true;
#endif
}

template <typename Clock, typename Dur>
bool platform_wait_until(const platform_wait_t* addr, platform_wait_t old,
    const chrono::time_point<Clock, Dur>& timeout) {
    if constexpr (is_same_v<wait_clock_t, Clock>) {
        return _INNER platform_wait_until_impl(addr, old, timeout);
    } else {
        if (!_INNER platform_wait_until_impl(addr, old, _INNER to_wait_clock(timeout))) {
            if (Clock::now() < timeout) return true;
        }
        return false;
    }
}

struct timed_waiter_pool : waiter_pool_base {
    template <typename Clock, typename Dur>
    bool do_wait_until(platform_wait_t* addr, platform_wait_t old,
        const chrono::time_point<Clock, Dur>& timeout) {
        return _INNER platform_wait_until(addr, old, timeout);
    }
};

struct timed_backoff_spin_policy {
    wait_clock_t::time_point deadline;
    wait_clock_t::time_point start_time;

    template <typename Clock, typename Dur>
    timed_backoff_spin_policy(chrono::time_point<Clock, Dur> deadline_time = Clock::time_point::max(),
        chrono::time_point<Clock, Dur> start_time_point = Clock::now()) noexcept
    : deadline(_INNER to_wait_clock(deadline_time)),
    start_time(_INNER to_wait_clock(start_time_point)) {}

    bool operator ()() const noexcept {
        const auto now = wait_clock_t::now();
        if (deadline <= now)
            return false;
        
        const auto elapsed = now - start_time;
        if (elapsed > 128_ms) {
            this_thread::sleep_for(64_ms);
        } else if (elapsed > 64_us) {
            this_thread::sleep_for(elapsed / 2);
        } else if (elapsed > 4_us) {
            _INNER thread_yield();
        } else {
            return false;
        }
        return true;
    }
};

template <typename EntersWait>
struct timed_waiter : waiter_base<timed_waiter_pool> {
    using base_type = waiter_base<timed_waiter_pool>;

    template <typename T>
    explicit timed_waiter(const T* addr) noexcept : base_type(addr) {
        if constexpr (EntersWait::value) {
            waiter_.waiter_enter_wait();
        }
    }

    ~timed_waiter() {
        if constexpr (EntersWait::value) {
            waiter_.waiter_leave_wait();
        }
    }

    template <typename T, typename Func, typename Clock, typename Dur>
    bool waiter_do_wait_until_v(T old, Func func,
        const chrono::time_point<Clock, Dur>& timeout) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin(old, _MSTL move(func), value,
            timed_backoff_spin_policy(timeout))) {
            return true;
        }
        return base_type::waiter_.do_wait_until(base_type::addr_, value, timeout);
    }

    template <typename Pred, typename Clock, typename Dur>
    bool waiter_do_wait_until(Pred pred, platform_wait_t value,
        const chrono::time_point<Clock, Dur>& timeout) noexcept {
        for (auto now = Clock::now(); now < timeout; now = Clock::now()) {
            if (base_type::waiter_.do_wait_until(base_type::addr_, value, timeout) && pred()) {
                return true;
            }
            if (base_type::waiter_do_spin(pred, value,
                _INNER timed_backoff_spin_policy(timeout, now)))
                return true;
        }
        return false;
    }

    template <typename Pred, typename Clock, typename Dur>
    bool waiter_do_wait_until(Pred pred, const chrono::time_point<Clock, Dur>& timeout) noexcept {
        platform_wait_t value;
        if (this->waiter_do_spin(pred, value,
            _INNER timed_backoff_spin_policy(timeout))) {
            return true;
        }
        return this->waiter_do_wait_until(pred, value, timeout);
    }

    template <typename T, typename Func, typename Rep, typename Period>
    bool waiter_do_wait_for_v(T old, Func func, const chrono::duration<Rep, Period>& rt) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin_v(old, _MSTL move(func), value)) {
            return true;
        }
        if (!rt.count()) {
            return false;
        }
        auto rtc = chrono::ceil<wait_clock_t::duration>(rt);
        return base_type::waiter_.do_wait_until(base_type::addr_, value, chrono::steady_clock::now() + rtc);
    }

    template <typename Pred, typename Rep, typename Period>
    bool waiter_do_wait_for(Pred pred, const chrono::duration<Rep, Period>& rt) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin(pred, value)) {
            return true;
        }
        if (!rt.count()) {
            return false;
        }
        auto rtc = chrono::ceil<wait_clock_t::duration>(rt);
        return this->waiter_do_wait_until(pred, value, chrono::steady_clock::now() + rtc);
    }
};

using enters_timed_wait = timed_waiter<true_type>;
using bare_timed_wait = timed_waiter<false_type>;

MSTL_END_INNER__

template <typename T, typename Func, typename Clock, typename Dur>
bool atomic_wait_address_until_v(const T* addr, T&& old,
    Func&& func, const chrono::time_point<Clock, Dur>& timeout) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until_v(old, func, timeout);
}

template <typename T, typename Pred, typename Clock, typename Dur>
bool atomic_wait_address_until(const T* addr, Pred pred,
    const chrono::time_point<Clock, Dur>& timeout) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until(pred, timeout);
}

template <typename Pred, typename Clock, typename Dur>
bool atomic_wait_address_until(const _INNER platform_wait_t* addr, Pred pred,
    const chrono::time_point<Clock, Dur>& timeout) noexcept {
    _INNER bare_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until(pred, timeout);
}

template <typename T, typename Func, typename Rep, typename Period>
bool atomic_wait_address_for_v(const T* addr, T&& old, Func&& func,
    const chrono::duration<Rep, Period>& rt) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for_v(old, func, rt);
}

template <typename T, typename Pred, typename Rep, typename Period>
bool atomic_wait_address_for(const T* addr, Pred pred,
    const chrono::duration<Rep, Period>& rt) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for(pred, rt);
}

template <typename Pred, typename Rep, typename Period>
bool atomic_wait_address_for(const _INNER platform_wait_t* addr, Pred pred,
    const chrono::duration<Rep, Period>& rt) noexcept {
    _INNER bare_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for(pred, rt);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
