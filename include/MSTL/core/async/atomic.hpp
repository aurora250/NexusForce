#ifndef MSTL_CORE_ASYNC_ATOMIC_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_HPP__
#include "atomic_base.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

#ifdef MSTL_COMPILER_MSVC__

template <size_t Size>
struct store_generic_impl {
	template <typename T>
	static void call(volatile T* ptr, T value, memory_order mo) noexcept {
		T expected;
		memory_copy(&expected, ptr, sizeof(T));
		while (!compare_exchange_weak_generic(ptr, &expected, value, mo, memory_order_relaxed)) {
			// Retry
		}
	}
};

template <>
struct store_generic_impl<1> {
	template <typename T>
	static void call(volatile T* ptr, T value, memory_order mo) noexcept {
		_InterlockedExchange8(
		    reinterpret_cast<volatile char*>(ptr),
		    __atomic_reinterpret_as<char>(value));
		apply_memory_order_store(mo);
	}
};

template <>
struct store_generic_impl<2> {
	template <typename T>
	static void call(volatile T* ptr, T value, memory_order mo) noexcept {
		_InterlockedExchange16(
		    reinterpret_cast<volatile short*>(ptr),
		    __atomic_reinterpret_as<short>(value));
		apply_memory_order_store(mo);
	}
};

template <>
struct store_generic_impl<4> {
	template <typename T>
	static void call(volatile T* ptr, T value, memory_order mo) noexcept {
		_InterlockedExchange(
		    reinterpret_cast<volatile long*>(ptr),
		    __atomic_reinterpret_as<long>(value));
		apply_memory_order_store(mo);
	}
};

template <>
struct store_generic_impl<8> {
	template <typename T>
	static void call(volatile T* ptr, T value, memory_order mo) noexcept {
		_InterlockedExchange64(
		    reinterpret_cast<volatile long long*>(ptr),
		    __atomic_reinterpret_as<long long>(value));
		apply_memory_order_store(mo);
	}
};

template <typename T>
MSTL_ALWAYS_INLINE void
store_generic(volatile T* ptr, T value, memory_order mo) noexcept {
	store_generic_impl<sizeof(T)>::call(ptr, value, mo);
}

template <size_t Size>
struct load_generic_impl {
	template <typename T>
	static T call(const volatile T* ptr, memory_order mo) noexcept {
		T result;
		memory_copy(&result, const_cast<const T*>(ptr), sizeof(T));
		apply_memory_order_load(mo);
		return result;
	}
};

template <>
struct load_generic_impl<1> {
	template <typename T>
	static T call(const volatile T* ptr, memory_order mo) noexcept {
		char val = __iso_volatile_load8(reinterpret_cast<const volatile char*>(ptr));
		apply_memory_order_load(mo);
		T result;
		memory_copy(&result, &val, sizeof(T));
		return result;
	}
};

template <>
struct load_generic_impl<2> {
	template <typename T>
	static T call(const volatile T* ptr, memory_order mo) noexcept {
		short val = __iso_volatile_load16(reinterpret_cast<const volatile short*>(ptr));
		apply_memory_order_load(mo);
		T result;
		memory_copy(&result, &val, sizeof(T));
		return result;
	}
};

template <>
struct load_generic_impl<4> {
	template <typename T>
	static T call(const volatile T* ptr, memory_order mo) noexcept {
		int val = __iso_volatile_load32(reinterpret_cast<const volatile int*>(ptr));
		apply_memory_order_load(mo);
		T result;
		memory_copy(&result, &val, sizeof(T));
		return result;
	}
};

template <>
struct load_generic_impl<8> {
	template <typename T>
	static T call(const volatile T* ptr, memory_order mo) noexcept {
		long long val = __iso_volatile_load64(reinterpret_cast<const volatile long long*>(ptr));
		apply_memory_order_load(mo);
		T result;
		memory_copy(&result, &val, sizeof(T));
		return result;
	}
};

template <typename T>
MSTL_ALWAYS_INLINE T
load_generic(const volatile T* ptr, memory_order mo) noexcept {
	return load_generic_impl<sizeof(T)>::call(ptr, mo);
}

template <size_t Size>
struct exchange_generic_impl {
    template <typename T>
    static T call(volatile T* ptr, T desired, memory_order mo) noexcept {
        T old = load_generic(ptr, memory_order_relaxed);
        while (!compare_exchange_weak_generic(ptr, &old, desired, mo, memory_order_relaxed)) {
            // Retry
        }
        return old;
    }
};

template <>
struct exchange_generic_impl<1> {
    template <typename T>
    static T call(volatile T* ptr, T desired, memory_order mo) noexcept {
        char old_val = _InterlockedExchange8(
            reinterpret_cast<volatile char*>(ptr),
            __atomic_reinterpret_as<char>(desired));
        apply_memory_order_seq_cst(mo);
        T result;
        memory_copy(&result, &old_val, sizeof(T));
        return result;
    }
};

template <>
struct exchange_generic_impl<2> {
    template <typename T>
    static T call(volatile T* ptr, T desired, memory_order mo) noexcept {
        short old_val = _InterlockedExchange16(
            reinterpret_cast<volatile short*>(ptr),
            __atomic_reinterpret_as<short>(desired));
        apply_memory_order_seq_cst(mo);
        T result;
        memory_copy(&result, &old_val, sizeof(T));
        return result;
    }
};

