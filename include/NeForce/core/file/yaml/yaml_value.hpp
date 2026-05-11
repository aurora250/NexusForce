#ifndef NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__

/**
 * @file yaml_value.hpp
 * @brief YAML配置格式变量
 *
 * 此文件提供了YAML（YAML Ain't Markup Language）配置格式的抽象基类和具体实现类。
 * YAML是一种人类可读的数据序列化语言，常用于配置文件、数据交换和持久化存储。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/core/time/datetime.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @defgroup YamlConfig YAML配置
 * @brief YAML（YAML Ain't Markup Language）1.2 配置格式支持
 *
 * 本模块提供 YAML 1.2.2 规范的完整实现，包含解析（yaml_parser）、
 * 构建（yaml_builder）和值类型（yaml_value 层次结构）。
 *
 * YAML 1.2 是 JSON 的严格超集（IETF RFC 8259），支持更丰富的数据类型
 * 和更灵活的表达方式，广泛应用于配置文件、数据交换和持久化存储。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下 YAML 及相关标准规范：
 *
 * **YAML 数据序列化规范：**
 * - **YAML 1.2.2 (2021)**：YAML Ain't Markup Language 规范（第3版）
 *   https://yaml.org/spec/1.2.2/
 * - **YAML 1.2 (2009)**：YAML 1.2 规范（第3版预览）
 *   https://yaml.org/spec/1.2/
 * - **YAML 1.1 (2005)**：YAML 1.1 规范（第2版，已废弃）
 *   https://yaml.org/spec/1.1/
 *
 * **JSON 兼容性标准（YAML 1.2 是 JSON 的超集）：**
 * - **IETF RFC 8259**：JSON 数据交换格式（YAML 1.2 完全兼容）
 *   https://www.rfc-editor.org/rfc/rfc8259.html
 * - **ECMA-404:2017**：JSON 数据交换格式
 *   https://ecma-international.org/publications-and-standards/standards/ecma-404/
 *
 * **时间戳格式标准（YAML 1.2 §10.4.2 引用）：**
 * - **IETF RFC 3339**：互联网日期时间格式
 *   https://www.rfc-editor.org/rfc/rfc3339.html
 * - **ISO 8601-1:2019**：日期和时间表示法
 *   https://www.iso.org/standard/70907.html
 *
 * **Unicode 编码标准：**
 * - **Unicode 15.0.0**：Unicode 字符编码标准（YAML 1.2 §5.1）
 *   https://unicode.org/versions/Unicode15.0.0/
 * - **ISO/IEC 10646:2020**：通用编码字符集（UCS）
 *   https://www.iso.org/standard/76835.html
 *
 * **浮点数标准：**
 * - **IEEE 754-2019**：浮点数算术标准（YAML 数字遵循）
 *   https://standards.ieee.org/ieee/754/6210/
 *
 * @section yaml_types YAML 值类型定义
 * 根据 YAML 1.2.2 规范，支持以下八种核心值类型：
 *
 * | 类型        | YAML 1.2.2 引用 | 本实现类          | 说明                               |
 * |-------------|-----------------|-------------------|------------------------------------|
 * | Null        | §10.2           | yaml_null         | null、Null、NULL、~                 |
 * | Boolean     | §10.3           | yaml_boolean      | true/false、True/False、yes/no等   |
 * | Integer     | §10.4.1         | yaml_integer      | 64位有符号整数（支持十进制、十六进制、八进制） |
 * | Float       | §10.4.1         | yaml_float        | IEEE 754 双精度浮点数（支持 .inf、-.inf、.nan） |
 * | String      | §10.5           | yaml_string       | Unicode 字符串（UTF-8/UTF-16/UTF-32） |
 * | Timestamp   | §10.4.2         | yaml_timestamp    | ISO 8601 / RFC 3339 时间戳         |
 * | Sequence    | §10.1.1         | yaml_sequence     | 有序值列表（数组）                  |
 * | Mapping     | §10.1.2         | yaml_mapping      | 键值对集合（字典/对象）             |
 *
 * @section string_styles 字符串标量样式
 * 根据 YAML 1.2.2 §7.3，支持五种字符串标量样式：
 *
 * | 样式           | 语法示例           | 转义序列 | 换行处理 | 说明                     |
 * |----------------|--------------------|----------|----------|--------------------------|
 * | Plain          | string             | 不支持   | 折叠空格 | 无引号纯文本              |
 * | SingleQuoted   | 'string'           | 有限支持 | 保留     | 单引号字符串              |
 * | DoubleQuoted   | "string"           | 完全支持 | 保留     | 双引号字符串（含转义）    |
 * | Literal        | \|                 | 不支持   | 保留     | 块字面量（保留换行）      |
 * | Folded         | >                  | 不支持   | 折叠     | 块折叠（换行转空格）      |
 *
 * @section collection_styles 集合样式
 * 根据 YAML 1.2.2 §7.4，序列和映射支持两种集合样式：
 *
 * | 类型    | 块样式（Block）        | 流样式（Flow）         |
 * |---------|------------------------|------------------------|
 * | 序列    | - item1\n- item2       | [item1, item2]         |
 * | 映射    | key1: value1\nkey2: value2 | {key1: value1, key2: value2} |
 *
 * @section boolean_synonyms 布尔值同义词
 * 根据 YAML 1.2.2 §10.3.2，支持以下布尔值同义词：
 *
 * | 语义值 | 规范形式 | 同义词                                        |
 * |--------|----------|-----------------------------------------------|
 * | true   | true     | True、TRUE、y、Y、yes、Yes、YES、on、On、ON   |
 * | false  | false    | False、FALSE、n、N、no、No、NO、off、Off、OFF |
 *
 * @section null_synonyms 空值同义词
 * 根据 YAML 1.2.2 §10.2.2，支持以下空值同义词：
 *
 * | 规范形式 | 同义词                              |
 * |----------|-------------------------------------|
 * | null     | Null、NULL、~（空字符串在某些上下文中） |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 编码              | UTF-8（YAML 1.2 §5.1）                     |
 * | 整数范围          | -2^63 到 2^63-1（YAML 1.2 §10.4.1）       |
 * | 浮点数精度        | IEEE 754-2019 双精度（YAML 1.2 §10.4.1）  |
 * | 字符串转义序列    | YAML 1.2 §5.7 定义                         |
 * | 键名唯一性        | YAML 1.2 §3.2.1（映射中键名应唯一）        |
 * | 缩进要求          | 空格字符（YAML 1.2 §6.1），禁止制表符      |
 * | 注释支持          | # 行注释（YAML 1.2 §6.8）                   |
 * | 锚点与别名        | &anchor 和 *alias（YAML 1.2 §3.2.2）       |
 * | 标签系统          | !tag 和 !!type（YAML 1.2 §3.2.3）          |
 *
 * @section string_escape 字符串转义序列
 * 根据 YAML 1.2.2 §5.7，双引号字符串支持以下转义序列：
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
 * | \\UXXXXXXXX | Unicode 字符（全范围） | U+XXXXXXXX |
 *
 * @section yaml_vs_json YAML 1.2 与 JSON 兼容性
 * YAML 1.2 是 JSON 的严格超集，所有有效的 JSON 文档也是有效的 YAML 文档。
 * 这意味着 JSON 可以作为 YAML 的子集进行解析。
 *
 * | 特性              | JSON                    | YAML 1.2                      |
 * |-------------------|-------------------------|-------------------------------|
 * | 注释              | 不支持                  | # 行注释                       |
 * | 字符串引号        | 仅双引号                | 五种样式                       |
 * | 尾部逗号          | 不允许                  | 允许（可选）                   |
 * | 键名引号          | 必须引号                | 可选（裸键）                   |
 * | 锚点与别名        | 不支持                  | &anchor 和 *alias              |
 * | 标签              | 不支持                  | !tag 和 !!type                 |
 *
 * @warning 根据 YAML 1.2 §6.1，制表符（tab）不应用于缩进，仅允许在内容中使用。
 *          流样式映射中重复的键名行为未定义，应避免使用。
 * @warning 根据 YAML 1.2 §3.2.1，映射中的键名在 YAML 1.2 中应唯一。
 *
 * @see https://yaml.org/
 * @see https://yaml.org/spec/1.2.2/
 * @see https://json.org/
 * @see https://www.rfc-editor.org/rfc/rfc8259
 * @{
 */

