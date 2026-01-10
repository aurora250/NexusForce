#ifndef MSTL_CORE_ASYNC_SHARED_MUTEX_HPP__
#define MSTL_CORE_ASYNC_SHARED_MUTEX_HPP__
#include "mutex.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API shared_mutex {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::SRWLOCK;
#else
    using native_handle_type = ::pthread_rwlock_t;
#endif

private:
    mutable native_handle_type shared_mutex_;

public:
    shared_mutex();
    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;
    ~shared_mutex();

    native_handle_type* native_handle() noexcept { return &shared_mutex_; }
    const native_handle_type* native_handle() const noexcept { return &shared_mutex_; }

    void lock();
    void unlock();
    bool try_lock() noexcept;

    void lock_shared();
    void unlock_shared();
    bool try_lock_shared() noexcept;
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

    shared_lock(mutex_type& m, defer_lock_tag) noexcept
    : mutex_(&m) {}

    shared_lock(mutex_type& m, try_lock_tag) noexcept
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
#endif // MSTL_CORE_ASYNC_SHARED_MUTEX_HPP__
