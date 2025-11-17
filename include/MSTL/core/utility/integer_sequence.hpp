#ifndef MSTL_CORE_UTILITY_INTEGER_SEQUENCE_HPP__
#define MSTL_CORE_UTILITY_INTEGER_SEQUENCE_HPP__
#include "type_traits.hpp"
#include "../config/types.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, T... Values>
struct integer_sequence {
    static_assert(is_integral<T>::value, "integer sequence requires integral types.");

    using value_type = T;

    MSTL_NODISCARD static constexpr size_t size() noexcept {
        return sizeof...(Values);
    }
};

template <typename T, T Size>
using make_integer_sequence =
#if defined(MSTL_COMPILER_MSVC__) || defined(MSTL_COMPILER_CLANG__)
__make_integer_seq<integer_sequence, T, Size>;
#else
    integer_sequence<T, __integer_pack(Size)...>;
#endif

template <size_t... Values>
using index_sequence = integer_sequence<size_t, Values...>;
template <size_t Size>
using make_index_sequence = make_integer_sequence<size_t, Size>;
template <typename... Types>
using index_sequence_for = make_index_sequence<sizeof...(Types)>;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_INTEGER_SEQUENCE_HPP__
