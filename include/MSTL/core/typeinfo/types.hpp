#ifndef MSTL_CORE_TYPEINFO_TYPES_HPP__
#define MSTL_CORE_TYPEINFO_TYPES_HPP__

/**
 * @file types.hpp
 * @brief MSTL核心类型定义
 * @namespace MSTL
 * @ingroup TypeAlias
 *
 * 此文件定义了MSTL库中的基本类型别名、固定大小类型和平台相关的类型定义，提供统一和跨平台的基本类型。
 */

#include "../config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup CoreTypes 核心类型
 * @brief 基础类型别名和通用类型定义
 * @{
 */

/**
 * @typedef nullptr_t
 * @brief 空指针类型
 */
using nullptr_t	  = decltype(nullptr);

/**
 * @typedef max_align_t
 * @brief 最大对齐类型
 */
using max_align_t = double;

/**
 * @typedef byte_t
 * @brief 字节类型，定义为无符号字符
 */
using byte_t      = unsigned char;

/** @} */ // CoreTypes

/**
 * @defgroup FixedWidthIntegers 固定宽度整数类型
 * @brief 精确宽度的整数类型定义
 * @{
 */

/**
 * @typedef int8_t
 * @brief 8位有符号整数类型
 */
using int8_t	= signed char;

/**
 * @typedef int16_t
 * @brief 16位有符号整数类型
 */
using int16_t	= short;

/**
 * @typedef int32_t
 * @brief 32位有符号整数类型
 */
using int32_t	= int;

#ifdef MSTL_PLATFORM_LINUX64__
/**
 * @typedef int64_t
 * @brief 64位有符号整数类型
 */
using int64_t	= long;
#else
/**
 * @typedef int64_t
 * @brief 64位有符号整数类型
 */
using int64_t	= long long;
#endif

/**
 * @typedef uint8_t
 * @brief 8位无符号整数类型
 */
using uint8_t	= unsigned char;

/**
 * @typedef uint16_t
 * @brief 16位无符号整数类型
 */
using uint16_t	= unsigned short;

/**
 * @typedef uint32_t
 * @brief 32位无符号整数类型
 */
using uint32_t	= unsigned int;

#ifdef MSTL_PLATFORM_LINUX64__
/**
 * @typedef uint64_t
 * @brief 64位无符号整数类型
 */
using uint64_t	= unsigned long;
#else
/**
 * @typedef uint64_t
 * @brief 64位无符号整数类型
 */
using uint64_t	= unsigned long long;
#endif

/** @} */ // FixedWidthIntegers

/**
 * @defgroup FloatingPointTypes 浮点数类型
 * @brief 浮点数类型定义
 * @{
 */

/**
 * @typedef float32_t
 * @brief 32位单精度浮点数类型
 */
using float32_t	= float;

/**
 * @typedef float64_t
 * @brief 64位双精度浮点数类型
 */
using float64_t	= double;

/**
 * @typedef decimal_t
 * @brief 扩展精度浮点数类型
 *
 * 使用long double实现，提供比double更高的精度。
 */
using decimal_t = long double;

/** @} */ // FloatingPointTypes

/**
 * @defgroup PlatformDependentTypes 平台相关类型
 * @brief 根据平台位数定义的大小和指针相关类型
 * @{
 */

#ifdef MSTL_DATA_BUS_WIDTH_64__

/**
 * @typedef size_t
 * @brief 无符号大小类型
 */
using size_t	= uint64_t;

/**
 * @typedef ssize_t
 * @brief 有符号大小类型
 */
using ssize_t	= int64_t;

/**
 * @typedef ptrdiff_t
 * @brief 指针差类型
 */
using ptrdiff_t = int64_t;

/**
 * @typedef intptr_t
 * @brief 可容纳指针的有符号整数类型
 */
using intptr_t	= int64_t;

/**
 * @typedef uintptr_t
 * @brief 可容纳指针的无符号整数类型
 */
using uintptr_t = uint64_t;

#else

/**
 * @typedef size_t
 * @brief 无符号大小类型
 */
using size_t	= uint32_t;

/**
 * @typedef ssize_t
 * @brief 有符号大小类型
 */
using ssize_t	= int32_t;

/**
 * @typedef ptrdiff_t
 * @brief 指针差类型
 */
using ptrdiff_t = int32_t;

/**
 * @typedef intptr_t
 * @brief 可容纳指针的有符号整数类型
 */
using intptr_t	= int32_t;

/**
 * @typedef uintptr_t
 * @brief 可容纳指针的无符号整数类型
 */
using uintptr_t = uint32_t;

#endif

/** @} */ // PlatformDependentTypes

/**
 * @defgroup LeastFastTypes 最小和最快类型
 * @brief 至少指定宽度和最快访问的类型定义
 * @{
 */

/**
 * @typedef int_least8_t
 * @brief 至少8位的有符号整数类型
 */
using int_least8_t   = int8_t;

/**
 * @typedef int_least16_t
 * @brief 至少16位的有符号整数类型
 */
using int_least16_t  = int16_t;

/**
 * @typedef int_least32_t
 * @brief 至少32位的有符号整数类型
 */
using int_least32_t  = int32_t;

/**
 * @typedef int_least64_t
 * @brief 至少64位的有符号整数类型
 */
using int_least64_t  = int64_t;

/**
 * @typedef uint_least8_t
 * @brief 至少8位的无符号整数类型
 */
using uint_least8_t  = uint8_t;

/**
 * @typedef uint_least16_t
 * @brief 至少16位的无符号整数类型
 */
using uint_least16_t = uint16_t;

/**
 * @typedef uint_least32_t
 * @brief 至少32位的无符号整数类型
 */
using uint_least32_t = uint32_t;

/**
 * @typedef uint_least64_t
 * @brief 至少64位的无符号整数类型
 */
using uint_least64_t = uint64_t;

/**
 * @typedef int_fast8_t
 * @brief 快速8位有符号整数类型
 */
using int_fast8_t    = int8_t;

/**
 * @typedef int_fast16_t
 * @brief 快速16位有符号整数类型（使用ssize_t）
 */
using int_fast16_t   = ssize_t;

/**
 * @typedef int_fast32_t
 * @brief 快速32位有符号整数类型（使用ssize_t）
 */
using int_fast32_t   = ssize_t;

/**
 * @typedef int_fast64_t
 * @brief 快速64位有符号整数类型
 */
using int_fast64_t   = int64_t;

/**
 * @typedef uint_fast8_t
 * @brief 快速8位无符号整数类型
 */
using uint_fast8_t   = uint8_t;

/**
 * @typedef uint_fast16_t
 * @brief 快速16位无符号整数类型（使用size_t）
 */
using uint_fast16_t  = size_t;

/**
 * @typedef uint_fast32_t
 * @brief 快速32位无符号整数类型（使用size_t）
 */
using uint_fast32_t  = size_t;

/**
 * @typedef uint_fast64_t
 * @brief 快速64位无符号整数类型
 */
using uint_fast64_t  = uint64_t;

/** @} */ // LeastFastTypes

/**
 * @defgroup MaxWidthIntegers 最大宽度整数类型
 * @brief 能够容纳最大整数值的类型
 * @{
 */

/**
 * @typedef intmax_t
 * @brief 最大有符号整数类型
 */
using intmax_t	= int64_t;

/**
 * @typedef uintmax_t
 * @brief 最大无符号整数类型
 */
using uintmax_t = uint64_t;

/** @} */ // MaxWidthIntegers

/**
 * @defgroup TypeAliasMacros 类型别名宏
 * @brief 用于快速定义标准类型别名的宏
 * @{
 */

/**
 * @def MSTL_BUILD_TYPE_ALIAS
 * @brief 快速构建标准类型别名
 * @param TYPE 要为其创建别名的基类型
 *
 * 此宏为一组常用的STL风格类型别名生成定义，包括：
 * - value_type: 值类型
 * - pointer: 指针类型
 * - reference: 引用类型
 * - const_pointer: 常量指针类型
 * - const_reference: 常量引用类型
 * - size_type: 大小类型（使用size_t）
 * - difference_type: 差值类型（使用ptrdiff_t）
 *
 * @code
 * // 使用示例：
 * template <typename T>
 * class MyContainer {
 *     MSTL_BUILD_TYPE_ALIAS(T);
 *     // 现在可以快速使用value_type、pointer等类型别名
 * };
 * @endcode
 */

// quickly define standard type alias.
#define MSTL_BUILD_TYPE_ALIAS(TYPE) \
using value_type        = TYPE; \
using pointer           = TYPE*; \
using reference         = TYPE&; \
using const_pointer     = const TYPE*; \
using const_reference   = const TYPE&; \
using size_type         = size_t; \
using difference_type   = ptrdiff_t;

/** @} */ // TypeAliasMacros

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_TYPEINFO_TYPES_HPP__
