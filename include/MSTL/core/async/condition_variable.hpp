#ifndef MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#define MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#include "../async/mutex.hpp"
#include "../time/clocks.hpp"
#include "../config/assertion.hpp"
#include "../config/terminate.hpp"
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


class condition_variable_base {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::CONDITION_VARIABLE;
#else
    using native_handle_type = ::pthread_cond_t;
#endif

private:
    native_handle_type cond_;

#ifdef MSTL_PLATFORM_WINDOWS__
    static ::DWORD timespec_to_relative_ms(const timespec& abs, bool is_monotonic) {
        using namespace chrono;
        const nanoseconds abs_ns {
            static_cast<int64_t>(abs.tv_sec) * 1'000'000'000LL + abs.tv_nsec
        };
        time_point<system_clock, nanoseconds> abs_sys_time;
        time_point<steady_clock, nanoseconds> abs_steady_time;

        if (is_monotonic) {
            abs_steady_time = time_point<steady_clock, nanoseconds>{ abs_ns };
        } else {
            abs_sys_time = system_clock::from_time_t(0) + abs_ns;
        }

        nanoseconds now_ns;
        if (is_monotonic) {
            now_ns = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch();
        } else {
            now_ns = time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch();
        }

        int64_t diff_ns;
        if (is_monotonic) {
            diff_ns = (abs_steady_time.time_since_epoch() - now_ns).count();
        } else {
            diff_ns = (abs_sys_time.time_since_epoch() - now_ns).count();
        }
        if (diff_ns <= 0) return 0;

        const uint64_t diff_ms = static_cast<uint64_t>(diff_ns) / 1'000'000ULL;
        constexpr uint64_t MAX_DWORD = numeric_limits<::DWORD>::max();
        if (diff_ms > MAX_DWORD - 1) {
            return static_cast<::DWORD>(MAX_DWORD - 1);
        }
        return static_cast<::DWORD>(diff_ms);
    }
#endif

public:
#ifdef MSTL_PLATFORM_WINDOWS__
    condition_variable_base() noexcept {
        ::InitializeConditionVariable(&cond_);
    }
#else
    condition_variable_base() noexcept
    : cond_(__GTHREAD_COND_INIT) {}
#endif

    condition_variable_base(const condition_variable_base&) = delete;
    condition_variable_base& operator=(const condition_variable_base&) = delete;

    ~condition_variable_base() {
#ifdef MSTL_PLATFORM_LINUX__
        int err MSTL_UNUSED = ::__gthread_cond_destroy(&cond_);
        MSTL_CONSTEXPR_ASSERT(err != EBUSY);
#endif
    }

    native_handle_type* native_handle() noexcept { return &cond_; }

    void wait(mutex& mtx) {
#ifdef MSTL_PLATFORM_WINDOWS__
        const ::BOOL result = ::SleepConditionVariableSRW(
            &cond_, mtx.native_handle(), INFINITE, 0);
        if (!result) {
            _MSTL terminate();
        }
#else
        int err MSTL_UNUSED = ::__gthread_cond_wait(&cond_, mtx.native_handle());
        MSTL_CONSTEXPR_ASSERT(err == 0);
#endif
    }

    cv_status wait_until(mutex& mtx, const ::timespec& abs) {
#ifdef MSTL_PLATFORM_WINDOWS__
        const ::DWORD timeout_ms = timespec_to_relative_ms(abs, false);
        ::BOOL result = ::SleepConditionVariableSRW(
            &cond_, mtx.native_handle(), timeout_ms, 0);
        if (result) {
            return cv_status::no_timeout;
        }
        const::DWORD err = ::GetLastError();
        if (err == ERROR_TIMEOUT) {
            return cv_status::timeout;
        }
        _MSTL terminate();
#else
        const int result = ::__gthread_cond_timedwait(&cond_, mtx.native_handle(), &abs);
        return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::no_timeout;
#endif
    }

