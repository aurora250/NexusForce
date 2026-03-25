#ifndef NEFORCE_CORE_FILE_INI_INI_VALUE_HPP__
#define NEFORCE_CORE_FILE_INI_INI_VALUE_HPP__

/**
 * @file ini_value.hpp
 * @brief ini配置变量
 *
 * 此文件提供了ini配置格式的抽象基类和具体实现类。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct ini_exception
 * @extends value_exception
 * @brief ini格式操作失败
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(ini_exception, value_exception, "INI Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup IniConfig ini配置
 * @brief ini配置格式管理
 * @{
 */

class ini_value;
class ini_section;
class ini_property;


/**
 * @class ini_value
 * @brief ini值抽象基类
 *
 * 提供ini配置元素的统一接口，支持类型识别和字符串转换。
 */
class NEFORCE_API ini_value : public istringify<ini_value> {
public:
    /**
     * @enum types
     * @brief ini值类型枚举
     */
    enum types {
        Section,  ///< 节类型
        Property  ///< 属性类型
    };

    /**
     * @brief 虚析构函数
     */
    virtual ~ini_value() = default;

    /**
     * @brief 获取ini值类型
     * @return 类型枚举值
     */
    NEFORCE_NODISCARD virtual types type() const noexcept = 0;

    /**
     * @brief 转换为节指针
     * @return 如果是节类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const ini_section* as_section() const noexcept { return nullptr; }

    /**
     * @brief 转换为属性指针
     * @return 如果是属性类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const ini_property* as_property() const noexcept { return nullptr; }

    /**
     * @brief 判断是否为节类型
     * @return 如果是节类型返回true
     */
    NEFORCE_NODISCARD bool is_section() const noexcept { return type() == Section; }

    /**
     * @brief 判断是否为属性类型
     * @return 如果是属性类型返回true
     */
    NEFORCE_NODISCARD bool is_property() const noexcept { return type() == Property; }

    /**
     * @brief 转换为字符串
     * @return ini值的字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档字符串
     * @return ini值的文档格式字符串
     */
    NEFORCE_NODISCARD string to_document() const;
};


/**
 * @class ini_property
 * @brief ini属性类
 *
 * 表示ini配置文件中的一个键值对属性。
 * 提供类型转换方法和值访问接口。
 */
class NEFORCE_API ini_property final : public ini_value {
private:
    string value_;  ///< 属性值

public:
    /**
     * @brief 构造函数
     * @param value 属性值
     */
    explicit ini_property(string value) noexcept
    : value_(_NEFORCE move(value)) {}

    /**
     * @brief 获取类型
     * @return 返回Property类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Property; }

    /**
     * @brief 转换为属性指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const ini_property* as_property() const noexcept override { return this; }

    /**
     * @brief 获取属性值
     * @return 属性值的常量引用
     */
    NEFORCE_NODISCARD const string& get_value() const noexcept { return value_; }

    /**
     * @brief 设置属性值
     * @param value 新的属性值
     */
    void set_value(string value) noexcept { value_ = _NEFORCE move(value); }

    /**
     * @brief 获取整数值
     * @param default_value 解析失败时的默认值
     * @return 解析后的整数值
     */
    NEFORCE_NODISCARD int get_int(int default_value = 0) const noexcept;

    /**
     * @brief 获取双精度浮点值
     * @param default_value 解析失败时的默认值
     * @return 解析后的双精度浮点值
     */
    NEFORCE_NODISCARD double get_double(double default_value = 0.0) const noexcept;

    /**
     * @brief 获取布尔值
     * @param default_value 解析失败时的默认值
     * @return 解析后的布尔值
     */
    NEFORCE_NODISCARD bool get_bool(bool default_value = false) const noexcept;
};


/**
 * @class ini_section
 * @brief ini节类
 *
 * 表示ini配置文件中的一个节(section)，包含多个属性。
 * 节可以有名（如[database]）或无名（全局节）。
 */
class NEFORCE_API ini_section final : public ini_value {
private:
    unordered_map<string, unique_ptr<ini_property>> properties_;  ///< 属性映射表
    string name_{};   ///< 节名称

public:
    ini_section() = default;

    /**
     * @brief 构造函数
     * @param name 节名称，默认为空（全局节）
     */
    explicit ini_section(string name) noexcept
    : name_(_NEFORCE move(name)) {}

