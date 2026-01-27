#ifndef MSTL_CORE_ASYNC_ATOMIC_FUTEX_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_FUTEX_HPP__
#include "MSTL/core/async/atomic.hpp"
#include "MSTL/core/time/clocks.hpp"
MSTL_BEGIN_NAMESPACE__

template <uint32_t WaiterBit = 0x80000000>
class atomic_futex {
    atomic<uint32_t> data_;

	uint32_t load_and_test_until(uint32_t assumed, const uint32_t operand,
	    const bool equal, const memory_order mo, const bool has_timeout,
	    const int64_t sec, const int64_t ns) {
		for (;;) {
			data_.fetch_or(WaiterBit, memory_order_relaxed);
			const bool ret = _MSTL futex_wait_until(
				&data_, assumed | WaiterBit, has_timeout, sec, ns);
			assumed = load(mo);
			if (!ret || ((operand == assumed) == equal)) {
				return assumed;
			}
		}
	}

	uint32_t load_and_test_until_steady(uint32_t assumed, const uint32_t operand,
	    const bool equal, const memory_order mo, const bool has_timeout,
	    const int64_t sec, const int64_t ns) {
		for (;;) {
			data_.fetch_or(WaiterBit, memory_order_relaxed);
			const bool ret = _MSTL futex_wait_until_steady(
				&data_, assumed | WaiterBit, has_timeout, sec, ns);
			assumed = load(mo);
			if (!ret || ((operand == assumed) == equal)) {
				return assumed;
			}
		}
	}

	uint32_t load_and_test(const uint32_t assumed, const uint32_t operand,
	    const bool equal, const memory_order mo) {
		return load_and_test_until(
			assumed, operand, equal, mo, false, 0, 0);
	}

	template <typename Dur>
	uint32_t load_and_test_until_impl(uint32_t assumed, uint32_t operand,
	    bool equal, memory_order mo,
	    const time_point<system_clock, Dur>& atime) {
		auto sec = atime.to_sec();
		const auto ns = nanoseconds(atime - sec).count();
		return this->load_and_test_until(
			assumed, operand, equal, mo,
			true, sec.since_epoch().count(), ns);
	}

	template <typename Dur>
	uint32_t load_and_test_until_impl(uint32_t assumed, uint32_t operand,
	    bool equal, memory_order mo,
	    const time_point<steady_clock, Dur>& atime) {
		auto sec = atime.to_sec();
		const auto ns = nanoseconds(atime - sec).count();
		return this->load_and_test_until_steady(
			assumed, operand, equal, mo,
		    true, sec.since_epoch().count(), ns);
	}

public:
	explicit atomic_futex(const uint32_t data) : data_(data) {}

	MSTL_NODISCARD MSTL_ALWAYS_INLINE uint32_t
    load(const memory_order mo) const {
		return data_.load(mo) & ~WaiterBit;
	}
	
	MSTL_ALWAYS_INLINE uint32_t
	load_when_not_equal(const uint32_t value, const memory_order mo) {
		const uint32_t old = load(mo);
		if ((old & ~WaiterBit) != value) {
			return (old & ~WaiterBit);
		}
		return load_and_test(old, value, false, mo);
	}

	MSTL_ALWAYS_INLINE void
	load_when_equal(const uint32_t value, const memory_order mo) {
		const uint32_t old = load(mo);
		if ((old & ~WaiterBit) == value) {
			return;
		}
		load_and_test(old, value, true, mo);
	}

	template <typename Rep, typename Period>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_for(const uint32_t value, const memory_order mo,
		const duration<Rep, Period>& rtime) {
		const auto atime = steady_clock::now() + ceil<steady_clock::duration>(rtime);
		return this->load_when_equal_until(value, mo, atime);
	}

	template <typename Clock, typename Dur>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_until(const uint32_t value, const memory_order mo,
		const time_point<Clock, Dur>& atime) {
		auto now = Clock::now();
		do {
			const auto s_atime = steady_clock::now() + ceil<steady_clock::duration>(atime - now);
			if (this->load_when_equal_until(value, mo, s_atime)) {
				return true;
			}
			now = Clock::now();
		} while (now < atime);
		return false;
	}

	template <typename Dur>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_until(const uint32_t value, const memory_order mo,
	    const time_point<system_clock, Dur>& atime) {
		uint32_t old = load(mo);
		if ((old & ~WaiterBit) == value) {
			return true;
		}
		old = this->load_and_test_until_impl(old, value, true, mo, atime);
		return (old & ~WaiterBit) == value;
	}

	template <typename Dur>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_until(const uint32_t value, const memory_order mo,
	    const time_point<steady_clock, Dur>& atime) {
		uint32_t old = load(mo);
		if ((old & ~WaiterBit) == value) {
			return true;
		}
		old = this->load_and_test_until_impl(old, value, true, mo, atime);
		return (old & ~WaiterBit) == value;
	}

	MSTL_ALWAYS_INLINE void store_notify_all(const uint32_t value, const memory_order mo) {
        const auto futex = static_cast<uint32_t*>(static_cast<void*>(&data_));
		if (data_.exchange(value, mo) & WaiterBit) {
			_MSTL futex_notify(futex, true);
		}
	}
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_FUTEX_HPP__