NEFORCE_ERROR_BUILD_FINAL_CLASS(yaml_exception, value_exception, "YAML Operation Failed.")

class yaml_value;
class yaml_null;
class yaml_boolean;
class yaml_integer;
class yaml_float;
class yaml_string;
class yaml_timestamp;
class yaml_sequence;
class yaml_mapping;


/**
 * @class yaml_value
 * @brief YAML值的抽象基类
 *
 * 所有YAML值类型的基类，定义了YAML值树的公共接口和类型系统。
 *
 * 每个 yaml_value 节点可携带可选的锚点和标签元数据：
 * - `anchor`：锚点名（对应YAML中的 &anchor 语法）
 * - `tag`：类型标签（对应YAML中的 !tag 语法）
 *
 * @note 此类为抽象基类，不可直接实例化。
 * @note 使用 type() 配合 as_*() 方法进行类型安全的向下转型。
 */
class NEFORCE_API yaml_value : public istringify<yaml_value> {
public:
    string anchor; ///< 锚点名（YAML &anchor 语法），空字符串表示无锚点
    string tag;    ///< 类型标签（YAML !tag 语法），空字符串表示无标签

    /**
     * @enum types
     * @brief YAML值类型枚举
     */
    enum types {
        Null,      ///< 空值类型
        Boolean,   ///< 布尔值类型
        Integer,   ///< 64位有符号整数类型
        Float,     ///< 双精度浮点数类型
        String,    ///< Unicode字符串类型
        Timestamp, ///< ISO 8601 / RFC 3339 时间戳类型
        Sequence,  ///< 有序值列表类型
        Mapping    ///< 键值对集合类型
    };

