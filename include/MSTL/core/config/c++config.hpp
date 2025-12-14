#ifndef MSTL_CORE_CONFIG_CPPCONFIG_HPP__
#define MSTL_CORE_CONFIG_CPPCONFIG_HPP__
#include "undef_cmacro.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(_M_X86)
	// defined when project compiled in windows, whether in 32bits or 64bits.
	#define MSTL_PLATFORM_WINDOWS__		1
	// defined when project compiled in windows of 32bits system.
	#define MSTL_PLATFORM_WIN32__		1
	#if defined(WIN64) || defined(_WIN64) || defined(_WIN64_) || defined(_M_X64)
		// defined when project compiled in windows of 64bits system.
		#define MSTL_PLATFORM_WIN64__	1
	#endif
#elif defined(__linux__)
	// defined when project compiled in linux, whether in 32bits or 64bits.
	#define MSTL_PLATFORM_LINUX__		1
	#if (__WORDSIZE == 64) || (__SIZEOF_POINTER__ == 8)
		// defined when project compiled in linux of 64bits system.
		#define MSTL_PLATFORM_LINUX64__ 1
	#elif (__WORDSIZE == 32) || (__SIZEOF_POINTER__ == 4) || defined(MSTL_PLATFORM_LINUX64__)
		// defined when project compiled in linux of 32bits system.
		#define MSTL_PLATFORM_LINUX32__ 1
	#endif
#else
	// defined when project compiled in not supported systems.
	#define MSTL_PLATFORM_UNSUPPORT__	1
#endif


#ifdef MSTL_PLATFORM_WINDOWS__
#define WIN32_LEAN_AND_MEAN
#endif


#if defined(__GNUC__)
	// defined when project compiled by gnuc compilers.
	#define MSTL_COMPILER_GNUC__		1
	#if defined(__clang__)
		// defined when project compiled by clang compiler.
		#define MSTL_COMPILER_CLANG__	1
	#else
		// defined when project compiled by gcc compilers.
		#define MSTL_COMPILER_GCC__		1
	#endif
#elif defined(_MSC_VER)
	// defined when project compiled by msvc compilers.
	#define MSTL_COMPILER_MSVC__		1
#else
	// defined when project compiled by not supported compilers.
	#define MSTL_COMPILER_UNSUPPORT__	1
#endif


#ifdef MSTL_COMPILER_MSVC__
    #ifdef MSTL_DLLEXPORTS
        #define MSTL_API __declspec(dllexport)
    #else
        #define MSTL_API __declspec(dllimport)
    #endif
#else
    #define MSTL_API
#endif


#ifdef MSTL_COMPILER_MSVC__
	#define MSTL_THREAD_LOCAL __declspec(thread)
#elif defined(MSTL_COMPILER_GNUC__)
	#define MSTL_THREAD_LOCAL thread_local
#else
	#define MSTL_THREAD_LOCAL
#endif


#if defined(MSTL_PLATFORM_WIN64__) || defined(MSTL_PLATFORM_LINUX64__) || defined(__amd64__) || defined(__x86_64__) || defined(__aarch64__)
	// defined when project compiled in 64bits systems.
	#define MSTL_DATA_BUS_WIDTH_64__	1
#endif
#if defined(MSTL_PLATFORM_WIN32__) || defined(MSTL_PLATFORM_LINUX32__) || defined(__i386__)
	// defined when project compiled in 32bits systems.
	#define MSTL_DATA_BUS_WIDTH_32__	1
#endif


#define __MSTL_GLOBAL_NAMESPACE__ MSTL
#define MSTL_BEGIN_NAMESPACE__ namespace __MSTL_GLOBAL_NAMESPACE__ {
#define MSTL_END_NAMESPACE__ }
#define _MSTL __MSTL_GLOBAL_NAMESPACE__ ::

#define __MSTL_INNER_NAMESPACE__ inner
#define MSTL_BEGIN_INNER__ namespace __MSTL_INNER_NAMESPACE__ {
#define MSTL_END_INNER__ }
#define _INNER __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_INNER_NAMESPACE__ ::

#define __MSTL_CONSTANTS_NAMESPACE__ constants
#define MSTL_BEGIN_CONSTANTS__ namespace __MSTL_CONSTANTS_NAMESPACE__ {
#define MSTL_END_CONSTANTS__ }
#define _CONSTANTS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_CONSTANTS_NAMESPACE__ ::

