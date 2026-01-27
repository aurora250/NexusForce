#ifndef MSTL_CORE_ASYNC_FUTEX_HPP__
#define MSTL_CORE_ASYNC_FUTEX_HPP__
#include "MSTL/core/typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

using platform_wait_t =
#ifdef MSTL_PLATFORM_WINDOWS__
    long;
#else
    int;
#endif

template <typename T>
MSTL_INLINE17 constexpr bool platform_wait_valid_v = is_scalar_v<T>
    && sizeof(T) == sizeof(platform_wait_t)
    && alignof(T*) >= alignof(platform_wait_t);

enum class futex_wait_flags : platform_wait_t {
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


void MSTL_API futex_wait(void* addr, int32_t value) noexcept;

bool MSTL_API futex_wait_until(void* addr, int32_t value, bool has_timeout, int64_t sec, int64_t ns);

bool MSTL_API futex_wait_until_steady(void* addr, int32_t value, bool has_timeout, int64_t sec, int64_t ns);

void MSTL_API futex_notify(void* addr, bool all) noexcept;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ASYNC_FUTEX_HPP__
