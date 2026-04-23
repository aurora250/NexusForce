#ifndef NEFORCE_CORE_ASYNC_MEMORY_ORDER_HPP__
#define NEFORCE_CORE_ASYNC_MEMORY_ORDER_HPP__

/**
 * @file memory_order.hpp
 * @brief 原子内存序定义
 *
 * 此文件提供了内存模型中的原子内存顺序枚举定义，
 * 用于控制多线程环境下原子操作的内存可见性和指令重排行为。
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup MemoryOrder 原子内存序
 * @brief 原子内存顺序行为
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下并发编程与内存模型相关标准规范：
 *
 * **硬件架构规范（内存屏障与指令集参考）：**
 * - **Intel® 64 and IA-32 Architectures Software Developer's Manual**：x86/x64 内存排序
 *   https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
 * - **ARM Architecture Reference Manual (Armv8-A)**：ARM 弱内存序模型
 *   https://developer.arm.com/documentation/ddi0487/latest/
 *
 * **事务内存扩展规范：**
 * - **Intel® Transactional Synchronization Extensions (TSX)**：硬件锁消除（HLE）规范
 *   https://www.intel.com/content/www/us/en/docs/programmable/683364/current/transactional-synchronization-extensions.html
 *
 * @section memory_order_semantics 内存顺序语义
 * 根据 ISO/IEC 14882:2020 §31.4，六种内存顺序语义如下：
 *
 * | 内存顺序       | 枚举值     | 语义说明                                                     |
 * |----------------|------------|--------------------------------------------------------------|
 * | relaxed        | 0          | 最宽松：仅保证原子性，无同步或顺序约束                        |
 * | consume        | 1          | 数据依赖：依赖该值的操作不会被重排到前面（通常由编译器优化为 acquire） |
 * | acquire        | 2          | 获取语义：后续读写不会被重排到该操作之前                      |
 * | release        | 3          | 释放语义：之前的读写不会被重排到该操作之后                    |
 * | acq_rel        | 4          | 获取-释放组合：兼具 acquire 和 release 语义                   |
 * | seq_cst        | 5          | 顺序一致性：全局单一总顺序，最严格的同步                      |
 *
 * @section synchronization_patterns 同步模式
 * 根据 C++ 内存模型，不同内存顺序构成以下同步关系：
 *
 * | 模式              | 存储操作          | 加载操作          | 同步效果                         |
 * |-------------------|-------------------|-------------------|----------------------------------|
 * | Relaxed           | relaxed           | relaxed           | 无同步，仅保证原子性             |
 * | Release-Acquire   | release           | acquire           | 释放-获取同步，构成 happens-before |
 * | Release-Consume   | release           | consume           | 释放-消费同步（依赖排序）        |
 * | Sequential        | seq_cst           | seq_cst           | 全局顺序一致性                   |
 *
 * @section fence_equivalents 内存屏障等价
 * 各内存顺序在不同硬件架构上的内存屏障指令等价：
 *
 * | 内存顺序    | x86/x64 指令              | ARMv8 指令              | 说明                     |
 * |-------------|---------------------------|-------------------------|--------------------------|
 * | relaxed     | 无（mov）                 | 无（ldr/str）           | 无屏障                   |
 * | acquire     | 编译器屏障（mov + 约束）  | LDAR                    | 加载-获取                |
 * | release     | 编译器屏障（mov + 约束）  | STLR                    | 存储-释放                |
 * | acq_rel     | mfence（或 lock xchg）    | LDAR + STLR 组合        | 完全屏障                 |
 * | seq_cst     | mfence（或 lock xadd）    | DMB ISH                 | 顺序一致性屏障           |
 *
 * @note x86/x64 架构提供相对强的内存模型（TSO），load 自带 acquire 语义，
 *       store 自带 release 语义，因此 relaxed、acquire、release 通常仅需编译器屏障。
 *
 * @section cmpexch_failure_order 比较交换失败顺序规则
 * 根据 ISO/IEC 14882:2020 §31.8.2，compare_exchange 操作的失败内存顺序必须满足：
 * - 不能为 `release` 或 `acq_rel`（失败时不执行存储操作）
 * - 不能强于成功时的内存顺序
 *
 * 本实现提供的转换规则：
 * - `acq_rel` 失败 → `acquire`
 * - `release` 失败 → `relaxed`
 * - 其他情况保持不变
 *
 * @section hle_modifiers 硬件锁消除（HLE）修饰符
 * HLE 是 Intel TSX 扩展的一部分，允许将锁前缀的原子操作转换为事务执行：
 *
 * | 修饰符                 | 值        | 语义                                   |
 * |------------------------|-----------|----------------------------------------|
 * | memory_order_hle_acquire | 0x10000 | 事务开始（相当于 XACQUIRE 前缀）       |
 * | memory_order_hle_release | 0x20000 | 事务结束（相当于 XRELEASE 前缀）       |
 *
 * @note HLE 修饰符仅在支持 Intel TSX 的处理器上有效，在不支持的平台上自动退化为普通原子操作。
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 内存顺序基类型    | int32_t（与 C 标准兼容）                  |
 * | 修饰符基类型      | int64_t（预留扩展空间）                   |
 * | 顺序掩码          | 0x0000ffff（低 16 位）                    |
 * | 修饰符掩码        | 0xffff0000（高 16 位）                    |
 * | 修饰符位宽        | 16 位（支持最多 65536 种修饰符）          |
 *
 * @section usage_guidelines 使用指南
 * - **性能优先**：使用 `relaxed` 进行简单计数
 * - **生产者-消费者**：生产者使用 `release`，消费者使用 `acquire`
 * - **引用计数**：增加计数使用 `relaxed`，减少计数使用 `acq_rel`
 * - **互斥锁实现**：获取锁使用 `acquire`，释放锁使用 `release`
 * - **全局顺序要求**：使用 `seq_cst`（默认且最安全，但性能代价最大）
 *
 * @warning 错误使用内存顺序可能导致：
 *          - 数据竞争（未定义行为）
 *          - 死锁或活锁
 *          - 内存可见性问题（读旧值）
 *          - 平台间移植性问题（强/弱内存模型差异）
 *
 * @see https://en.cppreference.com/w/cpp/atomic/memory_order
 * @see
 * https://www.intel.com/content/www/us/en/docs/programmable/683364/current/transactional-synchronization-extensions.html
 * @see https://developer.arm.com/documentation/100941/0101/Memory-ordering
 * @{
 */

