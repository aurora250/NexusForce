#ifndef MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#define MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#include "../async/mutex.hpp"
#include "../time/clocks.hpp"
#include "../exception/assertion.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include "../config/undef_cmacro.hpp"
#else
#include <bits/gthr.h>
#include <cerrno>
#endif
MSTL_BEGIN_NAMESPACE__

enum class cv_status {
    no_timeout, timeout
};


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
    condition_variable_base& operator=(const condition_variable_base&) = delete;
    ~condition_variable_base();

    native_handle_type* native_handle() noexcept { return &cond_; }

    void wait(mutex& mtx);
    cv_status wait_until(mutex& mtx, const ::timespec& abs);
    cv_status wait_until(mutex& mtx, const int clock, const ::timespec& abs);

    void notify_one() noexcept;
    void notify_all() noexcept;
};


class condition_variable {
public:
    using steady_clock = _MSTL_CHRONO steady_clock;
    using system_clock = _MSTL_CHRONO system_clock;
    using base_type = condition_variable_base;
    using clock_t = steady_clock;

private:
    base_type cond_;

    template <typename Dur>
    cv_status __wait_until_impl(unique_lock<mutex>& lock,
        const _MSTL_CHRONO time_point<steady_clock, Dur>& util) {
        auto sec = _MSTL_CHRONO time_point_cast<_MSTL_CHRONO seconds>(util);
        auto nanosec = _MSTL_CHRONO duration_cast<_MSTL_CHRONO nanoseconds>(util - sec);

        const ::timespec ts = {
            static_cast<std::time_t>(sec.time_since_epoch().count()),
            static_cast<long>(nanosec.count())
        };
        cond_.wait_until(*lock.mutex(), 1, ts);
        return (steady_clock::now() < util ? cv_status::no_timeout : cv_status::timeout);
    }

    template <typename Dur>
    cv_status __wait_until_impl(unique_lock<mutex>& lock,
        const _MSTL_CHRONO time_point<system_clock, Dur>& util) {
        auto sec = _MSTL_CHRONO time_point_cast<_MSTL_CHRONO seconds>(util);
        auto nanosec = _MSTL_CHRONO duration_cast<_MSTL_CHRONO nanoseconds>(util - sec);

        const ::timespec ts = {
            static_cast<std::time_t>(sec.time_since_epoch().count()),
            static_cast<long>(nanosec.count())
        };
        cond_.wait_until(*lock.mutex(), ts);
        return (system_clock::now() < util ? cv_status::no_timeout : cv_status::timeout);
    }

public:
    using native_handle_type = base_type::native_handle_type;

    condition_variable() noexcept = default;
    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;
    ~condition_variable() noexcept = default;

    native_handle_type* native_handle() { return cond_.native_handle(); }

    void notify_one() noexcept { cond_.notify_one(); }
    void notify_all() noexcept { cond_.notify_all(); }

    void wait(unique_lock<mutex>& lock) {
        cond_.wait(*lock.mutex());
    }

    template <typename Pred>
    void wait(unique_lock<mutex>& lock, Pred pred) {
        while (!pred()) wait(lock);
    }

    template <typename Dur>
    cv_status wait_until(unique_lock<mutex>& lock,
        const _MSTL_CHRONO time_point<steady_clock, Dur>& util) {
        return __wait_until_impl(lock, util);
    }

    template <typename Dur>
    cv_status wait_until(unique_lock<mutex>& lock,
        const _MSTL_CHRONO time_point<system_clock, Dur>& util) {
        return __wait_until_impl(lock, util);
    }

    template <typename Clock, typename Dur>
    cv_status wait_until(unique_lock<mutex>& lock,
        const _MSTL_CHRONO time_point<Clock, Dur>& util) {
        const typename Clock::time_point entry = Clock::now();
        const auto atime = clock_t::now() + _MSTL_CHRONO ceil<clock_t::duration>(util - entry);

        if (__wait_until_impl(lock, atime) == cv_status::no_timeout) {
            return cv_status::no_timeout;
        }
        if (Clock::now() < util) {
            return cv_status::no_timeout;
        }
        return cv_status::timeout;
    }

    template <typename Clock, typename Dur, typename Pred>
    bool wait_until(unique_lock<mutex>& lock,
        const _MSTL_CHRONO time_point<Clock, Dur>& util, Pred pred) {
        while (!pred()) {
            if (wait_until(lock, util) == cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }

    template <typename Rep, typename Period>
    cv_status wait_for(unique_lock<mutex>& lock,
        const _MSTL_CHRONO duration<Rep, Period>& rest) {
        const auto atime = steady_clock::now() + _MSTL_CHRONO ceil<steady_clock::duration>(rest);
        return wait_until(lock, atime);
    }

    template <typename Rep, typename Period, typename Pred>
    bool wait_for(unique_lock<mutex>& lock,
        const _MSTL_CHRONO duration<Rep, Period>& rest, Pred pred) {
        const auto atime = steady_clock::now() + _MSTL_CHRONO ceil<steady_clock::duration>(rest);
        return wait_until(lock, atime, _MSTL move(pred));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
