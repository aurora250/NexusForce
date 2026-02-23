#ifndef MSTL_CORE_FILE_ENV_ENV_VALUE_HPP__
#define MSTL_CORE_FILE_ENV_ENV_VALUE_HPP__

/**
 * @file env_value.hpp
 * @brief env配置格式变量
 *
 * 此文件提供了env配置格式的抽象基类和具体实现类。
 */

#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/interface/istringify.hpp"
#include "MSTL/core/memory/unique_ptr.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief MSTL异常类集
 * @{
 */

/**
 * @struct env_exception
 * @extends value_exception
 * @brief env格式操作失败
 */
MSTL_ERROR_BUILD_FINAL_CLASS(env_exception, value_exception, "ENV Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup EnvConfig env配置
 * @brief env配置格式管理
 * @{
 */

class env_variable;


/**
 * @class env_value
 * @brief 环境值抽象基类
 *
 * 提供环境值的统一接口，支持类型识别和字符串转换。
 */
class MSTL_API env_value : public istringify<env_value> {
public:
    /**
     * @enum types
     * @brief 环境值类型枚举
     */
    enum types {
        Variable  ///< 变量类型
    };

    /**
     * @brief 虚析构函数
     */
    virtual ~env_value() = default;

    /**
     * @brief 获取环境值类型
     * @return 类型枚举值
     */
    MSTL_NODISCARD virtual types type() const noexcept = 0;

    /**
     * @brief 转换为环境变量指针
     * @return 如果是变量类型返回自身指针，否则返回nullptr
     */
    MSTL_NODISCARD virtual const env_variable* as_variable() const noexcept { return nullptr; }

    /**
     * @brief 判断是否为变量类型
     * @return 如果是变量类型返回true
     */
    MSTL_NODISCARD bool is_variable() const noexcept { return type() == Variable; }

    /**
     * @brief 转换为字符串
     * @return 环境值的字符串表示
     */
    MSTL_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档字符串
     * @return 环境值的文档格式字符串
     */
    MSTL_NODISCARD string to_document() const;
};


/**
 * @class env_variable
 * @brief 环境变量类
 *
 * 表示一个具体的环境变量，包含变量值、引号类型和导出标记。
 * 提供类型转换方法和值访问接口。
 */
class MSTL_API env_variable final : public env_value {
public:
    /**
     * @enum quote_type
     * @brief 引号类型枚举
     */
    enum quote_type {
        None,   ///< 无引号
        Single, ///< 单引号
        Double  ///< 双引号
    };

private:
    string value_;                  ///< 变量值
    quote_type quote_type_ = None;  ///< 引号类型
    bool is_exported_ = false;      ///< 是否导出

public:
    /**
     * @brief 构造函数
     * @param value 变量值
     * @param quote 引号类型，默认为无引号
     * @param exported 是否导出，默认为false
     */
    explicit env_variable(string value, const quote_type quote = None, const bool exported = false) noexcept
    : value_(_MSTL move(value)), quote_type_(quote), is_exported_(exported) {}

    /**
     * @brief 获取类型
     * @return 返回Variable类型
     */
    MSTL_NODISCARD types type() const noexcept override { return Variable; }

    /**
     * @brief 转换为环境变量指针
     * @return 返回自身指针
     */
    MSTL_NODISCARD const env_variable* as_variable() const noexcept override { return this; }

    /**
     * @brief 获取变量值
     * @return 变量值的常量引用
     */
    MSTL_NODISCARD const string& get_value() const noexcept { return value_; }

    /**
     * @brief 设置变量值
     * @param value 新的变量值
     */
    void set_value(string value) noexcept { value_ = _MSTL move(value); }

    /**
     * @brief 获取引号类型
     * @return 当前引号类型
     */
    MSTL_NODISCARD quote_type get_quote_type() const noexcept { return quote_type_; }

    /**
     * @brief 设置引号类型
     * @param quote 新的引号类型
     */
    void set_quote_type(const quote_type quote) noexcept { quote_type_ = quote; }

    /**
     * @brief 检查是否导出
     * @return 是否导出
     */
    MSTL_NODISCARD bool is_exported() const noexcept { return is_exported_; }

    /**
     * @brief 设置导出标记
     * @param exported 新的导出标记
     */
    void set_exported(const bool exported) noexcept { is_exported_ = exported; }

    /**
     * @brief 获取整数值
     * @param default_value 解析失败时的默认值
     * @return 解析后的整数值
     */
    MSTL_NODISCARD int get_int(int default_value = 0) const noexcept;

    /**
     * @brief 获取64位整数值
     * @param default_value 解析失败时的默认值
     * @return 解析后的64位整数值
     */
    MSTL_NODISCARD int64_t get_int64(int64_t default_value = 0) const noexcept;

    /**
     * @brief 获取双精度浮点值
     * @param default_value 解析失败时的默认值
     * @return 解析后的双精度浮点值
     */
    MSTL_NODISCARD double get_double(double default_value = 0.0) const noexcept;

    /**
     * @brief 获取布尔值
     * @param default_value 解析失败时的默认值
     * @return 解析后的布尔值
     */
    MSTL_NODISCARD bool get_bool(bool default_value = false) const noexcept;
};


/**
 * @class env_document
 * @brief 环境变量文档类
 *
 * 管理多个环境变量和相关注释，提供变量的增删改查操作。
 * 支持将整个文档序列化为字符串。
 */
class MSTL_API env_document final {
private:
    unordered_map<string, unique_ptr<env_variable>> variables_;  ///< 变量映射表
    vector<string> comments_;  ///< 注释列表

public:
    /**
     * @brief 默认构造函数
     */
    env_document() = default;

    env_document(const env_document&) = delete;
    env_document& operator =(const env_document&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源文档
     */
    env_document(env_document&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源文档
     * @return 自身引用
     */
    env_document& operator =(env_document&& other) noexcept = default;

    /**
     * @brief 添加变量
     * @param name 变量名
     * @param variable 变量智能指针
     */
    void add_variable(const string& name, unique_ptr<env_variable> variable) {
        variables_[name] = _MSTL move(variable);
    }

    /**
     * @brief 设置变量
     * @param name 变量名
     * @param value 变量值
     * @param quote 引号类型，默认为无引号
     * @param exported 是否导出，默认为false
     */
    void set_variable(const string& name, string value,
                      env_variable::quote_type quote = env_variable::None,
                      bool exported = false) {
        variables_[name] = make_unique<env_variable>(_MSTL move(value), quote, exported);
    }

    /**
     * @brief 获取常量变量指针
     * @param name 变量名
     * @return 变量的常量指针，不存在返回nullptr
     */
    MSTL_NODISCARD const env_variable* get_variable(const string& name) const {
        const auto it = variables_.find(name);
        if (it != variables_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 获取变量指针
     * @param name 变量名
     * @return 变量的指针，不存在返回nullptr
     */
    MSTL_NODISCARD env_variable* get_variable(const string& name) {
        const auto it = variables_.find(name);
        if (it != variables_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 检查变量是否存在
     * @param name 变量名
     * @return 是否存在
     */
    MSTL_NODISCARD bool has_variable(const string& name) const {
        return variables_.find(name) != variables_.end();
    }

    /**
     * @brief 移除变量
     * @param name 要移除的变量名
     */
    void remove_variable(const string& name) {
        variables_.erase(name);
    }

    /**
     * @brief 获取所有变量的常量引用
     * @return 变量映射表的常量引用
     */
    MSTL_NODISCARD const unordered_map<string, unique_ptr<env_variable>>& get_variables() const noexcept {
        return variables_;
    }

    /**
     * @brief 添加注释
     * @param comment 注释内容
     */
    void add_comment(string comment) noexcept {
        comments_.emplace_back(move(comment));
    }

    /**
     * @brief 获取所有注释
     * @return 注释列表的常量引用
     */
    MSTL_NODISCARD const vector<string>& get_comments() const noexcept {
        return comments_;
    }

    /**
     * @brief 获取字符串值
     * @param name 变量名
     * @param default_value 默认值
     * @return 变量的字符串值
     */
    MSTL_NODISCARD string get_string(const string& name, const string& default_value = "") const {
        const auto* var = get_variable(name);
        return var ? var->get_value() : default_value;
    }

    /**
     * @brief 获取整数值
     * @param name 变量名
     * @param default_value 默认值
     * @return 变量的整数值
     */
    MSTL_NODISCARD int get_int(const string& name, const int default_value = 0) const {
        const auto* var = get_variable(name);
        return var ? var->get_int(default_value) : default_value;
    }

    /**
     * @brief 获取64位整数值
     * @param name 变量名
     * @param default_value 默认值
     * @return 变量的64位整数值
     */
    MSTL_NODISCARD int64_t get_int64(const string& name, const int64_t default_value = 0) const {
        const auto* var = get_variable(name);
        return var ? var->get_int64(default_value) : default_value;
    }

    /**
     * @brief 获取双精度浮点值
     * @param name 变量名
     * @param default_value 默认值
     * @return 变量的双精度浮点值
     */
    MSTL_NODISCARD double get_double(const string& name, const double default_value = 0.0) const {
        const auto* var = get_variable(name);
        return var ? var->get_double(default_value) : default_value;
    }

    /**
     * @brief 获取布尔值
     * @param name 变量名
     * @param default_value 默认值
     * @return 变量的布尔值
     */
    MSTL_NODISCARD bool get_bool(const string& name, const bool default_value = false) const {
        const auto* var = get_variable(name);
        return var ? var->get_bool(default_value) : default_value;
    }

    /**
     * @brief 转换为字符串
     * @return 整个文档的字符串表示
     */
    MSTL_NODISCARD string to_string() const;
};

/// @cond
MSTL_BEGIN_INNER__

/**
 * @brief 环境值转换为字符串的内部实现
 * @param value 环境值指针
 * @param key 变量键名
 * @return 字符串表示
 */
string MSTL_API env_value_to_string(const env_value* value, const string& key = "");

/**
 * @brief 环境文档转换为字符串的内部实现
 * @param doc 环境文档指针
 * @return 字符串表示
 */
string MSTL_API env_document_to_string(const env_document* doc);

MSTL_END_INNER__
/// @endcond


/**
 * @brief 环境值指针转换为字符串
 * @param value 环境值指针
 * @return 字符串表示
 */
MSTL_ALWAYS_INLINE_INLINE string to_string(const env_value* value) {
    return _INNER env_value_to_string(value);
}

/**
 * @brief 环境值引用转换为字符串
 * @param value 环境值引用
 * @return 字符串表示
 */
MSTL_ALWAYS_INLINE_INLINE string to_string(const env_value& value) {
    return _INNER env_value_to_string(&value);
}

/**
 * @brief 环境值智能指针转换为字符串
 * @param value 环境值智能指针
 * @return 字符串表示
 */
MSTL_ALWAYS_INLINE_INLINE string to_string(const unique_ptr<env_value>& value) {
    return _INNER env_value_to_string(value.get());
}

/**
 * @brief 环境文档转换为字符串
 * @param doc 环境文档引用
 * @return 字符串表示
 */
MSTL_ALWAYS_INLINE_INLINE string to_string(const env_document& doc) {
    return _INNER env_document_to_string(&doc);
}


MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string env_value::to_string() const {
    return _INNER env_value_to_string(this);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string env_value::to_document() const {
    return _INNER env_value_to_string(this);
}

MSTL_NODISCARD MSTL_ALWAYS_INLINE_INLINE string env_document::to_string() const {
    return _INNER env_document_to_string(this);
}

/** @} */ // EnvConfig

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_ENV_ENV_VALUE_HPP__
