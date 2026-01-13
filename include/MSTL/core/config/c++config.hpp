#ifndef MSTL_CORE_CONFIG_CPPCONFIG_HPP__
#define MSTL_CORE_CONFIG_CPPCONFIG_HPP__

/**
 * @file c++config.hpp
 * @brief MSTL核心配置头文件
 * @namespace MSTL
 * @mainpage MSTL (My Standard Template Library)
 *
 * @section intro_sec 简介
 * MSTL是一个跨平台的C++标准模板库实现，提供了STL的替代实现和扩展功能
 * 此头文件定义了整个库的平台、编译器和语言特性的配置宏
 *
 * @section usage_sec 使用说明
 * 包含此头文件以获取所有配置定义
 * 利用条件编译宏处理平台差异
 */

#include "undef_cmacro.hpp"

/**
 * @defgroup PlatformDetection 平台检测
 * @brief 检测和定义目标平台的宏
 * @{
 */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(_M_X86)
    /**
     * @def MSTL_PLATFORM_WINDOWS__
     * @brief 定义在Windows平台编译
     */
	#define MSTL_PLATFORM_WINDOWS__		1
    /**
     * @def MSTL_PLATFORM_WIN32__
     * @brief 定义在32位Windows平台编译
     */
	#define MSTL_PLATFORM_WIN32__		1
	#if defined(WIN64) || defined(_WIN64) || defined(_WIN64_) || defined(_M_X64)
        /**
         * @def MSTL_PLATFORM_WIN64__
         * @brief 定义在64位Windows平台编译
         */
		#define MSTL_PLATFORM_WIN64__	1
	#endif
#elif defined(__linux__)
    /**
     * @def MSTL_PLATFORM_LINUX__
     * @brief 定义在Linux平台编译
     */
	#define MSTL_PLATFORM_LINUX__		1
	#if (__WORDSIZE == 64) || (__SIZEOF_POINTER__ == 8)
        /**
         * @def MSTL_PLATFORM_LINUX64__
         * @brief 定义在64位Linux平台编译
         */
		#define MSTL_PLATFORM_LINUX64__ 1
	#elif (__WORDSIZE == 32) || (__SIZEOF_POINTER__ == 4) || defined(MSTL_PLATFORM_LINUX64__)
        /**
         * @def MSTL_PLATFORM_LINUX32__
         * @brief 定义在32位Linux平台编译
         */
		#define MSTL_PLATFORM_LINUX32__ 1
	#endif
#else
    #error "不支持的平台"
#endif

/** @} */ // PlatformDetection

/**
 * @defgroup CompilerDetection 编译器检测
 * @brief 检测和定义编译器的宏
 * @{
 */

#ifdef MSTL_PLATFORM_WINDOWS__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif


#if defined(__GNUC__)
    /**
     * @def MSTL_COMPILER_GNUC__
     * @brief 定义使用GNU编译器编译
     */
	#define MSTL_COMPILER_GNUC__		1
	#if defined(__clang__)
        /**
         * @def MSTL_COMPILER_CLANG__
         * @brief 定义使用Clang编译器编译
         */
		#define MSTL_COMPILER_CLANG__	1
	#else
        /**
         * @def MSTL_COMPILER_GCC__
         * @brief 定义使用GCC编译器编译
         */
		#define MSTL_COMPILER_GCC__		1
	#endif
#elif defined(_MSC_VER)
    /**
     * @def MSTL_COMPILER_MSVC__
     * @brief 定义使用Microsoft Visual C++编译器编译
     */
	#define MSTL_COMPILER_MSVC__		1
#else
    #error "不支持的编译器"
#endif

/** @} */ // CompilerDetection

/**
 * @defgroup APIDeclSpec API声明规范
 * @brief 动态库导入导出声明
 * @{
 */

#ifdef MSTL_COMPILER_MSVC__
    #ifdef MSTL_DLLEXPORTS
        /**
         * @def MSTL_API
         * @brief MSVC动态库导出声明
         */
        #define MSTL_API __declspec(dllexport)
    #else
        /**
         * @def MSTL_API
         * @brief MSVC动态库导入声明
         */
        #define MSTL_API __declspec(dllimport)
    #endif
#else
    /**
     * @def MSTL_API
     * @brief 空定义
     */
    #define MSTL_API
