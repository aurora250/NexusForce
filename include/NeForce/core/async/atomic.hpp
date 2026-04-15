#ifndef NEFORCE_CORE_ASYNC_ATOMIC_HPP__
#define NEFORCE_CORE_ASYNC_ATOMIC_HPP__

/**
 * @file atomic.hpp
 * @brief 原子类型完整实现
 *
 * 此文件提供了完整的原子类型实现，包括通用原子类型和特定类型的特化。
 */

#include "NeForce/core/async/atomic_base.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup AtomicOperations 原子操作
 * @brief 原子变量的操作
 * @{
 */

template <typename T>
struct atomic;

/**
 * @struct atomic
 * @brief 通用原子类型模板
 * @tparam T 值类型
 *
 * 提供通用类型的原子操作，支持任意可平凡复制的类型。
 */
template <typename T>
struct atomic {
    using value_type = T; ///< 值类型

private:
    static constexpr int min_align = (sizeof(T) & (sizeof(T) - 1)) || sizeof(T) > 16 ? 0 : sizeof(T);
    static constexpr int align_inner = min_align > alignof(T) ? min_align : alignof(T);

    alignas(align_inner) T value_; ///< 原子值存储

    static_assert(is_trivially_copyable_v<T>, "atomic requires a trivially copyable type");
    static_assert(sizeof(T) > 0, "Incomplete or zero-sized types are not supported");
    static_assert(is_copy_constructible_v<T>, "atomic need copy constructible T");
    static_assert(is_move_constructible_v<T>, "atomic need move constructible T");
    static_assert(is_copy_assignable_v<T>, "atomic copy move assignable T");
    static_assert(is_move_assignable_v<T>, "atomic need move assignable T");

public:
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param value 初始值
     */
    constexpr atomic(T value) noexcept :
    value_(value) {}

    /**
     * @brief 类型转换运算符
     * @return 当前值
     */
    operator T() const noexcept { return load(); }

    /**
     * @brief volatile版本的类型转换运算符
     */
    operator T() const volatile noexcept { return load(); }

    /**
     * @brief 赋值运算符
     * @param value 要设置的值
     * @return 设置后的值
     */
    T operator=(T value) noexcept {
        atomic::store(value);
        return value;
    }

    /**
     * @brief volatile版本的赋值运算符
     */
    T operator=(T value) volatile noexcept {
        atomic::store(value);
        return value;
    }

    /**
     * @brief 检查是否支持无锁操作
     * @return 是否支持无锁
     */
    bool is_lock_free() const noexcept { return _NEFORCE is_always_lock_free<sizeof(value_), align_inner>(); }

    /**
     * @brief volatile版本的检查是否支持无锁操作
     */
    bool is_lock_free() const volatile noexcept { return _NEFORCE is_always_lock_free<sizeof(value_), align_inner>(); }

    /**
     * @brief 原子存储操作
     * @param value 要存储的值
     * @param mo 内存顺序
     */
    void store(T value, const memory_order mo = memory_order_seq_cst) noexcept {
        _NEFORCE atomic_store_any(_NEFORCE addressof(value_), value, mo);
    }

    /**
     * @brief volatile版本的原子存储操作
     */
    void store(T value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
        _NEFORCE atomic_store_any(_NEFORCE addressof(value_), value, mo);
    }

    /**
     * @brief 原子加载操作
     * @param mo 内存顺序
     * @return 加载的值
     */
    T load(const memory_order mo = memory_order_seq_cst) const noexcept {
        return _NEFORCE atomic_load_any(_NEFORCE addressof(value_), mo);
    }

    /**
     * @brief volatile版本的原子加载操作
     */
    T load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
        return _NEFORCE atomic_load_any(_NEFORCE addressof(value_), mo);
    }

    /**
     * @brief 原子交换操作
     * @param value 要交换的值
     * @param mo 内存顺序
     * @return 交换前的值
     */
    T exchange(T value, const memory_order mo = memory_order_seq_cst) noexcept {
        return _NEFORCE atomic_exchange_any(_NEFORCE addressof(value_), value, mo);
    }

    /**
     * @brief volatile版本的原子交换操作
     */
    T exchange(T value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
        return _NEFORCE atomic_exchange_any(_NEFORCE addressof(value_), value, mo);
    }

    /**
     * @brief 弱比较交换操作
     * @param expected 期望值
     * @param desired 期望设置的值
     * @param success 成功时的内存顺序
     * @param failure 失败时的内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_weak(T& expected, T desired, const memory_order success,
                               const memory_order failure) noexcept {
        NEFORCE_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
        return _NEFORCE atomic_cmpexch_weak_any(_NEFORCE addressof(value_), &expected, &desired, success, failure);
    }

    /**
     * @brief volatile版本的弱比较交换操作
     */
    bool compare_exchange_weak(T& expected, T desired, const memory_order success,
                               const memory_order failure) volatile noexcept {
        NEFORCE_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
        return _NEFORCE atomic_cmpexch_weak_any(_NEFORCE addressof(value_), &expected, &desired, success, failure);
    }

    /**
     * @brief 简化版弱比较交换操作
     * @param expected 期望值
     * @param desired 期望设置的值
     * @param mo 内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_weak(T& expected, T desired, const memory_order mo = memory_order_seq_cst) noexcept {
        return atomic::compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
    }

    /**
     * @brief volatile版本的简化版弱比较交换操作
     */
    bool compare_exchange_weak(T& expected, T desired, const memory_order mo = memory_order_seq_cst) volatile noexcept {
        return atomic::compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
    }

    /**
     * @brief 强比较交换操作
     * @param expected 期望值
     * @param desired 期望设置的值
     * @param success 成功时的内存顺序
     * @param failure 失败时的内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_strong(T& expected, T desired, const memory_order success,
                                 const memory_order failure) noexcept {
        NEFORCE_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
        return _NEFORCE atomic_cmpexch_strong_any(_NEFORCE addressof(value_), _NEFORCE addressof(expected),
                                                  _NEFORCE addressof(desired), success, failure);
    }

    /**
     * @brief volatile版本的强比较交换操作
     */
    bool compare_exchange_strong(T& expected, T desired, const memory_order success,
                                 const memory_order failure) volatile noexcept {
        NEFORCE_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
        return _NEFORCE atomic_cmpexch_strong_any(_NEFORCE addressof(value_), _NEFORCE addressof(expected),
                                                  _NEFORCE addressof(desired), success, failure);
    }

    /**
     * @brief 简化版强比较交换操作
     * @param expected 期望值
     * @param value 期望设置的值
     * @param mo 内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_strong(T& expected, T value, const memory_order mo = memory_order_seq_cst) noexcept {
        return atomic::compare_exchange_strong(expected, value, mo, cmpexch_failure_order(mo));
    }

    /**
     * @brief volatile版本的简化版强比较交换操作
     */
    bool compare_exchange_strong(T& expected, T value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
        return atomic::compare_exchange_strong(expected, value, mo, cmpexch_failure_order(mo));
    }
};

