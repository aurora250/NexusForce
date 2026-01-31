#ifndef MSTL_CORE_CONFIG_CPPCONFIG_HPP__
#define MSTL_CORE_CONFIG_CPPCONFIG_HPP__

/**
 * @file c++config.hpp
 * @brief MSTL核心配置
 *
 * 此头文件定义了整个库的平台、编译器和语言特性的配置宏
 *
 * 项目内部使用的宏将不写入文档，具体您可以查看本文件内容
 */

#include "MSTL/core/config/undef_cmacro.hpp"
#include <assert.h>

/**
 * @defgroup PlatformDetection 平台检测
 * @brief 检测和定义目标平台的宏
 * @{
 */

#if defined(WIN32) || defined(_WIN32) || defined(_M_X86) || defined(MSTL_DOXYGEN_GENERATE)

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

	#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(MSTL_DOXYGEN_GENERATE)
        /**
         * @def MSTL_PLATFORM_WIN64__
         * @brief 定义在64位Windows平台编译
         */
		#define MSTL_PLATFORM_WIN64__	1
	#endif
#endif

#if defined(__linux__) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_PLATFORM_LINUX__
     * @brief 定义在Linux平台编译
     */
	#define MSTL_PLATFORM_LINUX__		1

	#if (__WORDSIZE == 64) || (__SIZEOF_POINTER__ == 8) || defined(MSTL_DOXYGEN_GENERATE)
        /**
         * @def MSTL_PLATFORM_LINUX64__
         * @brief 定义在64位Linux平台编译
         */
		#define MSTL_PLATFORM_LINUX64__ 1
    #endif

	#if (__WORDSIZE == 32) || (__SIZEOF_POINTER__ == 4) || defined(MSTL_PLATFORM_LINUX64__) || defined(MSTL_DOXYGEN_GENERATE)
        /**
         * @def MSTL_PLATFORM_LINUX32__
         * @brief 定义在32位Linux平台编译
         */
		#define MSTL_PLATFORM_LINUX32__ 1
	#endif
#endif

#if !(defined(MSTL_PLATFORM_WINDOWS__) || defined(MSTL_PLATFORM_LINUX__))
#error "MSTL: 不支持的操作系统"
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


#if defined(__GNUC__) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_COMPILER_GNUC__
     * @brief 定义使用GNU编译器编译
     */
	#define MSTL_COMPILER_GNUC__		1

	#if defined(__clang__) || defined(MSTL_DOXYGEN_GENERATE)
        /**
         * @def MSTL_COMPILER_CLANG__
         * @brief 定义使用Clang编译器编译
         */
		#define MSTL_COMPILER_CLANG__	1
    #endif

	#if !defined(MSTL_COMPILER_CLANG__) || defined(MSTL_DOXYGEN_GENERATE)
        /**
         * @def MSTL_COMPILER_GCC__
         * @brief 定义使用GCC编译器编译
         */
		#define MSTL_COMPILER_GCC__		1
	#endif
#endif

#if defined(_MSC_VER) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_COMPILER_MSVC__
     * @brief 定义使用Microsoft Visual C++编译器编译
     */
	#define MSTL_COMPILER_MSVC__		1
#endif

#if !(defined(MSTL_COMPILER_GNUC__) || defined(MSTL_COMPILER_MSVC__))
#error "MSTL: 不支持的编译器"
#endif

/** @} */ // CompilerDetection

/**
 * @defgroup APIImpExpSpec API导入导出规范
 * @brief 动态库导入导出声明
 * @{
 */

#if defined(MSTL_COMPILER_MSVC__) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_API_EXPORT_DLL
     * @brief 在MSVC编译器下使用DLL导出
     */
    #define MSTL_API_EXPORT_DLL __declspec(dllexport)
    /**
     * @def MSTL_API_IMPORT_DLL
     * @brief 在MSVC编译器下使用DLL导入
     */
    #define MSTL_API_IMPORT_DLL __declspec(dllimport)
#endif