template <>
struct exchange_generic_impl<4> {
    template <typename T>
    static T call(volatile T* ptr, T desired, memory_order mo) noexcept {
        long old_val = _InterlockedExchange(
            reinterpret_cast<volatile long*>(ptr),
            __atomic_reinterpret_as<long>(desired));
        apply_memory_order_seq_cst(mo);
        T result;
        memory_copy(&result, &old_val, sizeof(T));
        return result;
    }
};

template <>
struct exchange_generic_impl<8> {
    template <typename T>
    static T call(volatile T* ptr, T desired, memory_order mo) noexcept {
        long long old_val = _InterlockedExchange64(
            reinterpret_cast<volatile long long*>(ptr),
            __atomic_reinterpret_as<long long>(desired));
        apply_memory_order_seq_cst(mo);
        T result;
        memory_copy(&result, &old_val, sizeof(T));
        return result;
    }
};

template <typename T>
MSTL_ALWAYS_INLINE T
exchange_generic(volatile T* ptr, T desired, memory_order mo) noexcept {
	return exchange_generic_impl<sizeof(T)>::call(ptr, desired, mo);
}

template <size_t Size>
struct compare_exchange_weak_generic_impl {
    template <typename T>
    static bool call(
    	volatile T* ptr, T* expected, T desired,
    	memory_order success, memory_order failure) noexcept {
        T current;
        memory_copy(&current, const_cast<const T*>(ptr), sizeof(T));

        if (memory_compare(&current, expected, sizeof(T)) == 0) {
            memory_copy(const_cast<T*>(ptr), &desired, sizeof(T));
            if (success == memory_order_seq_cst) {
                ::_ReadWriteBarrier();
            }
            return true;
        } else {
            memory_copy(expected, &current, sizeof(T));
            if (failure == memory_order_seq_cst) {
                ::_ReadWriteBarrier();
            }
            return false;
        }
    }
};

template <>
struct compare_exchange_weak_generic_impl<1> {
    template <typename T>
    static bool call(
    	volatile T* ptr, T* expected, T desired,
    	memory_order success, memory_order failure) noexcept {
        char exp_val = __atomic_reinterpret_as<char>(*expected);
        char old_val = _InterlockedCompareExchange8(
            reinterpret_cast<volatile char*>(ptr),
            __atomic_reinterpret_as<char>(desired),
            exp_val);

        bool result = (old_val == exp_val);
        if (!result) {
            memory_copy(expected, &old_val, sizeof(T));
        }

        if ((result && success == memory_order_seq_cst) ||
            (!result && failure == memory_order_seq_cst)) {
            ::_ReadWriteBarrier();
        }
        return result;
    }
};

template <>
struct compare_exchange_weak_generic_impl<2> {
    template <typename T>
    static bool call(
    	volatile T* ptr, T* expected, T desired,
    	memory_order success, memory_order failure) noexcept {
        short exp_val = __atomic_reinterpret_as<short>(*expected);
        short old_val = _InterlockedCompareExchange16(
            reinterpret_cast<volatile short*>(ptr),
            __atomic_reinterpret_as<short>(desired),
            exp_val);

        bool result = (old_val == exp_val);
        if (!result) {
            memory_copy(expected, &old_val, sizeof(T));
        }

        if ((result && success == memory_order_seq_cst) ||
            (!result && failure == memory_order_seq_cst)) {
            ::_ReadWriteBarrier();
        }
        return result;
    }
};

template <>
struct compare_exchange_weak_generic_impl<4> {
    template <typename T>
    static bool call(
    	volatile T* ptr, T* expected, T desired,
    	memory_order success, memory_order failure) noexcept {
        long exp_val = __atomic_reinterpret_as<long>(*expected);
        long old_val = _InterlockedCompareExchange(
            reinterpret_cast<volatile long*>(ptr),
            __atomic_reinterpret_as<long>(desired),
            exp_val);

        bool result = (old_val == exp_val);
        if (!result) {
            memory_copy(expected, &old_val, sizeof(T));
        }

        if ((result && success == memory_order_seq_cst) ||
            (!result && failure == memory_order_seq_cst)) {
            ::_ReadWriteBarrier();
        }
        return result;
    }
};

template <>
struct compare_exchange_weak_generic_impl<8> {
    template <typename T>
    static bool call(
    	volatile T* ptr, T* expected, T desired,
    	memory_order success, memory_order failure) noexcept {
        long long exp_val = __atomic_reinterpret_as<long long>(*expected);
        long long old_val = _InterlockedCompareExchange64(
            reinterpret_cast<volatile long long*>(ptr),
            __atomic_reinterpret_as<long long>(desired),
            exp_val);

        bool result = (old_val == exp_val);
        if (!result) {
            memory_copy(expected, &old_val, sizeof(T));
        }

        if ((result && success == memory_order_seq_cst) ||
            (!result && failure == memory_order_seq_cst)) {
            ::_ReadWriteBarrier();
        }
        return result;
    }
};

