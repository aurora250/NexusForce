#include <NeForce/core/async/condition_variable.hpp>
#include <NeForce/core/exception/terminate.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#include <errhandlingapi.h>
#include <winerror.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <bits/gthr.h>
#include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

condition_variable_base::condition_variable_base()
#ifdef NEFORCE_PLATFORM_WINDOWS
{ ::InitializeConditionVariable(&cond_);
#else
: cond_(__GTHREAD_COND_INIT) {
#endif
}

condition_variable_base::~condition_variable_base() {
#ifdef NEFORCE_PLATFORM_LINUX
    int err NEFORCE_UNUSED = ::__gthread_cond_destroy(&cond_);
    NEFORCE_CONSTEXPR_ASSERT(err != EBUSY);
#endif
}

void condition_variable_base::wait(mutex& mtx) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::BOOL result = ::SleepConditionVariableSRW(
        &cond_, mtx.native_handle(), numeric_traits<::DWORD>::max(), 0);
    if (!result) {
        _NEFORCE terminate();
    }
#else
    int err NEFORCE_UNUSED = ::__gthread_cond_wait(&cond_, mtx.native_handle());
    NEFORCE_CONSTEXPR_ASSERT(err == 0);
#endif
}

cv_status condition_variable_base::wait_until(
    mutex& mtx, const int64_t sec, const int64_t ns) {
#ifdef NEFORCE_PLATFORM_WINDOWS
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
    _NEFORCE terminate();
#else
    const ::timespec ts { static_cast<ssize_t>(sec), static_cast<ssize_t>(ns) };
    const int result = ::__gthread_cond_timedwait(&cond_, mtx.native_handle(), &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
#endif
}

cv_status condition_variable_base::wait_until(
    mutex& mtx, const bool is_monotonic, const int64_t sec, const int64_t ns) {
#ifdef NEFORCE_PLATFORM_WINDOWS
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
    _NEFORCE terminate();
#else
    const ::timespec ts { static_cast<ssize_t>(sec), static_cast<ssize_t>(ns) };
    const int result = ::pthread_cond_clockwait(
        &cond_, mtx.native_handle(), static_cast<int>(is_monotonic), &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
#endif
}

void condition_variable_base::notify_one() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WakeConditionVariable(&cond_);
#else
    int err NEFORCE_UNUSED = ::__gthread_cond_signal(&cond_);
    NEFORCE_CONSTEXPR_ASSERT(err == 0);
#endif
}

void condition_variable_base::notify_all() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WakeAllConditionVariable(&cond_);
#else
    int err NEFORCE_UNUSED = ::__gthread_cond_broadcast(&cond_);
    NEFORCE_CONSTEXPR_ASSERT(err == 0);
#endif
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
