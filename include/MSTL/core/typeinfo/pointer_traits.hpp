#ifndef MSTL_CORE_UTILITY_POINTER_TRAITS_HPP__
#define MSTL_CORE_UTILITY_POINTER_TRAITS_HPP__

/**
 * @file pointer_traits.hpp
 * @brief MSTL指针特性库
 * @namespace MSTL
 * @ingroup PointerTraits
 *
 * 此文件提供了指针特性的实现，用于抽象和操作不同类型的指针。
 * 支持标准指针、智能指针、分配器指针等各种指针类型的统一操作。
 */

#include "type_traits.hpp"
#include "types.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup PointerTraitsUtilities 指针特性工具
 * @brief 提取和操作指针类型元信息的辅助工具
 * @{
 */

/**
 * @struct get_first_parameter
 * @brief 提取模板的第一个类型参数
 * @tparam T 模板类型
 */
template <typename T>
struct get_first_parameter;

/// @cond
template <template <typename, typename...> class T, typename First, typename... Rest>
struct get_first_parameter<T<First, Rest...>> {
    using type = First;
};
/// @endcond

/**
 * @typedef get_first_parameter_t
 * @brief get_first_parameter的便捷别名
 */
template <typename Ptr>
using get_first_parameter_t = typename get_first_parameter<Ptr>::type;


/**
 * @struct get_ptr_difference_type
 * @brief 获取指针的差值类型
 * @tparam T 指针类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 如果指针类型定义了difference_type，则使用该类型，否则使用默认的ptrdiff_t。
 */
template <typename T, typename Dummy = void>
struct get_ptr_difference_type {
    using type = ptrdiff_t;
};

/// @cond
template <typename T>
struct get_ptr_difference_type<T, enable_if_t<
    is_same<typename T::difference_type, typename T::difference_type>::value>> {
    using type = typename T::difference_type;
};
/// @endcond

/**
 * @typedef get_ptr_difference_type_t
 * @brief get_ptr_difference_type的便捷别名
 */
template <typename T>
using get_ptr_difference_type_t = typename get_ptr_difference_type<T>::type;


/**
 * @struct replace_first_parameter
 * @brief 替换模板的第一个类型参数
 * @tparam NewFirst 新的第一个参数
 * @tparam T 原始模板类型
 */
template <typename NewFirst, typename T>
struct replace_first_parameter;

/// @cond
template <typename NewFirst, template <typename, typename...> class T, typename First, typename... Rest>
struct replace_first_parameter<NewFirst, T<First, Rest...>> {
    using type = T<NewFirst, Rest...>;
};
/// @endcond

/**
 * @typedef replace_first_parameter_t
 * @brief replace_first_parameter的便捷别名
 */
template <typename T, typename U>
using replace_first_parameter_t = typename replace_first_parameter<T, U>::type;


/**
 * @struct get_rebind_type
 * @brief 获取指针的重新绑定类型
 * @tparam T 原始指针类型
 * @tparam U 新元素类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 如果指针类型定义了rebind模板，则使用该模板，否则通过替换第一个参数来创建新类型。
 */
template <typename T, typename U, typename Dummy = void>
struct get_rebind_type {
    using type = replace_first_parameter_t<U, T>;
};

/// @cond
template <typename T, typename U>
struct get_rebind_type<T, U, enable_if_t<
    is_same<typename T::template rebind<U>, typename T::template rebind<U>>::value>> {
    using type = typename T::template rebind<U>;
};
/// @endcond

/**
 * @typedef get_rebind_type_t
 * @brief get_rebind_type的便捷别名
 */
template <typename T, typename U>
using get_rebind_type_t = typename get_rebind_type<T, U>::type;

/** @} */ // PointerTraitsUtilities

MSTL_BEGIN_INNER__

/// @cond

/**
 * @struct __ptr_traits_base
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
struct __ptr_traits_base {
    using pointer = Ptr;  ///< 指针类型
    using element_type = Elem;  ///< 元素类型
    using difference_type = get_ptr_difference_type_t<Ptr>;  ///< 差值类型
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
struct __ptr_traits_extract<T, U, void_t<get_first_parameter_t<T>>>
    : __ptr_traits_base<T, typename get_first_parameter<T>::type> {
};

template <typename T>
struct __ptr_traits_extract<T, void_t<typename T::element_type>, void>
    : __ptr_traits_base<T, typename T::element_type> {
};
/// @endcond

MSTL_END_INNER__

/**
 * @defgroup PointerTraits 指针特性操作
 * @brief 统一处理各种指针类型的特性
 * @{
 */

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

MSTL_BEGIN_INNER__
/// @cond

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

/// @endcond
MSTL_END_INNER__

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
 * @defgroup AllocatorTraitsExtractors 分配器特性提取器
 * @brief 从分配器类型中提取相关特性
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
#endif // MSTL_CORE_UTILITY_POINTER_TRAITS_HPP__
