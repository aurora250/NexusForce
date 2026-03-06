#ifndef NEFORCE_CORE_FUNCTIONAL_APPLY_HPP__
#define NEFORCE_CORE_FUNCTIONAL_APPLY_HPP__

/**
 * @file apply.hpp
 * @brief 元组应用函数
 *
 * 此文件提供了元组应用函数的实现，用于将元组中的元素解包作为参数调用函数。
 */

#include "NeForce/core/utility/tuple.hpp"
#include "NeForce/core/functional/invoke.hpp"
NEFORCE_BEGIN_NAMESPACE__

/// @cond
NEFORCE_BEGIN_INNER__

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
constexpr auto __apply_impl(F&& f, Tuple&& t, _NEFORCE index_sequence<Idx...>) {
    return _NEFORCE invoke(_NEFORCE forward<F>(f),
        _NEFORCE forward<decltype(_NEFORCE get<Idx>(_NEFORCE forward<Tuple>(t)))>
            (_NEFORCE get<Idx>(_NEFORCE forward<Tuple>(t)))...
        );
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @defgroup Tuple 元组
 * @brief 元组的主模板、特化实现和辅助函数
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
constexpr decltype(auto) apply(Func&& f, Tuple&& t)
noexcept(_INNER __apply_unpack_tuple<_NEFORCE is_nothrow_invocable, Func, Tuple>::value) {
    using Indices = make_index_sequence<tuple_size<remove_reference_t<Tuple>>::value>;
    return _INNER __apply_impl(_NEFORCE forward<Func>(f), _NEFORCE forward<Tuple>(t), Indices{});
}

/** @} */ // Tuple

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FUNCTIONAL_APPLY_HPP__
