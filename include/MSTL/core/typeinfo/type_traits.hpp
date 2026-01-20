#ifndef MSTL_CORE_TYPEINFO_TYPE_TRAITS_HPP__
#define MSTL_CORE_TYPEINFO_TYPE_TRAITS_HPP__

/**
 * @file type_traits.hpp
 * @brief MSTL类型萃取
 *
 * 此文件提供了完整的类型萃取实现，用于在编译时查询和操作类型信息。
 */

#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup TypeTraitsUtilities 类型推导辅助工具
 * @brief 类型推导辅助工具类
 * @{
 */

/**
 * @struct integral_constant
 * @brief 整数常量包装器
 * @tparam T 整数类型
 * @tparam Value 整数值
 *
 * 将编译时常量包装为类型，是类型特性库的基础设施。
 * 提供value静态成员和operator()用于获取值。
 */
template <typename T, T Value>
struct integral_constant {
    static constexpr T value = Value; ///< 存储的整数值

    using value_type    = T;          ///< 值类型
    using type          = integral_constant<T, Value>; ///< 自身类型

    /**
     * @brief 显式转换到值类型的运算符
     * @return Value 的转换结果
     */
    constexpr explicit operator value_type() const noexcept {
        return value;
    }

    /**
     * @brief 函数调用运算符，用于获取值
     * @return 存储的整数值
     */
    MSTL_NODISCARD constexpr value_type operator ()() const noexcept {
        return value;
    }
};


/**
 * @typedef bool_constant
 * @brief 布尔常量包装器
 * @tparam Value 布尔值
 */
template <bool Value>
using bool_constant = integral_constant<bool, Value>;

using true_type = bool_constant<true>;   ///< 表示true的类型
using false_type = bool_constant<false>; ///< 表示false的类型

/**
 * @typedef uint32_constant
 * @brief 32位无符号整数常量包装器
 * @tparam Value 32位无符号整数值
 */
template <uint32_t Value>
using uint32_constant = integral_constant<uint32_t, Value>;

/**
 * @typedef uint64_constant
 * @brief 64位无符号整数常量包装器
 * @tparam Value 64位无符号整数值
 */
template <uint64_t Value>
using uint64_constant = integral_constant<uint64_t, Value>;


/**
 * @typedef void_t
 * @brief 将任意类型映射为void
 * @tparam Types 任意类型序列
 *
 * 用于SFINAE技术中检测表达式是否合法。
 */
template <typename... Types>
using void_t = void;

/**
 * @struct enable_if
 * @brief 条件启用模板
 * @tparam Test 布尔测试条件
 * @tparam T 如果Test为true时启用的类型，默认为void
 *
 * 当Test为false时，主模板没有::type成员，触发SFINAE。
 * 当Test为true时，特化版本提供::type成员。
 */
template <bool Test, typename T = void>
struct enable_if {};

/// @cond
template <typename T>
struct enable_if<true, T> {
    using type = T;
};
/// @endcond

/**
 * @typedef enable_if_t
 * @brief enable_if的便捷别名
 */
template <bool Test, typename T = void>
using enable_if_t = typename enable_if<Test, T>::type;


/**
 * @struct conditional
 * @brief 条件选择类型
 * @tparam Test 布尔测试条件
 * @tparam T1 如果Test为true时选择的类型
 * @tparam T2 如果Test为false时选择的类型
 */
template <bool Test, typename T1, typename T2>
struct conditional {
    using type = T1;
};

/// @cond
template <typename T1, typename T2>
struct conditional<false, T1, T2> {
    using type = T2;
};
/// @endcond

/**
 * @typedef conditional_t
 * @brief conditional的便捷别名
 */
template <bool Test, typename T1, typename T2>
using conditional_t = typename conditional<Test, T1, T2>::type;


/**
 * @struct negation
 * @brief 逻辑非包装器
 * @tparam T 具有::value成员的布尔类型
 *
 * 对给定布尔类型特化的值进行逻辑非操作。
 */
template <typename T>
struct negation : bool_constant<!static_cast<bool>(T::value)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var negation_v
 * @brief negation的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool negation_v = negation<T>::value;
#endif


/**
 * @struct is_same
 * @brief 判断两个类型是否相同
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
struct is_same : false_type {};

/// @cond
template <typename T>
struct is_same<T, T> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_same_v
 * @brief is_same的便捷变量模板
 */
template <typename T1, typename T2>
MSTL_INLINE17 constexpr bool is_same_v = is_same<T1, T2>::value;
#endif


/**
 * @struct type_identity
 * @brief 类型标识包装器
 * @tparam T 要包装的类型
 *
 * 将类型T包装为::type成员，用于防止模板参数推导。
 */
template <typename T>
struct type_identity {
    using type = T;
};

/**
 * @typedef type_identity_t
 * @brief type_identity的便捷别名
 */
template <typename T>
using type_identity_t = typename type_identity<T>::type;


/**
 * @struct is_any_of
 * @brief 判断类型是否在类型集合中
 * @tparam T 要查找的类型
 * @tparam Types 类型集合
 */
template <typename T, typename... Types>
struct is_any_of;

#ifdef MSTL_STANDARD_17__
template <typename T, typename... Types>
struct is_any_of : bool_constant<(is_same_v<T, Types> || ...)> {};
#else
template <typename T, typename... Types>
struct is_any_of : false_type {};

/// @cond
template <typename T, typename U>
struct is_any_of<T, U> : is_same<T, U> {};

template <typename T, typename U, typename... Types>
struct is_any_of<T, U, Types...> 
    : conditional<is_same<T, U>::value, true_type, is_any_of<T, Types...>>::type {};
/// @endcond
#endif // MSTL_STANDARD_17__

#ifdef MSTL_STANDARD_14__
/**
 * @var is_any_of_v
 * @brief is_any_of的便捷变量模板
 */
template <typename T, typename... Types>
MSTL_INLINE17 constexpr bool is_any_of_v = is_any_of<T, Types...>::value;
#endif



/// @cond
MSTL_BEGIN_INNER__
// 析取辅助实现
template <bool, typename first, typename...>
struct __disjunction_aux {
    using type = first;
};
template <typename Curr, typename Next, typename... Rest>
struct __disjunction_aux<false, Curr, Next, Rest...> {
    using type = typename __disjunction_aux<static_cast<bool>(Next::value), Next, Rest...>::type;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct disjunction
 * @brief 类型集合的逻辑或操作
 * @tparam Args 具有::value成员的布尔类型集合
 *
 * 计算Args::value的逻辑或，短路求值。
 */
template <typename... Args>
struct disjunction : false_type {};

/// @cond
template <typename First, typename... Rest>
struct disjunction<First, Rest...>
    : _INNER __disjunction_aux<static_cast<bool>(First::value), First, Rest...>::type {
};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var disjunction_v
 * @brief disjunction的便捷变量模板
 */
template <typename... Args>
MSTL_INLINE17 constexpr bool disjunction_v = disjunction<Args...>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
// 合取辅助实现
template <bool, typename First, typename...>
struct __conjunction_aux {
    using type = First;
};
template <typename Curr, typename Next, typename... Rest>
struct __conjunction_aux<true, Curr, Next, Rest...> {
    using type = typename __conjunction_aux<static_cast<bool>(Next::value), Next, Rest...>::type;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct conjunction
 * @brief 类型集合的逻辑与操作
 * @tparam Args 具有::value成员的布尔类型集合
 *
 * 计算Args::value的逻辑与，短路求值。
 */
template <typename... Args>
struct conjunction : true_type {};

/// @cond
template <typename First, typename... Rest>
struct conjunction<First, Rest...>
    : _INNER __conjunction_aux<static_cast<bool>(First::value), First, Rest...>::type {
};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var conjunction_v
 * @brief conjunction的便捷变量模板
 */
template <typename... Args>
MSTL_INLINE17 constexpr bool conjunction_v = conjunction<Args...>::value;
#endif

/** @} */ // TypeTraitsUtilities

/**
 * @defgroup RemoveQualifiers 类型修饰移除
 * @brief 移除类型限定符
 * @{
 */

/**
 * @struct remove_const
 * @brief 移除const限定符
 * @tparam T 输入类型
 */
template <typename T>
struct remove_const {
    using type = T;
};

/// @cond
template <typename T>
struct remove_const<const T> {
    using type = T;
};
/// @endcond

/**
 * @typedef remove_const_t
 * @brief remove_const的便捷别名
 */
template <typename T>
using remove_const_t = typename remove_const<T>::type;


/**
 * @struct remove_volatile
 * @brief 移除volatile限定符
 * @tparam T 输入类型
 */
template <typename T>
struct remove_volatile {
    using type = T;
};

/// @cond
template <typename T>
struct remove_volatile<volatile T> {
    using type = T;
};
/// @endcond

/**
 * @typedef remove_volatile_t
 * @brief remove_volatile的便捷别名
 */
template <typename T>
using remove_volatile_t = typename remove_volatile<T>::type;


/**
 * @struct remove_cv
 * @brief 移除const和volatile限定符
 * @tparam T 输入类型
*
 * 同时提供bind_cv_t元函数，用于将原类型的cv限定符应用到其他类型。
 */
template <typename T>
struct remove_cv {
    using type = T;

    /**
     * @brief 将原类型的cv限定符应用到其他类型
     * @tparam wrapper 要应用限定符的类型
     */
    template <typename wrapper>
    using bind_cv_t = wrapper;
};

/// @cond
template <typename T>
struct remove_cv<const T> {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = const wrapper;
};

template <typename T>
struct remove_cv<volatile T> {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = volatile wrapper;
};

template <typename T>
struct remove_cv<const volatile T> {
    using type = T;

    template <typename wrapper>
    using bind_cv_t = const volatile wrapper;
};
/// @endcond

/**
 * @typedef remove_cv_t
 * @brief remove_cv的便捷别名
 */
template <typename T>
using remove_cv_t = typename remove_cv<T>::type;

/**
 * @typedef copy_cv_t
 * @brief 复制cv限定符
 * @tparam From 源类型，提供cv限定符
 * @tparam To 目标类型，接受cv限定符
 */
template <typename From, typename To>
using copy_cv_t = typename remove_cv<From>::template bind_cv_t<To>;


/**
 * @struct remove_reference
 * @brief 移除引用限定符
 * @tparam T 输入类型
 *
 * 同时提供bind_ref_t元函数，用于将原类型的引用限定符应用到其他类型。
 */
template <typename T>
struct remove_reference {
    using type = T;

    /**
     * @brief 将原类型的引用限定符应用到其他类型
     * @tparam wrapper 要应用引用限定符的类型
     */
    template <typename wrapper>
    using bind_ref_t = wrapper;
};

/// @cond
template <typename T>
struct remove_reference<T&> {
    using type = T;

    template <typename wrapper>
    using bind_ref_t = wrapper&;
};

template <typename T>
struct remove_reference<T&&> {
    using type = T;

    template <typename wrapper>
    using bind_ref_t = wrapper&&;
};
/// @endcond

/**
 * @typedef remove_reference_t
 * @brief remove_reference的便捷别名
 */
template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

/**
 * @typedef copy_ref_t
 * @brief 复制引用限定符
 * @tparam From 源类型，提供引用限定符
 * @tparam To 目标类型，接受引用限定符
 */
template <typename From, typename To>
using copy_ref_t = typename remove_reference<From>::template bind_ref_t<To>;

/**
 * @typedef copy_cvref_t
 * @brief 同时复制cv和引用限定符
 * @tparam From 源类型，提供cv和引用限定符
 * @tparam To 目标类型，接受限定符
 */
template <typename From, typename To>
using copy_cvref_t = copy_ref_t<From, copy_cv_t<From, To>>;


/**
 * @struct remove_cvref
 * @brief 同时移除cv和引用限定符的类型包装
 * @tparam T 输入类型
 */
template <typename T>
struct remove_cvref {
    using type = remove_cv_t<remove_reference_t<T>>;
};

/**
 * @typedef remove_cvref_t
 * @brief remove_cvref的便捷别名
 */
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;


/**
 * @struct remove_extent
 * @brief 移除数组的最外层维度
 * @tparam T 输入类型
 */
template <typename T>
struct remove_extent {
    using type = T;
};

/// @cond
template <typename T, size_t Idx>
struct remove_extent<T[Idx]> {
    using type = T;
};

template <typename T>
struct remove_extent<T[]> {
    using type = T;
};
/// @endcond

/**
 * @typedef remove_extent_t
 * @brief remove_extent的便捷别名
 */
template <typename T>
using remove_extent_t = typename remove_extent<T>::type;


/**
 * @struct remove_all_extents
 * @brief 移除数组的所有维度
 * @tparam T 输入类型
 */
template <typename T>
struct remove_all_extents {
    using type = T;
};

/// @cond
template <typename T, size_t Idx>
struct remove_all_extents<T[Idx]> {
    using type = typename remove_all_extents<T>::type;
};

template <typename T>
struct remove_all_extents<T[]> {
    using type = typename remove_all_extents<T>::type;
};
/// @endcond

/**
 * @typedef remove_all_extents_t
 * @brief remove_all_extents的便捷别名
 */
template <typename T>
using remove_all_extents_t = typename remove_all_extents<T>::type;


/**
 * @struct remove_pointer
 * @brief 移除指针限定符
 * @tparam T 输入类型
 *
 * 同时提供bind_pointer_t元函数，用于将原类型的指针限定符应用到其他类型。
 */
template <typename T>
struct remove_pointer {
    using type = T;

    /**
     * @brief 将原类型的指针限定符应用到其他类型
     * @tparam wrapper 要应用指针限定符的类型
     */
    template <typename wrapper>
    using bind_pointer_t = wrapper;
};

/// @cond
template <typename T>
struct remove_pointer<T*> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = wrapper*;
};

template <typename T>
struct remove_pointer<T* const> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = const wrapper*;
};

template <typename T>
struct remove_pointer<T* volatile> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = volatile wrapper*;
};

