#ifndef NEFORCE_CORE_CONFIG_CPPCONFIG_HPP__
#define NEFORCE_CORE_CONFIG_CPPCONFIG_HPP__

/**
 * @file c++config.hpp
 * @brief 核心配置头文件
 *
 * 此头文件定义了整个库的平台、编译器和语言特性的配置宏。
 * 是所有其他头文件的基础依赖
 *
 * 主要功能：
 * - 平台检测
 * - 编译器检测
 * - 指令集架构检测
 * - C++标准版本检测
 * - 命名空间宏定义
 * - 编译器特定属性宏
 *
 * @note 项目内部使用的宏将不写入文档，具体您可以查看本文件内容
 */

#include <cassert>

/**
 * @defgroup PlatformDetection 平台检测
 * @brief 检测和定义目标平台的宏
 * @{
 */

#if defined(WIN32) || defined(_WIN32) || defined(_M_X86)

/**
 * @def NEFORCE_PLATFORM_WINDOWS
 * @brief 定义在Windows平台编译
 */
#    define NEFORCE_PLATFORM_WINDOWS 1

/**
 * @def NEFORCE_PLATFORM_WINDOWS32
 * @brief 定义在32位Windows平台编译
 */
#    define NEFORCE_PLATFORM_WINDOWS32 1

#    if defined(WIN64) || defined(_WIN64) || defined(_M_X64)
/**
 * @def NEFORCE_PLATFORM_WINDOWS64
 * @brief 定义在64位Windows平台编译
 */
#        define NEFORCE_PLATFORM_WINDOWS64 1
#    endif
#endif

#if defined(__linux__)
/**
 * @def NEFORCE_PLATFORM_LINUX
 * @brief 定义在Linux平台编译
 */
#    define NEFORCE_PLATFORM_LINUX 1

#    if (__WORDSIZE == 32) || (__SIZEOF_POINTER__ == 4) || defined(NEFORCE_PLATFORM_LINUX64) || \
            defined(NEXUSFORCE_ENABLE_DOXYGEN)
/**
 * @def NEFORCE_PLATFORM_LINUX32
 * @brief 定义在32位Linux平台编译
 */
#        define NEFORCE_PLATFORM_LINUX32 1
#    endif

#    if (__WORDSIZE == 64) || (__SIZEOF_POINTER__ == 8)
/**
 * @def NEFORCE_PLATFORM_LINUX64
 * @brief 定义在64位Linux平台编译
 */
#        define NEFORCE_PLATFORM_LINUX64 1
#    endif
#endif

#if !(defined(NEFORCE_PLATFORM_WINDOWS) || defined(NEFORCE_PLATFORM_LINUX))
#    error "NeForce: 不支持的操作系统"
#endif


#ifdef NEFORCE_PLATFORM_WINDOWS

#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif

#    ifndef NOMINMAX
#        define NOMINMAX
#    endif

#    ifdef bool
#        undef bool
#    endif

#    ifdef true
#        undef true
#    endif

#    ifdef false
#        undef false
#    endif

#    ifdef max
#        undef max
#    endif

#    ifdef min
#        undef min
#    endif

#endif

/** @} */ // PlatformDetection

/**
 * @defgroup CompilerDetection 编译器检测
 * @brief 检测和定义编译器的宏
 * @{
 */

#if defined(__clang__)
/**
 * @def NEFORCE_COMPILER_CLANG
 * @brief 定义使用Clang编译器编译
 */
#    define NEFORCE_COMPILER_CLANG 1
#endif

#if defined(__GNUC__)
/**
 * @def NEFORCE_COMPILER_GNUC
 * @brief 定义使用GNU编译器编译
 */
#    define NEFORCE_COMPILER_GNUC 1
#endif

#if defined(_MSC_VER)
/**
 * @def NEFORCE_COMPILER_MSVC
 * @brief 定义使用Microsoft Visual C++编译器编译
 */
#    define NEFORCE_COMPILER_MSVC 1

#    if defined(NEFORCE_COMPILER_CLANG)
#        define NEFORCE_COMPILER_CLANG_CL 1
#    endif
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
/**
 * @def NEFORCE_COMPILER_GCC
 * @brief 定义使用GCC编译器编译
 */
