#ifndef MSTL_CORE_ASYNC_ATOMIC_BASE_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_BASE_HPP__

/**
 * @file atomic_base.hpp
 * @brief MSTL原子操作基本工具
 *
 * 此文件提供了原子操作的基本工具，包括内存序定义、原子类型基础类等。
 */

#include "atomic_wait.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <intrin0.h>
#endif
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup MemoryOrder 原子内存序
 * @brief 原子内存顺序行为
 * @{
 */

/**
 * @enum memory_order
 * @brief 内存顺序
 *
 * 定义原子操作的内存顺序语义，控制不同线程间的内存可见性顺序。
 */
enum class memory_order : int32_t {
	relaxed,    ///< 最宽松的内存顺序，只保证原子性
	consume,    ///< 数据依赖顺序，用于依赖读取的场景
	acquire,    ///< 获取操作，确保后续读写不会被重排到前面
	release,    ///< 释放操作，确保前面的读写不会被重排到后面
	acq_rel,    ///< 获取-释放组合操作
	seq_cst     ///< 顺序一致性，最严格的内存顺序
};

/// @brief 宽松内存顺序常量
MSTL_INLINE17 constexpr auto memory_order_relaxed = memory_order::relaxed;
/// @brief 数据依赖内存顺序常量
MSTL_INLINE17 constexpr auto memory_order_consume = memory_order::consume;
/// @brief 获取内存顺序常量
MSTL_INLINE17 constexpr auto memory_order_acquire = memory_order::acquire;
/// @brief 释放内存顺序常量
MSTL_INLINE17 constexpr auto memory_order_release = memory_order::release;
/// @brief 获取-释放内存顺序常量
MSTL_INLINE17 constexpr auto memory_order_acq_rel = memory_order::acq_rel;
/// @brief 顺序一致性内存顺序常量
MSTL_INLINE17 constexpr auto memory_order_seq_cst = memory_order::seq_cst;


/**
 * @enum memory_order_modifier
 * @brief 内存顺序修饰符枚举
 *
 * 用于扩展memory_order的功能，支持硬件锁消除（HLE）等高级特性。
 */
enum class memory_order_modifier : int64_t {
	memory_order_mask          = 0x0ffff,      ///< 内存顺序掩码
	memory_order_modifier_mask = 0xffff0000,   ///< 修饰符掩码
	memory_order_hle_acquire   = 0x10000,      ///< HLE获取修饰符
	memory_order_hle_release   = 0x20000       ///< HLE释放修饰符
};

/**
 * @brief 内存顺序与修饰符的或操作符
 * @param mo 内存顺序
 * @param mod 内存顺序修饰符
 * @return 组合后的内存顺序
 *
 * @note 用于将内存顺序与修饰符组合使用
 */
constexpr memory_order operator |(memory_order mo, memory_order_modifier mod) noexcept {
    return static_cast<memory_order>(static_cast<int64_t>(mo) | static_cast<int64_t>(mod));
}

/**
 * @brief 内存顺序与修饰符的与操作符
 * @param mo 内存顺序
 * @param mod 内存顺序修饰符
 * @return 提取后的内存顺序
 *
 * @note 用于从组合值中提取特定的内存顺序或修饰符
 */
constexpr memory_order operator &(memory_order mo, memory_order_modifier mod) noexcept {
    return static_cast<memory_order>(static_cast<int64_t>(mo) & static_cast<int64_t>(mod));
}


/**
 * @brief 获取原子比较交换操作失败时的内存顺序
 *
 * 对于原子比较交换（compare-exchange）操作，成功和失败的内存顺序可以不同。
 * 此函数根据给定的内存顺序，返回对应的失败内存顺序。
 *
 * 转换规则：
 * - 如果基础内存顺序为 `memory_order_acq_rel`，失败顺序为 `memory_order_acquire`
 * - 如果基础内存顺序为 `memory_order_release`，失败顺序为 `memory_order_relaxed`
 * - 其他基础内存顺序保持不变
 * - 保留原始内存顺序中的所有修饰符
 */
constexpr memory_order cmpexch_failure_order(const memory_order mo) noexcept {
	constexpr auto mask = memory_order_modifier::memory_order_mask;
	const memory_order base_mo = mo & mask;
	const memory_order failure_order =
		base_mo == memory_order_acq_rel ? memory_order_acquire
		: base_mo == memory_order_release ? memory_order_relaxed
		: base_mo;
	constexpr auto modifier_mask = memory_order_modifier::memory_order_modifier_mask;
	const auto modifiers = static_cast<memory_order_modifier>(mo & modifier_mask);
	return failure_order | modifiers;
}

/**
 * @brief 检查比较交换失败内存顺序是否有效
 * @param mo 待检查的内存顺序
 * @return 是否有效
 * @note 失败内存顺序不能是release或acq_rel
 */
constexpr bool is_valid_cmpexch_failure_order(const memory_order mo) noexcept {
    return (mo & memory_order_modifier::memory_order_mask) != memory_order_release
        && (mo & memory_order_modifier::memory_order_mask) != memory_order_acq_rel;
}


/**
 * @brief 线程内存屏障
 * @param mo 内存顺序
 *
 * 在不同线程间插入内存屏障，确保内存操作的顺序性。
 */
MSTL_ALWAYS_INLINE_INLINE void atomic_thread_fence(const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_MSVC__
	if (mo == memory_order_relaxed) return;
#if defined(MSTL_ARCH_X86__)
	::_ReadWriteBarrier();
	if (mo == memory_order_seq_cst) {
		volatile long guard;
		::_InterlockedIncrement(&guard);
		::_ReadWriteBarrier();
	}
#elif defined(MSTL_ARCH_ARM__)
	if (mo == memory_order_acquire || mo == memory_order_consume) {
		::_Memory_load_acquire_barrier();
	} else {
		::_ReadWriteBarrier();
	}
#else
	assert(false);
#endif
#else
	__atomic_thread_fence(static_cast<int32_t>(mo));
#endif
}

/**
 * @brief 信号内存屏障
 * @param mo 内存顺序
 *
 * 在同一线程内的信号处理程序和普通代码间插入内存屏障。
 */
MSTL_ALWAYS_INLINE_INLINE void atomic_signal_fence(const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_MSVC__
	if (mo != memory_order_relaxed) ::_ReadWriteBarrier();
#else
	__atomic_signal_fence(static_cast<int32_t>(mo));
#endif
}

/** @} */ // MemoryOrder

/// @cond
MSTL_BEGIN_INNER__

#ifdef MSTL_COMPILER_MSVC__

/**
 * @brief 交换操作实现模板
 * @tparam Size 数据类型大小
 *
 * 针对不同大小的数据类型提供特定的交换操作实现。
 */
template <size_t Size>
struct interlocked_exchange_impl;

template <>
struct interlocked_exchange_impl<1> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(value)));
	}
};

template <>
struct interlocked_exchange_impl<2> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(value)));
	}
};

template <>
struct interlocked_exchange_impl<4> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(value)));
	}
};

template <>
struct interlocked_exchange_impl<8> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_interlockedexchange64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


/**
 * @brief 获取并添加操作实现模板
 * @tparam Size 数据类型大小
 */
template <size_t Size>
struct interlocked_compare_exchange_impl;

template <>
struct interlocked_compare_exchange_impl<1> {
	template <typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		const char old = ::_InterlockedCompareExchange8(
		    reinterpret_cast<volatile char*>(target),
		    *reinterpret_cast<char*>(&desired),
			*reinterpret_cast<char*>(expected));
		if (old == *reinterpret_cast<char*>(expected)) return true;
		*reinterpret_cast<char*>(expected) = old;
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<2> {
	template <typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		const short old = ::_InterlockedCompareExchange16(
		    reinterpret_cast<volatile short*>(target),
		    *reinterpret_cast<short*>(&desired),
			*reinterpret_cast<short*>(expected));
		if (old == *reinterpret_cast<short*>(expected)) return true;
		*reinterpret_cast<short*>(expected) = old;
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<4> {
	template <typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		const long old = ::_InterlockedCompareExchange(
		    reinterpret_cast<volatile long*>(target),
		    *reinterpret_cast<long*>(&desired),
			*reinterpret_cast<long*>(expected));
		if (old == *reinterpret_cast<long*>(expected)) return true;
		*reinterpret_cast<long*>(expected) = old;
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<8> {
	template <typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		const long long old = ::_InterlockedCompareExchange64(
		    reinterpret_cast<volatile long long*>(target),
		    *reinterpret_cast<long long*>(&desired),
			*reinterpret_cast<long long*>(expected));
		if (old == *reinterpret_cast<long long*>(expected)) return true;
		*reinterpret_cast<long long*>(expected) = old;
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<16> {
	template <typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		alignas(16) long long exp_arr[2];
		alignas(16) long long des_arr[2];
		_MSTL memory_copy(exp_arr, expected, 16);
		_MSTL memory_copy(des_arr, &desired, 16);

		const bool result = ::_InterlockedCompareExchange128(
			reinterpret_cast<volatile long long*>(target),
			des_arr[1], des_arr[0], exp_arr) != 0;

		if (!result) {
			_MSTL memory_copy(expected, exp_arr, 16);
		}
		return result;
	}
};


/**
 * @brief 获取并与操作实现模板
 * @tparam Size 数据类型大小
 */
template <size_t Size>
struct interlocked_fetch_add_impl;

template <>
struct interlocked_fetch_add_impl<1> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(value)));
	}
};

template <>
struct interlocked_fetch_add_impl<2> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(value)));
	}
};

template <>
struct interlocked_fetch_add_impl<4> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(value)));
	}
};

