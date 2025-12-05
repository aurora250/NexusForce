#ifndef MSTL_CORE_ASYNC_ATOMIC_BASE_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_BASE_HPP__
#include "../exception/assertion.hpp"
#include "atomic_wait.hpp"
MSTL_BEGIN_NAMESPACE__

enum class memory_order : int32_t {
    relaxed,
    consume,
    acquire,
    release,
    acq_rel,
    seq_cst
};

MSTL_INLINE17 constexpr auto memory_order_relaxed = memory_order::relaxed;
MSTL_INLINE17 constexpr auto memory_order_consume = memory_order::consume;
MSTL_INLINE17 constexpr auto memory_order_acquire = memory_order::acquire;
MSTL_INLINE17 constexpr auto memory_order_release = memory_order::release;
MSTL_INLINE17 constexpr auto memory_order_acq_rel = memory_order::acq_rel;
MSTL_INLINE17 constexpr auto memory_order_seq_cst = memory_order::seq_cst;


enum class memory_order_modifier : int64_t {
    memory_order_mask          = 0x0ffff,
    memory_order_modifier_mask = 0xffff0000,
    memory_order_hle_acquire   = 0x10000,
    memory_order_hle_release   = 0x20000
};

constexpr memory_order operator |(memory_order mo, memory_order_modifier mod) noexcept {
    return static_cast<memory_order>(static_cast<int64_t>(mo) | static_cast<int64_t>(mod));
}

constexpr memory_order operator &(memory_order mo, memory_order_modifier mod) noexcept {
    return static_cast<memory_order>(static_cast<int64_t>(mo) & static_cast<int64_t>(mod));
}


constexpr memory_order cmpexch_failure_order2(const memory_order mo) noexcept {
    return mo == memory_order_acq_rel ? memory_order_acquire
         : mo == memory_order_release ? memory_order_relaxed : mo;
}

constexpr memory_order cmpexch_failure_order(const memory_order mo) noexcept {
    return cmpexch_failure_order2(mo & memory_order_modifier::memory_order_mask) |
        static_cast<memory_order_modifier>(mo & memory_order_modifier::memory_order_modifier_mask);
}

constexpr bool is_valid_cmpexch_failure_order(const memory_order mo) noexcept {
    return (mo & memory_order_modifier::memory_order_mask) != memory_order_release
        && (mo & memory_order_modifier::memory_order_mask) != memory_order_acq_rel;
}


#ifdef MSTL_PLATFORM_WINDOWS__
MSTL_INLINE17 constexpr byte_t ATOMIC_TS_TRUEVAL = 4;
#else
MSTL_INLINE17 constexpr byte_t ATOMIC_TS_TRUEVAL = 1;
#endif
MSTL_INLINE17 constexpr bool ATOMIC_TS_IS_BOOL = ATOMIC_TS_TRUEVAL == 1;


MSTL_BEGIN_INNER__

#ifdef MSTL_COMPILER_MSVC__

template <typename Int, typename T>
MSTL_ALWAYS_INLINE Int __atomic_reinterpret_impl(const T& value, true_type, true_type) {
	return static_cast<Int>(value);
}

template <typename Int, typename T>
MSTL_ALWAYS_INLINE Int __atomic_reinterpret_impl(const T& value, false_type, true_type) {
	return reinterpret_cast<Int>(value);
}

template <typename Int, typename T, typename Cond1, typename Cond2>
Int __atomic_reinterpret_impl(const T& value, Cond1, Cond2) {
	Int result{};
	_MSTL memory_copy(&result, _MSTL addressof(value), sizeof(value));
	return result;
}

template <typename Int, typename T>
MSTL_ALWAYS_INLINE Int __atomic_reinterpret_as(const T& value) noexcept {
	static_assert(is_integral_v<Int>, "Int must be an integral type");
	return _INNER __atomic_reinterpret_impl<Int>(value,
	    bool_constant<is_integral_v<T> && sizeof(Int) == sizeof(T)>(),
	    bool_constant<is_pointer_v<T> && sizeof(Int) == sizeof(T)>()
	);
}


MSTL_ALWAYS_INLINE void apply_memory_order_load(const memory_order mo) noexcept {
	if (mo == memory_order_seq_cst || mo == memory_order_acquire) {
		::_ReadWriteBarrier();
	}
}

MSTL_ALWAYS_INLINE void apply_memory_order_store(const memory_order mo) noexcept {
	if (mo == memory_order_seq_cst || mo == memory_order_release) {
		::_ReadWriteBarrier();
	}
}

MSTL_ALWAYS_INLINE void apply_memory_order_seq_cst(const memory_order mo) noexcept {
	if (mo == memory_order_seq_cst) {
		::_ReadWriteBarrier();
	}
}

#endif


#ifdef MSTL_COMPILER_GNUC__
template<size_t Size>
struct atomic_is_always_lock_free_impl {
	static constexpr bool value = __atomic_always_lock_free(Size, nullptr);
};
#else
template <size_t> struct atomic_is_always_lock_free_impl {
	static constexpr bool value = false;
};
template <> struct atomic_is_always_lock_free_impl<1> {
	static constexpr bool value = true;
};
template <> struct atomic_is_always_lock_free_impl<2> {
	static constexpr bool value = true;
};
template <> struct atomic_is_always_lock_free_impl<4> {
	static constexpr bool value = true;
};
template <> struct atomic_is_always_lock_free_impl<8> {
	static constexpr bool value = true;
};
#endif

template <size_t Size>
constexpr bool atomic_is_always_lock_free = atomic_is_always_lock_free_impl<Size>::value;


template <typename T>
using __atomic_raw_value = remove_volatile_t<T>;

template <typename T>
using __atomic_diff = conditional_t<is_pointer_v<T>, ptrdiff_t, __atomic_raw_value<T>>;


