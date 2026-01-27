#ifndef MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
#define MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__

/**
 * @file iterator_traits.hpp
 * @brief MSTL迭代器萃取和指针萃取
 *
 * 此文件提供了迭代器萃取实现，用于在编译时查询迭代器的各种属性，
 * 为算法提供统一的迭代器信息访问接口；
 *
 * 提供了指针萃取的实现，用于抽象和操作不同类型的指针，
 * 以支持各种指针类型的统一操作。
 */

#include "MSTL/core/typeinfo/type_traits.hpp"
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

/**
 * @defgroup PointerTraits 指针萃取
 * @brief 统一处理各种指针类型的特性
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct pointer_traits_base
 * @brief 指针特性的基础实现类
 * @tparam Ptr 指针类型
 * @tparam Elem 元素类型
 *
 * 提供指针类型的基本特性定义：
 * - pointer: 指针类型自身
 * - element_type: 指向的元素类型
 * - difference_type: 指针差值类型
 * - reference: 引用类型
 * - rebind: 重新绑定到其他元素类型的模板
 * - pointer_to: 从引用创建指针的静态方法
 */
template <typename Ptr, typename Elem>
struct pointer_traits_base {
    using pointer = Ptr;  ///< 指针类型
    using element_type = Elem;  ///< 元素类型
    using difference_type = get_ptr_difference_t<Ptr>;  ///< 差值类型
    using reference = conditional_t<is_void<Elem>::value, char, Elem>&;  ///< 引用类型

    /**
     * @brief 重新绑定到其他元素类型的模板
     * @tparam U 新元素类型
     */
    template <typename U>
    using rebind = typename get_rebind_type<Ptr, U>::type;

    /**
     * @brief 从引用创建指针
     * @param x 元素引用
     * @return 指向该元素的指针
     */
    MSTL_NODISCARD static constexpr pointer pointer_to(reference x)
        noexcept(noexcept(Ptr::pointer_to(x))) {
        return Ptr::pointer_to(x);
    }
};

template <typename, typename = void, typename = void>
struct __ptr_traits_extract {};

template <typename T, typename U>
struct __ptr_traits_extract<T, U, void_t<get_first_temp_para_t<T>>>
    : pointer_traits_base<T, typename get_first_temp_para<T>::type> {
};

template <typename T>
struct __ptr_traits_extract<T, void_t<typename T::element_type>, void>
    : pointer_traits_base<T, typename T::element_type> {
};

MSTL_END_INNER__
/// @endcond


/**
 * @struct pointer_traits
 * @brief 指针特性主模板
 * @tparam T 指针类型
 *
 * 为任意指针类型提供统一的接口，包括：
 * 1. 元素类型提取
 * 2. 差值类型提取
 * 3. 重新绑定能力
 * 4. 从引用创建指针
 */
template <typename T>
struct pointer_traits : _INNER __ptr_traits_extract<T> {};


/**
 * @brief 原始指针的特化版本
 * @tparam T 元素类型
 */
template <typename T>
struct pointer_traits<T*> {
    using pointer = T*;  ///< 指针类型
    using element_type = T;  ///< 元素类型
    using difference_type = ptrdiff_t;  ///< 差值类型
    using reference = conditional_t<is_void<T>::value, char, T>&;  ///< 引用类型

   /**
     * @brief 重新绑定到其他元素类型的模板
     * @tparam U 新元素类型
     */
    template <typename U>
    using rebind = U*;

    /**
     * @brief 从引用创建指针
     * @param x 元素引用
     * @return 指向该元素的原始指针
     */
    MSTL_NODISCARD static constexpr pointer pointer_to(reference x) noexcept {
        return _MSTL addressof(x);
    }
};

/**
 * @typedef pointer_rebind
 * @brief 指针重新绑定的便捷别名
 * @tparam Ptr 原始指针类型
 * @tparam T 新元素类型
 *
 * 将指针Ptr重新绑定到新元素类型T。
 */
template <typename Ptr, typename T>
using pointer_rebind = typename pointer_traits<Ptr>::template rebind<T>;