template <>
struct interlocked_fetch_add_impl<8> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_interlockedexchangeadd64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


/**
 * @brief 获取并与操作实现模板
 * @tparam Size 数据类型大小
 */
template <size_t Size>
struct interlocked_fetch_and_impl;

template <>
struct interlocked_fetch_and_impl<1> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(value)));
	}
};

template <>
struct interlocked_fetch_and_impl<2> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(value)));
	}
};

template <>
struct interlocked_fetch_and_impl<4> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(value)));
	}
};

template <>
struct interlocked_fetch_and_impl<8> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_interlockedand64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


/**
 * @brief 获取并或操作实现模板
 * @tparam Size 数据类型大小
 */
template <size_t Size>
struct interlocked_fetch_or_impl;

template <>
struct interlocked_fetch_or_impl<1> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr8(
		    reinterpret_cast<volatile char*>(target), static_cast<char>(value)));
	}
};
template <>
struct interlocked_fetch_or_impl<2> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr16(
		    reinterpret_cast<volatile short*>(target), static_cast<short>(value)));
	}
};
template <>
struct interlocked_fetch_or_impl<4> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr(
		    reinterpret_cast<volatile long*>(target), static_cast<long>(value)));
	}
};
template <>
struct interlocked_fetch_or_impl<8> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_interlockedor64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


/**
 * @brief 获取并异或操作实现模板
 * @tparam Size 数据类型大小
 */
template <size_t Size>
struct interlocked_fetch_xor_impl;

template <>
struct interlocked_fetch_xor_impl<1> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor8(
		    reinterpret_cast<volatile char*>(target), static_cast<char>(value)));
	}
};
template <>
struct interlocked_fetch_xor_impl<2> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor16(
		    reinterpret_cast<volatile short*>(target), static_cast<short>(value)));
	}
};
template <>
struct interlocked_fetch_xor_impl<4> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor(
		    reinterpret_cast<volatile long*>(target), static_cast<long>(value)));
	}
};
template <>
struct interlocked_fetch_xor_impl<8> {
	template <typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_interlockedxor64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};

#endif

#ifdef MSTL_COMPILER_GNUC__

/**
 * @brief 编译时常量：是否总是无锁
 * @tparam Size 数据类型大小
 *
 * 查询特定大小的数据类型是否总是支持无锁原子操作。
 */
template <size_t Size>
struct atomic_is_always_lock_free_impl {
	static constexpr bool value = __atomic_always_lock_free(Size, nullptr);
};

#else

template <size_t Size>
struct atomic_is_always_lock_free_impl {
	static constexpr bool value = false;
};
template <>
struct atomic_is_always_lock_free_impl<1> {
	static constexpr bool value = true;
};
template <>
struct atomic_is_always_lock_free_impl<2> {
	static constexpr bool value = true;
};
template <>
struct atomic_is_always_lock_free_impl<4> {
	static constexpr bool value = true;
};
template <>
struct atomic_is_always_lock_free_impl<8> {
	static constexpr bool value = true;
};

#endif

MSTL_END_INNER__
/// @endcond

/**
 * @defgroup AtomicOperations 原子操作
 * @brief 原子变量的操作
 * @{
 */

/**
 * @brief 原子操作的差值类型
 * @tparam T 原始类型
 */
template <typename T>
using atomic_diff_t = conditional_t<is_pointer_v<T>, ptrdiff_t, remove_volatile_t<T>>;


