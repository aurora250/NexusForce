#ifndef NEFORCE_CORE_ASYNC_MUTEX_HPP__
#define NEFORCE_CORE_ASYNC_MUTEX_HPP__

/**
 * @file mutex.hpp
 * @brief 互斥锁
 *
 * 此文件提供了互斥锁与锁工具的实现。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#include "NeForce/core/config/windef.hpp"
#include <synchapi.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Mutex 互斥锁
 * @brief 互斥锁类型和工具
 * @{
 */

/**
 * @class mutex
 * @brief 非递归互斥锁
 */
class NEFORCE_API mutex {
public:
    /**
     * @brief 互斥锁的系统句柄类型
     */
    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::SRWLOCK;
#else
        ::pthread_mutex_t;
#endif

private:
    mutable native_handle_type mutex_;  ///< 互斥锁句柄

public:
    /**
     * @brief 构造函数
     */
    mutex();

    /**
     * @brief 析构函数
     */
    ~mutex();

    mutex(const mutex&) = delete;
    mutex& operator =(const mutex&) = delete;

    mutex(mutex&&) = default;
    mutex& operator =(mutex&&) = default;

    /**
     * @brief 获取原生句柄
     * @return 指向互斥锁原生句柄的指针
     */
    native_handle_type* native_handle() noexcept { return &mutex_; }

    /**
     * @brief 获取常量原生句柄
     * @return 指向互斥锁原生句柄的常量指针
     */
    const native_handle_type* native_handle() const noexcept { return &mutex_; }

    /**
     * @brief 锁定互斥锁
     *
     * 阻塞当前线程，直到获得互斥锁的所有权。
     */
    void lock();

    /**
     * @brief 解锁互斥锁
     *
     * 释放互斥锁的所有权。
     */
    void unlock();

    /**
     * @brief 尝试锁定互斥锁
     * @return 如果成功获得锁则返回true，否则返回false
     *
     * 非阻塞地尝试获取互斥锁的所有权。
     */
    bool try_lock() noexcept;
};

/**
 * @class recursive_mutex
 * @brief 递归互斥锁
 *
 * 允许同一线程多次锁定同一互斥锁，需要相同次数的解锁。
 */
class NEFORCE_API recursive_mutex {
public:
    /**
     * @brief 递归互斥锁的系统句柄类型
     */
    using native_handle_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::CRITICAL_SECTION;
#else
        ::pthread_mutex_t;
#endif

private:
    mutable native_handle_type recursive_mutex_;  ///< 互斥锁句柄

public:
    /**
     * @brief 构造函数
     */
    recursive_mutex();

    /**
     * @brief 析构函数
     */
    ~recursive_mutex();

    recursive_mutex(const recursive_mutex&) = delete;
    recursive_mutex& operator =(const recursive_mutex&) = delete;

    /**
     * @brief 获取原生句柄
     * @return 指向递归互斥锁原生句柄的指针
     */
    native_handle_type* native_handle() noexcept { return &recursive_mutex_; }

    /**
     * @brief 获取常量原生句柄
     * @return 指向递归互斥锁原生句柄的常量指针
     */
    const native_handle_type* native_handle() const noexcept { return &recursive_mutex_; }

    /**
     * @brief 锁定递归互斥锁
     *
     * 阻塞当前线程，直到获得递归互斥锁的所有权。
     * 同一线程可以多次锁定，但需要相同次数的解锁。
     */
    void lock();

    /**
     * @brief 解锁递归互斥锁
     *
     * 减少锁定计数，当计数为零时解锁。
     */
    void unlock();

    /**
     * @brief 尝试锁定递归互斥锁
     * @return 如果成功获得锁则返回true，否则返回false
     *
     * 非阻塞地尝试获取递归互斥锁的所有权。
     */
    bool try_lock() noexcept;
};


/**
 * @class lock
 * @brief 锁管理器模板
 * @tparam Mutex 互斥锁类型
 *
 * RAII风格的锁管理器，在构造时锁定互斥锁，在析构时解锁。
 */
template <typename Mutex>
class lock {
public:
    using mutex_type = Mutex;  ///< 互斥锁类型

private:
    mutex_type& mutex_;  ///< 引用的互斥锁

public:
    /**
     * @brief 构造函数
     * @param m 要管理的互斥锁引用
     *
     * 构造时锁定互斥锁。
     */
    explicit lock(mutex_type& m) : mutex_(m) {
        mutex_.lock();
    }

    /**
     * @brief 析构函数
     *
     * 析构时解锁互斥锁。
     */
    ~lock() {
        mutex_.unlock();
    }

    lock(const lock&) = delete;
    lock& operator =(const lock&) = delete;
};


/**
 * @struct defer_lock_tag
 * @brief 延迟锁定标签
 *
 * 用于unique_lock构造函数，表示延迟锁定互斥锁。
 */
struct defer_lock_tag {
    constexpr defer_lock_tag() noexcept = default;
};
/**
 * @brief 延迟锁定标签实例
 */