#if defined(MSTL_COMPILER_GNUC__) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_API_EXPORT
     * @brief 在GNUC编译器下使用空定义，无需显式的导入导出辅助
     */
    #define MSTL_API_EXPORT
#endif

#if defined(MSTL_COMPILER_GNUC__)
    #define MSTL_API MSTL_API_EXPORT
#else
    #if defined(MSTL_DLLEXPORTS)
        #define MSTL_API MSTL_API_EXPORT_DLL
    #else
        #define MSTL_API MSTL_API_IMPORT_DLL
    #endif
#endif

/** @} */ // APIImpExpSpec


#if defined(__i386__) || defined(__i386) || defined(_M_IX86) || \
    defined(__X86__) || defined(_X86_) || defined(__I86__)
    #define MSTL_ARCH_X86_32__ 1
#endif

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || \
    defined(__amd64__) || defined(__amd64) || defined(_M_AMD64)
    #define MSTL_ARCH_X86_64__ 1
#endif

#if defined(MSTL_ARCH_X86_32__) || defined(MSTL_ARCH_X86_64__)
    #define MSTL_ARCH_X86__ 1
#endif


#if defined(__arm__) || defined(__arm) || defined(_ARM_) || \
    defined(_M_ARM) || defined(__ARM_ARCH) || defined(__TARGET_ARCH_ARM)
    #define MSTL_ARCH_ARM32__ 1
#endif

#if defined(__aarch64__) || defined(__aarch64) || defined(_M_ARM64) || \
    defined(__ARM64_ARCH_8__) || defined(__ARM_ARCH_ISA_A64)
    #define MSTL_ARCH_AARCH64__ 1
#endif

#if defined(MSTL_ARCH_ARM32__) || defined(MSTL_ARCH_AARCH64__)
    #define MSTL_ARCH_ARM__ 1
#endif


#if defined(__riscv) || defined(__riscv__) || defined(riscv)
    #define MSTL_ARCH_RISCV__ 1
    #if __riscv_xlen == 32
        #define MSTL_ARCH_RISCV32__ 1
    #elif __riscv_xlen == 64
        #define MSTL_ARCH_RISCV64__ 1
    #endif
#endif


#if defined(__loongarch__) || defined(__loongarch) || \
    defined(__loongarch32) || defined(__loongarch64) || \
    defined(_LOONGARCH_SIM) || defined(_LOONGARCH)
    #define MSTL_ARCH_LOONGARCH__ 1
    #if defined(__loongarch32) || defined(_LOONGARCH_SIM == _ABILP32_SIM)
        #define MSTL_ARCH_LOONGARCH32__ 1
    #elif defined(__loongarch64) || defined(_LOONGARCH_SIM == _ABILP64_SIM)
        #define MSTL_ARCH_LOONGARCH64__ 1
    #endif
#endif

/**
 * @defgroup DataBusWidth 数据总线宽度
 * @brief 系统架构位宽检测
 * @{
 */

#if defined(MSTL_ARCH_X86_64__) || defined(MSTL_ARCH_AARCH64__) || defined(MSTL_ARCH_RISCV64__) \
    || defined(MSTL_ARCH_LOONGARCH64__) || defined(MSTL_DOXYGEN_GENERATE)
	/**
     * @def MSTL_DATA_BUS_WIDTH_64__
     * @brief 定义在64位系统编译
     */
	#define MSTL_DATA_BUS_WIDTH_64__	1
#endif
#if defined(MSTL_ARCH_X86_32__) || defined(MSTL_ARCH_ARM32__) || defined(MSTL_ARCH_RISCV32__) \
    || defined(MSTL_ARCH_LOONGARCH32__) || defined(MSTL_DOXYGEN_GENERATE)
	/**
     * @def MSTL_DATA_BUS_WIDTH_32__
     * @brief 定义在32位系统编译
     */
	#define MSTL_DATA_BUS_WIDTH_32__	1
#endif