/**
 * @brief 原子存储操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 存储的值
 * @param mo 内存顺序
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE void
atomic_store(volatile T* ptr, remove_volatile_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    __atomic_store_n(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER interlocked_exchange_impl<sizeof(T)>::call(ptr, value);
	if (mo == memory_order_seq_cst || mo == memory_order_release) {
		::_ReadWriteBarrier();
	}
#endif
}

/**
 * @brief 原子加载操作
 * @tparam T 数据类型
 * @param ptr 源指针
 * @param mo 内存顺序
 * @return 加载的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_load(const volatile T* ptr, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_load_n(ptr, static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> result = *ptr;
	if (mo == memory_order_seq_cst || mo == memory_order_acquire) {
		::_ReadWriteBarrier();
	}
    return result;
#endif
}

/**
 * @brief 原子交换操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 交换的值
 * @param mo 内存顺序
 * @return 交换前的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_exchange(volatile T* ptr, remove_volatile_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_exchange_n(ptr, value, static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> old = _INNER interlocked_exchange_impl<sizeof(T)>::call(ptr, value);
	if (mo == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
    return old;
#endif
}

/**
 * @brief 弱比较交换操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param expected 期望值的指针
 * @param desired 期望设置的值
 * @param success 成功时的内存顺序
 * @param failure 失败时的内存顺序
 * @return 是否交换成功
 * @note 弱比较交换可能伪失败，需要循环重试
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE bool
atomic_cmpexch_weak(
	volatile T* ptr, remove_volatile_t<T>* expected,
	remove_volatile_t<T> desired, const memory_order success, const memory_order failure) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
	MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_compare_exchange_n(
		ptr, expected, desired, true,
		static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
#if defined(MSTL_ARCH_X86__)
	const bool result = _INNER interlocked_compare_exchange_impl<sizeof(T)>::call(ptr, expected, desired);
	if (success == memory_order_seq_cst || failure == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
	return result;
#else
	remove_volatile_t<T> old_val = *expected;
	remove_volatile_t<T> loaded;
	bool success_flag;
#if defined(MSTL_ARCH_ARM__)
	MSTL_IF_CONSTEXPR (sizeof(T) == 1) {
		asm volatile(
			"ldrexb %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strexb %w[success], %w[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 2) {
		asm volatile(
			"ldrexh %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strexh %w[success], %w[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 4) {
		asm volatile(
			"ldrex %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strex %w[success], %w[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 8) {
		asm volatile(
			"ldrexd %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strexd %w[success], %[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	}
#elif defined(MSTL_ARCH_RISCV__)
	MSTL_IF_CONSTEXPR (sizeof(T) == 4) {
		asm volatile(
			"lr.w %[loaded], (%[ptr])\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.w %[success], %[desired], (%[ptr])\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 8) {
		asm volatile(
			"lr.d %[loaded], (%[ptr])\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.d %[success], %[desired], (%[ptr])\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	}
#elif defined(MSTL_ARCH_LOONGARCH__)
	MSTL_IF_CONSTEXPR (sizeof(T) == 4) {
		asm volatile(
			"ll.w %[loaded], %[ptr]\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.w %[desired], %[ptr]\n\t"
			"move %[success], %[desired]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "m" (*ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 8) {
		asm volatile(
			"ll.d %[loaded], %[ptr]\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.d %[desired], %[ptr]\n\t"
			"move %[success], %[desired]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "m" (*ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	}
#endif
	if (loaded != old_val) {
		*expected = loaded;
		return false;
	}
	return success_flag == 0;
#endif
#endif
}

/**
 * @brief 强比较交换操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param expected 期望值的指针
 * @param desired 期望设置的值
 * @param success 成功时的内存顺序
 * @param failure 失败时的内存顺序
 * @return 是否交换成功
 * @note 强比较交换保证不伪失败
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE bool
atomic_cmpexch_strong(
	volatile T* ptr, remove_volatile_t<T>* expected,
	remove_volatile_t<T> desired, const memory_order success, const memory_order failure) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
    MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_compare_exchange_n(
		ptr, expected, desired, false,
		static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
#if defined(MSTL_ARCH_X86__)
	return _MSTL atomic_cmpexch_weak(ptr, expected, desired, success, failure);
#else
	remove_volatile_t<T> old_val = *expected;
	while (true) {
		if (_INNER cmpexch_weak(ptr, expected, desired, success, failure)) {
			return true;
		}
		if (*expected != old_val) {
			return false;
		}
	}
#endif
#endif
}

/**
 * @brief 原子获取并添加操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要添加的值
 * @param mo 内存顺序
 * @return 添加前的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_fetch_add(volatile T* ptr, atomic_diff_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_add(ptr, value, static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> old = _INNER interlocked_fetch_add_impl<sizeof(T)>::call(ptr, value);
	if (mo == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
    return old;
#endif
}

/**
 * @brief 原子获取并减去操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要减去的值
 * @param mo 内存顺序
 * @return 减去前的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_fetch_sub(volatile T* ptr, atomic_diff_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_sub(ptr, value, static_cast<int32_t>(mo));
#else
    return _MSTL atomic_fetch_add(ptr, static_cast<atomic_diff_t<T>>(-value), mo);
#endif
}

/**
 * @brief 原子获取并与操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要进行与操作的值
 * @param mo 内存顺序
 * @return 操作前的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_fetch_and(volatile T* ptr, remove_volatile_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_and(ptr, value, static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> old = _INNER interlocked_fetch_and_impl<sizeof(T)>::call(ptr, value);
	if (mo == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
    return old;
#endif
}

/**
 * @brief 原子获取并或操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要进行或操作的值
 * @param mo 内存顺序
 * @return 操作前的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_fetch_or(volatile T* ptr, remove_volatile_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_or(ptr, value, static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> old = _INNER interlocked_fetch_or_impl<sizeof(T)>::call(ptr, value);
	if (mo == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
    return old;
#endif
}

/**
 * @brief 原子获取并异或操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要进行异或操作的值
 * @param mo 内存顺序
 * @return 操作前的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_fetch_xor(volatile T* ptr, remove_volatile_t<T> value, const memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_xor(ptr, value, static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> old = _INNER interlocked_fetch_xor_impl<sizeof(T)>::call(ptr, value);
	if (mo == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
    return old;
#endif
}

/**
 * @brief 原子添加并获取操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要添加的值
 * @param mo 内存顺序
 * @return 添加后的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_add_fetch(volatile T* ptr, atomic_diff_t<T> value, memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_add_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _MSTL atomic_fetch_add(ptr, value, mo) + value;
#endif
}

/**
 * @brief 原子减去并获取操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要减去的值
 * @param mo 内存顺序
 * @return 减去后的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_sub_fetch(volatile T* ptr, atomic_diff_t<T> value, memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_sub_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _MSTL atomic_fetch_sub(ptr, value, mo) - value;
#endif
}

/**
 * @brief 原子与并获取操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要进行与操作的值
 * @param mo 内存顺序
 * @return 操作后的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_and_fetch(volatile T* ptr, remove_volatile_t<T> value, memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_and_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _MSTL atomic_fetch_and(ptr, value, mo) & value;
#endif
}

/**
 * @brief 原子或并获取操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要进行或操作的值
 * @param mo 内存顺序
 * @return 操作后的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_or_fetch(volatile T* ptr, remove_volatile_t<T> value, memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_or_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _MSTL atomic_fetch_or(ptr, value, mo) | value;
#endif
}

/**
 * @brief 原子异或并获取操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要进行异或操作的值
 * @param mo 内存顺序
 * @return 操作后的值
 * @note 要求操作类型为整形
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_xor_fetch(volatile T* ptr, remove_volatile_t<T> value, memory_order mo) noexcept {
	static_assert(is_integral_v<T>, "T must be integral type");
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_xor_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _MSTL atomic_fetch_xor(ptr, value, mo) ^ value;
#endif
}


/**
 * @brief 通用弱比较交换操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param expected 期望值的指针
 * @param desired 期望设置的值
 * @param success 成功时的内存顺序
 * @param failure 失败时的内存顺序
 * @return 是否交换成功
 * @note 弱比较交换可能伪失败，需要循环重试
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE bool
atomic_cmpexch_weak_any(
	volatile T* ptr, remove_volatile_t<T>* expected,
	remove_volatile_t<T>* desired, const memory_order success, const memory_order failure) noexcept {
	MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_compare_exchange(
		ptr, expected, desired, true,
		static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
#if defined(MSTL_ARCH_X86__)
	const bool result = _INNER interlocked_compare_exchange_impl<sizeof(T)>::call(ptr, expected, *desired);
	if (success == memory_order_seq_cst || failure == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
	return result;
#else
	remove_volatile_t<T> old_val = *expected;
	remove_volatile_t<T> loaded;
	bool success_flag;
#if defined(MSTL_ARCH_ARM__)
	MSTL_IF_CONSTEXPR (sizeof(T) == 1) {
		asm volatile(
			"ldrexb %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strexb %w[success], %w[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 2) {
		asm volatile(
			"ldrexh %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strexh %w[success], %w[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 4) {
		asm volatile(
			"ldrex %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strex %w[success], %w[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 8) {
		asm volatile(
			"ldrexd %[loaded], [%[ptr]]\n\t"
			"cmp %[loaded], %[old_val]\n\t"
			"bne 1f\n\t"
			"strexd %w[success], %[desired], [%[ptr]]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "cc", "memory");
	}
#elif defined(MSTL_ARCH_RISCV__)
	MSTL_IF_CONSTEXPR (sizeof(T) == 4) {
		asm volatile(
			"lr.w %[loaded], (%[ptr])\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.w %[success], %[desired], (%[ptr])\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 8) {
		asm volatile(
			"lr.d %[loaded], (%[ptr])\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.d %[success], %[desired], (%[ptr])\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "r" (ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	}
#elif defined(MSTL_ARCH_LOONGARCH__)
	MSTL_IF_CONSTEXPR (sizeof(T) == 4) {
		asm volatile(
			"ll.w %[loaded], %[ptr]\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.w %[desired], %[ptr]\n\t"
			"move %[success], %[desired]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "m" (*ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	} else MSTL_IF_CONSTEXPR (sizeof(T) == 8) {
		asm volatile(
			"ll.d %[loaded], %[ptr]\n\t"
			"bne %[loaded], %[old_val], 1f\n\t"
			"sc.d %[desired], %[ptr]\n\t"
			"move %[success], %[desired]\n\t"
			"1:"
			: [loaded] "=&r" (loaded), [success] "=&r" (success_flag)
			: [ptr] "m" (*ptr), [old_val] "r" (old_val), [desired] "r" (desired)
			: "memory");
	}
#endif
	if (loaded != old_val) {
		*expected = loaded;
		return false;
	}
	return success_flag == 0;
#endif
#endif
}

/**
 * @brief 通用强比较交换操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param expected 期望值的指针
 * @param desired 期望设置的值
 * @param success 成功时的内存顺序
 * @param failure 失败时的内存顺序
 * @return 是否交换成功
 * @note 强比较交换保证不伪失败
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE bool
atomic_cmpexch_strong_any(
	volatile T* ptr, remove_volatile_t<T>* expected,
	remove_volatile_t<T>* desired, const memory_order success, const memory_order failure) noexcept {
	MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_compare_exchange(
		ptr, expected, desired, false,
		static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
#if defined(MSTL_ARCH_X86__)
	const bool result = _INNER interlocked_compare_exchange_impl<sizeof(T)>::call(ptr, expected, *desired);
	if (success == memory_order_seq_cst || failure == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
	return result;
#else
	remove_volatile_t<T> old_val = *expected;
	while (true) {
		if (_INNER cmpexch_weak(ptr, expected, desired, success, failure)) {
			return true;
		}
		if (*expected != old_val) {
			return false;
		}
	}
#endif
#endif
}

/**
 * @brief 通用原子存储操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param value 要存储的值
 * @param mo 内存顺序
 * @note 用于非整数类型的原子存储
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE void
atomic_store_any(T* ptr, remove_volatile_t<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	__atomic_store(ptr, _MSTL addressof(value), static_cast<int32_t>(mo));
#else
	remove_volatile_t<T> expected = *ptr;
	while (!_MSTL atomic_cmpexch_weak_any(ptr, &expected, &value, mo, memory_order_relaxed)) {
		// Retry
	}
#endif
}

/**
 * @brief 通用原子加载操作
 * @tparam T 数据类型
 * @param ptr 源指针
 * @param mo 内存顺序
 * @return 加载的值
 * @note 用于非整数类型的原子加载
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_load_any(const T* ptr, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	alignas(T) byte_t buffer[sizeof(T)];
	T* dest = reinterpret_cast<remove_volatile_t<T>*>(buffer);
	__atomic_load(ptr, dest, static_cast<int32_t>(mo));
	return *dest;
#else
	remove_volatile_t<T> result;
	_MSTL memory_copy<remove_volatile_t<T>>(&result, ptr);
	if (mo == memory_order_seq_cst || mo == memory_order_acquire) {
		::_ReadWriteBarrier();
	}
	return result;
#endif
}

/**
 * @brief 通用原子交换操作
 * @tparam T 数据类型
 * @param ptr 目标指针
 * @param desired 要交换的值
 * @param mo 内存顺序
 * @return 交换前的值
 * @note 用于非整数类型的原子交换
 */
template <typename T>
MSTL_ALWAYS_INLINE_INLINE remove_volatile_t<T>
atomic_exchange_any(T* ptr, remove_volatile_t<T> desired, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	alignas(T) byte_t buffer[sizeof(T)];
	T* dest = reinterpret_cast<remove_volatile_t<T>*>(buffer);
	__atomic_exchange(ptr, _MSTL addressof(desired), dest, static_cast<int32_t>(mo));
	return *dest;
#else
	remove_volatile_t<T> old = _MSTL atomic_load(ptr, memory_order_relaxed);
	while (!_MSTL atomic_cmpexch_weak_any(ptr, &old, &desired, mo, memory_order_relaxed)) {
		// Retry
	}
	return old;
#endif
}

/**
 * @brief 通用原子获取并添加操作
 * @tparam T 浮点类型
 * @param ptr 目标指针
 * @param value 要添加的值
 * @param mo 内存顺序
 * @return 添加前的值
 */
template <typename T>
T atomic_fetch_add_any(T* ptr, remove_volatile_t<T> value, memory_order mo) noexcept {
	remove_volatile_t<T> old_value = _MSTL atomic_load(ptr, memory_order_relaxed);
	remove_volatile_t<T> new_value = old_value + value;
	while (!_MSTL atomic_cmpexch_weak_any(ptr, &old_value, &new_value, mo, memory_order_relaxed)) {
		new_value = old_value + value;
	}
	return old_value;
}

/**
 * @brief 通用原子获取并减去操作
 * @tparam T 浮点类型
 * @param ptr 目标指针
 * @param value 要减去的值
 * @param mo 内存顺序
 * @return 减去前的值
 */
template <typename T>
T atomic_fetch_sub_any(T* ptr, remove_volatile_t<T> value, memory_order mo) noexcept {
	remove_volatile_t<T> old_value = _MSTL atomic_load(ptr, memory_order_relaxed);
	remove_volatile_t<T> new_value = old_value - value;
	while (!_MSTL atomic_cmpexch_weak_any(ptr, &old_value, &new_value, mo, memory_order_relaxed)) {
		new_value = old_value - value;
	}
	return old_value;
}

/**
 * @brief 通用原子添加并获取操作
 * @tparam T 浮点类型
 * @param ptr 目标指针
 * @param value 要添加的值
 * @return 添加后的值
 */
template <typename T>
T atomic_add_fetch_any(T* ptr, remove_volatile_t<T> value) noexcept {
	remove_volatile_t<T> old_value = _MSTL atomic_load(ptr, memory_order_relaxed);
	remove_volatile_t<T> new_value = old_value + value;
	while (!_MSTL atomic_cmpexch_weak_any(ptr, &old_value, &new_value, memory_order_seq_cst, memory_order_relaxed)) {
		new_value = old_value + value;
	}
	return new_value;
}

