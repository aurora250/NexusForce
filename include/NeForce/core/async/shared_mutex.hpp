#ifndef NEFORCE_CORE_ASYNC_SHARED_MUTEX_HPP__
#define NEFORCE_CORE_ASYNC_SHARED_MUTEX_HPP__

/**
 * @file shared_mutex.hpp
 * @brief 共享互斥锁支持
 *
 * 此文件提供了共享互斥锁和共享锁的实现。
 */

#include "mutex.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Mutex 互斥锁
 * @brief 互斥锁类型和工具
 * @{
 */

/**
 * @class shared_mutex
 * @brief 共享互斥锁类
 *
 * 提供读-写锁语义的互斥锁，允许多个线程同时进行读操作，
 * 但写操作需要独占访问。
 */
class NEFORCE_API shared_mutex {
public:
    /**
     * @brief 共享互斥锁的系统句柄类型
     */
    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::SRWLOCK;
#else
        ::pthread_rwlock_t;
#endif

private:
    mutable native_handle_type shared_mutex_;  ///< 共享互斥锁系统句柄

public:
    /**
     * @brief 构造函数
     */
    shared_mutex();

    /**
     * @brief 析构函数
     */
    ~shared_mutex();

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator =(const shared_mutex&) = delete;

    shared_mutex(shared_mutex&&) = default;
    shared_mutex& operator =(shared_mutex&&) = default;

    /**
     * @brief 获取原生句柄
     * @return 指向互斥锁原生句柄的指针
     */
    native_handle_type* native_handle() noexcept {
        return &shared_mutex_;
    }

    /**
     * @brief 获取常量原生句柄
     * @return 指向互斥锁原生句柄的常量指针
     */
    const native_handle_type* native_handle() const noexcept {
        return &shared_mutex_;
    }

    /**
     * @brief 获取写锁
     *
     * 阻塞当前线程，直到获得独占访问权。
     * 在获取写锁期间，其他线程不能获取读锁或写锁。
     */
    void lock();

    /**
     * @brief 释放写锁
     *
     * 释放独占访问权，允许其他线程获取读锁或写锁。
     */
    void unlock();

    /**
     * @brief 尝试获取写锁
     * @return 如果成功获得写锁则返回true，否则返回false
     *
     * 非阻塞地尝试获取独占访问权。
     */
    bool try_lock() noexcept;

    /**
     * @brief 获取读锁
     *
     * 阻塞当前线程，直到获得共享访问权。多个线程可以同时持有读锁，
     * 但不能与写锁同时存在。
     */
    void lock_shared();

    /**
     * @brief 释放读锁
     *
     * 释放共享访问权。如果所有读锁都已释放，则允许获取写锁。
     */
    void unlock_shared();

    /**
     * @brief 尝试获取读锁
     * @return 如果成功获得读锁则返回true，否则返回false
     *
     * 非阻塞地尝试获取共享访问权。
     */
    bool try_lock_shared() noexcept;
};

/**
 * @class shared_lock
 * @brief 共享锁类模板
 * @tparam SharedMutex 共享互斥锁类型
 *
 * RAII共享锁管理器，专门用于管理共享互斥锁的读锁。
 */
template <typename SharedMutex>
class shared_lock {
public:
    using mutex_type = SharedMutex;  ///< 共享互斥锁类型

private:
    mutex_type* mutex_ = nullptr;  ///< 指向共享互斥锁的指针
    bool owns_lock_ = false;       ///< 是否拥有共享锁的所有权

public:
    /**
     * @brief 默认构造函数
     *
     * 创建不管理任何共享互斥锁的共享锁。
     */
    shared_lock() = default;

    /**
     * @brief 从共享互斥锁构造
     * @param m 要管理的共享互斥锁引用
     *
     * 构造时立即获取共享互斥锁的读锁。
     */
    explicit shared_lock(mutex_type& m)
    : mutex_(&m), owns_lock_(true) {
        mutex_->lock_shared();
    }

    /**
     * @brief 延迟锁定构造函数
     * @param m 要管理的共享互斥锁引用
     * @param tag 延迟锁定标签
     *
     * 构造时不锁定共享互斥锁，稍后可以手动获取读锁。
     */
    shared_lock(mutex_type& m, defer_lock_tag tag) noexcept
    : mutex_(&m) {}

    /**
     * @brief 尝试锁定构造函数
     * @param m 要管理的共享互斥锁引用
     * @param tag 尝试锁定标签
     *
     * 构造时尝试获取共享互斥锁的读锁，如果失败不会阻塞。
     */
    shared_lock(mutex_type& m, try_lock_tag tag) noexcept
    : mutex_(&m), owns_lock_(m.try_lock_shared()) {}

    shared_lock(const shared_lock&) = delete;
    shared_lock& operator =(const shared_lock&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的共享锁
     *
     * 转移共享互斥锁的所有权和锁定状态。
     */
    shared_lock(shared_lock&& other) noexcept
    : mutex_(other.mutex_), owns_lock_(other.owns_lock_) {
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的共享锁
     * @return 当前对象的引用
     *
     * 释放当前锁，然后转移所有权。
     */
    shared_lock& operator =(shared_lock&& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        if (owns_lock_) mutex_->unlock_shared();
        mutex_ = other.mutex_;
        owns_lock_ = other.owns_lock_;
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 如果拥有共享锁的所有权，则释放读锁。
     */
    ~shared_lock() {
        if (owns_lock_) mutex_->unlock_shared();
    }

    /**
     * @brief 转换为布尔值
     * @return 是否拥有共享锁的所有权
     */
    NEFORCE_NODISCARD explicit operator bool() const noexcept { return owns_lock_; }

    /**
     * @brief 检查是否拥有共享锁
     * @return 是否拥有共享锁的所有权
     */
    NEFORCE_NODISCARD bool owns_lock() const noexcept { return owns_lock_; }

    /**
     * @brief 获取管理的共享互斥锁指针
     * @return 指向管理的共享互斥锁的指针，如果没有管理则返回nullptr
     */
    NEFORCE_NODISCARD mutex_type* mutex() const noexcept { return mutex_; }

    /**
     * @brief 获取读锁
     *
     * 如果已拥有锁或未管理共享互斥锁，则不执行任何操作。
     */
    void lock() {
        if (!mutex_) return;
        if (owns_lock_) return;
        mutex_->lock_shared();
        owns_lock_ = true;
    }

    /**
     * @brief 释放读锁
     *
     * 如果未拥有锁或未管理共享互斥锁，则不执行任何操作。
     */
    void unlock() {
        if (!mutex_) return;
        if (!owns_lock_) return;
        mutex_->unlock_shared();
        owns_lock_ = false;
    }

    /**
     * @brief 尝试获取读锁
     * @return 如果成功获得读锁则返回true，否则返回false
     *
     * 非阻塞地尝试获取共享互斥锁的读锁。
     */
    bool try_lock() noexcept {
        if (!mutex_) return false;
        if (owns_lock_) return true;
        owns_lock_ = mutex_->try_lock_shared();
        return owns_lock_;
    }

    /**
     * @brief 释放所有权
     * @return 之前管理的共享互斥锁指针
     *
     * 放弃对共享互斥锁的管理权，返回互斥锁指针但不释放锁。
     */
    mutex_type* release() noexcept {
        mutex_type* ret = mutex_;
        mutex_ = nullptr;
        owns_lock_ = false;
        return ret;
    }
};

/** @} */ // Mutex

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_SHARED_MUTEX_HPP__
