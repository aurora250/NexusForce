#ifndef MSTL_CORE_ASYNC_ATOMIC_FUTEX_BASE_HPP__
#define MSTL_CORE_ASYNC_ATOMIC_FUTEX_BASE_HPP__
#include "../time/clocks.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API atomic_futex_base {
	bool futex_wait_until(unsigned* addr, unsigned value, bool has_timeout,
		_MSTL_CHRONO seconds sec, _MSTL_CHRONO nanoseconds ns);

	bool futex_wait_until_steady(unsigned* addr, unsigned value, bool has_timeout,
		_MSTL_CHRONO seconds sec, _MSTL_CHRONO nanoseconds ns);

	static void futex_notify_all(unsigned* addr);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_ATOMIC_FUTEX_BASE_HPP__