template <typename T>
struct remove_pointer<T* const volatile> {
    using type = T;

    template <typename wrapper>
    using bind_pointer_t = volatile const wrapper*;
};
/// @endcond

/**
 * @typedef remove_pointer_t
 * @brief remove_pointer的便捷别名
 */
template <typename T>
using remove_pointer_t = typename remove_pointer<T>::type;

/**
 * @typedef copy_pointer_t
 * @brief 复制指针限定符
 * @tparam From 源类型，提供指针限定符
 * @tparam To 目标类型，接受指针限定符
 */
template <typename From, typename To>
using copy_pointer_t = typename remove_pointer<From>::template bind_pointer_t<To>;


/**
 * @struct remove_function_qualifiers
 * @brief 移除函数类型的限定符
 * @tparam T 函数类型
 */
template <typename T>
struct remove_function_qualifiers {
    using type = T;
};

/// @cond

template <typename Ret, typename... Args>
struct remove_function_qualifiers<Ret(Args...)> {
    using type = Ret(Args...);
};

#define __MSTL_EXPAND_REM_FUNC_QULF(QUF) \
template <typename Ret, typename... Args> \
struct remove_function_qualifiers<Ret(Args...) QUF> { \
    using type = Ret(Args...); \
};

MSTL_MACRO_RANGES_CV(__MSTL_EXPAND_REM_FUNC_QULF)
MSTL_MACRO_RANGES_CV_REF(__MSTL_EXPAND_REM_FUNC_QULF)
MSTL_MACRO_RANGES_CV_REF_NOEXCEPT(__MSTL_EXPAND_REM_FUNC_QULF)
#undef __MSTL_EXPAND_REM_FUNC_QULF

/// @endcond

/**
 * @typedef remove_function_qualifiers_t
 * @brief remove_function_qualifiers的便捷别名
 */
template <typename T>
using remove_function_qualifiers_t = typename remove_function_qualifiers<T>::type;

/** @} */ // RemoveQualifiers

/**
 * @defgroup BaseTypeProperties 类型基本属性查询
 * @brief 查询类型的基本属性
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__
template <typename>
struct __is_void_helper : false_type {};

template <>
struct __is_void_helper<void> : true_type {};
MSTL_END_INNER__
/// @endcond

/**
 * @struct is_void
 * @brief 判断类型是否为void
 * @tparam T 要检查的类型
 *
 * 移除cv限定符后检查是否为void类型。
 */