#    define NEFORCE_COMPILER_GCC 1
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
/**
 * @def NEFORCE_COMPILER_MINGW
 * @brief 定义使用MinGW编译器编译
 */
#    define NEFORCE_COMPILER_MINGW 1
#    if defined(NEFORCE_COMPILER_CLANG)
/**
 * @def NEFORCE_COMPILER_LLVM_MINGW
 * @brief 定义使用LLVM MinGW编译器编译
 */
#        define NEFORCE_COMPILER_LLVM_MINGW 1
#    endif
#endif

#if !(defined(NEFORCE_COMPILER_GNUC) || defined(NEFORCE_COMPILER_MSVC))
#    error "NeForce: 不支持的编译器"
#endif

#ifdef __clang_analyzer__
#    define NEFORCE_ANALYZER_CLANG 1
#endif

/** @} */ // CompilerDetection

/**
 * @defgroup APIImpExpSpec API导入导出规范
 * @brief 动态库导入导出声明
 * @{
 */

#if defined(NEFORCE_COMPILER_MSVC)
/**
 * @def NEFORCE_API_EXPORT_DLL
 * @brief 在MSVC编译器下使用DLL导出
 */
#    define NEFORCE_API_EXPORT_DLL __declspec(dllexport)
/**
 * @def NEFORCE_API_IMPORT_DLL
 * @brief 在MSVC编译器下使用DLL导入
 */
#    define NEFORCE_API_IMPORT_DLL __declspec(dllimport)
#endif

#if defined(NEFORCE_COMPILER_GNUC)
/**
 * @def NEFORCE_API_EXPORT
 * @brief 在GNUC编译器下使用空定义，无需显式的导入导出辅助
 */
#    define NEFORCE_API_EXPORT
#endif

#if defined(NEFORCE_COMPILER_GNUC)
#    define NEFORCE_API NEFORCE_API_EXPORT
#else
#    if defined(NEFORCE_DLLEXPORTS)
#        define NEFORCE_API NEFORCE_API_EXPORT_DLL
#    else
#        define NEFORCE_API NEFORCE_API_IMPORT_DLL
#    endif
#endif

/** @} */ // APIImpExpSpec

/**
 * @defgroup ArchitectureDetection 架构检测
 * @brief CPU架构检测宏
 * @{
 */

#if defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || \
        defined(__I86__) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    define NEFORCE_ARCH_X86_32 1 ///< 32位x86架构
#endif

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || \
        defined(_M_AMD64) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    define NEFORCE_ARCH_X86_64 1 ///< 64位x86_64架构
#endif

#if defined(NEFORCE_ARCH_X86_32) || defined(NEFORCE_ARCH_X86_64)
#    define NEFORCE_ARCH_X86 1 ///< x86架构
#endif


#if defined(__arm__) || defined(__arm) || defined(_ARM_) || defined(_M_ARM) || defined(__ARM_ARCH) || \
        defined(__TARGET_ARCH_ARM) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    define NEFORCE_ARCH_ARM32 1 ///< 32位ARM架构
#endif

#if defined(__aarch64__) || defined(__aarch64) || defined(_M_ARM64) || defined(__ARM64_ARCH_8__) || \
        defined(__ARM_ARCH_ISA_A64) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    define NEFORCE_ARCH_AARCH64 1 ///< 64位AArch64架构
#endif

#if defined(NEFORCE_ARCH_ARM32) || defined(NEFORCE_ARCH_AARCH64)
#    define NEFORCE_ARCH_ARM 1 ///< ARM架构
#endif


#if defined(__riscv) || defined(__riscv__) || defined(riscv) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    define NEFORCE_ARCH_RISCV 1 ///< RISC-V架构
#    if __riscv_xlen == 32 || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#        define NEFORCE_ARCH_RISCV32 1 ///< 32位RISC-V
#    endif
#    if __riscv_xlen == 64 || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#        define NEFORCE_ARCH_RISCV64 1 ///< 64位RISC-V
#    endif
#endif


