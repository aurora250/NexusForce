#ifndef NEFORCE_CORE_REFLECT_PROPERTY_HPP__
#define NEFORCE_CORE_REFLECT_PROPERTY_HPP__

/**
 * @file property.hpp
 * @brief 属性反射元数据
 *
 * 此文件提供了属性反射的元数据类，用于描述和访问对象的成员变量。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/reflect/any.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @class meta_property
 * @brief 属性反射元数据类
 *
 * 描述一个成员变量的元信息，支持通过反射读写属性值。
 */
class meta_property {
public:
    using getter = function<meta_any(void*)>;              ///< 属性读取器类型
    using setter = function<void(void*, const meta_any&)>; ///< 属性写入器类型

private:
    string_view name_;         ///< 属性名称
    reflect::type_id type_id_; ///< 属性类型ID
    getter getter_;            ///< 读取器
    setter setter_;            ///< 写入器

public:
    /**
     * @brief 构造函数
     * @param name 属性名称
     * @param type_id 属性类型ID
     * @param getter 读取器
     * @param setter 写入器
     */
    meta_property(string_view name, reflect::type_id type_id, getter getter, setter setter) :
    name_(name),
    type_id_(type_id),
    getter_(move(getter)),
    setter_(move(setter)) {}

    /**
     * @brief 获取属性名称
     * @return 名称视图
     */
    NEFORCE_NODISCARD string_view name() const noexcept { return name_; }

    /**
     * @brief 获取属性类型ID
     * @return 类型ID
     */
    NEFORCE_NODISCARD reflect::type_id type_id() const noexcept { return type_id_; }

    /**
     * @brief 获取属性值
     * @param obj 对象指针
     * @return 属性值（包装为any）
     */
    NEFORCE_NODISCARD meta_any get(void* obj) const {
        if (obj == nullptr || !getter_) {
            return meta_any{};
        }
        return getter_(move(obj));
    }

    /**
     * @brief 设置属性值
     * @param obj 对象指针
     * @param value 要设置的值
     * @return 设置成功返回true
     */
    bool set(void* obj, const meta_any& value) const {
        if (obj == nullptr || !setter_) {
            return false;
        }
        try {
            setter_(move(obj), value);
            return true;
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief 设置属性值（直接值版本）
     * @tparam T 值类型
     * @param obj 对象指针
     * @param value 要设置的值
     * @return 设置成功返回true
     */
    template <typename T, enable_if_t<!is_same_v<meta_any, decay_t<T>>, int> = 0>
    bool set(void* obj, T&& value) const {
        return this->set(obj, meta_any{_NEFORCE forward<T>(value)});
    }
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_PROPERTY_HPP__
