#ifndef NEFORCE_CORE_EXCEPTION_BREAKPOINT_HPP__
#define NEFORCE_CORE_EXCEPTION_BREAKPOINT_HPP__

/**
 * @file breakpoint.hpp
 * @brief 调试断点和断言工具
 *
 * 此文件提供了跨平台的调试断点触发和调试断言功能。
 * 支持检测调试器是否存在、触发断点、条件断点等调试辅助功能。
 */

#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup DebugBreakpoints 调试断点
 * @brief 调试断点和断言工具
 * @{
 */

/**
 * @brief 检测当前进程是否正在被调试器附加
 * @return 如果正在调试则返回true，否则返回false
 */
bool NEFORCE_API is_debugger_present() noexcept;

/**
 * @brief 调试断言
 * @param condition 条件表达式
 * @param message 断言失败时的消息
 *
 * 当条件为false时，输出断言失败信息并触发调试断点（如果正在调试）。
 * 仅在调试构建中有效，发布构建中此函数为空操作。
 */
void NEFORCE_API debug_assert(bool condition, const char* message = nullptr) noexcept;

/**
 * @brief 触发调试断点
 *
 * 此函数会直接触发断点，无论是否有调试器附加。
 */
NEFORCE_ALWAYS_INLINE_INLINE void breakpoint() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    __debugbreak();
#else
#ifdef NEFORCE_COMPILER_CLANG
    __builtin_debugtrap();
#elif defined(NEFORCE_ARCH_X86)
    asm volatile("int3");
#elif defined(NEFORCE_ARCH_AARCH64)
    asm volatile(".inst 0xd4200000");
#elif defined(NEFORCE_ARCH_ARM32)
    asm volatile(".inst 0xe7f001f0");
#elif defined(NEFORCE_ARCH_RISCV)
    asm volatile("ebreak");
#elif defined(NEFORCE_ARCH_LOONGARCH)
    asm volatile("break 0");
#else
    __builtin_trap();
#endif
#endif
}

/**
 * @brief 如果正在调试则触发断点
 *
 * 首先检查是否有调试器附加，如果有则触发断点。
 * 适用于条件性断点，避免在非调试环境中意外中断。
 */
NEFORCE_ALWAYS_INLINE_INLINE void breakpoint_if_debugging() noexcept {
    if (is_debugger_present()) {
        breakpoint();
    }
}

/** @} */ // DebugBreakpoints

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_BREAKPOINT_HPP__
