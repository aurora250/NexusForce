#ifndef NEFORCE_CORE_INTERFACE_ISTRINGIFY_HPP__
#define NEFORCE_CORE_INTERFACE_ISTRINGIFY_HPP__

/**
 * @file istringify.hpp
 * @brief 可字符串化接口
 *
 * 此文件提供了可字符串化接口的定义。
 * 继承此接口的类需要实现to_string()方法，从而获得统一的字符串表示能力。
 * 同时提供了全局to_string函数，用于将实现了该接口的对象转换为字符串。
 */

#include "NeForce/core/string/string.hpp"
#include "NeForce/core/string/lexical_cast.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @class istringify
 * @brief 可字符串化接口
 * @tparam T 派生类类型
 *
 * 采用奇异递归模板模式（CRTP）。
 * 继承该接口的派生类需要实现 to_string() 成员函数，
 * 该函数返回对象的字符串表示。通过此接口可以获得统一的字符串转换能力。
 */
template <typename T>
struct istringify {
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
     * @brief 转换为字符串
     * @return 对象的字符串表示
     *
     * 此函数调用派生类的to_string()实现。
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const {
        return derived().to_string();
    }
};

/** @} */ // CRTPInterfaces

/**
 * @defgroup ToString 转换字符串
 * @brief 各类型到字符串的转换函数
 * @{
 */

/**
 * @brief 将实现了istringify接口的对象转换为字符串
 * @tparam T 对象类型
 * @param obj 要转换的对象
 * @return 对象的字符串表示
 *
 * 全局辅助函数，用于方便地调用实现了istringify接口的对象的to_string方法。
 * 仅当T是istringify<T>的派生类时可用。
 */
template <typename T, enable_if_t<is_base_of_v<istringify<T>, T>, int> = 0>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const T& obj) {
    return obj.to_string();
}


template <typename From>
struct lexical_caster<string, From> {
    static enable_if_t<is_base_of_v<istringify<From>, From>, string>
    cast(const From& value) {
        return value.to_string();
    }
};

/** @} */ // ToString

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_INTERFACE_ISTRINGIFY_HPP__
