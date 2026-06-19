#ifndef NEFORCE_CORE_REFLECT_METATYPE_HPP__
#define NEFORCE_CORE_REFLECT_METATYPE_HPP__

/**
 * @file type.hpp
 * @brief 类型反射元数据
 *
 * 此文件提供了类型反射的核心元数据类，用于描述一个类型的完整信息，
 * 包括名称、大小、基类、属性、函数、构造函数、枚举信息和容器信息等。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/reflect/enum.hpp"
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
 * @enum container_kind
 * @brief 容器类型标识
 */
enum class container_kind : uint8_t {
    none,        ///< 非容器类型
    sequential,  ///< 顺序容器
    associative, ///< 关联容器
};

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
 * - 枚举信息
 * - 容器信息
 * - 克隆工厂
 */
class meta_type {
public:
    using constructor_func = _NEFORCE function<meta_any(const vector<meta_any>&)>; ///< 构造函数调用器类型
    using clone_func = _NEFORCE function<meta_any(const void*)>;                   ///< 克隆函数类型
    using container_size_func = _NEFORCE function<size_t(const void*)>;            ///< 容器大小查询
    using container_get_func = _NEFORCE function<meta_any(const void*, size_t)>;   ///< 按索引获取元素
    using container_insert_func = _NEFORCE function<void(void*, const meta_any&)>; ///< 插入元素
    using container_insert_kv_func = _NEFORCE function<void(void*, const meta_any&, const meta_any&)>; ///< 插入键值对

private:
    reflect::type_id type_id_;                                    ///< 类型ID
    string_view name_;                                            ///< 类型名称
    size_t size_;                                                 ///< 类型大小
    string table_name_;                                           ///< 数据库表名（空表示使用类型名称）
    constructor_func constructor_;                                ///< 构造函数调用器
    clone_func cloner_;                                           ///< 克隆函数
    vector<meta_type*> base_types_;                               ///< 直接基类列表
    vector<string> pending_base_names_;                           ///< 待解析的基类名称
    unordered_map<string, unique_ptr<meta_property>> properties_; ///< 属性映射
    unordered_map<string, unique_ptr<meta_function>> functions_;  ///< 函数映射
    vector<string> signal_names_;                                 ///< 信号名称列表
    unique_ptr<meta_enum> enum_info_;                             ///< 枚举信息
    container_kind container_kind_ = container_kind::none;        ///< 容器类型
    reflect::type_id element_type_id_ = 0;                        ///< 元素类型 ID
    reflect::type_id key_type_id_ = 0;                            ///< 键类型 ID
    reflect::type_id mapped_type_id_ = 0;                         ///< 值类型 ID
    container_size_func container_size_;                          ///< 容器大小函数
    container_get_func container_get_;                            ///< 按索引获取函数
    container_insert_func container_insert_;                      ///< 插入元素函数
    container_insert_kv_func container_insert_kv_;                ///< 插入键值对函数

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
     * @brief 获取数据库表名
     * @return 表名视图，未设置时返回空
     */
    NEFORCE_NODISCARD string_view table_name() const noexcept { return table_name_.view(); }

