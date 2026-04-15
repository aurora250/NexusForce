#ifndef NEFORCE_CORE_ASYNC_THIS_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_THIS_THREAD_HPP__

/**
 * @file this_thread.hpp
 * @brief 当前线程操作
 *
 * 此文件提供了跨平台的当前线程操作函数。
 */

#include "NeForce/core/typeinfo/types.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include "NeForce/core/config/windef.hpp"
#    include <processthreadsapi.h>
#    include <synchapi.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <sched.h>
#    include <unistd.h>
#endif
#ifdef NEFORCE_ARCH_RISCV
#    include <riscv_pause.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_THIS_THREAD__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @brief 获取当前CPU核心编号
 * @return 当前线程运行的CPU核心编号
 *
 * 返回当前线程正在运行的CPU核心的逻辑编号。
 */
NEFORCE_ALWAYS_INLINE_INLINE int64_t current_cpu() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return static_cast<int64_t>(::GetCurrentProcessorNumber());
#else
    return static_cast<int64_t>(::sched_getcpu());
#endif
}

/**
 * @brief 让出当前线程的时间片
 *
 * 主动让出当前线程的CPU时间片，让其他线程可以运行。
 */
NEFORCE_ALWAYS_INLINE_INLINE void yield() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SwitchToThread();
#else
    ::sched_yield();
#endif
}

/**
 * @brief 线程放松
 *
 * 使用CPU特定的暂停指令让当前线程短暂放松，减少CPU功耗。
 * 适用于自旋锁等场景中的忙等待。
 */
NEFORCE_ALWAYS_INLINE_INLINE void relax() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::_mm_pause();
#else
#    if defined(NEFORCE_ARCH_X86)
    __builtin_ia32_pause();
#    elif defined(NEFORCE_ARCH_ARM)
    asm volatile("yield" ::: "memory");
#    elif defined(NEFORCE_ARCH_RISCV)
    ::riscv_pause();
#    elif defined(NEFORCE_ARCH_LOONGARCH)
    asm volatile("dbar 0" ::: "memory");
#    else
    this_thread::yield();
#    endif
#endif
}

/**
 * @brief 根据计数进行线程放松
 * @param count 放松计数
 *
 * 根据计数值选择不同程度的放松策略：
 * - 小计数：多次调用relax()
 * - 中等计数：调用一次relax()
 * - 大计数：直接让出时间片
 */
NEFORCE_ALWAYS_INLINE_INLINE void relax(const int count) noexcept {
    if (count < 4) {
        for (int i = 0; i < (1 << count); ++i) {
            this_thread::relax();
        }
    } else if (count < 8) {
        this_thread::relax();
    } else {
        this_thread::yield();
    }
}

/**
 * @brief 睡眠指定毫秒数
 * @param milliseconds 要睡眠的毫秒数
 *
 * 让当前线程睡眠指定的时间。
 *
 * @note 使用系统睡眠，实际睡眠时间受系统调度影响。
 */
NEFORCE_ALWAYS_INLINE_INLINE void sleep_for_ms(uint32_t milliseconds) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::Sleep(milliseconds);
#else
    ::usleep(milliseconds * 1000);
#endif
}

/**
 * @brief 精确睡眠指定毫秒数
 * @param ms 要睡眠的毫秒数
 * @param busy_wait 是否使用忙等待获得更高精度
 *
 * 提供相对精确的睡眠功能。当use_busy_wait为true时，使用忙等待+系统睡眠组合
 * 以获得更高的精度，但会增加CPU使用率。为false时使用纯系统睡眠。
 *
 * @note Windows平台最小精度通常为1-15ms，Linux为1ms，忙等待可达到微秒级精度
 */
void NEFORCE_API sleep_for_ms(uint32_t ms, bool busy_wait) noexcept;

/**
 * @brief 精确睡眠指定微秒数
 * @param us 要睡眠的微秒数
 *
 * 提供微秒级精度的睡眠，主要用于短时间延迟。
 *
 * @note 实际精度受系统时钟精度限制。
 */
void NEFORCE_API sleep_for_us(uint64_t us) noexcept;

/**
 * @brief 精确睡眠指定纳秒数
 * @param ns 要睡眠的纳秒数
 *
 * 提供纳秒级精度的睡眠，主要用于极短时间延迟。
 *
 * @note 实际精度受CPU频率和指令执行时间限制。
 */
void NEFORCE_API sleep_for_ns(uint64_t ns) noexcept;

/**
 * @brief 设置线程的CPU亲和性
 * @param cpu_mask CPU掩码，每个位表示一个CPU核心
 * @return 设置是否成功
 *
 * 设置当前线程可以在哪些CPU核心上运行。
 */
bool NEFORCE_API affinity(size_t cpu_mask) noexcept;

/**
 * @brief 设置线程优先级
 * @param priority 优先级值
 * @return 设置是否成功
 *
 * 设置当前线程的调度优先级。
 */
bool NEFORCE_API priority(int priority) noexcept;

/** @} */ // Thread

/** @} */ // AsyncComponents

NEFORCE_END_THIS_THREAD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THIS_THREAD_HPP__
