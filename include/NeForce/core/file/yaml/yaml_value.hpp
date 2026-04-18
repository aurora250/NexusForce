#ifndef NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__

/**
 * @file yaml_value.hpp
 * @brief YAML配置格式变量
 *
 * 此文件提供了YAML（YAML Ain't Markup Language）配置格式的抽象基类和具体实现类。
 * YAML是一种人类可读的数据序列化语言，常用于配置文件、数据交换和持久化存储。
 *
 * 支持YAML 1.2规范中的核心数据类型：
 * - 空值（Null）
 * - 布尔值（Boolean）
 * - 整数（Integer，64位有符号）
 * - 浮点数（Float，双精度）
 * - 字符串（String，支持五种标量样式）
 * - 时间戳（Timestamp，ISO 8601 / RFC 3339格式）
 * - 序列（Sequence，支持块样式和流样式）
 * - 映射（Mapping，支持块样式和流样式）
 * - 锚点与别名（Anchor & Alias，通过 anchor 字段支持）
 * - 标签（Tag，自定义类型标注）
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
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/core/time/datetime.hpp"
NEFORCE_BEGIN_NAMESPACE__

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


class NEFORCE_API yaml_value : public istringify<yaml_value> {
public:
    string anchor;
    string tag;

    enum types {
        Null,
        Boolean,
        Integer,
        Float,
        String,
        Timestamp,
        Sequence,
        Mapping
    };

    virtual ~yaml_value() = default;
    NEFORCE_NODISCARD virtual types type() const noexcept = 0;

    NEFORCE_NODISCARD virtual const yaml_null* as_null() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_boolean* as_boolean() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_integer* as_integer() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_float* as_float() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_string* as_string() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_timestamp* as_timestamp() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_sequence* as_sequence() const noexcept { return nullptr; }
    NEFORCE_NODISCARD virtual const yaml_mapping* as_mapping() const noexcept { return nullptr; }

    NEFORCE_NODISCARD bool is_null() const noexcept { return type() == Null; }
    NEFORCE_NODISCARD bool is_boolean() const noexcept { return type() == Boolean; }
    NEFORCE_NODISCARD bool is_integer() const noexcept { return type() == Integer; }
    NEFORCE_NODISCARD bool is_float() const noexcept { return type() == Float; }
    NEFORCE_NODISCARD bool is_string() const noexcept { return type() == String; }
    NEFORCE_NODISCARD bool is_timestamp() const noexcept { return type() == Timestamp; }
    NEFORCE_NODISCARD bool is_sequence() const noexcept { return type() == Sequence; }
    NEFORCE_NODISCARD bool is_mapping() const noexcept { return type() == Mapping; }

    void set_anchor(const string& a) { this->anchor = a; }
    void set_tag(const string& t) { this->tag = t; }

    NEFORCE_NODISCARD string to_string() const;
    NEFORCE_NODISCARD string to_document() const;
};

using yaml_ptr = shared_ptr<yaml_value>;


class NEFORCE_API yaml_null final : public yaml_value {
public:
    yaml_null() = default;
    NEFORCE_NODISCARD types type() const noexcept override { return Null; }
    NEFORCE_NODISCARD const yaml_null* as_null() const noexcept override { return this; }
};

class NEFORCE_API yaml_boolean final : public yaml_value {
private:
    bool value;

public:
    explicit yaml_boolean(const bool v) noexcept :
    value(v) {}
    NEFORCE_NODISCARD types type() const noexcept override { return Boolean; }
    NEFORCE_NODISCARD const yaml_boolean* as_boolean() const noexcept override { return this; }
    NEFORCE_NODISCARD bool get_value() const noexcept { return value; }
};

class NEFORCE_API yaml_integer final : public yaml_value {
private:
    int64_t value;

public:
    explicit yaml_integer(const int64_t v) noexcept :
    value(v) {}
    NEFORCE_NODISCARD types type() const noexcept override { return Integer; }
    NEFORCE_NODISCARD const yaml_integer* as_integer() const noexcept override { return this; }
    NEFORCE_NODISCARD int64_t get_value() const noexcept { return value; }
};

class NEFORCE_API yaml_float final : public yaml_value {
private:
    double value;

public:
    explicit yaml_float(const double v) noexcept :
    value(v) {}
    NEFORCE_NODISCARD types type() const noexcept override { return Float; }
    NEFORCE_NODISCARD const yaml_float* as_float() const noexcept override { return this; }
    NEFORCE_NODISCARD double get_value() const noexcept { return value; }
};

class NEFORCE_API yaml_string final : public yaml_value {
public:
    enum string_style {
        Plain,
        SingleQuoted,
        DoubleQuoted,
        Literal,
        Folded
    };

private:
    string value;
    string_style style;

public:
    explicit yaml_string(string v, const string_style s = Plain) noexcept :
    value(_NEFORCE move(v)),
    style(s) {}

    NEFORCE_NODISCARD types type() const noexcept override { return String; }
    NEFORCE_NODISCARD const yaml_string* as_string() const noexcept override { return this; }
    NEFORCE_NODISCARD const string& get_value() const noexcept { return value; }
    NEFORCE_NODISCARD string_style get_style() const noexcept { return style; }
};

class NEFORCE_API yaml_timestamp final : public yaml_value {
private:
    datetime value;

public:
    explicit yaml_timestamp(const string_view v) {
        datetime dt;
        if (dt.try_parse_RFC3339(v) || dt.try_parse_ISO8601(v)) {
            value = dt;
        } else {
            NEFORCE_THROW_EXCEPTION(yaml_exception(("Invalid timestamp format: " + string(v)).data()));
        }
    }

    explicit yaml_timestamp(const datetime& dt) noexcept :
    value(dt) {}

    NEFORCE_NODISCARD types type() const noexcept override { return Timestamp; }
    NEFORCE_NODISCARD const yaml_timestamp* as_timestamp() const noexcept override { return this; }
    NEFORCE_NODISCARD const datetime& get_value() const noexcept { return value; }

    NEFORCE_NODISCARD string get_string_value() const noexcept { return value.to_RFC3339(); }
};

class NEFORCE_API yaml_sequence final : public yaml_value {
public:
    enum sequence_style {
        Block,
        Flow
    };

private:
    vector<yaml_ptr> elements;
    sequence_style style;

public:
    explicit yaml_sequence(const sequence_style s = Block) :
    style(s) {}

    yaml_sequence(const yaml_sequence&) = delete;
    yaml_sequence& operator=(const yaml_sequence&) = delete;
    yaml_sequence(yaml_sequence&&) = default;
    yaml_sequence& operator=(yaml_sequence&&) = default;

    NEFORCE_NODISCARD types type() const noexcept override { return Sequence; }
    NEFORCE_NODISCARD const yaml_sequence* as_sequence() const noexcept override { return this; }

    void add_element(yaml_ptr value) { elements.emplace_back(_NEFORCE move(value)); }

    NEFORCE_NODISCARD const yaml_value* get_element(const size_t index) const noexcept {
        if (index < elements.size()) {
            return elements[index].get();
        }
        return nullptr;
    }

    NEFORCE_NODISCARD yaml_value* get_element(const size_t index) noexcept {
        if (index < elements.size()) {
            return elements[index].get();
        }
        return nullptr;
    }

    NEFORCE_NODISCARD size_t size() const noexcept { return elements.size(); }
    NEFORCE_NODISCARD const vector<yaml_ptr>& get_elements() const noexcept { return elements; }
    NEFORCE_NODISCARD sequence_style get_style() const noexcept { return style; }
    void set_style(const sequence_style s) noexcept { style = s; }
};

class NEFORCE_API yaml_mapping final : public yaml_value {
public:
    enum mapping_style {
        Block,
        Flow
    };

private:
    unordered_map<string, yaml_ptr> members;
    mapping_style style;

public:
    explicit yaml_mapping(const mapping_style s = Block) :
    style(s) {}

    yaml_mapping(const yaml_mapping&) = delete;
    yaml_mapping& operator=(const yaml_mapping&) = delete;
    yaml_mapping(yaml_mapping&&) = default;
    yaml_mapping& operator=(yaml_mapping&&) = default;

    NEFORCE_NODISCARD types type() const noexcept override { return Mapping; }
    NEFORCE_NODISCARD const yaml_mapping* as_mapping() const noexcept override { return this; }

    void add_member(const string& key, yaml_ptr value) { members[key] = _NEFORCE move(value); }

    NEFORCE_NODISCARD const yaml_value* get_member(const string& key) const {
        const auto it = members.find(key);
        if (it != members.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    NEFORCE_NODISCARD yaml_value* get_member(const string& key) {
        const auto it = members.find(key);
        if (it != members.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    NEFORCE_NODISCARD bool has_member(const string& key) const { return members.find(key) != members.end(); }

    NEFORCE_NODISCARD const unordered_map<string, yaml_ptr>& get_members() const noexcept { return members; }

    NEFORCE_NODISCARD mapping_style get_style() const noexcept { return style; }
    void set_style(const mapping_style s) noexcept { style = s; }

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

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