    /**
     * @brief 设置数据库表名
     * @param table 表名
     * @return 自身引用
     */
    meta_type& set_table_name(string_view table) {
        table_name_ = table;
        return *this;
    }

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
        for (const auto* base: base_types_) {
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
    NEFORCE_NODISCARD bool is_derived_from(string_view base_name) const {
        for (const auto* base: base_types_) {
            if (base != nullptr && (base->name() == base_name || base->is_derived_from(base_name))) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 添加属性
     * @param name 属性名称
     * @param prop_type_id 属性类型ID
     * @param getter 读取器
     * @param setter 写入器
     * @param attrs 属性注解
     * @return 自身引用
     */
    meta_type& property(string_view name, reflect::type_id prop_type_id, meta_property::getter getter,
                        meta_property::setter setter, const uint16_t attrs = PROP_NONE) {
        auto prop = make_unique<meta_property>(name, prop_type_id, move(getter), move(setter), attrs);
        properties_.emplace(name, move(prop));
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
     * @brief 设置克隆函数
     * @param cloner 克隆函数
     * @return 自身引用
     */
    meta_type& cloner(clone_func cloner) {
        cloner_ = move(cloner);
        return *this;
    }

    /**
     * @brief 克隆对象
     * @param src 源对象指针
     * @return 克隆出的对象
     */
    NEFORCE_NODISCARD meta_any clone(const void* src) const { return cloner_ ? cloner_(src) : meta_any{}; }

    /**
     * @brief 获取枚举信息
     * @return 枚举信息指针，非枚举类型返回 nullptr
     */
    NEFORCE_NODISCARD const meta_enum* enum_info() const noexcept { return enum_info_.get(); }

    /**
     * @brief 是否为枚举类型
     */
    NEFORCE_NODISCARD bool is_enum() const noexcept { return !!enum_info_; }

    /**
     * @brief 设置枚举信息
     * @param info 枚举信息
     * @return 自身引用
     */
    meta_type& enum_info(unique_ptr<meta_enum> info) {
        enum_info_ = move(info);
        return *this;
    }

    /**
     * @brief 获取容器类型
     */
    NEFORCE_NODISCARD container_kind container() const noexcept { return container_kind_; }

    /**
     * @brief 设置容器信息
     * @param kind 容器类型
     * @return 自身引用
     */
    meta_type& container(container_kind kind) {
        container_kind_ = kind;
        return *this;
    }

    /**
     * @brief 获取元素类型 ID（顺序容器）
     */
    NEFORCE_NODISCARD reflect::type_id element_type_id() const noexcept { return element_type_id_; }

    /**
     * @brief 设置元素类型 ID
     */
    meta_type& element_type_id(reflect::type_id id) {
        element_type_id_ = id;
        return *this;
    }

    /**
     * @brief 获取键类型 ID（关联容器）
     */
    NEFORCE_NODISCARD reflect::type_id key_type_id() const noexcept { return key_type_id_; }

    /**
     * @brief 设置键类型 ID
     */
    meta_type& key_type_id(reflect::type_id id) {
        key_type_id_ = id;
        return *this;
    }

    /**
     * @brief 获取值类型 ID（关联容器）
     */
    NEFORCE_NODISCARD reflect::type_id mapped_type_id() const noexcept { return mapped_type_id_; }

    /**
     * @brief 设置值类型 ID
     */
    meta_type& mapped_type_id(reflect::type_id id) {
        mapped_type_id_ = id;
        return *this;
    }

    /**
     * @brief 设置容器大小查询函数
     */
    meta_type& container_size(container_size_func func) {
        container_size_ = move(func);
        return *this;
    }

    /**
     * @brief 获取容器元素数量
     * @param container_ptr 容器对象指针
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_t container_element_count(const void* container_ptr) const {
        return container_size_ ? container_size_(container_ptr) : 0;
    }

    /**
     * @brief 设置按索引获取元素的函数
     */
    meta_type& container_get(container_get_func func) {
        container_get_ = move(func);
        return *this;
    }

    /**
     * @brief 获取容器中指定索引的元素
     * @param container_ptr 容器对象指针
     * @param index 元素索引
     * @return 包装为 meta_any 的元素
     */
    NEFORCE_NODISCARD meta_any container_element_at(const void* container_ptr, size_t index) const {
        return container_get_ ? container_get_(container_ptr, index) : meta_any{};
    }

    /**
     * @brief 设置插入元素函数（顺序容器）
     */
    meta_type& container_insert(container_insert_func func) {
        container_insert_ = move(func);
        return *this;
    }

    /**
     * @brief 向容器追加元素
     */
    void container_push_back(void* container_ptr, const meta_any& value) const {
        if (container_insert_) {
            container_insert_(container_ptr, value);
        }
    }

    /**
     * @brief 设置插入键值对函数（关联容器）
     */
    meta_type& container_insert_kv(container_insert_kv_func func) {
        container_insert_kv_ = move(func);
        return *this;
    }

    /**
     * @brief 向关联容器插入键值对
     */
    void container_insert_pair(void* container_ptr, const meta_any& key, const meta_any& value) const {
        if (container_insert_kv_) {
            container_insert_kv_(container_ptr, key, value);
        }
    }

    /**
     * @brief 是否为容器类型
     */
    NEFORCE_NODISCARD bool is_container() const noexcept { return container_kind_ != container_kind::none; }

    /**
     * @brief 获取属性
     * @param name 属性名称
     * @return 属性元数据指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const meta_property* get_property(string_view name) const {
        const auto it = properties_.find(name);
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
        const auto it = functions_.find(name);
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
     * @brief 注册信号名称
     * @param name 信号名称
     * @return 自身引用
     */
    meta_type& register_signal(string_view name) {
        signal_names_.push_back(name);
        return *this;
    }

    /**
     * @brief 获取信号名称列表
     */
    NEFORCE_NODISCARD const vector<string>& signal_names() const noexcept { return signal_names_; }

    /**
     * @brief 运行时添加属性（动态属性）
     * @param name 属性名称
     * @param prop 属性元数据
     */
    void add_property(string_view name, unique_ptr<meta_property> prop) { properties_.emplace(name, move(prop)); }

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
     * @brief 获取所有属性（包括继承的属性）
     * @return 属性列表
     */
    NEFORCE_NODISCARD vector<pair<string, const meta_property*>> all_properties() const {
        vector<pair<string, const meta_property*>> result;
        collect_properties(result);
        return result;
    }

    /**
     * @brief 获取所有函数（包括继承的函数）
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
     * @brief 解析待解析的基类名称（无需加锁版本）
     * @param registry 注册表指针
     */
    void resolve_bases_unlocked(registry* registry);

    /**
     * @brief 是否有待解析的基类
     */
    NEFORCE_NODISCARD bool has_pending_bases() const noexcept { return !pending_base_names_.empty(); }
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_METATYPE_HPP__
