#ifndef NEFORCE_CORE_REFLECT_METATYPE_HPP__
#define NEFORCE_CORE_REFLECT_METATYPE_HPP__

/**
 * @file type.hpp
 * @brief 类型反射元数据
 *
 * 此文件提供了类型反射的核心元数据类，用于描述一个类型的完整信息，
 * 包括名称、大小、基类、属性、函数和构造函数等。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/reflect/function.hpp"
#include "NeForce/core/reflect/property.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

class registry;

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @class meta_type
 * @brief 类型反射元数据类
 *
 * 描述一个类型的完整反射信息，包括：
 * - 类型基本信息（名称、大小、ID）
 * - 继承关系（基类列表）
 * - 属性列表（成员变量）
 * - 函数列表（成员函数和静态函数）
 * - 构造函数
 */
class meta_type {
public:
    using constructor_func = _NEFORCE function<meta_any(const vector<meta_any>&)>; ///< 构造函数调用器类型

private:
    reflect::type_id type_id_;                                    ///< 类型ID
    string_view name_;                                            ///< 类型名称
    size_t size_;                                                 ///< 类型大小
    constructor_func constructor_;                                ///< 构造函数调用器
    vector<meta_type*> base_types_;                               ///< 直接基类列表
    vector<string> pending_base_names_;                           ///< 待解析的基类名称
    unordered_map<string, unique_ptr<meta_property>> properties_; ///< 属性映射
    unordered_map<string, unique_ptr<meta_function>> functions_;  ///< 函数映射

    void collect_properties(vector<pair<string, const meta_property*>>& result,
                            vector<reflect::type_id>* visited = nullptr) const {
        vector<reflect::type_id> local_visited;
        if (visited == nullptr) {
            visited = &local_visited;
        }

        if (find(visited->begin(), visited->end(), type_id_) != visited->end()) {
            return;
        }
        visited->push_back(type_id_);

        for (const auto* base: base_types_) {
            if (base != nullptr) {
                base->collect_properties(result, visited);
            }
        }

        for (const auto& property: properties_) {
            const auto& name = property.first;
            const auto& prop = property.second;
            result.emplace_back(name, prop.get());
        }
    }

    void collect_functions(vector<pair<string, const meta_function*>>& result,
                           vector<reflect::type_id>* visited = nullptr) const {
        vector<reflect::type_id> local_visited;
        if (visited == nullptr) {
            visited = &local_visited;
        }

        if (find(visited->begin(), visited->end(), type_id_) != visited->end()) {
            return;
        }
        visited->push_back(type_id_);

        for (const auto* base: base_types_) {
            if (base != nullptr) {
                base->collect_functions(result, visited);
            }
        }

        for (const auto& f: functions_) {
            const auto& name = f.first;
            const auto& func = f.second;
            result.emplace_back(name, func.get());
        }
    }

public:
    /**
     * @brief 构造函数
     * @param name 类型名称
     * @param id 类型ID
     * @param size 类型大小
     */
    meta_type(string_view name, reflect::type_id id, size_t size) :
    type_id_(id),
    name_(name),
    size_(size) {}

    /**
     * @brief 获取类型ID
     */
    NEFORCE_NODISCARD reflect::type_id type_id() const noexcept { return type_id_; }

    /**
     * @brief 获取类型名称
     */
    NEFORCE_NODISCARD string_view name() const noexcept { return name_; }

    /**
     * @brief 获取类型大小
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 获取基类列表
     */
    NEFORCE_NODISCARD const vector<meta_type*>& base_types() const { return base_types_; }

    /**
     * @brief 添加基类（已解析）
     * @param base 基类元数据
     * @return 自身引用
     */
    meta_type& base_type(meta_type* base) {
        if (base != nullptr) {
            base_types_.push_back(base);
        }
        return *this;
    }

    /**
     * @brief 添加基类（延迟解析）
     * @param base_name 基类名称
     * @return 自身引用
     */
    meta_type& base_type(string_view base_name) {
        pending_base_names_.push_back(base_name);
        return *this;
    }

