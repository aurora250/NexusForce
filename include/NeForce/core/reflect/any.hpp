#ifndef NEFORCE_CORE_REFLECT_ANY_HPP__
#define NEFORCE_CORE_REFLECT_ANY_HPP__

/**
 * @file any.hpp
 * @brief 类型擦除容器
 *
 * 此文件提供了any类型的实现，用于存储任意类型的值，
 * 支持类型安全的存取操作，是反射系统的核心基础。
 */

#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/string/string_view.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

using type_id = size_t; ///< 类型标识符


/**
 * @struct type_name
 * @brief 类型名称获取器
 * @tparam T 目标类型
 *
 * 提供编译期类型名称字符串。可对自定义类型进行特化。
 */
template <typename T> struct type_name {
    static constexpr string_view value = "unknown";
};

/**
 * @brief type_name的便捷访问变量模板
 */
template <typename T> NEFORCE_INLINE17 constexpr string_view type_name_v = type_name<T>::value;

/// @cond
#define __NEFORCE_SPECIALIZE_TYPE_NAME(T)        \
    template <> struct type_name<T> {            \
        static constexpr string_view value = #T; \
    };

NEFORCE_MACRO_RANGE_ARITHMETIC(__NEFORCE_SPECIALIZE_TYPE_NAME)
#undef __NEFORCE_SPECIALIZE_TYPE_NAME
/// @endcond

/**
 * @class meta_any
 * @brief 类型擦除容器
 *
 * 可以存储任意类型的值，并提供类型安全的存取接口。
 */
class meta_any {
private:
    struct concepts {
        virtual ~concepts() = default;
        virtual unique_ptr<concepts> clone() const = 0;
        virtual reflect::type_id type_id() const noexcept = 0;
    };

    template <typename T> struct model final : concepts {
        T value_;

        explicit model(T value) :
        value_(_NEFORCE move(value)) {}

        unique_ptr<concepts> clone() const override { return _NEFORCE make_unique<model<T>>(value_); }

        reflect::type_id type_id() const noexcept override { return type_name_v<T>.to_hash(); }
    };

    unique_ptr<concepts> storage_{nullptr}; ///< 存储容器

public:
    /**
     * @brief 默认构造函数
     */
    meta_any() noexcept = default;

    /**
     * @brief 从任意值构造
     * @tparam T 值类型
     * @param value 要存储的值
     */
    template <typename T, typename = enable_if_t<!is_same_v<decay_t<T>, meta_any>>>
    explicit meta_any(T&& value) :
    storage_(_NEFORCE make_unique<model<decay_t<T>>>(_NEFORCE forward<T>(value))) {}

    meta_any(meta_any&&) noexcept = default;
    meta_any& operator=(meta_any&&) noexcept = default;

    /**
     * @brief 拷贝构造函数
     * @param other 源对象
     */
    meta_any(const meta_any& other) {
        if (other.storage_) {
            storage_ = other.storage_->clone();
        }
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    meta_any& operator=(const meta_any& other) {
        if (this != &other) {
            if (other.storage_) {
                storage_ = other.storage_->clone();
            } else {
                storage_.reset();
            }
        }
        return *this;
    }

    /**
     * @brief 获取存储值的类型ID
     * @return 类型ID，空对象返回0
     */
    NEFORCE_NODISCARD reflect::type_id type_id() const noexcept { return storage_ ? storage_->type_id() : 0; }

    /**
     * @brief 检查是否包含值
     * @return 包含值返回true
     */
    NEFORCE_NODISCARD bool has_value() const noexcept { return !!storage_; }

    /**
     * @brief 布尔转换运算符
     * @return 包含值返回true
     */
    explicit operator bool() const noexcept { return has_value(); }

    /**
     * @brief 尝试转换为指定类型的指针
     * @tparam T 目标类型
     * @return 类型匹配返回指针，否则返回nullptr
     */
    template <typename T> NEFORCE_NODISCARD T* cast() noexcept {
        if (!storage_) {
            return nullptr;
        }
        if (storage_->type_id() != type_name_v<T>.to_hash()) {
            return nullptr;
        }
        auto* md = dynamic_cast<model<T>*>(storage_.get());
        return md ? &md->value_ : nullptr;
    }

    /**
     * @brief 尝试转换为指定类型的常量指针
     * @tparam T 目标类型
     * @return 类型匹配返回指针，否则返回nullptr
     */
    template <typename T> NEFORCE_NODISCARD const T* cast() const noexcept {
        if (!storage_) {
            return nullptr;
        }
        if (storage_->type_id() != type_name_v<T>.to_hash()) {
            return nullptr;
        }
        auto* md = dynamic_cast<model<T>*>(storage_.get());
        return md ? &md->value_ : nullptr;
    }

    /**
     * @brief 获取存储值的引用
     * @tparam T 目标类型
     * @return 存储值的引用
     * @throws typecast_exception 类型不匹配时抛出
     */
    template <typename T> NEFORCE_NODISCARD T& get() {
        if (auto* ptr = cast<T>()) {
            return *ptr;
        }
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    /**
     * @brief 获取存储值的常量引用
     * @tparam T 目标类型
     * @return 存储值的常量引用
     * @throws typecast_exception 类型不匹配时抛出
     */
    template <typename T> NEFORCE_NODISCARD const T& get() const {
        if (auto* ptr = cast<T>()) {
            return *ptr;
        }
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }

    /**
     * @brief 检查是否可以转换为指定类型
     * @tparam T 目标类型
     * @return 可以转换返回true
     */
    template <typename T> NEFORCE_NODISCARD bool can_cast() const noexcept { return cast<T>() != nullptr; }

    /**
     * @brief 转换为指定类型的值
     * @tparam T 目标类型
     * @return 转换后的值
     * @throws typecast_exception 类型不匹配时抛出
     *
     * 返回存储值的副本，而非引用。
     */
    template <typename T> NEFORCE_NODISCARD T convert() const {
        if (auto* ptr = cast<T>()) {
            return *ptr;
        }
        NEFORCE_THROW_EXCEPTION(typecast_exception("Not a valid type"));
        unreachable();
    }
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_ANY_HPP__
