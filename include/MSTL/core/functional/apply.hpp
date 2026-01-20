#ifndef MSTL_CORE_FUNCTIONAL_APPLY_HPP__
#define MSTL_CORE_FUNCTIONAL_APPLY_HPP__

/**
 * @file apply.hpp
 * @brief MSTL元组应用函数
 *
 * 此文件提供了元组应用函数的实现，用于将元组中的元素解包作为参数调用函数。
 */

#include "../utility/tuple.hpp"
#include "invoke.hpp"
MSTL_BEGIN_NAMESPACE__

/// @cond
MSTL_BEGIN_INNER__

template <template <typename...> class, typename, typename>
struct __apply_unpack_tuple : false_type {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, tuple<U...>> : bool_constant<Trait<T, U...>::value> {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, tuple<U...>&> : bool_constant<Trait<T, U&...>::value> {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, const tuple<U...>> : bool_constant<Trait<T, const U...>::value> {};

template <template <typename...> class Trait, typename T, typename... U>
struct __apply_unpack_tuple<Trait, T, const tuple<U...>&> : bool_constant<Trait<T, const U&...>::value> {};


template <typename F, typename Tuple, size_t... Idx>
constexpr auto __apply_impl(F&& f, Tuple&& t, _MSTL index_sequence<Idx...>) {
    return _MSTL invoke(_MSTL forward<F>(f),
        _MSTL forward<decltype(_MSTL get<Idx>(_MSTL forward<Tuple>(t)))>
            (_MSTL get<Idx>(_MSTL forward<Tuple>(t)))...
        );
}

MSTL_END_INNER__
/// @endcond

/**
 * @defgroup ApplyFunction 元组应用函数
 * @brief 将元组元素解包作为参数调用的主函数
 * @{
 */

/**
 * @brief 将元组元素解包作为参数调用函数
 * @tparam Func 可调用对象类型
 * @tparam Tuple 元组类型
 * @param f 可调用对象
 * @param t 元组
 * @return 函数调用结果
 *
 * 将元组中的元素解包，作为参数调用函数。
 * 支持完美转发，可以处理元组的不同引用和常量限定。
 */
template <typename Func, typename Tuple>
constexpr auto apply(Func&& f, Tuple&& t)
noexcept(_INNER __apply_unpack_tuple<_MSTL is_nothrow_invocable, Func, Tuple>::value) -> decltype(auto) {
    using Indices = make_index_sequence<tuple_size<remove_reference_t<Tuple>>::value>;
    return _INNER __apply_impl(_MSTL forward<Func>(f), _MSTL forward<Tuple>(t), Indices{});
}

/** @} */ // ApplyFunction

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_APPLY_HPP__
