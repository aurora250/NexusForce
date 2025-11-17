#ifndef MSTL_CORE_FUNCTIONAL_APPLY_HPP__
#define MSTL_CORE_FUNCTIONAL_APPLY_HPP__
#include "../compound/tuple.hpp"
#include "invoke.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

template <template <typename...> class, typename, typename>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v = false;

template <template <typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, tuple<U...>> = Trait<T, U...>::value;

template <template <typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, tuple<U...>&> = Trait<T, U&...>::value;

template <template <typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, const tuple<U...>> = Trait<T, const U...>::value;

template<template<typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, const tuple<U...>&> = Trait<T, const U&...>::value;

template <template <typename...> class Trait, typename T, typename Tuple>
struct __apply_unpack_tuple : bool_constant<__apply_unpack_tuple_v<Trait, T, Tuple>> {};

template <typename F, typename Tuple, size_t... Idx>
constexpr decltype(auto) __apply_impl(F&& f, Tuple&& t, _MSTL index_sequence<Idx...>) {
    return _MSTL invoke(_MSTL forward<F>(f),
        _MSTL forward<decltype(_MSTL get<Idx>(_MSTL forward<Tuple>(t)))>
            (_MSTL get<Idx>(_MSTL forward<Tuple>(t)))...
        );
}

MSTL_END_INNER__

template <typename F, typename Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
noexcept(_INNER __apply_unpack_tuple<_MSTL is_nothrow_invocable, F, Tuple>::value) {
    using Indices = make_index_sequence<tuple_size_v<remove_reference_t<Tuple>>>;
    return _INNER __apply_impl(_MSTL forward<F>(f), _MSTL forward<Tuple>(t), Indices{});
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_APPLY_HPP__