#endif

/** @} */ // APIDeclSpec

/**
 * @defgroup DataBusWidth 数据总线宽度
 * @brief 系统架构位宽检测
 * @{
 */

#if defined(MSTL_PLATFORM_WIN64__) || defined(MSTL_PLATFORM_LINUX64__) || defined(__amd64__) || defined(__x86_64__) || defined(__aarch64__)
	/**
     * @def MSTL_DATA_BUS_WIDTH_64__
     * @brief 定义在64位系统编译
     */
	#define MSTL_DATA_BUS_WIDTH_64__	1
#endif
#if defined(MSTL_PLATFORM_WIN32__) || defined(MSTL_PLATFORM_LINUX32__) || defined(__i386__)
	/**
     * @def MSTL_DATA_BUS_WIDTH_32__
     * @brief 定义在32位系统编译
     */
	#define MSTL_DATA_BUS_WIDTH_32__	1
#endif

/** @} */ // DataBusWidth

/**
 * @defgroup NamespaceMacros 命名空间宏
 * @brief 定义MSTL库的命名空间结构
 * @{
 */

/**
 * @def __MSTL_GLOBAL_NAMESPACE__
 * @brief 全局命名空间MSTL名称
 */
#define __MSTL_GLOBAL_NAMESPACE__ MSTL

/**
 * @def MSTL_BEGIN_NAMESPACE__
 * @brief 开始全局命名空间MSTL
 */
#define MSTL_BEGIN_NAMESPACE__ namespace __MSTL_GLOBAL_NAMESPACE__ {

/**
 * @def MSTL_END_NAMESPACE__
 * @brief 结束全局命名空间MSTL
 */
#define MSTL_END_NAMESPACE__ }

/**
 * @def _MSTL
 * @brief 全局命名空间MSTL前缀
 */
#define _MSTL __MSTL_GLOBAL_NAMESPACE__ ::


/**
 * @def __MSTL_INNER_NAMESPACE__
 * @brief inner命名空间名称
 */
#define __MSTL_INNER_NAMESPACE__ inner

/**
 * @def MSTL_BEGIN_INNER__
 * @brief 开始inner命名空间
 */
#define MSTL_BEGIN_INNER__ namespace __MSTL_INNER_NAMESPACE__ {

/**
 * @def MSTL_END_INNER__
 * @brief 结束inner命名空间
 */
#define MSTL_END_INNER__ }

/**
 * @def _INNER
 * @brief inner命名空间前缀
 */
#define _INNER __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_INNER_NAMESPACE__ ::


/**
 * @def __MSTL_CONSTANTS_NAMESPACE__
 * @brief constants命名空间名称
 */
#define __MSTL_CONSTANTS_NAMESPACE__ constants

/**
 * @def MSTL_BEGIN_CONSTANTS__
 * @brief 开始constants命名空间
 */
#define MSTL_BEGIN_CONSTANTS__ namespace __MSTL_CONSTANTS_NAMESPACE__ {

/**
 * @def MSTL_END_CONSTANTS__
 * @brief 结束constants命名空间
 */
#define MSTL_END_CONSTANTS__ }

/**
 * @def _CONSTANTS
 * @brief constants命名空间前缀
 */
#define _CONSTANTS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_CONSTANTS_NAMESPACE__ ::


/**
 * @def __MSTL_THIS_THREAD_NAMESPACE__
 * @brief this_thread命名空间名称
 */
#define __MSTL_THIS_THREAD_NAMESPACE__ this_thread

/**
 * @def MSTL_BEGIN_THIS_THREAD__
 * @brief this_thread命名空间
 */
#define MSTL_BEGIN_THIS_THREAD__ namespace __MSTL_THIS_THREAD_NAMESPACE__ {

/**
 * @def MSTL_END_THIS_THREAD__
 * @brief 结束this_thread命名空间
 */
#define MSTL_END_THIS_THREAD__ }

/**
 * @def _THIS_THREAD
 * @brief this_thread命名空间前缀
 */
#define _THIS_THREAD __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_THIS_THREAD_NAMESPACE__ ::


/**
 * @def __MSTL_RANGES_NAMESPACE__
 * @brief ranges命名空间名称
 */
#define __MSTL_RANGES_NAMESPACE__ ranges

