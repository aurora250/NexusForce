#ifndef MSTL_CORE_ASYNC_ATOMIC_WAIT_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_WAIT_HPP__
#include "../numeric/numeric_limits.hpp"
#include "../memory/memory.hpp"
#include "../exception/terminate.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include <intrin.h>
#include "../config/undef_cmacro.hpp"
#else
#include <unistd.h>
#include <cerrno>
#include <syscall.h>
#include <sched.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

#ifdef MSTL_PLATFORM_WINDOWS__
using platform_wait_t = ::LONG;
#else
using platform_wait_t = int;
#endif

MSTL_INLINE17 constexpr size_t PLATFORM_WAIT_ALIGN = alignof(platform_wait_t);

template <typename T>
MSTL_INLINE17 constexpr bool PLATFORM_WAIT_USE_T = is_scalar_v<T>
	&& ((sizeof(T) == sizeof(_INNER platform_wait_t))
	&& (alignof(T*) >= _INNER PLATFORM_WAIT_ALIGN));

MSTL_INLINE17 constexpr auto ATOMIC_SPIN_COUNT_RELAX = 12;
MSTL_INLINE17 constexpr auto ATOMIC_SPIN_COUNT = 16;


enum class futex_wait_flags : int32_t {
	private_flag = 0,
	wait = 0,
	wake = 1,
	wait_bitset = 9,
	wake_bitset = 10,
	wait_private = wait | private_flag,
	wake_private = wake | private_flag,
	wait_bitset_private = wait_bitset | private_flag,
	wake_bitset_private = wake_bitset | private_flag,
	bitset_match_any = -1
};

struct default_spin_policy {
	bool operator ()() const noexcept { return false; }
};


template <typename T>
void platform_wait(const T* addr, const platform_wait_t val) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    auto p = reinterpret_cast<volatile platform_wait_t*>(const_cast<T*>(addr));
	const ::BOOL result = ::WaitOnAddress(
		p, const_cast<platform_wait_t*>(&val),
		sizeof(platform_wait_t),
		_MSTL numeric_limits<uint32_t>::max());

	if (result == 0) {
		::DWORD err = ::GetLastError();
		if (err != 0 && err != ERROR_TIMEOUT) {
			_MSTL terminate();
		}
	}
#else
    const auto err = ::syscall(
		SYS_futex, static_cast<const void*>(addr),
		static_cast<int32_t>(futex_wait_flags::wait_private),
		val, nullptr);

	if (!err) return;
	if (errno == EAGAIN) return;
	_MSTL terminate();
#endif
}

template <typename T>
void platform_notify(const T* addr, const bool all) noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    const auto p = reinterpret_cast<volatile platform_wait_t*>(const_cast<T*>(addr));
	if (all) {
		::WakeByAddressAll(const_cast<platform_wait_t*>(p));
	} else {
		::WakeByAddressSingle(const_cast<platform_wait_t*>(p));
	}
#else
	::syscall(
		SYS_futex, static_cast<const void*>(addr),
		static_cast<int32_t>(futex_wait_flags::wake_private),
		all ? numeric_limits<int32_t>::max() : 1);
#endif
}

MSTL_ALWAYS_INLINE_INLINE void thread_yield() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
	::SwitchToThread();
#else
	::sched_yield();
#endif
}

MSTL_ALWAYS_INLINE_INLINE void thread_relax() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
	::YieldProcessor();
#else
#if __has_builtin(__builtin_ia32_pause)
	__builtin_ia32_pause();
#else
	thread_yield();
#endif
#endif
}

template <typename Pred, typename Spin = default_spin_policy>
bool atomic_spin(Pred& pred, Spin spin = Spin{}) noexcept {
	for (auto idx = 0; idx < ATOMIC_SPIN_COUNT; ++idx) {
		if (pred()) return true;

		if (idx < ATOMIC_SPIN_COUNT_RELAX) {
			thread_relax();
		} else {
			thread_yield();
		}
	}

	while (spin()) {
		if (pred()) return true;
	}
	return false;
}

