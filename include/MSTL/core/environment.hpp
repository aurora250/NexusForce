#ifndef MSTL_BASICLIB_HPP__
#define MSTL_BASICLIB_HPP__
#ifdef MSTL_SUPPORT_BOOST__
#include <boost/version.hpp>
#endif
#ifdef MSTL_SUPPORT_QT6__
#include <QtGlobal>
#include "undef_cmacro.hpp"
#endif

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


#if defined(_M_CEE)
	// defined when project is compiled with CRL.
	#define MSTL_COMPILE_WITH_CRL__		1
#elif defined(__EDG__)
	// defined when project is compiled with EDG.
	#define MSTL_COMPILE_WITH_EDG__		1
#elif defined(QT_VERSION)
    // defined when project is compiled with QT Framework.
    #define MSTL_COMPILE_WITH_QT__		1
#else
	// defined when project is compiled with OS.
	#define MSTL_COMPILE_WITH_OS__		1
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


#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG) || !defined(QT_NO_DEBUG)
	// defined when project compiled under debug statement.
	#define MSTL_STATE_DEBUG__			1
#else
	// defined when project compiled under release statement.
	#define MSTL_STATE_RELEASE__		1
#endif


#if defined(MSTL_PLATFORM_WIN64__) || defined(MSTL_PLATFORM_LINUX64__) || defined(__amd64__) || defined(__x86_64__) || defined(__aarch64__)
	// defined when project compiled in 64bits systems.
	#define MSTL_DATA_BUS_WIDTH_64__	1
#endif
#if defined(MSTL_PLATFORM_WIN32__) || defined(MSTL_PLATFORM_LINUX32__) || defined(__i386__)
	// defined when project compiled in 32bits systems.
	#define MSTL_DATA_BUS_WIDTH_32__	1
#endif


#ifdef MSTL_COMPILER_MSVC__
	#if defined(_M_IX86)
		#define MSTL_SUPPORT_INLINE_ASM__	1
	#endif
#elif defined(MSTL_COMPILER_GNUC__)
	#if defined(__x86_64__) || defined(__i386__)
		#define MSTL_SUPPORT_INLINE_ASM__	1
	#endif
#endif


#define __MSTL_GLOBAL_NAMESPACE__ MSTL
#define USE_MSTL using namespace __MSTL_GLOBAL_NAMESPACE__;
#define MSTL_BEGIN_NAMESPACE__ namespace __MSTL_GLOBAL_NAMESPACE__ {
#define MSTL_END_NAMESPACE__ }
#define _MSTL __MSTL_GLOBAL_NAMESPACE__ ::

#define __MSTL_INNER_NAMESPACE__ inner
#define MSTL_BEGIN_INNER__ namespace __MSTL_INNER_NAMESPACE__ {
#define MSTL_END_INNER__ }
#define _INNER __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_INNER_NAMESPACE__ ::

#define __MSTL_LITERALS_NAMESPACE__ literals
#define MSTL_BEGIN_LITERALS__ inline namespace __MSTL_LITERALS_NAMESPACE__ {
#define MSTL_END_LITERALS__ }
#define _LITERALS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_LITERALS_NAMESPACE__ ::

#define __MSTL_CONSTANTS_NAMESPACE__ constants
#define MSTL_BEGIN_CONSTANTS__ namespace __MSTL_CONSTANTS_NAMESPACE__ {
#define MSTL_END_CONSTANTS__ }
#define _CONSTANTS __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_CONSTANTS_NAMESPACE__ ::

#define __MSTL_TAG_NAMESPACE__ tags
#define MSTL_BEGIN_TAG__ inline namespace __MSTL_TAG_NAMESPACE__ {
#define MSTL_END_TAG__ }
#define _MSTL_TAG __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_TAG_NAMESPACE__ ::


#if _HAS_CXX23 || (__cplusplus >= 202100L) || (_MSVC_LANG >= 202100L)
	// defined when project compiled by using C++23 or upper version of standard library.
	#define MSTL_VERSION_23__	1
#endif
#if _HAS_CXX20 || (__cplusplus >= 202002L) || (_MSVC_LANG >= 202002L)
	// defined when project compiled by using C++20 or upper version of standard library.
	#define MSTL_VERSION_20__	1
