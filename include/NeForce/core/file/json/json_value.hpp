#ifndef NEFORCE_CORE_FILE_JSON_JSON_VALUE_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_VALUE_HPP__

/**
 * @file json_value.hpp
 * @brief json配置格式变量
 *
 * 此文件提供了json配置格式的抽象基类和具体实现类。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct json_exception
 * @extends value_exception
 * @brief json格式操作失败
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(json_exception, value_exception, "Json String Parse Failed")

/** @} */ // Exceptions

/**
 * @defgroup JsonConfig json配置
 * @brief json配置格式管理
 * @{
 */

class json_value;
class json_null;
class json_bool;
class json_number;
class json_string;
class json_object;
class json_array;


/**
 * @class json_value
 * @brief json值抽象基类
 *
 * 提供json值的统一接口，支持类型识别和字符串转换。
 */
class NEFORCE_API json_value : public istringify<json_value> {
public:
    /**
     * @enum types
     * @brief json值类型枚举
     */
    enum types {
        Null,    ///< null值类型
        Bool,    ///< 布尔值类型
        Number,  ///< 数字类型
        String,  ///< 字符串类型
        Object,  ///< 对象类型
        Array    ///< 数组类型
    };

    /**
     * @brief 虚析构函数
     */
    virtual ~json_value() = default;

    /**
     * @brief 获取json值类型
     * @return 类型枚举值
     */
    NEFORCE_NODISCARD virtual types type() const noexcept = 0;

    /**
     * @brief 转换为null指针
     * @return 如果是null类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const json_null* as_null() const noexcept { return nullptr; }

    /**
     * @brief 转换为布尔值指针
     * @return 如果是布尔类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const json_bool* as_bool() const noexcept { return nullptr; }

    /**
     * @brief 转换为数字指针
     * @return 如果是数字类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const json_number* as_number() const noexcept { return nullptr; }

    /**
     * @brief 转换为字符串指针
     * @return 如果是字符串类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const json_string* as_string() const noexcept { return nullptr; }

    /**
     * @brief 转换为对象指针
     * @return 如果是对象类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const json_object* as_object() const noexcept { return nullptr; }

    /**
     * @brief 转换为数组指针
     * @return 如果是数组类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const json_array* as_array() const noexcept { return nullptr; }

    /**
     * @brief 判断是否为null类型
     * @return 如果是null类型返回true
     */
    NEFORCE_NODISCARD bool is_null() const noexcept { return type() == Null; }

    /**
     * @brief 判断是否为布尔类型
     * @return 如果是布尔类型返回true
     */
    NEFORCE_NODISCARD bool is_bool() const noexcept { return type() == Bool; }

    /**
     * @brief 判断是否为数字类型
     * @return 如果是数字类型返回true
     */
    NEFORCE_NODISCARD bool is_number() const noexcept { return type() == Number; }

    /**
     * @brief 判断是否为字符串类型
     * @return 如果是字符串类型返回true
     */
    NEFORCE_NODISCARD bool is_string() const noexcept { return type() == String; }

    /**
     * @brief 判断是否为对象类型
     * @return 如果是对象类型返回true
     */
    NEFORCE_NODISCARD bool is_object() const noexcept { return type() == Object; }

    /**
     * @brief 判断是否为数组类型
     * @return 如果是数组类型返回true
     */
    NEFORCE_NODISCARD bool is_array() const noexcept { return type() == Array; }

    /**
     * @brief 转换为紧凑格式字符串
     * @return json值的紧凑格式字符串
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为缩进格式字符串
     * @return json值的格式化字符串（默认2空格缩进）
     */
    NEFORCE_NODISCARD string to_indent_string() const;
};


/**
 * @class json_null
 * @brief json null值类
 *
 * 表示json中的null值。
 * 采用单例模式设计，所有null值共享同一个实例。
 */
class NEFORCE_API json_null final : public json_value {
public:
    /**
     * @brief 获取类型
     * @return 返回Null类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Null; }

    /**
     * @brief 转换为null指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const json_null* as_null() const noexcept override { return this; }
};

/**
 * @class json_bool
 * @brief json布尔值类
 *
 * 表示json中的true或false值。
 */
class NEFORCE_API json_bool final : public json_value {
private:
    bool value_; ///< 布尔值

public:
    /**
     * @brief 构造函数
     * @param value 布尔值
     */
    explicit json_bool(const bool value) noexcept : value_(value) {}