/**
 * @enum memory_order
 * @brief 内存顺序
 *
 * 定义原子操作的内存顺序语义，控制不同线程间的内存可见性顺序。
 */
enum class memory_order : int32_t {
    relaxed, ///< 最宽松的内存顺序，只保证原子性
    consume, ///< 数据依赖顺序，用于依赖读取的场景
    acquire, ///< 获取操作，确保后续读写不会被重排到前面
    release, ///< 释放操作，确保前面的读写不会被重排到后面
    acq_rel, ///< 获取-释放组合操作
    seq_cst  ///< 顺序一致性，最严格的内存顺序
};

/// @brief 宽松内存顺序常量
NEFORCE_INLINE17 constexpr auto memory_order_relaxed = memory_order::relaxed;
/// @brief 数据依赖内存顺序常量
NEFORCE_INLINE17 constexpr auto memory_order_consume = memory_order::consume;
/// @brief 获取内存顺序常量
NEFORCE_INLINE17 constexpr auto memory_order_acquire = memory_order::acquire;
/// @brief 释放内存顺序常量
NEFORCE_INLINE17 constexpr auto memory_order_release = memory_order::release;
/// @brief 获取-释放内存顺序常量
NEFORCE_INLINE17 constexpr auto memory_order_acq_rel = memory_order::acq_rel;
/// @brief 顺序一致性内存顺序常量
NEFORCE_INLINE17 constexpr auto memory_order_seq_cst = memory_order::seq_cst;


/**
 * @enum memory_order_modifier
 * @brief 内存顺序修饰符枚举
 *
 * 用于扩展memory_order的功能，支持硬件锁消除（HLE）等高级特性。
 * // TODO: deprecated memory_order_modifier
 */
enum class memory_order_modifier : int64_t {
    memory_order_none = 0,                   ///< 无修饰符
    memory_order_mask = 0x0000ffff,          ///< 内存顺序掩码
    memory_order_modifier_mask = 0xffff0000, ///< 修饰符掩码
    memory_order_hle_acquire = 0x10000,      ///< HLE获取修饰符
    memory_order_hle_release = 0x20000       ///< HLE释放修饰符
};

/**
 * @brief 内存顺序与修饰符的或操作符
 * @param mo 内存顺序
 * @param mod 内存顺序修饰符
 * @return 组合后的内存顺序
 *
 * @note 用于将内存顺序与修饰符组合使用
 */
constexpr memory_order operator|(memory_order mo, memory_order_modifier mod) noexcept {
    return static_cast<memory_order>(static_cast<int64_t>(mo) | static_cast<int64_t>(mod));
}

/**
 * @brief 内存顺序与修饰符的与操作符
 * @param mo 内存顺序
 * @param mod 内存顺序修饰符
 * @return 提取后的内存顺序
 *
 * @note 用于从组合值中提取特定的内存顺序或修饰符
 */
constexpr memory_order operator&(memory_order mo, memory_order_modifier mod) noexcept {
    return static_cast<memory_order>(static_cast<int64_t>(mo) & static_cast<int64_t>(mod));
}


/**
 * @brief 获取原子比较交换操作失败时的内存顺序
 *
 * 对于原子比较交换（compare-exchange）操作，成功和失败的内存顺序可以不同。
 * 此函数根据给定的内存顺序，返回对应的失败内存顺序。
 *
 * 转换规则：
 * - 如果基础内存顺序为 `memory_order_acq_rel`，失败顺序为 `memory_order_acquire`
 * - 如果基础内存顺序为 `memory_order_release`，失败顺序为 `memory_order_relaxed`
 * - 其他基础内存顺序保持不变
 * - 保留原始内存顺序中的所有修饰符
 */
constexpr memory_order cmpexch_failure_order(const memory_order mo) noexcept {
    constexpr auto mask = memory_order_modifier::memory_order_mask;
    const memory_order base_mo = mo & mask;
    const memory_order failure_order = base_mo == memory_order_acq_rel   ? memory_order_acquire
                                       : base_mo == memory_order_release ? memory_order_relaxed
                                                                         : base_mo;
    constexpr auto modifier_mask = memory_order_modifier::memory_order_modifier_mask;
    const auto modifiers = static_cast<memory_order_modifier>(mo & modifier_mask);
    return failure_order | modifiers;
}

/**
 * @brief 检查比较交换失败内存顺序是否有效
 * @param mo 待检查的内存顺序
 * @return 是否有效
 * @note 失败内存顺序不能是release或acq_rel
 */
constexpr bool is_valid_cmpexch_failure_order(const memory_order mo) noexcept {
    return (mo & memory_order_modifier::memory_order_mask) != memory_order_release &&
           (mo & memory_order_modifier::memory_order_mask) != memory_order_acq_rel;
}

/** @} */ // MemoryOrder

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_MEMORY_ORDER_HPP__