#if defined(__loongarch__) || defined(__loongarch) || defined(__loongarch32) || defined(__loongarch64) || \
        defined(_LOONGARCH_SIM) || defined(_LOONGARCH) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    define NEFORCE_ARCH_LOONGARCH 1 ///< LoongArch架构
#    if defined(__loongarch32) || defined(_LOONGARCH_SIM == _ABILP32_SIM) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#        define NEFORCE_ARCH_LOONGARCH32 1 ///< 32位LoongArch
#    endif
#    if defined(__loongarch64) || defined(_LOONGARCH_SIM == _ABILP64_SIM) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#        define NEFORCE_ARCH_LOONGARCH64 1 ///< 64位LoongArch
#    endif
#endif

#if !(defined(NEFORCE_ARCH_X86) || defined(NEFORCE_ARCH_ARM) || defined(NEFORCE_ARCH_RISCV) || \
      defined(NEFORCE_ARCH_LOONGARCH))
#    error "NeForce: 不支持的指令集"
#endif

/** @} */ // ArchitectureDetection

/**
 * @defgroup DataBusWidth 数据总线宽度
 * @brief 系统架构位宽检测
 * @{
 */

#if defined(NEFORCE_ARCH_X86_64) || defined(NEFORCE_ARCH_AARCH64) || defined(NEFORCE_ARCH_RISCV64) || \
        defined(NEFORCE_ARCH_LOONGARCH64) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
/**
 * @def NEFORCE_ARCH_BITS_64
 * @brief 定义在64位系统编译
 */
#    define NEFORCE_ARCH_BITS_64 1
#endif
#if defined(NEFORCE_ARCH_X86_32) || defined(NEFORCE_ARCH_ARM32) || defined(NEFORCE_ARCH_RISCV32) || \
        defined(NEFORCE_ARCH_LOONGARCH32) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
/**
 * @def NEFORCE_ARCH_BITS_32
 * @brief 定义在32位系统编译
 */
#    define NEFORCE_ARCH_BITS_32 1
#endif

#if !(defined(NEFORCE_ARCH_BITS_64) || defined(NEFORCE_ARCH_BITS_32))
#    error "NeForce: 不支持的位宽"
#endif

/** @} */ // DataBusWidth


#if defined(NEFORCE_ARCH_X86) && defined(NEFORCE_USING_INTEL_TSX)
#    define NEFORCE_SUPPORT_INTEL_TSX 1
#endif

#ifdef NEFORCE_ARCH_BITS_64
#    define NEFORCE_SUPPORT_INTRINSIC_INT128 1
#endif


/**
 * @defgroup CxxStandardDetection C++标准检测
 * @brief 检测和定义C++语言标准的宏
 * @{
 */

#if (__cplusplus >= 202100L) || (_MSVC_LANG >= 202100L)
/**
 * @def NEFORCE_STANDARD_23
 * @brief 使用C++23或更高标准编译
 */
#    define NEFORCE_STANDARD_23 1
#endif
#if (__cplusplus >= 202002L) || defined(NEFORCE_STANDARD_23) || (_MSVC_LANG >= 202002L)
/**
 * @def NEFORCE_STANDARD_20
 * @brief 使用C++20或更高标准编译
 */
#    define NEFORCE_STANDARD_20 1
#endif
#if (__cplusplus >= 201703L) || defined(NEFORCE_STANDARD_20) || (_MSVC_LANG >= 201703L)
/**
 * @def NEFORCE_STANDARD_17
 * @brief 使用C++17或更高标准编译
 */
#    define NEFORCE_STANDARD_17 1
#endif
#if (__cplusplus >= 201402L) || defined(NEFORCE_STANDARD_17) || (_MSVC_LANG >= 201402L)
/**
 * @def NEFORCE_STANDARD_14
 * @brief 使用C++14或更高标准编译
 */
#    define NEFORCE_STANDARD_14 1
#endif
#if (__cplusplus >= 201103L) || defined(NEFORCE_STANDARD_14) || (_MSVC_LANG >= 201103L)
/**
 * @def NEFORCE_STANDARD_11
 * @brief 使用C++11或更高标准编译
 */
