#ifndef MSTL_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
#include "atomic_wait.hpp"
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

using wait_clock_t = steady_clock;

template <typename Clock, typename Dur>
wait_clock_t::time_point to_wait_clock(const time_point<Clock, Dur>& time_point) noexcept {
    const typename Clock::time_point clock_entry = Clock::now();
    const wait_clock_t::time_point wait_entry = wait_clock_t::now();
    const auto delta = time_point - clock_entry;
    return wait_entry + ceil<wait_clock_t::duration>(delta);
}

template <typename Dur>
wait_clock_t::time_point to_wait_clock(const time_point<wait_clock_t, Dur>& time_point) noexcept {
    return ceil<wait_clock_t::duration>(time_point);
}

template <typename Dur>
bool __platform_wait_until_impl(const platform_wait_t* addr, const platform_wait_t old,
    const time_point<wait_clock_t, Dur>& timeout) noexcept {
    const bool has_timeout = (timeout != wait_clock_t::time_point::max());

    if (!has_timeout) {
        _MSTL futex_wait(const_cast<void*>(static_cast<const void*>(addr)), old);
        return true;
    }

    const auto now = wait_clock_t::now();
    if (timeout <= now) {
        return false;
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    const auto dur = timeout - now;
    const auto sec = _MSTL time_cast<seconds>(dur);
    const auto ns = _MSTL time_cast<nanoseconds>(dur - sec);
#else
    const auto sys_timeout = steady_clock::to_system<Dur>(timeout);
    const auto sys_now = system_clock::now();
    if (sys_timeout <= sys_now) {
        return false;
    }
    const auto sys_dur = sys_timeout - sys_now;
    const auto sec = _MSTL time_cast<seconds>(sys_dur);
    const auto ns = _MSTL time_cast<nanoseconds>(sys_dur - sec);
#endif

    return _MSTL futex_wait_until_steady(
        const_cast<void*>(static_cast<const void*>(addr)),
        old,
        true,
        sec.count(),
        ns.count());
}

template <typename Clock, typename Dur ,enable_if_t<is_same_v<wait_clock_t, Clock>, int> = 0>
bool __platform_wait_until_dispatch(const platform_wait_t* addr, platform_wait_t old,
    const time_point<Clock, Dur>& timeout) {
    return _INNER __platform_wait_until_impl(addr, old, timeout);
}

template <typename Clock, typename Dur ,enable_if_t<!is_same_v<wait_clock_t, Clock>, int> = 0>
bool __platform_wait_until_dispatch(const platform_wait_t* addr, platform_wait_t old,
    const time_point<Clock, Dur>& timeout) {
    if (!_INNER __platform_wait_until_impl(addr, old, _INNER to_wait_clock(timeout))) {
        if (Clock::now() < timeout) return true;
    }
    return false;
}

MSTL_END_INNER__

template <typename Clock, typename Dur>
bool futex_wait_until(const platform_wait_t* addr, platform_wait_t old,
    const time_point<Clock, Dur>& timeout) {
    return _INNER __platform_wait_until_dispatch(addr, old, timeout);
}

MSTL_BEGIN_INNER__

struct timed_waiter_pool : waiter_pool_base {
    template <typename Clock, typename Dur>
    bool do_wait_until(platform_wait_t* addr, platform_wait_t old,
        const time_point<Clock, Dur>& timeout) {
        return _MSTL futex_wait_until(addr, old, timeout);
    }
};

struct timed_backoff_spin_policy {
    wait_clock_t::time_point deadline;
    wait_clock_t::time_point start_time;

    template <typename Clock, typename Dur>
    timed_backoff_spin_policy(
        time_point<Clock, Dur> deadline_time = Clock::time_point::max(),
        time_point<Clock, Dur> start_time_point = Clock::now()) noexcept
    : deadline(_INNER to_wait_clock(deadline_time)),
      start_time(_INNER to_wait_clock(start_time_point)) {}

    bool operator ()() const noexcept {
        const auto now = wait_clock_t::now();
        if (deadline <= now) {
            return false;
        }
        
        const auto elapsed = now - start_time;
        if (elapsed > 128_ms) {
            this_thread::sleep_for(64_ms);
        } else if (elapsed > 64_us) {
            this_thread::sleep_for(elapsed / 2);
        } else if (elapsed > 4_us) {
            this_thread::yield();
        } else {
            return false;
        }
        return true;
    }
};

template <typename EntersWait>
struct timed_waiter : waiter_base<timed_waiter_pool> {
    using base_type = waiter_base<timed_waiter_pool>;

private:
    template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
    MSTL_ALWAYS_INLINE void enter() const {
        waiter_.waiter_enter_wait();
    }
    template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
    MSTL_ALWAYS_INLINE void enter() const noexcept {}

    template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
    MSTL_ALWAYS_INLINE void leave() const {
        waiter_.waiter_leave_wait();
    }
    template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
    MSTL_ALWAYS_INLINE void leave() const noexcept {}

public:
    template <typename T>
    explicit timed_waiter(const T* addr) noexcept : base_type(addr) { enter(); }
    ~timed_waiter() { leave(); }

    template <typename T, typename Func, typename Clock, typename Dur>
    bool waiter_do_wait_until_v(T old, Func func,
        const time_point<Clock, Dur>& timeout) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin(old, _MSTL move(func), value,
            timed_backoff_spin_policy(timeout))) {
            return true;
        }
        return base_type::waiter_.do_wait_until(base_type::addr_, value, timeout);
    }

    template <typename Pred, typename Clock, typename Dur>
    bool waiter_do_wait_until(Pred pred, platform_wait_t value,
        const time_point<Clock, Dur>& timeout) noexcept {
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
    bool waiter_do_wait_until(Pred pred, const time_point<Clock, Dur>& timeout) noexcept {
        platform_wait_t value;
        if (this->waiter_do_spin(pred, value,
            _INNER timed_backoff_spin_policy(timeout))) {
            return true;
        }
        return this->waiter_do_wait_until(pred, value, timeout);
    }

    template <typename T, typename Func, typename Rep, typename Period>
    bool waiter_do_wait_for_v(T old, Func func, const duration<Rep, Period>& rt) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin_v(old, _MSTL move(func), value)) {
            return true;
        }
        if (!rt.count()) {
            return false;
        }
        auto rtc = ceil<wait_clock_t::duration>(rt);
        return base_type::waiter_.do_wait_until(base_type::addr_, value, steady_clock::now() + rtc);
    }

    template <typename Pred, typename Rep, typename Period>
    bool waiter_do_wait_for(Pred pred, const duration<Rep, Period>& rt) noexcept {
        platform_wait_t value;
        if (base_type::waiter_do_spin(pred, value)) {
            return true;
        }
        if (!rt.count()) {
            return false;
        }
        auto rtc = ceil<wait_clock_t::duration>(rt);
        return this->waiter_do_wait_until(pred, value, steady_clock::now() + rtc);
    }
};