/**
 * @brief 通用原子减去并获取操作
 * @tparam T 浮点类型
 * @param ptr 目标指针
 * @param value 要减去的值
 * @return 减去后的值
 */
template <typename T>
T atomic_sub_fetch_any(T* ptr, remove_volatile_t<T> value) noexcept {
	remove_volatile_t<T> old_value = _MSTL atomic_load(ptr, memory_order_relaxed);
	remove_volatile_t<T> new_value = old_value - value;
	while (!_MSTL atomic_cmpexch_weak_any(ptr, &old_value, &new_value, memory_order_seq_cst, memory_order_relaxed)) {
		new_value = old_value - value;
	}
	return new_value;
}


/**
 * @brief 检查是否支持无锁操作
 * @tparam Size 数据类型大小
 * @tparam Align 数据对齐要求
 * @return 是否支持无锁操作
 */
template <size_t Size, size_t Align>
constexpr bool is_always_lock_free() noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_is_lock_free(Size, reinterpret_cast<void *>(-Align));
#else
	return _INNER atomic_is_always_lock_free_impl<Size>::value;
#endif
}


/**
 * @struct atomic_flag
 * @brief 原子标志
 *
 * 最简单的原子类型。
 */
struct atomic_flag {
	/**
	 * @brief 原子标志类型
	 */
	using value_type =
#ifdef MSTL_PLATFORM_WINDOWS__
		long;
#else
		bool;
#endif

	value_type flag_;   ///< 原子标志值

	atomic_flag() noexcept = default;
	atomic_flag(const atomic_flag&) = delete;
	atomic_flag& operator =(const atomic_flag&) = delete;
	atomic_flag& operator =(const atomic_flag&) volatile = delete;
	atomic_flag(atomic_flag&&) noexcept = default;
	atomic_flag& operator =(atomic_flag&&) noexcept = default;
	~atomic_flag() noexcept = default;

	/**
	 * @brief 构造函数
	 * @param flag 初始标志值
	 */
	constexpr atomic_flag(const value_type flag) noexcept
	: flag_(static_cast<value_type>(flag ? 1 : 0))  {}

	/**
	 * @brief 测试并设置标志
	 * @param mo 内存顺序
	 * @return 设置前的标志值
	 */
	MSTL_ALWAYS_INLINE bool
	test_and_set(const memory_order mo = memory_order_seq_cst) noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_test_and_set(&flag_, static_cast<int32_t>(mo));
#else
		const long old_val = ::_InterlockedExchange(&flag_, 1);
		if (mo == memory_order_seq_cst) ::_ReadWriteBarrier();
		return old_val != 0;
#endif
	}

	/**
	 * @brief volatile版本的测试并设置标志
	 */
	MSTL_ALWAYS_INLINE_INLINE bool
	test_and_set(const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_test_and_set(&flag_, static_cast<int32_t>(mo));
#else
		const long old_val = ::_InterlockedExchange(&flag_, 1);
		if (mo == memory_order_seq_cst) ::_ReadWriteBarrier();
		return old_val != 0;
#endif
	}

	/**
	 * @brief 测试标志值
	 * @param mo 内存顺序
	 * @return 当前标志值
	 */
	MSTL_ALWAYS_INLINE bool
	test(const memory_order mo = memory_order_seq_cst) const noexcept {
#ifdef MSTL_COMPILER_GNUC__
		value_type value;
		__atomic_load(&flag_, &value, static_cast<int32_t>(mo));
		return value == sizeof(value_type);
#else
		const long as_bytes = flag_;
		if (mo != memory_order_relaxed) ::_ReadWriteBarrier();
		return as_bytes != 0;
#endif
	}

	/**
	 * @brief volatile版本的测试标志值
	 */
	MSTL_ALWAYS_INLINE_INLINE bool
	test(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		value_type value;
		__atomic_load(&flag_, &value, static_cast<int32_t>(mo));
		return value == sizeof(value_type);
#else
		const long as_bytes = flag_;
		if (mo != memory_order_relaxed) ::_ReadWriteBarrier();
		return as_bytes != 0;
#endif
	}

	/**
	 * @brief 等待标志值改变
	 * @param old 期望的旧值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(const bool old, const memory_order mo = memory_order_seq_cst) const noexcept {
		const value_type value = old ? 1 : 0;
		_MSTL atomic_wait_address_v(
			const_cast<const value_type*>(&flag_), value,
			[this, mo] { return this->test(mo); }
		);
	}

	/**
	 * @brief volatile版本的等待标志值改变
	 */
	MSTL_ALWAYS_INLINE_INLINE void
	wait(const bool old, const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		const value_type value = old ? 1 : 0;
		_MSTL atomic_wait_address_v(
			const_cast<const value_type*>(&flag_), value,
			[this, mo] { return this->test(mo); }
		);
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&flag_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&flag_, true);
	}

	/**
	 * @brief 清除标志
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	clear(const memory_order mo = memory_order_seq_cst) noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);

#ifdef MSTL_COMPILER_GNUC__
		__atomic_clear(&flag_, static_cast<int32_t>(mo));
#else
		_MSTL atomic_store(&flag_, static_cast<value_type>(0), mo);
#endif
	}

	/**
	 * @brief volatile版本的清除标志
	 */
	MSTL_ALWAYS_INLINE_INLINE void
	clear(const memory_order mo = memory_order_seq_cst) volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);

#ifdef MSTL_COMPILER_GNUC__
		__atomic_clear(&flag_, static_cast<int32_t>(mo));
#else
		_MSTL atomic_store(&flag_, static_cast<value_type>(0), mo);
#endif
	}
};


/**
 * @brief 原子类型基础模板类
 * @tparam T 值类型
 *
 * 提供整数类型的原子操作实现。
 */
template <typename T>
struct atomic_base {
	using value_type = T;       ///< 值类型
	using difference_type = T;  ///< 差值类型

	static_assert(is_integral_like_v<T>, "T must be an integral-like type");

private:
	static constexpr size_t align_inner = sizeof(T) > alignof(T) ? sizeof(T) : alignof(T);

	alignas(align_inner) value_type value_;  ///< 原子值存储

public:
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(T), align_inner>();

	atomic_base() noexcept = default;
	~atomic_base() noexcept = default;
	atomic_base(const atomic_base&) = delete;
	atomic_base& operator =(const atomic_base&) = delete;
	atomic_base& operator =(const atomic_base&) volatile = delete;
	atomic_base(atomic_base&&) noexcept = default;
	atomic_base& operator =(atomic_base&&) noexcept = default;

	/**
	 * @brief 构造函数
	 * @param value 初始值
	 */
	constexpr atomic_base(value_type value) noexcept
	: value_ (value) {}

	/**
	 * @brief 类型转换运算符
	 * @return 当前值
	 */
	operator value_type() const noexcept {
		return load();
	}

	/**
	 * @brief volatile版本的类型转换运算符
	 */
	operator value_type() const volatile noexcept {
		return load();
	}

	/**
	 * @brief 赋值运算符
	 * @param value 要设置的值
	 * @return 设置后的值
	 */
	value_type operator =(value_type value) noexcept {
		atomic_base::store(value);
		return value;
	}

	/**
	 * @brief volatile版本的赋值运算符
	 */
	value_type operator =(value_type value) volatile noexcept {
		atomic_base::store(value);
		return value;
	}