#    define NEFORCE_STANDARD_11 1
#endif
#if (__cplusplus >= 199711L) || defined(NEFORCE_STANDARD_11) || (_MSVC_LANG >= 199711L)
/**
 * @def NEFORCE_STANDARD_98
 * @brief 使用C++98或更高标准编译
 */
#    define NEFORCE_STANDARD_98 1
#endif

/** @} */ // CxxStandardDetection


#define __NEFORCE_GLOBAL_NAMESPACE__ neforce
#define NEFORCE_BEGIN_NAMESPACE__ namespace __NEFORCE_GLOBAL_NAMESPACE__ {
#define NEFORCE_END_NAMESPACE__ }
#define _NEFORCE __NEFORCE_GLOBAL_NAMESPACE__ ::

#define __NEFORCE_INNER_NAMESPACE__ inner
#define NEFORCE_BEGIN_INNER__ namespace __NEFORCE_INNER_NAMESPACE__ {
#define NEFORCE_END_INNER__ }

#define __NEFORCE_CONSTANTS_NAMESPACE__ constants
#define NEFORCE_BEGIN_CONSTANTS__ namespace __NEFORCE_CONSTANTS_NAMESPACE__ {
#define NEFORCE_END_CONSTANTS__ }

#define __NEFORCE_THIS_THREAD_NAMESPACE__ this_thread
#define NEFORCE_BEGIN_THIS_THREAD__ namespace __NEFORCE_THIS_THREAD_NAMESPACE__ {
#define NEFORCE_END_THIS_THREAD__ }

#define __NEFORCE_RANGES_NAMESPACE__ ranges
#define NEFORCE_BEGIN_RANGES__ namespace __NEFORCE_RANGES_NAMESPACE__ {
#define NEFORCE_END_RANGES__ }

#define __NEFORCE_RANGES_VIEWS_NAMESPACE__ views
#define NEFORCE_BEGIN_RANGES_VIEWS__ namespace __NEFORCE_RANGES_VIEWS_NAMESPACE__ {
#define NEFORCE_END_RANGES_VIEWS__ }

#define __NEFORCE_LITERALS_NAMESPACE__ literals
#define NEFORCE_BEGIN_LITERALS__ inline namespace __NEFORCE_LITERALS_NAMESPACE__ {
#define NEFORCE_END_LITERALS__ }

#define __NEFORCE_REFLECT_NAMESPACE__ reflect
#define NEFORCE_BEGIN_REFLECT__ namespace __NEFORCE_REFLECT_NAMESPACE__ {
#define NEFORCE_END_REFLECT__ }

#define __NEFORCE_HTTP_NAMESPACE__ http
#define NEFORCE_BEGIN_HTTP__ namespace __NEFORCE_HTTP_NAMESPACE__ {
#define NEFORCE_END_HTTP__ }


#ifdef NEFORCE_STANDARD_11
#    define NEFORCE_CONSTEXPR11 constexpr
#else
#    define NEFORCE_CONSTEXPR11 inline
#endif // NEFORCE_STANDARD_11

#ifdef NEFORCE_STANDARD_14
#    define NEFORCE_CONSTEXPR14 constexpr
#else
#    define NEFORCE_CONSTEXPR14 inline
#endif // NEFORCE_STANDARD_14

#ifdef NEFORCE_STANDARD_17
#    define NEFORCE_CONSTEXPR17 constexpr
#    define NEFORCE_INLINE17 inline
#else
#    define NEFORCE_CONSTEXPR17 inline
#    define NEFORCE_INLINE17
#endif // NEFORCE_STANDARD_17

#ifdef NEFORCE_STANDARD_20
#    define NEFORCE_CONSTEXPR20 constexpr
#else
#    define NEFORCE_CONSTEXPR20 inline
#endif // NEFORCE_STANDARD_20

#ifdef NEFORCE_STANDARD_23
#    define NEFORCE_CONSTEXPR23 constexpr
#else
#    define NEFORCE_CONSTEXPR23 inline
#endif // NEFORCE_STANDARD_23


#ifdef NEFORCE_STANDARD_17
#    define NEFORCE_IF_CONSTEXPR if constexpr
#else
#    define NEFORCE_IF_CONSTEXPR if
#endif


