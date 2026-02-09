#ifndef MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
#define MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__

/**
 * @file icollector.hpp
 * @brief MSTL集合器接口
 *
 * 此文件定义了集合器的接口，为容器类型提供统一的集合操作接口。
 */

#include "MSTL/core/interface/icommon.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct icollector
 * @brief 集合器接口模板
 * @tparam T 派生类型
 *
 * 为集合类型提供统一的接口，包括大小查询、空检查和哈希计算。
 */
template <typename T>
struct icollector : icomparable<T> {
private:
    /**
     * @brief 获取派生类常量引用
     * @return 派生类常量引用
     */
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }

public:
    /**
     * @brief 获取集合大小
     * @return 集合中元素的数量
     */
    MSTL_NODISCARD constexpr decltype(auto) size() const
    noexcept(noexcept(derived().size())) {
        return derived().size();
    }

    /**
     * @brief 检查集合是否为空
     * @return 是否为空
     */
    MSTL_NODISCARD constexpr bool empty() const
    noexcept(noexcept(derived().empty())) {
        return derived().empty();
    }
};

/** @} */ // CRTPInterfaces

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_ICOLLECTOR_HPP__