    ini_section(const ini_section&) = delete;
    ini_section& operator =(const ini_section&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源节对象
     */
    ini_section(ini_section&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源节对象
     * @return 自身引用
     */
    ini_section& operator =(ini_section&& other) noexcept = default;

    /**
     * @brief 获取类型
     * @return 返回Section类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Section; }

    /**
     * @brief 转换为节指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const ini_section* as_section() const noexcept override { return this; }

    /**
     * @brief 获取节名称
     * @return 节名称的常量引用
     */
    NEFORCE_NODISCARD const string& get_name() const noexcept { return name_; }

    /**
     * @brief 设置节名称
     * @param name 新的节名称
     */
    void set_name(string name) noexcept { name_ = _NEFORCE move(name); }

    /**
     * @brief 添加属性
     * @param key 属性键名
     * @param property 属性智能指针
     */
    void add_property(const string& key, unique_ptr<ini_property> property) {
        properties_[key] = _NEFORCE move(property);
    }

    /**
     * @brief 设置属性
     * @param key 属性键名
     * @param value 属性值
     */
    void set_property(const string& key, string value) {
        properties_[key] = make_unique<ini_property>(move(value));
    }

    /**
     * @brief 获取常量属性指针
     * @param key 属性键名
     * @return 属性的常量指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const ini_property* get_property(const string& key) const {
        const auto it = properties_.find(key);
        if (it != properties_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 获取属性指针
     * @param key 属性键名
     * @return 属性的指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD ini_property* get_property(const string& key) {
        const auto it = properties_.find(key);
        if (it != properties_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 检查属性是否存在
     * @param key 属性键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_property(const string& key) const {
        return properties_.find(key) != properties_.end();
    }

    /**
     * @brief 获取所有属性的常量引用
     * @return 属性映射表的常量引用
     */
    NEFORCE_NODISCARD const unordered_map<string, unique_ptr<ini_property>>& get_properties() const noexcept {
        return properties_;
    }

    /**
     * @brief 获取字符串值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的字符串值
     */
    NEFORCE_NODISCARD string get_string(const string& key, const string& default_value = "") const {
        const ini_property* prop = get_property(key);
        return prop ? prop->get_value() : default_value;
    }

    /**
     * @brief 获取整数值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的整数值
     */
    NEFORCE_NODISCARD int get_int(const string& key, int default_value = 0) const {
        const ini_property* prop = get_property(key);
        return prop ? prop->get_int(default_value) : default_value;
    }

    /**
     * @brief 获取双精度浮点值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的双精度浮点值
     */
    NEFORCE_NODISCARD double get_double(const string& key, double default_value = 0.0) const {
        const ini_property* prop = get_property(key);
        return prop ? prop->get_double(default_value) : default_value;
    }

    /**
     * @brief 获取布尔值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的布尔值
     */
    NEFORCE_NODISCARD bool get_bool(const string& key, bool default_value = false) const {
        const ini_property* prop = get_property(key);
        return prop ? prop->get_bool(default_value) : default_value;
    }
};


/**
 * @class ini_document
 * @brief ini文档类
 *
 * 管理整个ini配置文件，包含多个节(section)和一个全局节。
 * 提供节的增删改查操作和类型安全的属性访问接口。
 */
class NEFORCE_API ini_document final {
private:
    unordered_map<string, unique_ptr<ini_section>> sections_;  ///< 节映射表
    unique_ptr<ini_section> global_section_;  ///< 全局节（无名节）

public:
    /**
     * @brief 构造函数
     *
     * 创建空文档，初始化全局节。
     */
    ini_document()
    : global_section_(make_unique<ini_section>("")) {}

    ini_document(const ini_document&) = delete;
    ini_document& operator =(const ini_document&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源文档
     */
    ini_document(ini_document&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源文档
     * @return 自身引用
     */
    ini_document& operator =(ini_document&& other) noexcept = default;

    /**
     * @brief 添加节
     * @param name 节名称
     * @param section 节智能指针
     *
     * 如果名称为空，则设置为全局节。
     */
    void add_section(const string& name, unique_ptr<ini_section> section) {
        if (name.empty()) {
            global_section_ = _NEFORCE move(section);
        } else {
            sections_[name] = _NEFORCE move(section);
        }
    }

    /**
     * @brief 获取常量节指针
     * @param name 节名称
     * @return 节的常量指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const ini_section* get_section(const string& name) const {
        if (name.empty()) return global_section_.get();
        const auto it = sections_.find(name);
        if (it != sections_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 获取节指针
     * @param name 节名称
     * @return 节的指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD ini_section* get_section(const string& name) {
        if (name.empty()) return global_section_.get();
        const auto it = sections_.find(name);
        if (it != sections_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 检查节是否存在
     * @param name 节名称
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_section(const string& name) const {
        if (name.empty()) return global_section_ != nullptr;
        return sections_.find(name) != sections_.end();
    }

    /**
     * @brief 获取所有节的常量引用
     * @return 节映射表的常量引用
     */
    NEFORCE_NODISCARD const unordered_map<string, unique_ptr<ini_section>>& get_sections() const noexcept {
        return sections_;
    }

    /**
     * @brief 获取全局节的常量指针
     * @return 全局节的常量指针
     */
    NEFORCE_NODISCARD const ini_section* get_global_section() const noexcept {
        return global_section_.get();
    }

    /**
     * @brief 获取全局节的指针
     * @return 全局节的指针
     */
    NEFORCE_NODISCARD ini_section* get_global_section() noexcept {
        return global_section_.get();
    }

    /**
     * @brief 获取字符串值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的字符串值
     */
    NEFORCE_NODISCARD string get_string(const string& section, const string& key, const string& default_value = "") const {
        const ini_section* sec = get_section(section);
        return sec ? sec->get_string(key, default_value) : default_value;
    }

    /**
     * @brief 获取整数值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的整数值
     */
    NEFORCE_NODISCARD int get_int(const string& section, const string& key, int default_value = 0) const {
        const ini_section* sec = get_section(section);
        return sec ? sec->get_int(key, default_value) : default_value;
    }

    /**
     * @brief 获取双精度浮点值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的双精度浮点值
     */
    NEFORCE_NODISCARD double get_double(const string& section, const string& key, double default_value = 0.0) const {
        const ini_section* sec = get_section(section);
        return sec ? sec->get_double(key, default_value) : default_value;
    }

    /**
     * @brief 获取布尔值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的布尔值
     */
    NEFORCE_NODISCARD bool get_bool(const string& section, const string& key, bool default_value = false) const {
        const ini_section* sec = get_section(section);
        return sec ? sec->get_bool(key, default_value) : default_value;
    }

    /**
     * @brief 转换为字符串
     * @return 整个文档的字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;
};

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief ini值转换为字符串的内部实现
 * @param value ini值指针
 * @return 字符串表示
 */
string NEFORCE_API ini_value_to_string(const ini_value* value);

/**
 * @brief ini文档转换为字符串的内部实现
 * @param doc ini文档指针
 * @return 字符串表示
 */
string NEFORCE_API ini_document_to_string(const ini_document* doc);

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief ini值指针转换为字符串
 * @param value ini值指针
 * @return 字符串表示
 */
NEFORCE_ALWAYS_INLINE_INLINE string to_string(const ini_value* value) {
    return inner::ini_value_to_string(value);
}

/**
 * @brief ini值引用转换为字符串
 * @param value ini值引用
 * @return 字符串表示
 */
NEFORCE_ALWAYS_INLINE_INLINE string to_string(const ini_value& value) {
    return inner::ini_value_to_string(&value);
}

/**
 * @brief ini值智能指针转换为字符串
 * @param value ini值智能指针
 * @return 字符串表示
 */
NEFORCE_ALWAYS_INLINE_INLINE string to_string(const unique_ptr<ini_value>& value) {
    return inner::ini_value_to_string(value.get());
}

/**
 * @brief ini文档转换为字符串
 * @param doc ini文档引用
 * @return 字符串表示
 */
NEFORCE_ALWAYS_INLINE_INLINE string to_string(const ini_document& doc) {
    return inner::ini_document_to_string(&doc);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE string ini_value::to_string() const {
    return inner::ini_value_to_string(this);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE string ini_value::to_document() const {
    return inner::ini_value_to_string(this);
}

NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE_INLINE string ini_document::to_string() const {
    return inner::ini_document_to_string(this);
}

/** @} */ // IniConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_INI_INI_VALUE_HPP__