#if !(defined(MSTL_DATA_BUS_WIDTH_64__) || defined(MSTL_DATA_BUS_WIDTH_32__))
#error "MSTL: 不支持的架构"
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

#if (__cplusplus >= 202100L) || (_MSVC_LANG >= 202100L) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_STANDARD_23__
     * @brief 使用C++23或更高标准编译
     */
	#define MSTL_STANDARD_23__	1
#endif
#if (__cplusplus >= 202002L) || defined(MSTL_STANDARD_23__) || (_MSVC_LANG >= 202002L) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_STANDARD_20__
     * @brief 使用C++20或更高标准编译
     */
	#define MSTL_STANDARD_20__	1
#endif
#if (__cplusplus >= 201703L) || defined(MSTL_STANDARD_20__) || (_MSVC_LANG >= 201703L) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_STANDARD_17__
     * @brief 使用C++17或更高标准编译
     */
	#define MSTL_STANDARD_17__	1
#endif
#if (__cplusplus >= 201402L) || defined(MSTL_STANDARD_17__) || (_MSVC_LANG >= 201402L) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_STANDARD_14__
     * @brief 使用C++14或更高标准编译
     */
	#define MSTL_STANDARD_14__	1
#endif
#if (__cplusplus >= 201103L) || defined(MSTL_STANDARD_14__) || (_MSVC_LANG >= 201103L) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_STANDARD_11__
     * @brief 使用C++11或更高标准编译
     */
	#define MSTL_STANDARD_11__	1
#endif
#if (__cplusplus >= 199711L) || defined(MSTL_STANDARD_11__) || (_MSVC_LANG >= 199711L) || defined(MSTL_DOXYGEN_GENERATE)
    /**
     * @def MSTL_STANDARD_98__
     * @brief 使用C++98或更高标准编译
     */
	#define MSTL_STANDARD_98__	1
#endif

/** @} */ // CxxStandardDetection


#ifdef MSTL_STANDARD_17__
	#define MSTL_SUPPORT_DEDUCTION_GUIDES__ 1
#endif


#ifdef MSTL_STANDARD_11__
	#define MSTL_CONSTEXPR11 constexpr
#else
	#define MSTL_CONSTEXPR11 inline
#endif // MSTL_STANDARD_11__

#ifdef MSTL_STANDARD_14__
    #define MSTL_CONSTEXPR14 constexpr
#else
    #define MSTL_CONSTEXPR14 inline
#endif // MSTL_STANDARD_14__

#ifdef MSTL_STANDARD_17__
    #define MSTL_CONSTEXPR17 constexpr
    #define MSTL_INLINE17 inline
#else
    #define MSTL_CONSTEXPR17 inline
    #define MSTL_INLINE17
#endif // MSTL_STANDARD_17__

#ifdef MSTL_STANDARD_20__
    #define MSTL_CONSTEXPR20 constexpr
#else
    #define MSTL_CONSTEXPR20 inline
#endif // MSTL_STANDARD_20__

#ifdef MSTL_STANDARD_23__
    #define MSTL_CONSTEXPR23 constexpr
#else
    #define MSTL_CONSTEXPR23 inline
#endif // MSTL_STANDARD_23__


#ifdef MSTL_STANDARD_17__
    #define MSTL_IF_CONSTEXPR if constexpr
#else
    #define MSTL_IF_CONSTEXPR if
#endif


#ifdef MSTL_STANDARD_17__
    #define MSTL_NODISCARD [[nodiscard]]

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


#ifdef MSTL_DOXYGEN_GENERATE
    #define MSTL_ALWAYS_INLINE inline
    #define MSTL_ALWAYS_INLINE_INLINE inline
#elif defined(MSTL_COMPILER_GNUC__)
    #define MSTL_ALWAYS_INLINE __attribute__((always_inline))
    #define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE inline
#elif defined(MSTL_COMPILER_MSVC__)
    #define MSTL_ALWAYS_INLINE __forceinline
    #define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE
#else
    #define MSTL_ALWAYS_INLINE inline
    #define MSTL_ALWAYS_INLINE_INLINE inline