    /**
     * @brief 检查是否派生自指定类型
     * @param base_id 基类类型ID
     * @return 是派生类返回true
     */
    NEFORCE_NODISCARD bool is_derived_from(reflect::type_id base_id) const {
        if (type_id_ == base_id) {
            return true;
        }
        for (auto* base: base_types_) {
            if (base != nullptr && base->is_derived_from(base_id)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 检查是否派生自指定类型
     * @param base_name 基类名称
     * @return 是派生类返回true
     */
    NEFORCE_NODISCARD bool is_derived_from(string_view base_name) const { return is_derived_from(base_name.to_hash()); }

    /**
     * @brief 添加属性
     * @param name 属性名称
     * @param prop_type_id 属性类型ID
     * @param getter 读取器
     * @param setter 写入器
     * @return 自身引用
     */
    meta_type& property(string_view name, reflect::type_id prop_type_id, meta_property::getter getter,
                        meta_property::setter setter) {
        properties_.emplace(name, make_unique<meta_property>(name, prop_type_id, move(getter), move(setter)));
        return *this;
    }

    /**
     * @brief 添加函数
     * @param name 函数名称
     * @param invoker 调用器
     * @return 函数元数据指针
     */
    meta_function* function(string_view name, meta_function::invoker invoker) {
        const auto it = functions_.emplace(name, make_unique<meta_function>(name, move(invoker))).first;
        return it->second.get();
    }

    /**
     * @brief 设置构造函数
     * @param ctor 构造函数调用器
     * @return 自身引用
     */
    meta_type& constructor(constructor_func ctor) {
        constructor_ = move(ctor);
        return *this;
    }

    /**
     * @brief 获取属性
     * @param name 属性名称
     * @return 属性元数据指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const meta_property* get_property(string_view name) const {
        const auto it = properties_.find(string(name));
        if (it != properties_.end()) {
            return it->second.get();
        }

        for (const auto* base: base_types_) {
            if (base != nullptr) {
                if (const auto* prop = base->get_property(name)) {
                    return prop;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief 获取函数
     * @param name 函数名称
     * @return 函数元数据指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const meta_function* get_function(string_view name) const {
        const auto it = functions_.find(string(name));
        if (it != functions_.end()) {
            return it->second.get();
        }

        for (const auto* base: base_types_) {
            if (base != nullptr) {
                if (const auto* func = base->get_function(name)) {
                    return func;
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief 创建对象（无参构造）
     * @return 创建的对象
     */
    NEFORCE_NODISCARD meta_any create() const { return constructor_ ? constructor_({}) : meta_any{}; }

    /**
     * @brief 创建对象（带参数）
     * @param args 构造参数
     * @return 创建的对象
     */
    NEFORCE_NODISCARD meta_any create(const vector<meta_any>& args) const {
        return constructor_ ? constructor_(args) : meta_any{};
    }

    /**
     * @brief 获取属性映射
     * @return 属性映射常量引用
     */
    NEFORCE_NODISCARD const auto& properties() const { return properties_; }

    /**
     * @brief 获取函数映射
     * @return 函数映射常量引用
     */
    NEFORCE_NODISCARD const auto& functions() const { return functions_; }

    /**
     * @brief 获取所有属性
     * @return 属性列表
     */
    NEFORCE_NODISCARD vector<pair<string, const meta_property*>> all_properties() const {
        vector<pair<string, const meta_property*>> result;
        collect_properties(result);
        return result;
    }

    /**
     * @brief 获取所有函数
     * @return 函数列表
     */
    NEFORCE_NODISCARD vector<pair<string, const meta_function*>> all_functions() const {
        vector<pair<string, const meta_function*>> result;
        collect_functions(result);
        return result;
    }

    /**
     * @brief 解析待解析的基类名称
     * @param registry 注册表指针
     */
    void resolve_bases(registry* registry);

    /**
     * @brief 解析待解析的基类名称
     * @param registry 注册表指针
     */
    void resolve_bases_unlocked(registry* registry);
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_METATYPE_HPP__
