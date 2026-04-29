#ifndef NEFORCE_CORE_INTERFACE_ICOMMON_HPP__
#define NEFORCE_CORE_INTERFACE_ICOMMON_HPP__

/**
 * @file icommon.hpp
 * @brief 基本接口基类
 *
 * 通过CRTP实现静态多态，派生类只需实现少数核心方法，
 * 即可自动获得补全的完整功能。
 */

#include "NeForce/core/functional/hash.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct ihashable
 * @brief 可哈希对象接口模板
 * @tparam T 派生类类型
 *
 * 派生类需要实现to_hash()方法，即可通过此接口自动获得哈希支持。
 * 该接口会自动特化hash模板，使对象可用于哈希相关操作如哈希容器。
 */
template <typename T>
struct ihashable {
private:
    constexpr const T& derived() const noexcept { return static_cast<const T&>(*this); }

public:
    /**
     * @brief 获取对象的哈希值
     * @return 哈希值
     *
     * 实际调用派生类的to_hash()方法
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept(noexcept(derived().to_hash())) {
        return derived().to_hash();
    }
};

/** @} */ // CRTPInterfaces

/**
 * @addtogroup HashPrimary 哈希模板
 * @{
 */

/**
 * @brief ihashable的哈希特化
 * @tparam T 子类类型
 */
template <typename T>
struct hash<T, enable_if_t<is_base_of<ihashable<T>, T>::value>> {
    NEFORCE_NODISCARD constexpr size_t operator()(const T& obj) const noexcept(noexcept(obj.to_hash())) {
        return obj.to_hash();
    }
};

/** @} */ // HashPrimary

/**
 * @addtogroup CRTPInterfaces CRTP接口
 * @{
 */

/**
 * @struct icomparable
 * @brief 可比较对象接口模板
 * @tparam T 派生类类型
 *
 * 通过CRTP模式实现，派生类只需实现operator ==和operator <两个基本比较操作，
 * 即可自动获得所有比较运算符的完整实现。
 */
template <typename T>
struct icomparable {
private:
    constexpr const T& derived() const noexcept { return static_cast<const T&>(*this); }

public:
    /**
     * @brief 相等比较运算符
     * @param rhs 右操作数
     * @return 是否相等
     *
     * 调用派生类实现的operator ==进行相等性判断。
     */
    NEFORCE_NODISCARD constexpr bool operator==(const T& rhs) const noexcept(noexcept(derived() == rhs)) {
        return derived() == rhs;
    }

    /**
     * @brief 不等比较运算符
     * @param rhs 右操作数
     * @return 是否不相等
     *
     * 基于operator ==的结果取反得到不等比较结果。
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const T& rhs) const noexcept(noexcept(derived() == rhs)) {
        return !(*this == rhs);
    }

    /**
     * @brief 小于比较运算符
     * @param rhs 右操作数
     * @return 是否小于右操作数
     *
     * 调用派生类实现的operator <进行小于比较。
     */
    NEFORCE_NODISCARD constexpr bool operator<(const T& rhs) const noexcept(noexcept(derived() < rhs)) {
        return derived() < rhs;
    }

    /**
     * @brief 大于比较运算符
     * @param rhs 右操作数
     * @return 是否大于右操作数
     *
     * 通过交换操作数并调用operator <来实现大于比较。
     */
    NEFORCE_NODISCARD constexpr bool operator>(const T& rhs) const noexcept(noexcept(rhs < derived())) {
        return rhs < derived();
    }

    /**
     * @brief 小于等于比较运算符
     * @param rhs 右操作数
     * @return 是否小于等于右操作数
     *
     * 通过取反operator >的结果实现小于等于比较。
     */
    NEFORCE_NODISCARD constexpr bool operator<=(const T& rhs) const noexcept(noexcept(!(derived() > rhs))) {
        return !(derived() > rhs);
    }

    /**
     * @brief 大于等于比较运算符
     * @param rhs 右操作数
     * @return 是否大于等于右操作数
     *
     * 通过取反operator <的结果实现大于等于比较。
     */
    NEFORCE_NODISCARD constexpr bool operator>=(const T& rhs) const noexcept(noexcept(!(derived() < rhs))) {
        return !(derived() < rhs);
    }
};


/**
 * @struct icommon
 * @brief 通用接口，同时具备可比较和可哈希功能
 * @tparam T 派生类类型
 *
 * 这是一个便利类，同时继承自icomparable和ihashable，
 * 为派生类提供完整的比较和哈希支持。派生类需实现三个核心方法：
 * operator ==, operator < 和 to_hash()。
 */
template <typename T>
struct icommon : icomparable<T>, ihashable<T> {};

/** @} */ // CRTPInterfaces

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_INTERFACE_ICOMMON_HPP__