#endif


#ifdef MSTL_COMPILER_GNUC__
    #define MSTL_UNUSED __attribute__((unused))
#else
    #define MSTL_UNUSED
#endif


#ifdef MSTL_STANDARD_17__
    #define MSTL_UNLIKELY [[unlikely]]
#else
    #define MSTL_UNLIKELY
#endif


#ifdef MSTL_STANDARD_20__
    #define MSTL_LIKELY [[likely]]
#else
    #define MSTL_LIKELY
#endif


#if defined(MSTL_STANDARD_11__)
    #define MSTL_NORETURN [[noreturn]]
#elif defined(MSTL_COMPILER_GNUC__)
    #define MSTL_NORETURN __attribute__((noreturn))
#elif defined(MSTL_COMPILER_MSVC__)
    #define MSTL_NORETURN __declspec(noreturn)
#else
    #define MSTL_NORETURN
#endif


#if !defined(MSTL_COMPILER_GNUC__) || defined(MSTL_DOXYGEN_GENERATE)
    #define MSTL_PURE_FUNCTION
    #define MSTL_MALLOC_FUNCTION
    #define MSTL_CONST_FUNCTION
    #define MSTL_NOTNULL_FUNCTION(PARAMS)
#else
    #define MSTL_PURE_FUNCTION __attribute__((__pure__))
    #define MSTL_MALLOC_FUNCTION __attribute__((__malloc__))
    #define MSTL_CONST_FUNCTION __attribute__((__const__))
    #define MSTL_NONNULL_FUNCTION(PARAMS) __attribute__((__nonnull__ PARAMS))
#endif


#ifdef MSTL_STANDARD_14__
    #define MSTL_DEPRECATED [[deprecated]]
    #define MSTL_DEPRECATE_FOR(MSG) [[deprecated(MSG)]]

    #define MSTL_FUNC_ADAPTER_DEPRECATE \
    MSTL_DEPRECATE_FOR("C++ 11 and later versions no longer use functor base types and functor adapters.")

    #define MSTL_TRAITS_DEPRECATE \
    MSTL_DEPRECATE_FOR("C++ 11 and later versions no longer use iterator traits functions.")
#else
    #define MSTL_DEPRECATED
    #define MSTL_DEPRECATE_FOR(MSG)
    #define MSTL_FUNC_ADAPTER_DEPRECATE
    #define MSTL_TRAITS_DEPRECATE
#endif


#if defined(MSTL_COMPILER_GNUC__)
    #define MSTL_NOVTABLE __attribute__((novtable))
#elif defined(MSTL_COMPILER_MSVC__)
    #define MSTL_NOVTABLE __declspec(novtable)
#else
    #define MSTL_NOVTABLE
#endif


#if defined(MSTL_COMPILER_GNUC__)
    #define MSTL_ALLOC_OPTIMIZE MSTL_ALWAYS_INLINE
#elif defined(MSTL_COMPILER_MSVC__)
    #define MSTL_ALLOC_OPTIMIZE __declspec(allocator)
#else
    #define MSTL_ALLOC_OPTIMIZE
#endif


#if defined(MSTL_COMPILER_GNUC__)
    #define MSTL_RESTRICT __restrict__
#else
    #define MSTL_RESTRICT __restrict
#endif


#ifdef MSTL_COMPILER_GNUC__
    #define MSTL_UNREACHABLE __builtin_unreachable()
#elif defined(MSTL_COMPILER_MSVC__)
    #define MSTL_UNREACHABLE __assume(false)
#else
    #define MSTL_UNREACHABLE ((void)0)
#endif


#ifdef MSTL_STANDARD_20__
    #define MSTL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
    #define MSTL_NO_UNIQUE_ADDRESS
#endif


#define MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(char) \
	MAC(signed char) \
	MAC(unsigned char) \

#if defined(MSTL_STANDARD_20__) || defined(MSTL_DOXYGEN_GENERATE)