/**
 * @def MSTL_BEGIN_RANGES__
 * @brief 开始ranges命名空间
 */
#define MSTL_BEGIN_RANGES__ namespace __MSTL_RANGES_NAMESPACE__ {

/**
 * @def MSTL_END_RANGES__
 * @brief 结束ranges命名空间
 */
#define MSTL_END_RANGES__ }

/**
 * @def _MSTL_RANGES
 * @brief ranges命名空间前缀
 */
#define _MSTL_RANGES __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_RANGES_NAMESPACE__ ::

/**
 * @def __MSTL_RANGES_VIEWS_NAMESPACE__
 * @brief ranges::view命名空间名称
 */
#define __MSTL_RANGES_VIEWS_NAMESPACE__ views

/**
 * @def MSTL_BEGIN_RANGES_VIEWS__
 * @brief 开始ranges::view命名空间
 */
#define MSTL_BEGIN_RANGES_VIEWS__ namespace __MSTL_RANGES_VIEWS_NAMESPACE__ {

/**
 * @def MSTL_END_RANGES_VIEWS__
 * @brief 结束ranges::view命名空间
 */
#define MSTL_END_RANGES_VIEWS__ }

/**
 * @def _RANGES_VIEWS
 * @brief ranges::view命名空间前缀
 */
#define _RANGES_VIEWS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_RANGES_NAMESPACE__ :: __MSTL_RANGES_VIEWS_NAMESPACE__ ::


/**
 * @def __MSTL_LITERALS_NAMESPACE__
 * @brief literals命名空间名称
 */
#define __MSTL_LITERALS_NAMESPACE__ literals

/**
 * @def MSTL_BEGIN_LITERALS__
 * @brief 开始literals命名空间（内联）
 */
#define MSTL_BEGIN_LITERALS__ inline namespace __MSTL_LITERALS_NAMESPACE__ {

/**
 * @def MSTL_END_LITERALS__
 * @brief 结束literals命名空间
 */
#define MSTL_END_LITERALS__ }

/**
 * @def _LITERALS
 * @brief literals命名空间前缀
 */
#define _LITERALS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_LITERALS_NAMESPACE__ ::

/** @} */ // NamespaceMacros

/**
 * @defgroup CxxStandardDetection C++标准检测
 * @brief 检测和定义C++语言标准的宏
 * @{
 */

#if _HAS_CXX23 || (__cplusplus >= 202100L) || (_MSVC_LANG >= 202100L)
    /**
     * @def MSTL_STANDARD_23__
     * @brief 使用C++23或更高标准编译
     */
	#define MSTL_STANDARD_23__	1
#endif
#if _HAS_CXX20 || (__cplusplus >= 202002L) || (_MSVC_LANG >= 202002L)
    /**
     * @def MSTL_STANDARD_20__
     * @brief 使用C++20或更高标准编译
     */
	#define MSTL_STANDARD_20__	1
#endif
#if _HAS_CXX17 || (__cplusplus >= 201703L) || defined(MSTL_STANDARD_20__) || (_MSVC_LANG >= 201703L)
    /**
     * @def MSTL_STANDARD_17__
     * @brief 使用C++17或更高标准编译
     */
	#define MSTL_STANDARD_17__	1
#endif
#if (__cplusplus >= 201402L) || defined(MSTL_STANDARD_17__) || (_MSVC_LANG >= 201402L)
    /**
     * @def MSTL_STANDARD_14__
     * @brief 使用C++14或更高标准编译
     */
	#define MSTL_STANDARD_14__	1
#endif
#if (__cplusplus >= 201103L) || defined(MSTL_STANDARD_14__) || (_MSVC_LANG >= 201103L)
    /**
     * @def MSTL_STANDARD_11__
     * @brief 使用C++11或更高标准编译
     */
	#define MSTL_STANDARD_11__	1
#endif
#if (__cplusplus >= 199711L) || defined(MSTL_STANDARD_11__) || (_MSVC_LANG >= 199711L)
    /**
     * @def MSTL_STANDARD_98__
     * @brief 使用C++98或更高标准编译
     */
	#define MSTL_STANDARD_98__	1
#endif

/** @} */ // CxxStandardDetection

/**
 * @defgroup LanguageFeatures 语言特性
 * @brief C++不同标准下的语言特性支持
 * @{
 */