#ifdef MSTL_DATA_BUS_WIDTH_64__
template <>
struct compare_exchange_weak_generic_impl<16> {
    template <typename T>
    static bool call(
    	volatile T* ptr, T* expected, T desired,
    	memory_order success, memory_order failure) noexcept {
        alignas(16) long long exp_arr[2];
        alignas(16) long long des_arr[2];
        memory_copy(exp_arr, expected, 16);
        memory_copy(des_arr, &desired, 16);

        bool result = _InterlockedCompareExchange128(
            reinterpret_cast<volatile long long*>(ptr),
            des_arr[1], des_arr[0],
            exp_arr) != 0;

        if (!result) {
            memory_copy(expected, exp_arr, 16);
        }

        if ((result && success == memory_order_seq_cst) ||
            (!result && failure == memory_order_seq_cst)) {
            ::_ReadWriteBarrier();
        }
        return result;
    }
};
#endif

template <typename T>
MSTL_ALWAYS_INLINE bool
compare_exchange_weak_generic(
	volatile T* ptr, T* expected, T desired,
	memory_order success, memory_order failure) noexcept {
	return compare_exchange_weak_generic_impl<sizeof(T)>::call(
	    ptr, expected, desired, success, failure);
}

template <typename T>
MSTL_ALWAYS_INLINE bool
compare_exchange_strong_generic(
	volatile T* ptr, T* expected, T desired,
	memory_order success, memory_order failure) noexcept {
	return compare_exchange_weak_generic(ptr, expected, desired, success, failure);
}

#endif

MSTL_END_INNER__


template <typename T>
struct atomic;

template <typename T>
struct atomic {
	using value_type = T;

private:
	static constexpr int min_align = (sizeof(T) & (sizeof(T) - 1)) || sizeof(T) > 16 ? 0 : sizeof(T);
	static constexpr int align_inner = min_align > alignof(T) ? min_align : alignof(T);

	alignas(align_inner) T value_;

	static_assert(is_trivially_copyable_v<T>, "atomic requires a trivially copyable type");
	static_assert(sizeof(T) > 0, "Incomplete or zero-sized types are not supported");
	static_assert(is_copy_constructible_v<T>, "atomic need copy constructible T");
	static_assert(is_move_constructible_v<T>, "atomic need move constructible T");
	static_assert(is_copy_assignable_v<T>, "atomic copy move assignable T");
	static_assert(is_move_assignable_v<T>, "atomic need move assignable T");

	volatile T* get_volatile_ptr() noexcept {
		return reinterpret_cast<volatile T*>(&value_);
	}
	const volatile T* get_volatile_ptr() const noexcept {
		return reinterpret_cast<const volatile T*>(&value_);
	}
	volatile T* get_volatile_ptr() volatile noexcept {
		return &value_;
	}
	const volatile T* get_volatile_ptr() const volatile noexcept {
		return &value_;
	}

public:
	static constexpr bool is_always_lock_free =
#ifdef MSTL_COMPILER_GNUC__
		__atomic_always_lock_free(sizeof(value_), nullptr);
#else
		_INNER atomic_is_always_lock_free<sizeof(T)>;
#endif

	atomic() = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(T value) noexcept : value_(value) {}

	operator T() const noexcept { return load(); }
	operator T() const volatile noexcept { return load(); }

	T operator =(T value) noexcept { store(value); return value; }
	T operator =(T value) volatile noexcept{ store(value); return value; }

	bool is_lock_free() const noexcept {
		return _INNER is_lock_free_impl<sizeof(value_), align_inner>();
	}
	bool is_lock_free() const volatile noexcept {
		return _INNER is_lock_free_impl<sizeof(value_), align_inner>();
	}

	void store(T value, memory_order mo = memory_order_seq_cst) noexcept {
		const_cast<volatile atomic*>(this)->store(value, mo);
	}
	void store(T value, memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		__atomic_store(_MSTL addressof(value_), _MSTL addressof(value), static_cast<int32_t>(mo));
#else
		_INNER store_generic(get_volatile_ptr(), value, mo);
#endif
	}

	T load(memory_order mo = memory_order_seq_cst) const noexcept {
		return const_cast<const volatile atomic*>(this)->load(mo);
	}
	T load(memory_order mo = memory_order_seq_cst) const volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		alignas(T) unsigned char buffer[sizeof(T)];
		T* ptr = reinterpret_cast<T*>(buffer);
		__atomic_load(_MSTL addressof(value_), ptr, static_cast<int32_t>(mo));
		return *ptr;
#else
		return _INNER load_generic(get_volatile_ptr(), mo);
#endif
	}

	T exchange(T value, memory_order mo = memory_order_seq_cst) noexcept {
		return const_cast<volatile atomic*>(this)->exchange(value, mo);
    }
	T exchange(T value, memory_order mo = memory_order_seq_cst) volatile noexcept {
#ifdef MSTL_COMPILER_GNUC__
		alignas(T) unsigned char buffer[sizeof(T)];
		T* ptr = reinterpret_cast<T*>(buffer);
		__atomic_exchange(_MSTL addressof(value_), _MSTL addressof(value), ptr, static_cast<int32_t>(mo));
		return *ptr;
#else
		return _INNER exchange_generic(get_volatile_ptr(), value, mo);
#endif
	}

	bool compare_exchange_weak(T& expected, T desired,
		memory_order success, memory_order failure) noexcept {
		return const_cast<volatile atomic*>(this)->compare_exchange_weak(
			    expected, desired, success, failure);
    }
	bool compare_exchange_weak(T& expected, T desired,
		memory_order success, memory_order failure) volatile noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));

