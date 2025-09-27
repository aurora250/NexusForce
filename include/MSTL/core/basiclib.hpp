#ifndef MSTL_BASICLIB_HPP__
#define MSTL_BASICLIB_HPP__
#include "undef_cmacro.hpp"
#include <iostream>
#ifdef MSTL_SUPPORT_BOOST__
#include <boost/version.hpp>
#endif
#ifdef MSTL_SUPPORT_QT6__
#include <QtGlobal>
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
    #ifdef MSTL_EXPORTS
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


#ifdef MSTL_PLATFORM_WINDOWS__
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <fcntl.h>
#include <io.h>
#define _CRT_SECURE_NO_WARNINGS
#include "undef_cmacro.hpp"
#elif defined(MSTL_PLATFORM_LINUX__)
#include <sys/sysinfo.h>
#endif


#define __MSTL_GLOBAL_NAMESPACE__ MSTL
#define USE_MSTL using namespace __MSTL_GLOBAL_NAMESPACE__;
#define MSTL_BEGIN_NAMESPACE__ namespace __MSTL_GLOBAL_NAMESPACE__ {
#define MSTL_END_NAMESPACE__ }
#define _MSTL __MSTL_GLOBAL_NAMESPACE__ ::

#define __MSTL_INNER_NAMESPACE__ __inner
#define MSTL_BEGIN_INNER__ namespace __MSTL_INNER_NAMESPACE__ {
#define MSTL_END_INNER__ }
#define _INNER __MSTL_GLOBAL_NAMESPACE__ :: __MSTL_INNER_NAMESPACE__ ::

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

#ifdef MSTL_STATE_DEBUG__
	#define SIMPLE_LOG(MESG) \
		std::cout << __FILE__ << ":" << __LINE__ << " " << __TIMESTAMP__ << " : " << MESG << std::endl;
#else
	#define SIMPLE_LOG(MESG)
#endif


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
#ifdef MSTL_PLATFORM_WINDOWS__
using int64_t	= long long;
#elif defined(MSTL_PLATFORM_LINUX__)
using int64_t	= long;
#endif

using uint8_t	= unsigned char;
using uint16_t	= unsigned short;
using uint32_t	= unsigned int;
#ifdef MSTL_PLATFORM_WINDOWS__
using uint64_t	= unsigned long long;
#elif defined(MSTL_PLATFORM_LINUX__)
using uint64_t	= unsigned long;
#endif

using float32_t	= float;
using float64_t	= double;
using decimal_t = long double;

#if defined(MSTL_PLATFORM_LINUX__)
using uintptr_t = unsigned long;
using size_t	= unsigned long;
using ssize_t	= long;
using ptrdiff_t = long;
using intptr_t	= long;
#elif defined(MSTL_PLATFORM_WIN64__)
using uintptr_t = unsigned long long;
using size_t	= unsigned long long;
using ssize_t	= long long;
using ptrdiff_t = long long;
using intptr_t	= long long;
#elif defined(MSTL_PLATFORM_WIN32__)
using uintptr_t = unsigned int;
using size_t	= unsigned int;
using ssize_t	= int;
using ptrdiff_t = int;
using intptr_t	= int;
#endif


MSTL_INLINE17 constexpr int8_t  INT8_MIN_SIZE  = -128;
MSTL_INLINE17 constexpr int16_t INT16_MIN_SIZE = -32768;
MSTL_INLINE17 constexpr int32_t INT32_MIN_SIZE = -2147483647 - 1;
MSTL_INLINE17 constexpr int64_t INT64_MIN_SIZE = -9223372036854775807LL - 1;

MSTL_INLINE17 constexpr int8_t  INT8_MAX_SIZE  = 127;
MSTL_INLINE17 constexpr int16_t INT16_MAX_SIZE = 32767;
MSTL_INLINE17 constexpr int32_t INT32_MAX_SIZE = 2147483647;
MSTL_INLINE17 constexpr int64_t INT64_MAX_SIZE = 9223372036854775807LL;

MSTL_INLINE17 constexpr uint8_t  UINT8_MAX_SIZE  = 255;
MSTL_INLINE17 constexpr uint16_t UINT16_MAX_SIZE = 65535;
MSTL_INLINE17 constexpr uint32_t UINT32_MAX_SIZE = 0xffffffffU;
MSTL_INLINE17 constexpr uint64_t UINT64_MAX_SIZE = 0xffffffffffffffffULL;


MSTL_INLINE17 constexpr float32_t FLOAT32_MAX_NEGA_SIZE = -1.175494351e-38f;
MSTL_INLINE17 constexpr float32_t FLOAT32_MIN_NEGA_SIZE = -3.402823466e+38f;

MSTL_INLINE17 constexpr float64_t FLOAT64_MAX_NEGA_SIZE = -2.2250738585072014e-308;
MSTL_INLINE17 constexpr float64_t FLOAT64_MIN_NEGA_SIZE = -1.7976931348623157e+308;

#ifdef MSTL_COMPILER_MSVC__
MSTL_INLINE17 constexpr decimal_t DECIMAL_MAX_NEGA_SIZE = FLOAT64_MAX_NEGA_SIZE;
MSTL_INLINE17 constexpr decimal_t DECIMAL_MIN_NEGA_SIZE = FLOAT32_MIN_NEGA_SIZE;
#elif defined(MSTL_COMPILER_GNUC__)
MSTL_INLINE17 constexpr decimal_t DECIMAL_MAX_NEGA_SIZE = -3.36210314311209350626267781732175260e-4932L;
MSTL_INLINE17 constexpr decimal_t DECIMAL_MIN_NEGA_SIZE = -1.18973149535723176502126385303097021e+4932L;
#endif


MSTL_INLINE17 constexpr float32_t FLOAT32_MAX_POSI_SIZE = 3.402823466e+38f;
MSTL_INLINE17 constexpr float32_t FLOAT32_MIN_POSI_SIZE = 1.175494351e-38f;

MSTL_INLINE17 constexpr float64_t FLOAT64_MAX_POSI_SIZE = 1.7976931348623157e+308;
MSTL_INLINE17 constexpr float64_t FLOAT64_MIN_POSI_SIZE = 2.2250738585072014e-308;

