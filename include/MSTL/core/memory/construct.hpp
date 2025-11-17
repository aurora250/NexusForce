#ifndef MSTLCORE_MEMORY_CONSTRUCT_HPP__
#define MSTLCORE_MEMORY_CONSTRUCT_HPP__
#include "../utility/concepts.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename... Args>
MSTL_CONSTEXPR20 void* construct(T* ptr, Args&&... args)
noexcept(is_nothrow_constructible_v<T, Args...>) {
    return new (static_cast<void*>(ptr)) T(_MSTL forward<Args>(args)...);
}

template <typename T>
MSTL_CONSTEXPR20 void destroy(T* pointer) noexcept(is_nothrow_destructible_v<T>) {
    pointer->~T();
}

template <typename Iterator, enable_if_t<
    is_iter_v<Iterator> && !is_trivially_destructible_v<iter_value_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 void destroy(Iterator first, Iterator last)
noexcept(is_nothrow_destructible_v<iter_value_t<Iterator>>) {
    for (; first < last; ++first) _MSTL destroy(&*first);
}

template <typename Iterator, enable_if_t<
    is_iter_v<Iterator> && is_trivially_destructible_v<iter_value_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 void destroy(Iterator, Iterator) noexcept {}


MSTL_END_NAMESPACE__
#endif // MSTLCORE_MEMORY_CONSTRUCT_HPP__
