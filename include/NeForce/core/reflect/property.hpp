#ifndef NEFORCE_CORE_REFLECT_PROPERTY_HPP__
#define NEFORCE_CORE_REFLECT_PROPERTY_HPP__

/**
 * @file property.hpp
 * @brief 属性反射元数据
 *
 * 此文件提供了属性反射的元数据类，用于描述和访问对象的成员变量。
 * 支持属性注解（transient、required、read_only、optional、versioned）
 * 以控制序列化行为。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/reflect/any.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @enum property_attr
 * @brief 属性注解标志
 *
 * 用于控制属性在序列化/反序列化中的行为。
 */
enum property_attr : uint8_t {
    PROP_NONE = 0,           ///< 无注解
    PROP_TRANSIENT = 1 << 0, ///< 序列化时跳过此属性
    PROP_REQUIRED = 1 << 1,  ///< 反序列化时必须存在
    PROP_READ_ONLY = 1 << 2, ///< 仅 getter，无 setter
    PROP_OPTIONAL = 1 << 3,  ///< 反序列化时可缺失
    PROP_VERSIONED = 1 << 4, ///< 启用版本追踪
};

/**
 * @brief 检查属性注解是否包含指定标志
 * @param attrs 属性注解组合
 * @param flag 要检查的标志
 * @return 包含返回 true
 */
NEFORCE_NODISCARD constexpr bool prop_has_attr(const uint8_t attrs, const property_attr flag) noexcept {
    return (attrs & static_cast<uint8_t>(flag)) != 0;
}

/**
 * @class meta_property
 * @brief 属性反射元数据类
 *
 * 描述一个成员变量的元信息，支持通过反射读写属性值。
 */
class meta_property {
public:
    using getter = function<meta_any(const void*)>;        ///< 属性读取器类型
    using setter = function<void(void*, const meta_any&)>; ///< 属性写入器类型

private:
    string_view name_;          ///< 属性名称
    reflect::type_id type_id_;  ///< 属性类型ID
    getter getter_;             ///< 读取器
    setter setter_;             ///< 写入器
    uint8_t attrs_ = PROP_NONE; ///< 属性注解
    uint32_t version_ = 0;      ///< 版本号
    string notify_signal_name_; ///< 变更通知信号名称（空表示无通知）

public:
    /**
     * @brief 构造函数
     * @param name 属性名称
     * @param type_id 属性类型ID
     * @param getter 读取器
     * @param setter 写入器
     * @param attrs 属性注解
     */
    meta_property(string_view name, reflect::type_id type_id, getter getter, setter setter,
                  const uint8_t attrs = PROP_NONE) :
    name_(name),
    type_id_(type_id),
    getter_(move(getter)),
    setter_(move(setter)),
    attrs_(attrs) {}

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
     * @brief 获取属性注解
     * @return 注解标志组合
     */
    NEFORCE_NODISCARD uint8_t attributes() const noexcept { return attrs_; }

    /**
     * @brief 设置属性注解
     * @param attrs 注解标志组合
     */
    void set_attributes(const uint8_t attrs) noexcept { attrs_ = attrs; }

    /**
     * @brief 是否为暂态属性（不参与序列化）
     */
    NEFORCE_NODISCARD bool is_transient() const noexcept { return prop_has_attr(attrs_, PROP_TRANSIENT); }

    /**
     * @brief 是否为必填属性（反序列化时必须存在）
     */
    NEFORCE_NODISCARD bool is_required() const noexcept { return prop_has_attr(attrs_, PROP_REQUIRED); }

    /**
     * @brief 是否为只读属性
     */
    NEFORCE_NODISCARD bool is_readonly() const noexcept { return prop_has_attr(attrs_, PROP_READ_ONLY); }

    /**
     * @brief 是否为可选属性
     */
    NEFORCE_NODISCARD bool is_optional() const noexcept { return prop_has_attr(attrs_, PROP_OPTIONAL); }

    /**
     * @brief 是否启用版本追踪
     */
    NEFORCE_NODISCARD bool is_versioned() const noexcept { return prop_has_attr(attrs_, PROP_VERSIONED); }

    /**
     * @brief 设置变更通知信号名称
     * @param signal_name 信号名称
     */
    void set_notify_signal(string_view signal_name) { notify_signal_name_ = signal_name; }

    /**
     * @brief 获取变更通知信号名称
     */
    NEFORCE_NODISCARD string_view notify_signal() const noexcept { return notify_signal_name_.view(); }

    /**
     * @brief 是否有变更通知信号
     */
    NEFORCE_NODISCARD bool has_notify_signal() const noexcept { return !notify_signal_name_.empty(); }

    /**
     * @brief 获取版本号
     */
    NEFORCE_NODISCARD uint32_t version() const noexcept { return version_; }

    /**
     * @brief 设置版本号
     * @param ver 版本号
     */
    void set_version(const uint32_t ver) noexcept { version_ = ver; }

    /**
     * @brief 获取属性值
     * @param obj 对象指针
     * @return 属性值（包装为any）
     */
    NEFORCE_NODISCARD meta_any get(const void* obj) const {
        if (obj == nullptr || !getter_) {
            return meta_any{};
        }
        return getter_(obj);
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
            setter_(obj, value);
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