    virtual ~yaml_value() = default;

    /**
     * @brief 获取值的具体类型
     * @return 类型枚举值
     */
    NEFORCE_NODISCARD virtual types type() const noexcept = 0;

    /// @name 类型转换方法
    /// 安全地向下转型到具体类型，失败时返回 nullptr。
    /// @{
    NEFORCE_NODISCARD virtual const yaml_null* as_null() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_boolean* as_boolean() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_integer* as_integer() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_float* as_float() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_string* as_string() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_timestamp* as_timestamp() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_sequence* as_sequence() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_mapping* as_mapping() const noexcept { return nullptr; }
    /// @}

    /// @name 类型检查方法
    /// 便捷的类型判断方法，等价于 type() == types::Xxx。
    /// @{
    NEFORCE_NODISCARD bool is_null() const noexcept { return type() == Null; }
    NEFORCE_NODISCARD bool is_boolean() const noexcept { return type() == Boolean; }
    NEFORCE_NODISCARD bool is_integer() const noexcept { return type() == Integer; }
    NEFORCE_NODISCARD bool is_float() const noexcept { return type() == Float; }
    NEFORCE_NODISCARD bool is_string() const noexcept { return type() == String; }
    NEFORCE_NODISCARD bool is_timestamp() const noexcept { return type() == Timestamp; }
    NEFORCE_NODISCARD bool is_sequence() const noexcept { return type() == Sequence; }
    NEFORCE_NODISCARD bool is_mapping() const noexcept { return type() == Mapping; }
    /// @}

    /**
     * @brief 设置锚点名
     * @param a 锚点名字符串（对应 &anchor 语法）
     */
    void set_anchor(const string& a) { this->anchor = a; }

    /**
     * @brief 设置类型标签
     * @param t 标签字符串（对应 !tag 语法）
     */
    void set_tag(const string& t) { this->tag = t; }

    /**
     * @brief 紧凑单行序列化
     * @return 流样式的YAML字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 格式化文档序列化
     * @return 块样式的YAML文档表示（以换行结尾）
     */
    NEFORCE_NODISCARD string to_document() const;
};

/**
 * @class yaml_null
 * @brief YAML空值类型
 *
 * 表示YAML中的空值（null、Null、NULL、~）。
 * 这是一个简单的标记类型，不包含任何数据。
 */
class NEFORCE_API yaml_null final : public yaml_value {
public:
    yaml_null() = default;
    NEFORCE_NODISCARD types type() const noexcept override { return Null; }
    NEFORCE_NODISCARD const yaml_null* as_null() const noexcept override { return this; }
};

/**
 * @class yaml_boolean
 * @brief YAML布尔值类型
 *
 * 表示YAML中的布尔值。支持所有YAML 1.2布尔同义词：
 * true、True、TRUE、y、Y、yes、Yes、YES、on、On、ON
 * false、False、FALSE、n、N、no、No、NO、off、Off、OFF
 */
class NEFORCE_API yaml_boolean final : public yaml_value {
private:
    bool value; ///< 布尔值存储

public:
    /**
     * @brief 构造布尔值
     * @param v 布尔值
     */
    explicit yaml_boolean(const bool v) noexcept :
    value(v) {}

    NEFORCE_NODISCARD types type() const noexcept override { return Boolean; }
    NEFORCE_NODISCARD const yaml_boolean* as_boolean() const noexcept override { return this; }

