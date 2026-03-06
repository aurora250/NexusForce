#include <NeForce/core/async/mutex.hpp>
#include <NeForce/core/async/shared_mutex.hpp>
NEFORCE_BEGIN_NAMESPACE__

mutex::mutex() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::InitializeSRWLock(&mutex_);
#else
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutex_init(&mutex_, &attr);
    ::pthread_mutexattr_destroy(&attr);
#endif
}

mutex::~mutex() {
#ifdef NEFORCE_PLATFORM_LINUX
    ::pthread_mutex_destroy(&mutex_);
#endif
}

void mutex::lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::AcquireSRWLockExclusive(&mutex_);
#else
    ::pthread_mutex_lock(&mutex_);
#endif
}

void mutex::unlock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ReleaseSRWLockExclusive(&mutex_);
#else
    ::pthread_mutex_unlock(&mutex_);
#endif
}

bool mutex::try_lock() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::TryAcquireSRWLockExclusive(&mutex_) != 0;
#else
    return ::pthread_mutex_trylock(&mutex_) == 0;
#endif
}


recursive_mutex::recursive_mutex() {
#ifdef NEFORCE_PLATFORM_WINDOWS
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
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DeleteCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutex_destroy(&recursive_mutex_);
#endif
}

void recursive_mutex::lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::EnterCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutex_lock(&recursive_mutex_);
#endif
}

void recursive_mutex::unlock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::LeaveCriticalSection(&recursive_mutex_);
#else
    ::pthread_mutex_unlock(&recursive_mutex_);
#endif
}

bool recursive_mutex::try_lock() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::TryEnterCriticalSection(&recursive_mutex_) != 0;
#else
    return ::pthread_mutex_trylock(&recursive_mutex_) == 0;
#endif
}


shared_mutex::shared_mutex() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::InitializeSRWLock(&shared_mutex_);
#else
    ::pthread_rwlock_init(&shared_mutex_, nullptr);
#endif
}

shared_mutex::~shared_mutex() {
#ifdef NEFORCE_PLATFORM_LINUX
    ::pthread_rwlock_destroy(&shared_mutex_);
#endif
}

void shared_mutex::lock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::AcquireSRWLockExclusive(&shared_mutex_);
#else
    ::pthread_rwlock_wrlock(&shared_mutex_);
#endif
}

void shared_mutex::unlock() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ReleaseSRWLockExclusive(&shared_mutex_);
#else
    ::pthread_rwlock_unlock(&shared_mutex_);
#endif
}

bool shared_mutex::try_lock() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::TryAcquireSRWLockExclusive(&shared_mutex_) != 0;
#else
    return ::pthread_rwlock_trywrlock(&shared_mutex_) == 0;
#endif
}

void shared_mutex::lock_shared() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::AcquireSRWLockShared(&shared_mutex_);
#else
    ::pthread_rwlock_rdlock(&shared_mutex_);
#endif
}

void shared_mutex::unlock_shared() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::ReleaseSRWLockShared(&shared_mutex_);
#else
    ::pthread_rwlock_unlock(&shared_mutex_);
#endif
}

bool shared_mutex::try_lock_shared() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::TryAcquireSRWLockShared(&shared_mutex_) != 0;
#else
    return ::pthread_rwlock_tryrdlock(&shared_mutex_) == 0;
#endif
}

NEFORCE_END_NAMESPACE__
