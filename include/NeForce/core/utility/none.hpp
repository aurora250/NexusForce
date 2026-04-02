#ifndef NEFORCE_CORE_UTILITY_NON_HPP__
#define NEFORCE_CORE_UTILITY_NON_HPP__

/**
 * @file none.hpp
 * @brief 空状态类型
 *
 * 此文件提供了空状态类型的实现，用于表示不包含任何值的特殊状态，
 * 用于作为可空类型的空值表示或作为占位符类型。
 */

#include "NeForce/core/interface/icommon.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup None 空状态
 * @brief 空状态的实现
 * @{
 */

/**
 * @struct none_t
 * @brief 空状态类型
 *
 * 表示一个不包含任何值的状态。
 *
 * 主要用于以下场景：
 * 1. 作为可空类型的空值表示
 * 2. 作为占位符类型，用于类型推导和模板元编程
 * 3. 作为标记类型
 */
struct none_t : icommon<none_t> {
    /**
     * @brief 默认构造函数
     *
     * 构造一个空状态对象，不执行任何操作。
     */
    constexpr none_t() noexcept = default;

    /**
     * @brief 相等比较运算符
     * @param other 另一个空状态对象
     * @return 总是返回true，因为所有空状态都是相等的
     */
    constexpr bool operator==(const none_t& other) const noexcept { return true; }

    /**
     * @brief 小于比较运算符
     * @param other 另一个空状态对象
     * @return 总是返回false，因为空状态不小于任何其他空状态
     */
    constexpr bool operator<(const none_t& other) const noexcept { return false; }

    /**
     * @brief 计算哈希值
     * @return FNV偏移基准值
     */
    constexpr size_t to_hash() const noexcept { return constants::FNV_OFFSET_BASIS; }

    /**
     * @brief 交换操作
     * @param other 另一个空状态对象
     *
     * 交换两个空状态对象，由于空状态不包含任何数据，此操作不执行任何操作。
     */
    constexpr void swap(none_t& other) noexcept {}
};

/**
 * @var none
 * @brief 默认空表示
 */
NEFORCE_INLINE17 constexpr none_t none{};

/** @} */ // Non

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_NON_HPP__