/**
 * @brief 移除指针的const限定符
 * @tparam Ptr 原始指针类型
 * @param ptr 要转换的指针
 * @return 移除了const限定符的指针
 */
template <typename Ptr>
constexpr decltype(auto) ptr_const_cast(Ptr ptr) noexcept {
    using T = typename pointer_traits<Ptr>::element_type;
    using NonConst = remove_const_t<T>;
    using Dest = typename pointer_traits<Ptr>::template rebind<NonConst>;

    return pointer_traits<Dest>::pointer_to(const_cast<NonConst&>(*ptr));
}

/**
 * @brief 移除原始指针的const限定符
 * @tparam T 元素类型
 * @param ptr 要转换的原始指针
 * @return 移除了const限定符的原始指针
 */
template <typename T>
constexpr decltype(auto) ptr_const_cast(T* ptr) noexcept {
    return const_cast<remove_const_t<T>*>(ptr);
}


/// @cond
MSTL_BEGIN_INNER__

template <typename T>
constexpr T* __to_address(T* ptr) noexcept {
    static_assert(!is_function<T>::value, "not a function pointer");
    return ptr;
}

template <typename Ptr>
constexpr decltype(auto) __to_address(const Ptr& ptr) noexcept {
    return pointer_traits<Ptr>::to_address(ptr);
}

template <typename Ptr, typename... None, enable_if_t<!has_base<Ptr>::value, int> = 0>
constexpr decltype(auto) __to_address(const Ptr& ptr, None...) noexcept {
    return __to_address(ptr.operator->());
}

template <typename Ptr, typename... None, enable_if_t<has_base<Ptr>::value, int> = 0>
constexpr decltype(auto) __to_address(const Ptr& ptr, None...) noexcept {
    return __to_address(ptr.base().operator->());
}

MSTL_END_INNER__
/// @endcond


/**
 * @brief 安全地获取原始指针指向的地址
 * @tparam T 元素类型
 * @param ptr 原始指针
 * @return 指针指向的地址
 */
template <typename T>
constexpr T* to_address(T* ptr) noexcept {
    return _INNER __to_address(ptr);
}

/**
 * @brief 安全地获取任意指针类型指向的地址
 * @tparam Ptr 指针类型
 * @param ptr 指针对象
 * @return 指针指向的地址
 */
template <typename Ptr>
constexpr decltype(auto) to_address(const Ptr& ptr) noexcept {
    return _INNER __to_address(ptr);
}

/** @} */ // PointerTraits

/**
 * @defgroup AllocatorTraitsExtractors 分配器特性萃取器
 * @brief 从分配器类型中萃取相关特性
 * @{
 */

/**
 * @struct get_pointer_type
 * @brief 获取分配器的指针类型
 * @tparam T 分配器类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 如果分配器定义了pointer类型，则使用该类型，否则使用value_type*。
 */
template <typename T, typename Dummy = void>
struct get_pointer_type {
    using type = typename T::value_type*;
};

/// @cond
template <typename T>
struct get_pointer_type<T, void_t<typename T::pointer>> {
    using type = typename T::pointer;
};
/// @endcond


/**
 * @struct get_difference_type
 * @brief 获取分配器的差值类型
 * @tparam T 分配器类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T, typename Dummy = void>
struct get_difference_type {
    using pointer = typename get_pointer_type<T>::type;
    using type = typename pointer_traits<pointer>::difference_type;
};

/// @cond
template <typename T>
struct get_difference_type<T, void_t<typename T::difference_type>> {
    using type = typename T::difference_type;
};
/// @endcond


/**
 * @struct get_size_type
 * @brief 获取分配器的大小类型
 * @tparam T 分配器类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T, typename Dummy = void>
struct get_size_type {
    using type = make_unsigned_t<typename get_difference_type<T>::type>;
};

/// @cond
template <typename T>
struct get_size_type<T, void_t<typename T::size_type>> {
    using type = typename T::size_type;
};
/// @endcond

/** @} */ // AllocatorTraitsExtractors

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_ITERATOR_TRAITS_HPP__