	/**
	 * @brief 后置递增运算符
	 * @return 递增前的值
	 */
	value_type operator ++(int) noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief volatile版本的后置递增运算符
	 */
	value_type operator ++(int) volatile noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief 后置递减运算符
	 * @return 递减前的值
	 */
	value_type operator --(int) noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief volatile版本的后置递减运算符
	 */
	value_type operator --(int) volatile noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief 前置递增运算符
	 * @return 递增后的值
	 */
	value_type operator ++() noexcept {
		return _MSTL atomic_add_fetch(&value_, 1, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的前置递增运算符
	 */
	value_type operator ++() volatile noexcept {
		return _MSTL atomic_add_fetch(&value_, 1, memory_order_seq_cst);
	}

	/**
	 * @brief 前置递减运算符
	 * @return 递减后的值
	 */
	value_type operator --() noexcept {
		return _MSTL atomic_sub_fetch(&value_, 1, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的前置递减运算符
	 */
	value_type operator --() volatile noexcept {
		return _MSTL atomic_sub_fetch(&value_, 1, memory_order_seq_cst);
	}

	/**
	 * @brief 加法赋值运算符
	 * @param value 要加的值
	 * @return 加法后的值
	 */
	value_type operator +=(value_type value) noexcept {
		return _MSTL atomic_add_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的加法赋值运算符
	 */
	value_type operator +=(value_type value) volatile noexcept {
		return _MSTL atomic_add_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief 减法赋值运算符
	 * @param value 要减的值
	 * @return 减法后的值
	 */
	value_type operator -=(value_type value) noexcept {
		return _MSTL atomic_sub_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的减法赋值运算符
	 */
	value_type operator -=(value_type value) volatile noexcept {
		return _MSTL atomic_sub_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief 位与赋值运算符
	 * @param value 要进行与操作的值
	 * @return 与操作后的值
	 */
	value_type operator &=(value_type value) noexcept {
		return _MSTL atomic_and_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的位与赋值运算符
	 */
	value_type operator &=(value_type value) volatile noexcept {
		return _MSTL atomic_and_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief 位或赋值运算符
	 * @param value 要进行或操作的值
	 * @return 或操作后的值
	 */
	value_type operator |=(value_type value) noexcept {
		return _MSTL atomic_or_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的位或赋值运算符
	 */
	value_type operator |=(value_type value) volatile noexcept {
		return _MSTL atomic_or_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief 位异或赋值运算符
	 * @param value 要进行异或操作的值
	 * @return 异或操作后的值
	 */
	value_type operator ^=(value_type value) noexcept {
		return _MSTL atomic_xor_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief volatile版本的位异或赋值运算符
	 */
	value_type operator ^=(value_type value) volatile noexcept {
		return _MSTL atomic_xor_fetch(&value_, value, memory_order_seq_cst);
	}

	/**
	 * @brief 检查是否支持无锁操作
	 * @return 是否支持无锁
	 */
	bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief volatile版本的检查是否支持无锁操作
	 */
	bool is_lock_free() const volatile noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储操作
	 * @param value 要存储的值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	store(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);
		_MSTL atomic_store(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子存储操作
	 */
	MSTL_ALWAYS_INLINE void
	store(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);
		_MSTL atomic_store(&value_, value, mo);
	}

	/**
	 * @brief 原子加载操作
	 * @param mo 内存顺序
	 * @return 加载的值
	 */
	MSTL_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_release);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		return _MSTL atomic_load(&value_, mo);
	}

	/**
	 * @brief volatile版本的原子加载操作
	 */
	MSTL_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_release);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		return _MSTL atomic_load(&value_, mo);
	}

	/**
	 * @brief 原子交换操作
	 * @param value 要交换的值
	 * @param mo 内存顺序
	 * @return 交换前的值
	 */
	MSTL_ALWAYS_INLINE value_type
	exchange(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_exchange(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子交换操作
	 */
	MSTL_ALWAYS_INLINE value_type
	exchange(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_exchange(&value_, value, mo);
	}

	/**
	 * @brief 弱比较交换操作
	 * @param expected 期望值
	 * @param desired 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_weak_any(&value_, &expected, &desired, success, failure);
	}

	/**
	 * @brief volatile版本的弱比较交换操作
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _MSTL atomic_cmpexch_weak_any(&value_, &expected, &desired, success, failure);
	}

	/**
	 * @brief 简化版弱比较交换操作
	 * @param expected 期望值
	 * @param desired 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return this->compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief volatile版本的简化版弱比较交换操作
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return this->compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 强比较交换操作
	 * @param expected 期望值
	 * @param desired 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_strong_any(&value_, &expected, &desired, success, failure);
	}

	/**
	 * @brief volatile版本的强比较交换操作
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _MSTL atomic_cmpexch_strong_any(&value_, &expected, &desired, success, failure);
	}

	/**
	 * @brief 简化版强比较交换操作
	 * @param expected 期望值
	 * @param desired 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return this->compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief volatile版本的简化版强比较交换操作
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return this->compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待值改变
	 * @param old 期望的旧值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(value_type old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(&value_, old, [mo, this] {
			return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&value_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&value_, true);
	}

	/**
	 * @brief 原子获取并添加操作
	 * @param value 要添加的值
	 * @param mo 内存顺序
	 * @return 添加前的值
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_add(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并添加操作
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_add(&value_, value, mo);
	}

	/**
	 * @brief 原子获取并减去操作
	 * @param value 要减去的值
	 * @param mo 内存顺序
	 * @return 减去前的值
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_sub(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并减去操作
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_sub(&value_, value, mo);
	}

	/**
	 * @brief 原子获取并与操作
	 * @param value 要进行与操作的值
	 * @param mo 内存顺序
	 * @return 操作前的值
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_and(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_and(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并与操作
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_and(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_and(&value_, value, mo);
	}

	/**
	 * @brief 原子获取并或操作
	 * @param value 要进行或操作的值
	 * @param mo 内存顺序
	 * @return 操作前的值
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_or(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_or(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并或操作
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_or(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_or(&value_, value, mo);
	}

	/**
	 * @brief 原子获取并异或操作
	 * @param value 要进行异或操作的值
	 * @param mo 内存顺序
	 * @return 操作前的值
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_xor(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_xor(&value_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并异或操作
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_xor(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_xor(&value_, value, mo);
	}
};

/**
 * @brief 指针类型的原子操作特化
 * @tparam T 指针指向的类型
 *
 * 提供指针类型的原子操作实现，支持指针算术运算。
 */
template <typename T>
struct atomic_base<T*> {
	using value_type = T*;   ///< 指针类型
	using difference_type = ptrdiff_t;   ///< 差值类型

private:
	value_type ptr_ = nullptr;  ///< 存储的指针

	/**
	 * @brief 计算实际类型大小
	 * @param dest 元素数量
	 * @return 字节大小
	 */
    MSTL_ALWAYS_INLINE_INLINE static constexpr difference_type
	real_type_sizes(const difference_type dest) noexcept {
		return dest * sizeof(T);
	}

public:
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(value_type), alignof(value_type)>();

	atomic_base() noexcept = default;
	atomic_base(const atomic_base&) = delete;
	atomic_base& operator =(const atomic_base&) = delete;
	atomic_base& operator =(const atomic_base&) volatile = delete;
	atomic_base(atomic_base&&) noexcept = default;
	atomic_base& operator =(atomic_base&&) noexcept = default;
	~atomic_base() noexcept = default;

	/**
	 * @brief 构造函数
	 * @param ptr 初始指针
	 */
	constexpr atomic_base(const value_type ptr) noexcept
	: ptr_(ptr) {}

	/**
	 * @brief 类型转换运算符
	 * @return 当前指针值
	 */
	operator value_type() const noexcept {
		return load();
	}

	/**
	 * @brief volatile版本的类型转换运算符
	 */
	operator value_type() const volatile noexcept {
		return load();
	}

	/**
	 * @brief 赋值运算符
	 * @param ptr 要设置的指针
	 * @return 设置后的指针
	 */
	value_type operator =(const value_type ptr) noexcept {
		atomic_base::store(ptr);
		return ptr;
	}

	/**
	 * @brief volatile版本的赋值运算符
	 */
	value_type operator =(const value_type ptr) volatile noexcept {
		atomic_base::store(ptr);
		return ptr;
	}

	/**
	 * @brief 后置递增运算符
	 * @return 递增前的指针
	 */
	value_type operator ++(int) noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief volatile版本的后置递增运算符
	 */
	value_type operator ++(int) volatile noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief 后置递减运算符
	 * @return 递减前的指针
	 */
	value_type operator --(int) noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief volatile版本的后置递减运算符
	 */
	value_type operator --(int) volatile noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief 前置递增运算符
	 * @return 递增后的指针
	 */
	value_type operator ++() noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_add_fetch(&ptr_, real_type_sizes(1), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
			::_interlockedexchangeadd64(
				reinterpret_cast<volatile long long*>(&ptr_),
				static_cast<long long>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr + sizeof(T));
#endif
	}

	/**
	 * @brief volatile版本的前置递增运算符
	 */
	value_type operator ++() volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_add_fetch(&ptr_, real_type_sizes(1), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_interlockedexchangeadd64(
				reinterpret_cast<volatile long long*>(&ptr_),
				static_cast<long long>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr + sizeof(T));
#endif
	}

	/**
	 * @brief 前置递减运算符
	 * @return 递减后的指针
	 */
	value_type operator --() noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_sub_fetch(&ptr_, real_type_sizes(1), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
			::_interlockedexchangeadd64(
				reinterpret_cast<volatile long long*>(&ptr_),
				-static_cast<ptrdiff_t>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr - sizeof(T));
#endif
	}

	/**
	 * @brief volatile版本的前置递减运算符
	 */
	value_type operator --() volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_sub_fetch(&ptr_, real_type_sizes(1), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_interlockedexchangeadd64(
				reinterpret_cast<volatile long long*>(&ptr_),
				-static_cast<ptrdiff_t>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr - sizeof(T));
#endif
	}

	/**
	 * @brief 指针加法赋值运算符
	 * @param dest 要增加的元素数量
	 * @return 增加后的指针
	 */
	value_type operator +=(const ptrdiff_t dest) noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_add_fetch(&ptr_, real_type_sizes(dest), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
			::_interlockedexchangeadd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(dest * sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr + dest * sizeof(T));
#endif
	}

	/**
	 * @brief volatile版本的指针加法赋值运算符
	 */
	value_type operator +=(const ptrdiff_t dest) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_add_fetch(&ptr_, real_type_sizes(dest), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_interlockedexchangeadd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(dest * sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr + dest * sizeof(T));
#endif
	}

	/**
	 * @brief 指针减法赋值运算符
	 * @param dest 要减少的元素数量
	 * @return 减少后的指针
	 */
	value_type operator -=(const ptrdiff_t dest) noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_sub_fetch(&ptr_, real_type_sizes(dest), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
			::_interlockedexchangeadd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			-dest * static_cast<ptrdiff_t>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr - dest * sizeof(T));
#endif
	}

	/**
	 * @brief volatile版本的指针减法赋值运算符
	 */
	value_type operator -=(const ptrdiff_t dest) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_sub_fetch(&ptr_, real_type_sizes(dest), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_interlockedexchangeadd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			-dest * static_cast<ptrdiff_t>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr - dest * sizeof(T));
#endif
	}

	/**
	 * @brief 检查是否支持无锁操作
	 * @return 是否支持无锁
	 */
	bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief volatile版本的检查是否支持无锁操作
	 */
	bool is_lock_free() const volatile noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储指针操作
	 * @param ptr 要存储的指针
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	store(const value_type ptr, const memory_order mo = memory_order_seq_cst) noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);

#ifdef MSTL_COMPILER_GNUC__
		__atomic_store_n(&ptr_, ptr, static_cast<int32_t>(mo));
#else
		::_InterlockedExchangePointer(
			reinterpret_cast<void* volatile*>(&ptr_), ptr);
		if (mo == memory_order_seq_cst || mo == memory_order_release) {
			::_ReadWriteBarrier();
		}
#endif
	}

	/**
	 * @brief volatile版本的原子存储指针操作
	 */
	MSTL_ALWAYS_INLINE_INLINE void
	store(const value_type ptr, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);

#ifdef MSTL_COMPILER_GNUC__
		__atomic_store_n(&ptr_, ptr, static_cast<int32_t>(mo));
#else
		::_InterlockedExchangePointer(
		    reinterpret_cast<void* volatile*>(&ptr_), ptr);
		if (mo == memory_order_seq_cst || mo == memory_order_release) {
			::_ReadWriteBarrier();
		}
#endif
	}

	/**
	 * @brief 原子加载指针操作
	 * @param mo 内存顺序
	 * @return 加载的指针
	 */
	MSTL_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_release);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_load_n(&ptr_, static_cast<int32_t>(mo));
#else
		const value_type result = *reinterpret_cast<value_type const volatile*>(&ptr_);
		if (mo == memory_order_seq_cst || mo == memory_order_acquire) {
			::_ReadWriteBarrier();
		}
		return result;
#endif
	}

	/**
	 * @brief volatile版本的原子加载指针操作
	 */
	MSTL_ALWAYS_INLINE_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_release);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_load_n(&ptr_, static_cast<int32_t>(mo));
#else
		const value_type result = *reinterpret_cast<value_type const volatile*>(&ptr_);
		if (mo == memory_order_seq_cst || mo == memory_order_acquire) {
			::_ReadWriteBarrier();
		}
		return result;
#endif
	}

	/**
	 * @brief 原子交换指针操作
	 * @param ptr 要交换的指针
	 * @param mo 内存顺序
	 * @return 交换前的指针
	 */
	MSTL_ALWAYS_INLINE value_type
	exchange(const value_type ptr, const memory_order mo = memory_order_seq_cst) noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_exchange_n(&ptr_, ptr, static_cast<int32_t>(mo));
#else
		const value_type old = static_cast<value_type>(
			::_InterlockedExchangePointer(
			reinterpret_cast<void* volatile*>(&ptr_), ptr));
		if (mo == memory_order_seq_cst) {
			::_ReadWriteBarrier();
		}
		return old;
#endif
	}

	/**
	 * @brief volatile版本的原子交换指针操作
	 */
	MSTL_ALWAYS_INLINE_INLINE value_type
	exchange(const value_type ptr, const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_exchange_n(&ptr_, ptr, static_cast<int32_t>(mo));
#else
		const value_type old = static_cast<value_type>(
		    ::_InterlockedExchangePointer(
			reinterpret_cast<void* volatile*>(&ptr_), ptr));
		if (mo == memory_order_seq_cst) {
			::_ReadWriteBarrier();
		}
		return old;
#endif
	}

	/**
	 * @brief 弱比较交换指针操作
	 * @param expected 期望指针
	 * @param desired 期望设置的指针
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
		return _MSTL atomic_cmpexch_weak_any(
			_MSTL addressof(ptr_), _MSTL addressof(expected),
			_MSTL addressof(desired), success, failure);
	}

	/**
	 * @brief volatile版本的弱比较交换指针操作
	 */
	MSTL_ALWAYS_INLINE_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
		return _MSTL atomic_cmpexch_weak_any(
			_MSTL addressof(ptr_), _MSTL addressof(expected),
			_MSTL addressof(desired), success, failure);
	}

	/**
	 * @brief 简化版弱比较交换指针操作
	 * @param expected 期望指针
	 * @param desired 期望设置的指针
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return atomic_base::compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief volatile版本的简化版弱比较交换指针操作
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(
		value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return atomic_base::compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 强比较交换指针操作
	 * @param expected 期望指针
	 * @param desired 期望设置的指针
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
		return _MSTL atomic_cmpexch_strong_any(
			_MSTL addressof(ptr_), _MSTL addressof(expected),
			_MSTL addressof(desired), success, failure);
	}

	/**
	 * @brief volatile版本的强比较交换指针操作
	 */
	MSTL_ALWAYS_INLINE_INLINE bool
	compare_exchange_strong(
		value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _MSTL atomic_cmpexch_strong_any(
			_MSTL addressof(ptr_), _MSTL addressof(expected),
			_MSTL addressof(desired), success, failure);
	}

	/**
	 * @brief 简化版强比较交换指针操作
	 * @param expected 期望指针
	 * @param desired 期望设置的指针
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return atomic_base::compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief volatile版本的简化版强比较交换指针操作
	 */
	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return atomic_base::compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待指针改变
	 * @param old 期望的旧指针
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(value_type old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(&ptr_, old, [mo, this] {
			return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&ptr_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&ptr_, true);
	}

	/**
	 * @brief 原子获取并添加指针偏移
	 * @param dest 要增加的元素数量
	 * @param mo 内存顺序
	 * @return 添加前的指针
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_add(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_add(&ptr_, dest * sizeof(T), mo);
	}

	/**
	 * @brief volatile版本的原子获取并添加指针偏移
	 */
	MSTL_ALWAYS_INLINE_INLINE value_type
	fetch_add(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_add(&ptr_, dest * sizeof(T), mo);
	}

	/**
	 * @brief 原子获取并减去指针偏移
	 * @param dest 要减少的元素数量
	 * @param mo 内存顺序
	 * @return 减去前的指针
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_sub(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_fetch_sub(&ptr_, real_type_sizes(dest), static_cast<int32_t>(mo));
#else
		const char* old_ptr = reinterpret_cast<char*>(
			::_interlockedexchangeadd64(
				reinterpret_cast<volatile long long*>(&ptr_),
				-dest * static_cast<ptrdiff_t>(sizeof(T))));
		if (mo == memory_order_seq_cst) {
			::_ReadWriteBarrier();
		}
		return reinterpret_cast<value_type>(old_ptr);
#endif
	}

	/**
	 * @brief volatile版本的原子获取并减去指针偏移
	 */
	MSTL_ALWAYS_INLINE_INLINE value_type
	fetch_sub(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_fetch_sub(&ptr_, real_type_sizes(dest), static_cast<int32_t>(mo));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_interlockedexchangeadd64(
				reinterpret_cast<volatile long long*>(&ptr_),
				-dest * static_cast<ptrdiff_t>(sizeof(T))));
		if (mo == memory_order_seq_cst) {
			::_ReadWriteBarrier();
		}
		return reinterpret_cast<value_type>(old_ptr);
#endif
	}
};


/**
 * @struct atomic_float_base
 * @brief 浮点数原子操作基础类
 * @tparam Float 浮点类型
 *
 * 提供浮点类型的原子操作实现，支持加减运算。
 */
template <typename Float>
struct atomic_float_base {
	static_assert(is_floating_point_v<Float>, "atomic_ref_base need floating point T");

