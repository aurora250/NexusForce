#ifndef MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
#define MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__

/**
 * @file iterator_traits.hpp
 * @brief MSTL迭代器萃取
 *
 * 此文件提供了迭代器萃取实现，用于在编译时查询迭代器的各种属性，
 * 为算法提供统一的迭代器信息访问接口。
 */

#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/tags.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup IteratorTraits 迭代器萃取
 * @brief 迭代器萃取的实现
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__

template <typename, typename = void>
struct iterator_traits_base {};

template <typename Iterator>
struct iterator_traits_base<Iterator,
    void_t<typename Iterator::iterator_category, typename Iterator::value_type,
    typename Iterator::difference_type, typename Iterator::pointer, typename Iterator::reference>>
{
    using iterator_category = typename Iterator::iterator_category;
    using value_type        = typename Iterator::value_type;
    using difference_type   = typename Iterator::difference_type;
    using pointer           = typename Iterator::pointer;
    using reference         = typename Iterator::reference;
};

MSTL_END_INNER__
/// @endcond


/**
 * @struct iterator_traits
 * @brief 迭代器特性主模板
 * @tparam Iterator 迭代器类型
 *
 * 为所有迭代器类型提供统一的特性查询接口。
 * 主模板使用iterator_traits_base从自定义迭代器类型提取特性。
 *
 * 这是迭代器特性的主要接口，算法应通过此模板查询迭代器属性。
 */
template <typename Iterator>
struct iterator_traits : _INNER iterator_traits_base<Iterator> {};

/**
 * @brief 原始指针的迭代器特性特化
 * @tparam T 指针指向的类型
 *
 * 为原始指针提供优化的迭代器特性定义。
 * 原始指针被视为连续迭代器（contiguous iterator）。
 *
 * @note 要求T必须是对象类型（满足is_object<T>）。
 */
template <typename T>
struct iterator_traits<T*> {
    static_assert(is_object<T>::value, "iterator traits requires object types.");

    using iterator_category = contiguous_iterator_tag; ///< 迭代器类别
    using value_type        = remove_cv_t<T>;          ///< 值类型
    using difference_type   = ptrdiff_t;               ///< 差值类型
    using pointer           = T*;                      ///< 指针类型
    using reference         = T&;                      ///< 引用类型
};

/**
 * @typedef iter_category_t
 * @brief 获取迭代器的类别标签
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
using iter_category_t   = typename iterator_traits<Iterator>::iterator_category;

/**
 * @typedef iter_value_t
 * @brief 获取迭代器的值类型
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
using iter_value_t      = typename iterator_traits<Iterator>::value_type;

/**
 * @typedef iter_difference_t
 * @brief 获取迭代器的差值类型
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
using iter_difference_t = typename iterator_traits<Iterator>::difference_type;

/**
 * @typedef iter_pointer_t
 * @brief 获取迭代器的指针类型
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
using iter_pointer_t    = typename iterator_traits<Iterator>::pointer;

/**
 * @typedef iter_reference_t
 * @brief 获取迭代器的引用类型
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
using iter_reference_t  = typename iterator_traits<Iterator>::reference;


/**
 * @typedef iter_map_key_t
 * @brief 从映射迭代器中提取键类型
 * @tparam Iterator 映射迭代器类型
 *
 * 取得迭代器指向的value_type的first_type成员。
 * 移除键的const限定符，返回可修改的键类型。
 */
template <typename Iterator>
#ifdef MSTL_STANDARD_20__
requires is_pair_v<iter_value_t<Iterator>>
#endif
using iter_map_key_t = remove_const_t<typename iter_value_t<Iterator>::first_type>;

/**
 * @typedef iter_map_value_t
 * @brief 从映射迭代器中提取值类型
 * @tparam Iterator 映射迭代器类型
 *
 * 取得迭代器指向的value_type的second_type成员。
 */
template <typename Iterator>
#ifdef MSTL_STANDARD_20__
requires is_pair_v<iter_value_t<Iterator>>
#endif
using iter_map_value_t = typename iter_value_t<Iterator>::second_type;

/** @} */ // IteratorTraits

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