    /**
     * @brief 获取布尔值
     * @return 存储的布尔值
     */
    NEFORCE_NODISCARD bool get_value() const noexcept { return value; }
};

/**
 * @class yaml_integer
 * @brief YAML整数值类型
 *
 * 表示YAML中的整数值。存储为64位有符号整数（int64_t），
 * 范围 -2^63 到 2^63-1。支持YAML中的各种整数表示法：
 * - 十进制：42、-17、+99
 * - 十六进制：0x2A、0xFF
 * - 八进制：0o52、0o77
 * - 二进制：0b101010
 * - 支持下划线分隔：1_000_000
 */
class NEFORCE_API yaml_integer final : public yaml_value {
private:
    int64_t value; ///< 64位有符号整数存储

public:
    /**
     * @brief 构造整数值
     * @param v 64位有符号整数
     */
    explicit yaml_integer(const int64_t v) noexcept :
    value(v) {}

    NEFORCE_NODISCARD types type() const noexcept override { return Integer; }
    NEFORCE_NODISCARD const yaml_integer* as_integer() const noexcept override { return this; }

    /**
     * @brief 获取整数值
     * @return 存储的64位有符号整数
     */
    NEFORCE_NODISCARD int64_t get_value() const noexcept { return value; }
};

/**
 * @class yaml_float
 * @brief YAML浮点数值类型
 *
 * 表示YAML中的浮点数值。存储为IEEE 754双精度浮点数（double）。
 * 支持YAML中的特殊浮点值：
 * - 无穷大：.inf、.Inf、.INF
 * - 负无穷大：-.inf、-.Inf、-.INF
 * - 非数字：.nan、.NaN、.NAN
 * - 科学记数法：1.23e+4、5.67E-10
 */
class NEFORCE_API yaml_float final : public yaml_value {
private:
    double value; ///< 双精度浮点数存储

public:
    /**
     * @brief 构造浮点数值
     * @param v 双精度浮点数
     */
    explicit yaml_float(const double v) noexcept :
    value(v) {}

    NEFORCE_NODISCARD types type() const noexcept override { return Float; }
    NEFORCE_NODISCARD const yaml_float* as_float() const noexcept override { return this; }

    /**
     * @brief 获取浮点数值
     * @return 存储的双精度浮点数
     */
    NEFORCE_NODISCARD double get_value() const noexcept { return value; }
};

/**
 * @class yaml_string
 * @brief YAML字符串值类型
 *
 * 表示YAML中的字符串值。支持YAML 1.2定义的五种标量样式：
 *
 * | 样式        | 枚举值        | 语法示例                | 说明                   |
 * |-------------|---------------|-------------------------|------------------------|
 * | Plain       | Plain         | hello world             | 无引号纯文本            |
 * | SingleQuoted| SingleQuoted  | 'hello world'           | 单引号，'' 表示字面引号 |
 * | DoubleQuoted| DoubleQuoted  | "hello\\nworld"         | 双引号，支持转义序列    |
 * | Literal     | Literal       | \\|\\n  line1\\n  line2 | 块字面量，保留换行      |
 * | Folded      | Folded        | >\\n  line1\\n  line2   | 块折叠，换行转空格      |
 */
class NEFORCE_API yaml_string final : public yaml_value {
public:
    /**
     * @enum string_style
     * @brief 字符串标量样式枚举
     */
    enum string_style {
        Plain,        ///< 纯文本样式（无引号）
        SingleQuoted, ///< 单引号样式（'string'）
        DoubleQuoted, ///< 双引号样式（"string"，支持转义）
        Literal,      ///< 块字面量样式（|，保留换行）
        Folded        ///< 块折叠样式（>，换行转空格）
    };

private:
    string value;       ///< 字符串内容
    string_style style; ///< 标量样式

public:
    /**
     * @brief 构造字符串值
     * @param v 字符串内容
     * @param s 标量样式，默认为纯文本
     */
    explicit yaml_string(string v, const string_style s = Plain) noexcept :
    value(_NEFORCE move(v)),
    style(s) {}

    NEFORCE_NODISCARD types type() const noexcept override { return String; }
    NEFORCE_NODISCARD const yaml_string* as_string() const noexcept override { return this; }

    /**
     * @brief 获取字符串内容
     * @return 字符串常量引用
     */
    NEFORCE_NODISCARD const string& get_value() const noexcept { return value; }

