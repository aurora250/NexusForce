#ifndef NEFORCE_CORE_ASYNC_FUTEX_HPP__
#define NEFORCE_CORE_ASYNC_FUTEX_HPP__

/**
 * @file futex.hpp
 * @brief 快速用户空间互斥锁
 *
 * 此文件提供了FUTEX（Fast Userspace muTEX）的跨平台封装，用于实现高效的同步原语。
 * FUTEX是Linux独有的高性能同步机制，这里提供了跨平台的统一接口。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Futex FUTEX
 * @brief FUTEX同步原语的跨平台封装
 * @{
 */

/**
 * @brief 平台等待类型别名
 *
 * 用于FUTEX操作的等待类型
 */
using platform_wait_t =
#ifdef NEFORCE_PLATFORM_WINDOWS
        long;
#else
        int;
#endif


/**
 * @enum futex_wait_flags
 * @brief FUTEX操作标志枚举
 *
 * 定义FUTEX系统调用的操作标志。这些标志对应于Linux内核的FUTEX操作。
 */
enum class futex_wait_flags : platform_wait_t {
    private_flag = 0,                                 ///< 私有标志位
    wait = 0,                                         ///< 等待操作
    wake = 1,                                         ///< 唤醒操作
    requeue = 3,                                      ///< 将等待队列转移到另一个FUTEX
    cmp_requeue = 4,                                  ///< 带比较的转移
    wake_op = 5,                                      ///< 唤醒并原子修改另一个FUTEX
    lock_pi = 6,                                      ///< 优先级继承锁（加锁）
    unlock_pi = 7,                                    ///< 优先级继承锁（解锁）
    trylock_pi = 8,                                   ///< 尝试加锁（非阻塞）
    wait_bitset = 9,                                  ///< 位集等待操作
    wake_bitset = 10,                                 ///< 位集唤醒操作
    wait_requeue_pi = 11,                             ///< 等待并转移到PI FUTEX
    cmp_requeue_pi = 12,                              ///< 带比较的等待并转移
    wait_private = wait | private_flag,               ///< 私有等待操作
    wake_private = wake | private_flag,               ///< 私有唤醒操作
    wait_bitset_private = wait_bitset | private_flag, ///< 私有位集等待操作
    wake_bitset_private = wake_bitset | private_flag, ///< 私有位集唤醒操作
    bitset_match_any = -1                             ///< 匹配任何等待集合的位掩码
};


/**
 * @brief 无限期等待FUTEX
 * @param addr FUTEX变量的地址
 * @param value 期望的值，只有当地址处的值等于此值时才等待
 *
 * 阻塞当前线程，直到addr处的值不等于value或收到通知。
 * 这是最基本的FUTEX等待操作，没有超时限制。
 *
 * @warning 调用者必须确保 addr 在等待期间保持有效且可访问
 */
void NEFORCE_API futex_wait(void* addr, platform_wait_t value) noexcept;

/**
 * @brief 等待FUTEX直到指定时间点或条件满足
 *
 * @param addr FUTEX地址
 * @param value 期望的值
 * @param has_timeout 是否启用超时。如果为false，则忽略时间参数
 * @param sec 超时时间的秒数部分
 * @param ns 超时时间的纳秒数部分
 * @param is_monotonic 是否使用单调时钟
 * @return bool 等待结果：
 *              - true: 等待成功结束（被唤醒或值已改变）
 *              - false: 等待超时（仅当has_timeout为true时可能）
 *
 * 阻塞当前线程，直到以下条件之一满足：
 * 1. addr 处的值不等于期望的 value
 * 2. 达到指定的超时时间
 * 3. 被其他线程唤醒
 *
 * @note 函数可能因系统信号等原因返回，即使条件未满足，产生虚假唤醒。调用者需要循环检查条件
 * @note 实际超时精度受系统调度器和时钟精度限制
 * @warning 调用者必须确保 addr 在等待期间保持有效且可访问
 */
bool NEFORCE_API futex_wait_until(void* addr, platform_wait_t value, bool has_timeout, int64_t sec, int64_t ns,
                                  bool is_monotonic = false);

/**
 * @brief 通知等待的线程
 * @param addr FUTEX变量的地址
 * @param all 是否通知所有等待线程
 *
 * 唤醒正在等待addr处值变化的线程。
 * 如果all为true，唤醒所有等待线程；否则只唤醒一个等待线程。
 *
 * @warning 调用者必须确保 addr 在等待期间保持有效且可访问
 */
void NEFORCE_API futex_notify(void* addr, bool all) noexcept;

#ifdef NEFORCE_PLATFORM_LINUX
#    define NEFORCE_FUTEX_HAS_ADVANCED 1

int futex(void* wait_addr, futex_wait_flags flags, int wake_count, void* requeue_addr, int requeue_count,
          int cmp_value) noexcept;

/**
 * @brief 将部分等待线程转移到另一个 futex
 * @param wait_addr     源 futex 地址
 * @param wake_count    唤醒的线程数（从队列头部）
 * @param requeue_addr  目标 futex 地址
 * @param requeue_count 转移的线程数（唤醒后剩余队列中转移）
 * @return 0 成功；负值错误码（如 -EINVAL）
 */
int NEFORCE_API futex_requeue(void* wait_addr, int wake_count, void* requeue_addr, int requeue_count) noexcept;

/**
 * @brief 唤醒线程的同时原子修改另一个 futex 的值
 * @param addr      要唤醒的 futex 地址
 * @param wake_count 唤醒线程数
 * @param op_addr   要操作的 futex 地址
 * @param op_arg    操作参数
 * @param op        操作码（FUTEX_OP_* 常量）
 * @param cmp       比较操作（FUTEX_OP_CMP_* 常量）
 * @param cmp_arg   比较参数
 * @return 0 成功；负值错误码
 */
int NEFORCE_API futex_wake_op(void* addr, int wake_count, void* op_addr, int op_arg, int op, int cmp,
                              int cmp_arg) noexcept;

int NEFORCE_API futex_lock_pi(void* addr) noexcept;
int NEFORCE_API futex_trylock_pi(void* addr) noexcept;
int NEFORCE_API futex_unlock_pi(void* addr) noexcept;

/**
 * @brief 等待并转移到 PI futex（用于条件变量与 PI 锁配合）
 * @param wait_addr     等待的 futex 地址
 * @param value         期望值
 * @param requeue_addr  目标 PI futex 地址
 * @return 0 成功；负值错误码
 */
int NEFORCE_API futex_wait_requeue_pi(void* wait_addr, int value, void* requeue_addr) noexcept;

#endif // NEFORCE_PLATFORM_LINUX

/** @} */ // Futex

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_FUTEX_HPP__
