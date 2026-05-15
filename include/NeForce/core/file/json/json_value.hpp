#ifndef NEFORCE_CORE_FILE_JSON_JSON_VALUE_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_VALUE_HPP__

/**
 * @file json_value.hpp
 * @brief JSON配置格式变量
 *
 * 此文件提供了JSON（JavaScript Object Notation）配置格式的抽象基类和具体实现类。
 * JSON是一种轻量级的数据交换格式，易于人类阅读和编写，也易于机器解析和生成。
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
 * @brief JSON格式操作失败
 */
struct json_exception final : value_exception {
    explicit json_exception(const char* info = "JSON Operation Failed.", const char* type = static_type,
                            const int code = 0) noexcept :
    value_exception(info, type, code) {}

    explicit json_exception(const exception& e) :
    value_exception(e) {}

    ~json_exception() override = default;
    static constexpr auto static_type = "json_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup ConfigFormat 配置格式操作
 * @brief 配置格式管理
 * @{
 */

/**
 * @defgroup JsonConfig JSON配置
 * @brief JSON配置格式管理
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下 JSON 相关标准规范：
 *
 * **JSON 数据格式标准：**
 * - **IETF STD 90 / RFC 8259**：JavaScript Object Notation (JSON) 数据交换格式
 *   https://www.rfc-editor.org/rfc/rfc8259.html
 * - **ECMA-404:2017**：JSON 数据交换格式（第2版）
 *   https://ecma-international.org/publications-and-standards/standards/ecma-404/
 *
 * **历史 JSON 规范（信息参考）：**
 * - **IETF RFC 7159**：JSON 数据交换格式（已被 RFC 8259 废弃）
 *   https://www.rfc-editor.org/rfc/rfc7159.html
 * - **IETF RFC 7158**：JSON 文本序列化格式
 *   https://www.rfc-editor.org/rfc/rfc7158.html
 * - **IETF RFC 4627**：JSON 原始规范（已被 RFC 7159 废弃）
 *   https://www.rfc-editor.org/rfc/rfc4627.html
 *
 * **JSON Schema 验证标准（相关参考）：**
 * - **IETF JSON Schema**：JSON 模式验证规范（Draft 2020-12）
 *   https://json-schema.org/specification.html
 * - **IETF RFC 8927**：JSON 类型定义
 *   https://www.rfc-editor.org/rfc/rfc8927.html
 *
 * @section json_types JSON 值类型定义
 * 根据 RFC 8259 §3，JSON 支持以下六种值类型：
 *
 * | 类型      | RFC 8259 引用 | 本实现类          | 说明                           |
 * |-----------|---------------|-------------------|--------------------------------|
 * | null      | §3            | json_null         | 空值                           |
 * | boolean   | §3            | json_bool         | 布尔值（true 或 false）         |
 * | number    | §6            | json_number       | 双精度浮点数（IEEE 754-2019）   |
 * | string    | §7            | json_string       | Unicode 字符串（UTF-8 编码）    |
 * | object    | §4            | json_object       | 无序键值对集合                  |
 * | array     | §5            | json_array        | 有序值列表                      |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 编码              | UTF-8（RFC 8259 §8.1）                    |
 * | 字符串转义        | RFC 8259 §7 定义的控制字符转义序列         |
 * | 数字范围          | IEEE 754-2019 双精度浮点数                |
 * | 对象键唯一性      | RFC 8259 §4（键在对象中应唯一）            |
 * | 对象键顺序        | 无序（JSON 不保证键的顺序）                |
 * | 数组顺序          | 有序（保持插入顺序）                       |
 * | 最大嵌套深度      | 实现定义（建议不超过 1000 层）             |
 *
 * @section escape_sequences 字符串转义序列
 * 根据 RFC 8259 §7，支持以下转义序列：
 *
 * | 转义序列 | 字符          | Unicode 码点 |
 * |----------|---------------|--------------|
 * | \\\"     | 引号          | U+0022       |
 * | \\\\     | 反斜杠        | U+005C       |
 * | \\/      | 斜杠          | U+002F       |
 * | \\b      | 退格          | U+0008       |
 * | \\f      | 换页          | U+000C       |
 * | \\n      | 换行          | U+000A       |
 * | \\r      | 回车          | U+000D       |
 * | \\t      | 制表符        | U+0009       |
 * | \\uXXXX  | Unicode 字符  | U+XXXX       |
 *
 * @note JSON 规范要求有效的 JSON 文本必须是 UTF-8、UTF-16 或 UTF-32 编码。
 *       本实现默认使用 UTF-8 编码处理所有字符串。
 *       JSON 数字采用双精度浮点数存储，遵循 IEEE 754-2019 标准。
 *
 * @warning 根据 RFC 8259 §9，JSON 解析器应拒绝无效的 UTF-8 序列。
 *          对象中的重复键名虽然规范不禁止，但可能导致不可预测的行为。
 *
 * @see https://www.json.org/
 * @see https://www.rfc-editor.org/rfc/rfc8259
 * @see https://ecma-international.org/publications-and-standards/standards/ecma-404/
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
 * @brief JSON值抽象基类
 *
 * 提供JSON值的统一接口，支持类型识别和字符串转换。
 */
class NEFORCE_API json_value : public istringify<json_value> {
public:
    /**
     * @enum types
     * @brief JSON值类型枚举
     */
    enum types {
        Null,   ///< null值类型
        Bool,   ///< 布尔值类型
        Number, ///< 数字类型
        String, ///< 字符串类型
        Object, ///< 对象类型
        Array   ///< 数组类型
    };

    /**
     * @brief 虚析构函数
     */
    virtual ~json_value() = default;

    /**
     * @brief 获取JSON值类型
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
     * @return JSON值的紧凑格式字符串
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为缩进格式字符串
     * @return JSON值的格式化字符串（默认2空格缩进）
     */
    NEFORCE_NODISCARD string to_indent_string() const;
};


/**
 * @class json_null
 * @brief JSON null值类
 *
 * 表示JSON中的null值。
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
 * @brief JSON布尔值类
 *
 * 表示JSON中的true或false值。
 */
class NEFORCE_API json_bool final : public json_value {
private:
    bool value_; ///< 布尔值

public:
    /**
     * @brief 构造函数
     * @param value 布尔值
     */
    explicit json_bool(const bool value) noexcept :
    value_(value) {}

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
 * @brief JSON数字值类
 *
 * 表示JSON中的数字类型。
 * 内部使用double存储，支持整数和浮点数。
 */
class NEFORCE_API json_number final : public json_value {
private:
    double value_; ///< 数字值

public:
    /**
     * @brief 构造函数
     * @param value 双精度浮点数值
     */
    explicit json_number(const double value) noexcept :
    value_(value) {}

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
 * @brief JSON字符串值类
 *
 * 表示JSON中的字符串类型。
 * 存储原始的字符串值（不包含转义）。
 */
class NEFORCE_API json_string final : public json_value {
private:
    string value_; ///< 字符串值

public:
    /**
     * @brief 构造函数
     * @param value 字符串值
     */
    explicit json_string(string value) noexcept :
    value_(_NEFORCE move(value)) {}

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
 * @brief JSON对象类
 *
 * 表示JSON中的对象类型，包含多个键值对成员。
 * 键为字符串，值为任意JSON类型。
 */
class NEFORCE_API json_object final : public json_value {
private:
    unordered_map<string, unique_ptr<json_value>> members_; ///< 成员映射表

public:
    /**
     * @brief 默认构造函数
     */
    json_object() = default;

    json_object(const json_object&) = delete;
    json_object& operator=(const json_object&) = delete;

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
    json_object& operator=(json_object&& other) = default;

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
    void add_member(const string& key, unique_ptr<json_value> value) { members_[key] = _NEFORCE move(value); }

    /**
     * @brief 获取常量成员指针
     * @param key 成员键名
     * @return 成员的常量指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const json_value* get_member(const string& key) const {
        const auto it = members_.find(key);
        if (it != members_.end()) {
            return it->second.get();
        }
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
 * @brief JSON数组类
 *
 * 表示JSON中的数组类型，包含有序的元素列表。
 * 每个元素可以是任意JSON类型。
 */
class NEFORCE_API json_array final : public json_value {
private:
    vector<unique_ptr<json_value>> elements_; ///< 元素列表

public:
    /**
     * @brief 默认构造函数
     */
    json_array() = default;

    json_array(const json_array&) = delete;
    json_array& operator=(const json_array&) = delete;

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
    json_array& operator=(json_array&& other) = default;

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
    void add_element(unique_ptr<json_value> value) { elements_.emplace_back(_NEFORCE move(value)); }

    /**
     * @brief 获取常量元素指针
     * @param index 元素索引
     * @return 元素的常量指针，索引越界返回nullptr
     */
    NEFORCE_NODISCARD const json_value* get_element(const size_t index) const noexcept {
        if (index < elements_.size()) {
            return elements_[index].get();
        }
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

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_VALUE_HPP__