#ifdef MSTL_STANDARD_17__
    /**
     * @def MSTL_SUPPORT_DEDUCTION_GUIDES__
     * @brief 支持C++17的模板参数推导
     */
	#define MSTL_SUPPORT_DEDUCTION_GUIDES__ 1
#endif


#ifdef MSTL_STANDARD_11__
    /**
     * @def MSTL_CONSTEXPR11
     * @brief C++11的constexpr
     */
	#define MSTL_CONSTEXPR11 constexpr
#else
    /**
     * @def MSTL_CONSTEXPR11
     * @brief C++11之前降级为inline
     */
	#define MSTL_CONSTEXPR11 inline
#endif // MSTL_STANDARD_11__

#ifdef MSTL_STANDARD_14__
    /**
     * @def MSTL_CONSTEXPR14
     * @brief C++14的constexpr
     */
    #define MSTL_CONSTEXPR14 constexpr
#else
    /**
     * @def MSTL_CONSTEXPR14
     * @brief C++14之前降级为inline
     */
    #define MSTL_CONSTEXPR14 inline
#endif // MSTL_STANDARD_14__

#ifdef MSTL_STANDARD_17__
    /**
     * @def MSTL_CONSTEXPR17
     * @brief C++17的constexpr
     */
    #define MSTL_CONSTEXPR17 constexpr

    /**
     * @def MSTL_INLINE17
     * @brief C++17的inline标记
     */
    #define MSTL_INLINE17 inline
#else
    /**
     * @def MSTL_CONSTEXPR17
     * @brief C++17之前降级为inline
     */
    #define MSTL_CONSTEXPR17 inline

    /**
     * @def MSTL_INLINE17
     * @brief C++17之前为空
     */
    #define MSTL_INLINE17
#endif // MSTL_STANDARD_17__

#ifdef MSTL_STANDARD_20__
    /**
     * @def MSTL_CONSTEXPR20
     * @brief C++20的constexpr
     */
    #define MSTL_CONSTEXPR20 constexpr
#else
    /**
     * @def MSTL_CONSTEXPR20
     * @brief C++20之前降级为inline
     */
    #define MSTL_CONSTEXPR20 inline
#endif // MSTL_STANDARD_20__

#ifdef MSTL_STANDARD_23__
    /**
     * @def MSTL_CONSTEXPR23
     * @brief C++23的constexpr
     */
    #define MSTL_CONSTEXPR23 constexpr
#else
    /**
     * @def MSTL_CONSTEXPR23
     * @brief C++23之前降级为inline
     */
    #define MSTL_CONSTEXPR23 inline
#endif // MSTL_STANDARD_23__


#ifdef MSTL_STANDARD_17__
    /**
     * @def MSTL_IF_CONSTEXPR
     * @brief C++17的if constexpr
     */
    #define MSTL_IF_CONSTEXPR if constexpr
#else
    /**
     * @def MSTL_IF_CONSTEXPR
     * @brief C++17之前的版本使用普通if
     * @warning 此宏需谨慎使用，因其会在C++17之前破坏静态重载
     */
    #define MSTL_IF_CONSTEXPR if
#endif

/** @} */ // LanguageFeatures

/**
 * @defgroup Attributes 属性
 * @brief C++和编译器特定的属性
 * @{
 */

#ifdef MSTL_STANDARD_17__
    /**
     * @def MSTL_NODISCARD
     * @brief [[nodiscard]]属性，禁止丢弃返回值
     */
    #define MSTL_NODISCARD [[nodiscard]]

    /**
     * @def MSTL_ALLOC_NODISCARD
     * @brief 分配器的nodiscard属性，丢弃返回值会导致内存泄漏
     */
    #define MSTL_ALLOC_NODISCARD \
        [[nodiscard("discard the return of allocators will cause memory leaks.")]]
#else
    /**
     * @def MSTL_NODISCARD
     * @brief C++17之前的版本为空
     */
    #define MSTL_NODISCARD

    /**
     * @def MSTL_ALLOC_NODISCARD
     * @brief C++17之前的版本为空
     */
    #define MSTL_ALLOC_NODISCARD
#endif