    /**
     * @brief 获取类型
     * @return 返回Bool类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Bool; }

    /**
     * @brief 转换为布尔指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const json_bool* as_bool() const noexcept override { return this; }

    /**
     * @brief 获取布尔值
     * @return 存储的布尔值
     */
    NEFORCE_NODISCARD bool get_value() const noexcept { return value_; }
};

/**
 * @class json_number
 * @brief json数字值类
 *
 * 表示json中的数字类型。
 * 内部使用double存储，支持整数和浮点数。
 */
class NEFORCE_API json_number final : public json_value {
private:
    double value_;  ///< 数字值

public:
    /**
     * @brief 构造函数
     * @param value 双精度浮点数值
     */
    explicit json_number(const double value) noexcept : value_(value) {}

    /**
     * @brief 获取类型
     * @return 返回Number类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Number; }

    /**
     * @brief 转换为数字指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const json_number* as_number() const noexcept override { return this; }

    /**
     * @brief 获取数字值
     * @return 存储的双精度浮点数值
     */
    NEFORCE_NODISCARD double get_value() const noexcept { return value_; }
};

/**
 * @class json_string
 * @brief json字符串值类
 *
 * 表示json中的字符串类型。
 * 存储原始的字符串值（不包含转义）。
 */
class NEFORCE_API json_string final : public json_value {
private:
    string value_;  ///< 字符串值

public:
    /**
     * @brief 构造函数
     * @param value 字符串值
     */
    explicit json_string(string value) noexcept : value_(_NEFORCE move(value)) {}

    /**
     * @brief 获取类型
     * @return 返回String类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return String; }

    /**
     * @brief 转换为字符串指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const json_string* as_string() const noexcept override { return this; }

    /**
     * @brief 获取字符串值
     * @return 字符串值的常量引用
     */
    NEFORCE_NODISCARD const string& get_value() const noexcept { return value_; }
};

/**
 * @class json_object
 * @brief json对象类
 *
 * 表示json中的对象类型，包含多个键值对成员。
 * 键为字符串，值为任意json类型。
 */
class NEFORCE_API json_object final : public json_value {
private:
    unordered_map<string, unique_ptr<json_value>> members_{};  ///< 成员映射表

public:
    /**
     * @brief 默认构造函数
     */
    json_object() = default;

    json_object(const json_object&) = delete;
    json_object& operator =(const json_object&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    json_object(json_object&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    json_object& operator =(json_object&& other) = default;

    /**
     * @brief 获取类型
     * @return 返回Object类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Object; }

    /**
     * @brief 转换为对象指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const json_object* as_object() const noexcept override { return this; }

    /**
     * @brief 添加成员
     * @param key 成员键名
     * @param value 成员值指针
     */
    void add_member(const string& key, unique_ptr<json_value> value) {
        members_[key] = _NEFORCE move(value);
    }

    /**
     * @brief 获取常量成员指针
     * @param key 成员键名
     * @return 成员的常量指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const json_value* get_member(const string& key) const {
        const auto it = members_.find(key);
        if (it != members_.end()) return it->second.get();
        return nullptr;
    }

    /**
     * @brief 获取所有成员的常量引用
     * @return 成员映射表的常量引用
     */
    NEFORCE_NODISCARD const unordered_map<string, unique_ptr<json_value>>& get_members() const noexcept {
        return members_;
    }
};

/**
 * @class json_array
 * @brief json数组类
 *
 * 表示json中的数组类型，包含有序的元素列表。
 * 每个元素可以是任意json类型。
 */
class NEFORCE_API json_array final : public json_value {
private:
    vector<unique_ptr<json_value>> elements_;  ///< 元素列表

public:
    /**
     * @brief 默认构造函数
     */
    json_array() = default;

    json_array(const json_array&) = delete;
    json_array& operator =(const json_array&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源数组
     */
    json_array(json_array&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源数组
     * @return 自身引用
     */
    json_array& operator =(json_array&& other) = default;

    /**
     * @brief 获取类型
     * @return 返回Array类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Array; }

    /**
     * @brief 转换为数组指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const json_array* as_array() const noexcept override { return this; }

    /**
     * @brief 添加元素
     * @param value 元素值指针
     */
    void add_element(unique_ptr<json_value> value) {
        elements_.emplace_back(_NEFORCE move(value));
    }

    /**
     * @brief 获取常量元素指针
     * @param index 元素索引
     * @return 元素的常量指针，索引越界返回nullptr
     */
    NEFORCE_NODISCARD const json_value* get_element(const size_t index) const noexcept  {
        if (index < elements_.size()) return elements_[index].get();
        return nullptr;
    }

    /**
     * @brief 获取数组大小
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return elements_.size(); }

    /**
     * @brief 获取所有元素的常量引用
     * @return 元素列表的常量引用
     */
    NEFORCE_NODISCARD const vector<unique_ptr<json_value>>& get_elements() const noexcept { return elements_; }
};

/** @} */ // JsonConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_VALUE_HPP__