#define __MSTL_CHRONO_NAMESPACE__ chrono
#define MSTL_BEGIN_CHRONO__ namespace __MSTL_CHRONO_NAMESPACE__ {
#define MSTL_END_CHRONO__ }
#define _MSTL_CHRONO __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_CHRONO_NAMESPACE__ ::

#define __MSTL_THIS_THREAD_NAMESPACE__ this_thread
#define MSTL_BEGIN_THIS_THREAD__ namespace __MSTL_THIS_THREAD_NAMESPACE__ {
#define MSTL_END_THIS_THREAD__ }
#define _THIS_THREAD __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_THIS_THREAD_NAMESPACE__ ::

#define __MSTL_RANGES_NAMESPACE__ ranges
#define MSTL_BEGIN_RANGES__ namespace __MSTL_RANGES_NAMESPACE__ {
#define MSTL_END_RANGES__ }
#define _MSTL_RANGES __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_RANGES_NAMESPACE__ ::

#define __MSTL_RANGES_VIEWS_NAMESPACE__ views
#define MSTL_BEGIN_RANGES_VIEWS__ namespace __MSTL_RANGES_VIEWS_NAMESPACE__ {
#define MSTL_END_RANGES_VIEWS__ }
#define _RANGES_VIEWS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_RANGES_NAMESPACE__ :: __MSTL_RANGES_VIEWS_NAMESPACE__ ::

#define __MSTL_LITERALS_NAMESPACE__ literals
#define MSTL_BEGIN_LITERALS__ inline namespace __MSTL_LITERALS_NAMESPACE__ {
#define MSTL_END_LITERALS__ }
#define _LITERALS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_LITERALS_NAMESPACE__ ::

#define __MSTL_TAG_NAMESPACE__ tags
#define MSTL_BEGIN_TAG__ inline namespace __MSTL_TAG_NAMESPACE__ {
#define MSTL_END_TAG__ }
#define _MSTL_TAG __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_TAG_NAMESPACE__ ::

#ifdef MSTL_SUPPORT_POSTGRESQL__
#define __MSTL_POSTGRESQL_NAMESPACE__ postgresql
#define MSTL_BEGIN_POSTGRESQL__ namespace __MSTL_POSTGRESQL_NAMESPACE__ {
#define MSTL_END_POSTGRESQL__ }
#define _MSTL_POSTGRESQL __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_POSTGRESQL_NAMESPACE__ ::
#endif

#ifdef MSTL_SUPPORT_MYSQL__
#define __MSTL_MYSQL_NAMESPACE__ mysql
#define MSTL_BEGIN_MYSQL__ namespace __MSTL_MYSQL_NAMESPACE__ {
#define MSTL_END_MYSQL__ }
#define _MSTL_MYSQL __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_MYSQL_NAMESPACE__ ::
#endif

#ifdef MSTL_SUPPORT_SQLITE3__
#define __MSTL_SQLITE_NAMESPACE__ sqlite
#define MSTL_BEGIN_SQLITE__ namespace __MSTL_SQLITE_NAMESPACE__ {
#define MSTL_END_SQLITE__ }
#define _MSTL_SQLITE __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_SQLITE_NAMESPACE__ ::
#endif

#ifdef MSTL_SUPPORT_REDIS__
#define __MSTL_REDIS_NAMESPACE__ redis
#define MSTL_BEGIN_REDIS__ namespace __MSTL_REDIS_NAMESPACE__ {
#define MSTL_END_REDIS__ }
#define _MSTL_REDIS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_REDIS_NAMESPACE__ ::
#endif


#if _HAS_CXX23 || (__cplusplus >= 202100L) || (_MSVC_LANG >= 202100L)
	// defined when project compiled by using C++23 or upper version of standard library.
	#define MSTL_STANDARD_23__	1
#endif
#if _HAS_CXX20 || (__cplusplus >= 202002L) || (_MSVC_LANG >= 202002L)
	// defined when project compiled by using C++20 or upper version of standard library.
	#define MSTL_STANDARD_20__	1
#endif
#if _HAS_CXX17 || (__cplusplus >= 201703L) || defined(MSTL_STANDARD_20__) || (_MSVC_LANG >= 201703L)
	// defined when project compiled by using C++17 or upper version of standard library.
	#define MSTL_STANDARD_17__	1