#ifdef MSTL_COMPILER_GNUC__
    /**
     * @def MSTL_ALIGNED_DEFAULT
     * @brief 默认对齐属性
     */
	#define MSTL_ALIGNED_DEFAULT() __attribute__((__aligned__))
    /**
     * @def MSTL_ALIGNED
     * @brief 指定对齐属性
     * @param ALIGN 对齐字节数
     */
	#define MSTL_ALIGNED(ALIGN) __attribute__((__aligned__((ALIGN))))
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_ALIGNED_DEFAULT
     * @brief 默认对齐属性
     */
	#define MSTL_ALIGNED_DEFAULT() __declspec(align(alignof(_MSTL max_align_t)))
    /**
     * @def MSTL_ALIGNED
     * @brief 指定对齐属性
     * @param ALIGN 对齐字节数
     */
	#define MSTL_ALIGNED(ALIGN) __declspec(align(ALIGN))
#else
    /**
     * @def MSTL_ALIGNED_DEFAULT
     * @brief 默认对齐属性
     */
	#define MSTL_ALIGNED_DEFAULT() alignas(alignof(_MSTL max_align_t))
    /**
     * @def MSTL_ALIGNED
     * @brief 空定义
     * @param ALIGN 对齐字节数
     */
	#define MSTL_ALIGNED(ALIGN) alignas(ALIGN)
#endif


#ifdef MSTL_COMPILER_GNUC__
    /**
     * @def MSTL_ALWAYS_INLINE
     * @brief 强制内联
     */
    #define MSTL_ALWAYS_INLINE __attribute__((always_inline))

    /**
     * @def MSTL_ALWAYS_INLINE_INLINE
     * @brief 强制内联加inline
     */
    #define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE inline
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_ALWAYS_INLINE
     * @brief 强制内联
     */
    #define MSTL_ALWAYS_INLINE __forceinline

    /**
     * @def MSTL_ALWAYS_INLINE_INLINE
     * @brief 强制内联
     */
    #define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE
#elif defined(MSTL_STANDARD_17__)
    /**
     * @def MSTL_ALWAYS_INLINE
     * @brief 强制内联
     */
    #define MSTL_ALWAYS_INLINE [[always_inline]]

    /**
     * @def MSTL_ALWAYS_INLINE_INLINE
     * @brief 强制内联加inline
     */
    #define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE inline
#else
    /**
     * @def MSTL_ALWAYS_INLINE
     * @brief 空定义
     */
    #define MSTL_ALWAYS_INLINE

    /**
     * @def MSTL_ALWAYS_INLINE_INLINE
     * @brief 空定义
     */
    #define MSTL_ALWAYS_INLINE_INLINE
#endif


#ifdef MSTL_COMPILER_GNUC__
    /**
     * @def MSTL_UNUSED
     * @brief 未使用变量警告抑制
     */
    #define MSTL_UNUSED __attribute__((unused))
#else
    /**
     * @def MSTL_UNUSED
     * @brief 空定义
     */
    #define MSTL_UNUSED
#endif


#ifdef MSTL_STANDARD_17__
    /**
     * @def MSTL_UNLIKELY
     * @brief 分支预测：不太可能执行
     */
    #define MSTL_UNLIKELY [[unlikely]]
#else
    /**
     * @def MSTL_UNLIKELY
     * @brief 空定义
     */
    #define MSTL_UNLIKELY
#endif


#ifdef MSTL_STANDARD_20__
    /**
     * @def MSTL_LIKELY
     * @brief 分支预测：很可能执行
     */
    #define MSTL_LIKELY [[likely]]
#else
    /**
     * @def MSTL_LIKELY
     * @brief 空定义
     */
    #define MSTL_LIKELY
#endif


#ifdef MSTL_COMPILER_GNUC__
    /**
     * @def MSTL_NORETURN
     * @brief 无返回值函数属性
     */
    #define MSTL_NORETURN __attribute__((noreturn))
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_NORETURN
     * @brief 无返回值函数属性
     */
    #define MSTL_NORETURN __declspec(noreturn)
#elif defined(MSTL_STANDARD_11__)
    /**
     * @def MSTL_NORETURN
     * @brief 无返回值函数属性
     */
    #define MSTL_NORETURN [[noreturn]]
#else
    /**
     * @def MSTL_NORETURN
     * @brief 空定义
     */
    #define MSTL_NORETURN
#endif