/**
 * @brief 指针类型的原子特化
 * @tparam T 指针指向的类型
 */
template <typename T>
struct atomic<T*> : atomic_base<T*> {
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    explicit atomic(T* value) noexcept :
    atomic_base<T*>(value) {}

    using atomic_base<T*>::operator=;
};

/**
 * @brief 引用类型的原子特化
 * @tparam T 引用指向的类型
 */
template <typename T>
struct atomic<T&> : atomic_ref_base<T> {
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    explicit atomic(T& value) noexcept :
    atomic_ref_base<T>(value) {}

    using atomic_ref_base<T>::operator=;
};


/**
 * @brief bool类型的原子特化
 * @note 提供bool类型的原子操作
 */
template <>
struct atomic<bool> {
    using value_type = bool; ///< 值类型

private:
    atomic_base<bool> base_; ///< 基础实例

public:
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param value 初始值
     */
    constexpr atomic(const bool value) noexcept :
    base_(value) {}

    /**
     * @brief 赋值运算符
     * @param value 要设置的值
     * @return 设置后的值
     */
    bool operator=(const bool value) noexcept { return base_.operator=(value); }

    /**
     * @brief volatile版本的赋值运算符
     */
    bool operator=(const bool value) volatile noexcept { return base_.operator=(value); }

    /**
     * @brief 类型转换运算符
     * @return 当前值
     */
    operator bool() const noexcept { return base_.load(); }