#endif
#if _HAS_CXX17 || (__cplusplus >= 201703L) || defined(MSTL_VERSION_20__) || (_MSVC_LANG >= 201703L)
	// defined when project compiled by using C++17 or upper version of standard library.
	#define MSTL_VERSION_17__	1
#endif
#if (__cplusplus >= 201402L) || defined(MSTL_VERSION_17__) || (_MSVC_LANG >= 201402L)
	// defined when project compiled by using C++14 or upper version of standard library.
	#define MSTL_VERSION_14__	1
#endif
#if (__cplusplus >= 201103L) || defined(MSTL_VERSION_14__) || (_MSVC_LANG >= 201103L)
	// defined when project compiled by using C++11 or upper version of standard library.
	#define MSTL_VERSION_11__	1
#endif
#if (__cplusplus >= 199711L) || defined(MSTL_VERSION_11__) || (_MSVC_LANG >= 199711L)
	// defined when project compiled by using C++98 or upper version of standard library.
	#define MSTL_VERSION_98__	1
#endif


#if defined(MSTL_VERSION_11__)
	#define MSTL_SUPPORT_CONSTEXPR__		1
#endif
#if defined(MSTL_VERSION_11__)
	#define MSTL_SUPPORT_STATIC_ASSERT__	1
#endif
#if defined(MSTL_VERSION_11__)
	#define MSTL_SUPPORT_NORETURN__			1
#endif
#if defined(MSTL_VERSION_14__)
	#define MSTL_SUPPORT_DEPRECATED__		1
#endif
#if defined(MSTL_VERSION_17__)
	#define MSTL_SUPPORT_DEDUCTION_GUIDES__ 1
#endif
#if defined(MSTL_VERSION_17__)
	#define MSTL_SUPPORT_NODISCARD__		1
#endif
#if defined(MSTL_VERSION_17__)
	#define MSTL_SUPPORT_ALIGNED__		1
#endif
#if defined(MSTL_VERSION_17__)
	#define MSTL_SUPPORT_ALWAYS_INLINE__	1
#endif
#if defined(MSTL_VERSION_17__)
	#define MSTL_SUPPORT_IF_CONSTEXPR__		1
#endif
#if defined(MSTL_VERSION_20__)
	#define MSTL_SUPPORT_NO_UNIQUE_ADS__	1
#endif
#if defined(MSTL_VERSION_20__)
	#define MSTL_SUPPORT_CONCEPTS__			1
#endif
#if defined(MSTL_VERSION_20__)
	#define MSTL_SUPPORT_CONSTEVAL__		1
#endif

#if defined(MSTL_COMPILER_GNUC__) && defined(MSTL_VERSION_17__)
	#define MSTL_SUPPORT_UNLIKELY__			1
#endif
#if defined(MSTL_COMPILER_MSVC__) || defined(MSTL_COMPILER_CLANG__)
	#define MSTL_SUPPORT_MAKE_INTEGER_SEQ__	1
#endif
#if defined(MSTL_VERSION_20__) && !defined(MSTL_COMPILER_CLANG__)
	#define MSTL_SUPPORT_U8_INTRINSICS__	1
#endif


#define TO_STRING(VALUE) #VALUE

#define FOR_EACH(VALUE, CONTAINER) \
	for(auto VALUE = CONTAINER.begin(); VALUE != CONTAINER.end(); ++VALUE)


#ifdef MSTL_SUPPORT_CONSTEXPR__
	#define MSTL_CONSTEXPR constexpr
	#ifdef MSTL_VERSION_23__
		#define MSTL_CONSTEXPR23 MSTL_CONSTEXPR
	#else
		#define MSTL_CONSTEXPR23 inline
	#endif // MSTL_VERSION_23__
	#ifdef MSTL_VERSION_20__
		#define MSTL_CONSTEXPR20 MSTL_CONSTEXPR
	#else
		#define MSTL_CONSTEXPR20 inline
	#endif // MSTL_VERSION_20__
	#ifdef MSTL_VERSION_17__
		#define MSTL_CONSTEXPR17 MSTL_CONSTEXPR
		#define MSTL_INLINE17 inline
	#else
		#define MSTL_CONSTEXPR17 inline
		#define MSTL_INLINE17
	#endif // MSTL_VERSION_17__
	#ifdef MSTL_VERSION_14__
		#define MSTL_CONSTEXPR14 MSTL_CONSTEXPR
	#else
		#define MSTL_CONSTEXPR14 inline
	#endif // MSTL_VERSION_14__
    #ifdef MSTL_VERSION_11__
        #define MSTL_CONSTEXPR11 MSTL_CONSTEXPR
    #else
        #define MSTL_CONSTEXPR11 inline
    #endif // MSTL_VERSION_11__
