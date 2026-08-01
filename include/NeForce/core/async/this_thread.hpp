#ifndef NEFORCE_CORE_ASYNC_THIS_THREAD_HPP__
#define NEFORCE_CORE_ASYNC_THIS_THREAD_HPP__

/**
 * @file this_thread.hpp
 * @brief 当前线程操作
 *
 * 此文件提供了跨平台的当前线程操作函数。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
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
#    include <ctime>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @struct cpu_times
 * @brief CPU时间信息类
 */
struct cpu_times {
    uint64_t user;   ///< 用户态时间
    uint64_t kernel; ///< 内核态时间
    uint64_t idle;   ///< 空闲时间
    NEFORCE_NODISCARD uint64_t total() const noexcept { return user + kernel + idle; }
};

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
#ifdef NEFORCE_COMPILER_MSVC
#    ifdef NEFORCE_ARCH_X86
    ::_mm_pause();
#    else
    ::__yield();
#    endif
#else
#    if defined(NEFORCE_ARCH_X86)
    asm volatile("pause" ::: "memory");
#    elif defined(NEFORCE_ARCH_ARM)
    asm volatile("yield" ::: "memory");
#    elif defined(NEFORCE_ARCH_RISCV)
#        ifdef __riscv_zihintpause
    asm volatile("pause" ::: "memory");
#        else
    asm volatile("fence" ::: "memory");
#        endif
#    elif defined(NEFORCE_ARCH_LOONGARCH)
    asm volatile("dbar 0" ::: "memory");
#    else
    this_thread::yield();
#    endif
#endif
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
    ::timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = static_cast<decay_t<decltype(ts.tv_nsec)>>(milliseconds % 1000) * 1000000;
    ::nanosleep(&ts, nullptr);
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
 * @brief 设置线程的 CPU 亲和性
 * @param cpu_mask CPU 掩码，每个位表示一个 CPU 核心
 * @return 设置是否成功
 *
 * 设置当前线程可以在哪些 CPU 核心上运行。
 */
bool NEFORCE_API set_affinity(size_t cpu_mask) noexcept;

/**
 * @brief 获取线程的 CPU 亲和性
 * @param affi CPU 亲和性
 * @return 获取是否成功
 */
bool NEFORCE_API affinity(uint64_t& affi) noexcept;

/**
 * @brief 获取当前线程的 CPU 时间
 * @param times 用户态和内核态时间
 * @return 成功返回 true
 */
bool NEFORCE_API cpu_time(cpu_times& times) noexcept;

/**
 * @brief 设置线程优先级
 * @param priority 优先级值
 * @return 设置是否成功
 *
 * 设置当前线程的调度优先级。
 */
bool NEFORCE_API set_priority(int priority) noexcept;

/**
 * @brief 获取线程优先级
 * @return 当前线程的优先级值（0-100，0为最低）
 *
 * 获取当前线程的调度优先级。
 * 若获取失败，返回 0。
 */
int NEFORCE_API priority() noexcept;

/** @} */ // Thread

/** @} */ // AsyncComponents

NEFORCE_END_THIS_THREAD__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THIS_THREAD_HPP__