	using value_type = Float;   ///< 值类型
	using difference_type = value_type;   ///< 差值类型

private:
	alignas(alignof(Float)) Float float_ = _MSTL initialize<Float>();   ///< 浮点数值存储
	
public:
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(Float), alignof(Float)>();

	atomic_float_base() = default;
	atomic_float_base(const atomic_float_base&) = delete;
	atomic_float_base& operator =(const atomic_float_base&) = delete;
	atomic_float_base& operator =(const atomic_float_base&) volatile = delete;
	atomic_float_base(atomic_float_base&&) noexcept = default;
	atomic_float_base& operator =(atomic_float_base&&) noexcept = default;

	/**
	 * @brief 构造函数
	 * @param value 初始值
	 */
	constexpr atomic_float_base(Float value)
	noexcept(is_nothrow_copy_constructible_v<Float>)
	: float_(value) {}

	/**
	 * @brief 赋值运算符
	 */
	Float operator =(Float value) noexcept {
		this->store(value);
		return value;
	}

	/**
	 * @brief volatile版本的赋值运算符
	 */
	Float operator =(Float value) volatile noexcept {
		this->store(value);
		return value;
	}

	/**
	 * @brief 检查是否支持无锁操作
	 */
	bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief volatile版本的检查是否支持无锁操作
	 */
	bool is_lock_free() const volatile noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储操作
	 */
	void store(Float value, const memory_order mo = memory_order_seq_cst) noexcept {
		_MSTL atomic_store_any(&float_, value, mo);
	}

	/**
	 * @brief volatile版本的原子存储操作
	 */
	void store(Float value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		_MSTL atomic_store_any(&float_, value, mo);
	}

	/**
	 * @brief 原子加载操作
	 */
	Float load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _MSTL atomic_load_any(&float_, mo);
	}