#ifdef MSTL_COMPILER_GNUC__
		return __atomic_compare_exchange(
			_MSTL addressof(value_), _MSTL addressof(expected),
			_MSTL addressof(desired), true,
			static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
		return _INNER compare_exchange_weak_generic(
			get_volatile_ptr(), &expected, desired, success, failure);
#endif
	}

	bool compare_exchange_weak(T& expected, T desired,
		memory_order mo = memory_order_seq_cst) noexcept {
		return atomic::compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}
	bool compare_exchange_weak(T& expected, T desired,
		memory_order mo = memory_order_seq_cst) volatile noexcept {
		return atomic::compare_exchange_weak(expected, desired, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(T& expected, T desired,
		memory_order success, memory_order failure) noexcept {
		return const_cast<volatile atomic*>(this)->compare_exchange_strong(
			    expected, desired, success, failure);
	}
	bool compare_exchange_strong(T& expected, T desired,
		memory_order success, memory_order failure) volatile noexcept {
		MSTL_CONSTEXPR_ASSERT(is_valid_cmpexch_failure_order(failure));

#ifdef MSTL_COMPILER_GNUC__
		return __atomic_compare_exchange(
			_MSTL addressof(value_), _MSTL addressof(expected),
			_MSTL addressof(desired), false,
			static_cast<int32_t>(success), static_cast<int32_t>(failure));
#else
		return _INNER compare_exchange_strong_generic(
			get_volatile_ptr(), &expected, desired, success, failure);
#endif
	}
	
	bool compare_exchange_strong(T& expected, T value,
		memory_order mo = memory_order_seq_cst) noexcept {
		return atomic::compare_exchange_strong(expected, value, mo, cmpexch_failure_order(mo));
	}
	bool compare_exchange_strong(T& expected, T value,
		memory_order mo = memory_order_seq_cst) volatile noexcept {
		return atomic::compare_exchange_strong(expected, value, mo, cmpexch_failure_order(mo));
	}
};


template <typename T>
struct atomic<T*> {
	using value_type = T*;
	using difference_type = ptrdiff_t;

private:
	using base_type = atomic_base<T*>;
	base_type base_;

public:
	static constexpr bool is_always_lock_free = true;
	
	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(value_type ptr) noexcept : base_(ptr) { }

	operator value_type() const noexcept { return value_type(base_); }
	operator value_type() const volatile noexcept { return value_type(base_); }

	value_type operator =(value_type ptr) noexcept { return base_.operator=(ptr); }
	value_type operator =(value_type ptr) volatile noexcept { return base_.operator=(ptr); }

	value_type operator ++(int) noexcept { return base_++; }
	value_type operator ++(int) volatile noexcept { return base_++; }

	value_type operator --(int) noexcept { return base_--; }
	value_type operator --(int) volatile noexcept { return base_--; }

	value_type operator ++() noexcept { return ++base_; }
	value_type operator ++() volatile noexcept { return ++base_; }

	value_type operator --() noexcept { return --base_; }
	value_type operator --() volatile noexcept { return --base_; }

	value_type operator +=(ptrdiff_t dest) noexcept { return base_.operator+=(dest); }
	value_type operator +=(ptrdiff_t dest) volatile noexcept { return base_.operator+=(dest); }

	value_type operator -=(ptrdiff_t dest) noexcept { return base_.operator-=(dest); }
	value_type operator -=(ptrdiff_t dest) volatile noexcept { return base_.operator-=(dest); }

	bool is_lock_free() const noexcept { return base_.is_lock_free(); }
	bool is_lock_free() const volatile noexcept { return base_.is_lock_free(); }

	void store(value_type ptr, memory_order mo = memory_order_seq_cst) noexcept {
		return base_.store(ptr, mo);
	}
	void store(value_type ptr, memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.store(ptr, mo);
	}

	value_type load(memory_order mo = memory_order_seq_cst) const noexcept {
		return base_.load(mo);
	}
	value_type load(memory_order mo = memory_order_seq_cst) const volatile noexcept {
		return base_.load(mo);
	}

	value_type exchange(value_type ptr, memory_order mo = memory_order_seq_cst) noexcept {
		return base_.exchange(ptr, mo);
	}
	value_type exchange(value_type ptr, memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.exchange(ptr, mo);
	}

	bool compare_exchange_weak(value_type& ptr1, value_type ptr2,
		memory_order mo1, memory_order mo2) noexcept {
		return base_.compare_exchange_weak(ptr1, ptr2, mo1, mo2);
	}
	bool compare_exchange_weak(value_type& ptr1, value_type ptr2,
		memory_order mo1, memory_order mo2) volatile noexcept {
		return base_.compare_exchange_weak(ptr1, ptr2, mo1, mo2);
	}

	bool compare_exchange_weak(value_type& ptr1, value_type ptr2,
		memory_order mo = memory_order_seq_cst) noexcept {
		return compare_exchange_weak(ptr1, ptr2, mo, cmpexch_failure_order(mo));
	}
	bool compare_exchange_weak(value_type& ptr1, value_type ptr2,
		memory_order mo = memory_order_seq_cst) volatile noexcept {
		return compare_exchange_weak(ptr1, ptr2, mo, cmpexch_failure_order(mo));
	}

	bool compare_exchange_strong(value_type& ptr1, value_type ptr2,
		memory_order mo1, memory_order mo2) noexcept {
		return base_.compare_exchange_strong(ptr1, ptr2, mo1, mo2);
	}
	bool compare_exchange_strong(value_type& ptr1, value_type ptr2,
		memory_order mo1, memory_order mo2) volatile noexcept {
		return base_.compare_exchange_strong(ptr1, ptr2, mo1, mo2);
	}

	bool compare_exchange_strong(value_type& ptr1, value_type ptr2,
		memory_order mo = memory_order_seq_cst) noexcept {
		return base_.compare_exchange_strong(ptr1, ptr2, mo, cmpexch_failure_order(mo));
	}
	bool compare_exchange_strong(value_type& ptr1, value_type ptr2,
		memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.compare_exchange_strong(ptr1, ptr2, mo, cmpexch_failure_order(mo));
	}

	
	void wait(value_type old, memory_order mo = memory_order_seq_cst) const noexcept {
		base_.wait(old, mo);
	}
	
	void notify_one() noexcept {
		base_.notify_one();
	}

	void notify_all() noexcept {
		base_.notify_all();
	}


	value_type fetch_add(ptrdiff_t dest, memory_order mo = memory_order_seq_cst) noexcept {
		return base_.fetch_add(dest, mo);
	}
	value_type fetch_add(ptrdiff_t dest, memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.fetch_add(dest, mo);
	}

	value_type fetch_sub(ptrdiff_t dest, memory_order mo = memory_order_seq_cst) noexcept {
		return base_.fetch_sub(dest, mo);
	}
	value_type fetch_sub(ptrdiff_t dest, memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.fetch_sub(dest, mo);
	}
};


template <>
struct atomic<bool> {
	using value_type = bool;

private:
	atomic_base<bool> base_;

public:
	static constexpr bool is_always_lock_free = true;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(bool value) noexcept : base_(value) { }

	bool operator=(bool value) noexcept { return base_.operator=(value); }
	bool operator=(bool value) volatile noexcept { return base_.operator=(value); }

	operator bool() const noexcept { return base_.load(); }
	operator bool() const volatile noexcept { return base_.load(); }

	bool is_lock_free() const noexcept { return base_.is_lock_free(); }
	bool is_lock_free() const volatile noexcept { return base_.is_lock_free(); }

	void store(bool value, memory_order mo = memory_order_seq_cst) noexcept {
		base_.store(value, mo);
	}
	void store(bool value, memory_order mo = memory_order_seq_cst) volatile noexcept {
		base_.store(value, mo);
	}

	bool load(memory_order mo = memory_order_seq_cst) const noexcept {
		return base_.load(mo);
	}
	bool load(memory_order mo = memory_order_seq_cst) const volatile noexcept {
		return base_.load(mo);
	}

	bool exchange(bool value, memory_order mo = memory_order_seq_cst) noexcept {
		return base_.exchange(value, mo);
	}
	bool exchange(bool value, memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.exchange(value, mo);
	}

	bool compare_exchange_weak(bool& value1, bool value2,
		memory_order mo1, memory_order mo2) noexcept {
		return base_.compare_exchange_weak(value1, value2, mo1, mo2);
	}
	bool compare_exchange_weak(bool& value1, bool value2,
		memory_order mo1, memory_order mo2) volatile noexcept {
		return base_.compare_exchange_weak(value1, value2, mo1, mo2);
	}

	bool compare_exchange_weak(bool& value1, bool value2,
		memory_order mo = memory_order_seq_cst) noexcept {
		return base_.compare_exchange_weak(value1, value2, mo);
	}
	bool compare_exchange_weak(bool& value1, bool value2,
		memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.compare_exchange_weak(value1, value2, mo);
	}

	bool compare_exchange_strong(bool& value1, bool value2,
		memory_order mo1, memory_order mo2) noexcept {
		return base_.compare_exchange_strong(value1, value2, mo1, mo2);
	}
	bool compare_exchange_strong(bool& value1, bool value2,
		memory_order mo1, memory_order mo2) volatile noexcept {
		return base_.compare_exchange_strong(value1, value2, mo1, mo2);
	}

	bool compare_exchange_strong(bool& value1, bool value2,
		memory_order mo = memory_order_seq_cst) noexcept {
		return base_.compare_exchange_strong(value1, value2, mo);
	}
	bool compare_exchange_strong(bool& value1, bool value2,
		memory_order mo = memory_order_seq_cst) volatile noexcept {
		return base_.compare_exchange_strong(value1, value2, mo);
	}

	
	void wait(bool old, memory_order mo = memory_order_seq_cst) const noexcept {
		base_.wait(old, mo);
	}
	
	void notify_one() noexcept {
		base_.notify_one();
	}

	void notify_all() noexcept {
		base_.notify_all();
	}
};

template<>
struct atomic<char> : atomic_base<char> {
	using integral_type = char;
	using base_type = atomic_base<char>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<signed char> : atomic_base<signed char> {
	using integral_type = signed char;
	using base_type = atomic_base<signed char>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<unsigned char> : atomic_base<unsigned char> {
	using integral_type = unsigned char;
	using base_type = atomic_base<unsigned char>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<short> : atomic_base<short> {
	using integral_type = short;
	using base_type = atomic_base<short>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<unsigned short> : atomic_base<unsigned short> {
	using integral_type = unsigned short;
	using base_type = atomic_base<unsigned short>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<int> : atomic_base<int> {
	using integral_type = int;
	using base_type = atomic_base<int>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<unsigned int> : atomic_base<unsigned int> {
	using integral_type = unsigned int;
	using base_type = atomic_base<unsigned int>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<long> : atomic_base<long> {
	using integral_type = long;
	using base_type = atomic_base<long>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<unsigned long> : atomic_base<unsigned long> {
	using integral_type = unsigned long;
	using base_type = atomic_base<unsigned long>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<long long> : atomic_base<long long> {
	using integral_type = long long;
	using base_type = atomic_base<long long>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<unsigned long long> : atomic_base<unsigned long long> {
	using integral_type = unsigned long long;
	using base_type = atomic_base<unsigned long long>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<wchar_t> : atomic_base<wchar_t> {
	using integral_type = wchar_t;
	using base_type = atomic_base<wchar_t>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

#ifdef MSTL_STANDARD_20__
template<>
struct atomic<char8_t> : atomic_base<char8_t> {
	using integral_type = char8_t;
	using base_type = atomic_base<char8_t>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};
#endif

template<>
struct atomic<char16_t> : atomic_base<char16_t> {
	using integral_type = char16_t;
	using base_type = atomic_base<char16_t>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};

template<>
struct atomic<char32_t> : atomic_base<char32_t> {
	using integral_type = char32_t;
	using base_type = atomic_base<char32_t>;

	atomic() noexcept = default;
	~atomic() noexcept = default;
	atomic(const atomic&) = delete;
	atomic& operator=(const atomic&) = delete;
	atomic& operator=(const atomic&) volatile = delete;

	constexpr atomic(integral_type value) noexcept : base_type(value) {}

	using base_type::operator integral_type;
	using base_type::operator=;

	static constexpr bool is_always_lock_free = true;
};


template<>
struct atomic<float> : atomic_float_base<float> {
	atomic() noexcept = default;

	constexpr atomic(float value) noexcept : atomic_float_base<float>(value) {}

	atomic& operator=(const atomic&) volatile = delete;
	atomic& operator=(const atomic&) = delete;

	using atomic_float_base<float>::operator=;
};

template<>
struct atomic<double> : atomic_float_base<double> {
	atomic() noexcept = default;

	constexpr atomic(double value) noexcept : atomic_float_base<double>(value) {}

	atomic& operator=(const atomic&) volatile = delete;
	atomic& operator=(const atomic&) = delete;

	using atomic_float_base<double>::operator=;
};

template<>
struct atomic<long double> : atomic_float_base<long double> {
	atomic() noexcept = default;

	constexpr atomic(long double value) noexcept : atomic_float_base<long double>(value) {}

	atomic& operator=(const atomic&) volatile = delete;
	atomic& operator=(const atomic&) = delete;

	using atomic_float_base<long double>::operator=;
};

template <typename T>
struct atomic_ref : atomic_ref_base<T> {
	explicit atomic_ref(T& value) noexcept : atomic_ref_base<T>(value) {}

	atomic_ref& operator=(const atomic_ref&) = delete;

	atomic_ref(const atomic_ref&) = default;

	using atomic_ref_base<T>::operator=;
};


using atomic_bool     = atomic<bool>;
using atomic_schar    = atomic<signed char>;
using atomic_uchar    = atomic<unsigned char>;
using atomic_short    = atomic<short>;
using atomic_ushort   = atomic<unsigned short>;
using atomic_int      = atomic<int>;
using atomic_uint     = atomic<unsigned int>;
using atomic_long     = atomic<long>;
using atomic_ulong    = atomic<unsigned long>;
using atomic_llong    = atomic<long long>;
using atomic_ullong	  = atomic<unsigned long long>;

using atomic_float    = atomic<float>;
using atomic_double   = atomic<double>;
using atomic_ldouble  = atomic<long double>;

using atomic_char     = atomic<char>;
using atomic_wchar_t  = atomic<wchar_t>;
#ifdef MSTL_STANDARD_20__
using atomic_char8_t  = atomic<char8_t>;
#endif
using atomic_char16_t = atomic<char16_t>;
using atomic_char32_t = atomic<char32_t>;

using atomic_int8_t   = atomic<int8_t>;
using atomic_uint8_t  = atomic<uint8_t>;
using atomic_int16_t  = atomic<int16_t>;
using atomic_uint16_t = atomic<uint16_t>;
using atomic_int32_t  = atomic<int32_t>;
using atomic_uint32_t = atomic<uint32_t>;
using atomic_int64_t  = atomic<int64_t>;
using atomic_uint64_t = atomic<uint64_t>;

using atomic_float32  = atomic<float32_t>;
using atomic_float64  = atomic<float64_t>;
using atomic_decimal  = atomic<decimal_t>;

using atomic_int_least8_t   = atomic<int_least8_t>;
using atomic_uint_least8_t  = atomic<uint_least8_t>;
using atomic_int_least16_t  = atomic<int_least16_t>;
using atomic_uint_least16_t = atomic<uint_least16_t>;
using atomic_int_least32_t  = atomic<int_least32_t>;
using atomic_uint_least32_t = atomic<uint_least32_t>;
using atomic_int_least64_t  = atomic<int_least64_t>;
using atomic_uint_least64_t = atomic<uint_least64_t>;

using atomic_int_fast8_t    = atomic<int_fast8_t>;
using atomic_uint_fast8_t   = atomic<uint_fast8_t>;
using atomic_int_fast16_t   = atomic<int_fast16_t>;
using atomic_uint_fast16_t  = atomic<uint_fast16_t>;
using atomic_int_fast32_t   = atomic<int_fast32_t>;
using atomic_uint_fast32_t  = atomic<uint_fast32_t>;
using atomic_int_fast64_t   = atomic<int_fast64_t>;
using atomic_uint_fast64_t  = atomic<uint_fast64_t>;

using atomic_intptr_t  = atomic<intptr_t>;
using atomic_uintptr_t = atomic<uintptr_t>;
using atomic_size_t    = atomic<size_t>;
using atomic_ptrdiff_t = atomic<ptrdiff_t>;

using atomic_intmax_t  = atomic<intmax_t>;
using atomic_uintmax_t = atomic<uintmax_t>;

inline bool
atomic_flag_test_and_set_explicit(atomic_flag* flag, memory_order mo) noexcept {
	return flag->test_and_set(mo);
}

inline bool
atomic_flag_test_and_set_explicit(volatile atomic_flag* flag, memory_order mo) noexcept {
	return flag->test_and_set(mo);
}


inline bool
atomic_flag_test(const atomic_flag* flag) noexcept {
	return flag->test();
}

inline bool
atomic_flag_test(const volatile atomic_flag* flag) noexcept {
	return flag->test();
}

inline bool
atomic_flag_test_explicit(const atomic_flag* flag, memory_order mo) noexcept {
	return flag->test(mo);
}

inline bool
atomic_flag_test_explicit(const volatile atomic_flag* flag, memory_order mo) noexcept {
	return flag->test(mo);
}


inline void
atomic_flag_clear_explicit(atomic_flag* flag, memory_order mo) noexcept {
	flag->clear(mo);
}

inline void
atomic_flag_clear_explicit(volatile atomic_flag* flag, memory_order mo) noexcept {
	flag->clear(mo);
}

inline bool
atomic_flag_test_and_set(atomic_flag* flag) noexcept {
	return atomic_flag_test_and_set_explicit(flag, memory_order_seq_cst);
}

inline bool
atomic_flag_test_and_set(volatile atomic_flag* flag) noexcept {
	return atomic_flag_test_and_set_explicit(flag, memory_order_seq_cst);
}

inline void
atomic_flag_clear(atomic_flag* flag) noexcept {
	atomic_flag_clear_explicit(flag, memory_order_seq_cst);
}

inline void
atomic_flag_clear(volatile atomic_flag* flag) noexcept {
	atomic_flag_clear_explicit(flag, memory_order_seq_cst);
}


inline void
atomic_flag_wait(atomic_flag* flag, bool old) noexcept {
	flag->wait(old);
}

inline void
atomic_flag_wait_explicit(atomic_flag* flag, bool old, memory_order mo) noexcept {
	flag->wait(old, mo);
}

inline void
atomic_flag_notify_one(atomic_flag* flag) noexcept {
	flag->notify_one();
}

inline void
atomic_flag_notify_all(atomic_flag* flag) noexcept {
	flag->notify_all();
}


template <typename T>
bool atomic_is_lock_free(const atomic<T>* flag) noexcept {
	return flag->is_lock_free();
}

template <typename T>
bool atomic_is_lock_free(const volatile atomic<T>* flag) noexcept {
	return flag->is_lock_free();
}

template <typename T>
void atomic_init(atomic<T>* flag, type_identity_t<T> value) noexcept {
	flag->store(value, memory_order_relaxed);
}

template <typename T>
void atomic_init(volatile atomic<T>* flag, type_identity_t<T> value) noexcept {
	flag->store(value, memory_order_relaxed);
}

template <typename T>
void atomic_store_explicit(atomic<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	flag->store(value, mo);
}

template <typename T>
void atomic_store_explicit(volatile atomic<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	flag->store(value, mo);
}

template <typename T>
T atomic_load_explicit(const atomic<T>* flag, memory_order mo) noexcept {
	return flag->load(mo);
}

template <typename T>
T atomic_load_explicit(const volatile atomic<T>* flag, memory_order mo) noexcept {
	return flag->load(mo);
}

template <typename T>
T atomic_exchange_explicit(atomic<T>* flag, type_identity_t<T> value, memory_order mo) noexcept {
	return flag->exchange(value, mo);
}

template <typename T>
T atomic_exchange_explicit(volatile atomic<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->exchange(value, mo);
}

template <typename T>
bool atomic_compare_exchange_weak_explicit(atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2, memory_order mo1, memory_order mo2) noexcept {
	return flag->compare_exchange_weak(*value1, value2, mo1, mo2);
}

template <typename T>
bool atomic_compare_exchange_weak_explicit(volatile atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2, memory_order mo1, memory_order mo2) noexcept {
	return flag->compare_exchange_weak(*value1, value2, mo1, mo2);
}

template <typename T>
bool atomic_compare_exchange_strong_explicit(atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2, memory_order mo1, memory_order mo2) noexcept {
	return flag->compare_exchange_strong(*value1, value2, mo1, mo2);
}

template <typename T>
bool atomic_compare_exchange_strong_explicit(volatile atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2, memory_order mo1, memory_order mo2) noexcept {
	return flag->compare_exchange_strong(*value1, value2, mo1, mo2);
}

template <typename T>
void atomic_store(atomic<T>* flag, type_identity_t<T> value) noexcept {
	_MSTL atomic_store_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
void atomic_store(volatile atomic<T>* flag, type_identity_t<T> value) noexcept {
	_MSTL atomic_store_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_load(const atomic<T>* flag) noexcept {
	return _MSTL atomic_load_explicit(flag, memory_order_seq_cst);
}

template <typename T>
T atomic_load(const volatile atomic<T>* flag) noexcept {
	return _MSTL atomic_load_explicit(flag, memory_order_seq_cst);
}

template <typename T>
T atomic_exchange(atomic<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_exchange_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_exchange(volatile atomic<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_exchange_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
bool atomic_compare_exchange_weak(atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2) noexcept {
	return _MSTL atomic_compare_exchange_weak_explicit(
		flag, value1, value2, memory_order_seq_cst, memory_order_seq_cst);
}

template <typename T>
bool atomic_compare_exchange_weak(volatile atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2) noexcept {
	return _MSTL atomic_compare_exchange_weak_explicit(
		flag, value1, value2, memory_order_seq_cst, memory_order_seq_cst);
}

template <typename T>
bool atomic_compare_exchange_strong(atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2) noexcept {
	return _MSTL atomic_compare_exchange_strong_explicit(
		flag, value1, value2, memory_order_seq_cst, memory_order_seq_cst);
}

template <typename T>
bool atomic_compare_exchange_strong(volatile atomic<T>* flag, type_identity_t<T>* value1,
	type_identity_t<T> value2) noexcept {
	return _MSTL atomic_compare_exchange_strong_explicit(
		flag, value1, value2, memory_order_seq_cst, memory_order_seq_cst);
}


template <typename T>
void atomic_wait(const atomic<T>* flag, typename atomic<T>::value_type old) noexcept {
	flag->wait(old);
}

template <typename T>
void atomic_wait_explicit(const atomic<T>* flag,
	typename atomic<T>::value_type old, memory_order mo) noexcept {
	flag->wait(old, mo);
}

template <typename T>
void atomic_notify_one(atomic<T>* flag) noexcept {
	flag->notify_one();
}

template <typename T>
void atomic_notify_all(atomic<T>* flag) noexcept {
	flag->notify_all();
}


template <typename T>
T atomic_fetch_add_explicit(atomic<T>* flag,
	typename atomic<T>::difference_type value, memory_order mo) noexcept {
	return flag->fetch_add(value, mo);
}

template <typename T>
T atomic_fetch_add_explicit(volatile atomic<T>* flag,
	typename atomic<T>::difference_type value, memory_order mo) noexcept {
	return flag->fetch_add(value, mo);
}

template <typename T>
T atomic_fetch_sub_explicit(atomic<T>* flag,
	typename atomic<T>::difference_type value, memory_order mo) noexcept {
	return flag->fetch_sub(value, mo);
}

template <typename T>
T atomic_fetch_sub_explicit(volatile atomic<T>* flag,
	typename atomic<T>::difference_type value, memory_order mo) noexcept {
	return flag->fetch_sub(value, mo);
}

template <typename T>
T atomic_fetch_and_explicit(atomic_base<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->fetch_and(value, mo);
}

template <typename T>
T atomic_fetch_and_explicit(volatile atomic_base<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->fetch_and(value, mo);
}

template <typename T>
T atomic_fetch_or_explicit(atomic_base<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->fetch_or(value, mo);
}

template <typename T>
T atomic_fetch_or_explicit(volatile atomic_base<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->fetch_or(value, mo);
}

template <typename T>
T atomic_fetch_xor_explicit(atomic_base<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->fetch_xor(value, mo);
}

template <typename T>
T atomic_fetch_xor_explicit(volatile atomic_base<T>* flag,
	type_identity_t<T> value, memory_order mo) noexcept {
	return flag->fetch_xor(value, mo);
}

template <typename T>
T atomic_fetch_add(atomic<T>* flag, typename atomic<T>::difference_type value) noexcept {
	return _MSTL atomic_fetch_add_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_add(volatile atomic<T>* flag, typename atomic<T>::difference_type value) noexcept {
	return _MSTL atomic_fetch_add_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_sub(atomic<T>* flag, typename atomic<T>::difference_type value) noexcept {
	return _MSTL atomic_fetch_sub_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_sub(volatile atomic<T>* flag, typename atomic<T>::difference_type value) noexcept {
	return _MSTL atomic_fetch_sub_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_and(atomic_base<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_fetch_and_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_and(volatile atomic_base<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_fetch_and_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_or(atomic_base<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_fetch_or_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_or(volatile atomic_base<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_fetch_or_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_xor(atomic_base<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_fetch_xor_explicit(flag, value, memory_order_seq_cst);
}

template <typename T>
T atomic_fetch_xor(volatile atomic_base<T>* flag, type_identity_t<T> value) noexcept {
	return _MSTL atomic_fetch_xor_explicit(flag, value, memory_order_seq_cst);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_HPP__