template <size_t Size, size_t Align>
bool is_lock_free_impl() noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_is_lock_free(Size, reinterpret_cast<void *>(-Align));
#else
	return atomic_is_always_lock_free<Size>;
#endif
}


#ifdef MSTL_COMPILER_MSVC__

template <size_t Size>
struct interlocked_exchange_impl;

template <>
struct interlocked_exchange_impl<1> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(value)));
	}
};

template <>
struct interlocked_exchange_impl<2> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(value)));
	}
};

template <>
struct interlocked_exchange_impl<4> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(value)));
	}
};

template <>
struct interlocked_exchange_impl<8> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchange64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


template <size_t Size>
struct interlocked_compare_exchange_impl;

template <>
struct interlocked_compare_exchange_impl<1> {
	template <typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		char old = ::_InterlockedCompareExchange8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(desired),
		    static_cast<char>(*expected));
		if (old == static_cast<char>(*expected)) return true;
		*expected = static_cast<T>(old);
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<2> {
	template<typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		short old = ::_InterlockedCompareExchange16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(desired),
		    static_cast<short>(*expected));
		if (old == static_cast<short>(*expected)) return true;
		*expected = static_cast<T>(old);
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<4> {
	template<typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		long old = ::_InterlockedCompareExchange(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(desired),
		    static_cast<long>(*expected));
		if (old == static_cast<long>(*expected)) return true;
		*expected = static_cast<T>(old);
		return false;
	}
};

template <>
struct interlocked_compare_exchange_impl<8> {
	template<typename T>
	static bool call(volatile T* target, T* expected, T desired) {
		long long old = ::_InterlockedCompareExchange64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(desired),
		    static_cast<long long>(*expected));
		if (old == static_cast<long long>(*expected)) return true;
		*expected = static_cast<T>(old);
		return false;
	}
};


template <size_t Size>
struct interlocked_fetch_add_impl;

template <>
struct interlocked_fetch_add_impl<1> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(value)));
	}
};

template <>
struct interlocked_fetch_add_impl<2> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(value)));
	}
};

template <>
struct interlocked_fetch_add_impl<4> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(value)));
	}
};

template <>
struct interlocked_fetch_add_impl<8> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedExchangeAdd64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


template <size_t Size>
struct interlocked_fetch_and_impl;

template <>
struct interlocked_fetch_and_impl<1> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd8(
		    reinterpret_cast<volatile char*>(target),
		    static_cast<char>(value)));
	}
};

template <>
struct interlocked_fetch_and_impl<2> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd16(
		    reinterpret_cast<volatile short*>(target),
		    static_cast<short>(value)));
	}
};

template <>
struct interlocked_fetch_and_impl<4> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd(
		    reinterpret_cast<volatile long*>(target),
		    static_cast<long>(value)));
	}
};

template <>
struct interlocked_fetch_and_impl<8> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedAnd64(
		    reinterpret_cast<volatile long long*>(target),
		    static_cast<long long>(value)));
	}
};


template <size_t Size>
struct interlocked_fetch_or_impl;

template <>
struct interlocked_fetch_or_impl<1> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr8(
		    reinterpret_cast<volatile char*>(target), static_cast<char>(value)));
	}
};
template <>
struct interlocked_fetch_or_impl<2> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr16(
		    reinterpret_cast<volatile short*>(target), static_cast<short>(value)));
	}
};
template <>
struct interlocked_fetch_or_impl<4> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr(
		    reinterpret_cast<volatile long*>(target), static_cast<long>(value)));
	}
};
template <>
struct interlocked_fetch_or_impl<8> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedOr64(
		    reinterpret_cast<volatile long long*>(target), static_cast<long long>(value)));
	}
};


template <size_t Size>
struct interlocked_fetch_xor_impl;

template <>
struct interlocked_fetch_xor_impl<1> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor8(
		    reinterpret_cast<volatile char*>(target), static_cast<char>(value)));
	}
};
template <>
struct interlocked_fetch_xor_impl<2> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor16(
		    reinterpret_cast<volatile short*>(target), static_cast<short>(value)));
	}
};
template <>
struct interlocked_fetch_xor_impl<4> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor(
		    reinterpret_cast<volatile long*>(target), static_cast<long>(value)));
	}
};
template <>
struct interlocked_fetch_xor_impl<8> {
	template<typename T>
	static T call(volatile T* target, T value) {
		return static_cast<T>(::_InterlockedXor64(
		    reinterpret_cast<volatile long long*>(target), static_cast<long long>(value)));
	}
};

#endif


#ifdef MSTL_COMPILER_MSVC__
#define ATOMIC_ALWAYS_INLINE
#else
#define ATOMIC_ALWAYS_INLINE MSTL_ALWAYS_INLINE inline
#endif