#endif
#if (__cplusplus >= 201402L) || defined(MSTL_STANDARD_17__) || (_MSVC_LANG >= 201402L)
	// defined when project compiled by using C++14 or upper version of standard library.
	#define MSTL_STANDARD_14__	1
#endif
#if (__cplusplus >= 201103L) || defined(MSTL_STANDARD_14__) || (_MSVC_LANG >= 201103L)
	// defined when project compiled by using C++11 or upper version of standard library.
	#define MSTL_STANDARD_11__	1
#endif
#if (__cplusplus >= 199711L) || defined(MSTL_STANDARD_11__) || (_MSVC_LANG >= 199711L)
	// defined when project compiled by using C++98 or upper version of standard library.
	#define MSTL_STANDARD_98__	1
#endif


#ifdef MSTL_STANDARD_17__
	#define MSTL_SUPPORT_DEDUCTION_GUIDES__ 1
#endif


#ifdef MSTL_STANDARD_11__
	#define MSTL_CONSTEXPR11 constexpr
#else
	#define MSTL_CONSTEXPR11 inline
#endif // MSTL_STANDARD_11__

#ifdef MSTL_STANDARD_14__
	#define MSTL_CONSTEXPR14 MSTL_CONSTEXPR11
#else
	#define MSTL_CONSTEXPR14 inline
#endif // MSTL_STANDARD_14__

#ifdef MSTL_STANDARD_17__
	#define MSTL_CONSTEXPR17 MSTL_CONSTEXPR14
	#define MSTL_INLINE17 inline
#else
	#define MSTL_CONSTEXPR17 inline
	#define MSTL_INLINE17
#endif // MSTL_STANDARD_17__

#ifdef MSTL_STANDARD_20__
	#define MSTL_CONSTEXPR20 MSTL_CONSTEXPR17
#else
	#define MSTL_CONSTEXPR20 inline
#endif // MSTL_STANDARD_20__

#ifdef MSTL_STANDARD_23__
	#define MSTL_CONSTEXPR23 MSTL_CONSTEXPR20
#else
	#define MSTL_CONSTEXPR23 inline
#endif // MSTL_STANDARD_23__


#ifdef MSTL_STANDARD_17__
	#define MSTL_IF_CONSTEXPR if constexpr
#else
	// this macro will be used with caution, as it may break static overload under C++17.
	#define MSTL_IF_CONSTEXPR if
#endif


#ifdef MSTL_STANDARD_20__
	#define MSTL_CONSTEVAL consteval
#else
	#define MSTL_CONSTEVAL MSTL_CONSTEXPR
#endif


#ifdef MSTL_STANDARD_17__
	#define MSTL_NODISCARD [[nodiscard]]
	#define MSTL_ALLOC_NODISCARD \
		[[nodiscard("discard the return of allocators will cause memory leaks.")]]
#else
	#define MSTL_NODISCARD
	#define MSTL_ALLOC_NODISCARD
#endif


#ifdef MSTL_COMPILER_GNUC__
	#define MSTL_ALIGNOF_DEFAULT() __attribute__((__aligned__))
	#define MSTL_ALIGNOF(ALIGN) __attribute__((__aligned__((ALIGN))))
#elif defined(MSTL_COMPILER_MSVC__) && defined(MSTL_STANDARD_11__)
	#define MSTL_ALIGNOF_DEFAULT() [[aligned]]
	#define MSTL_ALIGNOF(ALIGN) [[aligned(ALIGN)]]
#else
	#define MSTL_ALIGNOF_DEFAULT()
	#define MSTL_ALIGNOF(ALIGN)
#endif


#ifdef MSTL_COMPILER_GNUC__
	#define MSTL_ALWAYS_INLINE __attribute__((always_inline))
	#define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE inline
#elif defined(MSTL_COMPILER_MSVC__)
	#define MSTL_ALWAYS_INLINE __forceinline
	#define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE
#elif defined(MSTL_STANDARD_17__)
	#define MSTL_ALWAYS_INLINE [[always_inline]]
	#define MSTL_ALWAYS_INLINE_INLINE MSTL_ALWAYS_INLINE inline
#else
	#define MSTL_ALWAYS_INLINE
	#define MSTL_ALWAYS_INLINE_INLINE
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