#define MSTL_MACRO_RANGES_UNICODE_CHARS(MAC) \
	MAC(char8_t) \
	MAC(char16_t) \
	MAC(char32_t)
#else
#define MSTL_MACRO_RANGES_UNICODE_CHARS(MAC) \
	MAC(char16_t) \
	MAC(char32_t)
#endif

#define MSTL_MACRO_RANGE_CHARS(MAC) \
	MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(wchar_t) \
	MSTL_MACRO_RANGES_UNICODE_CHARS(MAC)

#define MSTL_MACRO_RANGE_SINT(MAC) \
	MAC(short) \
	MAC(int) \
	MAC(long) \
	MAC(long long)

#define MSTL_MACRO_RANGE_USINT(MAC) \
	MAC(unsigned short) \
	MAC(unsigned int) \
	MAC(unsigned long) \
	MAC(unsigned long long)

#define MSTL_MACRO_RANGE_INT(MAC) \
	MSTL_MACRO_RANGE_SINT(MAC) \
	MSTL_MACRO_RANGE_USINT(MAC)

#define MSTL_MACRO_RANGE_FLOAT(MAC) \
	MAC(float) \
	MAC(double) \
	MAC(long double)

#define MSTL_MACRO_RANGES_ALL(MAC) \
	MSTL_MACRO_RANGE_CHARS(MAC) \
	MSTL_MACRO_RANGE_INT(MAC) \
	MSTL_MACRO_RANGE_FLOAT(MAC)


#define MSTL_MACRO_RANGES_CV(MAC) \
	MAC(const) \
	MAC(volatile) \
	MAC(const volatile)

#define MSTL_MACRO_RANGES_CV_REF(MAC) \
    MAC(&) \
	MAC(const &) \
	MAC(volatile &) \
	MAC(const volatile &) \
    MAC(&&) \
	MAC(const &&) \
	MAC(volatile &&) \
	MAC(const volatile &&)

#define MSTL_MACRO_RANGES_CV_REF_NOEXCEPT(MAC) \
    MAC(noexcept) \
    MAC(const noexcept) \
    MAC(volatile noexcept) \
    MAC(const volatile noexcept) \
	MAC(& noexcept) \
	MAC(const & noexcept) \
	MAC(volatile & noexcept) \
	MAC(const volatile & noexcept) \
	MAC(&& noexcept) \
	MAC(const && noexcept) \
	MAC(volatile && noexcept) \
	MAC(const volatile && noexcept) \


#define MSTL_IGNORE (void)


#ifdef MSTL_STATE_DEBUG__
#define MSTL_DEBUG_VERIFY(CON, MESG) \
    { if (CON) {} else { assert(false && MESG); } }
#else
#define MSTL_DEBUG_VERIFY(CON, MESG)
#endif

#define __MSTL_DEBUG_MESG_OPERATE_NULLPTR(ITER, ACT) "can`t " ACT ": " #ITER " is pointing to nullptr."
#define __MSTL_DEBUG_MESG_OUT_OF_RANGE(CLASS, ACT) "can`t " ACT ": " #CLASS " out of ranges."
#define __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(ITER) "not comparable :" #ITER " container incompatible."

#define __MSTL_DEBUG_TAG_DEREFERENCE "dereference"
#define __MSTL_DEBUG_TAG_INCREMENT "increment"
#define __MSTL_DEBUG_TAG_DECREMENT "decrement"


#if defined(MSTL_STANDARD_20__)
#define MSTL_CONSTEXPR_ASSERT(COND) \
do { \
    if (__builtin_is_constant_evaluated() && !bool(COND)) \
        MSTL_UNREACHABLE; \
} while (false);
#elif defined(MSTL_STATE_DEBUG__)
#define MSTL_CONSTEXPR_ASSERT(COND) \
do { \
    if (!bool(COND)) \
        assert(false); \
} while (false);
#else
#define MSTL_CONSTEXPR_ASSERT(COND)
#endif

#endif // MSTL_CORE_CONFIG_CPPCONFIG_HPP__
