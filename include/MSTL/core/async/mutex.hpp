#ifndef MSTL_CORE_ASYNC_MUTEX_HPP__
#define MSTL_CORE_ASYNC_MUTEX_HPP__
#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/tags.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include "../config/undef_cmacro.hpp"
#elif defined(MSTL_PLATFORM_LINUX__)
#include <pthread.h>
#endif
MSTL_BEGIN_NAMESPACE__

class MSTL_API mutex {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::SRWLOCK;
#else
    using native_handle_type = ::pthread_mutex_t;
#endif

private:
    mutable native_handle_type mutex_;

public:
    mutex();
    ~mutex();

    mutex(const mutex&) = delete;
    mutex& operator =(const mutex&) = delete;
    mutex(mutex&&) noexcept = default;
    mutex& operator =(mutex&&) noexcept = default;

    native_handle_type* native_handle() noexcept { return &mutex_; }
    const native_handle_type* native_handle() const noexcept { return &mutex_; }

    void lock();
    void unlock();
    bool try_lock() noexcept;
};


class MSTL_API recursive_mutex {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using native_handle_type = ::CRITICAL_SECTION;
#else
    using native_handle_type = ::pthread_mutex_t;
#endif

private:
    mutable native_handle_type recursive_mutex_;

public:
    recursive_mutex();
    recursive_mutex(const recursive_mutex&) = delete;
    recursive_mutex& operator =(const recursive_mutex&) = delete;
    ~recursive_mutex();

    native_handle_type* native_handle() noexcept { return &recursive_mutex_; }
    const native_handle_type* native_handle() const noexcept { return &recursive_mutex_; }
    void lock();
    void unlock();
    bool try_lock() noexcept;
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
    lock_guard& operator =(const lock_guard&) = delete;

    ~lock_guard() {
        mutex_.unlock();
    }
};


struct defer_lock_tag {
    constexpr defer_lock_tag() noexcept = default;
};
MSTL_INLINE17 constexpr defer_lock_tag defer_lock{};

struct try_lock_tag {
    constexpr try_lock_tag() noexcept = default;
};
MSTL_INLINE17 constexpr try_lock_tag try_lock{};


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

    unique_lock(mutex_type& m, defer_lock_tag) noexcept
    : mutex_(&m) {}

    unique_lock(mutex_type& m, try_lock_tag) noexcept
    : mutex_(&m), owns_lock_(m.try_lock()) {}

    unique_lock(const unique_lock&) = delete;
    unique_lock& operator =(const unique_lock&) = delete;

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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_MUTEX_HPP__