    cv_status wait_until(mutex& mtx, const int clock, const ::timespec& abs) {
#ifdef MSTL_PLATFORM_WINDOWS__
        const bool is_monotonic = (clock == 1); // CLOCK_MONOTONIC = 1
        const ::DWORD timeout_ms = timespec_to_relative_ms(abs, is_monotonic);
        ::BOOL result = ::SleepConditionVariableSRW(
            &cond_, mtx.native_handle(), timeout_ms, 0);
        if (result) {
            return cv_status::no_timeout;
        }
        const ::DWORD err = ::GetLastError();
        if (err == ERROR_TIMEOUT) {
            return cv_status::timeout;
        }
        _MSTL terminate();
        return cv_status::timeout;
#else
        const int result = ::pthread_cond_clockwait(&cond_, mtx.native_handle(), clock, &abs);
        return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::no_timeout;
#endif
    }

    void notify_one() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::WakeConditionVariable(&cond_);
#else
        int err MSTL_UNUSED = ::__gthread_cond_signal(&cond_);
        MSTL_CONSTEXPR_ASSERT(err == 0);
#endif
    }

    void notify_all() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::WakeAllConditionVariable(&cond_);
#else
        int err MSTL_UNUSED = ::__gthread_cond_broadcast(&cond_);
        MSTL_CONSTEXPR_ASSERT(err == 0);
#endif
    }
};


class condition_variable {
public:
    using steady_clock = chrono::steady_clock;
    using system_clock = chrono::system_clock;
    using base_type = condition_variable_base;
    using clock_t = steady_clock;

private:
    base_type cond_;

    template <typename Dur>
    cv_status __wait_until_impl(unique_lock<mutex>& lock,
        const chrono::time_point<steady_clock, Dur>& util) {
        auto sec = chrono::time_point_cast<chrono::seconds>(util);
        auto nanosec = chrono::duration_cast<chrono::nanoseconds>(util - sec);

        const ::timespec ts = {
            static_cast<std::time_t>(sec.time_since_epoch().count()),
            static_cast<long>(nanosec.count())
        };
        cond_.wait_until(*lock.mutex(), 1, ts);
        return (steady_clock::now() < util ? cv_status::no_timeout : cv_status::timeout);
    }

    template <typename Dur>
    cv_status __wait_until_impl(unique_lock<mutex>& lock,
        const chrono::time_point<system_clock, Dur>& util) {
        auto sec = chrono::time_point_cast<chrono::seconds>(util);
        auto nanosec = chrono::duration_cast<chrono::nanoseconds>(util - sec);

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
        const chrono::time_point<steady_clock, Dur>& util) {
        return __wait_until_impl(lock, util);
    }

    template <typename Dur>
    cv_status wait_until(unique_lock<mutex>& lock,
        const chrono::time_point<system_clock, Dur>& util) {
        return __wait_until_impl(lock, util);
    }

    template <typename Clock, typename Dur>
    cv_status wait_until(unique_lock<mutex>& lock,
        const chrono::time_point<Clock, Dur>& util) {
        const typename Clock::time_point entry = Clock::now();
        const auto atime = clock_t::now() + chrono::ceil<clock_t::duration>(util - entry);

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
        const chrono::time_point<Clock, Dur>& util, Pred pred) {
        while (!pred()) {
            if (wait_until(lock, util) == cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }

    template <typename Rep, typename Period>
    cv_status wait_for(unique_lock<mutex>& lock,
        const chrono::duration<Rep, Period>& rest) {
        const auto atime = steady_clock::now() + chrono::ceil<steady_clock::duration>(rest);
        return wait_until(lock, atime);
    }

    template <typename Rep, typename Period, typename Pred>
    bool wait_for(unique_lock<mutex>& lock,
        const chrono::duration<Rep, Period>& rest, Pred pred) {
        const auto atime = steady_clock::now() + chrono::ceil<steady_clock::duration>(rest);
        return wait_until(lock, atime, _MSTL move(pred));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_CONDITION_VARIABLE_HPP__