    /**
     * @brief 获取标量样式
     * @return 字符串样式枚举值
     */
    NEFORCE_NODISCARD string_style get_style() const noexcept { return style; }
};

/**
 * @class yaml_timestamp
 * @brief YAML时间戳值类型
 *
 * 表示YAML中的时间戳值。内部使用 datetime 对象存储，
 * 支持 ISO 8601 和 RFC 3339 格式的解析。
 *
 * 支持的格式示例：
 * - 日期时间：2024-01-15T10:30:00Z
 * - 仅日期：2024-01-15
 * - 带时区偏移：2024-01-15T10:30:00+08:00
 *
 * @see datetime 底层日期时间类型
 */
class NEFORCE_API yaml_timestamp final : public yaml_value {
private:
    datetime value; ///< 日期时间存储

public:
    /**
     * @brief 从字符串构造时间戳（自动检测格式）
     * @param v ISO 8601 / RFC 3339 格式的字符串
     * @throws yaml_exception 格式无效时抛出
     */
    explicit yaml_timestamp(const string_view v) {
        datetime dt;
        if (dt.try_parse_RFC3339(v) || dt.try_parse_ISO8601(v)) {
            value = dt;
        } else {
            NEFORCE_THROW_EXCEPTION(yaml_exception(("Invalid timestamp format: " + string(v)).data()));
        }
    }

    /**
     * @brief 从 datetime 对象构造时间戳
     * @param dt 日期时间对象
     */
    explicit yaml_timestamp(const datetime& dt) noexcept :
    value(dt) {}

    NEFORCE_NODISCARD types type() const noexcept override { return Timestamp; }
    NEFORCE_NODISCARD const yaml_timestamp* as_timestamp() const noexcept override { return this; }

    /**
     * @brief 获取日期时间值
     * @return datetime 对象的常量引用
     */
    NEFORCE_NODISCARD const datetime& get_value() const noexcept { return value; }

    /**
     * @brief 获取 RFC 3339 格式的字符串表示
     * @return 格式化的时间戳字符串
     */
    NEFORCE_NODISCARD string get_string_value() const noexcept { return value.to_RFC3339(); }
};

/**
 * @class yaml_sequence
 * @brief YAML序列值类型（数组）
 *
 * 表示YAML中的有序值列表。支持两种集合样式：
 * - **块样式**（Block）：每行以 - 开头的缩进列表
 * - **流样式**（Flow）：方括号内的逗号分隔列表 [a, b, c]
 *
 * 序列中的元素可以是任意YAML值类型（包括嵌套的序列和映射）。
 *
 * @note 此类禁止拷贝，仅允许移动。
 */
class NEFORCE_API yaml_sequence final : public yaml_value {
public:
    /**
     * @enum sequence_style
     * @brief 序列集合样式枚举
     */
    enum sequence_style {
        Block, ///< 块样式（- item）
        Flow   ///< 流样式（[item, ...]）
    };

private:
    vector<shared_ptr<yaml_value>> elements; ///< 元素列表
    sequence_style style;                    ///< 集合样式

public:
    /**
     * @brief 构造序列
     * @param s 集合样式，默认为块样式
     */
    explicit yaml_sequence(const sequence_style s = Block) :
    style(s) {}

    yaml_sequence(const yaml_sequence&) = delete;
    yaml_sequence& operator=(const yaml_sequence&) = delete;
    yaml_sequence(yaml_sequence&&) = default;
    yaml_sequence& operator=(yaml_sequence&&) = default;

    NEFORCE_NODISCARD types type() const noexcept override { return Sequence; }
    NEFORCE_NODISCARD const yaml_sequence* as_sequence() const noexcept override { return this; }

    /**
     * @brief 添加元素到序列末尾
     * @param value 要添加的YAML值
     */
    void add_element(shared_ptr<yaml_value> value) { elements.emplace_back(_NEFORCE move(value)); }

    /**
     * @brief 获取指定索引的元素（常量版本）
     * @param index 元素索引（从0开始）
     * @return 元素指针，索引越界返回 nullptr
     */
    NEFORCE_NODISCARD const yaml_value* get_element(const size_t index) const noexcept {
        if (index < elements.size()) {
            return elements[index].get();
        }
        return nullptr;
    }

    /**
     * @brief 获取指定索引的元素（可变版本）
     * @param index 元素索引（从0开始）
     * @return 元素指针，索引越界返回 nullptr
     */
    NEFORCE_NODISCARD yaml_value* get_element(const size_t index) noexcept {
        if (index < elements.size()) {
            return elements[index].get();
        }
        return nullptr;
    }

