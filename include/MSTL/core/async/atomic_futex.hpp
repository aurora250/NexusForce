#ifndef MSTL_CORE_ASYNC_ATOMIC_FUTEX_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_FUTEX_HPP__
#include "atomic_futex_base.hpp"
#include "atomic.hpp"
MSTL_BEGIN_NAMESPACE__

template <unsigned WaiterBit = 0x80000000>
class atomic_futex : atomic_futex_base {
    using clock_t = steady_clock;

    atomic<unsigned> data_;

	unsigned load_and_test_until(unsigned assumed, const unsigned operand,
	    const bool equal, const memory_order mo, const bool has_timeout,
	    const seconds sec, const nanoseconds ns) {
		for (;;) {
			data_.fetch_or(WaiterBit, memory_order_relaxed);
			const bool ret = futex_wait_until(
				static_cast<unsigned*>(static_cast<void*>(&data_)),
				assumed | WaiterBit, has_timeout, sec, ns);
			assumed = load(mo);
			if (!ret || ((operand == assumed) == equal)) {
				return assumed;
			}
		}
	}

	unsigned load_and_test_until_steady(unsigned assumed, const unsigned operand,
	    const bool equal, const memory_order mo, const bool has_timeout,
	    const seconds sec, const nanoseconds ns) {
		for (;;) {
			data_.fetch_or(WaiterBit, memory_order_relaxed);
			const bool ret = futex_wait_until_steady(
				static_cast<unsigned*>(static_cast<void*>(&data_)),
				assumed | WaiterBit, has_timeout, sec, ns);
			assumed = load(mo);
			if (!ret || ((operand == assumed) == equal)) {
				return assumed;
			}
		}
	}

	unsigned load_and_test(const unsigned assumed, const unsigned operand,
	    const bool equal, const memory_order mo) {
		return load_and_test_until(assumed, operand, equal, mo, false, {}, {});
	}

	template <typename Dur>
	unsigned load_and_test_until_impl(unsigned assumed, unsigned operand,
	    bool equal, memory_order mo,
	    const time_point<system_clock, Dur>& atime) {
		auto sec = time_point_cast<seconds>(atime);
		auto ns = duration_cast<nanoseconds>(atime - sec);
		return this->load_and_test_until(
			assumed, operand, equal, mo,
			true, sec.time_since_epoch(), ns);
	}

	template <typename Dur>
	unsigned load_and_test_until_impl(unsigned assumed, unsigned operand,
	    bool equal, memory_order mo,
	    const time_point<steady_clock, Dur>& atime) {
		auto sec = time_point_cast<seconds>(atime);
		auto ns = duration_cast<nanoseconds>(atime - sec);
		return this->load_and_test_until_steady(
			assumed, operand, equal, mo,
		    true, sec.time_since_epoch(), ns);
	}

public:
	explicit atomic_futex(const unsigned data) : data_(data) {}

	MSTL_NODISCARD MSTL_ALWAYS_INLINE unsigned
    load(const memory_order mo) const {
		return data_.load(mo) & ~WaiterBit;
	}
	
	MSTL_ALWAYS_INLINE unsigned
	load_when_not_equal(const unsigned value, const memory_order mo) {
		const unsigned old = load(mo);
		if ((old & ~WaiterBit) != value) {
			return (old & ~WaiterBit);
		}
		return load_and_test(old, value, false, mo);
	}

	MSTL_ALWAYS_INLINE void
	load_when_equal(const unsigned value, const memory_order mo) {
		const unsigned old = load(mo);
		if ((old & ~WaiterBit) == value) {
			return;
		}
		load_and_test(old, value, true, mo);
	}

	template <typename Rep, typename Period>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_for(const unsigned value, const memory_order mo,
		const duration<Rep, Period>& rtime) {
		const auto atime = clock_t::now() + ceil<clock_t::duration>(rtime);
		return this->load_when_equal_until(value, mo, atime);
	}

	template <typename Clock, typename Dur>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_until(const unsigned value, const memory_order mo,
		const time_point<Clock, Dur>& atime) {
		auto now = Clock::now();
		do {
			const auto s_atime = clock_t::now() +
				ceil<clock_t::duration>(atime - now);
			if (this->load_when_equal_until(value, mo, s_atime)) {
				return true;
			}
			now = Clock::now();
		} while (now < atime);
		return false;
	}

	template <typename Dur>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_until(const unsigned value, const memory_order mo,
	    const time_point<system_clock, Dur>& atime) {
		unsigned old = load(mo);
		if ((old & ~WaiterBit) == value) {
			return true;
		}
		old = this->load_and_test_until_impl(old, value, true, mo, atime);
		return (old & ~WaiterBit) == value;
	}

	template <typename Dur>
	MSTL_ALWAYS_INLINE bool
	load_when_equal_until(const unsigned value, const memory_order mo,
	    const time_point<steady_clock, Dur>& atime) {
		unsigned old = load(mo);
		if ((old & ~WaiterBit) == value) {
			return true;
		}
		old = this->load_and_test_until_impl(old, value, true, mo, atime);
		return (old & ~WaiterBit) == value;
	}

	MSTL_ALWAYS_INLINE void store_notify_all(const unsigned value, const memory_order mo) {
        const auto futex = static_cast<unsigned*>(static_cast<void*>(&data_));
		if (data_.exchange(value, mo) & WaiterBit) {
			futex_notify_all(futex);
		}
	}
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_FUTEX_HPP__
