#include <MSTL/core/async/mutex.hpp>
MSTL_BEGIN_NAMESPACE__

mutex::mutex() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::InitializeSRWLock(&mutex_);
#else
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutex_init(&mutex_, &attr);
    ::pthread_mutexattr_destroy(&attr);
#endif
}

mutex::~mutex() {
#ifdef MSTL_PLATFORM_LINUX__
    ::pthread_mutex_destroy(&mutex_);
#endif
}

void mutex::lock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::AcquireSRWLockExclusive(&mutex_);
#else
    ::pthread_mutex_lock(&mutex_);
#endif
}

void mutex::unlock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::ReleaseSRWLockExclusive(&mutex_);
#else
    ::pthread_mutex_unlock(&mutex_);
#endif
}

bool mutex::try_lock() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::TryAcquireSRWLockExclusive(&mutex_) != 0;
#else
    return ::pthread_mutex_trylock(&mutex_) == 0;
#endif
}


recursive_mutex::recursive_mutex() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::InitializeCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&recursive_mutex_, &attr);
    ::pthread_mutexattr_destroy(&attr);
#endif
}

recursive_mutex::~recursive_mutex() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DeleteCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutex_destroy(&recursive_mutex_);
#endif
}

void recursive_mutex::lock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::EnterCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutex_lock(&recursive_mutex_);
#endif
}

void recursive_mutex::unlock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::LeaveCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutex_unlock(&recursive_mutex_);
#endif
}

bool recursive_mutex::try_lock() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::TryEnterCriticalSection(&recursive_mutex_) != 0;
#else
    return ::pthread_mutex_trylock(&recursive_mutex_) == 0;
#endif
}


shared_mutex::shared_mutex() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::InitializeSRWLock(&shared_mutex_);
#else
    ::pthread_rwlock_init(&shared_mutex_, nullptr);
#endif
}

shared_mutex::~shared_mutex() {
#ifdef MSTL_PLATFORM_LINUX__
    ::pthread_rwlock_destroy(&shared_mutex_);
#endif
}

void shared_mutex::lock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::AcquireSRWLockExclusive(&shared_mutex_);
#else
    ::pthread_rwlock_wrlock(&shared_mutex_);
#endif
}

void shared_mutex::unlock() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::ReleaseSRWLockExclusive(&shared_mutex_);
#else
    ::pthread_rwlock_unlock(&shared_mutex_);
#endif
}

bool shared_mutex::try_lock() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::TryAcquireSRWLockExclusive(&shared_mutex_) != 0;
#else
    return ::pthread_rwlock_trywrlock(&shared_mutex_) == 0;
#endif
}

void shared_mutex::lock_shared() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::AcquireSRWLockShared(&shared_mutex_);
#else
    ::pthread_rwlock_rdlock(&shared_mutex_);
#endif
}

void shared_mutex::unlock_shared() {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::ReleaseSRWLockShared(&shared_mutex_);
#else
    ::pthread_rwlock_unlock(&shared_mutex_);
#endif
}

bool shared_mutex::try_lock_shared() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::TryAcquireSRWLockShared(&shared_mutex_) != 0;
#else
    return ::pthread_rwlock_tryrdlock(&shared_mutex_) == 0;
#endif
}

MSTL_END_NAMESPACE__
