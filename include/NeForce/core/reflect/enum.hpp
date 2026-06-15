#ifndef NEFORCE_CORE_REFLECT_ENUM_HPP__
#define NEFORCE_CORE_REFLECT_ENUM_HPP__

/**
 * @file enum.hpp
 * @brief 枚举反射元数据
 *
 * 此文件提供了枚举类型的反射支持，用于在运行时查询枚举值的名称、
 * 以及通过名称查找对应的枚举值。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/reflect/any.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @struct enum_entry
 * @brief 枚举条目
 */
struct enum_entry {
    string_view name; ///< 枚举值名称
    int64_t value;    ///< 枚举数值
};

/**
 * @class meta_enum
 * @brief 枚举反射元数据类
 *
 * 描述一个枚举类型的元信息，支持名称与值的双向查找。
 */
class meta_enum {
private:
    string_view name_;           ///< 枚举类型名称
    type_id underlying_type_id_; ///< 底层整数类型 ID
    vector<enum_entry> entries_; ///< 枚举条目列表

public:
    /**
     * @brief 构造函数
     * @param name 枚举类型名称
     * @param underlying_id 底层类型 ID
     */
    meta_enum(string_view name, type_id underlying_id) :
    name_(name),
    underlying_type_id_(underlying_id) {}

    /**
     * @brief 添加枚举条目
     * @param entry_name 条目名称
     * @param entry_value 条目数值
     */
    void add_entry(string_view entry_name, const int64_t entry_value) { entries_.push_back({entry_name, entry_value}); }

    /**
     * @brief 获取枚举类型名称
     */
    NEFORCE_NODISCARD string_view name() const noexcept { return name_; }

    /**
     * @brief 获取底层类型 ID
     */
    NEFORCE_NODISCARD type_id underlying_type_id() const noexcept { return underlying_type_id_; }

    /**
     * @brief 获取所有枚举条目
     */
    NEFORCE_NODISCARD const vector<enum_entry>& entries() const noexcept { return entries_; }

    /**
     * @brief 通过名称查找枚举值
     * @param entry_name 条目名称
     * @param out_value 输出参数，接收查找到的数值
     * @return 找到返回 true
     */
    NEFORCE_NODISCARD bool value_of(string_view entry_name, int64_t& out_value) const noexcept {
        for (const auto& entry: entries_) {
            if (entry.name == entry_name) {
                out_value = entry.value;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 通过数值查找枚举名称
     * @param entry_value 条目数值
     * @return 找到返回名称视图，否则返回空视图
     */
    NEFORCE_NODISCARD string_view name_of(const int64_t entry_value) const noexcept {
        for (const auto& entry: entries_) {
            if (entry.value == entry_value) {
                return entry.name;
            }
        }
        return {};
    }
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_ENUM_HPP__