#ifdef NEFORCE_STANDARD_17
#    define NEFORCE_NODISCARD [[nodiscard]]

#    define NEFORCE_ALLOC_NODISCARD [[nodiscard("discard the return of allocators will cause memory leaks.")]]
#else
#    define NEFORCE_NODISCARD

#    define NEFORCE_ALLOC_NODISCARD
#endif


#ifdef NEFORCE_COMPILER_GNUC
#    define NEFORCE_ALWAYS_INLINE __attribute__((always_inline))
#    define NEFORCE_ALWAYS_INLINE_INLINE NEFORCE_ALWAYS_INLINE inline
#elif defined(NEFORCE_COMPILER_MSVC)
#    define NEFORCE_ALWAYS_INLINE __forceinline
#    define NEFORCE_ALWAYS_INLINE_INLINE NEFORCE_ALWAYS_INLINE
#else
#    define NEFORCE_ALWAYS_INLINE inline
#    define NEFORCE_ALWAYS_INLINE_INLINE inline
#endif


#ifdef NEFORCE_COMPILER_GNUC
#    define NEFORCE_UNUSED __attribute__((unused))
#else
#    define NEFORCE_UNUSED
#endif


#ifdef NEFORCE_STANDARD_17
#    define NEFORCE_UNLIKELY [[unlikely]]
#else
#    define NEFORCE_UNLIKELY
#endif


#ifdef NEFORCE_STANDARD_20
#    define NEFORCE_LIKELY [[likely]]
#else
#    define NEFORCE_LIKELY
#endif


#if defined(NEFORCE_STANDARD_11)
#    define NEFORCE_NORETURN [[noreturn]]
#elif defined(NEFORCE_COMPILER_GNUC)
#    define NEFORCE_NORETURN __attribute__((noreturn))
#elif defined(NEFORCE_COMPILER_MSVC)
#    define NEFORCE_NORETURN __declspec(noreturn)
#else
#    define NEFORCE_NORETURN
#endif


#ifndef NEFORCE_COMPILER_GNUC
#    define NEFORCE_PURE_FUNCTION
#    define NEFORCE_CONST_FUNCTION
#else
#    define NEFORCE_PURE_FUNCTION __attribute__((__pure__))
#    define NEFORCE_CONST_FUNCTION __attribute__((__const__))
#endif


#ifdef NEFORCE_STANDARD_14
#    define NEFORCE_DEPRECATED [[deprecated]]
#    define NEFORCE_DEPRECATED_FOR(MSG) [[deprecated(MSG)]]
#else
#    define NEFORCE_DEPRECATED
#    define NEFORCE_DEPRECATED_FOR(MSG)
#endif


#if defined(NEFORCE_COMPILER_GNUC)
#    define NEFORCE_NOVTABLE __attribute__((novtable))
#elif defined(NEFORCE_COMPILER_MSVC)
#    define NEFORCE_NOVTABLE __declspec(novtable)
#else
#    define NEFORCE_NOVTABLE
#endif


#if defined(NEFORCE_COMPILER_GNUC)
#    define NEFORCE_ALLOC_OPTIMIZE NEFORCE_ALWAYS_INLINE
#elif defined(NEFORCE_COMPILER_MSVC)
#    define NEFORCE_ALLOC_OPTIMIZE __declspec(allocator)
#else
#    define NEFORCE_ALLOC_OPTIMIZE
#endif


#if defined(NEFORCE_COMPILER_GNUC)
#    define NEFORCE_RESTRICT __restrict__
#else
#    define NEFORCE_RESTRICT __restrict
#endif


/**
 * @defgroup TypeRangeMacros 类型范围宏
 * @brief 用于遍历类型列表的宏
 *
 * 提供宏定义，用于在代码生成时遍历各种类型列表。
 * @{
 */

#ifdef NEFORCE_STANDARD_20
/**
 * @def NEFORCE_MACRO_RANGE_UNICODE_CHARS
 * @brief Unicode字符类型列表宏
 */