using enters_timed_wait = timed_waiter<true_type>;
using bare_timed_wait = timed_waiter<false_type>;

MSTL_END_INNER__

template <typename T, typename Func, typename Clock, typename Dur>
bool atomic_wait_address_until_v(const T* addr, T&& old,
    Func&& func, const time_point<Clock, Dur>& timeout) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until_v(old, func, timeout);
}

template <typename T, typename Pred, typename Clock, typename Dur>
bool atomic_wait_address_until(const T* addr, Pred pred,
    const time_point<Clock, Dur>& timeout) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until(pred, timeout);
}

template <typename Pred, typename Clock, typename Dur>
bool atomic_wait_address_until(const platform_wait_t* addr, Pred pred,
    const time_point<Clock, Dur>& timeout) noexcept {
    _INNER bare_timed_wait waiter{addr};
    return waiter.waiter_do_wait_until(pred, timeout);
}

template <typename T, typename Func, typename Rep, typename Period>
bool atomic_wait_address_for_v(const T* addr, T&& old, Func&& func,
    const duration<Rep, Period>& rt) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for_v(old, func, rt);
}

template <typename T, typename Pred, typename Rep, typename Period>
bool atomic_wait_address_for(const T* addr, Pred pred,
    const duration<Rep, Period>& rt) noexcept {
    _INNER enters_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for(pred, rt);
}

template <typename Pred, typename Rep, typename Period>
bool atomic_wait_address_for(const platform_wait_t* addr, Pred pred,
    const duration<Rep, Period>& rt) noexcept {
    _INNER bare_timed_wait waiter{addr};
    return waiter.waiter_do_wait_for(pred, rt);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_TIMED_WAIT_HPP__