#else
	#define MSTL_CONSTEXPR inline
	#define MSTL_CONSTEXPR23 inline
	#define MSTL_CONSTEXPR20 inline
	#define MSTL_CONSTEXPR17 inline
	#define MSTL_CONSTEXPR14 inline
	#define MSTL_INLINE17 inline
#endif // MSTL_SUPPORT_CONSTEXPR__


#ifdef MSTL_SUPPORT_IF_CONSTEXPR__
	#define MSTL_IF_CONSTEXPR if constexpr
#else
	// this macro will be used with caution, as it may break static overload under C++17.
	#define MSTL_IF_CONSTEXPR if
#endif


#ifdef MSTL_SUPPORT_CONSTEVAL__
	#define MSTL_CONSTEVAL consteval
#else
	#define MSTL_CONSTEVAL MSTL_CONSTEXPR
#endif


#ifdef MSTL_SUPPORT_NODISCARD__
	#define MSTL_NODISCARD [[nodiscard]]
	#define MSTL_ALLOC_NODISCARD \
		[[nodiscard("discard the return of allocators will cause memory leaks.")]]
#else
	#define MSTL_NODISCARD
	#define MSTL_ALLOC_NODISCARD
#endif


#ifdef MSTL_SUPPORT_ALIGNED__
    #if defined(MSTL_COMPILER_GNUC__)
		#define MSTL_ALIGNOF_DEFAULT() __attribute__((__aligned__))
		#define MSTL_ALIGNOF(ALIGN) __attribute__((__aligned__((ALIGN))))
	#elif defined(MSTL_COMPILER_MSVC__)
		#define MSTL_ALIGNOF_DEFAULT() [[aligned]]
		#define MSTL_ALIGNOF(ALIGN) [[aligned(ALIGN)]]
	#else
		#define MSTL_ALIGNOF_DEFAULT()
		#define MSTL_ALIGNOF(ALIGN)
	#endif
#else
	#define MSTL_ALIGNOF_DEFAULT()
	#define MSTL_ALIGNOF(ALIGN)
#endif


#ifdef MSTL_SUPPORT_ALWAYS_INLINE__
	#ifdef MSTL_COMPILER_GNUC__
		#define MSTL_ALWAYS_INLINE __attribute__((always_inline))
	#elif defined(MSTL_COMPILER_MSVC__)
		#define MSTL_ALWAYS_INLINE [[always_inline]]
	#else
		#define MSTL_ALWAYS_INLINE
	#endif
#else
	#define MSTL_ALWAYS_INLINE
#endif


#ifdef MSTL_SUPPORT_UNLIKELY__
	#define MSTL_UNLIKELY [[unlikely]]
#else
	#define MSTL_UNLIKELY
#endif


#ifdef MSTL_SUPPORT_NORETURN__
	#ifdef MSTL_COMPILER_GNUC__
		#define MSTL_NORETURN __attribute__((noreturn))
	#elif defined(MSTL_COMPILER_MSVC__)
		#define MSTL_NORETURN [[noreturn]]
	#else
		#define MSTL_NORETURN
	#endif
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


#ifdef MSTL_SUPPORT_DEPRECATED__
	#define MSTL_DEPRECATED [[deprecated]]
	// after C++ 11, we can use lambda expressions to quickly build closures
	// instead of using functor adapters.
	#define MSTL_FUNC_ADAPTER_DEPRECATE \
		[[deprecated("C++ 11 and later versions no longer use functor base types and functor adapters.")]]
	#define MSTL_TRAITS_DEPRECATE \
		[[deprecated("C++ 11 and later versions no longer use iterator traits functions.")]]
    #define MSTL_DEPRECATE_FOR(MSG) [[deprecated(MSG)]]