    /**
     * @brief 获取序列大小
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return elements.size(); }

    /**
     * @brief 获取所有元素的常量引用
     * @return 元素列表的常量引用
     */
    NEFORCE_NODISCARD const vector<shared_ptr<yaml_value>>& get_elements() const noexcept { return elements; }

    /**
     * @brief 获取集合样式
     * @return 序列样式枚举值
     */
    NEFORCE_NODISCARD sequence_style get_style() const noexcept { return style; }

    /**
     * @brief 设置集合样式
     * @param s 新的序列样式
     */
    void set_style(const sequence_style s) noexcept { style = s; }
};

/**
 * @class yaml_mapping
 * @brief YAML映射值类型（字典/对象）
 *
 * 表示YAML中的键值对集合。支持两种集合样式：
 * - **块样式**（Block）：每行 key: value 的缩进格式
 * - **流样式**（Flow）：花括号内的逗号分隔格式 {key: value, ...}
 *
 * 映射中的键名为字符串类型，值可以是任意YAML值类型
 * （包括嵌套的序列和映射）。
 *
 * @note 此类禁止拷贝，仅允许移动。
 * @note 键名比较基于字符串相等，不区分YAML标量样式。
 */
class NEFORCE_API yaml_mapping final : public yaml_value {
public:
    /**
     * @enum mapping_style
     * @brief 映射集合样式枚举
     */
    enum mapping_style {
        Block, ///< 块样式（key: value）
        Flow   ///< 流样式（{key: value}）
    };

private:
    unordered_map<string, shared_ptr<yaml_value>> members; ///< 键值对存储
    mapping_style style;                                   ///< 集合样式

public:
    /**
     * @brief 构造映射
     * @param s 集合样式，默认为块样式
     */
    explicit yaml_mapping(const mapping_style s = Block) :
    style(s) {}

    yaml_mapping(const yaml_mapping&) = delete;
    yaml_mapping& operator=(const yaml_mapping&) = delete;
    yaml_mapping(yaml_mapping&&) = default;
    yaml_mapping& operator=(yaml_mapping&&) = default;

    NEFORCE_NODISCARD types type() const noexcept override { return Mapping; }
    NEFORCE_NODISCARD const yaml_mapping* as_mapping() const noexcept override { return this; }

    /**
     * @brief 添加或覆盖键值对
     * @param key 键名字符串
     * @param value 关联的YAML值
     */
    void add_member(const string& key, shared_ptr<yaml_value> value) { members[key] = _NEFORCE move(value); }

    /**
     * @brief 获取指定键名的成员（常量版本）
     * @param key 键名字符串
     * @return 值指针，键不存在返回 nullptr
     */
    NEFORCE_NODISCARD const yaml_value* get_member(const string& key) const {
        const auto it = members.find(key);
        if (it != members.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief 获取指定键名的成员（可变版本）
     * @param key 键名字符串
     * @return 值指针，键不存在返回 nullptr
     */
    NEFORCE_NODISCARD yaml_value* get_member(const string& key) {
        const auto it = members.find(key);
        if (it != members.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    /**
     * @brief 检查键名是否存在
     * @param key 键名字符串
     * @return 键存在返回 true
     */
    NEFORCE_NODISCARD bool has_member(const string& key) const { return members.find(key) != members.end(); }

    /**
     * @brief 获取所有成员的常量引用
     * @return 键值对映射表的常量引用
     */
    NEFORCE_NODISCARD const unordered_map<string, shared_ptr<yaml_value>>& get_members() const noexcept {
        return members;
    }

    /**
     * @brief 获取集合样式
     * @return 映射样式枚举值
     */
    NEFORCE_NODISCARD mapping_style get_style() const noexcept { return style; }

    /**
     * @brief 设置集合样式
     * @param s 新的映射样式
     */
    void set_style(const mapping_style s) noexcept { style = s; }

    /**
     * @brief 合并另一个映射的成员
     * @param other 源映射指针
     *
     * 将 other 中的键值对合并到当前映射中。
     * 如果键名已存在，保留当前映射中的值（不覆盖）。
     */
    void merge_from(const yaml_mapping* other) {
        if (!other) {
            return;
        }
        for (const auto& pair: other->get_members()) {
            if (members.find(pair.first) == members.end()) {
                members[pair.first] = pair.second;
            }
        }
    }
};

/** @} */ // YamlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
