#include <NeForce/core/async/semaphore.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <WinBase.h>
#    include <handleapi.h>
#    include <windef.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
#ifdef NEFORCE_PLATFORM_LINUX
    template <typename Rep, typename Period> struct timespec relative_to_timespec(const nanoseconds relative) noexcept {
        struct ::timespec now{};
        ::clock_gettime(CLOCK_REALTIME, &now);

        auto total_ns = relative.count();
        if (total_ns < 0) {
            total_ns = 0;
        }

        constexpr long kNsPerSec = 1'000'000'000L;
        now.tv_sec += static_cast<::time_t>(total_ns / kNsPerSec);
        now.tv_nsec += static_cast<long>(total_ns % kNsPerSec);

        if (now.tv_nsec >= kNsPerSec) {
            now.tv_sec += 1;
            now.tv_nsec -= kNsPerSec;
        }
        return now;
    }
#endif
} // namespace


#ifdef NEFORCE_PLATFORM_WINDOWS
bool semaphore::try_acquire_for_impl(const milliseconds timeout) noexcept {
    auto ms = timeout.count();
    if (ms < 0) {
        ms = 0;
    }
    const ::DWORD result = ::WaitForSingleObjectEx(handle_, static_cast<::DWORD>(ms), FALSE);
    return result == WAIT_OBJECT_0;
}
#else
bool semaphore::try_acquire_for_impl(const nanoseconds timeout) noexcept {
    struct ::timespec ts = relative_to_timespec(timeout);
    int ret;
    do {
        ret = ::sem_timedwait(&sem_, &ts);
    } while (ret == -1 && errno == EINTR);
    return ret == 0;
}
#endif

semaphore::semaphore(long initial, long maximum) {
    NEFORCE_CONSTEXPR_ASSERT(initial >= 0);
    NEFORCE_CONSTEXPR_ASSERT(maximum >= initial);

#ifdef NEFORCE_PLATFORM_WINDOWS
    handle_ = ::CreateSemaphoreW(nullptr, initial, maximum, nullptr);
    if (handle_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(system_exception("CreateSemaphoreW failed"));
    }
#else
    NEFORCE_IGNORE maximum;
    int ret = ::sem_init(&sem_, 0, static_cast<unsigned>(initial));
    if (ret == 0) {
        NEFORCE_THROW_EXCEPTION(system_exception("sem_init failed"));
    }
#endif
}

semaphore::~semaphore() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (handle_) {
        ::CloseHandle(handle_);
        handle_ = nullptr;
    }
#else
    ::sem_destroy(&sem_);
#endif
}

void semaphore::acquire() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::WaitForSingleObjectEx(handle_, INFINITE, FALSE);
#else
    int ret;
    do {
        ret = ::sem_wait(&sem_);
    } while (ret == -1 && errno == EINTR);
#endif
}

bool semaphore::try_acquire() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD result = ::WaitForSingleObjectEx(handle_, 0, FALSE);
    return result == WAIT_OBJECT_0;
#else
    int ret;
    do {
        ret = ::sem_trywait(&sem_);
    } while (ret == -1 && errno == EINTR);
    return ret == 0;
#endif
}

void semaphore::release(long update) {
    if (update > 0) {
        NEFORCE_THROW_EXCEPTION(system_exception("update failed"));
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ReleaseSemaphore(handle_, update, nullptr);
#else
    for (long i = 0; i < update; ++i) {
        ::sem_post(&sem_);
    }
#endif
}

int semaphore::value() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::DWORD result = ::WaitForSingleObjectEx(const_cast<::HANDLE>(handle_), 0, FALSE);
    if (result == WAIT_OBJECT_0) {
        ::ReleaseSemaphore(const_cast<::HANDLE>(handle_), 1, nullptr);
        return 1;
    }
    return 0;
#else
    int val = 0;
    ::sem_getvalue(const_cast<::sem_t*>(&sem_), &val);
    return val;
#endif
}

NEFORCE_END_NAMESPACE__