    /**
     * @brief volatile版本的类型转换运算符
     */
    operator bool() const volatile noexcept { return base_.load(); }

    /**
     * @brief 检查是否支持无锁操作
     * @return 是否支持无锁
     */
    bool is_lock_free() const noexcept { return base_.is_lock_free(); }

    /**
     * @brief volatile版本的检查是否支持无锁操作
     */
    bool is_lock_free() const volatile noexcept { return base_.is_lock_free(); }

    /**
     * @brief 原子存储操作
     * @param value 要存储的值
     * @param mo 内存顺序
     */
    void store(const bool value, const memory_order mo = memory_order_seq_cst) noexcept { base_.store(value, mo); }

    /**
     * @brief volatile版本的原子存储操作
     */
    void store(const bool value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
        base_.store(value, mo);
    }

    /**
     * @brief 原子加载操作
     * @param mo 内存顺序
     * @return 加载的值
     */
    bool load(const memory_order mo = memory_order_seq_cst) const noexcept { return base_.load(mo); }

    /**
     * @brief volatile版本的原子加载操作
     */
    bool load(const memory_order mo = memory_order_seq_cst) const volatile noexcept { return base_.load(mo); }

    /**
     * @brief 原子交换操作
     * @param value 要交换的值
     * @param mo 内存顺序
     * @return 交换前的值
     */
    bool exchange(const bool value, const memory_order mo = memory_order_seq_cst) noexcept {
        return base_.exchange(value, mo);
    }

    /**
     * @brief volatile版本的原子交换操作
     */
    bool exchange(const bool value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
        return base_.exchange(value, mo);
    }

    /**
     * @brief 弱比较交换操作
     * @param value1 期望值
     * @param value2 期望设置的值
     * @param success 成功时的内存顺序
     * @param failure 失败时的内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_weak(bool& value1, const bool value2, const memory_order success,
                               const memory_order failure) noexcept {
        return base_.compare_exchange_weak(value1, value2, success, failure);
    }

    /**
     * @brief volatile版本的弱比较交换操作
     */
    bool compare_exchange_weak(bool& value1, const bool value2, const memory_order success,
                               const memory_order failure) volatile noexcept {
        return base_.compare_exchange_weak(value1, value2, success, failure);
    }

    /**
     * @brief 简化版弱比较交换操作
     * @param value1 期望值
     * @param value2 期望设置的值
     * @param mo 内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_weak(bool& value1, const bool value2, const memory_order mo = memory_order_seq_cst) noexcept {
        return base_.compare_exchange_weak(value1, value2, mo);
    }

    /**
     * @brief volatile版本的简化版弱比较交换操作
     */
    bool compare_exchange_weak(bool& value1, const bool value2,
                               const memory_order mo = memory_order_seq_cst) volatile noexcept {
        return base_.compare_exchange_weak(value1, value2, mo);
    }

    /**
     * @brief 强比较交换操作
     * @param value1 期望值
     * @param value2 期望设置的值
     * @param success 成功时的内存顺序
     * @param failure 失败时的内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_strong(bool& value1, const bool value2, const memory_order success,
                                 const memory_order failure) noexcept {
        return base_.compare_exchange_strong(value1, value2, success, failure);
    }

    /**
     * @brief volatile版本的强比较交换操作
     */
    bool compare_exchange_strong(bool& value1, const bool value2, const memory_order success,
                                 const memory_order failure) volatile noexcept {
        return base_.compare_exchange_strong(value1, value2, success, failure);
    }

    /**
     * @brief 简化版强比较交换操作
     * @param value1 期望值
     * @param value2 期望设置的值
     * @param mo 内存顺序
     * @return 是否交换成功
     */
    bool compare_exchange_strong(bool& value1, const bool value2,
                                 const memory_order mo = memory_order_seq_cst) noexcept {
        return base_.compare_exchange_strong(value1, value2, mo);
    }

    /**
     * @brief volatile版本的简化版强比较交换操作
     */
    bool compare_exchange_strong(bool& value1, const bool value2,
                                 const memory_order mo = memory_order_seq_cst) volatile noexcept {
        return base_.compare_exchange_strong(value1, value2, mo);
    }

    /**
     * @brief 等待值改变
     * @param old 期望的旧值
     * @param mo 内存顺序
     */
    void wait(const bool old, const memory_order mo = memory_order_seq_cst) const noexcept { base_.wait(old, mo); }