template <typename T>
bool atomic_compare(const T& lhs, const T& rhs) {
	return _MSTL memory_compare(&lhs, &rhs, sizeof(T)) == 0;
}


struct waiter_pool_base {
	static constexpr auto align_inner = 64;
	alignas(align_inner) platform_wait_t wait = 0;
	alignas(align_inner) platform_wait_t value = 0;

	waiter_pool_base() = default;

	void waiter_enter_wait() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
		::_InterlockedIncrement(&wait);
#else
		__atomic_fetch_add(&wait, 1, __ATOMIC_SEQ_CST);
#endif
	}

	void waiter_leave_wait() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
		::_InterlockedDecrement(&wait);
#else
		__atomic_fetch_sub(&wait, 1, __ATOMIC_RELEASE);
#endif
	}

	bool waiter_waiting() const noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
		platform_wait_t res = ::_InterlockedExchangeAdd(
			const_cast<volatile ::LONG*>(&wait), 0);
		return res != 0;
#else
		platform_wait_t res;
		__atomic_load(&wait, &res, __ATOMIC_SEQ_CST);
		return res != 0;
#endif
	}

	void waiter_notify(platform_wait_t* addr, bool all, const bool bare) const noexcept {
		if (addr == &value) {
#ifdef MSTL_PLATFORM_WINDOWS__
			::_InterlockedIncrement(addr);
#else
			__atomic_fetch_add(addr, 1, __ATOMIC_SEQ_CST);
#endif
			all = true;
		}
		if (bare || waiter_waiting()) {
			platform_notify(addr, all);
		}
	}

	static waiter_pool_base& waiter_for(const void* addr) noexcept{
		constexpr uintptr_t pool_size = 16;
		static waiter_pool_base waiter[pool_size];
		const auto key = (reinterpret_cast<uintptr_t>(addr) >> 2) % pool_size;
		return waiter[key];
	}
};

struct waiter_pool : waiter_pool_base {
	void waiter_do_wait(const platform_wait_t* addr, const platform_wait_t old) const noexcept {
		platform_wait(addr, old);
	}
};


template <typename T>
struct waiter_base {
private:
	template <typename U, enable_if_t<PLATFORM_WAIT_USE_T<U>, int> = 0>
	MSTL_ALWAYS_INLINE static void waiter_do_spin_v_impl(
		platform_wait_t*, const U& old, platform_wait_t& value) {
		_MSTL memory_copy(&value, &old, sizeof(value));
	}
	template <typename U, enable_if_t<!PLATFORM_WAIT_USE_T<U>, int> = 0>
	MSTL_ALWAYS_INLINE static void waiter_do_spin_v_impl(
		platform_wait_t* addr, const U&, platform_wait_t& value) {
#ifdef MSTL_PLATFORM_WINDOWS__
		value = ::_InterlockedExchangeAdd(addr, 0);
#else
		__atomic_load(addr, &value, __ATOMIC_ACQUIRE);
#endif
	}

public:
	using waiter_type = T;

	waiter_type& waiter_;
	platform_wait_t* addr_;

	template <typename U, enable_if_t<PLATFORM_WAIT_USE_T<U>, int> = 0>
	static platform_wait_t* waiter_wait_addr(const U* addr, platform_wait_t*) {
		return reinterpret_cast<platform_wait_t*>(const_cast<U*>(addr));
	}
	template <typename U, enable_if_t<!PLATFORM_WAIT_USE_T<U>, int> = 0>
	static platform_wait_t* waiter_wait_addr(const U*, platform_wait_t* wait) {
		return wait;
	}

	static waiter_type& waiter_for(const void* addr) noexcept {
		static_assert(sizeof(waiter_type) == sizeof(waiter_pool_base),
			"waiter_for should be same size with waiter_pool_base");
		auto& res = waiter_pool_base::waiter_for(addr);
		return reinterpret_cast<waiter_type&>(res);
	}

