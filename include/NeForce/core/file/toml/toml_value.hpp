#ifndef NEFORCE_CORE_FILE_TOML_TOML_VALUE_HPP__
#define NEFORCE_CORE_FILE_TOML_TOML_VALUE_HPP__

/**
 * @file toml_value.hpp
 * @brief TOML配置格式变量
 *
 * 此文件提供了TOML（Tom's Obvious, Minimal Language）配置格式的抽象基类和具体实现类。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/datetime.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct toml_exception
 * @brief toml格式操作失败
 */
struct toml_exception final : value_exception {
    explicit toml_exception(const char* info = "TOML Operation Failed.", const char* type = static_type,
                            const int code = 0) noexcept :
    value_exception(info, type, code) {}

    explicit toml_exception(const exception& e) :
    value_exception(e) {}

    ~toml_exception() override = default;
    static constexpr auto static_type = "toml_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup ConfigFormat 配置格式操作
 * @brief env配置格式管理
 * @{
 */

/**
 * @defgroup TomlConfig toml配置
 * @brief toml配置格式管理
 *
 * TOML是一种易于阅读的配置文件格式，设计目标是比JSON更友好、比YAML更简单。
 *
 * 支持TOML v1.0.0规范中的所有数据类型：
 * - 布尔值（Boolean）
 * - 整数（Integer，64位有符号）
 * - 浮点数（Float，双精度）
 * - 字符串（String，支持四种引号类型）
 * - 日期时间（DateTime，支持四种日期时间格式）
 * - 数组（Array）
 * - 表格（Table，支持内联表格和标准表格）
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下 TOML 及相关标准规范：
 *
 * **TOML 配置格式规范：**
 * - **TOML v1.0.0**：Tom's Obvious, Minimal Language 规范
 *   https://toml.io/en/v1.0.0
 * - **TOML GitHub 规范**：TOML 语言定义与 ABNF 语法
 *   https://github.com/toml-lang/toml
 *
 * **相关日期时间标准（TOML 引用的日期时间格式）：**
 * - **IETF RFC 3339**：互联网上的日期和时间格式（TOML 偏移日期时间格式）
 *   https://www.rfc-editor.org/rfc/rfc3339.html
 * - **ISO 8601-1:2019**：日期和时间 — 信息交换的表示法
 *   https://www.iso.org/standard/70907.html
 *
 * **相关数据类型标准：**
 * - **IEEE 754-2019**：浮点数算术标准（TOML 浮点数遵循）
 *   https://standards.ieee.org/ieee/754/6210/
 * - **Unicode 15.0.0**：Unicode 字符编码标准（TOML 字符串编码）
 *   https://unicode.org/versions/Unicode15.0.0/
 *
 * @section toml_types TOML 值类型定义
 * 根据 TOML v1.0.0 规范，支持以下七种值类型：
 *
 * | 类型        | TOML 规范引用          | 本实现类          | 说明                               |
 * |-------------|------------------------|-------------------|------------------------------------|
 * | Boolean     | §2.1                   | toml_boolean      | true 或 false                       |
 * | Integer     | §2.2                   | toml_integer      | 64位有符号整数（-2^63 到 2^63-1）   |
 * | Float       | §2.3                   | toml_float        | IEEE 754 双精度浮点数               |
 * | String      | §2.4                   | toml_string       | Unicode 字符串（UTF-8 编码）        |
 * | Offset Date-Time | §2.5.1             | toml_datetime     | RFC 3339 格式带时区偏移             |
 * | Local Date-Time  | §2.5.2             | toml_datetime     | ISO 8601 格式无时区                 |
 * | Local Date       | §2.5.3             | toml_datetime     | 仅日期（YYYY-MM-DD）                |
 * | Local Time       | §2.5.4             | toml_datetime     | 仅时间（HH:MM:SS）                  |
 * | Array       | §2.6                   | toml_array        | 有序值列表（可混合类型）            |
 * | Table       | §2.7                   | toml_table        | 键值对集合（标准表格或内联表格）    |
 *
 * @section string_types 字符串引号类型
 * 根据 TOML v1.0.0 §2.4，支持四种字符串引号类型：
 *
 * | 类型           | 语法         | 转义序列 | 多行支持 | 说明                     |
 * |----------------|--------------|----------|----------|--------------------------|
 * | Basic          | "string"     | 支持     | 否       | 标准双引号字符串         |
 * | MultiBasic     | \"\"\"string\"\"\" | 支持 | 是       | 多行双引号字符串         |
 * | Literal        | 'string'     | 不支持   | 否       | 字面量单引号字符串       |
 * | MultiLiteral   | '''string''' | 不支持   | 是       | 多行字面量单引号字符串   |
 *
 * @section datetime_formats 日期时间格式
 * 根据 TOML v1.0.0 §2.5，支持四种日期时间格式：
 *
 * | 类型             | 示例                            | 标准引用            |
 * |------------------|---------------------------------|---------------------|
 * | Offset Date-Time | 1979-05-27T07:32:00Z           | RFC 3339 §5.6       |
 * | Offset Date-Time | 1979-05-27T00:32:00-07:00      | RFC 3339 §5.6       |
 * | Local Date-Time  | 1979-05-27T07:32:00            | ISO 8601-1:2019     |
 * | Local Date       | 1979-05-27                     | ISO 8601-1:2019     |
 * | Local Time       | 07:32:00                       | ISO 8601-1:2019     |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 编码              | UTF-8（TOML v1.0.0 §1）                    |
 * | 整数范围          | -2^63 到 2^63-1（TOML v1.0.0 §2.2）        |
 * | 浮点数精度        | IEEE 754-2019 双精度（TOML v1.0.0 §2.3）   |
 * | 字符串转义序列    | TOML v1.0.0 §2.4.1 定义                    |
 * | 键名规则          | 裸键、引号键、点分隔键（TOML v1.0.0 §3.1） |
 * | 表格类型          | 标准表格、内联表格、数组表格（§3.2-3.4）   |
 * | 数组元素混合类型  | 允许（TOML v1.0.0 §2.6）                   |
 * | 注释支持          | # 行注释（TOML v1.0.0 §1.2）                |
 *
 * @section string_escape 字符串转义序列
 * 根据 TOML v1.0.0 §2.4.1，支持以下转义序列（仅 Basic 和 MultiBasic 字符串）：
 *
 * | 转义序列 | 字符          | Unicode 码点 |
 * |----------|---------------|--------------|
 * | \\\"     | 引号          | U+0022       |
 * | \\\\     | 反斜杠        | U+005C       |
 * | \\b      | 退格          | U+0008       |
 * | \\f      | 换页          | U+000C       |
 * | \\n      | 换行          | U+000A       |
 * | \\r      | 回车          | U+000D       |
 * | \\t      | 制表符        | U+0009       |
 * | \\uXXXX  | Unicode 字符  | U+XXXX       |
 * | \\UXXXXXXXX | Unicode 字符（全范围） | U+XXXXXXXX |
 *
 * @warning 根据 TOML v1.0.0 §3.3，表格定义后不应再向该表格添加新的键值对。
 *          内联表格不应包含换行符，建议仅用于简单结构。
 *
 * @see https://toml.io/
 * @see https://github.com/toml-lang/toml/blob/main/toml.md
 * @see https://www.rfc-editor.org/rfc/rfc3339
 * @{
 */

class toml_value;
class toml_boolean;
class toml_integer;
class toml_float;
class toml_string;
class toml_datetime;
class toml_array;
class toml_table;


/**
 * @class toml_value
 * @brief toml值抽象基类
 *
 * 提供toml值的统一接口，支持类型识别和字符串转换。
 */
class NEFORCE_API toml_value : public istringify<toml_value> {
public:
    /**
     * @enum types
     * @brief toml值类型枚举
     */
    enum types {
        Boolean,  ///< 布尔值类型
        Integer,  ///< 整数类型
        Float,    ///< 浮点数类型
        String,   ///< 字符串类型
        DateTime, ///< 日期时间类型
        Array,    ///< 数组类型
        Table     ///< 表格类型
    };

    /**
     * @brief 虚析构函数
     */
    virtual ~toml_value() = default;

    /**
     * @brief 获取toml值类型
     * @return 类型枚举值
     */
    NEFORCE_NODISCARD virtual types type() const noexcept = 0;

    /**
     * @brief 转换为布尔值指针
     * @return 如果是布尔类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_boolean* as_boolean() const noexcept { return nullptr; }

    /**
     * @brief 转换为整数指针
     * @return 如果是整数类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_integer* as_integer() const noexcept { return nullptr; }

    /**
     * @brief 转换为浮点数指针
     * @return 如果是浮点数类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_float* as_float() const noexcept { return nullptr; }

    /**
     * @brief 转换为字符串指针
     * @return 如果是字符串类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_string* as_string() const noexcept { return nullptr; }

    /**
     * @brief 转换为日期时间指针
     * @return 如果是日期时间类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_datetime* as_datetime() const noexcept { return nullptr; }

    /**
     * @brief 转换为数组指针
     * @return 如果是数组类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_array* as_array() const noexcept { return nullptr; }

    /**
     * @brief 转换为表格指针
     * @return 如果是表格类型返回自身指针，否则返回nullptr
     */
    NEFORCE_NODISCARD virtual const toml_table* as_table() const noexcept { return nullptr; }

    /**
     * @brief 判断是否为布尔类型
     * @return 如果是布尔类型返回true
     */
    NEFORCE_NODISCARD bool is_boolean() const noexcept { return type() == Boolean; }

    /**
     * @brief 判断是否为整数类型
     * @return 如果是整数类型返回true
     */
    NEFORCE_NODISCARD bool is_integer() const noexcept { return type() == Integer; }

    /**
     * @brief 判断是否为浮点数类型
     * @return 如果是浮点数类型返回true
     */
    NEFORCE_NODISCARD bool is_float() const noexcept { return type() == Float; }

    /**
     * @brief 判断是否为字符串类型
     * @return 如果是字符串类型返回true
     */
    NEFORCE_NODISCARD bool is_string() const noexcept { return type() == String; }

    /**
     * @brief 判断是否为日期时间类型
     * @return 如果是日期时间类型返回true
     */
    NEFORCE_NODISCARD bool is_datetime() const noexcept { return type() == DateTime; }

    /**
     * @brief 判断是否为数组类型
     * @return 如果是数组类型返回true
     */
    NEFORCE_NODISCARD bool is_array() const noexcept { return type() == Array; }

    /**
     * @brief 判断是否为表格类型
     * @return 如果是表格类型返回true
     */
    NEFORCE_NODISCARD bool is_table() const noexcept { return type() == Table; }

    /**
     * @brief 转换为紧凑格式字符串
     * @return toml值的紧凑格式字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档格式字符串
     * @return toml值的完整文档格式字符串表示
     */
    NEFORCE_NODISCARD string to_document() const;
};


/**
 * @class toml_boolean
 * @brief toml布尔值类
 *
 * 表示toml中的true或false值。
 */
class NEFORCE_API toml_boolean final : public toml_value {
private:
    bool value_; ///< 布尔值

public:
    /**
     * @brief 构造函数
     * @param value 布尔值
     */
    explicit toml_boolean(const bool value) noexcept :
    value_(value) {}

    /**
     * @brief 获取类型
     * @return 返回Boolean类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Boolean; }

    /**
     * @brief 转换为布尔值指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_boolean* as_boolean() const noexcept override { return this; }

    /**
     * @brief 获取布尔值
     * @return 存储的布尔值
     */
    NEFORCE_NODISCARD bool get_value() const noexcept { return value_; }
};

/**
 * @class toml_integer
 * @brief toml整数值类
 *
 * 表示toml中的64位有符号整数。
 */
class NEFORCE_API toml_integer final : public toml_value {
private:
    int64_t value_; ///< 整数值

public:
    /**
     * @brief 构造函数
     * @param value 64位整数值
     */
    explicit toml_integer(const int64_t value) noexcept :
    value_(value) {}

    /**
     * @brief 获取类型
     * @return 返回Integer类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Integer; }

    /**
     * @brief 转换为整数指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_integer* as_integer() const noexcept override { return this; }

    /**
     * @brief 获取整数值
     * @return 存储的64位整数值
     */
    NEFORCE_NODISCARD int64_t get_value() const noexcept { return value_; }
};

/**
 * @class toml_float
 * @brief toml浮点数值类
 *
 * 表示toml中的双精度浮点数。
 */
class NEFORCE_API toml_float final : public toml_value {
private:
    double value_; ///< 浮点数值

public:
    /**
     * @brief 构造函数
     * @param value 双精度浮点数值
     */
    explicit toml_float(const double value) noexcept :
    value_(value) {}

    /**
     * @brief 获取类型
     * @return 返回Float类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Float; }

    /**
     * @brief 转换为浮点数指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_float* as_float() const noexcept override { return this; }

    /**
     * @brief 获取浮点数值
     * @return 存储的双精度浮点数值
     */
    NEFORCE_NODISCARD double get_value() const noexcept { return value_; }
};

/**
 * @class toml_string
 * @brief toml字符串值类
 *
 * 表示toml中的字符串，支持四种引号类型：
 * - Basic：双引号字符串 "string"
 * - Literal：单引号字符串 'string'
 * - MultiBasic：多行双引号字符串 """string"""
 * - MultiLiteral：多行单引号字符串 '''string'''
 */
class NEFORCE_API toml_string final : public toml_value {
public:
    /**
     * @enum string_type
     * @brief 字符串引号类型枚举
     */
    enum string_type {
        Basic,       ///< 基本字符串 "string"
        Literal,     ///< 字面量字符串 'string'
        MultiBasic,  ///< 多行基本字符串 """string"""
        MultiLiteral ///< 多行字面量字符串 '''string'''
    };

private:
    string value_;     ///< 字符串值
    string_type type_; ///< 引号类型

public:
    /**
     * @brief 构造函数
     * @param value 字符串值
     * @param type 引号类型，默认为Basic
     */
    explicit toml_string(string value, const string_type type = Basic) noexcept :
    value_(_NEFORCE move(value)),
    type_(type) {}

    /**
     * @brief 获取类型
     * @return 返回String类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return String; }

    /**
     * @brief 转换为字符串指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_string* as_string() const noexcept override { return this; }

    /**
     * @brief 获取字符串值
     * @return 字符串值的常量引用
     */
    NEFORCE_NODISCARD const string& get_value() const noexcept { return value_; }

    /**
     * @brief 获取字符串引号类型
     * @return 引号类型枚举值
     */
    NEFORCE_NODISCARD string_type get_string_type() const noexcept { return type_; }
};

/**
 * @class toml_datetime
 * @brief toml日期时间值类
 *
 * 表示toml中的日期时间类型，支持四种格式：
 * - OffsetDateTime：带时区偏移的完整日期时间 1979-05-27T07:32:00Z
 * - LocalDateTime：本地日期时间 1979-05-27T07:32:00
 * - LocalDate：本地日期 1979-05-27
 * - LocalTime：本地时间 07:32:00
 */
class NEFORCE_API toml_datetime final : public toml_value {
public:
    /**
     * @enum datetime_type
     * @brief 日期时间类型枚举
     */
    enum datetime_type {
        OffsetDateTime, ///< 偏移日期时间 1979-05-27T07:32:00Z
        LocalDateTime,  ///< 本地日期时间 1979-05-27T07:32:00
        LocalDate,      ///< 本地日期 1979-05-27
        LocalTime       ///< 本地时间 07:32:00
    };

private:
    datetime value_;     ///< 日期时间值
    datetime_type type_; ///< 日期时间类型

public:
    /**
     * @brief 构造函数
     * @param value 日期时间字符串视图
     * @param type 日期时间类型
     *
     * 根据指定的类型解析字符串并存储为datetime对象。
     */
    explicit toml_datetime(const string_view value, const datetime_type type) noexcept :
    type_(type) {
        switch (type_) {
            case datetime_type::OffsetDateTime: {
                datetime dt;
                dt.try_parse_ISO_UTC(value);
                value_ = dt;
                break;
            }
            case datetime_type::LocalDateTime: {
                datetime dt;
                dt.try_parse_ISO(value);
                value_ = dt;
                break;
            }
            case datetime_type::LocalDate: {
                date d{};
                d.try_parse(value);
                value_ = d;
                break;
            }
            case datetime_type::LocalTime: {
                time t{};
                t.try_parse(value);
                value_ = t;
                break;
            }
            default: {
                unreachable();
            }
        }
    }

    /**
     * @brief 获取类型
     * @return 返回DateTime类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return DateTime; }

    /**
     * @brief 转换为日期时间指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_datetime* as_datetime() const noexcept override { return this; }

    /**
     * @brief 获取日期时间值
     * @return datetime对象的常量引用
     */
    NEFORCE_NODISCARD const datetime& get_value() const noexcept { return value_; }

    /**
     * @brief 获取字符串格式的日期时间值
     * @return 根据类型格式化的日期时间字符串
     */
    NEFORCE_NODISCARD string get_string_value() const noexcept {
        switch (type_) {
            case datetime_type::OffsetDateTime: {
                return value_.to_string_ISO_UTC();
            }
            case datetime_type::LocalDateTime: {
                return value_.to_string_ISO();
            }
            case datetime_type::LocalDate: {
                return value_.date().to_string();
            }
            case datetime_type::LocalTime: {
                return value_.time().to_string();
            }
            default: {
                unreachable();
            }
        }
    }

    /**
     * @brief 获取日期时间类型
     * @return 日期时间类型枚举值
     */
    NEFORCE_NODISCARD datetime_type get_datetime_type() const noexcept { return type_; }
};

/**
 * @class toml_array
 * @brief toml数组类
 *
 * 表示toml中的数组类型，包含有序的元素列表。
 * 每个元素可以是任意toml值类型。
 */
class NEFORCE_API toml_array final : public toml_value {
private:
    vector<unique_ptr<toml_value>> elements_; ///< 元素列表

public:
    /**
     * @brief 默认构造函数
     */
    toml_array() = default;

    toml_array(const toml_array&) = delete;
    toml_array& operator=(const toml_array&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源数组
     */
    toml_array(toml_array&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源数组
     * @return 自身引用
     */
    toml_array& operator=(toml_array&& other) = default;

    /**
     * @brief 获取类型
     * @return 返回Array类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Array; }

    /**
     * @brief 转换为数组指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_array* as_array() const noexcept override { return this; }

    /**
     * @brief 添加元素
     * @param value 元素值指针
     */
    void add_element(unique_ptr<toml_value> value) { elements_.emplace_back(_NEFORCE move(value)); }

    /**
     * @brief 获取常量元素指针
     * @param index 元素索引
     * @return 元素的常量指针，索引越界返回nullptr
     */
    NEFORCE_NODISCARD const toml_value* get_element(const size_t index) const noexcept {
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
    NEFORCE_NODISCARD const vector<unique_ptr<toml_value>>& get_elements() const noexcept { return elements_; }
};

/**
 * @class toml_table
 * @brief toml表格类
 *
 * 表示toml中的表格类型，包含多个键值对成员。
 * 支持标准表格和内联表格两种形式。
 */
class NEFORCE_API toml_table final : public toml_value {
private:
    unordered_map<string, unique_ptr<toml_value>> members_{}; ///< 成员映射表
    bool is_inline_ = false;                                  ///< 是否为内联表格

public:
    /**
     * @brief 默认构造函数
     */
    toml_table() = default;

    /**
     * @brief 构造函数
     * @param is_inline 是否为内联表格
     */
    explicit toml_table(const bool is_inline) :
    is_inline_(is_inline) {}

    toml_table(const toml_table&) = delete;
    toml_table& operator=(const toml_table&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源表格
     */
    toml_table(toml_table&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源表格
     * @return 自身引用
     */
    toml_table& operator=(toml_table&& other) = default;

    /**
     * @brief 获取类型
     * @return 返回Table类型
     */
    NEFORCE_NODISCARD types type() const noexcept override { return Table; }

    /**
     * @brief 转换为表格指针
     * @return 返回自身指针
     */
    NEFORCE_NODISCARD const toml_table* as_table() const noexcept override { return this; }

    /**
     * @brief 添加成员
     * @param key 成员键名
     * @param value 成员值指针
     */
    void add_member(const string& key, unique_ptr<toml_value> value) { members_[key] = _NEFORCE move(value); }

    /**
     * @brief 获取常量成员指针
     * @param key 成员键名
     * @return 成员的常量指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD const toml_value* get_member(const string& key) const {
        const auto it = members_.find(key);
        if (it != members_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief 获取成员指针
     * @param key 成员键名
     * @return 成员的指针，不存在返回nullptr
     */
    NEFORCE_NODISCARD toml_value* get_member(const string& key) {
        const auto it = members_.find(key);
        if (it != members_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief 检查成员是否存在
     * @param key 成员键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_member(const string& key) const { return members_.find(key) != members_.end(); }

    /**
     * @brief 获取所有成员的常量引用
     * @return 成员映射表的常量引用
     */
    NEFORCE_NODISCARD const unordered_map<string, unique_ptr<toml_value>>& get_members() const noexcept {
        return members_;
    }

    /**
     * @brief 判断是否为内联表格
     * @return 是否为内联表格
     */
    NEFORCE_NODISCARD bool is_inline() const noexcept { return is_inline_; }

    /**
     * @brief 设置内联表格标记
     * @param is_inline 是否为内联表格
     */
    void set_inline(const bool is_inline) noexcept { is_inline_ = is_inline; }
};

/** @} */ // TomlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TOML_TOML_VALUE_HPP__
