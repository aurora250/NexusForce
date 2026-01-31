#include <MSTL/core/async/condition_variable.hpp>
#include <MSTL/core/exception/terminate.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <bits/gthr.h>
#include <cerrno>
#endif
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

condition_variable_base::condition_variable_base()
#ifdef MSTL_PLATFORM_WINDOWS__
{ ::InitializeConditionVariable(&cond_);
#else
: cond_(__GTHREAD_COND_INIT) {
#endif
}

condition_variable_base::~condition_variable_base() {
#ifdef MSTL_PLATFORM_LINUX__
    int err MSTL_UNUSED = ::__gthread_cond_destroy(&cond_);
    MSTL_CONSTEXPR_ASSERT(err != EBUSY);
#endif
}

void condition_variable_base::wait(mutex& mtx) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::BOOL result = ::SleepConditionVariableSRW(
        &cond_, mtx.native_handle(), numeric_traits<::DWORD>::max(), 0);
    if (!result) {
        _MSTL terminate();
    }
#else
    int err MSTL_UNUSED = ::__gthread_cond_wait(&cond_, mtx.native_handle());
    MSTL_CONSTEXPR_ASSERT(err == 0);
#endif
}

cv_status condition_variable_base::wait_until(
    mutex& mtx, const int64_t sec, const int64_t ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const milliseconds timeout_ms = relative_time(sec, ns, false);
    const ::BOOL result = ::SleepConditionVariableSRW(
        &cond_, mtx.native_handle(), timeout_ms.count(), 0);
    if (result) {
        return cv_status::success;
    }
    const::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return cv_status::timeout;
    }
    _MSTL terminate();
#else
    const ::timespec ts { sec, ns };
    const int result = ::__gthread_cond_timedwait(&cond_, mtx.native_handle(), &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
#endif
}

cv_status condition_variable_base::wait_until(
    mutex& mtx, const bool is_monotonic, const int64_t sec, const int64_t ns) {
#ifdef MSTL_PLATFORM_WINDOWS__
    const milliseconds timeout_ms = relative_time(sec, ns, is_monotonic);
    ::BOOL result = ::SleepConditionVariableSRW(
        &cond_, mtx.native_handle(), timeout_ms.count(), 0);
    if (result) {
        return cv_status::success;
    }
    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return cv_status::timeout;
    }
    _MSTL terminate();
#else
    const ::timespec ts { sec, ns };
    const int result = ::pthread_cond_clockwait(
        &cond_, mtx.native_handle(), static_cast<int>(is_monotonic), &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
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