template <typename T>
ATOMIC_ALWAYS_INLINE void
store(volatile T* ptr, __atomic_raw_value<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    __atomic_store_n(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER interlocked_exchange_impl<sizeof(T)>::call(ptr, value);
    _INNER apply_memory_order_store(mo);
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
load(const volatile T* ptr, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_load_n(ptr, static_cast<int32_t>(mo));
#else
    _INNER __atomic_raw_value<T> result = *ptr;
    _INNER apply_memory_order_load(mo);
    return result;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
exchange(volatile T* ptr, __atomic_raw_value<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_exchange_n(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER __atomic_raw_value<T> old = _INNER interlocked_exchange_impl<sizeof(T)>::call(ptr, value);
    _INNER apply_memory_order_seq_cst(mo);
    return old;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE bool
compare_exchange_weak(
	volatile T* ptr, __atomic_raw_value<T>* expected,
	__atomic_raw_value<T> desired, const memory_order success, const memory_order failure) noexcept {
    MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_compare_exchange_n(ptr, expected, desired, 1,
                                       static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
    const bool result = _INNER interlocked_compare_exchange_impl<sizeof(T)>::call(ptr, expected, desired);
    if (success == memory_order_seq_cst || failure == memory_order_seq_cst) {
        ::_ReadWriteBarrier();
    }
    return result;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE bool
compare_exchange_strong(
	volatile T* ptr, __atomic_raw_value<T>* expected,
	__atomic_raw_value<T> desired, const memory_order success, const memory_order failure) noexcept {
    MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
    return _INNER compare_exchange_weak(ptr, expected, desired, success, failure);
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
fetch_add(volatile T* ptr, __atomic_diff<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_add(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER __atomic_raw_value<T> old = _INNER interlocked_fetch_add_impl<sizeof(T)>::call(ptr, value);
    _INNER apply_memory_order_seq_cst(mo);
    return old;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
fetch_sub(volatile T* ptr, __atomic_diff<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_sub(ptr, value, static_cast<int32_t>(mo));
#else
    return _INNER fetch_add(ptr, static_cast<__atomic_diff<T>>(-value), mo);
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
fetch_and(volatile T* ptr, __atomic_raw_value<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_and(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER __atomic_raw_value<T> old = _INNER interlocked_fetch_and_impl<sizeof(T)>::call(ptr, value);
    _INNER apply_memory_order_seq_cst(mo);
    return old;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
fetch_or(volatile T* ptr, __atomic_raw_value<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_or(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER __atomic_raw_value<T> old = _INNER interlocked_fetch_or_impl<sizeof(T)>::call(ptr, value);
    _INNER apply_memory_order_seq_cst(mo);
    return old;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
fetch_xor(volatile T* ptr, __atomic_raw_value<T> value, const memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
    return __atomic_fetch_xor(ptr, value, static_cast<int32_t>(mo));
#else
    _INNER __atomic_raw_value<T> old = _INNER interlocked_fetch_xor_impl<sizeof(T)>::call(ptr, value);
    _INNER apply_memory_order_seq_cst(mo);
    return old;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
add_fetch(volatile T* ptr, __atomic_diff<T> value, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_add_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _INNER fetch_add(ptr, value, mo) + value;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
sub_fetch(volatile T* ptr, __atomic_diff<T> value, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_sub_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _INNER fetch_sub(ptr, value, mo) - value;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
and_fetch(volatile T* ptr, __atomic_raw_value<T> value, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_and_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _INNER fetch_and(ptr, value, mo) & value;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
or_fetch(volatile T* ptr, __atomic_raw_value<T> value, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_or_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _INNER fetch_or(ptr, value, mo) | value;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
xor_fetch(volatile T* ptr, __atomic_raw_value<T> value, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	return __atomic_xor_fetch(ptr, value, static_cast<int32_t>(mo));
#else
	return _INNER fetch_xor(ptr, value, mo) ^ value;
#endif
}

MSTL_END_INNER__


struct atomic_flag {
	using value_type = conditional_t<ATOMIC_TS_IS_BOOL, bool, long>;

	value_type flag_;

	atomic_flag() noexcept = default;
	atomic_flag(const atomic_flag&) = delete;
	atomic_flag& operator =(const atomic_flag&) = delete;
	atomic_flag& operator =(const atomic_flag&) volatile = delete;
	~atomic_flag() noexcept = default;

	constexpr atomic_flag(const value_type flag) noexcept
	: flag_{static_cast<value_type>(flag ? 1 : 0)}  {}

	MSTL_ALWAYS_INLINE bool
	test_and_set(const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_flag*>(this)->test_and_set(mo);
	}

	ATOMIC_ALWAYS_INLINE bool
	test_and_set(const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_test_and_set(&flag_, static_cast<int32_t>(mo));
#else
		const long old_val = ::_InterlockedExchange(
			reinterpret_cast<volatile value_type*>(&flag_),
			static_cast<long>(1));
		if (mo == memory_order_seq_cst) ::_ReadWriteBarrier();
		return old_val != 0;
#endif
	}

	MSTL_ALWAYS_INLINE bool
	test(const memory_order mo = memory_order_seq_cst) const noexcept {
		return const_cast<const volatile atomic_flag*>(this)->test(mo);
	}

	ATOMIC_ALWAYS_INLINE bool
	test(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		value_type value;
		__atomic_load(&flag_, &value, static_cast<int32_t>(mo));
		return value == ATOMIC_TS_TRUEVAL;
#else
		const long as_bytes = flag_;
		if (mo != memory_order_relaxed) ::_ReadWriteBarrier();
		return as_bytes != 0;
#endif
	}

	MSTL_ALWAYS_INLINE void
	wait(const bool old, const memory_order mo = memory_order_seq_cst) const noexcept {
		const_cast<const volatile atomic_flag*>(this)->wait(old, mo);
	}

	ATOMIC_ALWAYS_INLINE void
	wait(const bool old, const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		const value_type value = old ? 1 : 0;
		_MSTL atomic_wait_address_v(
			const_cast<const value_type*>(&flag_), value,
			[this, mo] { return this->test(mo); }
			);
	}


	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&flag_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&flag_, true);
	}


	MSTL_ALWAYS_INLINE void
	clear(const memory_order mo = memory_order_seq_cst) noexcept {
		const_cast<volatile atomic_flag*>(this)->clear(mo);
	}

	ATOMIC_ALWAYS_INLINE void
	clear(const memory_order mo = memory_order_seq_cst) volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);

#ifdef MSTL_COMPILER_GNUC__
		__atomic_clear(&flag_, static_cast<int32_t>(mo));
#else
		_INNER store(&flag_, static_cast<value_type>(0), mo);
#endif
	}
};

template <typename T>
struct atomic_base {
	using value_type = T;
	using difference_type = value_type;

private:
	static constexpr int align_inner = sizeof(T) > alignof(T) ? sizeof(T) : alignof(T);
	alignas(align_inner) value_type value_;

public:
	atomic_base() noexcept = default;
	~atomic_base() noexcept = default;
	atomic_base(const atomic_base&) = delete;
	atomic_base& operator=(const atomic_base&) = delete;
	atomic_base& operator=(const atomic_base&) volatile = delete;

	constexpr atomic_base(value_type value) noexcept : value_ (value) { }

	operator value_type() const noexcept { return load(); }
	operator value_type() const volatile noexcept { return load(); }

	value_type operator =(value_type value) noexcept {
		atomic_base::store(value);
		return value;
	}
	value_type operator =(value_type value) volatile noexcept {
		atomic_base::store(value);
		return value;
	}

	value_type operator ++(int) noexcept { return fetch_add(1); }
	value_type operator ++(int) volatile noexcept { return fetch_add(1); }

	value_type operator --(int) noexcept { return fetch_sub(1); }
	value_type operator --(int) volatile noexcept { return fetch_sub(1); }

	value_type operator ++() noexcept {
		return const_cast<volatile atomic_base*>(this)->operator ++();
	}
	value_type operator ++() volatile noexcept {
		return _INNER add_fetch(&value_, 1, memory_order_seq_cst);
	}

	value_type operator --() noexcept {
		return const_cast<volatile atomic_base*>(this)->operator --();
	}
	value_type operator --() volatile noexcept {
		return _INNER sub_fetch(&value_, 1, memory_order_seq_cst);
	}

	value_type operator +=(value_type value) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator +=(value);
	}
	value_type operator +=(value_type value) volatile noexcept {
		return _INNER add_fetch(&value_, value, memory_order_seq_cst);
	}

	value_type operator -=(value_type value) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator -=(value);
	}
	value_type operator -=(value_type value) volatile noexcept {
		return _INNER sub_fetch(&value_, value, memory_order_seq_cst);
	}

	value_type operator &=(value_type value) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator &=(value);
	}
	value_type operator &=(value_type value) volatile noexcept {
		return _INNER and_fetch(&value_, value, memory_order_seq_cst);
	}

	value_type operator |=(value_type value) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator |=(value);
	}
	value_type operator |=(value_type value) volatile noexcept {
		return _INNER or_fetch(&value_, value, memory_order_seq_cst);
	}

	value_type operator ^=(value_type value) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator ^=(value);
	}
	value_type operator ^=(value_type value) volatile noexcept {
		return _INNER xor_fetch(&value_, value, memory_order_seq_cst);
	}

	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(T), align_inner>();
	}
	bool is_lock_free() const volatile noexcept {
		return _INNER is_lock_free_impl<sizeof(T), align_inner>();
	}

	MSTL_ALWAYS_INLINE void
	store(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		const_cast<volatile atomic_base*>(this)->store(value, mo);
	}

	MSTL_ALWAYS_INLINE void
	store(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acquire);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_consume);
		_INNER store(&value_, value, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return const_cast<const volatile atomic_base*>(this)->load(mo);
	}

	MSTL_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_release);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);
		return _INNER load(&value_, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	exchange(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->exchange(value, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	exchange(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER exchange(&value_, value, mo);
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		return const_cast<volatile atomic_base*>(this)->compare_exchange_weak(expected, desired, success, failure);
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _INNER compare_exchange_weak(&value_, &expected, desired, success, failure);
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return this->compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return this->compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		return const_cast<volatile atomic_base*>(this)->compare_exchange_strong(expected, desired, success, failure);
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _INNER compare_exchange_strong(&value_, &expected, desired, success, failure);
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return this->compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return this->compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(value_type old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(&value_, old, [mo, this] {
			return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&value_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&value_, true);
	}


	MSTL_ALWAYS_INLINE value_type
	fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_add(value, mo);
	}
	MSTL_ALWAYS_INLINE value_type
	fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_add(&value_, value, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_sub(value, mo);
	}
	MSTL_ALWAYS_INLINE value_type
	fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_sub(&value_, value, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	fetch_and(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_and(value, mo);
	}
	MSTL_ALWAYS_INLINE value_type
	fetch_and(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_and(&value_, value, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	fetch_or(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_or(value, mo);
	}
	MSTL_ALWAYS_INLINE value_type
	fetch_or(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_or(&value_, value, mo);
	}

	MSTL_ALWAYS_INLINE value_type
	fetch_xor(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_xor(value, mo);
	}
	MSTL_ALWAYS_INLINE value_type
	fetch_xor(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_xor(&value_, value, mo);
	}
};


template <typename T>
struct atomic_base<T*> {
	using value_type = T*;
	using difference_type = ptrdiff_t;

private:
	value_type ptr_ = nullptr;

	constexpr ptrdiff_t real_type_sizes(const ptrdiff_t dest) const noexcept {
		return dest * sizeof(T);
	}

public:
	atomic_base() noexcept = default;
	atomic_base(const atomic_base&) = delete;
	atomic_base& operator=(const atomic_base&) = delete;
	atomic_base& operator=(const atomic_base&) volatile = delete;
	~atomic_base() noexcept = default;

	constexpr atomic_base(const value_type ptr) noexcept : ptr_(ptr) {}

	operator value_type() const noexcept { return load(); }
	operator value_type() const volatile noexcept { return load(); }

	value_type operator =(const value_type ptr) noexcept {
		atomic_base::store(ptr);
		return ptr;
	}
	value_type operator =(const value_type ptr) volatile noexcept {
		atomic_base::store(ptr);
		return ptr;
	}

	value_type operator ++(int) noexcept { return fetch_add(1); }
	value_type operator ++(int) volatile noexcept { return fetch_add(1); }

	value_type operator --(int) noexcept { return fetch_sub(1); }
	value_type operator --(int) volatile noexcept { return fetch_sub(1); }

	value_type operator ++() noexcept {
		return const_cast<volatile atomic_base*>(this)->operator ++();
	}
	value_type operator ++() volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_add_fetch(&ptr_, real_type_sizes(1), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_InterlockedExchangeAdd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr + sizeof(T));
#endif
	}

	value_type operator --() noexcept {
		return const_cast<volatile atomic_base*>(this)->operator --();
	}
	value_type operator --() volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_sub_fetch(&ptr_, real_type_sizes(1), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_InterlockedExchangeAdd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(-static_cast<ptrdiff_t>(sizeof(T)))));
		return reinterpret_cast<value_type>(old_ptr - sizeof(T));
#endif
	}

	value_type operator +=(const ptrdiff_t dest) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator +=(dest);
	}
	value_type operator +=(const ptrdiff_t dest) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_add_fetch(&ptr_, real_type_sizes(dest), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_InterlockedExchangeAdd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(dest * sizeof(T))));
		return reinterpret_cast<value_type>(old_ptr + dest * sizeof(T));
#endif
	}

	value_type operator -=(const ptrdiff_t dest) noexcept {
		return const_cast<volatile atomic_base*>(this)->operator -=(dest);
	}
	value_type operator -=(const ptrdiff_t dest) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_sub_fetch(&ptr_, real_type_sizes(dest), static_cast<int32_t>(memory_order_seq_cst));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_InterlockedExchangeAdd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(-dest * static_cast<ptrdiff_t>(sizeof(T)))));
		return reinterpret_cast<value_type>(old_ptr - dest * sizeof(T));
#endif
	}

	bool is_lock_free() const noexcept {
		return _INNER atomic_is_always_lock_free<sizeof(ptr_)>;
	}
	bool is_lock_free() const volatile noexcept {
		return _INNER atomic_is_always_lock_free<sizeof(ptr_)>;
	}

	MSTL_ALWAYS_INLINE void
	store(const value_type ptr, const memory_order mo = memory_order_seq_cst) noexcept {
		const_cast<volatile atomic_base*>(this)->store(ptr, mo);
	}

	ATOMIC_ALWAYS_INLINE void
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
		_INNER apply_memory_order_store(mo);
#endif
	}

	MSTL_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return const_cast<const volatile atomic_base*>(this)->load(mo);
	}

	ATOMIC_ALWAYS_INLINE value_type
	load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		memory_order rmo MSTL_UNUSED = mo & memory_order_modifier::memory_order_mask;
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_release);
		MSTL_CONSTEXPR_ASSERT(rmo != memory_order_acq_rel);

#ifdef MSTL_COMPILER_GNUC__
		return __atomic_load_n(&ptr_, static_cast<int32_t>(mo));
#else
		const value_type result = *reinterpret_cast<value_type const volatile*>(&ptr_);
		_INNER apply_memory_order_load(mo);
		return result;
#endif
	}

	MSTL_ALWAYS_INLINE value_type
	exchange(const value_type ptr, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->exchange(ptr, mo);
	}

	ATOMIC_ALWAYS_INLINE value_type
	exchange(const value_type ptr, const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_exchange_n(&ptr_, ptr, static_cast<int32_t>(mo));
#else
		const value_type old = static_cast<value_type>(
		    ::_InterlockedExchangePointer(
			reinterpret_cast<void* volatile*>(&ptr_), ptr));
		_INNER apply_memory_order_seq_cst(mo);
		return old;
#endif
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		return const_cast<volatile atomic_base*>(this)->compare_exchange_weak(expected, desired, success, failure);
	}

	ATOMIC_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_compare_exchange_n(
		    &ptr_, &expected, desired, 1,
		    static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
		void* old = ::_InterlockedCompareExchangePointer(
		    reinterpret_cast<void* volatile*>(&ptr_),
		    desired,
		    expected);
		if (old == expected) {
			if (success == memory_order_seq_cst) {
				::_ReadWriteBarrier();
			}
			return true;
		}
		expected = static_cast<value_type>(old);
		if (failure == memory_order_seq_cst) {
			::_ReadWriteBarrier();
		}
		return false;
#endif
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_weak(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) noexcept {
		return const_cast<volatile atomic_base*>(this)->compare_exchange_strong(expected, desired, success, failure);
	}

	ATOMIC_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order success, const memory_order failure) volatile noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_compare_exchange_n(
		    &ptr_, &expected, desired, 0,
		    static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
		// On x86/x64, weak and strong are the same
		return _INNER compare_exchange_weak(expected, desired, success, failure);
#endif
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}

	MSTL_ALWAYS_INLINE bool
	compare_exchange_strong(value_type& expected, value_type desired,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER compare_exchange_strong(expected, desired, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(value_type old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(&ptr_, old, [mo, this] {
			return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&ptr_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&ptr_, true);
	}


	MSTL_ALWAYS_INLINE value_type
	fetch_add(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_add(dest, mo);
	}
	ATOMIC_ALWAYS_INLINE value_type
	fetch_add(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_fetch_add(&ptr_, real_type_sizes(dest), static_cast<int32_t>(mo));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_InterlockedExchangeAdd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(dest * sizeof(T))));
		_INNER apply_memory_order_seq_cst(mo);
		return reinterpret_cast<value_type>(old_ptr);
#endif
	}

	MSTL_ALWAYS_INLINE value_type
	fetch_sub(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic_base*>(this)->fetch_sub(dest, mo);
	}
	ATOMIC_ALWAYS_INLINE value_type
	fetch_sub(const ptrdiff_t dest, const memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		return __atomic_fetch_sub(&ptr_, real_type_sizes(dest), static_cast<int32_t>(mo));
#else
		const char* old_ptr = reinterpret_cast<char*>(
		    ::_InterlockedExchangeAdd64(
			reinterpret_cast<volatile long long*>(&ptr_),
			static_cast<long long>(-dest * static_cast<ptrdiff_t>(sizeof(T)))));
		_INNER apply_memory_order_seq_cst(mo);
		return reinterpret_cast<value_type>(old_ptr);
#endif
	}
};


MSTL_BEGIN_INNER__

template <typename T>
ATOMIC_ALWAYS_INLINE void
store(T* ptr, __atomic_raw_value<T> value, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	__atomic_store(ptr, _MSTL addressof(value), static_cast<int32_t>(mo));
#else
	__atomic_raw_value<T> expected = *ptr;
	while (!_INNER compare_exchange_weak(ptr, expected, value, mo, memory_order_relaxed)) {
		// Retry
	}
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
load(const T* ptr, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	alignas(T) unsigned char buffer[sizeof(T)];
	auto* dest = reinterpret_cast<__atomic_raw_value<T>*>(buffer);
	__atomic_load(ptr, dest, static_cast<int32_t>(mo));
	return *dest;
#else
	__atomic_raw_value<T> result;
	_MSTL memory_copy(&result, ptr, sizeof(T));
	_INNER apply_memory_order_load(mo);
	return result;
#endif
}

template <typename T>
ATOMIC_ALWAYS_INLINE __atomic_raw_value<T>
exchange(T* ptr, __atomic_raw_value<T> desired, memory_order mo) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	alignas(T) unsigned char buffer[sizeof(T)];
	auto* dest = reinterpret_cast<__atomic_raw_value<T>*>(buffer);
	__atomic_exchange(ptr, _MSTL addressof(desired), dest, static_cast<int32_t>(mo));
	return *dest;
#else
	__atomic_raw_value<T> old = _INNER load(ptr, memory_order_relaxed);
	while (!_INNER compare_exchange_weak(ptr, old, desired, mo, memory_order_relaxed)) {
		// Retry
	}
	return old;
#endif
}

template <typename T>
T fetch_add_float(T* ptr, __atomic_raw_value<T> value, memory_order mo) noexcept {
	__atomic_raw_value<T> old_value = _INNER load(ptr, memory_order_relaxed);
	__atomic_raw_value<T> new_value = old_value + value;
	while (!_INNER compare_exchange_weak(ptr, old_value, new_value, mo, memory_order_relaxed)) {
		new_value = old_value + value;
	}
	return old_value;
}

template <typename T>
T fetch_sub_float(T* ptr, __atomic_raw_value<T> value, memory_order mo) noexcept {
	__atomic_raw_value<T> old_value = _INNER load(ptr, memory_order_relaxed);
	__atomic_raw_value<T> new_value = old_value - value;
	while (!_INNER compare_exchange_weak(ptr, old_value, new_value, mo, memory_order_relaxed)) {
		new_value = old_value - value;
	}
	return old_value;
}

template <typename T>
T add_fetch_float(T* ptr, __atomic_raw_value<T> value) noexcept {
	__atomic_raw_value<T> old_value = _INNER load(ptr, memory_order_relaxed);
	__atomic_raw_value<T> new_value = old_value + value;
	while (!_INNER compare_exchange_weak(ptr, old_value, new_value, memory_order_seq_cst, memory_order_relaxed)) {
		new_value = old_value + value;
	}
	return new_value;
}

template <typename T>
T sub_fetch_float(T* ptr, __atomic_raw_value<T> value) noexcept {
	__atomic_raw_value<T> old_value = _INNER load(ptr, memory_order_relaxed);
	__atomic_raw_value<T> new_value = old_value - value;
	while (!_INNER compare_exchange_weak(ptr, old_value, new_value, memory_order_seq_cst, memory_order_relaxed)) {
		new_value = old_value - value;
	}
	return new_value;
}

MSTL_END_INNER__


template <typename Float>
struct atomic_float_base {
	static_assert(is_floating_point_v<Float>, "atomic_ref_base need floating point T");

	using value_type = Float;
	using difference_type = value_type;

	static constexpr bool is_always_lock_free =
#ifdef MSTL_COMPILER_GNUC__
		__atomic_always_lock_free(sizeof(Float), 0);
#else
		_INNER atomic_is_always_lock_free<sizeof(Float)>;
#endif
	static constexpr size_t inner_align = alignof(Float);

private:
	alignas(inner_align) Float float_ = _MSTL initialize<Float>();
	
public:
	atomic_float_base() = default;
	constexpr atomic_float_base(Float value) : float_(value) {}

	atomic_float_base(const atomic_float_base&) = delete;
	atomic_float_base& operator =(const atomic_float_base&) = delete;
	atomic_float_base& operator =(const atomic_float_base&) volatile = delete;

	Float operator =(Float value) volatile noexcept {
		this->store(value);
		return value;
	}
	Float operator =(Float value) noexcept {
		this->store(value);
		return value;
	}

	bool is_lock_free() const volatile noexcept {
		return _INNER is_lock_free_impl<sizeof(Float), inner_align>();
	}
	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(Float), inner_align>();
	}

	void store(Float value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		_INNER store(&float_, value, mo);
	}
	void store(Float value, const memory_order mo = memory_order_seq_cst) noexcept {
		_INNER store(&float_, value, mo);
	}

	Float load(const memory_order mo = memory_order_seq_cst) const volatile noexcept {
		return _INNER load(&float_, mo);
	}
	Float load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER load(&float_, mo);
	}

	operator Float() const volatile noexcept { return this->load(); }
	operator Float() const noexcept { return this->load(); }

	Float exchange(Float desire, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER exchange(&float_, desire, mo);
	}
	Float exchange(Float desire, const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER exchange(&float_, desire, mo);
	}

	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order success, const memory_order failure) noexcept {
		return _INNER compare_exchange_weak(&float_, expected, desire, success, failure);
	}
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _INNER compare_exchange_weak(&float_, expected, desire, success, failure);
	}

	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order success, const memory_order failure) noexcept {
		return _INNER compare_exchange_strong(&float_, expected, desire, success, failure);
	}
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order success, const memory_order failure) volatile noexcept {
		return _INNER compare_exchange_strong(&float_, expected, desire, success, failure);
	}

	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER compare_exchange_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}
	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER compare_exchange_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER compare_exchange_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}
	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER compare_exchange_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(Float old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(&float_, old, [mo, this] {
			return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() noexcept {
		_MSTL atomic_notify_address(&float_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() noexcept {
		_MSTL atomic_notify_address(&float_, true);
	}


	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER fetch_add_float(&float_, value, mo);
	}
	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_add_float(&float_, value, mo);
	}

	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) noexcept {
		return _INNER fetch_sub_float(&float_, value, mo);
	}
	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) volatile noexcept {
		return _INNER fetch_sub_float(&float_, value, mo);
	}

	value_type operator +=(value_type value) noexcept {
		return _INNER add_fetch_float(&float_, value);
	}
	value_type operator +=(value_type value) volatile noexcept {
		return _INNER add_fetch_float(&float_, value);
	}

	value_type operator -=(value_type value) noexcept {
		return _INNER sub_fetch_float(&float_, value);
	}
	value_type operator -=(value_type value) volatile noexcept {
		return _INNER sub_fetch_float(&float_, value);
	}
};