#ifdef MSTL_COMPILER_GNUC__
	#define MSTL_NORETURN __attribute__((noreturn))
#elif defined(MSTL_COMPILER_MSVC__)
	#define MSTL_NORETURN __declspec(noreturn)
#elif defined(MSTL_STANDARD_11__)
	#define MSTL_NORETURN [[noreturn]]
#else
	#define MSTL_NORETURN
#endif


#ifdef MSTL_COMPILER_GNUC__
	#define MSTL_PURE_FUNCTION __attribute__((__pure__))
	#define MSTL_MALLOC_FUNCTION __attribute__((__malloc__))
	#define MSTL_CONST_FUNCTION __attribute__((__const__))
	#define MSTL_NONNULL_FUNCTION(PARAMS) __attribute__((__nonnull__ PARAMS))
#elif defined(MSTL_COMPILER_MSVC__)
	#define MSTL_PURE_FUNCTION
	#define MSTL_MALLOC_FUNCTION
	#define MSTL_CONST_FUNCTION
	#define MSTL_NOTNULL_FUNCTION(PARAMS) _Check_return_ _In_ PARAMS
#else
    #define MSTL_PURE_FUNCTION
    #define MSTL_MALLOC_FUNCTION
    #define MSTL_CONST_FUNCTION
    #define MSTL_NOTNULL_FUNCTION(PARAMS)
#endif


#ifdef MSTL_STANDARD_14__
	#define MSTL_DEPRECATED [[deprecated]]
    #define MSTL_DEPRECATE_FOR(MSG) [[deprecated(MSG)]]
	// after C++ 11, we can use lambda expressions to quickly build closures
	// instead of using functor adapters.
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


#define MSTL_RESTRICT __restrict


#ifdef MSTL_STANDARD_20__
	#define MSTL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
	#define MSTL_NO_UNIQUE_ADDRESS
#endif


#ifdef MSTL_SUPPORT_CUDA__
	#define MSTL_KERNEL __global__
	#define MSTL_FOR_GPU __device__
	// all functions are defined at host by default.
	#define MSTL_FOR_CPU __host__
	#define MSTL_FOR_ALL_DEVICES MSTL_FOR_CPU MSTL_FOR_GPU
#else
	#define MSTL_KERNEL
	#define MSTL_FOR_GPU
	#define MSTL_FOR_CPU
	#define MSTL_FOR_ALL_DEVICES
#endif


// expand macro with basic char types.
#define MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(char) \
	MAC(signed char) \
	MAC(unsigned char) \

#ifdef MSTL_STANDARD_20__
// expand macro with Unicode char types.
#define MSTL_MACRO_RANGES_UNICODE_CHARS(MAC) \
	MAC(char8_t) \
	MAC(char16_t) \
	MAC(char32_t)
#else
// expand macro with Unicode char types.
#define MSTL_MACRO_RANGES_UNICODE_CHARS(MAC) \
	MAC(char16_t) \
	MAC(char32_t)
#endif

// expand macro with all char types.
#define MSTL_MACRO_RANGE_CHARS(MAC) \
	MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(wchar_t) \
	MSTL_MACRO_RANGES_UNICODE_CHARS(MAC)

// expand macro with signed integral types.
#define MSTL_MACRO_RANGE_SINT(MAC) \
	MAC(short) \
	MAC(int) \
	MAC(long) \
	MAC(long long)

// expand macro with unsigned integral types.
#define MSTL_MACRO_RANGE_USINT(MAC) \
	MAC(unsigned short) \
	MAC(unsigned int) \
	MAC(unsigned long) \
	MAC(unsigned long long)

// expand macro with integral types.
#define MSTL_MACRO_RANGE_INT(MAC) \
	MSTL_MACRO_RANGE_SINT(MAC) \
	MSTL_MACRO_RANGE_USINT(MAC)

// expand macro with floating point types.
#define MSTL_MACRO_RANGE_FLOAT(MAC) \
	MAC(float) \
	MAC(double) \
	MAC(long double)


#define MSTL_MACRO_RANGES_ALL(MAC) \
	MSTL_MACRO_RANGE_CHARS(MAC) \
	MSTL_MACRO_RANGE_INT(MAC) \
	MSTL_MACRO_RANGE_FLOAT(MAC)

#endif // MSTL_CORE_CONFIG_CPPCONFIG_HPP__