#ifdef MSTL_COMPILER_GNUC__
    /**
     * @def MSTL_PURE_FUNCTION
     * @brief 纯函数属性
     */
    #define MSTL_PURE_FUNCTION __attribute__((__pure__))

    /**
     * @def MSTL_MALLOC_FUNCTION
     * @brief malloc类函数属性
     */
    #define MSTL_MALLOC_FUNCTION __attribute__((__malloc__))

    /**
     * @def MSTL_CONST_FUNCTION
     * @brief 常量函数属性
     */
    #define MSTL_CONST_FUNCTION __attribute__((__const__))

    /**
     * @def MSTL_NONNULL_FUNCTION
     * @brief 非空参数函数属性
     * @param PARAMS 非空参数列表
     */
    #define MSTL_NONNULL_FUNCTION(PARAMS) __attribute__((__nonnull__ PARAMS))
#else
    /**
     * @def MSTL_PURE_FUNCTION
     * @brief 空定义
     */
    #define MSTL_PURE_FUNCTION

    /**
     * @def MSTL_MALLOC_FUNCTION
     * @brief 空定义
     */
    #define MSTL_MALLOC_FUNCTION

    /**
     * @def MSTL_CONST_FUNCTION
     * @brief 空定义
     */
    #define MSTL_CONST_FUNCTION

    /**
     * @def MSTL_NOTNULL_FUNCTION
     * @brief 空定义
     * @param PARAMS 参数（未使用）
     */
    #define MSTL_NOTNULL_FUNCTION(PARAMS)
#endif


#ifdef MSTL_STANDARD_14__
    /**
     * @def MSTL_DEPRECATED
     * @brief 弃用标记
     */
    #define MSTL_DEPRECATED [[deprecated]]

    /**
     * @def MSTL_DEPRECATE_FOR
     * @brief 带消息的弃用标记
     * @param MSG 弃用消息
     */
    #define MSTL_DEPRECATE_FOR(MSG) [[deprecated(MSG)]]

    /**
     * @def MSTL_FUNC_ADAPTER_DEPRECATE
     * @brief 函数适配器弃用消息
     */
    #define MSTL_FUNC_ADAPTER_DEPRECATE \
    MSTL_DEPRECATE_FOR("C++ 11 and later versions no longer use functor base types and functor adapters.")

    /**
     * @def MSTL_TRAITS_DEPRECATE
     * @brief 迭代器特性弃用消息
     */
    #define MSTL_TRAITS_DEPRECATE \
    MSTL_DEPRECATE_FOR("C++ 11 and later versions no longer use iterator traits functions.")
#else
    /**
     * @def MSTL_DEPRECATED
     * @brief 空定义
     */
    #define MSTL_DEPRECATED

    /**
     * @def MSTL_DEPRECATE_FOR
     * @brief 空定义
     * @param MSG 消息（未使用）
     */
    #define MSTL_DEPRECATE_FOR(MSG)

    /**
     * @def MSTL_FUNC_ADAPTER_DEPRECATE
     * @brief 空定义
     */
    #define MSTL_FUNC_ADAPTER_DEPRECATE

    /**
     * @def MSTL_TRAITS_DEPRECATE
     * @brief 空定义
     */
    #define MSTL_TRAITS_DEPRECATE
#endif


#if defined(MSTL_COMPILER_GNUC__)
    /**
     * @def MSTL_NOVTABLE
     * @brief 无虚函数表属性
     */
    #define MSTL_NOVTABLE __attribute__((novtable))
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_NOVTABLE
     * @brief 无虚函数表属性
     */
    #define MSTL_NOVTABLE __declspec(novtable)
#else
    /**
     * @def MSTL_NOVTABLE
     * @brief 空定义
     */
    #define MSTL_NOVTABLE
#endif


#if defined(MSTL_COMPILER_GNUC__)
    /**
     * @def MSTL_ALLOC_OPTIMIZE
     * @brief 分配器优化标记
     */
    #define MSTL_ALLOC_OPTIMIZE MSTL_ALWAYS_INLINE
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_ALLOC_OPTIMIZE
     * @brief 分配器优化标记
     */
    #define MSTL_ALLOC_OPTIMIZE __declspec(allocator)
#else
    /**
     * @def MSTL_ALLOC_OPTIMIZE
     * @brief 空定义
     */
    #define MSTL_ALLOC_OPTIMIZE