NEFORCE_INLINE17 constexpr defer_lock_tag defer_lock{};

/**
 * @struct try_lock_tag
 * @brief 尝试锁定标签
 *
 * 用于unique_lock构造函数，表示尝试锁定互斥锁。
 */
struct try_lock_tag {
    constexpr try_lock_tag() noexcept = default;
};
/**
 * @brief 尝试锁定标签实例
 */
NEFORCE_INLINE17 constexpr try_lock_tag try_lock{};


/**
 * @brief 独占锁管理器模板
 * @tparam Mutex 互斥锁类型
 *
 * 独占锁，支持延迟锁定、尝试锁定、转移所有权等特性。
 */
template <typename Mutex>
class unique_lock {
public:
    using mutex_type = Mutex; ///< 互斥锁类型

private:
    mutex_type* mutex_ = nullptr; ///< 指向互斥锁的指针
    bool owns_lock_ = false;      ///< 是否拥有锁的所有权

public:
    /**
     * @brief 默认构造函数
     *
     * 创建不管理任何互斥锁的唯一锁。
     */
    unique_lock() = default;

    /**
     * @brief 从互斥锁构造
     * @param m 要管理的互斥锁引用
     *
     * 构造时立即锁定互斥锁。
     */
    explicit unique_lock(mutex_type& m)
    : mutex_(&m), owns_lock_(true) {
        mutex_->lock();
    }

    /**
     * @brief 延迟锁定构造函数
     * @param m 要管理的互斥锁引用
     * @param tag 延迟锁定标签
     *
     * 构造时不锁定互斥锁，稍后可以调用lock()锁定。
     */
    unique_lock(mutex_type& m, defer_lock_tag tag) noexcept
    : mutex_(&m) {}

    /**
     * @brief 尝试锁定构造函数
     * @param m 要管理的互斥锁引用
     * @param tag 尝试锁定标签
     *
     * 构造时尝试锁定互斥锁，如果失败则不会阻塞。
     */
    unique_lock(mutex_type& m, try_lock_tag tag) noexcept
    : mutex_(&m), owns_lock_(m.try_lock()) {}

    unique_lock(const unique_lock&) = delete;
    unique_lock& operator =(const unique_lock&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的唯一锁
     *
     * 转移互斥锁的所有权和锁定状态。
     */
    unique_lock(unique_lock&& other) noexcept
        : mutex_(other.mutex_), owns_lock_(other.owns_lock_) {
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的唯一锁
     * @return 当前对象的引用
     *
     * 释放当前锁，然后转移所有权。
     */
    unique_lock& operator =(unique_lock&& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        if (owns_lock_) mutex_->unlock();
        mutex_ = other.mutex_;
        owns_lock_ = other.owns_lock_;
        other.mutex_ = nullptr;
        other.owns_lock_ = false;
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 如果拥有锁的所有权，则解锁互斥锁。
     */
    ~unique_lock() {
        if (owns_lock_) mutex_->unlock();
    }

    /**
     * @brief 转换为布尔值
     * @return 是否拥有锁的所有权
     */
    NEFORCE_NODISCARD explicit operator bool() const noexcept {
        return owns_lock_;
    }

    /**
     * @brief 检查是否拥有锁
     * @return 是否拥有锁的所有权
     */
    NEFORCE_NODISCARD bool owns_lock() const noexcept {
        return owns_lock_;
    }

    /**
     * @brief 获取管理的互斥锁指针
     * @return 指向管理的互斥锁的指针，如果没有管理则返回nullptr
     */
    NEFORCE_NODISCARD mutex_type* mutex() const noexcept {
        return mutex_;
    }

    /**
     * @brief 锁定互斥锁
     *
     * 如果已拥有锁或未管理互斥锁，则不执行任何操作。
     */
    void lock_quiet() {
        if (!mutex_) return;
        if (owns_lock_) return;
        mutex_->lock();
        owns_lock_ = true;
    }

    /**
     * @brief 解锁互斥锁
     *
     * 如果未拥有锁或未管理互斥锁，则不执行任何操作。
     */
    void unlock_quiet() {
        if (!mutex_) return;
        if (!owns_lock_) return;
        mutex_->unlock();
        owns_lock_ = false;
    }

    /**
     * @brief 尝试锁定互斥锁
     * @return 如果成功获得锁则返回true，否则返回false
     *
     * 非阻塞地尝试锁定互斥锁。
     */
    bool try_lock() noexcept {
        if (!mutex_) return false;
        if (owns_lock_) return true;
        owns_lock_ = mutex_->try_lock();
        return owns_lock_;
    }

    /**
     * @brief 释放所有权
     * @return 之前管理的互斥锁指针
     *
     * 放弃对互斥锁的管理权，返回互斥锁指针但不解锁。
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
#endif // NEFORCE_CORE_ASYNC_MUTEX_HPP__
