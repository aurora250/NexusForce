#ifndef MSTL_CORE_FUNCTIONAL_APPLY_HPP__
#define MSTL_CORE_FUNCTIONAL_APPLY_HPP__
#include "../utility/tuple.hpp"
#include "invoke.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

template <template <typename...> class, typename, typename>
struct __apply_unpack_tuple : false_type {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, tuple<U...>> : bool_constant<Trait<T, U...>::value> {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, tuple<U...>&> : bool_constant<Trait<T, U&...>::value> {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, const tuple<U...>> : bool_constant<Trait<T, const U...>::value> {};

template<template<typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, const tuple<U...>&> : bool_constant<Trait<T, const U&...>::value> {};


template <typename F, typename Tuple, size_t... Idx>
constexpr auto __apply_impl(F&& f, Tuple&& t, _MSTL index_sequence<Idx...>) {
    return _MSTL invoke(_MSTL forward<F>(f),
        _MSTL forward<decltype(_MSTL get<Idx>(_MSTL forward<Tuple>(t)))>
            (_MSTL get<Idx>(_MSTL forward<Tuple>(t)))...
        );
}

MSTL_END_INNER__

template <typename F, typename Tuple>
constexpr auto apply(F&& f, Tuple&& t)
noexcept(_INNER __apply_unpack_tuple<_MSTL is_nothrow_invocable, F, Tuple>::value) -> decltype(auto) {
    using Indices = make_index_sequence<tuple_size<remove_reference_t<Tuple>>::value>;
    return _INNER __apply_impl(_MSTL forward<F>(f), _MSTL forward<Tuple>(t), Indices{});
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_APPLY_HPP__