template <typename T>
struct is_void : _INNER __is_void_helper<remove_cv_t<T>>::type {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_void_v
 * @brief is_void的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_void_v = is_void<T>::value;
#endif


/**
 * @struct package
 * @brief 类型包装器模板
 * @tparam T 要包装的类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T, typename Dummy = void>
struct package {
    using type = T;
};

/**
 * @typedef package_t
 * @brief package的便捷别名
 */
template <typename T>
using package_t = typename package<T>::type;

/**
 * @struct is_packaged
 * @brief 判断类型是否被包装
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_packaged : bool_constant<!is_same<package_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_packaged_v
 * @brief is_packaged的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_packaged_v = is_packaged<T>::value;
#endif

/**
 * @struct unpackage
 * @brief 类型解包器模板
 * @tparam T 要解包的类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T, typename Dummy = void>
struct unpackage {
    using type = T;
};

/**
 * @typedef unpackage_t
 * @brief unpackage的便捷别名
 */
template <typename T>
using unpackage_t = typename unpackage<T>::type;

/**
 * @typedef unpack_remove_cvref_t
 * @brief 同时解包并移除cv和引用限定符
 */
template <typename T>
using unpack_remove_cvref_t = unpackage_t<remove_cvref_t<T>>;

/**
 * @struct is_unpackaged
 * @brief 判断类型是否被解包
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_unpackaged : bool_constant<!is_same<unpackage_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_unpackaged_v
 * @brief is_unpackaged的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_unpackaged_v = is_unpackaged<T>::value;
#endif


/**
 * @struct is_character
 * @brief 判断类型是否为字符类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_character : bool_constant<is_any_of<unpack_remove_cvref_t<T>,
    char, signed char, unsigned char, wchar_t,
#ifdef MSTL_STANDARD_20__
    char8_t,
#endif
    char16_t, char32_t
>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_character_v
 * @brief is_character的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_character_v = is_character<T>::value;
#endif


/**
 * @struct is_standard_character
 * @brief 判断类型是否为标准字符类型
 * @tparam T 要检查的类型
 *
 * 不包括signed char和unsigned char，只包括标准字符类型。
 */
template <typename T>
struct is_standard_character : bool_constant<is_any_of<unpack_remove_cvref_t<T>,
    char, wchar_t,
#ifdef MSTL_STANDARD_20__
    char8_t,
#endif
    char16_t, char32_t
>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_standard_character_v
 * @brief is_standard_character的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_standard_character_v = is_standard_character<T>::value;
#endif


/**
 * @struct is_boolean
 * @brief 判断类型是否为布尔类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_boolean : bool_constant<is_same<unpack_remove_cvref_t<T>, bool>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_boolean_v
 * @brief is_boolean的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_boolean_v = is_boolean<T>::value;
#endif


/**
 * @struct is_standard_integral
 * @brief 判断类型是否为标准整数类型
 * @tparam T 要检查的类型
 *
 * 包括所有标准整数类型，不包括字符类型和布尔类型。
 */
template <typename T>
struct is_standard_integral : bool_constant<is_any_of<unpack_remove_cvref_t<T>,
    short, int, long, long long,
    unsigned short, unsigned int, unsigned long, unsigned long long>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_standard_integral_v
 * @brief is_standard_integral的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_standard_integral_v = is_standard_integral<T>::value;
#endif


/**
 * @struct is_integral
 * @brief 判断类型是否为整数类型
 * @tparam T 要检查的类型
 *
 * 包括标准整数类型、字符类型和布尔类型。
 */
template <typename T>
struct is_integral : bool_constant<disjunction<is_standard_integral<T>, is_character<T>, is_boolean<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_integral_v
 * @brief is_integral的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_integral_v = is_integral<T>::value;
#endif


/**
 * @struct is_floating_point
 * @brief 判断类型是否为浮点数类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_floating_point : bool_constant<is_any_of<unpack_remove_cvref_t<T>, float, double, long double>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_floating_point_v
 * @brief is_floating_point的便捷变量模板
 */
template <typename T> MSTL_INLINE17 constexpr bool is_floating_point_v = is_floating_point<T>::value;
#endif


/**
 * @struct is_arithmetic
 * @brief 判断类型是否为算术类型
 * @tparam T 要检查的类型
 *
 * 包括整数类型和浮点数类型。
 */
template <typename T>
struct is_arithmetic : bool_constant<disjunction<is_integral<T>, is_floating_point<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_arithmetic_v
 * @brief is_arithmetic的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename T, bool = is_integral<T>::value>
struct __check_sign_aux {
    static constexpr bool is_signed = static_cast<remove_cvref_t<T>>(-1) < static_cast<remove_cv_t<T>>(0);
    static constexpr bool is_unsigned = !is_signed;
};

template <typename T>
struct __check_sign_aux<T, false> {
    static constexpr bool is_signed = is_floating_point<T>::value;
    static constexpr bool is_unsigned = false;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct is_signed
 * @brief 判断类型是否为有符号类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_signed : bool_constant<_INNER __check_sign_aux<unpack_remove_cvref_t<T>>::is_signed> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_signed_v
 * @brief is_signed的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_signed_v = is_signed<T>::value;
#endif

/**
 * @struct is_unsigned
 * @brief 判断类型是否为无符号类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_unsigned : bool_constant<_INNER __check_sign_aux<unpack_remove_cvref_t<T>>::is_unsigned> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_unsigned_v
 * @brief is_unsigned的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_unsigned_v = is_unsigned<T>::value;
#endif

/** @} */ // TypeProperties

/**
 * @defgroup AddQualifiers 类型限定符添加
 * @brief 添加类型限定符
 * @{
 */

/**
 * @struct add_const
 * @brief 添加const限定符
 * @tparam T 输入类型
 */
template <typename T>
struct add_const {
    using type = const T;
};

/**
 * @typedef add_const_t
 * @brief add_const的便捷别名
 */
template <typename T>
using add_const_t = typename add_const<T>::type;

/**
 * @brief 将值转换为const引用
 * @tparam T 值类型
 * @param val 要转换的值
 * @return 值的const引用
 * @note 删除const右值重载以防止悬垂引用
 */
template <typename T>
MSTL_NODISCARD constexpr add_const_t<T>& as_const(T& val) noexcept {
    return val;
}

/// @cond
template <typename T>
void as_const(const T&&) = delete;
/// @endcond

/**
 * @struct add_volatile
 * @brief 添加volatile限定符
 * @tparam T 输入类型
 */
template <typename T>
struct add_volatile {
    using type = volatile T;
};

/**
 * @typedef add_volatile_t
 * @brief add_volatile的便捷别名
 */
template <typename T>
using add_volatile_t = typename add_volatile<T>::type;

/**
 * @struct add_cv
 * @brief 同时添加const和volatile限定符
 * @tparam T 输入类型
 */
template <typename T>
struct add_cv {
    using type = const volatile T;
};

/**
 * @typedef add_cv_t
 * @brief add_cv的类型别名
 */
template <typename T>
using add_cv_t = typename add_cv<T>::type;


/**
 * @struct add_reference
 * @brief 添加引用限定符
 * @tparam T 输入类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 提供lvalue和rvalue类型别名。
 */
template <typename T, typename Dummy = void>
struct add_reference {
    using lvalue = T;
    using rvalue = T;
};

/// @cond
template <typename T>
struct add_reference<T, void_t<T&>> {
    using lvalue = T&;
    using rvalue = T&&;
};
/// @endcond

/**
 * @struct add_lvalue_reference
 * @brief 添加左值引用
 * @tparam T 输入类型
 */
template <typename T>
struct add_lvalue_reference {
    using type = typename add_reference<T>::lvalue;
};

/**
 * @typedef add_lvalue_reference_t
 * @brief add_lvalue_reference的便捷别名
 */
template <typename T>
using add_lvalue_reference_t = typename add_reference<T>::lvalue;

/**
 * @struct add_rvalue_reference
 * @brief 添加右值引用
 * @tparam T 输入类型
 */
template <typename T>
struct add_rvalue_reference {
    using type = typename add_reference<T>::rvalue;
};

/**
 * @typedef add_rvalue_reference_t
 * @brief add_rvalue_reference的便捷别名
 */
template <typename T>
using add_rvalue_reference_t = typename add_reference<T>::rvalue;


/**
 * @struct add_pointer
 * @brief 添加指针限定符
 * @tparam T 输入类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T, typename Dummy = void>
struct add_pointer {
    using type = T;
};

/// @cond
template <typename T>
struct add_pointer<T, void_t<remove_reference_t<T>*>> {
    using type = remove_reference_t<T>*;
};
/// @endcond

/**
 * @typedef add_pointer_t
 * @brief add_pointer的便捷别名
 */
template <typename T>
using add_pointer_t = typename add_pointer<T>::type;

/** @} */ // AddQualifiers

/**
 * @defgroup DeclvalTools 非求值辅助工具
 * @brief 用于非求值上下文中的辅助工具
 * @{
 */

/**
 * @brief 获取类型的右值引用，仅用于非求值上下文
 * @tparam T 目标类型
 * @return 类型的右值引用
 * @warning 此函数仅有声明，不应被实际调用
 */
template <typename T>
add_rvalue_reference_t<T> declval() noexcept;

/**
 * @brief 获取类型的副本，仅用于非求值上下文
 * @tparam T 目标类型
 * @return 类型的副本
 * @warning 此函数仅有声明，不应被实际调用
 */
template <typename T>
type_identity_t<T> declcopy(type_identity_t<T>) noexcept;

/**
 * @brief 将类型映射为void，仅用于非求值上下文
 * @tparam T 目标类型
 * @warning 此函数仅有声明，不应被实际调用
 */
template <typename T>
void declvoid(type_identity_t<T>) noexcept;

/** @} */ // DeclvalTools

/**
 * @defgroup ArrayProperties 数组属性
 * @brief 查询数组类型的维度信息
 * @{
 */

/**
 * @struct rank
 * @brief 查询数组的维度数
 * @tparam T 数组类型
 *
 * 对于非数组类型返回0，对于数组类型返回其维度数。
 */
template <typename T>
struct rank : integral_constant<size_t, 0> {};

/// @cond
template <typename T, size_t Idx>
struct rank<T[Idx]> : integral_constant<size_t, rank<T>::value + 1> {};

template <typename T>
struct rank<T[]> : integral_constant<size_t, rank<T>::value + 1> {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var rank_v
 * @brief rank的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr size_t rank_v = rank<T>::value;
#endif


/**
 * @struct extent
 * @brief 查询数组指定维度的大小
 * @tparam T 数组类型
 * @tparam Idx 维度索引，从0开始
 *
 * 对于非数组类型或越界的维度返回0。
 */
template <typename T, uint32_t Idx = 0>
struct extent : integral_constant<size_t, 0> {};

/// @cond
template <typename T, size_t N>
struct extent<T[N], 0> : integral_constant<size_t, N> {};

template <typename T, uint32_t Idx, size_t N>
struct extent<T[N], Idx> : extent<T, Idx - 1> {};

template <typename T, uint32_t Idx>
struct extent<T[], Idx> : extent<T, Idx - 1> {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var extent_v
 * @brief extent的便捷变量模板
 */
template <typename T, uint32_t Idx = 0>
MSTL_INLINE17 constexpr size_t extent_v = extent<T, Idx>::value;
#endif

/** @} */ // ArrayProperties

/**
 * @defgroup TemplateTraitsUtilities 模板操作工具
 * @brief 提取和操作指针类型元信息的辅助工具
 * @{
 */

/**
 * @struct get_first_temp_para
 * @brief 提取模板的第一个类型参数
 * @tparam T 模板类型
 */
template <typename T>
struct get_first_temp_para;

/// @cond
template <template <typename, typename...> class T, typename First, typename... Rest>
struct get_first_temp_para<T<First, Rest...>> {
    using type = First;
};
/// @endcond

/**
 * @typedef get_first_temp_para_t
 * @brief get_first_temp_para的便捷别名
 */
template <typename Tmp>
using get_first_temp_para_t = typename get_first_temp_para<Tmp>::type;


/**
 * @struct get_first_para
 * @brief 提取参数列表的第一个类型参数
 * @tparam Types 参数列表
 */
template <typename... Types>
struct get_first_para;

/// @cond
template <typename First, typename... Rest>
struct get_first_para<First, Rest...> {
    using type = First;
};
/// @endcond

/**
 * @typedef get_first_para_t
 * @brief get_first_para的便捷别名
 */
template <typename... Types>
using get_first_para_t = typename get_first_para<Types...>::type;


/**
 * @struct get_ptr_difference
 * @brief 获取指针的差值类型
 * @tparam T 指针类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 如果指针类型定义了difference_type，则使用该类型，否则使用默认的ptrdiff_t。
 */
template <typename T, typename Dummy = void>
struct get_ptr_difference {
    using type = ptrdiff_t;
};

/// @cond
template <typename T>
struct get_ptr_difference<T, enable_if_t<
    is_same<typename T::difference_type, typename T::difference_type>::value>> {
    using type = typename T::difference_type;
};
/// @endcond

/**
 * @typedef get_ptr_difference_t
 * @brief get_ptr_difference的便捷别名
 */
template <typename T>
using get_ptr_difference_t = typename get_ptr_difference<T>::type;


/**
 * @struct replace_first_para
 * @brief 替换模板的第一个类型参数
 * @tparam NewFirst 新的第一个参数
 * @tparam T 原始模板类型
 */
template <typename NewFirst, typename T>
struct replace_first_para;

/// @cond
template <typename NewFirst, template <typename, typename...> class T, typename First, typename... Rest>
struct replace_first_para<NewFirst, T<First, Rest...>> {
    using type = T<NewFirst, Rest...>;
};
/// @endcond

/**
 * @typedef replace_first_para_t
 * @brief replace_first_para的便捷别名
 */
template <typename T, typename U>
using replace_first_para_t = typename replace_first_para<T, U>::type;


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
    using type = replace_first_para_t<U, T>;
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

/** @} */ // TemplateTraitsUtilities

/**
 * @defgroup BaseTypeQualifierCheck 类型修饰基本检查
 * @brief 检查类型的基本修饰信息
 * @{
 */

/**
 * @struct is_bounded_array
 * @brief 判断类型是否为有界数组
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_bounded_array : false_type {};

/// @cond
template <typename T, size_t Idx>
struct is_bounded_array<T[Idx]> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_bounded_array_v
 * @brief is_bounded_array的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_bounded_array_v = is_bounded_array<T>::value;
#endif


/**
 * @struct is_unbounded_array
 * @brief 判断类型是否为无界数组
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_unbounded_array : false_type {};

/// @cond
template <typename T>
struct is_unbounded_array<T[]> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_unbounded_array_v
 * @brief is_unbounded_array的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_unbounded_array_v = is_unbounded_array<T>::value;
#endif

/**
 * @struct is_array
 * @brief 判断类型是否为数组类型
 * @tparam T 要检查的类型
 *
 * 包括有界数组和无界数组。
 */
template <typename T>
struct is_array : bool_constant<is_unbounded_array<T>::value || is_bounded_array<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_array_v
 * @brief is_array的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_array_v = is_array<T>::value;
#endif


/**
 * @struct is_lvalue_reference
 * @brief 判断类型是否为左值引用
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_lvalue_reference : false_type {};

/// @cond
template <typename T>
struct is_lvalue_reference<T&> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_lvalue_reference_v
 * @brief is_lvalue_reference的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;
#endif

/**
 * @struct is_rvalue_reference
 * @brief 判断类型是否为右值引用
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_rvalue_reference : false_type {};

/// @cond
template <typename T>
struct is_rvalue_reference<T&&> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_rvalue_reference_v
 * @brief is_rvalue_reference的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;
#endif


/**
 * @struct is_reference
 * @brief 判断类型是否为引用类型
 * @tparam T 要检查的类型
 *
 * 包括左值引用和右值引用。
 */
template <typename T>
struct is_reference : bool_constant<is_lvalue_reference<T>::value || is_rvalue_reference<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_reference_v
 * @brief is_reference的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_reference_v = is_reference<T>::value;
#endif


/**
 * @struct is_null_pointer
 * @brief 判断类型是否为nullptr_t
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_null_pointer : bool_constant<is_same<remove_cvref_t<T>, nullptr_t>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_null_pointer_v
 * @brief is_null_pointer的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_null_pointer_v = is_null_pointer<T>::value;
#endif


/**
 * @struct is_pointer
 * @brief 判断类型是否为指针类型
 * @tparam T 要检查的类型
 *
 * 包括各种cv限定的指针类型。
 */
template <typename T>
struct is_pointer : false_type {};

/// @cond
template <typename T>
struct is_pointer<T*> : true_type {};

template <typename T>
struct is_pointer<T* const> : true_type {};

template <typename T>
struct is_pointer<T* volatile> : true_type {};

template <typename T>
struct is_pointer<T* const volatile> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_pointer_v
 * @brief is_pointer的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_pointer_v = is_pointer<T>::value;
#endif


/**
 * @struct is_enum
 * @brief 判断类型是否为枚举类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_enum : bool_constant<__is_enum(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_enum_v
 * @brief is_enum的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_enum_v = is_enum<T>::value;
#endif


/**
 * @struct is_integral_like
 * @brief 判断类型是否为类整数类型
 * @tparam T 要检查的类型
 *
 * 包括整数类型和枚举类型。
 */
template <typename T>
struct is_integral_like : bool_constant<disjunction<is_integral<T>, is_enum<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_integral_like_v
 * @brief is_integral_like的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_integral_like_v = is_integral_like<T>::value;
#endif


/**
 * @struct is_union
 * @brief 判断类型是否为联合类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_union : bool_constant<__is_union(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_union_v
 * @brief is_union的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_union_v = is_union<T>::value;
#endif


/**
 * @struct is_class
 * @brief 判断类型是否为类类型
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_class : bool_constant<__is_class(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_class_v
 * @brief is_class的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_class_v = is_class<T>::value;
#endif


/**
 * @struct is_fundamental
 * @brief 判断类型是否为基本类型
 * @tparam T 要检查的类型
 *
 * 包括算术类型、void和nullptr_t。
 */
template <typename T>
struct is_fundamental : bool_constant<
    disjunction<is_arithmetic<T>, is_void<T>, is_null_pointer<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_fundamental_v
 * @brief is_fundamental的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_fundamental_v = is_fundamental<T>::value;
#endif


/**
 * @struct is_compound
 * @brief 判断类型是否为复合类型
 * @tparam T 要检查的类型
 *
 * 非基本类型的类型都是复合类型。
 */
template <typename T>
struct is_compound : bool_constant<!is_fundamental<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_compound_v
 * @brief is_compound的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_compound_v = is_compound<T>::value;
#endif


/**
 * @struct is_const
 * @brief 判断类型是否被const限定
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_const : false_type {};

/// @cond
template <typename T>
struct is_const<const T> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_const_v
 * @brief is_const的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_const_v = is_const<T>::value;
#endif


/**
 * @struct is_volatile
 * @brief 判断类型是否被volatile限定
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_volatile : false_type {};

/// @cond
template <typename T>
struct is_volatile<volatile T> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_volatile_v
 * @brief is_volatile的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_volatile_v = is_volatile<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
// 禁用MSVC警告4180：从const限定的函数类型中移除const
#pragma warning(push)
#pragma warning(disable: 4180)
#endif

/**
 * @struct is_function
 * @brief 判断类型是否为函数类型
 * @tparam T 要检查的类型
 *
 * 函数类型不能被const限定，也不能是引用类型。
 * 通过检查const限定的函数类型是否仍为const来判断是否为函数类型。
 */
template <typename T>
struct is_function : bool_constant<
    !is_const<const remove_function_qualifiers_t<T>>::value
    && !is_reference<remove_function_qualifiers_t<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_function_v
 * @brief is_function的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_function_v = is_function<T>::value;
#endif

#ifdef MSTL_COMPILER_MSVC__
#pragma warning(pop)
#endif


/**
 * @struct is_allocable
 * @brief 判断类型是否可以进行内存分配
 * @tparam T 要检查的类型
 *
 * 类型必须同时满足以下条件才能被分配：
 * 1. 不是void类型
 * 2. 不是引用类型
 * 3. 不是函数类型
 * 4. 不是const限定的类型
 * 5. 已被实现
 */
template <typename T>
struct is_allocable : bool_constant<
    !(is_void<T>::value || is_reference<T>::value || is_function<T>::value || is_const<T>::value)
    && (sizeof(T) > 0)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_allocable_v
 * @brief is_allocable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_allocable_v = is_allocable<T>::value;
#endif


/**
 * @struct is_object
 * @brief 判断类型是否为对象类型
 * @tparam T 要检查的类型
 *
 * 对象类型包括除了函数、引用和void之外的所有类型。
 * 函数类型和引用类型不能被const限定，因此利用这一特性进行判断。
 */
template <typename T>
struct is_object : bool_constant<is_const<const T>::value && !is_void<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_object_v
 * @brief is_object的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_object_v = is_object<T>::value;
#endif


/**
 * @struct is_cstring
 * @brief 判断类型是否为C风格字符串类型
 * @tparam T 要检查的类型
 *
 * C风格字符串可以是：
 * 1. 指向字符类型的指针
 * 2. 字符类型的数组
 */
template <typename T>
struct is_cstring : bool_constant<
    (is_pointer<remove_cvref_t<T>>::value &&
        is_character<remove_pointer_t<remove_cvref_t<T>>>::value) ||
    (is_bounded_array<remove_cvref_t<T>>::value &&
        is_character<remove_all_extents_t<remove_cvref_t<T>>>::value)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_cstring_v
 * @brief is_cstring的便捷变量模板
 */
template <typename T>
constexpr bool is_cstring_v = is_cstring<T>::value;
#endif


/**
 * @struct is_member_function_pointer
 * @brief 判断类型是否为成员函数指针
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_member_function_pointer;

/// @cond
#ifdef MSTL_COMPILER_CLANG__
template <typename T>
struct is_member_function_pointer : bool_constant<__is_member_function_pointer(T)> {};
#else
MSTL_BEGIN_INNER__
template <typename>
struct __is_member_function_pointer_aux : false_type {};
template <typename T, typename C>
struct __is_member_function_pointer_aux<T C::*> : is_function<T> {};
MSTL_END_INNER__

template <typename T>
struct is_member_function_pointer : _INNER __is_member_function_pointer_aux<remove_cv_t<T>> {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_member_function_pointer_v
 * @brief is_member_function_pointer的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_member_function_pointer_v = is_member_function_pointer<T>::value;
#endif


/**
 * @struct is_member_object_pointer
 * @brief 判断类型是否为成员对象指针
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_member_object_pointer;

/// @cond
#ifdef MSTL_COMPILER_CLANG__
template <typename T>
struct is_member_object_pointer : bool_constant<__is_member_object_pointer(T)> {};
#else
template <typename T>
struct is_member_object_pointer : false_type {};
template <typename T, typename C>
struct is_member_object_pointer<T C::*> : bool_constant<!is_function<T>::value> {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_member_object_pointer_v
 * @brief is_member_object_pointer的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_member_object_pointer_v = is_member_object_pointer<remove_cv_t<T>>::value;
#endif


/**
 * @struct is_member_pointer
 * @brief 判断类型是否为成员指针
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_member_pointer;

/// @cond
#ifdef MSTL_COMPILER_CLANG__
template <typename T>
struct is_member_pointer : bool_constant<__is_member_pointer(T)> {};
#else
template <typename T>
struct is_member_pointer : bool_constant<is_member_object_pointer<T>::value || is_member_function_pointer<T>::value> {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_member_pointer_v
 * @brief is_member_pointer的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_member_pointer_v = is_member_pointer<T>::value;
#endif


/**
 * @struct is_scalar
 * @brief 判断类型是否为标量类型
 * @tparam T 要检查的类型
 *
 * 标量类型包括：
 * 1. 算术类型
 * 2. 枚举类型
 * 3. 指针类型
 * 4. 成员指针类型
 * 5. nullptr_t
 */
template <typename T>
struct is_scalar : bool_constant<disjunction<
    is_arithmetic<T>, is_enum<T>, is_pointer<T>, is_member_pointer<T>, is_null_pointer<T>
>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_scalar_v
 * @brief is_scalar的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_scalar_v = is_scalar<T>::value;
#endif


/**
 * @struct is_empty
 * @brief 判断类型是否为空类型
 * @tparam T 要检查的类型
 *
 * 空类型具有以下特征：
 * 1. 没有非静态数据成员
 * 2. 没有虚函数和虚基类
 * 3. 如果有基类，基类也必须是空类型
 */
template <typename T>
struct is_empty : bool_constant<__is_empty(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_empty_v
 * @brief is_empty的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_empty_v = is_empty<T>::value;
#endif


/**
 * @struct is_polymorphic
 * @brief 判断类型是否为多态类型
 * @tparam T 要检查的类型
 *
 * 多态类型包含虚函数。
 */
template <typename T>
struct is_polymorphic : bool_constant<__is_polymorphic(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_polymorphic_v
 * @brief is_polymorphic的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_polymorphic_v = is_polymorphic<T>::value;
#endif


/**
 * @struct is_abstract
 * @brief 判断类型是否为抽象类型
 * @tparam T 要检查的类型
 *
 * 抽象类型包含纯虚函数，不能直接实例化。
 */
template <typename T>
struct is_abstract : bool_constant<__is_abstract(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_abstract_v
 * @brief is_abstract的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_abstract_v = is_abstract<T>::value;
#endif


/**
 * @struct is_final
 * @brief 判断类型是否被final限定
 * @tparam T 要检查的类型
 *
 * final类不能被继承，final虚函数不能被重写。
 */
template <typename T>
struct is_final : bool_constant<__is_final(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_final_v
 * @brief is_final的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_final_v = is_final<T>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename T, bool = is_enum<T>::value>
struct __underlying_type_aux {
    using type = __underlying_type(T);
};
template <typename T>
struct __underlying_type_aux<T, false> {};
MSTL_END_INNER__
/// @endcond

/**
 * @struct underlying_type
 * @brief 获取枚举类型的底层整数类型
 * @tparam T 枚举类型
 *
 * 对于非枚举类型，不提供::type成员。
 */
template <typename T>
struct underlying_type : _INNER __underlying_type_aux<T> {};

/**
 * @typedef underlying_type_t
 * @brief underlying_type的便捷别名
 */
template <typename T>
using underlying_type_t = typename underlying_type<T>::type;


/**
 * @struct is_standard_layout
 * @brief 判断类型是否符合标准布局
 * @tparam T 要检查的类型
 *
 * 标准布局类型具有以下特征：
 * 1. 所有非静态成员具有一致的访问控制
 * 2. 没有虚函数和虚基类
 * 3. 没有基类，或者只有一个标准布局的基类
 * 4. 非静态数据成员不能同时出现在基类和派生类中
 */
template <typename T>
struct is_standard_layout : bool_constant<__is_standard_layout(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_standard_layout_v
 * @brief is_standard_layout的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_standard_layout_v = is_standard_layout<T>::value;
#endif


/**
 * @struct is_pod
 * @brief 判断类型是否为POD类型
 * @tparam T 要检查的类型
 *
 * POD类型同时满足标准布局和平凡(trivial)条件。
 */
template <typename T>
struct is_pod : bool_constant<__is_pod(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_pod_v
 * @brief is_pod的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_pod_v = is_pod<T>::value;
#endif


/**
 * @struct has_unique_object_representations
 * @brief 判断类型是否具有唯一的对象表示
 * @tparam T 要检查的类型
 *
 * 如果类型具有唯一的对象表示：
 * 1. 每个不同的值都有唯一的二进制表示
 * 2. 需要考虑填充字节和实现依赖
 * 3. 可以通过内存比较轻松判断相等性
 *
 * 标准布局类型和平凡类型通常具有唯一的对象表示。
 */
template <typename T>
struct has_unique_object_representations : bool_constant<__has_unique_object_representations(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var has_unique_object_representations_v
 * @brief has_unique_object_representations的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool has_unique_object_representations_v = has_unique_object_representations<T>::value;
#endif


/**
 * @struct is_aggregate
 * @brief 判断类型是否为聚合类型
 * @tparam T 要检查的类型
 *
 * 聚合类型具有以下特征：
 * 1. 数组类型是聚合类型
 * 2. 满足以下条件的类类型：
 *    - 没有用户定义的构造函数
 *    - 所有非静态数据成员都是public
 *    - 没有虚函数和虚基类
 */
template <typename T>
struct is_aggregate;

/// @cond
#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_aggregate : bool_constant<is_array<T>::value || __is_aggregate(T)> {};
#else
template <typename T>
struct is_aggregate : bool_constant<__is_aggregate(remove_cv_t<T>)> {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_aggregate_v
 * @brief is_aggregate的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_aggregate_v = is_aggregate<T>::value;
#endif


#ifdef MSTL_COMPILER_MSVC__
/**
 * @struct is_layout_compatible
 * @brief 判断两个类型是否布局兼容
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 布局兼容类型在内存中具有相同的布局：
 * 1. 成员变量具有相同的类型、数量和排列顺序
 * 2. 成员变量具有相同的对齐方式
 */
template <typename T1, typename T2>
struct is_layout_compatible : bool_constant<__is_layout_compatible(T1, T2)> {};

/**
 * @var is_layout_compatible_v
 * @brief is_layout_compatible的便捷变量模板
 */
#ifdef MSTL_STANDARD_14__
template <typename T1, typename T2>
MSTL_INLINE17 constexpr bool is_layout_compatible_v = is_layout_compatible<T1, T2>::value;
#endif


/**
 * @struct is_pointer_interconvertible_base_of
 * @brief 判断Base是否是Derived的指针可互转换基类
 * @tparam Base 基类类型
 * @tparam Derived 派生类类型
 *
 * 指针可互转换基类意味着可以将派生类指针安全地转换为基类指针。
 */
template <typename Base, typename Derived>
struct is_pointer_interconvertible_base_of : bool_constant<
    __is_pointer_interconvertible_base_of(Base, Derived)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_pointer_interconvertible_base_of_v
 * @brief is_pointer_interconvertible_base_of的便捷变量模板
 */
template <typename Base, typename Derived>
MSTL_INLINE17 constexpr bool is_pointer_interconvertible_base_of_v = is_pointer_interconvertible_base_of<Base, Derived>::value;
#endif
#endif


/**
 * @struct is_base_of
 * @brief 判断Base是否是Derived的基类
 * @tparam Base 基类类型
 * @tparam Derived 派生类类型
 */
template <typename Base, typename Derived>
struct is_base_of : bool_constant<__is_base_of(Base, Derived)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_base_of_v
 * @brief is_base_of的便捷变量模板
 */
template <typename Base, typename Derived>
MSTL_INLINE17 constexpr bool is_base_of_v = is_base_of<Base, Derived>::value;
#endif


#ifdef MSTL_STANDARD_20__
/**
 * @brief 判断成员指针是否指向类对象的起始位置
 * @tparam T 类类型
 * @tparam Mem 成员类型
 * @param mp 成员指针
 * @return 如果成员指针指向类对象的起始位置，返回true
 *
 * 用于检查成员指针是否与类对象指针可互相转换。
 */
template <typename T, typename Mem>
constexpr bool is_pointer_interconvertible_with_class(Mem T::*mp) noexcept {
    return __builtin_is_pointer_interconvertible_with_class(mp);
}

/**
 * @brief 判断两个成员指针是否指向对应位置的成员
 * @tparam S1 第一个类类型
 * @tparam S2 第二个类类型
 * @tparam M1 第一个成员类型
 * @tparam M2 第二个成员类型
 * @param m1 第一个成员指针
 * @param m2 第二个成员指针
 * @return 如果成员指针指向对应位置的成员，返回true
 *
 * 用于检查两个不同类中的成员指针是否指向布局中的对应位置。
 */
template <typename S1, typename S2, typename M1, typename M2>
constexpr bool is_corresponding_member(M1 S1::* m1, M2 S2::* m2) noexcept {
    return __builtin_is_corresponding_member(m1, m2);
}
#endif

/** @} */ // BaseTypeQualifierCheck

/**
 * @defgroup TypeSpecialMemberFunctionChecks 类型特殊成员函数信息检查
 * @brief 检查类型的构造/析构信息
 * @{
 */

/**
 * @struct is_trivial
 * @brief 判断类型是否为平凡类型
 * @tparam T 要检查的类型
 *
 * 平凡类型具有以下特征：
 * 1. 构造函数、拷贝构造函数、移动构造函数、拷贝赋值运算符、
 *    移动赋值运算符和析构函数由编译器自动生成(=default)
 * 2. 没有虚函数和虚基类
 */
template <typename T>
struct is_trivial : bool_constant<__is_trivial(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivial_v
 * @brief is_trivial的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivial_v = is_trivial<T>::value;
#endif


/**
 * @struct is_trivially_copyable
 * @brief 判断类型是否为平凡可复制类型
 * @tparam T 要检查的类型
 *
 * 平凡可复制类型可以通过逐字节复制安全复制。
 */
template <typename T>
struct is_trivially_copyable : bool_constant<__is_trivially_copyable(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_copyable_v
 * @brief is_trivially_copyable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
#endif


/**
 * @struct has_virtual_destructor
 * @brief 判断类型是否具有虚析构函数
 * @tparam T 要检查的类型
 */
template <typename T>
struct has_virtual_destructor : bool_constant<__has_virtual_destructor(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var has_virtual_destructor_v
 * @brief has_virtual_destructor的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool has_virtual_destructor_v = has_virtual_destructor<T>::value;
#endif


/**
 * @struct is_constructible
 * @brief 判断类型是否可以使用指定参数构造
 * @tparam T 要构造的类型
 * @tparam Args 构造参数类型
 */
template <typename T, typename... Args>
struct is_constructible : bool_constant<__is_constructible(T, Args...)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_constructible_v
 * @brief is_constructible的便捷变量模板
 */
template <typename T, typename... Args>
MSTL_INLINE17 constexpr bool is_constructible_v = is_constructible<T, Args...>::value;
#endif


/**
 * @struct is_copy_constructible
 * @brief 判断类型是否可复制构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_copy_constructible : bool_constant<
 is_constructible<T, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_copy_constructible_v
 * @brief is_copy_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_copy_constructible_v = is_copy_constructible<T>::value;
#endif


/**
 * @struct is_default_constructible
 * @brief 判断类型是否可默认构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_default_constructible : bool_constant<is_constructible<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_default_constructible_v
 * @brief is_default_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_default_constructible_v = is_default_constructible<T>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename T>
void __implicitly_default_construct_aux(const T&) noexcept;
MSTL_END_INNER__
/// @endcond

/**
 * @struct is_implicitly_default_constructible
 * @brief 判断类型是否可隐式默认构造
 * @tparam T 要检查的类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 检查是否可以用空初始化列表隐式构造类型。
 */
template <typename T, typename Dummy = void>
struct is_implicitly_default_constructible : false_type {};

/// @cond
template <typename T>
struct is_implicitly_default_constructible
    <T, void_t<decltype(_INNER __implicitly_default_construct_aux<T>({}))>> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_implicitly_default_constructible_v
 * @brief is_implicitly_default_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_implicitly_default_constructible_v = is_implicitly_default_constructible<T>::value;
#endif


/**
 * @struct is_move_constructible
 * @brief 判断类型是否可移动构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_move_constructible : bool_constant<is_constructible<T, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_move_constructible_v
 * @brief is_move_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_move_constructible_v = is_move_constructible<T>::value;
#endif


/**
 * @struct is_assignable
 * @brief 判断类型是否可以使用指定类型的值进行赋值
 * @tparam To 目标类型，接受赋值的类型
 * @tparam From 源类型，提供赋值的类型
 */
template <typename To, typename From>
struct is_assignable : bool_constant<__is_assignable(To, From)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_assignable_v
 * @brief is_assignable的便捷变量模板
 */
template <typename To, typename From>
MSTL_INLINE17 constexpr bool is_assignable_v = is_assignable<To, From>::value;
#endif


/**
 * @struct is_copy_assignable
 * @brief 判断类型是否可复制赋值
 * @tparam T 要检查的类型
 *
 * 检查是否可以用const左值引用进行赋值。
 */
template <typename T>
struct is_copy_assignable
    : bool_constant<is_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_copy_assignable_v
 * @brief is_copy_assignable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_copy_assignable_v = is_copy_assignable<T>::value;
#endif


/**
 * @struct is_move_assignable
 * @brief 判断类型是否可移动赋值
 * @tparam T 要检查的类型
 *
 * 检查是否可以用右值进行赋值。
 */
template <typename T>
struct is_move_assignable : bool_constant<is_assignable<add_lvalue_reference_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_move_assignable_v
 * @brief is_move_assignable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_move_assignable_v = is_move_assignable<T>::value;
#endif


/**
 * @struct is_destructible
 * @brief 判断类型是否可析构
 * @tparam T 要检查的类型
 *
 * 检查类型是否具有可访问的析构函数。
 * 特殊情况：
 * 1. void、无界数组和函数类型不可析构
 * 2. 引用和标量类型总是可析构
 */
template <typename T>
struct is_destructible;

/// @cond
#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_destructible : bool_constant<__is_destructible(T)> {};
#else
MSTL_BEGIN_INNER__
template <typename T>
struct __destructible_aux {
private:
    template <typename T1, typename = decltype(declval<T1&>().~T1())>
    static true_type __test(int);
    template <typename>
    static false_type __test(...);

public:
    using type =  decltype(__test<T>(0));
};

template <typename T,
    bool = disjunction<is_void<T>, is_unbounded_array<T>, is_function<T>>::value,
    bool = disjunction<is_reference<T>, is_scalar<T>>::value>
struct __is_destructible_dispatch;

template <typename T>
struct __is_destructible_dispatch<T, false, false>
    : __destructible_aux<remove_all_extents_t<T>>::type {};

template <typename T>
struct __is_destructible_dispatch<T, true, false> : false_type {};

template <typename T>
struct __is_destructible_dispatch<T, false, true> : true_type {};

MSTL_END_INNER__

template <typename T>
struct is_destructible : _INNER __is_destructible_dispatch<T>::type {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_destructible_v
 * @brief is_destructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_destructible_v = is_destructible<T>::value;
#endif


/**
 * @struct is_trivially_constructible
 * @brief 判断类型是否可以使用指定参数平凡构造
 * @tparam T 要构造的类型
 * @tparam Args 构造参数类型
 *
 * 平凡构造意味着构造操作由编译器自动生成。
 */
template <typename T, typename... Args>
struct is_trivially_constructible : bool_constant<__is_trivially_constructible(T, Args...)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_constructible_v
 * @brief is_trivially_constructible的便捷变量模板
 */
template <typename T, typename... Args>
MSTL_INLINE17 constexpr bool is_trivially_constructible_v = is_trivially_constructible<T, Args...>::value;
#endif


/**
 * @struct is_trivially_copy_constructible
 * @brief 判断类型是否可平凡复制构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_trivially_copy_constructible : bool_constant<
    is_trivially_constructible<T, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_copy_constructible_v
 * @brief is_trivially_copy_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_copy_constructible_v = is_trivially_copy_constructible<T>::value;
#endif


/**
 * @struct is_trivially_default_constructible
 * @brief 判断类型是否可平凡默认构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_trivially_default_constructible : bool_constant<is_trivially_constructible<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_default_constructible_v
 * @brief is_trivially_default_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_default_constructible_v = is_trivially_default_constructible<T>::value;
#endif


/**
 * @struct is_trivially_move_constructible
 * @brief 判断类型是否可平凡移动构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_trivially_move_constructible : bool_constant<is_trivially_constructible<T, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_move_constructible_v
 * @brief is_trivially_move_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_move_constructible_v = is_trivially_move_constructible<T>::value;
#endif


/**
 * @struct is_trivially_assignable
 * @brief 判断类型是否可以使用指定类型的值进行平凡赋值
 * @tparam To 目标类型
 * @tparam From 源类型
 */
template <typename To, typename From>
struct is_trivially_assignable : bool_constant<__is_trivially_assignable(To, From)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_assignable_v
 * @brief is_trivially_assignable的便捷变量模板
 */
template <typename To, typename From>
MSTL_INLINE17 constexpr bool is_trivially_assignable_v = is_trivially_assignable<To, From>::value;
#endif


/**
 * @struct is_trivially_copy_assignable
 * @brief 判断类型是否可平凡复制赋值
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_trivially_copy_assignable : bool_constant<
    is_trivially_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_copy_assignable_v
 * @brief is_trivially_copy_assignable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_copy_assignable_v = is_trivially_copy_assignable<T>::value;
#endif


/**
 * @struct is_trivially_move_assignable
 * @brief 判断类型是否可平凡移动赋值
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_trivially_move_assignable : bool_constant<
    is_trivially_assignable<add_lvalue_reference_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_move_assignable_v
 * @brief is_trivially_move_assignable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_move_assignable_v = is_trivially_move_assignable<T>::value;
#endif


/**
 * @struct is_trivially_destructible
 * @brief 判断类型是否可平凡析构
 * @tparam T 要检查的类型
 *
 * 平凡析构意味着析构函数可以被编译器自动生成。
 */
template <typename T>
struct is_trivially_destructible :
#if defined(MSTL_COMPILER_MSVC__) || defined(MSTL_COMPILER_CLANG__)
    bool_constant<__is_trivially_destructible(T)> {};
#else
    conjunction<is_destructible<T>, bool_constant<__has_trivial_destructor(T)>> {};
#endif

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_destructible_v
 * @brief is_trivially_destructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
#endif


/**
 * @struct is_nothrow_constructible
 * @brief 判断类型是否可以使用指定参数无异常构造
 * @tparam T 要构造的类型
 * @tparam Args 构造参数类型
 */
template <typename T, typename... Args>
struct is_nothrow_constructible;

/**
 * @struct is_nothrow_default_constructible
 * @brief 判断类型是否可无异常默认构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_default_constructible;

/// @cond
#ifdef MSTL_COMPILER_MSVC__
template <typename T, typename... Args>
struct is_nothrow_constructible : bool_constant<__is_nothrow_constructible(T, Args...)> {};
#else
MSTL_BEGIN_INNER__
template <typename T, bool = is_array<T>::value>
struct __is_nothrow_default_constructible_dispatch;

template <typename T>
struct __is_nothrow_default_constructible_dispatch<T, true> : conjunction<
    is_bounded_array<T>, bool_constant<noexcept(remove_all_extents_t<T>())>> {};

template <typename T>
struct __is_nothrow_default_constructible_dispatch<T, false>
    : bool_constant<noexcept(T())> {};

MSTL_END_INNER__

template <typename T>
struct is_nothrow_default_constructible : conjunction<
    is_default_constructible<T>, _INNER __is_nothrow_default_constructible_dispatch<T>> {};

MSTL_BEGIN_INNER__

template <typename T, typename... Args>
struct __is_nothrow_constructible_dispatch
    : bool_constant<noexcept(T(_MSTL declval<Args>()...))> {};

template <typename T>
struct __is_nothrow_constructible_dispatch<T>
    : is_nothrow_default_constructible<T> {};

MSTL_END_INNER__

template <typename T, typename... Args>
struct is_nothrow_constructible : conjunction<
    is_constructible<T, Args...>, _INNER __is_nothrow_constructible_dispatch<T, Args...>> {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_constructible_v
 * @brief is_nothrow_constructible的便捷变量模板
 */
template <typename T, typename... Args>
MSTL_INLINE17 constexpr bool is_nothrow_constructible_v = is_nothrow_constructible<T, Args...>::value;
#endif


/**
 * @struct is_nothrow_copy_constructible
 * @brief 判断类型是否可无异常复制构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_copy_constructible : bool_constant<
    is_nothrow_constructible<T, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_copy_constructible_v
 * @brief is_nothrow_copy_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_copy_constructible_v = is_nothrow_copy_constructible<T>::value;
#endif


/// @cond
#ifdef MSTL_COMPILER_MSVC__
template <typename T>
struct is_nothrow_default_constructible : bool_constant<is_nothrow_constructible_v<T>> {};
#endif
/// @endcond


#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_default_constructible_v
 * @brief is_nothrow_default_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_default_constructible_v = is_nothrow_default_constructible<T>::value;
#endif


/**
 * @struct is_nothrow_move_constructible
 * @brief 判断类型是否可无异常移动构造
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_move_constructible : bool_constant<is_nothrow_constructible<T, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_move_constructible_v
 * @brief is_nothrow_move_constructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_move_constructible_v = is_nothrow_move_constructible<T>::value;
#endif


/**
 * @struct is_nothrow_assignable
 * @brief 判断类型是否可以使用指定类型的值进行无异常赋值
 * @tparam To 目标类型
 * @tparam From 源类型
 */
template <typename To, typename From>
struct is_nothrow_assignable;

/// @cond
template <typename To, typename From>
struct is_nothrow_assignable :
#ifdef MSTL_COMPILER_MSVC__
    bool_constant<__is_nothrow_assignable(To, From)> {};
#else
    conjunction<is_assignable<To, From>, bool_constant<
        noexcept(_MSTL declval<To>() = _MSTL declval<From>())>> {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_assignable_v
 * @brief is_nothrow_assignable的便捷变量模板
 */
template <typename To, typename From>
MSTL_INLINE17 constexpr bool is_nothrow_assignable_v = is_nothrow_assignable<To, From>::value;
#endif


/**
 * @struct is_nothrow_copy_assignable
 * @brief 判断类型是否可无异常复制赋值
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_copy_assignable : bool_constant<
    is_nothrow_assignable<add_lvalue_reference_t<T>, add_lvalue_reference_t<const T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_copy_assignable_v
 * @brief is_nothrow_copy_assignable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_copy_assignable_v = is_nothrow_copy_assignable<T>::value;
#endif


/**
 * @struct is_nothrow_move_assignable
 * @brief 判断类型是否可无异常移动赋值
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_move_assignable : bool_constant<
    is_nothrow_assignable<add_lvalue_reference_t<T>, T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_move_assignable_v
 * @brief is_nothrow_move_assignable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_move_assignable_v = is_nothrow_move_assignable<T>::value;
#endif


#ifndef MSTL_COMPILER_MSVC__
/// @cond
MSTL_BEGIN_INNER__
template <typename T>
struct __is_nothrow_destructible_aux {
private:
    template <typename T1>
    static bool_constant<noexcept(declval<T1&>().~T1())> __test(int);

    template <typename>
    static false_type __test(...);

public:
    using type =  decltype(__test<T>(0));
};

template <typename T,
    bool = disjunction<is_void<T>, is_unbounded_array<T>, is_function<T>>::value,
    bool = disjunction<is_reference<T>, is_scalar<T>>::value>
struct __is_nothrow_destructible_dispatch;

template <typename T>
struct __is_nothrow_destructible_dispatch<T, false, false>
    : __is_nothrow_destructible_aux<remove_all_extents_t<T>>::type {};

template <typename T>
struct __is_nothrow_destructible_dispatch<T, true, false> : false_type {};

template <typename T>
struct __is_nothrow_destructible_dispatch<T, false, true> : true_type {};
MSTL_END_INNER__
/// @endcond
#endif


/**
 * @struct is_nothrow_destructible
 * @brief 判断类型是否可无异常析构
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_destructible;

/// @cond
template <typename T>
struct is_nothrow_destructible :
#ifdef MSTL_COMPILER_MSVC__
    bool_constant<__is_nothrow_destructible(T)> {};
#else
    _INNER __is_nothrow_destructible_dispatch<T>::type {};
#endif
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_destructible_v
 * @brief is_nothrow_destructible的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_destructible_v = is_nothrow_destructible<T>::value;
#endif


/**
 * @struct is_location_invariant
 * @brief 判断类型是否是位置不变的
 * @tparam T 要检查的类型
 *
 * 位置不变意味着类型可以在内存中自由移动而不影响其行为。
 * 默认情况下，平凡可复制的类型是位置不变的。
 */
template <typename T>
struct is_location_invariant : is_trivially_copyable<T>::type {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_location_invariant_v
 * @brief is_location_invariant的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_location_invariant_v = is_location_invariant<T>::value;
#endif

/** @} */ // TypeSpecialMemberFunctionChecks

/**
 * @defgroup ArgsForwardFunctions 参数转发函数
 * @brief 参数转发函数实现
 * @{
 */

/**
 * @brief 完美转发左值
 * @tparam T 目标类型
 * @param x 要转发的左值
 * @return 转发后的引用
 *
 * 用于实现完美转发，保持值的类别
 */
template <typename T>
MSTL_NODISCARD constexpr T&& forward(remove_reference_t<T>& x) noexcept {
    return static_cast<T&&>(x);
}

/**
 * @brief 完美转发右值
 * @tparam T 目标类型
 * @param x 要转发的右值
 * @return 转发后的引用
 * @note 如果T是左值引用类型，触发静态断言
 *
 * 用于实现完美转发，保持值的类别
 */
template <typename T>
MSTL_NODISCARD constexpr T&& forward(remove_reference_t<T>&& x) noexcept {
    static_assert(!is_lvalue_reference<T>::value, "forward failed.");
    return static_cast<T&&>(x);
}

/**
 * @brief 无条件转换为右值引用
 * @tparam T 参数类型
 * @param x 要转换的值
 * @return 转换后的右值引用
 *
 * 用于实现移动语义
 */
template <typename T>
MSTL_NODISCARD constexpr remove_reference_t<T>&& move(T&& x) noexcept {
    return static_cast<remove_reference_t<T>&&>(x);
}

/**
 * @brief 在安全的情况下执行移动操作
 * @tparam T 参数类型
 * @param x 要移动的值
 * @return 如果可以安全移动则返回右值引用，否则返回const左值引用
 *
如果类型不可无异常移动构造但可复制构造，则返回const引用以避免潜在异常
 */
template <typename T>
MSTL_NODISCARD constexpr
conditional_t<!is_nothrow_move_constructible<T>::value && is_copy_constructible<T>::value, const T&, T&&>
move_if_noexcept(T& x) noexcept {
    return _MSTL move(x);
}

/**
 * @brief 获取对象的地址
 * @tparam T 对象类型
 * @param x 对象引用
 * @return 对象的指针
 *
 * 可避免重载的operator&干扰
 */
template <typename T>
MSTL_NODISCARD constexpr T* addressof(T& x) noexcept {
    return __builtin_addressof(x);
}

/**
 * @brief 禁止获取const右值的地址
 * @tparam T 对象类型
 * @note 删除const右值重载以防止悬垂引用
 */
template <typename T>
const T* addressof(const T&&) = delete;


#ifdef MSTL_STANDARD_17__
/**
 * @brief 检查当前上下文是否在常量求值中
 * @return 如果在常量求值上下文中返回true，否则返回false
 *
 * 用于区分编译时和运行时
 */
MSTL_NODISCARD constexpr bool is_constant_evaluated() noexcept {
    return __builtin_is_constant_evaluated();
}
#endif

/** @} */ // ArgsForwardFunctions

/**
 * @defgroup ConvertibleChecks 可转换性检查
 * @brief 检查类型之间的转换能力
 * @{
 */

#if !defined(MSTL_COMPILER_MSVC__) && !defined(MSTL_COMPILER_CLANG__)
/// @cond
MSTL_BEGIN_INNER__
template <typename From, typename To, bool = disjunction_v<is_void<From>, is_function<To>, is_array<To>>>
struct __is_convertible_helper {
    using type = typename is_void<To>::type;
};

template <typename From, typename To>
struct __is_convertible_helper<From, To, false> {
private:
    template <typename From1, typename To1, typename = decltype(_MSTL declvoid<To1>(_MSTL declval<From1>()))>
    static true_type __test(int);

    template <typename, typename>
    static false_type __test(...);

public:
    using type = decltype(__test<From, To>(0));
};
MSTL_END_INNER__
/// @endcond
#endif

/**
 * @struct is_convertible
 * @brief 判断类型From是否可以隐式转换为类型To
 * @tparam From 源类型
 * @tparam To 目标类型
 */
template <typename From, typename To>
struct is_convertible :
#if defined(MSTL_COMPILER_MSVC__)
    bool_constant<__is_convertible_to(From, To)> {};
#elif defined(MSTL_COMPILER_CLANG__)
    bool_constant<__is_convertible(From, To)> {};
#else
    __is_convertible_helper<From, To>::type {};
#endif

#ifdef MSTL_STANDARD_14__
/**
 * @var is_convertible_v
 * @brief is_convertible的便捷变量模板
 */
template <typename From, typename To>
MSTL_INLINE17 constexpr bool is_convertible_v = is_convertible<From, To>::value;
#endif

#if defined(MSTL_STANDARD_20__) || defined(MSTL_DOXYGEN_GENERATE)
/**
 * @concept convertible_to
 * @brief 检查类型From是否可以转换为类型To
 * @tparam From 源类型
 * @tparam To 目标类型
 */
template <typename From, typename To>
concept convertible_to = is_convertible_v<From, To> && requires { static_cast<To>(_MSTL declval<From>()); };
#endif // MSTL_STANDARD_20__


/**
 * @typedef is_array_convertible
 * @brief 判断数组元素类型FromElement是否可以转换为ToElement
 * @tparam ToElement 目标元素类型
 * @tparam FromElement 源元素类型
 *
 * 通过检查指向未知大小数组的指针的转换性来实现。
 */
template <typename ToElement, typename FromElement>
using is_array_convertible = is_convertible<FromElement(*)[], ToElement(*)[]>;

#ifdef MSTL_STANDARD_14__
/**
 * @var is_array_convertible_v
 * @brief is_array_convertible的便捷变量模板
 */
template <typename ToElement, typename FromElement>
MSTL_INLINE17 constexpr bool is_array_convertible_v = is_array_convertible<ToElement, FromElement>::value;
#endif


/**
 * @struct is_nothrow_convertible
 * @brief 判断类型From是否可以无异常地转换为类型To
 * @tparam From 源类型
 * @tparam To 目标类型
 * @tparam IsConvertible From是否可以转换为To
 * @tparam IsVoid To是否为void类型
 */
template <typename From, typename To,
    bool IsConvertible = is_convertible<From, To>::value,
    bool IsVoid = is_void<To>::value>
struct is_nothrow_convertible : bool_constant<noexcept(_MSTL declcopy<To>(_MSTL declval<From>()))> {};

/// @cond
template <typename From, typename To, bool IsVoid>
struct is_nothrow_convertible<From, To, false, IsVoid> : false_type {};

template <typename From, typename To>
struct is_nothrow_convertible<From, To, true, true> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_convertible_v
 * @brief is_nothrow_convertible的便捷变量模板
 */
template <typename From, typename To>
MSTL_INLINE17 constexpr bool is_nothrow_convertible_v = is_nothrow_convertible<From, To>::value;
#endif


/**
 * @struct is_nothrow_arrow
 * @brief 判断迭代器的箭头运算符是否不会抛出异常
 * @tparam Iterator 迭代器类型
 * @tparam Ptr 期望的指针类型
 * @tparam IsPtr Iterator是否为指针类型
 */
template <typename Iterator, typename Ptr, bool IsPtr = is_pointer<remove_cvref_t<Iterator>>::value>
struct is_nothrow_arrow : bool_constant<is_nothrow_convertible<Iterator, Ptr>::value> {};

/// @cond
template <typename Iterator, typename Ptr>
struct is_nothrow_arrow<Iterator, Ptr, false> : bool_constant<
    noexcept(_MSTL declcopy<Ptr>(_MSTL declval<Iterator>().operator->()))> {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_arrow_v
 * @brief is_nothrow_arrow的便捷变量模板
 */
template <typename Iterator, typename Ptr>
MSTL_INLINE17 constexpr bool is_nothrow_arrow_v = is_nothrow_arrow<Iterator, Ptr>::value;
#endif

/** @} */ // ConvertibleChecks

/**
 * @defgroup SignManipulation 符号操作
 * @brief 类型的符号操作
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__
template <size_t>
struct __sign_byte_aux;

template <>
struct __sign_byte_aux<1> {
    template <typename>
    using signed_t = signed char;
    template <typename>
    using unsigned_t = unsigned char;
};
template <>
struct __sign_byte_aux<2> {
    template <typename>
    using signed_t = signed short;
    template <typename>
    using unsigned_t = unsigned short;
};
template <>
struct __sign_byte_aux<4> {
#ifdef MSTL_PLATFORM_WINDOWS__
    template <typename T>
    using signed_t = 
        conditional_t<is_same_v<T, signed long> || is_same_v<T, unsigned long>, signed long, signed int>;

    template <typename T>
    using unsigned_t = 
        conditional_t<is_same_v<T, signed long> || is_same_v<T, unsigned long>, unsigned long, unsigned int>;
#elif defined(MSTL_PLATFORM_LINUX__)
    template <typename>
    using signed_t = signed int;
    template <typename>
    using unsigned_t = unsigned int;
#endif
};
template <>
struct __sign_byte_aux<8> {
#ifdef MSTL_PLATFORM_WINDOWS__
    template <typename>
    using signed_t = signed long long;
    template <typename>
    using unsigned_t = unsigned long long;
#elif defined(MSTL_PLATFORM_LINUX__)
    template <typename T>
    using signed_t =
        conditional_t<is_same<T, signed long>::value || is_same<T, unsigned long>::value, signed long, signed long long>;

    template <typename T>
    using unsigned_t =
        conditional_t<is_same<T, signed long>::value || is_same<T, unsigned long>::value, unsigned long, unsigned long long>;
#endif
};

template <typename T>
using __set_signed_byte = typename __sign_byte_aux<sizeof(T)>::template signed_t<T>;
template <typename T>
using __set_unsigned_byte = typename __sign_byte_aux<sizeof(T)>::template unsigned_t<T>;

template <typename T>
struct __set_sign {
    static_assert(is_integral_like<T>::value && !is_boolean<T>::value,
        "make signed only support non-bool && integral-like types");

    using signed_type   = copy_cv_t<T, __set_signed_byte<T>>;
    using unsigned_type = copy_cv_t<T, __set_unsigned_byte<T>>;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct make_signed
 * @brief 将类整数类型转换为对应的有符号类型
 * @tparam T 输入类型
 *
 * 保持输入类型的cv限定符，只改变符号。
 */
template <typename T>
struct make_signed {
    using type = typename _INNER __set_sign<T>::signed_type;
};

/**
 * @typedef make_signed_t
 * @brief make_signed的便捷别名
 */
template <typename T>
using make_signed_t = typename make_signed<T>::type;

/**
 * @struct make_unsigned
 * @brief 将类整数类型转换为对应的无符号类型
 * @tparam T 输入类型
 *
 * 保持输入类型的cv限定符，只改变符号。
 */
template <typename T>
struct make_unsigned {
    using type = typename _INNER __set_sign<T>::unsigned_type;
};

/**
 * @typedef make_unsigned_t
 * @brief make_unsigned的便捷别名
 */
template <typename T>
using make_unsigned_t = typename make_unsigned<T>::type;


/// @cond
MSTL_BEGIN_INNER__
template <size_t Size, bool IsSigned>
struct __make_integer_impl;

template <size_t Size>
struct __make_integer_impl<Size, true> {
    using type = typename __sign_byte_aux<Size>::template signed_t<int>;
};

template <size_t Size>
struct __make_integer_impl<Size, false> {
    using type = typename __sign_byte_aux<Size>::template unsigned_t<int>;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct make_integer
 * @brief 根据大小和符号创建整数类型
 * @tparam Size 字节大小
 * @tparam IsSigned 是否为有符号类型，默认为true
 */
template <size_t Size, bool IsSigned = true>
struct make_integer {
    using type = typename _INNER __make_integer_impl<Size, IsSigned>::type;
};

/**
 * @typedef make_integer_t
 * @brief make_integer的便捷别名
 */
template <size_t Size, bool IsSigned = true>
using make_integer_t = typename make_integer<Size, IsSigned>::type;


/**
 * @struct max_value
 * @brief 获取值列表中的最大值
 * @tparam Values 值列表
 */
template <size_t... Values>
struct max_value;

/// @cond
template <size_t Value>
struct max_value<Value> : integral_constant<size_t, Value> {};

template <size_t First, size_t Second, size_t... Rest>
struct max_value<First, Second, Rest...> : max_value<(First > Second ? First : Second), Rest...> {};
/// @endcond

#ifdef MSTL_STANDARD_14__
template <size_t... Values>
MSTL_INLINE17 constexpr size_t max_value_v = max_value<Values...>::value;
#endif

/** @} */ // SignManipulation

/**
 * @defgroup Alignment 对齐操作
 * @brief 类型的对齐操作和查询
 * @{
 */

/**
 * @struct alignment_of
 * @brief 查询类型的对齐要求
 * @tparam T 要查询的类型
 */
template <typename T>
struct alignment_of : integral_constant<size_t, alignof(T)> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var alignment_of_v
 * @brief alignment_of的便捷变量模板
 */
template <typename T>
constexpr size_t alignment_of_v = alignment_of<T>::value;
#endif


/**
 * @struct aligned_storage
 * @brief 创建指定大小和对齐要求的存储类型
 * @tparam Len 存储大小
 * @tparam Align 对齐要求
 */
template <size_t Len, size_t Align = alignof(_MSTL max_align_t)>
struct aligned_storage {
    static_assert((Align & (Align - 1)) == 0, "Alignment must be power of two");

    /**
     * @struct type
     * @brief 实际的对齐存储类型
     */
    struct alignas(Align) type {
        byte_t data[Len];  ///< 原始存储数据
    };
};

/**
 * @typedef aligned_storage_t
 * @brief aligned_storage的便捷别名
 */
template <size_t Len, size_t Align = alignof(_MSTL max_align_t)>
using aligned_storage_t = typename aligned_storage<Len, Align>::type;


/**
 * @struct aligned_union
 * @brief 创建可以容纳多种类型的对齐存储类型
 * @tparam Len 最小存储大小
 * @tparam Types 可能的类型列表
 *
 * 创建一个可以容纳Types中任何类型的存储，具有严格的对齐要求。
 */
template <size_t Len, typename... Types>
struct aligned_union {
private:
    static constexpr size_t required_alignment = max_value<alignof(Types)...>::value;
    static constexpr size_t required_size = max_value<sizeof(Types)...>::value;
    static constexpr size_t storage_size = (Len > required_size) ? Len : required_size;

    static_assert((required_alignment & (required_alignment - 1)) == 0, "Alignment must be power of two");

public:
    /**
     * @brief 存储的对齐要求
     */
    static constexpr size_t alignment_value = required_alignment;
    /**
     * @brief 存储的实际大小
     */
    static constexpr size_t size_value = storage_size;

    /**
     * @struct type
     * @brief 实际的对齐存储类型
     */
    struct alignas(alignment_value) type {
        byte_t data[storage_size];  ///< 原始存储数据
    };

    /**
     * @brief 检查指定类型是否可以安全存储在aligned_union中
     * @tparam T 要检查的类型
     * @return true 如果类型T可以安全存储，false 否则
     *
     * 类型T必须满足以下条件：
     * 1. 是平凡可复制的(trivially copyable)
     * 2. 大小不超过存储大小
     * 3. 对齐要求不超过存储对齐
     */
    template <typename T>
    static constexpr bool is_storable() noexcept {
        return is_trivially_copyable<T>::value &&
            sizeof(T) <= storage_size &&
            alignof(T) <= alignment_value;
    }
};

/**
 * @typedef aligned_union_t
 * @brief aligned_union的便捷别名
 */
template <size_t Len, typename... Types>
using aligned_union_t = typename aligned_union<Len, Types...>::type;

#ifdef MSTL_STANDARD_14__
/**
 * @var aligned_union_v
 * @brief 获取aligned_union的对齐要求值
 */
template <size_t Len, typename... Types>
constexpr size_t aligned_union_v = aligned_union<Len, Types...>::align_value;
#endif

/** @} */ // Alignment

/**
 * @defgroup TypeAttributeOperations 类型属性操作
 * @brief 进行类型退化、公共化等操作
 * @{
 */

/**
 * @struct decay
 * @brief 模拟函数参数传递中的类型退化
 * @tparam T 输入类型
 *
 * 退化规则：
 * 1. 移除引用和cv限定符
 * 2. 数组类型转换为指针类型（除非用于初始化引用）
 * 3. 函数类型转换为函数指针类型（除非用于初始化引用）
 */
template <typename T>
struct decay {
private:
    using remove_ref_t = remove_reference_t<T>;
    using check_func_t = conditional_t<is_function<remove_ref_t>::value, add_pointer_t<remove_ref_t>, remove_cv_t<remove_ref_t>>;

public:
    using type = conditional_t<is_array<remove_ref_t>::value, add_pointer_t<remove_extent_t<remove_ref_t>>, check_func_t>;
};

/**
 * @typedef decay_t
 * @brief decay的便捷别名
 */
template <typename T>
using decay_t = typename decay<T>::type;


/// @cond
MSTL_BEGIN_INNER__
template <typename Default, typename, template <typename...> class, typename...>
struct __detector {
    using value_t = false_type;
    using type = Default;
};
template <typename Default, template <typename...> class Op, typename... Args>
struct __detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = true_type;
    using type = Op<Args...>;
};
MSTL_END_INNER__
/// @endcond

/**
 * @typedef detected_or
 * @brief 检测Op<Args...>是否有效，如果无效则使用Default类型
 * @tparam Default 默认类型
 * @tparam Op 要检测的模板
 * @tparam Args 模板参数
 */
template <typename Default, template <typename...> class Op, typename... Args>
using detected_or = _INNER __detector<Default, void, Op, Args...>;

/**
 * @typedef detected_or_t
 * @brief detected_or的便捷别名，返回检测到的类型或默认类型
 */
template <typename Default, template <typename...> class Op, typename... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;


/**
 * @typedef common_ternary_operator_t
 * @brief 三目运算符的公共类型推导
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 三目运算符的类型推导规则：
 * 1. 如果T1和T2是相同类型，则整个表达式的类型是该类型
 * 2. 如果T1和T2是不同的类型，编译器会尝试找到一个公共类型
 * 3. 转换规则通常基于标准的隐式类型转换规则
 */
template <typename T1, typename T2>
using common_ternary_operator_t = decltype(true ? _MSTL declval<T1>() : _MSTL declval<T2>());


/// @cond
MSTL_BEGIN_INNER__
template <typename, typename, typename = void>
struct __oper_decay_aux {};
template <typename T1, typename T2>
struct __oper_decay_aux<T1, T2, void_t<common_ternary_operator_t<decay_t<T1>, decay_t<T2>>>> {
    using type = decay_t<common_ternary_operator_t<decay_t<T1>, decay_t<T2>>>;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct common_type
 * @brief 查找多个类型的公共类型
 * @tparam Types 类型列表
 */
template <typename... Types>
struct common_type;

/**
 * @typedef common_type_t
 * @brief common_type的便捷别名
 */
template <typename... Types>
using common_type_t = typename common_type<Types...>::type;

/// @cond
template <>
struct common_type<> {};

template <typename T1>
struct common_type<T1> : common_type<T1, T1> {};

template <typename T1, typename T2>
struct common_type<T1, T2> : _INNER __oper_decay_aux<T1, T2> {};

template <typename T1, typename T2, typename... Rest>
struct common_type<T1, T2, Rest...> : common_type<common_type_t<T1, T2>, Rest...> {};
/// @endcond


#ifdef MSTL_STANDARD_20__

/**
 * @struct common_reference
 * @brief 查找多个类型的公共引用类型
 * @tparam Types 类型列表
 */
template <typename... Types>
struct common_reference;

/**
 * @typedef common_reference_t
 * @brief common_reference的便捷别名
 */
template <typename... Types>
using common_reference_t = typename common_reference<Types...>::type;

/// @cond

template <>
struct common_reference<> {};

template <typename T>
struct common_reference<T> {
    using type = T;
};


MSTL_BEGIN_INNER__

template <typename T1, typename T2>
struct __common_reference_base_aux : common_type<T1, T2> {};

template <typename T1, typename T2> requires requires {
    typename _MSTL common_ternary_operator_t<T1, T2>;
}
struct __common_reference_base_aux<T1, T2> {
    using type = _MSTL common_ternary_operator_t<T1, T2>;
};

template <typename, typename, template <typename> typename, template <typename> typename>
struct __basic_common_reference {};

template <typename T1>
struct __add_qualifier_aux {
    template <typename T2>
    using apply_t = copy_ref_t<T1, copy_cv_t<T1, T2>>;
};

template <typename T1, typename T2>
using qualifier_extract = typename __basic_common_reference<remove_cvref_t<T1>, remove_cvref_t<T2>,
    __add_qualifier_aux<T1>::template apply_t, __add_qualifier_aux<T2>::template apply_t>::type;

template <typename T1, typename T2>
struct __common_ref_qualify_aux : __common_reference_base_aux<T1, T2> {};

template <typename T1, typename T2>
    requires requires { typename qualifier_extract<T1, T2>; }
struct __common_ref_qualify_aux<T1, T2> {
    using type = qualifier_extract<T1, T2>;
};

template <typename T1, typename T2>
struct __common_reference_ptr_aux : __common_ref_qualify_aux<T1, T2> {};

template <typename T1, typename T2> requires
    is_lvalue_reference_v<common_ternary_operator_t<copy_cv_t<T1, T2>&, copy_cv_t<T2, T1>&>>
using __common_lvalue_aux = common_ternary_operator_t<copy_cv_t<T1, T2>&, copy_cv_t<T2, T1>&>;

template <typename, typename>
struct __common_reference_aux {};

template <typename T1, typename T2>
    requires requires { typename __common_lvalue_aux<T1, T2>; }
struct __common_reference_aux<T1&, T2&> {
    using type = __common_lvalue_aux<T1, T2>;
};

template <typename T1, typename T2> requires
    is_convertible_v<T1&&, __common_lvalue_aux<const T1, T2>>
struct __common_reference_aux<T1&&, T2&> {
    using type = __common_lvalue_aux<const T1, T2>;
};

template <typename T1, typename T2> requires
    is_convertible_v<T2&&, __common_lvalue_aux<const T2, T1>>
struct __common_reference_aux<T1&, T2&&> {
    using type = __common_lvalue_aux<const T2, T1>;
};

template <typename T1, typename T2>
using __common_rvalue_aux = remove_reference_t<__common_lvalue_aux<T1, T2>>&&;

template <typename T1, typename T2> requires
    is_convertible_v<T1&&, __common_rvalue_aux<T1, T2>> &&
    is_convertible_v<T2&&, __common_rvalue_aux<T1, T2>>
struct __common_reference_aux<T1&&, T2&&> {
    using type = __common_rvalue_aux<T1, T2>;
};

template <typename T1, typename T2>
using __common_reference_aux_t = typename __common_reference_aux<T1, T2>::type;

template <typename T1, typename T2> requires
    is_convertible_v<add_pointer_t<T1>, add_pointer_t<__common_reference_aux_t<T1, T2>>> &&
    is_convertible_v<add_pointer_t<T2>, add_pointer_t<__common_reference_aux_t<T1, T2>>>
struct __common_reference_ptr_aux<T1, T2> {
    using type = __common_reference_aux_t<T1, T2>;
};

MSTL_END_INNER__


template <typename T1, typename T2>
struct common_reference<T1, T2> : _INNER __common_reference_ptr_aux<T1, T2> {};

template <typename T1, typename T2, typename T3, typename... Rest>
struct common_reference<T1, T2, T3, Rest...> {};

template <typename T1, typename T2, typename T3, typename... Rest>
    requires requires { typename common_reference_t<T1, T2>; }
struct common_reference<T1, T2, T3, Rest...> : common_reference<common_reference_t<T1, T2>, T3, Rest...> {};

/// @endcond
#endif


/**
 * @struct is_specialization
 * @brief 判断类型T是否为模板Template的特化
 * @tparam T 要检查的类型
 * @tparam Template 要检查的模板
 *
 * 用于检查某个类型是否是特定模板的特化版本。
 */
template <typename T, template <typename...> class Template>
struct is_specialization : false_type {};

/// @cond
template <template <typename...> class Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_17__
/**
 * @var is_specialization_v
 * @brief is_specialization的便捷变量模板
 */
template <typename T, template <typename...> class Template>
MSTL_INLINE17 constexpr bool is_specialization_v = is_specialization<T, Template>::value;
#else
/**
 * @brief is_specialization的便捷函数模板
 * @tparam T 要检查的类型
 * @tparam Template 要检查的模板
 * @return 如果T是Template的特化返回true，否则返回false
 */
template <typename T, template <typename...> class Template>
constexpr bool is_specialization_v() {
    return is_specialization<T, Template>::value;
}
#endif

/** @} */ // TypeAttributeOperations

/**
 * @defgroup SwapUtility 交换性工具
 * @brief 实现类型交换相关操作
 * @{
 */

template <typename>
struct is_swappable;

template <typename>
struct is_nothrow_swappable;


/**
 * @brief 交换两个相同类型的值
 * @tparam T 值的类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 */
template <typename T>
MSTL_CONSTEXPR14 enable_if_t<conjunction<is_move_constructible<T>, is_move_assignable<T>>::value>
swap(T& lhs, T& rhs)
noexcept(is_nothrow_move_constructible<T>::value && is_nothrow_move_assignable<T>::value);

/**
 * @brief 交换两个相同类型的数组
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param lhs 左操作数数组
 * @param rhs 右操作数数组
 */
template <typename T, size_t Size>
MSTL_CONSTEXPR14 enable_if_t<is_swappable<T>::value>
swap(T(& lhs)[Size], T(& rhs)[Size])
noexcept(is_nothrow_swappable<T>::value);

/**
 * @brief 删除无参数的swap重载
 */
void swap() = delete;

/**
 * @brief 将新值赋给对象并返回旧值
 * @tparam T 对象类型
 * @tparam U 新值类型
 * @param val 要替换的对象
 * @param new_val 新值
 * @return 对象的旧值
 */
template <typename T, typename U = T>
MSTL_CONSTEXPR14 T exchange(T& val, U&& new_val)
noexcept(conjunction<is_nothrow_move_constructible<T>, is_nothrow_assignable<T&, U>>::value);


/**
 * @struct is_swappable_from
 * @brief 判断是否可以调用swap从T1交换到T2
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T1, typename T2, typename Dummy = void>
struct is_swappable_from : false_type {};

/// @cond
template <typename T1, typename T2>
struct is_swappable_from<T1, T2, void_t<decltype(
    _MSTL swap(_MSTL declval<T1>(), _MSTL declval<T2>()))>> : true_type {};
/// @endcond


/**
 * @struct is_swappable_with
 * @brief 判断两个类型是否可以互相交换
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 *
 * 要求可以双向调用swap。
 */
template <typename T1, typename T2>
struct is_swappable_with : bool_constant<
    conjunction<is_swappable_from<T1, T2>, is_swappable_from<T2, T1>>::value> {};


/**
 * @struct is_swappable
 * @brief 判断类型是否可以与自身交换
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_swappable : bool_constant<
    is_swappable_with<add_lvalue_reference_t<T>, add_lvalue_reference_t<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_swappable_v
 * @brief is_swappable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_swappable_v = is_swappable<T>::value;
#endif


/**
 * @struct is_nothrow_swappable_from
 * @brief 判断是否可以无异常地从T1交换到T2
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
struct is_nothrow_swappable_from : bool_constant<
    noexcept(_MSTL swap(_MSTL declval<T1>(), _MSTL declval<T2>())) &&
    noexcept(_MSTL swap(_MSTL declval<T2>(), _MSTL declval<T1>()))> {};


/**
 * @struct is_nothrow_swappable_with
 * @brief 判断两个类型是否可以无异常地互相交换
 * @tparam T1 第一个类型
 * @tparam T2 第二个类型
 */
template <typename T1, typename T2>
struct is_nothrow_swappable_with : bool_constant<
    conjunction<is_swappable_with<T1, T2>, is_nothrow_swappable_from<T1, T2>>::value> {};


/**
 * @struct is_nothrow_swappable
 * @brief 判断类型是否可以与自身无异常交换
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_nothrow_swappable : bool_constant<
    is_nothrow_swappable_with<add_lvalue_reference_t<T>, add_lvalue_reference_t<T>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_swappable_v
 * @brief is_nothrow_swappable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;
#endif


/**
 * @struct is_ADL_swappable
 * @brief 判断类型是否支持通过ADL查找的swap
 * @tparam T 要检查的类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 参数依赖查找 ADL 是一种函数查找机制，
 * 具体来说，当调用函数时，除了全局和局部作用域外，
 * 编译器还会在参数类型所属的命名空间中查找函数声明。
 */
template <typename T, typename Dummy = void>
struct is_ADL_swappable : false_type {};

/// @cond
template <typename T>
struct is_ADL_swappable<T, void_t<
    decltype(swap(_MSTL declval<T&>(), _MSTL declval<T&>()))>> : true_type {};
/// @endcond


/**
 * @struct is_trivially_swappable
 * @brief 判断类型是否可以平凡交换
 * @tparam T 要检查的类型
 *
 * 类型可以平凡交换需要满足以下条件：
 * 1. 可以平凡析构
 * 2. 可以平凡移动构造
 * 3. 可以平凡移动赋值
 * 4. 不支持ADL查找的swap
 */
template <typename T>
struct is_trivially_swappable : bool_constant<conjunction<
    is_trivially_destructible<T>, is_trivially_move_constructible<T>,
    is_trivially_move_assignable<T>, negation<is_ADL_swappable<T>>>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_trivially_swappable_v
 * @brief is_trivially_swappable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_trivially_swappable_v = is_trivially_swappable<T>::value;
#endif


template <typename T>
MSTL_CONSTEXPR14 enable_if_t<conjunction<is_move_constructible<T>, is_move_assignable<T>>::value>
swap(T& lhs, T& rhs)
noexcept(is_nothrow_move_constructible<T>::value && is_nothrow_move_assignable<T>::value) {
    T tmp = _MSTL move(lhs);
    lhs = _MSTL move(rhs);
    rhs = _MSTL move(tmp);
    return;
}

template <typename T, size_t Size>
MSTL_CONSTEXPR14 enable_if_t<is_swappable<T>::value>
swap(T(& lhs)[Size], T(& rhs)[Size])
noexcept(is_nothrow_swappable<T>::value) {
    if (&lhs == &rhs) return;
    T* first1 = lhs;
    T* last1 = first1 + Size;
    T* first2 = rhs;
    for (; first1 != last1; ++first1, ++first2) {
        _MSTL swap(*first1, *first2);
    }
    return;
}

template <typename T, typename U>
MSTL_CONSTEXPR14 T exchange(T& val, U&& new_val)
noexcept(conjunction<is_nothrow_move_constructible<T>, is_nothrow_assignable<T&, U>>::value) {
    T old_val = _MSTL move(val);
    val = _MSTL forward<U>(new_val);
    return old_val;
}

/** @} */ // SwapUtility

/**
 * @defgroup TypeActionCheck 类型行为检查
 * @brief 检查类型的行为是否符合要求
 * @{
 */

#ifdef MSTL_STANDARD_20__
/**
 * @concept is_pair_v
 * @brief 检查类型是否具有类似pair的结构
 * @tparam T 要检查的类型
 *
 * 要求类型具有：
 * 1. first_type和second_type类型成员
 * 2. first和second数据成员
 */
template <typename T>
concept is_pair_v = requires(T p) {
    typename T::first_type;
    typename T::second_type;
    p.first;
    p.second;
};
#endif // MSTL_STANDARD_20__


/**
 * @struct is_allocator
 * @brief 判断类型是否为分配器
 * @tparam Alloc 要检查的类型
 * @tparam Dummy SFINAE参数，默认为void
 *
 * 分配器需要具有：
 * 1. value_type类型成员
 * 2. allocate(size_t)成员函数
 */
template <typename Alloc, typename Dummy = void>
struct is_allocator : false_type {};

/// @cond
template <typename Alloc>
struct is_allocator<Alloc, void_t<
    typename Alloc::value_type, decltype(declval<Alloc&>().allocate(size_t{}))>>
    : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var is_allocator_v
 * @brief is_allocator的便捷变量模板
 */
template <typename Alloc>
MSTL_INLINE17 constexpr bool is_allocator_v = is_allocator<Alloc>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename T>
struct __has_valid_begin_end {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        declval<U>().begin(), declval<U>().end(),
        is_same<decltype(declval<U>().begin()), decltype(declval<U>().end())>(),
        true_type{}
    );

    template <typename U>
    static false_type __test(...);

public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__
/// @endcond


/**
 * @struct is_incrementible
 * @brief 判断类型是否可以递增
 * @tparam Iterator 要检查的类型
 */
template <typename Iterator>
struct is_incrementible {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        ++declval<U&>(),
        true_type{}
    );

    template <typename U>
    static false_type __test(...);

public:
    static constexpr bool value = decltype(__test<Iterator>(0))::value;
};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_incrementible_v
 * @brief iis_incrementible的便捷变量模板
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_incrementible_v = is_incrementible<Iterator>::value;
#endif


/**
 * @struct is_decrementible
 * @brief 判断类型是否可以递减
 * @tparam Iterator 要检查的类型
 */
template <typename Iterator>
struct is_decrementible {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        --declval<U&>(),
        true_type{}
    );

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<Iterator>(0))::value;
};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_decrementible_v
 * @brief is_decrementible的便捷变量模板
 */
template <typename Iterator>
MSTL_INLINE17 constexpr bool is_decrementible_v = is_decrementible<Iterator>::value;
#endif


/**
 * @struct is_iterable
 * @brief 判断类型是否可迭代
 * @tparam Container 要检查的类型
 *
 * 可迭代类型需要具有：
 * 1. begin()和end()成员函数
 * 2. begin()返回的迭代器可以递增
 */
template <typename Container>
struct is_iterable : bool_constant<
    _INNER __has_valid_begin_end<Container>::value &&
    is_incrementible<decltype(declval<Container>().begin())>::value
> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_iterable_v
 * @brief is_iterable的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_iterable_v = is_iterable<T>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename T>
struct __has_first_and_second {
private:
    template <typename U>
    static auto __test(int) -> decltype(
        declval<U>().first, declval<U>().second,
        true_type{}
    );

    template <typename U>
    static false_type __test(...);

public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct is_maplike
 * @brief 判断类型是否类似映射
 * @tparam Map 要检查的类型
 *
 * 类似映射的类型需要：
 * 1. 是可迭代的
 * 2. 迭代器的解引用结果具有first和second成员
 */
template <typename Map>
struct is_maplike : bool_constant<
    is_iterable<Map>::value &&
    _INNER __has_first_and_second<decltype(*declval<decltype(declval<Map>().begin())>())>::value
> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_maplike_v
 * @brief is_maplike的便捷变量模板
 */
template <typename Map>
MSTL_INLINE17 constexpr bool is_maplike_v = is_maplike<Map>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename Alloc, typename T, typename... Args>
struct __has_construct_impl {
private:
    template <typename Alloc1,
    typename = decltype(_MSTL declval<Alloc1*>()->construct(_MSTL declval<T*>(), _MSTL declval<Args>()...))>
    static true_type __test(int);

    template <typename>
    static false_type __test(...);

public:
    using type = decltype(__test<Alloc>(0));
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct has_construct
 * @brief 判断分配器是否具有construct成员函数
 * @tparam Alloc 分配器类型
 * @tparam T 要构造的对象类型
 * @tparam Args 构造参数类型
 */
template <typename Alloc, typename T, typename... Args>
struct has_construct : _INNER __has_construct_impl<Alloc, T, Args...>::type {};

#ifdef MSTL_STANDARD_14__
/**
 * @var has_construct_v
 * @brief has_construct的便捷变量模板
 */
template <typename Alloc, typename T, typename... Args>
MSTL_INLINE17 constexpr bool has_construct_v = has_construct<Alloc, T, Args...>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__
template <typename T>
struct __has_base_impl {
private:
    template <typename U>
    static auto __test(int) -> decltype(_MSTL declval<const U>().base(), true_type{});

    template <typename U>
    static false_type __test(...);
public:
    static constexpr bool value = decltype(__test<T>(0))::value;
};
MSTL_END_INNER__
/// @endcond

/**
 * @struct has_base
 * @brief 判断类型是否具有base成员函数
 * @tparam T 要检查的类型
 */
template <typename T>
struct has_base : bool_constant<_INNER __has_base_impl<T>::value> {};

#ifdef MSTL_STANDARD_14__
/**
 * @var has_base_v
 * @brief has_base的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool has_base_v = has_base<T>::value;
#endif

/** @} */ // TypeActionCheck

/**
 * @defgroup TypeInitializeFunction 类型初始化函数
 * @brief 返回类型T的默认初始化值
 * @{
 */

/**
 * @brief 返回类型T的默认初始化值
 * @tparam T 要初始化的类型
 * @return T的默认初始化值
 * @note 编译器执行NRVO而不是移动返回值。
 *
 * C++11后，编译器在满足以下条件时执行命名返回值优化 NRVO：
 *   - 函数返回类类型对象且该对象是函数的局部对象。
 *   - 函数的return语句直接返回该局部对象。
 */
template <typename T, enable_if_t<is_default_constructible<T>::value, int> = 0>
constexpr T initialize() noexcept(is_nothrow_default_constructible<T>::value) {
    return T();
}

#define INITIALIZE_BASIC_FUNCTION__(OPT) \
template <> constexpr OPT initialize() noexcept { return static_cast<OPT>(0); }
MSTL_MACRO_RANGE_CHARS(INITIALIZE_BASIC_FUNCTION__)
MSTL_MACRO_RANGE_FLOAT(INITIALIZE_BASIC_FUNCTION__)
MSTL_MACRO_RANGE_INT(INITIALIZE_BASIC_FUNCTION__)

#undef INITIALIZE_BASIC_FUNCTION__

/** @} */ // TypeInitializeFunction

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TYPEINFO_TYPE_TRAITS_HPP__