    /**
     * @brief 通知一个等待线程
     */
    void notify_one() noexcept { base_.notify_one(); }

    /**
     * @brief 通知所有等待线程
     */
    void notify_all() noexcept { base_.notify_all(); }
};

/// @brief char类型的原子特化
template <>
struct atomic<char> : atomic_base<char> {
    using integral_type = char;
    using base_type = atomic_base<char>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief signed char类型的原子特化
template <>
struct atomic<signed char> : atomic_base<signed char> {
    using integral_type = signed char;
    using base_type = atomic_base<signed char>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief unsigned char类型的原子特化
template <>
struct atomic<unsigned char> : atomic_base<unsigned char> {
    using integral_type = unsigned char;
    using base_type = atomic_base<unsigned char>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief short类型的原子特化
template <>
struct atomic<short> : atomic_base<short> {
    using integral_type = short;
    using base_type = atomic_base<short>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief unsigned short类型的原子特化
template <>
struct atomic<unsigned short> : atomic_base<unsigned short> {
    using integral_type = unsigned short;
    using base_type = atomic_base<unsigned short>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief int类型的原子特化
template <>
struct atomic<int> : atomic_base<int> {
    using integral_type = int;
    using base_type = atomic_base<int>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief unsigned int类型的原子特化
template <>
struct atomic<unsigned int> : atomic_base<unsigned int> {
    using integral_type = unsigned int;
    using base_type = atomic_base<unsigned int>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief long类型的原子特化
template <>
struct atomic<long> : atomic_base<long> {
    using integral_type = long;
    using base_type = atomic_base<long>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief unsigned long类型的原子特化
template <>
struct atomic<unsigned long> : atomic_base<unsigned long> {
    using integral_type = unsigned long;
    using base_type = atomic_base<unsigned long>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief long long类型的原子特化
template <>
struct atomic<long long> : atomic_base<long long> {
    using integral_type = long long;
    using base_type = atomic_base<long long>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief unsigned long long类型的原子特化
template <>
struct atomic<unsigned long long> : atomic_base<unsigned long long> {
    using integral_type = unsigned long long;
    using base_type = atomic_base<unsigned long long>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief wchar_t类型的原子特化
template <>
struct atomic<wchar_t> : atomic_base<wchar_t> {
    using integral_type = wchar_t;
    using base_type = atomic_base<wchar_t>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

#ifdef NEFORCE_STANDARD_20
/// @brief char8_t类型的原子特化
template <>
struct atomic<char8_t> : atomic_base<char8_t> {
    using integral_type = char8_t;
    using base_type = atomic_base<char8_t>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};
#endif

/// @brief char16_t类型的原子特化
template <>
struct atomic<char16_t> : atomic_base<char16_t> {
    using integral_type = char16_t;
    using base_type = atomic_base<char16_t>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief char32_t类型的原子特化
template <>
struct atomic<char32_t> : atomic_base<char32_t> {
    using integral_type = char32_t;
    using base_type = atomic_base<char32_t>;

    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const integral_type value) noexcept :
    base_type(value) {}

    using base_type::operator integral_type;
    using base_type::operator=;

    static constexpr bool is_always_lock_free = true;
};

/// @brief float类型的原子特化
template <>
struct atomic<float> : atomic_float_base<float> {
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const float value) noexcept :
    atomic_float_base<float>(value) {}

    using atomic_float_base<float>::operator=;
};

/// @brief double类型的原子特化
template <>
struct atomic<double> : atomic_float_base<double> {
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const double value) noexcept :
    atomic_float_base<double>(value) {}

    using atomic_float_base<double>::operator=;
};

/// @brief long double类型的原子特化
template <>
struct atomic<long double> : atomic_float_base<long double> {
    atomic() = default;
    ~atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;
    atomic& operator=(const atomic&) volatile = delete;
    atomic(atomic&&) noexcept = default;
    atomic& operator=(atomic&&) noexcept = default;

    constexpr atomic(const long double value) noexcept :
    atomic_float_base<long double>(value) {}

    using atomic_float_base<long double>::operator=;
};

/** @} */ // AtomicOperations

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ATOMIC_HPP__