#endif


#if defined(MSTL_COMPILER_GNUC__)
    /**
     * @def MSTL_RESTRICT
     * @brief 限制指针别名
     */
    #define MSTL_RESTRICT __restrict__
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_RESTRICT
     * @brief 限制指针别名
     */
    #define MSTL_RESTRICT __restrict
#else
    /**
     * @def MSTL_RESTRICT
     * @brief 限制指针别名（C标准）
     */
    #define MSTL_RESTRICT restrict
#endif


#ifdef MSTL_COMPILER_GNUC__
    /**
     * @def MSTL_UNREACHABLE
     * @brief 不可达代码标记
     */
    #define MSTL_UNREACHABLE __builtin_unreachable()
#elif defined(MSTL_COMPILER_MSVC__)
    /**
     * @def MSTL_UNREACHABLE
     * @brief 不可达代码标记
     */
    #define MSTL_UNREACHABLE __assume(false)
#else
    /**
     * @def MSTL_UNREACHABLE
     * @brief 空操作
     */
    #define MSTL_UNREACHABLE ((void)0)
#endif


#ifdef MSTL_STANDARD_20__
    /**
     * @def MSTL_NO_UNIQUE_ADDRESS
     * @brief 无唯一地址属性
     */
    #define MSTL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
    /**
     * @def MSTL_NO_UNIQUE_ADDRESS
     * @brief 空定义
     */
    #define MSTL_NO_UNIQUE_ADDRESS
#endif

/** @} */ // Attributes

/**
 * @defgroup TypeMacros 类型宏
 * @brief 类型相关的宏展开
 * @{
 */

/**
 * @def MSTL_MACRO_RANGE_BASIC_CHARS
 * @brief 展开基本字符类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(char) \
	MAC(signed char) \
	MAC(unsigned char) \

#ifdef MSTL_STANDARD_20__
/**
 * @def MSTL_MACRO_RANGES_UNICODE_CHARS
 * @brief 展开Unicode字符类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGES_UNICODE_CHARS(MAC) \
	MAC(char8_t) \
	MAC(char16_t) \
	MAC(char32_t)
#else
/**
 * @def MSTL_MACRO_RANGES_UNICODE_CHARS
 * @brief 展开Unicode字符类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGES_UNICODE_CHARS(MAC) \
	MAC(char16_t) \
	MAC(char32_t)
#endif

/**
 * @def MSTL_MACRO_RANGE_CHARS
 * @brief 展开所有字符类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGE_CHARS(MAC) \
	MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(wchar_t) \
	MSTL_MACRO_RANGES_UNICODE_CHARS(MAC)

/**
 * @def MSTL_MACRO_RANGE_SINT
 * @brief 展开有符号整数类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGE_SINT(MAC) \
	MAC(short) \
	MAC(int) \
	MAC(long) \
	MAC(long long)

/**
 * @def MSTL_MACRO_RANGE_USINT
 * @brief 展开无符号整数类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGE_USINT(MAC) \
	MAC(unsigned short) \
	MAC(unsigned int) \
	MAC(unsigned long) \
	MAC(unsigned long long)

/**
 * @def MSTL_MACRO_RANGE_INT
 * @brief 展开所有整数类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGE_INT(MAC) \
	MSTL_MACRO_RANGE_SINT(MAC) \
	MSTL_MACRO_RANGE_USINT(MAC)

/**
 * @def MSTL_MACRO_RANGE_FLOAT
 * @brief 展开浮点数类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGE_FLOAT(MAC) \
	MAC(float) \
	MAC(double) \
	MAC(long double)

/**
 * @def MSTL_MACRO_RANGES_ALL
 * @brief 展开所有基本类型
 * @param MAC 宏处理器
 */
#define MSTL_MACRO_RANGES_ALL(MAC) \
	MSTL_MACRO_RANGE_CHARS(MAC) \
	MSTL_MACRO_RANGE_INT(MAC) \
	MSTL_MACRO_RANGE_FLOAT(MAC)

/** @} */ // TypeMacros


/**
 * @def MSTL_IGNORE
 * @brief 忽略表达式结果
 */
#define MSTL_IGNORE (void)

#endif // MSTL_CORE_CONFIG_CPPCONFIG_HPP__
