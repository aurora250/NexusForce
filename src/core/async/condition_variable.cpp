#include <NeForce/core/async/condition_variable.hpp>
#include <NeForce/core/exception/terminate.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <errhandlingapi.h>
#    include <winerror.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <NeForce/core/exception/debug.hpp>
#    include <NeForce/core/algorithm/compare.hpp>
#    include <cerrno>
#    include <ctime>
#    include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_INNER__

condition_variable_base::condition_variable_base()
#ifdef NEFORCE_PLATFORM_WINDOWS
:
cond_(CONDITION_VARIABLE_INIT) {
    ::InitializeConditionVariable(&cond_);
#else
:
cond_(PTHREAD_COND_INITIALIZER) {
#endif
}

condition_variable_base::~condition_variable_base() {
#ifdef NEFORCE_PLATFORM_POSIX
    int err NEFORCE_UNUSED = ::pthread_cond_destroy(&cond_);
    NEFORCE_CONSTEXPR_ASSERT(err != EBUSY);
#endif
}

void condition_variable_base::wait(mutex& mtx) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::BOOL result = ::SleepConditionVariableSRW(&cond_, mtx.native_handle(), numeric_traits<::DWORD>::max(), 0);
    if (result == FALSE) {
        terminate();
    }
#elif defined(NEFORCE_PLATFORM_POSIX)
    int err NEFORCE_UNUSED = ::pthread_cond_wait(&cond_, mtx.native_handle());
    NEFORCE_CONSTEXPR_ASSERT(err == 0);
#endif
}

cv_status condition_variable_base::wait_until(mutex& mtx, const int64_t sec, const int64_t ns) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const milliseconds timeout_ms = relative_time(sec, ns, false);
    const ::BOOL result = ::SleepConditionVariableSRW(&cond_, mtx.native_handle(), timeout_ms.count(), 0);
    if (result == TRUE) {
        return cv_status::success;
    }
    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return cv_status::timeout;
    }
    terminate();
#elif defined(NEFORCE_PLATFORM_POSIX)
    const ::timespec ts{static_cast<::time_t>(sec), static_cast<long>(ns)};
    const int result = ::pthread_cond_timedwait(&cond_, mtx.native_handle(), &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
#endif
}

cv_status condition_variable_base::wait_until(mutex& mtx, const bool is_monotonic, const int64_t sec,
                                              const int64_t ns) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const milliseconds timeout_ms = relative_time(sec, ns, is_monotonic);
    const ::BOOL result = ::SleepConditionVariableSRW(&cond_, mtx.native_handle(), timeout_ms.count(), 0);
    if (result == TRUE) {
        return cv_status::success;
    }
    const ::DWORD err = ::GetLastError();
    if (err == ERROR_TIMEOUT) {
        return cv_status::timeout;
    }
    terminate();
#elif defined(NEFORCE_PLATFORM_FREEBSD)
    ::timespec ts{};
    if (is_monotonic) {
        ::timespec mono{};
        ::timespec real{};
        ::clock_gettime(CLOCK_MONOTONIC, &mono);
        ::clock_gettime(CLOCK_REALTIME, &real);
        int64_t delta =
                (sec - static_cast<int64_t>(mono.tv_sec)) * 1000000000LL + (ns - static_cast<int64_t>(mono.tv_nsec));
        delta = max<int64_t>(delta, 0);
        const int64_t total =
                static_cast<int64_t>(real.tv_sec) * 1000000000LL + static_cast<int64_t>(real.tv_nsec) + delta;
        ts.tv_sec = static_cast<::time_t>(total / 1000000000LL);
        ts.tv_nsec = static_cast<long>(total % 1000000000LL);
    } else {
        ts.tv_sec = static_cast<::time_t>(sec);
        ts.tv_nsec = static_cast<long>(ns);
    }
    const int result = ::pthread_cond_timedwait(&cond_, mtx.native_handle(), &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
#elif defined(NEFORCE_PLATFORM_POSIX)
    const ::timespec ts{static_cast<ssize_t>(sec), static_cast<ssize_t>(ns)};
    const ::clockid_t clock_id = is_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME;
    const int result = ::pthread_cond_clockwait(&cond_, mtx.native_handle(), clock_id, &ts);
    return (result == ETIMEDOUT) ? cv_status::timeout : cv_status::success;
#endif
}

void condition_variable_base::notify_one() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WakeConditionVariable(&cond_);
#elif defined(NEFORCE_PLATFORM_POSIX)
    int err NEFORCE_UNUSED = ::pthread_cond_signal(&cond_);
    NEFORCE_CONSTEXPR_ASSERT(err == 0);
#endif
}

void condition_variable_base::notify_all() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WakeAllConditionVariable(&cond_);
#elif defined(NEFORCE_PLATFORM_POSIX)
    int err NEFORCE_UNUSED = ::pthread_cond_broadcast(&cond_);
    NEFORCE_CONSTEXPR_ASSERT(err == 0);
#endif
}

NEFORCE_END_INNER__
NEFORCE_END_NAMESPACE__