	/**
	 * @brief volatile版本的原子加载操作
	 */
	Float load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		return _MSTL atomic_load_any(&float_, mo);
	}

	/**
	 * @brief 类型转换运算符
	 */
	operator Float() const noexcept {
		return this->load();
	}

	/**
	 * @brief volatile版本的类型转换运算符
	 */
	operator Float() const volatile noexcept {
		return this->load();
	}

	/**
	 * @brief 原子交换操作
	 */
	Float exchange(Float desire, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_exchange_any(&float_, desire, mo);
	}

	/**
	 * @brief volatile版本的原子交换操作
	 */
	Float exchange(Float desire, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_exchange_any(&float_, desire, mo);
	}

	/**
	 * @brief 弱比较交换操作
	 */
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_weak(&float_, expected, desire, success, failure);
	}

	/**
	 * @brief volatile版本的弱比较交换操作
	 */
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _MSTL atomic_cmpexch_weak(&float_, expected, desire, success, failure);
	}

	/**
	 * @brief 强比较交换操作
	 */
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_strong(&float_, expected, desire, success, failure);
	}

	/**
	 * @brief volatile版本的强比较交换操作
	 */
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _MSTL atomic_cmpexch_strong(&float_, expected, desire, success, failure);
	}

	/**
	 * @brief 简化版弱比较交换操作
	 */
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief volatile版本的简化版弱比较交换操作
	 */
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_cmpexch_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 简化版强比较交换操作
	 */
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief volatile版本的简化版强比较交换操作
	 */
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_cmpexch_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待值改变
	 * @param old 期望的旧值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(Float old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(&float_, old, [mo, this] {
			return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&float_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&float_, true);
	}

	/**
	 * @brief 原子获取并添加操作
	 * @param value 要添加的值
	 * @param mo 内存顺序
	 * @return 添加前的值
	 */
	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_add_any(&float_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并添加操作
	 */
	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_add_any(&float_, value, mo);
	}

	/**
	 * @brief 原子获取并减去操作
	 * @param value 要减去的值
	 * @param mo 内存顺序
	 * @return 减去前的值
	 */
	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_sub_any(&float_, value, mo);
	}

	/**
	 * @brief volatile版本的原子获取并减去操作
	 */
	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _MSTL atomic_fetch_sub_any(&float_, value, mo);
	}

	/**
	 * @brief 加法赋值运算符
	 * @param value 要加的值
	 * @return 加法后的值
	 */
	value_type operator +=(value_type value) noexcept {
		return _MSTL atomic_add_fetch_any(&float_, value);
	}

	/**
	 * @brief volatile版本的加法赋值运算符
	 */
	value_type operator +=(value_type value) volatile noexcept {
		return _MSTL atomic_add_fetch_any(&float_, value);
	}

	/**
	 * @brief 减法赋值运算符
	 * @param value 要减的值
	 * @return 减法后的值
	 */
	value_type operator -=(value_type value) noexcept {
		return _MSTL atomic_sub_fetch_any(&float_, value);
	}

	/**
	 * @brief volatile版本的减法赋值运算符
	 */
	value_type operator -=(value_type value) volatile noexcept {
		return _MSTL atomic_sub_fetch_any(&float_, value);
	}
};


/**
 * @struct atomic_ref_base
 * @brief 原子引用基础模板类
 * @tparam T 值类型
 * @tparam IsIntegral 是否为整数类型
 * @tparam IsFloatingPoint 是否为浮点类型
 *
 * 提供对现有变量的原子引用操作。
 */
template <typename T, bool IsIntegral = is_integral_v<T>, bool IsFloatingPoint = is_floating_point_v<T>>
struct atomic_ref_base;


/**
 * @brief 通用类型的原子引用特化
 * @tparam T 可平凡复制类型
 */
template <typename T>
struct atomic_ref_base<T, false, false> {
	static_assert(is_trivially_copyable_v<T>, "atomic_ref_base need trivially copyable T");

private:
	static constexpr int align_inner = (sizeof(T) & (sizeof(T) - 1)) || sizeof(T) > 16 ? 0 : sizeof(T);

	T* ptr_;   ///< 指向被引用对象的指针

public:
	using value_type = T;   ///< 值类型

	/// @brief 对齐需求
	static constexpr size_t required_alignment = align_inner > alignof(T) ? align_inner : alignof(T);
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(T), required_alignment>();

	/**
	 * @brief 构造函数
	 * @param value 被引用的对象
	 */
	explicit atomic_ref_base(T& value) : ptr_(_MSTL addressof(value)) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	atomic_ref_base(const atomic_ref_base&) noexcept = default;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	/**
	 * @brief 赋值运算符
	 * @param value 要设置的值
	 * @return 设置后的值
	 */
	T operator =(T value) noexcept {
		this->store(value);
		return value;
	}

	/**
	 * @brief 类型转换运算符
	 * @return 当前值
	 */
	operator T() const noexcept {
		return this->load();
	}

	/**
	 * @brief 检查是否支持无锁操作
	 * @return 是否支持无锁
	 */
	bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储操作
	 * @param value 要存储的值
	 * @param mo 内存顺序
	 */
	void store(T value, const memory_order mo = memory_order_seq_cst) noexcept {
		_MSTL atomic_store_any(ptr_, value, mo);
	}

	/**
	 * @brief 原子加载操作
	 * @param mo 内存顺序
	 * @return 加载的值
	 */
	T load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _MSTL atomic_load_any(ptr_, mo);
	}

	/**
	 * @brief 原子交换操作
	 * @param desire 要交换的值
	 * @param mo 内存顺序
	 * @return 交换前的值
	 */
	T exchange(T desire, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_exchange_any(ptr_, desire, mo);
	}

	/**
	 * @brief 弱比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(T& expected, T desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_weak(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 强比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(T& expected, T desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_strong(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 简化版弱比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 简化版强比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待值改变
	 * @param old 期望的旧值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(T old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
			return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}
};

/**
 * @brief 整数类型的原子引用特化
 * @tparam T 整数类型
 */
template <typename T>
struct atomic_ref_base<T, true, false> {
	static_assert(is_integral_like_v<T>, "atomic_ref need integral-like T");

private:

	T* ptr_;   ///< 指向被引用整数的指针

public:
	using value_type = T;  ///< 值类型
	using difference_type = value_type;  ///< 差值类型

	/// @brief 对齐需求
	static constexpr size_t required_alignment = sizeof(T) > alignof(T) ? sizeof(T) : alignof(T);
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(T), required_alignment>();

	atomic_ref_base() = delete;
	atomic_ref_base(const atomic_ref_base&) noexcept = default;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	/**
	 * @brief 构造函数
	 * @param value 被引用的整数
	 */
	explicit atomic_ref_base(T& value) : ptr_(&value) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	/**
	 * @brief 赋值运算符
	 * @param value 要设置的值
	 * @return 设置后的值
	 */
	T operator =(T value) noexcept {
		this->store(value);
		return value;
	}

	/**
	 * @brief 类型转换运算符
	 * @return 当前值
	 */
	MSTL_NODISCARD operator T() const noexcept {
		return this->load();
	}

	/**
	 * @brief 检查是否支持无锁操作
	 * @return 是否支持无锁
	 */
	MSTL_NODISCARD bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储操作
	 * @param value 要存储的值
	 * @param mo 内存顺序
	 */
	void store(T value, const memory_order mo = memory_order_seq_cst) noexcept {
		_MSTL atomic_store(ptr_, value, mo);
	}

	/**
	 * @brief 原子加载操作
	 * @param mo 内存顺序
	 * @return 加载的值
	 */
	T load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _MSTL atomic_load(ptr_, mo);
	}

	/**
	 * @brief 原子交换操作
	 * @param desire 要交换的值
	 * @param mo 内存顺序
	 * @return 交换前的值
	 */
	T exchange(T desire, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_exchange(ptr_, desire, mo);
	}

	/**
	 * @brief 弱比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(T& expected, T desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_weak(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 强比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(T& expected, T desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_strong(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 简化版弱比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 简化版强比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待值改变
	 * @param old 期望的旧值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(T old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
		    return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}

	/**
	 * @brief 原子获取并添加操作
	 * @param value 要添加的值
	 * @param mo 内存顺序
	 * @return 添加前的值
	 */
	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_add(ptr_, value, mo);
	}

	/**
	 * @brief 原子获取并减去操作
	 * @param value 要减去的值
	 * @param mo 内存顺序
	 * @return 减去前的值
	 */
	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_sub(ptr_, value, mo);
	}

	/**
	 * @brief 原子获取并与操作
	 * @param value 要进行与操作的值
	 * @param mo 内存顺序
	 * @return 操作前的值
	 */
	value_type fetch_and(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_and(ptr_, value, mo);
	}

	/**
	 * @brief 原子获取并或操作
	 * @param value 要进行或操作的值
	 * @param mo 内存顺序
	 * @return 操作前的值
	 */
	value_type fetch_or(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_or(ptr_, value, mo);
	}

	/**
	 * @brief 原子获取并异或操作
	 * @param value 要进行异或操作的值
	 * @param mo 内存顺序
	 * @return 操作前的值
	 */
	value_type fetch_xor(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_xor(ptr_, value, mo);
	}

	/**
	 * @brief 后置递增运算符
	 * @return 递增前的值
	 */
	MSTL_ALWAYS_INLINE value_type operator ++(int) noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief 后置递减运算符
	 * @return 递减前的值
	 */
	MSTL_ALWAYS_INLINE value_type operator --(int) noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief 前置递增运算符
	 * @return 递增后的值
	 */
	value_type operator ++() noexcept {
		return _MSTL atomic_add_fetch(ptr_, value_type(1));
	}

	/**
	 * @brief 前置递减运算符
	 * @return 递减后的值
	 */
	value_type operator --() noexcept {
		return _MSTL atomic_sub_fetch(ptr_, value_type(1));
	}

	/**
	 * @brief 加法赋值运算符
	 * @param value 要加的值
	 * @return 加法后的值
	 */
	value_type operator +=(value_type value) noexcept {
		return _MSTL atomic_add_fetch(ptr_, value);
	}

	/**
	 * @brief 减法赋值运算符
	 * @param value 要减的值
	 * @return 减法后的值
	 */
	value_type operator -=(value_type value) noexcept {
		return _MSTL atomic_sub_fetch(ptr_, value);
	}

	/**
	 * @brief 位与赋值运算符
	 * @param value 要进行与操作的值
	 * @return 与操作后的值
	 */
	value_type operator &=(value_type value) noexcept {
		return _MSTL atomic_and_fetch(ptr_, value);
	}

	/**
	 * @brief 位或赋值运算符
	 * @param value 要进行或操作的值
	 * @return 或操作后的值
	 */
	value_type operator |=(value_type value) noexcept {
		return _MSTL atomic_or_fetch(ptr_, value);
	}

	/**
	 * @brief 位异或赋值运算符
	 * @param value 要进行异或操作的值
	 * @return 异或操作后的值
	 */
	value_type operator ^=(value_type value) noexcept {
		return _MSTL atomic_xor_fetch(ptr_, value);
	}
};