#    define NEFORCE_MACRO_RANGE_UNICODE_CHARS(MAC) \
        MAC(char8_t)                               \
        MAC(char16_t)                              \
        MAC(char32_t)
#else
#    define NEFORCE_MACRO_RANGE_UNICODE_CHARS(MAC) \
        MAC(char16_t)                              \
        MAC(char32_t)
#endif

/**
 * @def NEFORCE_MACRO_RANGE_CHARS
 * @brief 所有字符类型列表宏
 */
#define NEFORCE_MACRO_RANGE_CHARS(MAC) \
    MAC(char)                          \
    MAC(wchar_t)                       \
    NEFORCE_MACRO_RANGE_UNICODE_CHARS(MAC)

/**
 * @def NEFORCE_MACRO_RANGE_SIGNED_INT
 * @brief 有符号整数类型列表宏
 */
#define NEFORCE_MACRO_RANGE_SIGNED_INT(MAC) \
    MAC(signed char)                        \
    MAC(short)                              \
    MAC(int)                                \
    MAC(long)                               \
    MAC(long long)

/**
 * @def NEFORCE_MACRO_RANGE_UNSIGNED_INT
 * @brief 无符号整数类型列表宏
 */
#define NEFORCE_MACRO_RANGE_UNSIGNED_INT(MAC) \
    MAC(unsigned char)                        \
    MAC(unsigned short)                       \
    MAC(unsigned int)                         \
    MAC(unsigned long)                        \
    MAC(unsigned long long)

/**
 * @def NEFORCE_MACRO_RANGE_INT
 * @brief 所有整数类型列表宏
 */
#define NEFORCE_MACRO_RANGE_INT(MAC)    \
    NEFORCE_MACRO_RANGE_SIGNED_INT(MAC) \
    NEFORCE_MACRO_RANGE_UNSIGNED_INT(MAC)

/**
 * @def NEFORCE_MACRO_RANGE_FLOAT
 * @brief 浮点类型列表宏
 */
#define NEFORCE_MACRO_RANGE_FLOAT(MAC) \
    MAC(float)                         \
    MAC(double)                        \
    MAC(long double)

/**
 * @def NEFORCE_MACRO_RANGE_ARITHMETIC
 * @brief 所有算术类型列表宏
 */
#define NEFORCE_MACRO_RANGE_ARITHMETIC(MAC) \
    NEFORCE_MACRO_RANGE_CHARS(MAC)          \
    NEFORCE_MACRO_RANGE_INT(MAC)            \
    NEFORCE_MACRO_RANGE_FLOAT(MAC)

/**
 * @def NEFORCE_MACRO_RANGES_CV
 * @brief cv限定符列表宏
 */
#define NEFORCE_MACRO_RANGES_CV(MAC) \
    MAC(const)                       \
    MAC(volatile)                    \
    MAC(const volatile)

/**
 * @def NEFORCE_MACRO_RANGES_CV_REF
 * @brief cv和引用限定符列表宏
 */
#define NEFORCE_MACRO_RANGES_CV_REF(MAC) \
    MAC(&)                               \
    MAC(const&)                          \
    MAC(volatile&)                       \
    MAC(const volatile&)                 \
    MAC(&&)                              \
    MAC(const&&)                         \
    MAC(volatile&&)                      \
    MAC(const volatile&&)

/**
 * @def NEFORCE_MACRO_RANGES_CV_REF_NOEXCEPT
 * @brief cv、引用和noexcept限定符列表宏
 */
#define NEFORCE_MACRO_RANGES_CV_REF_NOEXCEPT(MAC) \
    MAC(noexcept)                                 \
    MAC(const noexcept)                           \
    MAC(volatile noexcept)                        \
    MAC(const volatile noexcept)                  \
    MAC(& noexcept)                               \
    MAC(const& noexcept)                          \
    MAC(volatile& noexcept)                       \
    MAC(const volatile& noexcept)                 \
    MAC(&& noexcept)                              \
    MAC(const&& noexcept)                         \
    MAC(volatile&& noexcept)                      \
    MAC(const volatile&& noexcept)

/** @} */ // TypeRangeMacros

#endif // NEFORCE_CORE_CONFIG_CPPCONFIG_HPP__
