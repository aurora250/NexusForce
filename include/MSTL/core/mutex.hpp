#ifndef MSTL_MUTEX_HPP__
#define MSTL_MUTEX_HPP__
#include "type_traits.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#elif defined(MSTL_PLATFORM_LINUX__)
#include <pthread.h>
#endif
MSTL_BEGIN_NAMESPACE__

class MSTL_API mutex {
#ifdef MSTL_PLATFORM_WINDOWS__
    using mutex_type = ::CRITICAL_SECTION;
#else
    using mutex_type = ::pthread_mutex_t;
#endif

    mutable mutex_type mutex_;

public:
    mutex();
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;
    ~mutex();

    mutex_type* native_handle() noexcept { return &mutex_; }
    const mutex_type* native_handle() const noexcept { return &mutex_; }

    void lock();
    void unlock();
    bool try_lock() noexcept;
};


class MSTL_API shared_mutex {
#ifdef MSTL_PLATFORM_WINDOWS__
    using shared_mutex_type = ::SRWLOCK;
#else
    using shared_mutex_type = ::pthread_rwlock_t;
#endif

    mutable shared_mutex_type shared_mutex_;

public:
    shared_mutex();
    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;
    ~shared_mutex();

    shared_mutex_type* native_handle() noexcept { return &shared_mutex_; }
    const shared_mutex_type* native_handle() const noexcept { return &shared_mutex_; }

    void lock();
    void unlock();
    bool try_lock() noexcept;

    void lock_shared();
    void unlock_shared();
    bool try_lock_shared() noexcept;
};


template <typename Mutex>
class lock_guard {
public:
    using mutex_type = Mutex;

private:
    mutex_type& mutex_;

public:

    explicit lock_guard(mutex_type& m) : mutex_(m) {
        mutex_.lock();
    }

    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;

    ~lock_guard() {
        mutex_.unlock();
    }
};


MSTL_BEGIN_TAG__
struct defer_lock_tag {
    constexpr defer_lock_tag() noexcept = default;
};
struct try_lock_tag {
    constexpr try_lock_tag() noexcept = default;
};
MSTL_END_TAG__
MSTL_INLINE17 constexpr _MSTL_TAG defer_lock_tag defer_lock{};
MSTL_INLINE17 constexpr _MSTL_TAG try_lock_tag try_lock{};


template <typename Mutex>
class unique_lock {
public:
    using mutex_type = Mutex;

private:
    mutex_type* mutex_ = nullptr;
    bool owns_lock_ = false;

public:
    unique_lock() = default;

    explicit unique_lock(mutex_type& m)
    : mutex_(&m), owns_lock_(true) {
        mutex_->lock();
    }

    unique_lock(mutex_type& m, _MSTL_TAG defer_lock_tag) noexcept
    : mutex_(&m) {}

    unique_lock(mutex_type& m, _MSTL_TAG try_lock_tag) noexcept
    : mutex_(&m), owns_lock_(m.try_lock()) {}

    unique_lock(const unique_lock&) = delete;
    unique_lock& operator=(const unique_lock&) = delete;

    unique_lock(unique_lock&& other) noexcept
        : mutex_(other.mutex_), owns_lock_(other.owns_lock_) {
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
    }

    unique_lock& operator =(unique_lock&& other) noexcept {
        if (_MSTL addressof(other) == this) return *this;
        if (owns_lock_) mutex_->unlock();
        mutex_ = other.mutex_;
        owns_lock_ = other.owns_lock_;
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
        return *this;
    }

    ~unique_lock() {
        if (owns_lock_) mutex_->unlock();
    }

    MSTL_NODISCARD explicit operator bool() const noexcept { return owns_lock_; }
    MSTL_NODISCARD bool owns_lock() const noexcept { return owns_lock_; }
    MSTL_NODISCARD mutex_type* mutex() const noexcept { return mutex_; }

    void lock() {
        if (!mutex_) return;
        if (owns_lock_) return;
        mutex_->lock();
        owns_lock_ = true;
    }

    void unlock() {
        if (!mutex_) return;
        if (!owns_lock_) return;
        mutex_->unlock();
        owns_lock_ = false;
    }

    bool try_lock() noexcept {
        if (!mutex_) return false;
        if (owns_lock_) return true;
        owns_lock_ = mutex_->try_lock();
        return owns_lock_;
    }

    mutex_type* release() noexcept {
        mutex_type* ret = mutex_;
        mutex_ = nullptr;
        owns_lock_ = false;
        return ret;
    }
};


template <typename SharedMutex>
class shared_lock {
public:
    using mutex_type = SharedMutex;

private:
    mutex_type* mutex_ = nullptr;
    bool owns_lock_ = false;

public:
    shared_lock() = default;

    explicit shared_lock(mutex_type& m)
    : mutex_(&m), owns_lock_(true) {
        mutex_->lock_shared();
    }

    shared_lock(mutex_type& m, _MSTL_TAG defer_lock_tag) noexcept
    : mutex_(&m) {}

    shared_lock(mutex_type& m, _MSTL_TAG try_lock_tag) noexcept
    : mutex_(&m), owns_lock_(m.try_lock_shared()) {}

    shared_lock(const shared_lock&) = delete;
    shared_lock& operator=(const shared_lock&) = delete;

    shared_lock(shared_lock&& other) noexcept
        : mutex_(other.mutex_), owns_lock_(other.owns_lock_) {
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
    }

    shared_lock& operator=(shared_lock&& other) noexcept {
        if (_MSTL addressof(other) == this) return *this;
        if (owns_lock_) mutex_->unlock_shared();
        mutex_ = other.mutex_;
        owns_lock_ = other.owns_lock_;
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
        return *this;
    }

    ~shared_lock() {
        if (owns_lock_) mutex_->unlock_shared();
    }

    MSTL_NODISCARD explicit operator bool() const noexcept { return owns_lock_; }
    MSTL_NODISCARD bool owns_lock() const noexcept { return owns_lock_; }
    MSTL_NODISCARD mutex_type* mutex() const noexcept { return mutex_; }

    void lock() {
        if (!mutex_) return;
        if (owns_lock_) return;
        mutex_->lock_shared();
        owns_lock_ = true;
    }

    void unlock() {
        if (!mutex_) return;
        if (!owns_lock_) return;
        mutex_->unlock_shared();
        owns_lock_ = false;
    }

    bool try_lock() noexcept {
        if (!mutex_) return false;
        if (owns_lock_) return true;
        owns_lock_ = mutex_->try_lock_shared();
        return owns_lock_;
    }

    mutex_type* release() noexcept {
        mutex_type* ret = mutex_;
        mutex_ = nullptr;
        owns_lock_ = false;
        return ret;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_MUTEX_HPP__
