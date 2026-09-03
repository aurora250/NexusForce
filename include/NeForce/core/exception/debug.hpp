#ifndef NEFORCE_CORE_EXCEPTION_DEBUG_HPP__
#define NEFORCE_CORE_EXCEPTION_DEBUG_HPP__

/**
 * @file debug.hpp
 * @brief 调试断点和断言工具
 *
 * 此文件提供了跨平台的调试断点触发和调试断言功能。
 * 支持检测调试器是否存在、触发断点、条件断点等调试辅助功能。
 */

#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup DebugAAssertionsAOptimize 调试、断言与优化
 * @brief 调试、断言与优化工具
 * @{
 */

#ifdef NEFORCE_STATE_DEBUG
/**
 * @def NEFORCE_DEBUG_VERIFY
 * @brief 调试模式断言
 *
 * 仅在调试模式下检查条件，发布模式下不产生任何代码。
 */
#    define NEFORCE_DEBUG_VERIFY(CON, MESG) \
        {                                   \
            if (CON) {                      \
            } else {                        \
                assert(false && MESG);      \
            }                               \
        }
#else
#    define NEFORCE_DEBUG_VERIFY(CON, MESG)
#endif

#ifdef NEFORCE_STANDARD_20
/**
 * @def NEFORCE_CONSTEXPR_ASSERT
 * @brief 编译时常量断言
 *
 * 在常量求值上下文中进行断言，如果条件为false则触发不可达代码。
 * 仅在C++20及以上版本有效。
 */
#    define NEFORCE_CONSTEXPR_ASSERT(COND)                         \
        do {                                                       \
            if (_NEFORCE is_constant_evaluated() && !bool(COND)) { \
                _NEFORCE unreachable();                            \
            }                                                      \
        } while (false);
#else
#    define NEFORCE_CONSTEXPR_ASSERT(COND)
#endif


/**
 * @brief 标记不可达代码路径
 *
 * 该函数用于向编译器指示当前代码路径永远不会被执行。当编译器遇到此调用时，
 * 可以进行激进的优化，假设此后的代码永远不会运行。如果实际执行到了此函数，
 * 将导致未定义行为（通常是程序崩溃或产生不可预测的结果）。
 *
 * @note 此函数永远不会返回，调用后程序行为未定义。
 * @warning 仅在确定代码路径绝对不可达时使用，否则会导致严重的运行时问题。
 */
NEFORCE_NORETURN NEFORCE_ALWAYS_INLINE_INLINE void unreachable() noexcept {
#ifdef NEFORCE_COMPILER_GNUC
    __builtin_unreachable();
#else
    __assume(false);
#endif
}


/**
 * @brief 向编译器提示条件表达式很可能为真
 * @param x 条件表达式
 * @return 条件表达式的值
 * @warning 错误使用可能降低性能，仅在确实有概率偏差时使用
 */
NEFORCE_ALWAYS_INLINE_INLINE bool likely(bool x) {
#ifdef NEFORCE_COMPILER_GNUC
    return static_cast<bool>(__builtin_expect(static_cast<long>(x), 1L));
#else
    return x;
#endif
}

/**
 * @brief 向编译器提示条件表达式很可能为假
 * @param x 条件表达式
 * @return 条件表达式的值
 * @warning 错误使用可能降低性能，仅在确实有概率偏差时使用
 */
NEFORCE_ALWAYS_INLINE_INLINE bool unlikely(bool x) {
#ifdef NEFORCE_COMPILER_GNUC
    return static_cast<bool>(__builtin_expect(static_cast<long>(x), 0L));
#else
    return x;
#endif
}


/**
 * @brief 检查当前上下文是否在常量求值中
 * @return 如果在常量求值上下文中返回true，否则返回false
 *
 * 用于区分编译时和运行时
 */
NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr bool is_constant_evaluated() noexcept {
    return __builtin_is_constant_evaluated();
}

/**
 * @brief 检测当前进程是否正在被调试器附加
 * @return 如果正在调试则返回true，否则返回false
 */
NEFORCE_NODISCARD bool NEFORCE_API is_debugger_present() noexcept;

/**
 * @brief 调试断言
 * @param condition 条件表达式
 * @param message 断言失败时的消息
 *
 * 当条件为false时，输出断言失败信息并触发调试断点（如果正在调试）。
 * 仅在调试构建中有效，发布构建中此函数为空操作。
 */
void NEFORCE_API debug_assert(bool condition, const char* message = nullptr);

/**
 * @brief 触发调试断点
 *
 * 此函数会直接触发断点，无论是否有调试器附加。
 */
NEFORCE_ALWAYS_INLINE_INLINE void breakpoint() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    __debugbreak();
#else
#    ifdef NEFORCE_COMPILER_CLANG
    __builtin_debugtrap();
#    elif defined(NEFORCE_ARCH_X86)
    asm volatile("int3");
#    elif defined(NEFORCE_ARCH_AARCH64)
    asm volatile(".inst 0xd4200000");
#    elif defined(NEFORCE_ARCH_ARM32)
    asm volatile(".inst 0xe7f001f0");
#    elif defined(NEFORCE_ARCH_RISCV)
    asm volatile("ebreak");
#    elif defined(NEFORCE_ARCH_LOONGARCH)
    asm volatile("break 0");
#    else
    __builtin_trap();
#    endif
#endif
}

/**
 * @brief 如果正在调试则触发断点
 *
 * 首先检查是否有调试器附加，如果有则触发断点。
 * 适用于条件性断点，避免在非调试环境中意外中断。
 */
NEFORCE_ALWAYS_INLINE_INLINE void breakpoint_if_debugging() {
    if (is_debugger_present()) {
        breakpoint();
    }
}

/** @} */ // DebugAndAssertions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_DEBUG_HPP__