	template <typename U>
	explicit waiter_base(const U* addr) noexcept
	: waiter_(waiter_base::waiter_for(addr)),
	addr_(waiter_base::waiter_wait_addr(addr, &waiter_.value)) {}

	void waiter_notify(bool all, bool bare = false) noexcept {
		waiter_.waiter_notify(addr_, all, bare);
	}

	template <typename U, typename Func, typename Spin = default_spin_policy>
	static bool waiter_do_spin_v(platform_wait_t* addr, const U& old,
		Func f, platform_wait_t& value, Spin spin = Spin{}) {
		auto const pred = [=] {
			return !_INNER atomic_compare<U>(old, f());
		};
		waiter_base::waiter_do_spin_v_impl(addr, old, value);
		return _INNER atomic_spin(pred, spin);
	}

	template <typename U, typename Func, typename Spin = default_spin_policy>
	bool waiter_do_spin_v(const U& old, Func f, platform_wait_t& value, Spin spin = Spin{}) {
		return waiter_base::waiter_do_spin_v(addr_, old, f, value, spin);
	}

	template <typename Pred, typename Spin = default_spin_policy>
	static bool waiter_do_spin(const platform_wait_t* addr, Pred pred,
		platform_wait_t& value, Spin spin = Spin{}) {
#ifdef MSTL_PLATFORM_WINDOWS__
		value = ::_InterlockedExchangeAdd(const_cast<volatile LONG*>(addr), 0);
#else
		__atomic_load(addr, &value, __ATOMIC_ACQUIRE);
#endif
		return _INNER atomic_spin(pred, spin);
	}

	template <typename Pred, typename Spin = default_spin_policy>
	bool waiter_do_spin(Pred pred, platform_wait_t& value, Spin spin = Spin{}) {
		return waiter_base::waiter_do_spin(addr_, pred, value, spin);
	}
};

template <typename EntersWait>
struct waiter : waiter_base<waiter_pool> {
public:
	using base_type = waiter_base<waiter_pool>;

private:
	template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
	MSTL_ALWAYS_INLINE void enter() const {
		waiter_.waiter_enter_wait();
	}
	template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
	MSTL_ALWAYS_INLINE void enter() const noexcept {}

	template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
	MSTL_ALWAYS_INLINE void leave() const {
		waiter_.waiter_leave_wait();
	}
	template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
	MSTL_ALWAYS_INLINE void leave() const noexcept {}
	
public:
	template <typename T>
	explicit waiter(const T* addr) noexcept : base_type(addr) { enter(); }
	~waiter() { leave();}

	template <typename T, typename Func>
	void waiter_do_wait_v(T old, Func f) {
		do {
			platform_wait_t value;
			if (base_type::waiter_do_spin_v(old, f, value)) return;
			waiter_.waiter_do_wait(base_type::addr_, value);
		} while (_INNER atomic_compare<T>(old, f()));
	}

	template <typename Pred>
	void waiter_do_wait(Pred pred) noexcept {
		do {
			platform_wait_t value;
			if (base_type::waiter_do_spin(pred, value)) return;
			waiter_.waiter_do_wait(base_type::addr_, value);
		} while (!pred());
	}
};

using enters_wait = waiter<true_type>;
using bare_wait = waiter<false_type>;

MSTL_END_INNER__

template <typename T, typename Func>
void atomic_wait_address_v(const T* addr, T old, Func f) noexcept {
    _INNER enters_wait waiter(addr);
    waiter.waiter_do_wait_v(old, f);
}

template <typename T, typename Pred>
void atomic_wait_address(const T* addr, Pred pred) noexcept {
    _INNER enters_wait waiter(addr);
    waiter.waiter_do_wait(pred);
}

template <typename T>
void atomic_notify_address(const T* addr, const bool all) noexcept {
	_INNER bare_wait waiter(addr);
	waiter.waiter_notify(all);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_WAIT_HPP__