template <typename T, bool = is_integral_v<T>, bool = is_floating_point_v<T>>
struct atomic_ref_base;


template <typename T>
struct atomic_ref_base<T, false, false> {
	static_assert(is_trivially_copyable_v<T>, "atomic_ref_base need trivially copyable T");

private:
	T* ptr_;

public:
	using value_type = T;

	static constexpr bool is_always_lock_free =
#ifdef MSTL_COMPILER_GNUC__
		__atomic_always_lock_free(sizeof(T), 0);
#else
		_INNER atomic_is_always_lock_free<sizeof(T)>;
#endif
	static constexpr int align_inner = (sizeof(T) & (sizeof(T) - 1)) || sizeof(T) > 16 ? 0 : sizeof(T);
	static constexpr size_t required_alignment = align_inner > alignof(T) ? align_inner : alignof(T);

	explicit atomic_ref_base(T& value) : ptr_(_MSTL addressof(value)) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	atomic_ref_base(const atomic_ref_base&) noexcept = default;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	T operator =(T value) const noexcept {
		this->store(value);
		return value;
	}

	operator T() const noexcept { return this->load(); }

	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(T), required_alignment>();
	}

	void store(T value, const memory_order mo = memory_order_seq_cst) const noexcept {
		_INNER store(ptr_, value, mo);
	}

	T load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER load(ptr_, mo);
	}

	T exchange(T desire, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER exchange(ptr_, desire, mo);
	}

	bool compare_exchange_weak(T& expected, T desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_weak(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_strong(T& expected, T desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_strong(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_weak(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(T old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
			return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() const noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() const noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}
};

template <typename T>
struct atomic_ref_base<T, true, false> {
	static_assert(is_integral_v<T>, "atomic_ref need integral T");

private:
	T* ptr_;

public:
	using value_type = T;
	using difference_type = value_type;

	static constexpr bool is_always_lock_free =
#ifdef MSTL_COMPILER_GNUC__
		__atomic_always_lock_free(sizeof(T), 0);
#else
		_INNER atomic_is_always_lock_free<sizeof(T)>;
#endif
	static constexpr size_t required_alignment = sizeof(T) > alignof(T) ? sizeof(T) : alignof(T);

	atomic_ref_base() = delete;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	explicit atomic_ref_base(T& value) : ptr_(&value) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	atomic_ref_base(const atomic_ref_base&) noexcept = default;

	T operator=(T value) const noexcept {
		this->store(value);
		return value;
	}

	operator T() const noexcept { return this->load(); }

	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(T), required_alignment>();
	}

	void store(T value, const memory_order mo = memory_order_seq_cst) const noexcept {
		_INNER store(ptr_, value, mo);
	}

	T load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER load(ptr_, mo);
	}

	T exchange(T desire, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER exchange(ptr_, desire, mo);
	}

	bool compare_exchange_weak(T& expected, T desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_weak(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_strong(T& expected, T desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_strong(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_weak(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(T& expected, T desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(T old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
		    return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() const noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() const noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}


	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_add(ptr_, value, mo);
	}

	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_sub(ptr_, value, mo);
	}

	value_type fetch_and(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_and(ptr_, value, mo);
	}

	value_type fetch_or(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_or(ptr_, value, mo);
	}

	value_type fetch_xor(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_xor(ptr_, value, mo);
	}

	MSTL_ALWAYS_INLINE value_type operator ++(int) const noexcept { return fetch_add(1); }
	MSTL_ALWAYS_INLINE value_type operator --(int) const noexcept { return fetch_sub(1); }

	value_type operator ++() const noexcept {
		return _INNER add_fetch(ptr_, value_type(1));
	}
	value_type operator --() const noexcept {
		return _INNER sub_fetch(ptr_, value_type(1));
	}

	value_type operator +=(value_type value) const noexcept {
		return _INNER add_fetch(ptr_, value);
	}
	value_type operator -=(value_type value) const noexcept {
		return _INNER sub_fetch(ptr_, value);
	}
	value_type operator &=(value_type value) const noexcept {
		return _INNER and_fetch(ptr_, value);
	}
	value_type operator |=(value_type value) const noexcept {
		return _INNER or_fetch(ptr_, value);
	}
	value_type operator ^=(value_type value) const noexcept {
		return _INNER xor_fetch(ptr_, value);
	}
};

template <typename Float>
struct atomic_ref_base<Float, false, true> {
	static_assert(is_floating_point_v<Float>, "atomic_ref_base need floating point T");

private:
	Float* ptr_;

public:
	using value_type = Float;
	using difference_type = value_type;

	static constexpr bool is_always_lock_free =
#ifdef MSTL_COMPILER_GNUC__
		__atomic_always_lock_free(sizeof(Float), 0);
#else
		_INNER atomic_is_always_lock_free<sizeof(Float)>;
#endif
	static constexpr size_t required_alignment = alignof(Float);

	atomic_ref_base() = delete;
	atomic_ref_base& operator =(const atomic_ref_base&) = delete;

	explicit atomic_ref_base(Float& value) : ptr_(&value) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	atomic_ref_base(const atomic_ref_base&) noexcept = default;

	Float operator=(Float value) const noexcept {
		this->store(value);
		return value;
	}

	operator Float() const noexcept { return this->load(); }

	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(Float), required_alignment>();
	}

	void store(Float value, const memory_order mo = memory_order_seq_cst) const noexcept {
		_INNER store(ptr_, value, mo);
	}

	Float load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER load(ptr_, mo);
	}

	Float exchange(Float desire, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER exchange(ptr_, desire, mo);
	}

	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_weak(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_strong(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_weak(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(Float& expected, Float desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(Float old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
			return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() const noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() const noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}


	value_type fetch_add(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_add_float(ptr_, value, mo);
	}

	value_type fetch_sub(value_type value, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_sub_float(ptr_, value, mo);
	}

	value_type operator+=(value_type value) const noexcept {
		return _INNER add_fetch_float(ptr_, value);
	}

	value_type operator-=(value_type value) const noexcept {
		return _INNER sub_fetch_float(ptr_, value);
	}
};

template <typename T>
struct atomic_ref_base<T*, false, false> {
private:
	static constexpr ptrdiff_t real_type_sizes(ptrdiff_t dest) noexcept {
		static_assert(is_object_v<T>, "atomic_ref_base need object T");
		return dest * sizeof(T);
	}

	T** ptr_;

public:
	using value_type = T*;
	using difference_type = ptrdiff_t;

	static constexpr bool is_always_lock_free =
#ifdef MSTL_COMPILER_GNUC__
	    __atomic_always_lock_free(sizeof(T*), 0);
#else
	    _INNER atomic_is_always_lock_free<sizeof(T*)>;
#endif
	static constexpr size_t required_alignment = alignof(T*);

	atomic_ref_base() = delete;
	atomic_ref_base& operator=(const atomic_ref_base&) = delete;

	explicit atomic_ref_base(T*& value) : ptr_(_MSTL addressof(value)) {
		MSTL_CONSTEXPR_ASSERT((static_cast<uintptr_t>(ptr_) % required_alignment) == 0);
	}

	atomic_ref_base(const atomic_ref_base&) noexcept = default;

	T* operator =(T* value) const noexcept {
		this->store(value);
		return value;
	}

	operator T*() const noexcept { return this->load(); }

	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(T*), required_alignment>();
	}

	void store(T* value, const memory_order mo = memory_order_seq_cst) const noexcept {
		_INNER store(ptr_, value, mo);
	}

	T* load(const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER load(ptr_, mo);
	}

	T* exchange(T* desire, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER exchange(ptr_, desire, mo);
	}

	bool compare_exchange_weak(T*& expected, T* desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_weak(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_strong(T*& expected, T* desire,
		const memory_order success, const memory_order failure) const noexcept {
		return _INNER compare_exchange_strong(ptr_, expected, desire, success, failure);
	}

	bool compare_exchange_weak(T*& expected, T* desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_weak(expected, desire, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(T*& expected, T* desire,
		const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER compare_exchange_strong(expected, desire, mo, cmpexch_failure_order(mo));
	}


	MSTL_ALWAYS_INLINE void
	wait(T* old, const memory_order mo = memory_order_seq_cst) const noexcept {
		_MSTL atomic_wait_address_v(ptr_, old, [this, mo] {
			return this->load(mo);
		});
	}

	MSTL_ALWAYS_INLINE void notify_one() const noexcept {
		_MSTL atomic_notify_address(ptr_, false);
	}

	MSTL_ALWAYS_INLINE void notify_all() const noexcept {
		_MSTL atomic_notify_address(ptr_, true);
	}


	MSTL_ALWAYS_INLINE value_type
	fetch_add(const difference_type dest, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_add(ptr_, real_type_sizes(dest), mo);
	}

	MSTL_ALWAYS_INLINE value_type
	fetch_sub(const difference_type dest, const memory_order mo = memory_order_seq_cst) const noexcept {
		return _INNER fetch_sub(ptr_, real_type_sizes(dest), mo);
	}

	value_type operator ++(int) const noexcept { return fetch_add(1); }
	value_type operator --(int) const noexcept { return fetch_sub(1); }

	value_type operator ++() const noexcept {
		return _INNER add_fetch(ptr_, real_type_sizes(1));
	}

	value_type operator --() const noexcept {
		return _INNER sub_fetch(ptr_, real_type_sizes(1));
	}

	value_type operator +=(const difference_type dest) const noexcept {
		return _INNER add_fetch(ptr_, real_type_sizes(dest));
	}

	value_type operator -=(const difference_type dest) const noexcept {
		return _INNER sub_fetch(ptr_, real_type_sizes(dest));
	}
};

#undef ATOMIC_ALWAYS_INLINE

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_BASE_HPP__