#ifdef MSTL_COMPILER_MSVC__
MSTL_INLINE17 constexpr decimal_t DECIMAL_MAX_POSI_SIZE = FLOAT64_MAX_POSI_SIZE;
MSTL_INLINE17 constexpr decimal_t DECIMAL_MIN_POSI_SIZE = FLOAT64_MIN_POSI_SIZE;
#elif defined(MSTL_COMPILER_GNUC__)
MSTL_INLINE17 constexpr decimal_t DECIMAL_MAX_POSI_SIZE = 1.18973149535723176502126385303097021e+4932L;
MSTL_INLINE17 constexpr decimal_t DECIMAL_MIN_POSI_SIZE = 3.36210314311209350626267781732175260e-4932L;
#endif


MSTL_INLINE17 constexpr size_t POINTER_SIZE = sizeof(void*);
MSTL_INLINE17 constexpr size_t SIZE_T_MAX_SIZE = static_cast<size_t>(-1);


MSTL_INLINE17 constexpr size_t MEMORY_ALIGN_THRESHHOLD = 16UL;

#ifdef MSTL_COMPILER_MSVC__
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_ALIGN = 32UL;
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_THRESHHOLD = 4096UL;

#ifdef MSTL_STATE_DEBUG__
MSTL_INLINE17 constexpr size_t MEMORY_NO_USER_SIZE = 2 * POINTER_SIZE + MEMORY_BIG_ALLOC_ALIGN - 1;
#else
MSTL_INLINE17 constexpr size_t MEMORY_NO_USER_SIZE = POINTER_SIZE + MEMORY_BIG_ALLOC_ALIGN - 1;
#endif

#ifdef MSTL_DATA_BUS_WIDTH_64__
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_SENTINEL = 0xFAFAFAFAFAFAFAFAUL;
#else
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_SENTINEL = 0xFAFAFAFAUL;
#endif
#endif // MSTL_COMPILER_MSVC__


MSTL_ALWAYS_INLINE inline void set_utf8_console() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);
    ::_setmode(::_fileno(stdout), _O_BINARY);
    ::_setmode(::_fileno(stderr), _O_BINARY);
    ::_setmode(::_fileno(stdin), _O_BINARY);
#endif
}


MSTL_ALWAYS_INLINE inline size_t get_available_memory() noexcept {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    ::GlobalMemoryStatusEx(&statex);
    return statex.ullAvailPhys + statex.ullAvailVirtual;
#elif defined(MSTL_PLATFORM_LINUX__)
    struct ::sysinfo info{};
    ::sysinfo(&info);
    return info.freeram * info.mem_unit + info.freeswap * info.mem_unit;
#else
    return 0;
#endif
}


#ifdef MSTL_COMPILER_GNUC__
#define memory_barrier(p) \
	__asm__ volatile ("" : : "m" (*p) : "memory");
#elif defined(MSTL_COMPILER_MSVC__)
#define memory_barrier(p) \
	::_ReadWriteBarrier();
#else
#define memory_barrier(p)
#endif


constexpr uint64_t SPACE_MASK =
	(1ULL << 9)  |  // \t
	(1ULL << 10) |  // \n
	(1ULL << 11) |  // \v
	(1ULL << 12) |  // \f
	(1ULL << 13) |  // \r
	(1ULL << 32);   // space


MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_space(const char c) noexcept {
	return (SPACE_MASK & (1ULL << static_cast<byte_t>(c))) != 0;
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_space(const wchar_t c) noexcept {
	return c < 64 && (SPACE_MASK & (1ULL << c)) != 0;
}

#ifdef MSTL_VERSION_20__
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_space(const char8_t c) noexcept {
	return c < 64 && (SPACE_MASK & (1ULL << c)) != 0;
}
#endif

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_space(const char16_t c) noexcept {
	return c < 64 && (SPACE_MASK & (1ULL << c)) != 0;
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_space(const char32_t c) noexcept {
	return c < 64 && (SPACE_MASK & (1ULL << c)) != 0;
}


MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha(const char c) noexcept {
	const auto uc = static_cast<byte_t>(c);
	return (uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z';
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha(const wchar_t c) noexcept {
	if (c < 0 || c > 127) return false;
	const auto uc = static_cast<unsigned>(c);
	return (uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z';
}

#ifdef MSTL_VERSION_20__
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha(const char8_t c) noexcept {
	if (c > 127) return false;
	return (c & 0xDF) >= 'A' && (c & 0xDF) <= 'Z';
}
#endif

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha(const char16_t c) noexcept {
	if (c > 127) return false;
	return (c & 0xDF) >= 'A' && (c & 0xDF) <= 'Z';
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha(const char32_t c) noexcept {
	if (c > 127) return false;
	return (c & 0xDF) >= 'A' && (c & 0xDF) <= 'Z';
}


MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit(const char c) noexcept {
	const auto uc = static_cast<byte_t>(c);
	return (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit(const wchar_t c) noexcept {
	if (c < 0 || c > 127) return false;
	const auto uc = static_cast<unsigned>(c);
	return (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
}

#ifdef MSTL_VERSION_20__
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit(const char8_t c) noexcept {
	if (c > 127) return false;
	return (c & 0xF0) == 0x30 && (c & 0x0F) <= 9;
}
#endif

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit(const char16_t c) noexcept {
	if (c > 127) return false;
	return (c & 0xF0) == 0x30 && (c & 0x0F) <= 9;
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_digit(const char32_t c) noexcept {
	if (c > 127) return false;
	return (c & 0xF0) == 0x30 && (c & 0x0F) <= 9;
}


MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_xdigit(const char c) noexcept {
    const auto uc = static_cast<byte_t>(c);
    const bool is_09 = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 0x09;
    const bool is_AF = (uc & 0xF0) == 0x40 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    const bool is_af = (uc & 0xF0) == 0x60 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_xdigit(const wchar_t c) noexcept {
    if (c < 0 || c > 127) return false;
    const auto uc = static_cast<unsigned>(c);
    const bool is_09 = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 0x09;
    const bool is_AF = (uc & 0xF0) == 0x40 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    const bool is_af = (uc & 0xF0) == 0x60 && (uc & 0x0F) >= 0x01 && (uc & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}

#ifdef MSTL_VERSION_20__
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_xdigit(const char8_t c) noexcept {
    if (c > 127) return false;
    const bool is_09 = (c & 0xF0) == 0x30 && (c & 0x0F) <= 0x09;
    const bool is_AF = (c & 0xF0) == 0x40 && (c & 0x0F) >= 0x01 && (c & 0x0F) <= 0x06;
    const bool is_af = (c & 0xF0) == 0x60 && (c & 0x0F) >= 0x01 && (c & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}
#endif

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_xdigit(const char16_t c) noexcept {
    if (c > 127) return false;
    const bool is_09 = (c & 0xF0) == 0x30 && (c & 0x0F) <= 0x09;
    const bool is_AF = (c & 0xF0) == 0x40 && (c & 0x0F) >= 0x01 && (c & 0x0F) <= 0x06;
    const bool is_af = (c & 0xF0) == 0x60 && (c & 0x0F) >= 0x01 && (c & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_xdigit(const char32_t c) noexcept {
    if (c > 127) return false;
    const bool is_09 = (c & 0xF0) == 0x30 && (c & 0x0F) <= 0x09;
    const bool is_AF = (c & 0xF0) == 0x40 && (c & 0x0F) >= 0x01 && (c & 0x0F) <= 0x06;
    const bool is_af = (c & 0xF0) == 0x60 && (c & 0x0F) >= 0x01 && (c & 0x0F) <= 0x06;
    return is_09 || is_AF || is_af;
}


MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha_or_digit(const char c) noexcept {
	return is_digit(c) || is_alpha(c);
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha_or_digit(const wchar_t c) noexcept {
	return is_digit(c) || is_alpha(c);
}

#ifdef MSTL_VERSION_20__
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha_or_digit(const char8_t c) noexcept {
	if (c > 127) return false;
	const auto uc = static_cast<unsigned>(c);
	const bool digit = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
	return digit || ((uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z');
}
#endif

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha_or_digit(const char16_t c) noexcept {
	if (c > 127) return false;
	const auto uc = static_cast<unsigned>(c);
	const bool digit = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
	return digit || ((uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z');
}

MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 bool is_alpha_or_digit(const char32_t c) noexcept {
	if (c > 127) return false;
	const auto uc = static_cast<unsigned>(c);
	const bool digit = (uc & 0xF0) == 0x30 && (uc & 0x0F) <= 9;
	return digit || ((uc & 0xDF) >= 'A' && (uc & 0xDF) <= 'Z');
}


// case characters to lower.
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 char to_lowercase(const char c) noexcept {
	const auto uc = static_cast<byte_t>(c);
	return (uc >= 'A' && uc <= 'Z') ? static_cast<char>(uc | 0x20) : c;
}

// case characters to upper.
MSTL_CONST_FUNCTION MSTL_CONSTEXPR14 char to_uppercase(const char c) noexcept {
	const auto uc = static_cast<byte_t>(c);
	return (uc >= 'a' && uc <= 'z') ? static_cast<char>(uc & 0xDF) : c;
}


static constexpr uint8_t POPCOUNT_TABLE[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

inline int popcountll(const size_t x) {
#ifdef MSTL_COMPILER_GNUC__
	return __builtin_popcountll(x);
#elif defined(MSTL_COMPILER_MSVC__)
#ifdef MSTL_DATA_BUS_WIDTH_64__
	return static_cast<int>(__popcnt64(x));
#else
	return static_cast<int>(__popcnt(static_cast<uint32_t>(x)));
#endif
#else
	return
		POPCOUNT_TABLE[x & 0xFF] +
		POPCOUNT_TABLE[(x >> 8) & 0xFF] +
		POPCOUNT_TABLE[(x >> 16) & 0xFF] +
		POPCOUNT_TABLE[(x >> 24) & 0xFF] +
		POPCOUNT_TABLE[(x >> 32) & 0xFF] +
		POPCOUNT_TABLE[(x >> 40) & 0xFF] +
		POPCOUNT_TABLE[(x >> 48) & 0xFF] +
		POPCOUNT_TABLE[(x >> 56) & 0xFF];
#endif
}

inline int clzll(size_t x) {
	if (x == 0) return 64;
#ifdef MSTL_COMPILER_GNUC__
	return __builtin_clzll(x);
#elif defined(MSTL_COMPILER_MSVC__)
	unsigned long index;
#ifdef MSTL_DATA_BUS_WIDTH_64__
	::_BitScanReverse64(&index, x);
#else
	::_BitScanReverse(&index, x);
#endif
	return 63 - static_cast<int>(index);
#else
	if (x == 0) return 64;

	int n = 0;
	if (x <= 0x00000000FFFFFFFFULL) {
		n += 32;
		x <<= 32;
	}
	if (x <= 0x0000FFFFFFFFFFFFULL) {
		n += 16;
		x <<= 16;
	}
	if (x <= 0x00FFFFFFFFFFFFFFULL) {
		n += 8;
		x <<= 8;
	}
	if (x <= 0x0FFFFFFFFFFFFFFFULL) {
		n += 4;
		x <<= 4;
	}
	if (x <= 0x3FFFFFFFFFFFFFFFULL) {
		n += 2;
		x <<= 2;
	}
	if (x <= 0x7FFFFFFFFFFFFFFFULL) {
		n += 1;
	}
	return n;
#endif
}


#ifdef MSTL_COMPILER_MSVC__
namespace masm {
	extern "C" {
		void* masm_memory_copy(void* dest, const void* src, size_t count);
		void* masm_memory_copy_offset(void* dest, const void* src, size_t count);
		void* masm_memory_char_copy(void* dest, const void* src, int chr, size_t count);
		int masm_memory_compare(const void* lh, const void* rh, size_t count);
		int masm_memory_compare_ignore_case(const void* ptr1, const void* rh, size_t count);
		void* masm_memory_char(const void* dest, int value, size_t count);
		void* masm_memory_move(void* dest, const void* src, size_t count);
		void* masm_memory_set(void* dest, int value, size_t count);
		void masm_memory_zero(void* dest, size_t count);
		void masm_explicit_memory_zero(void* dest, size_t count);
		void* masm_memory_in_memory(void* data, size_t data_len, const void* pattern, size_t pattern_len);
		void* masm_memory_frobnicate(void* s, size_t n);
	}
}
#endif


// copy from source memory to destination memory with specific length.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memcpy.
inline void* memory_copy(void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

#ifdef MSTL_COMPILER_GNUC__
	void* res = dest;

	__asm__ volatile (
		"movq   %1, %%rsi\n\t"
		"movq   %2, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"cld\n\t"
		"rep    movsb\n\t"
		:
		: "r" (res), "r" (src), "r" (dest), "r" (count)
		: "rsi", "rdi", "rcx", "cc", "memory"
	);
	return res;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_copy(dest, src, count);
#else
	void* res = dest;
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	while (count--) {
		*dest_v = *src_v;
		dest_v++;
		src_v++;
	}
	memory_barrier(res);
	return res;
#endif
}

// it`s similar with std::mempcpy
inline void* memory_copy_offset(void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

#ifdef MSTL_COMPILER_GNUC__
	__asm__ volatile (
		"movq   %1, %%rsi\n\t"
		"movq   %2, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"cld\n\t"
		"rep    movsb\n\t"
		"movq   %%rdi, %0\n\t"
		: "=r" (dest)
		: "r" (src), "r" (dest), "r" (count)
		: "rsi", "rdi", "rcx", "cc", "memory"
	);
	return dest;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_copy_offset(dest, src, count);
#else
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	while (count--) {
		*dest_v = *src_v;
		dest_v++;
		src_v++;
	}
	memory_barrier(dest_v);
	return (void*) dest_v;
#endif
}

// copy from source memory to destination memory with specific length if not encounter target character.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memccpy.
inline void* memory_char_copy(void* dest, const void* src, const int chr, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) return nullptr;
    const byte_t target = static_cast<byte_t>(chr);

#ifdef MSTL_COMPILER_GNUC__
    void* result;
	__asm__ volatile (
		"movq   %1, %%rsi\n\t"
		"movq   %2, %%rdi\n\t"
		"movb   %b3, %%al\n\t"
		"movq   %4, %%rcx\n\t"
		"cld\n\t"
	".L1:\n\t"
		"jrcxz  .L2\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"cmpb   %b3, %%al\n\t"
		"je     .L3\n\t"
		"loop   .L1\n\t"
	".L2:\n\t"
		"xorq   %%rax, %%rax\n\t"
		"jmp    .L4\n\t"
	".L3:\n\t"
		"movq   %%rdi, %%rax\n\t"
		"jmp    .L4\n\t"
	".L4:\n\t"
		"movq   %%rax, %0\n\t"
		: "=r" (result)
		: "r" (src), "r" (dest), "r" (target), "r" (count)
		: "rsi", "rdi", "rcx", "rax", "cc", "memory"
	);
    return result;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_char_copy(dest, src, count, chr);
#else
    auto dest_v = static_cast<volatile byte_t*>(dest);
    auto src_v = static_cast<const volatile byte_t*>(src);

    while (count--) {
        const byte_t current = *src_v;
        *dest_v = current;
        if (current == target) {
        	memory_barrier(dest_v);
            return (void*) (++dest_v); // bypass the volatile check
        }
        dest_v++;
        src_v++;
    }
	memory_barrier(dest_v);
    return nullptr;
#endif
}

// compare with left-hand memory and right-hand memory in a specific length.
// return a positive number when left-hand memory is greater, a negative number when right-hand memory is greater
// and return zero when they are equal in specific length.
// it`s similar with std::memcmp.
MSTL_PURE_FUNCTION inline int memory_compare(const void* lh, const void* rh, size_t count) noexcept {
	if (lh == nullptr && rh == nullptr) return 0;
	if (lh == nullptr) return -1;
    if (rh == nullptr) return 1;
#ifdef MSTL_COMPILER_GNUC__
	int result;
	__asm__ volatile (
		"movq   %1, %%rsi\n\t"
		"movq   %2, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"cld\n\t"
		"repe   cmpsb\n\t"
		"je     1f\n\t"

		"movzbl -1(%%rsi), %%eax\n\t"
		"movzbl -1(%%rdi), %%edx\n\t"
		"subl   %%edx, %%eax\n\t"
		"jmp    2f\n"

		"1:\n\t"
		"xorl   %%eax, %%eax\n"
		"2:"
		: "=a" (result)
		: "r" (lh), "r" (rh), "r" (count)
		: "rsi", "rdi", "rcx", "rdx", "cc"
	);
	return result;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_compare(lh, rh, count);
#else
	while (count--) {
		if (*static_cast<const byte_t*>(lh) != *static_cast<const byte_t*>(rh))
			return *static_cast<const byte_t*>(lh) - *static_cast<const byte_t*>(rh);
		lh = static_cast<const byte_t*>(lh) + 1;
		rh = static_cast<const byte_t*>(rh) + 1;
	}
	return 0;
#endif
}

// compare with left-hand memory and right-hand memory in a specific length but ignored case of characters.
// return a positive number when left-hand memory is greater, a negative number when right-hand memory is greater
// and return zero when they are equal in specific length.
// it`s similar with std::memicmp.
MSTL_PURE_FUNCTION inline int memory_compare_ignore_case(const void* lh, const void* rh, size_t count) noexcept {
	if ((lh == nullptr && rh == nullptr) || count == 0) return 0;
	if (lh == nullptr) return -1;
	if (rh == nullptr) return 1;

#ifdef MSTL_COMPILER_GNUC__
    int result = 0;
	__asm__ volatile (
		"movq   %1, %%rsi\n\t"
		"movq   %2, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"cld\n\t"
	".L1:\n\t"
		"jrcxz  .L2\n\t"
		"lodsb\n\t"
		"movb   %%al, %%dl\n\t"
		"or     $0x20, %%dl\n\t"
		"movb   (%%rdi), %%al\n\t"
		"movb   %%al, %%bl\n\t"
		"or     $0x20, %%bl\n\t"
		"cmpb   %%bl, %%dl\n\t"
		"jne    .L3\n\t"
		"inc    %%rdi\n\t"
		"loop   .L1\n\t"
	".L2:\n\t"
		"xorl   %%eax, %%eax\n\t"
		"jmp    .L4\n\t"
	".L3:\n\t"
		"movzbl %%dl, %%eax\n\t"
		"movzbl %%bl, %%edx\n\t"
		"subl   %%edx, %%eax\n\t"
	".L4:\n\t"
		"movl   %%eax, %0\n\t"
		: "=r" (result)
		: "r" (lh), "r" (rh), "r" (count)
		: "rsi", "rdi", "rcx", "rax", "rdx", "rbx", "cc"
	);
    return result;
#elif defined(MSTL_COMPILER_MSVC__)
    return masm::masm_memory_compare_ignore_case(lh, rh, count);
#else
	const auto lh_v = static_cast<const volatile byte_t*>(lh);
	const auto rh_v = static_cast<const volatile byte_t*>(rh);

    for (size_t i = 0; i < count; ++i) {
        const byte_t c1 = static_cast<byte_t>(_MSTL to_lowercase(static_cast<char>(lh_v[i])));
        const byte_t c2 = static_cast<byte_t>(_MSTL to_lowercase(static_cast<char>(rh_v[i])));
        if (c1 != c2) {
            return static_cast<int>(c1) - static_cast<int>(c2);
        }
    }
    return 0;
#endif
}

// return a pointer which is pointing to the first place that equal to target value in a specific length.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::memchr.
MSTL_PURE_FUNCTION inline void* memory_char(const void* dest, const int value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
#ifdef MSTL_COMPILER_GNUC__
	void* result;
	__asm__ volatile (
		"movb   %b2, %%al\n\t"
		"movq   %1, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"cld\n\t"
		"repne  scasb\n\t"
		"je     1f\n\t"
		"xorq   %%rax, %%rax\n\t"
		"jmp    2f\n"
		"1:\n\t"
		"decq   %%rdi\n\t"
		"movq   %%rdi, %%rax\n"
		"2:"
		: "=a" (result)
		: "r" (dest), "r" (value), "r" (count)
		: "rdi", "rcx", "cc"
	);
	return result;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_char(dest, value, count);
#else
	auto p = static_cast<const byte_t *>(dest);
	while (count--) {
		if (*p == static_cast<byte_t>(value))
			return const_cast<byte_t*>(p);
		p++;
	}
	return nullptr;
#endif
}

// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memmove.
inline void* memory_move(void* dest, const void* src, size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
#ifdef MSTL_COMPILER_GNUC__
	void* res = dest;
	__asm__ volatile (
			"movq   %1, %%rsi\n\t"
			"movq   %2, %%rdi\n\t"
			"movq   %3, %%rcx\n\t"
			"cmpq   %%rsi, %%rdi\n\t"
			"jbe    1f\n\t"

			"addq   %%rcx, %%rsi\n\t"
			"addq   %%rcx, %%rdi\n\t"
			"decq   %%rsi\n\t"
			"decq   %%rdi\n\t"
			"std\n\t"
			"rep    movsb\n\t"
			"cld\n\t"
			"jmp    2f\n"

			"1:\n\t"
			"cld\n\t"
			"rep    movsb\n"
			"2:"
			:
			: "r" (res), "r" (src), "r" (dest), "r" (count)
			: "rsi", "rdi", "rcx", "cc", "memory"
		);
	return res;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_move(dest, src, count);
#else
	void* res = dest;
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	if (dest_v < src_v) {
		while (count--) {
			*dest_v = *src_v;
			dest_v = dest_v + 1;
			src_v = src_v + 1;
		}
	}
	else if (dest_v > src_v) {
		while (count--) {
			*(dest_v + count) = *(src_v + count);
		}
	}
	memory_barrier(res);
	return res;
#endif
}

// fill the destination memory with target value in the specific length.
// if parameter pointer is nullptr, return nullptr.
// it`s similar with std::memset.
inline void* memory_set(void* dest, const int value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
#ifdef MSTL_COMPILER_GNUC__
	void* ret = static_cast<byte_t *>(dest);
	__asm__ volatile (
		"movq   %1, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"movb   %b2, %%al\n\t"
		"cld\n\t"
		"rep    stosb"
		:
		: "r" (ret), "r" (dest), "r" (value), "r" (count)
		: "rdi", "rcx", "rax", "cc", "memory"
	);
	return ret;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_set(dest, value, count);
#else
	void* ret = static_cast<byte_t *>(dest);
	auto dest_v = static_cast<volatile byte_t *>(dest);
	while (count--) {
		*dest_v = static_cast<byte_t>(value);
		dest_v = dest_v + 1;
	}
	memory_barrier(ret);
	return ret;
#endif
}

// clear the destination memory with zero in the specific length.
// if parameter pointer is nullptr, do nothing.
// it`s similar with std::bzero.
inline void memory_zero(void* dest, const size_t count) noexcept {
#ifdef MSTL_COMPILER_GNUC__
	__asm__ volatile (
		"movq   %1, %%rdi\n\t"
		"movq   %2, %%rcx\n\t"
		"xorb   %%al, %%al\n\t"
		"cld\n\t"
		"rep    stosb\n\t"
		:
		: "r" (dest), "r" (dest), "r" (count)
		: "rdi", "rcx", "rax", "memory", "cc"
	);
#elif defined(MSTL_COMPILER_MSVC__)
	masm::masm_memory_zero(dest, count);
#else
	const auto dest_v = static_cast<volatile byte_t*>(dest);
	for (size_t i = 0; i < count; ++i) {
		dest_v[i] = 0;
	}
	memory_barrier(dest_v);
#endif
}

// explicit_bzero
inline void explicit_memory_zero(void* dest, const size_t count) noexcept {
	if (dest == nullptr || count == 0) return;

#ifdef MSTL_COMPILER_GNUC__
	__asm__ volatile (
			"movq   %0, %%rdi\n\t"
			"movq   %1, %%rcx\n\t"
			"xorb   %%al, %%al\n\t"
			"cld\n\t"
			"rep    stosb\n\t"
			""
			:
			: "r" (dest), "r" (count)
			: "rdi", "rcx", "rax", "memory", "cc"
		);
#elif defined(MSTL_COMPILER_MSVC__)
	masm::masm_explicit_memory_zero(dest, count);
#else
	const auto vptr = static_cast<volatile byte_t*>(dest);
	for (size_t i = 0; i < count; ++i)
		vptr[i] = 0;
	memory_barrier(vptr);
#endif
}

// std::memmem
inline void* memory_in_memory(void* data, size_t data_len, const void* pattern, size_t pattern_len) noexcept {
    if (data == nullptr || pattern == nullptr || data_len == 0 || pattern_len == 0 || pattern_len > data_len) {
        return nullptr;
    }
#ifdef MSTL_COMPILER_GNUC__
    void* result;
    const size_t last_possible = data_len - pattern_len + 1;
	__asm__ volatile (
		"movq   %1, %%rsi\n\t"
		"movq   %2, %%rdi\n\t"
		"movq   %3, %%rcx\n\t"
		"movq   %4, %%r8\n\t"
		"xorq   %%rax, %%rax\n\t"
		"cld\n\t"
	".L1:\n\t"
		"jrcxz  .L5\n\t"
		"movb   (%%rsi), %%al\n\t"
		"movb   (%%rdi), %%dl\n\t"
		"cmpb   %%dl, %%al\n\t"
		"jne    .L4\n\t"
		"movq   %%rsi, %%r9\n\t"
		"movq   %%rdi, %%r10\n\t"
		"movq   %%r8, %%r11\n\t"
	".L2:\n\t"
		"jrcxz  .L3\n\t"
		"movb   (%%r9), %%al\n\t"
		"movb   (%%r10), %%dl\n\t"
		"cmpb   %%dl, %%al\n\t"
		"jne    .L3\n\t"
		"inc    %%r9\n\t"
		"inc    %%r10\n\t"
		"dec    %%r11\n\t"
		"jmp    .L2\n\t"
	".L3:\n\t"
		"cmpq   $0, %%r11\n\t"
		"je     .L6\n\t"
	".L4:\n\t"
		"inc    %%rsi\n\t"
		"dec    %%rcx\n\t"
		"jmp    .L1\n\t"
	".L5:\n\t"
		"xorq   %%rax, %%rax\n\t"
		"jmp    .L7\n\t"
	".L6:\n\t"
		"movq   %%rsi, %%rax\n\t"
		"jmp    .L7\n\t"
	".L7:\n\t"
		"movq   %%rax, %0\n\t"
		: "=r" (result)
		: "r" (data), "r" (pattern), "r" (last_possible), "r" (pattern_len)
		: "rsi", "rdi", "rcx", "rax", "rdx", "r8", "r9", "r10", "r11", "cc"
	);
    return result;
#elif defined(MSTL_COMPILER_MSVC__)
    return masm::masm_memory_in_memory(data, data_len, pattern, pattern_len);
#else
    const size_t last_possible = data_len - pattern_len + 1;
    const auto data_v = static_cast<const byte_t*>(data);
    const auto pattern_v = static_cast<const byte_t*>(pattern);

    for (size_t i = 0; i < last_possible; ++i) {
        if (data_v[i] == pattern_v[0]) {
            if (memory_compare(data_v + i, pattern_v, pattern_len) == 0) {
                return const_cast<void*>(static_cast<const void*>(data_v + i));
            }
        }
    }
    return nullptr;
#endif
}

// std::memfrob
inline void* memory_frobnicate(void* s, const size_t n) {
	if (s == nullptr || n == 0) return s;

#ifdef MSTL_COMPILER_GNUC__
	__asm__ volatile (
		"movq   %1, %%rdi\n\t"
		"movq   %2, %%rcx\n\t"
		"movb   $42, %%al\n\t"
		"cld\n\t"
	".L1:"
		"jrcxz  .L2\n\t"
		"xorb   %%al, (%%rdi)\n\t"
		"inc    %%rdi\n\t"
		"loop   .L1\n\t"
	".L2:"
		:
		: "r" (s), "r" (s), "r" (n)
		: "rdi", "rcx", "rax", "memory", "cc"
	);
	return s;
#elif defined(MSTL_COMPILER_MSVC__)
	return masm::masm_memory_frobnicate(s, n);
#else
	const auto s_v = static_cast<volatile byte_t*>(s);
	for (size_t i = 0; i < n; i++) {
		s_v[i] ^= 42;
	}
	memory_barrier(s_v);
	return s;
#endif
}


// copy from source string to destination string.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::strcpy.
MSTL_CONSTEXPR14 char* string_copy(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	char* ret = dest;
	while (*src != '\0') {
		*dest = *src;
		dest++;
		src++;
	}
	*dest = *src;
	return ret;
}

// stpcpy
MSTL_CONSTEXPR14 char* string_copy_offset(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	while (*src != '\0') {
		*dest = *src;
		dest++;
		src++;
	}
	*dest = *src;
	return dest - 1;
}

// concatenate source string to the tail of destination string.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::strcat.
MSTL_CONSTEXPR14 char* string_concatenate(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	char* original_dest = dest;
	while (*dest != '\0')
		++dest;

	while (*src != '\0') {
		*dest = *src;
		++dest;
		++src;
	}
	*dest = '\0';
	return original_dest;
}

// compare with left-hand string and right-hand string in a specific length.
// return a positive number when left-hand string is greater, a negative number when right-hand string is greater
// and return zero when they are equal in specific length.
// it`s similar with std::strcmp.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 int string_compare(const char* dest, const char* src) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	while (*dest == *src) {
		if (*dest == '\0') return 0;
		dest++;
		src++;
	}
	if (*dest > *src) return 1;
	return -1;
}

// compare with left-hand string and right-hand string in a specific length but ignored case of characters.
// return a positive number when left-hand string is greater, a negative number when right-hand string is greater
// and return zero when they are equal in specific length.
// it`s similar with std::stricmp.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 int string_compare_ignore_case(const char* s1, const char* s2) {
	if (s1 == nullptr && s2 == nullptr) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	while (*s1 && *s2) {
		const char c1 = to_lowercase(*s1);
		const char c2 = to_lowercase(*s2);
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		++s1;
		++s2;
	}
	return *s1 == *s2 ? 0 : *s1 < *s2 ? -1 : 1;
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 int string_compare_natural(const char* s1, const char* s2) noexcept {
	if (s1 == nullptr && s2 == nullptr) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	while (*s1 != '\0' && *s2 != '\0') {
		if (is_digit(*s1) && is_digit(*s2)) {
			const char* s1_skip_zero = s1;
			while (*s1_skip_zero == '0' && is_digit(*(s1_skip_zero + 1))) {
				s1_skip_zero++;
			}
			const char* s2_skip_zero = s2;
			while (*s2_skip_zero == '0' && is_digit(*(s2_skip_zero + 1))) {
				s2_skip_zero++;
			}

			const char* s1_end = s1_skip_zero;
			while (is_digit(*s1_end)) s1_end++;
			const size_t len1 = s1_end - s1_skip_zero;

			const char* s2_end = s2_skip_zero;
			while (is_digit(*s2_end)) s2_end++;
			const size_t len2 = s2_end - s2_skip_zero;

			if (len1 != len2) {
				return len1 > len2 ? 1 : -1;
			}
			for (size_t i = 0; i < len1; i++) {
				if (s1_skip_zero[i] != s2_skip_zero[i]) {
					return s1_skip_zero[i] - s2_skip_zero[i];
				}
			}
			s1 = s1_end;
			s2 = s2_end;
			continue;
		}

		if (*s1 != *s2)
			return *s1 - *s2;
		s1++;
		s2++;
	}
	return *s1 - *s2;
}

// return the length of string when the loop encounter '\0'
// it`s similar with std::strlen.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 size_t string_length(const char* str) noexcept {
	const char* p = str;
	while (*p != '\0')
		++p;
	return static_cast<size_t>(p - str);
}

// return a pointer which is pointing to the first place that equal to target character.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::strchr.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 const char* string_char(const char* str, const char chr) noexcept {
	if (str == nullptr) return nullptr;
	while (*str != '\0') {
		if (*str == static_cast<char>(chr))
			return str;
		++str;
	}

	if (*str == static_cast<char>(chr))
		return str;
	return nullptr;
}

// return a pointer which is pointing to the last place that equal to target character.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::strchr.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 const char* string_last_char(const char* str, const char chr) noexcept {
	if (str == nullptr) return nullptr;
	const char* last = nullptr;

	while (*str != '\0') {
		if (*str == static_cast<char>(chr))
			last = str;
		++str;
	}
	return last;
}

// return the index which is pointing to the last place that equal to target character.
// if any parameter pointer is nullptr, return zero. if not found, return nullptr.
// it`s similar with std::strspn.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 size_t string_span_in(const char* str, const char* accept) noexcept {
	if (str == nullptr || *str == '\0' || accept == nullptr || *accept == '\0')
		return 0;

	const char* original_str = str;
	while (*str != '\0') {
		const char* a = accept;
		bool found = false;
		while (*a != '\0') {
			if (*str == *a) {
				found = true;
				break;
			}
			++a;
		}
		if (!found)
			return static_cast<size_t>(str - original_str);
		++str;
	}
	return static_cast<size_t>(str - original_str);
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 size_t string_span_not_in(const char* str, const char* reject) noexcept {
	if (str == nullptr || *str == '\0') return 0;
	if (reject == nullptr || *reject == '\0') {
		size_t len = 0;
		while (str[len] != '\0') ++len;
		return len;
	}

	const char* original_str = str;
	while (*str != '\0') {
		const char* r = reject;
		while (*r != '\0') {
			if (*str == *r)
				return static_cast<size_t>(str - original_str);
			++r;
		}
		++str;
	}
	return static_cast<size_t>(str - original_str);
}

MSTL_CONSTEXPR14 char* string_to_lowercase(char* str) noexcept {
	if (str == nullptr) return nullptr;

	constexpr size_t diff = 'a' - 'A';
	char* original = str;
	while (*str != '\0') {
		if (*str >= 'A' && *str <= 'Z')
			*str = static_cast<char>(*str + diff);
		++str;
	}
	return original;
}

MSTL_CONSTEXPR14 char* string_to_uppercase(char* str) noexcept {
	if (str == nullptr) return nullptr;

	constexpr size_t diff = 'a' - 'A';
	char* original = str;
	while (*str != '\0') {
		if (*str >= 'a' && *str <= 'z')
			*str = static_cast<char>(*str - diff);
		++str;
	}
	return original;
}

// same to std::strpbrk
MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 char* string_find_any(char* str, const char* accept) noexcept {
	if (str == nullptr || *str == '\0' || accept == nullptr || *accept == '\0')
		return nullptr;

	while (*str != '\0') {
		const char* a = accept;
		while (*a != '\0') {
			if (*str == *a)
				return str;
			++a;
		}
		++str;
	}
	return nullptr;
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 const char* string_in_string(const char* dest, const char* src) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	const char* cur = dest;
	while (*cur) {
		const char *str1 = cur;
		const char *str2 = src;
		while (*str1 && *str2 && *str1 == *str2) {
			str1++;
			str2++;
		}
		if (*str2 == '\0') return cur;
		cur++;
	}
	return nullptr;
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 const char* string_in_string_ignored_case(const char* dest, const char* src) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	if (*src == '\0') return dest;

	const char* cur = dest;
	while (*cur) {
		const char* str1 = cur;
		const char* str2 = src;
		while (*str1 && *str2) {
			const char c1 = to_lowercase(*str1);
			const char c2 = to_lowercase(*str2);
			if (c1 != c2) break;
			str1++;
			str2++;
		}
		if (*str2 == '\0') return cur;
		cur++;
	}
	return nullptr;
}

// strsep
MSTL_CONSTEXPR23 char* string_token(char* MSTL_RESTRICT str, const char* MSTL_RESTRICT delimiters) {
	static char* next_token = nullptr;
	if (str != nullptr) next_token = str;

	if (next_token == nullptr) return nullptr;
	while (*next_token != '\0' && string_char(delimiters, *next_token) != nullptr) {
		++next_token;
	}
	if (*next_token == '\0') {
		next_token = nullptr;
		return nullptr;
	}

	char* token = next_token;
	while (*next_token != '\0' && string_char(delimiters, *next_token) == nullptr)
		++next_token;
	if (*next_token != '\0') {
		*next_token = '\0';
		++next_token;
	}
	else next_token = nullptr;
	return token;
}

MSTL_CONSTEXPR23 char* string_last_token(char* MSTL_RESTRICT str, const char* MSTL_RESTRICT delimiters) {
	static char* prev_token = nullptr;
	static char* original_str = nullptr;

	if (str != nullptr) {
		prev_token = nullptr;
		original_str = str;
		if (original_str != nullptr) {
			char* end = original_str;
			while (*end != '\0') ++end;
			prev_token = end;
		}
	}
	if (prev_token == nullptr || prev_token == original_str)
		return nullptr;

	char* token_end = prev_token;
	while (token_end > original_str) {
		--token_end;
		if (string_char(delimiters, *token_end) == nullptr)
			break;
	}
	if (token_end == original_str && string_char(delimiters, *token_end) != nullptr) {
		prev_token = original_str;
		return nullptr;
	}

	char* token_start = token_end;
	while (token_start > original_str && string_char(delimiters, *(token_start - 1)) == nullptr)
		--token_start;

	prev_token = token_start;
	if (token_start > original_str)
		*(token_start - 1) = '\0';
	else
		prev_token = original_str;
	return token_start;
}

MSTL_MALLOC_FUNCTION MSTL_CONSTEXPR20 char* string_duplicate(const char* str) noexcept {
	if (str == nullptr) return nullptr;

	const size_t len = string_length(str);
	const auto new_str = static_cast<char*>(std::malloc(len + 1));
	if (new_str == nullptr) return nullptr;

	string_copy(new_str, str);
	return new_str;
}

MSTL_CONSTEXPR14 char* string_set(char* str, const char value) noexcept {
	if (str == nullptr) return nullptr;
	char* original = str;
	while (*str != '\0') {
		*str = value;
		++str;
	}
	return original;
}

MSTL_CONSTEXPR14 char* string_reverse(char* str) noexcept {
	if (str == nullptr || *str == '\0') return str;

	char* end = str;
	while (*end != '\0') ++end;
	--end;
	while (str < end) {
		const char temp = *str;
		*str = *end;
		*end = temp;
		++str;
		--end;
	}
	return str;
}


MSTL_CONSTEXPR14 char* string_n_copy(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	char* ret = dest;
	size_t i = 0;
	while (i < count && *src != '\0') {
		*dest = *src;
		dest++;
		src++;
		i++;
	}
	while (i < count) {
		*dest = '\0';
		dest++;
		i++;
	}
	return ret;
}

// stpncpy
MSTL_CONSTEXPR14 char* string_n_copy_offset(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, const size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	size_t i = 0;
	while (i < count && *src != '\0') {
		*dest = *src;
		dest++;
		src++;
		i++;
	}
	while (i < count) {
		*dest = '\0';
		dest++;
		i++;
	}
	return dest;
}

MSTL_CONSTEXPR14 char* string_n_concatenate(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

	char* original_dest = dest;
	while (*dest != '\0')
		++dest;
	size_t copied = 0;
	while (*src != '\0' && copied < count) {
		*dest = *src;
		++dest;
		++src;
		++copied;
	}
	*dest = '\0';
	return original_dest;
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 int string_n_compare(const char* dest, const char* src, size_t count) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	if (count == 0) return 0;
	size_t i = 0;
	while (*dest == *src && *dest != '\0' && i < count - 1) {
		++dest;
		++src;
		++i;
	}
	if (i == count - 1) return 0;
	return *dest < *src ? -1 : *dest > *src ? 1 : 0;
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 int string_n_compare_ignore_case(const char* s1, const char* s2, size_t count) noexcept {
	if ((s1 == nullptr && s2 == nullptr) || count == 0) return 0;
	if (s1 == nullptr) return -1;
	if (s2 == nullptr) return 1;

	size_t i = 0;
	while (*s1 && *s2 && i < count - 1) {
		const char c1 = to_lowercase(*s1);
		const char c2 = to_lowercase(*s2);
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		++s1;
		++s2;
		++i;
	}
	if (i == count - 1) return 0;

	const char c1 = to_lowercase(*s1);
	const char c2 = to_lowercase(*s2);
	return c1 < c2 ? -1 : c1 > c2 ? 1 : 0;
}

MSTL_PURE_FUNCTION MSTL_CONSTEXPR14 size_t string_n_length(const char* str, const size_t max_len) noexcept {
	const char* p = str;
	ptrdiff_t len = 0;
	while (*p != '\0' && len < max_len) {
		++p;
		++len;
	}
	return len;
}

MSTL_CONSTEXPR14 char* string_n_set(char* str, const char value, size_t count) noexcept {
	if (str == nullptr || count == 0) return str;
	char* original = str;
	size_t processed = 0;
	while (*str != '\0' && processed < count) {
		*str = value;
		++str;
		++processed;
	}
	return original;
}


// strlcpy
MSTL_CONSTEXPR14 size_t string_copy_safe(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, size_t size) noexcept {
	if (dest == nullptr || src == nullptr || size == 0) {
		return src != nullptr ? string_length(src) : 0;
	}
	size_t src_length = 0;
	const char* src_ptr = src;

	while (*src_ptr != '\0' && src_length < size - 1) {
		*dest = *src_ptr;
		dest++;
		src_ptr++;
		src_length++;
	}
	if (size > 0) *dest = '\0';

	while (*src_ptr != '\0') {
		src_ptr++;
		src_length++;
	}
	return src_length;
}

MSTL_CONSTEXPR14 size_t string_concatenate_safe(char* MSTL_RESTRICT dest, const char* MSTL_RESTRICT src, const size_t size) noexcept {
	const size_t src_size_len = string_n_length(src, size);
	if (dest == nullptr || src == nullptr || size == 0) {
		return src != nullptr ? src_size_len : 0;
	}

	size_t dest_length = string_n_length(dest, size);
	if (dest_length >= size) {
		return dest_length + src_size_len;
	}

	size_t remaining = size - dest_length - 1;
	const size_t src_length = string_n_length(src, remaining);

	if (src_length > 0) {
		char* dest_ptr = dest + dest_length;
		const char* src_ptr = src;

		while (*src_ptr != '\0' && remaining > 0) {
			*dest_ptr = *src_ptr;
			dest_ptr++;
			src_ptr++;
			remaining--;
		}
		*dest_ptr = '\0';
	}
	return dest_length + src_size_len;
}


MSTL_CONSTEXPR14 wchar_t* wchar_memory_copy(wchar_t* dest, const wchar_t* src, size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	wchar_t* res = dest;
	while (count--) {
		*dest = *src;
		++dest;
		++src;
	}
	return res;
}

MSTL_CONSTEXPR14 int wchar_memory_compare(const wchar_t* dest, const wchar_t* src, size_t count) noexcept {
	if (dest == nullptr && src == nullptr) return 0;
	if (dest == nullptr) return -1;
	if (src == nullptr) return 1;

	while (count--) {
		if (*dest != *src) return *dest - *src;
		++dest;
		++src;
	}
	return 0;
}

MSTL_CONSTEXPR14 wchar_t* wchar_memory_char(const wchar_t* dest, const wchar_t value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	const wchar_t* p = dest;
	while (count--) {
		if (*p == value)
			return const_cast<wchar_t*>(p);
		p++;
	}
	return nullptr;
}

MSTL_CONSTEXPR14 wchar_t* wchar_memory_move(wchar_t* dest, const wchar_t* src, const size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;
	return static_cast<wchar_t *>(memory_move(dest, src, count * sizeof(wchar_t)));
}

MSTL_CONSTEXPR14 wchar_t* wchar_memory_set(wchar_t* dest, const wchar_t value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	wchar_t* ret = dest;
	while (count--) {
		*dest = value;
		dest++;
	}
	return ret;
}


MSTL_CONSTEXPR14 ptrdiff_t wstring_length(const wchar_t* str) noexcept {
	const wchar_t* p = str;
	while (*p != L'\0')
		++p;
	return p - str;
}

#ifdef MSTL_VERSION_20__
MSTL_CONSTEXPR14 ptrdiff_t u8string_length(const char8_t* str) noexcept {
	const char8_t* p = str;
	while (*p != u8'\0')
		++p;
	return p - str;
}
#endif

MSTL_CONSTEXPR14 ptrdiff_t u16string_length(const char16_t* str) noexcept {
	const char16_t* p = str;
	while (*p != u'\0')
		++p;
	return p - str;
}

MSTL_CONSTEXPR14 ptrdiff_t u32string_length(const char32_t* str) noexcept {
	const char32_t* p = str;
	while (*p != U'\0')
		++p;
	return p - str;
}

MSTL_END_NAMESPACE__
#endif // MSTL_BASICLIB_HPP__
