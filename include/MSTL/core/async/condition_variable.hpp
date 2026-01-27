#ifndef MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#define MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#include "MSTL/core/async/mutex.hpp"
#include "MSTL/core/time/clocks.hpp"
MSTL_BEGIN_NAMESPACE__

enum class cv_status {
    no_timeout, timeout
};


MSTL_BEGIN_INNER__

class MSTL_API condition_variable_base {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::CONDITION_VARIABLE;
#else
    using native_handle_type = ::pthread_cond_t;
#endif

private:
    native_handle_type cond_;

public:
    condition_variable_base() noexcept;
    condition_variable_base(const condition_variable_base&) = delete;
    condition_variable_base& operator =(const condition_variable_base&) = delete;
    ~condition_variable_base();

    native_handle_type* native_handle() noexcept { return &cond_; }

    void wait(mutex& mtx);
    cv_status wait_until(mutex& mtx, int64_t sec, int64_t ns);
    cv_status wait_until(mutex& mtx, int clock, int64_t sec, int64_t ns);

    void notify_one() noexcept;
    void notify_all() noexcept;
};

MSTL_END_INNER__


class condition_variable {
public:
    using steady_clock = _MSTL steady_clock;
    using system_clock = _MSTL system_clock;
    using base_type = _INNER condition_variable_base;
    using clock_t = _MSTL steady_clock;

private:
    base_type cond_;

    template <typename Dur>
    cv_status __wait_until_impl(smart_lock<mutex>& lock,
        const time_point<steady_clock, Dur>& util) {
        auto s = util.to_sec();
        const nanoseconds ns(util - s);
        cond_.wait_until(*lock.mutex(), 1, s.since_epoch().count(), ns.count());
        return steady_clock::now() < util ?
            cv_status::no_timeout :
            cv_status::timeout;
    }

    template <typename Dur>
    cv_status __wait_until_impl(smart_lock<mutex>& lock,
        const time_point<system_clock, Dur>& util) {
        auto sec = util.to_sec();
        const nanoseconds nanosec(util - sec);
        cond_.wait_until(*lock.mutex(), sec.since_epoch().count(), nanosec.count());
        return system_clock::now() < util ?
            cv_status::no_timeout :
            cv_status::timeout;
    }

public:
    using native_handle_type = base_type::native_handle_type;

    condition_variable() noexcept = default;
    condition_variable(const condition_variable&) = delete;
    condition_variable& operator =(const condition_variable&) = delete;
    ~condition_variable() noexcept = default;

    native_handle_type* native_handle() { return cond_.native_handle(); }

    void notify_one() noexcept { cond_.notify_one(); }
    void notify_all() noexcept { cond_.notify_all(); }

    void wait(smart_lock<mutex>& lock) {
        cond_.wait(*lock.mutex());
    }

    template <typename Pred>
    void wait(smart_lock<mutex>& lock, Pred pred) {
        while (!pred()) wait(lock);
    }

    template <typename Dur>
    cv_status wait_until(smart_lock<mutex>& lock,
        const time_point<steady_clock, Dur>& util) {
        return this->__wait_until_impl(lock, util);
    }

    template <typename Dur>
    cv_status wait_until(smart_lock<mutex>& lock,
        const time_point<system_clock, Dur>& util) {
        return this->__wait_until_impl(lock, util);
    }

    template <typename Clock, typename Dur>
    cv_status wait_until(smart_lock<mutex>& lock,
        const time_point<Clock, Dur>& util) {
        const typename Clock::time_point entry = Clock::now();
        const auto atime = clock_t::now() + ceil<clock_t::duration>(util - entry);

        if (this->__wait_until_impl(lock, atime) == cv_status::no_timeout) {
            return cv_status::no_timeout;
        }
        if (Clock::now() < util) {
            return cv_status::no_timeout;
        }
        return cv_status::timeout;
    }

    template <typename Clock, typename Dur, typename Pred>
    bool wait_until(smart_lock<mutex>& lock,
        const time_point<Clock, Dur>& util, Pred pred) {
        while (!pred()) {
            if (this->wait_until(lock, util) == cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }

    template <typename Rep, typename Period>
    cv_status wait_for(smart_lock<mutex>& lock,
        const duration<Rep, Period>& rest) {
        const auto atime = steady_clock::now() + ceil<steady_clock::duration>(rest);
        return this->wait_until(lock, atime);
    }

    template <typename Rep, typename Period, typename Pred>
    bool wait_for(smart_lock<mutex>& lock,
        const duration<Rep, Period>& rest, Pred pred) {
        const auto atime = steady_clock::now() + ceil<steady_clock::duration>(rest);
        return this->wait_until(lock, atime, _MSTL move(pred));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