/**
 * @brief 浮点类型的原子引用特化
 * @tparam Float 浮点类型
 */
template <typename Float>
struct atomic_ref_base<Float, false, true> {
	static_assert(is_floating_point_v<Float>, "atomic_ref_base need floating point T");

private:
	Float* ptr_;  ///< 指向被引用浮点数的指针

public:
	using value_type = Float;           ///< 值类型
	using difference_type = value_type;  ///< 差值类型

	/// @brief 对齐需求
	static constexpr size_t required_alignment = alignof(Float);
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(Float), required_alignment>();

	atomic_ref_base() = delete;
	atomic_ref_base(const atomic_ref_base&) noexcept = default;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	/**
	 * @brief 构造函数
	 * @param value 被引用的浮点数
	 */
	explicit atomic_ref_base(Float& value) : ptr_(&value) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	/**
	 * @brief 赋值运算符
	 * @param value 要设置的值
	 * @return 设置后的值
	 */
	Float operator =(Float value) noexcept {
		this->store(value);
		return value;
	}

	/**
	 * @brief 类型转换运算符
	 * @return 当前值
	 */
	operator Float() const noexcept {
		return this->load();
	}

	/**
	 * @brief 检查是否支持无锁操作
	 * @return 是否支持无锁
	 */
	bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储操作
	 * @param value 要存储的值
	 * @param mo 内存顺序
	 */
	void store(Float value, const memory_order mo = memory_order_seq_cst) noexcept {
		_MSTL atomic_store_any(ptr_, value, mo);
	}

	/**
	 * @brief 原子加载操作
	 * @param mo 内存顺序
	 * @return 加载的值
	 */
	Float load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _MSTL atomic_load_any(ptr_, mo);
	}

	/**
	 * @brief 原子交换操作
	 * @param desire 要交换的值
	 * @param mo 内存顺序
	 * @return 交换前的值
	 */
	Float exchange(Float desire, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_exchange_any(ptr_, desire, mo);
	}

	/**
	 * @brief 弱比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_weak(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 强比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_strong(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 简化版弱比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 简化版强比较交换操作
	 * @param expected 期望值
	 * @param desire 期望设置的值
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待值改变
	 * @param old 期望的旧值
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(Float old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
			return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}

	/**
	 * @brief 原子获取并添加操作
	 * @param value 要添加的值
	 * @param mo 内存顺序
	 * @return 添加前的值
	 */
	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_add_any(ptr_, value, mo);
	}

	/**
	 * @brief 原子获取并减去操作
	 * @param value 要减去的值
	 * @param mo 内存顺序
	 * @return 减去前的值
	 */
	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_sub_any(ptr_, value, mo);
	}

	/**
	 * @brief 加法赋值运算符
	 * @param value 要加的值
	 * @return 加法后的值
	 */
	value_type operator +=(value_type value) const noexcept {
		return _MSTL atomic_add_fetch_any(ptr_, value);
	}

	/**
	 * @brief 减法赋值运算符
	 * @param value 要减的值
	 * @return 减法后的值
	 */
	value_type operator -=(value_type value) const noexcept {
		return _MSTL atomic_sub_fetch_any(ptr_, value);
	}
};


#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Watomic-alignment"
#endif

template <typename T>
struct atomic_ref_base<T*, false, false> {
public:
	using value_type = T*;  ///< 指针类型
	using difference_type = ptrdiff_t;  ///< 差值类型

private:
	T** ptr_;  ///< 指向指针的指针

	/**
	 * @brief 计算实际类型大小
	 * @param dest 元素数量
	 * @return 字节大小
	 */
	static constexpr difference_type real_type_sizes(const difference_type dest) noexcept {
		static_assert(is_object_v<T>, "atomic_ref_base need object T");
		return dest * sizeof(T);
	}

public:
	/// @brief 对齐需求
	static constexpr size_t required_alignment = sizeof(T*) == 8 ? 8 : alignof(T*);
	/// @brief 是否总是无锁
	static constexpr bool is_always_lock_free = _MSTL is_always_lock_free<sizeof(T*), required_alignment>();

	atomic_ref_base() = delete;
	atomic_ref_base(const atomic_ref_base&) noexcept = default;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	/**
	 * @brief 构造函数
	 * @param value 被引用的指针
	 */
	explicit atomic_ref_base(T*& value) : ptr_(_MSTL addressof(value)) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	/**
	 * @brief 赋值运算符
	 * @param value 要设置的指针
	 * @return 设置后的指针
	 */
	T* operator =(T* value) noexcept {
		this->store(value);
		return value;
	}

	/**
	 * @brief 类型转换运算符
	 * @return 当前指针
	 */
	operator T*() const noexcept {
		return this->load();
	}

	/**
	 * @brief 检查是否支持无锁操作
	 * @return 是否支持无锁
	 */
	bool is_lock_free() const noexcept {
		return is_always_lock_free;
	}

	/**
	 * @brief 原子存储指针操作
	 * @param value 要存储的指针
	 * @param mo 内存顺序
	 */
	void store(T* value, const memory_order mo = memory_order_seq_cst) noexcept {
		_MSTL atomic_store_any(ptr_, value, mo);
	}

	/**
	 * @brief 原子加载指针操作
	 * @param mo 内存顺序
	 * @return 加载的指针
	 */
	T* load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _MSTL atomic_load_any(ptr_, mo);
	}

	/**
	 * @brief 原子交换指针操作
	 * @param desire 要交换的指针
	 * @param mo 内存顺序
	 * @return 交换前的指针
	 */
	T* exchange(T* desire, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_exchange_any(ptr_, desire, mo);
	}

	/**
	 * @brief 弱比较交换指针操作
	 * @param expected 期望指针
	 * @param desire 期望设置的指针
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(T*& expected, T* desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_weak(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 强比较交换指针操作
	 * @param expected 期望指针
	 * @param desire 期望设置的指针
	 * @param success 成功时的内存顺序
	 * @param failure 失败时的内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(T*& expected, T* desire,
		const memory_order success, const memory_order failure) noexcept {
		return _MSTL atomic_cmpexch_strong(ptr_, expected, desire, success, failure);
	}

	/**
	 * @brief 简化版弱比较交换指针操作
	 * @param expected 期望指针
	 * @param desire 期望设置的指针
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_weak(T*& expected, T* desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 简化版强比较交换指针操作
	 * @param expected 期望指针
	 * @param desire 期望设置的指针
	 * @param mo 内存顺序
	 * @return 是否交换成功
	 */
	bool compare_exchange_strong(T*& expected, T* desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_cmpexch_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}

	/**
	 * @brief 等待指针改变
	 * @param old 期望的旧指针
	 * @param mo 内存顺序
	 */
	MSTL_ALWAYS_INLINE void
	wait(T* old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
			return this->load(mo);
		});
	}

	/**
	 * @brief 通知一个等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	/**
	 * @brief 通知所有等待线程
	 */
	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}

	/**
	 * @brief 原子获取并添加指针偏移
	 * @param dest 要增加的元素数量
	 * @param mo 内存顺序
	 * @return 添加前的指针
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_add(const difference_type dest, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_add_any(ptr_, real_type_sizes(dest), mo);
	}

	/**
	 * @brief 原子获取并减去指针偏移
	 * @param dest 要减少的元素数量
	 * @param mo 内存顺序
	 * @return 减去前的指针
	 */
	MSTL_ALWAYS_INLINE value_type
	fetch_sub(const difference_type dest, const memory_order mo = memory_order_seq_cst) noexcept {
		return _MSTL atomic_fetch_sub_any(ptr_, real_type_sizes(dest), mo);
	}

	/**
	 * @brief 后置递增运算符
	 * @return 递增前的指针
	 */
	value_type operator ++(int) noexcept {
		return fetch_add(1);
	}

	/**
	 * @brief 后置递减运算符
	 * @return 递减前的指针
	 */
	value_type operator --(int) noexcept {
		return fetch_sub(1);
	}

	/**
	 * @brief 前置递增运算符
	 * @return 递增后的指针
	 */
	value_type operator ++() noexcept {
		return _MSTL atomic_add_fetch_any(ptr_, real_type_sizes(1));
	}

	/**
	 * @brief 前置递减运算符
	 * @return 递减后的指针
	 */
	value_type operator --() noexcept {
		return _MSTL atomic_sub_fetch_any(ptr_, real_type_sizes(1));
	}

	/**
	 * @brief 指针加法赋值运算符
	 * @param dest 要增加的元素数量
	 * @return 增加后的指针
	 */
	value_type operator +=(const difference_type dest) noexcept {
	    return _MSTL atomic_add_fetch_any(ptr_, real_type_sizes(dest));
	}

	/**
	 * @brief 指针减法赋值运算符
	 * @param dest 要减少的元素数量
	 * @return 减少后的指针
	 */
	value_type operator -=(const difference_type dest) noexcept {
	    return _MSTL atomic_sub_fetch_any(ptr_, real_type_sizes(dest));
	}
};

#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic pop
#endif

/** @} */ // AtomicOperations

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_BASE_HPP__