#else
	#define MSTL_DEPRECATED
	#define MSTL_FUNC_ADAPTER_DEPRECATE
	#define MSTL_TRAITS_DEPRECATE
    #define MSTL_DEPRECATE_FOR(MSG)
#endif


#ifdef MSTL_COMPILER_MSVC__
	#define MSTL_ALLOC_OPTIMIZE __declspec(allocator)
	#define MSTL_NOVTABLE __declspec(novtable)
#elif defined(MSTL_COMPILER_GNUC__)
	#define MSTL_ALLOC_OPTIMIZE __attribute__((__always_inline__))
	#define MSTL_NOVTABLE __attribute__((novtable))
#else
	#define MSTL_ALLOC_OPTIMIZE
	#define MSTL_NOVTABLE
#endif


#ifndef MSTL_COMPILER_UNSUPPORT__
#define MSTL_RESTRICT __restrict
#else
#define MSTL_RESTRICT
#endif


#ifdef MSTL_SUPPORT_NO_UNIQUE_ADS__
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


#if BOOST_VERSION >= 106500
	#define MSTL_SUPPORT_STACKTRACE__
#endif


// expand macro with basic char types.
#define MSTL_MACRO_RANGE_BASIC_CHARS(MAC) \
	MAC(char) \
	MAC(signed char) \
	MAC(unsigned char) \

#ifdef MSTL_VERSION_20__
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


// quickly define standard type alias.
#define MSTL_BUILD_TYPE_ALIAS(TYPE) \
	using value_type        = TYPE; \
	using pointer           = TYPE*; \
	using reference         = TYPE&; \
	using const_pointer     = const TYPE*; \
	using const_reference   = const TYPE&; \
	using size_type         = size_t; \
	using difference_type   = ptrdiff_t;


MSTL_BEGIN_NAMESPACE__

using nullptr_t	= decltype(nullptr);
using byte_t    = unsigned char;

using int8_t	= signed char;
using int16_t	= short;
using int32_t	= int;
#ifdef MSTL_PLATFORM_LINUX64__
using int64_t	= long;
#elif defined(MSTL_PLATFORM_WINDOWS__) || defined(MSTL_PLATFORM_LINUX32__)
using int64_t	= long long;
#endif


using uint8_t	= unsigned char;
using uint16_t	= unsigned short;
using uint32_t	= unsigned int;
#ifdef MSTL_PLATFORM_LINUX64__
using uint64_t	= unsigned long;
#elif defined(MSTL_PLATFORM_WINDOWS__) || defined(MSTL_PLATFORM_LINUX32__)
using uint64_t	= unsigned long long;
#endif


using float32_t	= float;
using float64_t	= double;
using decimal_t = long double;

#ifdef MSTL_PLATFORM_LINUX64__
using size_t	= unsigned long;
using ssize_t	= long;
using ptrdiff_t = long;
using intptr_t	= long;
using uintptr_t = unsigned long;
using intmax_t	= long;
using uintmax_t = unsigned long;
#elif defined(MSTL_PLATFORM_WIN64__)
using uintptr_t = unsigned long long;
using size_t	= unsigned long long;
using ssize_t	= long long;
using ptrdiff_t = long long;
using intptr_t	= long long;
using intmax_t	= long long;
using uintmax_t = unsigned long long;
#elif defined(MSTL_PLATFORM_WIN32__) || defined(MSTL_PLATFORM_LINUX32__)
using uintptr_t = unsigned int;
using size_t	= unsigned int;
using ssize_t	= int;
using ptrdiff_t = int;
using intptr_t	= int;
using intmax_t	= long long;
using uintmax_t = unsigned long long;
#endif


MSTL_INLINE17 constexpr size_t POINTER_SIZE = sizeof(void*);
MSTL_INLINE17 constexpr size_t SIZE_T_MAX_SIZE = static_cast<size_t>(-1);
MSTL_INLINE17 constexpr bool SIZE_T_SAME_WITH_LONG = sizeof(long) == sizeof(size_t);

MSTL_INLINE17 constexpr size_t MEMORY_ALIGN_THRESHHOLD = 16UL;

MSTL_END_NAMESPACE__
#endif // MSTL_BASICLIB_HPP__
