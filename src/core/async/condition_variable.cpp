#include <MSTL/core/async/condition_variable.hpp>
#include <MSTL/core/exception/terminate.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <bits/gthr.h>
#include <cerrno>
#endif
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

#ifdef MSTL_PLATFORM_WINDOWS__
static ::DWORD timespec_to_relative_ms(const timespec& abs, const bool is_monotonic) {
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


#ifdef MSTL_PLATFORM_WINDOWS__
condition_variable_base::condition_variable_base() noexcept {
    ::InitializeConditionVariable(&cond_);
}
#else
condition_variable_base::condition_variable_base() noexcept
: cond_(__GTHREAD_COND_INIT) {}
#endif

condition_variable_base::~condition_variable_base() {
#ifdef MSTL_PLATFORM_LINUX__
    int err MSTL_UNUSED = ::__gthread_cond_destroy(&cond_);
    MSTL_CONSTEXPR_ASSERT(err != EBUSY);
#endif
}

void condition_variable_base::wait(mutex& mtx) {
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

cv_status condition_variable_base::wait_until(mutex& mtx, const ::timespec& abs) {
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

cv_status condition_variable_base::wait_until(mutex& mtx, const int clock, const ::timespec& abs) {
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

void condition_variable_base::notify_one() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WakeConditionVariable(&cond_);
#else
    int err MSTL_UNUSED = ::__gthread_cond_signal(&cond_);
    MSTL_CONSTEXPR_ASSERT(err == 0);
#endif
}

void condition_variable_base::notify_all() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::WakeAllConditionVariable(&cond_);
#else
    int err MSTL_UNUSED = ::__gthread_cond_broadcast(&cond_);
    MSTL_CONSTEXPR_ASSERT(err == 0);
#endif
}

MSTL_END_INNER__
MSTL_END_NAMESPACE__
