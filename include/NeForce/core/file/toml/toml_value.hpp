#ifndef NEFORCE_CORE_FILE_TOML_TOML_VALUE_HPP__
#define NEFORCE_CORE_FILE_TOML_TOML_VALUE_HPP__

/**
 * @file toml_value.hpp
 * @brief TOML 值、视图与文档
 *
 * 单个值固定 16 字节，标签联合存放布尔、整数、浮点、字符串、日期时间。
 */

#include "NeForce/core/memory/bump_arena.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/time/datetime.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct toml_exception
 * @brief TOML格式操作失败
 */
struct toml_exception final : value_exception {
    explicit toml_exception(const char* info = "TOML Operation Failed.") noexcept :
    value_exception(info) {}

    explicit toml_exception(const exception& e) :
    value_exception(e) {}

    ~toml_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "toml_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup ConfigFormat 配置格式操作
 * @brief 配置格式管理
 * @{
 */

/**
 * @defgroup TomlConfig TOML配置
 * @brief TOML配置格式管理
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
 * | 类型              | TOML 规范引用  | 本实现视图          | 说明                               |
 * |------------------|---------------|-------------------|------------------------------------|
 * | Boolean          | §2.1          | toml_boolean      | true 或 false                       |
 * | Integer          | §2.2          | toml_integer      | 64位有符号整数（-2^63 到 2^63-1）   |
 * | Float            | §2.3          | toml_float        | IEEE 754 双精度浮点数               |
 * | String           | §2.4          | toml_string       | Unicode 字符串（UTF-8 编码）        |
 * | Offset Date-Time | §2.5.1        | toml_datetime     | RFC 3339 格式带时区偏移             |
 * | Local Date-Time  | §2.5.2        | toml_datetime     | ISO 8601 格式无时区                 |
 * | Local Date       | §2.5.3        | toml_datetime     | 仅日期（YYYY-MM-DD）                |
 * | Local Time       | §2.5.4        | toml_datetime     | 仅时间（HH:MM:SS）                  |
 * | Array            | §2.6          | toml_array        | 有序值列表（可混合类型）            |
 * | Table            | §2.7          | toml_table        | 键值对集合（标准表格或内联表格）    |
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
class toml_document;
class toml_builder;
class toml_boolean;
class toml_integer;
class toml_float;
class toml_string;
class toml_datetime;
class toml_array;
class toml_table;
class toml_table_mut;
class toml_array_mut;

struct toml_member;

/**
 * @struct toml_container
 * @brief 表格与数组共享的载荷
 *
 * 值本身固定 16 字节，容器内容不由值持有。
 */
struct NEFORCE_API toml_container {
    uint32_t count = 0;        ///< 元素/成员个数
    uint16_t reserved = 0;     ///< 预分配容量
    bool inline_table = false; ///< 表格是否内联

    /**
     * @brief 载荷
     */
    union payload {
        toml_value* elements; ///< 数组元素数组
        toml_member* members; ///< 表格成员数组
    };

    payload data{};              ///< 载荷
    uint32_t* index = nullptr;   ///< 大表格的开放寻址索引，小表格为空
    uint32_t index_capacity = 0; ///< 索引槽位个数，2 的幂
};

/**
 * @enum toml_kind
 * @brief TOML 值类型
 */
enum class toml_kind : uint8_t {
    null_,    ///< 未设置
    boolean,  ///< 布尔
    integer,  ///< 64 位有符号整数
    floating, ///< 双精度浮点数
    string,   ///< 字符串
    datetime, ///< 日期时间
    array,    ///< 数组
    table     ///< 表格
};

/**
 * @enum toml_string_type
 * @brief TOML 字符串引号类型
 */
enum toml_string_type : uint8_t {
    toml_basic_string,        ///< 基本字符串 "string"
    toml_literal_string,      ///< 字面量字符串 'string'
    toml_multi_basic_string,  ///< 多行基本字符串 """string"""
    toml_multi_literal_string ///< 多行字面量字符串 '''string'''
};

/**
 * @enum toml_datetime_type
 * @brief TOML 日期时间类型
 */
enum toml_datetime_type : uint8_t {
    toml_offset_datetime, ///< 带时区偏移的日期时间 1979-05-27T07:32:00Z
    toml_local_datetime,  ///< 本地日期时间 1979-05-27T07:32:00
    toml_local_date,      ///< 本地日期 1979-05-27
    toml_local_time       ///< 本地时间 07:32:00
};


#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

/**
 * @class toml_value
 * @brief TOML DOM 值，16 字节、平凡可析构、无虚函数
 *
 * 字符串内容与容器载荷存放在拥有它们的 toml_document 中。
 */
class NEFORCE_API toml_value : public istringify<toml_value> {
public:
    /**
     * @brief 构造未设置的值
     */
    toml_value() noexcept = default;

    /**
     * @brief 构造布尔值
     * @param state 布尔值
     * @return 布尔值
     */
    NEFORCE_NODISCARD static toml_value make_boolean(const bool state) noexcept {
        toml_value result;
        result.storage_.ext_.tag_ = toml_kind::boolean;
        result.storage_.ext_.payload_.boolean_ = state;
        return result;
    }

    /**
     * @brief 构造整数
     * @param number 整数值
     * @return 整数值
     */
    NEFORCE_NODISCARD static toml_value make_integer(const int64_t number) noexcept {
        toml_value result;
        result.storage_.ext_.tag_ = toml_kind::integer;
        result.storage_.ext_.payload_.integer_ = number;
        return result;
    }

    /**
     * @brief 构造浮点数
     * @param number 浮点值
     * @return 浮点值
     */
    NEFORCE_NODISCARD static toml_value make_float(const double number) noexcept {
        toml_value result;
        result.storage_.ext_.tag_ = toml_kind::floating;
        result.storage_.ext_.payload_.real_ = number;
        return result;
    }

    /**
     * @brief 构造字符串值
     * @param text 字符串视图
     * @param type 引号类型
     * @return 字符串值
     * @note 调用方需保证其生命周期不短于使用期
     */
    NEFORCE_NODISCARD static toml_value make_string(const string_view text,
                                                    const toml_string_type type = toml_basic_string) noexcept {
        if (text.size() <= inline_string_capacity) {
            return make_inline_string_unchecked(text, type);
        }
        toml_value result;
        result.storage_.ext_.tag_ = toml_kind::string;
        result.storage_.ext_.flags_ = static_cast<uint8_t>(static_cast<uint8_t>(type) << string_type_shift);
        result.storage_.ext_.count_ = static_cast<uint32_t>(text.size());
        result.storage_.ext_.payload_.chars_ = text.data();
        return result;
    }

    /**
     * @brief 构造日期时间值
     * @param value 日期时间
     * @param type 日期时间类型
     * @return 日期时间值
     */
    NEFORCE_NODISCARD static toml_value make_datetime(const datetime& value, const toml_datetime_type type) noexcept {
        toml_value result;
        result.storage_.ext_.tag_ = toml_kind::datetime;
        result.storage_.ext_.flags_ = static_cast<uint8_t>(type);
        result.storage_.ext_.payload_.datetime_ = pack_datetime(value);
        return result;
    }

    /**
     * @brief 获取值类型
     * @return 类型标签
     */
    NEFORCE_NODISCARD toml_kind kind() const noexcept { return storage_.ext_.tag_; }

    /**
     * @brief 判断是否为布尔值
     * @return 是否为布尔值
     */
    NEFORCE_NODISCARD bool is_boolean() const noexcept { return storage_.ext_.tag_ == toml_kind::boolean; }

    /**
     * @brief 判断是否为整数
     * @return 是否为整数
     */
    NEFORCE_NODISCARD bool is_integer() const noexcept { return storage_.ext_.tag_ == toml_kind::integer; }

    /**
     * @brief 判断是否为浮点数
     * @return 是否为浮点数
     */
    NEFORCE_NODISCARD bool is_float() const noexcept { return storage_.ext_.tag_ == toml_kind::floating; }

    /**
     * @brief 判断是否为字符串
     * @return 是否为字符串
     */
    NEFORCE_NODISCARD bool is_string() const noexcept { return storage_.ext_.tag_ == toml_kind::string; }

    /**
     * @brief 判断是否为日期时间
     * @return 是否为日期时间
     */
    NEFORCE_NODISCARD bool is_datetime() const noexcept { return storage_.ext_.tag_ == toml_kind::datetime; }

    /**
     * @brief 判断是否为数组
     * @return 是否为数组
     */
    NEFORCE_NODISCARD bool is_array() const noexcept { return storage_.ext_.tag_ == toml_kind::array; }

    /**
     * @brief 判断是否为表格
     * @return 是否为表格
     */
    NEFORCE_NODISCARD bool is_table() const noexcept { return storage_.ext_.tag_ == toml_kind::table; }

    /**
     * @brief 获取字符串长度或数组/表格的元素个数
     * @return 个数
     */
    NEFORCE_NODISCARD uint32_t element_count() const noexcept {
        if (is_inline_string()) {
            return storage_.ext_.flags_ & mask_inline_length;
        }
        if (storage_.ext_.tag_ == toml_kind::array || storage_.ext_.tag_ == toml_kind::table) {
            return storage_.ext_.payload_.container_->count;
        }
        return storage_.ext_.count_;
    }

    /**
     * @brief 以布尔视图读取
     * @return 布尔视图
     */
    NEFORCE_NODISCARD toml_boolean as_boolean() const noexcept;

    /**
     * @brief 以整数视图读取
     * @return 整数视图
     */
    NEFORCE_NODISCARD toml_integer as_integer() const noexcept;

    /**
     * @brief 以浮点视图读取
     * @return 浮点视图
     */
    NEFORCE_NODISCARD toml_float as_float() const noexcept;

    /**
     * @brief 以字符串视图读取
     * @return 字符串视图
     */
    NEFORCE_NODISCARD toml_string as_string() const noexcept;

    /**
     * @brief 以日期时间视图读取
     * @return 日期时间视图
     */
    NEFORCE_NODISCARD toml_datetime as_datetime() const noexcept;

    /**
     * @brief 以数组视图读取
     * @return 数组视图
     */
    NEFORCE_NODISCARD toml_array as_array() const noexcept;

    /**
     * @brief 以表格视图读取
     * @return 表格视图
     */
    NEFORCE_NODISCARD toml_table as_table() const noexcept;

    /**
     * @brief 转换为紧凑格式字符串
     * @return TOML 值的紧凑表示
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档格式字符串
     * @return TOML 值的完整文档格式字符串表示
     */
    NEFORCE_NODISCARD string to_document() const;

private:
    friend class toml_document;
    friend class toml_builder;
    friend class toml_parser;
    friend class toml_table_mut;
    friend class toml_array_mut;
    friend class toml_string;
    friend class toml_datetime;

    static constexpr uint32_t inline_string_capacity = 14;
    static constexpr uint8_t flag_inline_string = 0x80;
    static constexpr uint8_t mask_inline_length = 0x0F;
    static constexpr uint8_t mask_string_type = 0x30;
    static constexpr uint8_t string_type_shift = 4;
    static constexpr uint8_t mask_datetime_type = 0x03;

    static constexpr int datetime_year_bits = 14;
    static constexpr int datetime_month_shift = 14;
    static constexpr int datetime_day_shift = 18;
    static constexpr int datetime_hour_shift = 23;
    static constexpr int datetime_minute_shift = 28;
    static constexpr int datetime_second_shift = 34;
    static constexpr int datetime_offset_shift = 40;
    static constexpr int64_t datetime_offset_bias = 131071;
    static constexpr uint64_t datetime_offset_mask = 0x3FFFF;

    NEFORCE_NODISCARD static toml_value make_inline_string_unchecked(string_view text, toml_string_type type) noexcept;

    NEFORCE_NODISCARD static const toml_container* empty_container() noexcept {
        static constexpr toml_container value{};
        return &value;
    }

    NEFORCE_NODISCARD static uint64_t pack_datetime(const datetime& value) noexcept {
        const auto year = static_cast<uint64_t>(value.year()) & ((1ULL << datetime_year_bits) - 1);
        const auto month = static_cast<uint64_t>(value.month()) & 0x0FULL;
        const auto day = static_cast<uint64_t>(value.day()) & 0x1FULL;
        const auto hour = static_cast<uint64_t>(value.hours()) & 0x1FULL;
        const auto minute = static_cast<uint64_t>(value.minutes()) & 0x3FULL;
        const auto second = static_cast<uint64_t>(value.seconds()) & 0x3FULL;
        const auto offset = static_cast<uint64_t>(value.offset_seconds() + datetime_offset_bias) & datetime_offset_mask;
        return year | (month << datetime_month_shift) | (day << datetime_day_shift) | (hour << datetime_hour_shift) |
               (minute << datetime_minute_shift) | (second << datetime_second_shift) |
               (offset << datetime_offset_shift);
    }

    NEFORCE_NODISCARD bool is_inline_string() const noexcept {
        return storage_.ext_.tag_ == toml_kind::string && (storage_.ext_.flags_ & flag_inline_string) != 0;
    }

    NEFORCE_NODISCARD toml_string_type inline_string_type() const noexcept {
        return static_cast<toml_string_type>((storage_.ext_.flags_ & mask_string_type) >> string_type_shift);
    }

    /**
     * @struct storage_form
     * @brief 值的 16 字节存储
     */
    union storage_form {
        /**
         * @brief 统一形态
         */
        struct extended_form {
            toml_kind tag_ = toml_kind::null_; ///< 类型标签
            uint8_t flags_;                    ///< 字符串类型、日期时间类型与表格标记
            uint16_t reserved_;                ///< 容器预分配容量
            uint32_t count_;                   ///< 字符串长度或元素/成员个数

            /**
             * @brief 载荷
             */
            union payload {
                bool boolean_;              ///< 布尔
                int64_t integer_;           ///< 整数
                double real_;               ///< 浮点
                uint64_t datetime_;         ///< 打包后的日期时间
                const char* chars_;         ///< 字符串起始地址
                toml_container* container_; ///< 表格/数组载荷
            } payload_;                     ///< 载荷

        } ext_; ///< 统一形态

        /**
         * @brief 内联字符串形态
         */
        struct inline_form {
            toml_kind tag_ = toml_kind::null_; ///< 类型标签
            uint8_t flags_;                    ///< 内部标记、引号类型与长度
            char chars_[14];                   ///< 内联字符

        } inl_; ///< 内联形态

    } storage_{}; ///< 值存储

    static_assert(sizeof(storage_form) == 16, "storage_form must stay 16 bytes");
};

#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic pop
#endif


/**
 * @struct toml_member
 * @brief TOML 表格的一个成员
 */
struct NEFORCE_API toml_member {
    /// 内联键最大字节数
    static constexpr uint32_t inline_key_capacity = 8;

    /// 键内联标记位
    static constexpr uint32_t flag_inline_key = 0x80000000U;

    /// 键前缀覆盖的字节数
    static constexpr uint32_t key_prefix_size = 4;

    const char* key_data = nullptr; ///< 键起始地址，或不超过 8 字节的内联键本体
    uint32_t key_size = 0;          ///< 键长度，最高位为内联标记
    uint32_t key_prefix = 0;        ///< 键前 4 字节，不足 4 字节时高位补 0
    toml_value value;               ///< 成员值

    /**
     * @brief 判断键是否内联存放在成员结构中
     * @return 是否内联
     */
    NEFORCE_NODISCARD bool key_inlined() const noexcept { return (key_size & flag_inline_key) != 0; }

    /**
     * @brief 获取键长度
     * @return 键长度
     */
    NEFORCE_NODISCARD uint32_t key_length() const noexcept { return key_size & ~flag_inline_key; }

    /**
     * @brief 获取键视图
     * @return 键的字符串视图
     */
    NEFORCE_NODISCARD string_view key() const noexcept {
        if (key_inlined()) {
            return string_view(reinterpret_cast<const char*>(&key_data), key_length());
        }
        return string_view(key_data, key_length());
    }

    /**
     * @brief 计算键前缀，前 4 字节之外的内容不参与，不足 4 字节时高位补 0
     * @param text 键
     * @return 键前缀
     */
    NEFORCE_NODISCARD static uint32_t prefix_of(const string_view text) noexcept {
        uint32_t prefix = 0;
        const size_t limit = text.size() < key_prefix_size ? text.size() : key_prefix_size;
        for (size_t i = 0; i < limit; ++i) {
            prefix |= static_cast<uint32_t>(static_cast<unsigned char>(text[i])) << (i * 8);
        }
        return prefix;
    }

    /**
     * @brief 判断键是否与给定文本相同
     * @param prefix 给定文本的前缀
     * @param text 给定文本
     * @return 是否相同
     */
    NEFORCE_NODISCARD bool key_equal(const uint32_t prefix, const string_view text) const noexcept {
        const uint32_t length = key_length();
        if (length != text.size() || key_prefix != prefix) {
            return false;
        }
        if (length <= key_prefix_size) {
            return true;
        }
        const char* mine = key_inlined() ? reinterpret_cast<const char*>(&key_data) : key_data;
        const size_t tail = length - key_prefix_size;
        for (size_t i = 0; i < tail; ++i) {
            if (mine[key_prefix_size + i] != text[key_prefix_size + i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 写入键，不超过 8 字节时内联存放
     * @param text 键视图，长度超过内联容量时调用方需保证其生命周期
     */
    void assign_key(string_view text) noexcept;
};

/**
 * @class toml_boolean
 * @brief 布尔视图
 */
class NEFORCE_API toml_boolean {
public:
    /**
     * @brief 获取布尔值
     * @return 布尔值
     */
    NEFORCE_NODISCARD bool get_value() const noexcept { return value_; }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_boolean* operator->() const noexcept { return this; }

private:
    friend class toml_value;

    explicit toml_boolean(const bool value) noexcept :
    value_(value) {}

    bool value_ = false;
};

/**
 * @class toml_integer
 * @brief 整数值视图
 */
class NEFORCE_API toml_integer {
public:
    /**
     * @brief 获取整数值
     * @return 64 位整数值
     */
    NEFORCE_NODISCARD int64_t get_value() const noexcept { return value_; }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_integer* operator->() const noexcept { return this; }

private:
    friend class toml_value;

    explicit toml_integer(const int64_t value) noexcept :
    value_(value) {}

    int64_t value_ = 0;
};

/**
 * @class toml_float
 * @brief 浮点数值视图
 */
class NEFORCE_API toml_float {
public:
    /**
     * @brief 获取浮点值
     * @return 双精度浮点值
     */
    NEFORCE_NODISCARD double get_value() const noexcept { return value_; }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_float* operator->() const noexcept { return this; }

private:
    friend class toml_value;

    explicit toml_float(const double value) noexcept :
    value_(value) {}

    double value_ = 0.0;
};

/**
 * @class toml_string
 * @brief 字符串视图
 */
class NEFORCE_API toml_string {
public:
    /**
     * @enum string_type
     * @brief 字符串引号类型枚举
     */
    enum string_type : uint8_t {
        Basic = toml_basic_string,               ///< 基本字符串 "string"
        Literal = toml_literal_string,           ///< 字面量字符串 'string'
        MultiBasic = toml_multi_basic_string,    ///< 多行基本字符串 """string"""
        MultiLiteral = toml_multi_literal_string ///< 多行字面量字符串 '''string'''
    };

    /**
     * @brief 获取字符串内容
     * @return 字符串视图
     */
    NEFORCE_NODISCARD string_view get_value() const noexcept { return string_view(data_, size_); }

    /**
     * @brief 获取数据地址
     * @return 数据地址
     */
    NEFORCE_NODISCARD const char* data() const noexcept { return data_; }

    /**
     * @brief 获取长度
     * @return 长度
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 获取字符串引号类型
     * @return 引号类型枚举值
     */
    NEFORCE_NODISCARD string_type get_string_type() const noexcept { return type_; }

    /**
     * @brief 转换为字符串视图
     * @return 字符串视图
     */
    operator string_view() const noexcept { return get_value(); }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_string* operator->() const noexcept { return this; }

private:
    friend class toml_value;

    toml_string(const char* data, const size_t size, const string_type type) noexcept :
    data_(data),
    size_(size),
    type_(type) {}

    const char* data_ = nullptr;
    size_t size_ = 0;
    string_type type_ = Basic;
};

/**
 * @class toml_datetime
 * @brief 日期时间视图
 */
class NEFORCE_API toml_datetime {
public:
    /**
     * @enum datetime_type
     * @brief 日期时间类型枚举
     */
    enum datetime_type : uint8_t {
        OffsetDateTime = toml_offset_datetime, ///< 偏移日期时间 1979-05-27T07:32:00Z
        LocalDateTime = toml_local_datetime,   ///< 本地日期时间 1979-05-27T07:32:00
        LocalDate = toml_local_date,           ///< 本地日期 1979-05-27
        LocalTime = toml_local_time            ///< 本地时间 07:32:00
    };

    /**
     * @brief 获取日期时间值
     * @return 日期时间对象
     */
    NEFORCE_NODISCARD datetime get_value() const noexcept;

    /**
     * @brief 获取字符串格式的日期时间值
     * @return 根据类型格式化的日期时间字符串
     */
    NEFORCE_NODISCARD string get_string_value() const;

    /**
     * @brief 获取日期时间类型
     * @return 日期时间类型枚举值
     */
    NEFORCE_NODISCARD datetime_type get_datetime_type() const noexcept { return type_; }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_datetime* operator->() const noexcept { return this; }

private:
    friend class toml_value;

    toml_datetime(const uint64_t raw, const datetime_type type) noexcept :
    raw_(raw),
    type_(type) {}

    uint64_t raw_ = 0;
    datetime_type type_ = LocalDateTime;
};

/**
 * @class toml_array
 * @brief 数组只读视图
 */
class NEFORCE_API toml_array {
public:
    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_array* operator->() const noexcept { return this; }

    /**
     * @brief 获取元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 判断数组是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief 按下标获取元素
     * @param index 下标
     * @return 元素指针，越界返回 nullptr
     */
    NEFORCE_NODISCARD const toml_value* get_element(const size_t index) const noexcept {
        return index < size_ ? data_ + index : nullptr;
    }

    /**
     * @brief 获取元素区间
     * @return 自身引用
     */
    NEFORCE_NODISCARD toml_array get_elements() const noexcept { return *this; }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const toml_value* begin() const noexcept { return data_; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const toml_value* end() const noexcept { return data_ + size_; }

private:
    friend class toml_value;

    toml_array(const toml_value* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const toml_value* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @class toml_table
 * @brief 表格只读视图
 */
class NEFORCE_API toml_table {
public:
    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const toml_table* operator->() const noexcept { return this; }

    /**
     * @brief 获取成员个数
     * @return 成员个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return container_->count; }

    /**
     * @brief 判断表格是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return container_->count == 0; }

    /**
     * @brief 按 key 查找成员
     * @param key 键
     * @return 成员值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const toml_value* get_member(string_view key) const noexcept;

    /**
     * @brief 按 key 查找成员
     * @param key 键
     * @return 成员值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const toml_value* get_member(const string& key) const noexcept {
        return this->get_member(string_view(key.data(), key.size()));
    }

    /**
     * @brief 按 key 查找成员
     * @param key 键
     * @return 成员值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const toml_value* get_member(const char* key) const noexcept {
        return this->get_member(string_view(key));
    }

    /**
     * @brief 检查成员是否存在
     * @param key 成员键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_member(const string_view key) const noexcept { return get_member(key) != nullptr; }

    /**
     * @brief 检查成员是否存在
     * @param key 成员键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_member(const string& key) const noexcept {
        return has_member(string_view(key.data(), key.size()));
    }

    /**
     * @brief 检查成员是否存在
     * @param key 成员键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_member(const char* key) const noexcept { return has_member(string_view(key)); }

    /**
     * @brief 获取成员区间
     * @return 自身引用
     */
    NEFORCE_NODISCARD toml_table get_members() const noexcept { return *this; }

    /**
     * @brief 判断是否为内联表格
     * @return 是否为内联表格
     */
    NEFORCE_NODISCARD bool is_inline() const noexcept { return container_->inline_table; }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const toml_member* begin() const noexcept { return container_->data.members; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const toml_member* end() const noexcept { return container_->data.members + container_->count; }

private:
    friend class toml_value;

    explicit toml_table(const toml_container* container) noexcept :
    container_(container) {}

    const toml_container* container_ = nullptr;
};

/**
 * @class toml_document
 * @brief 拥有 DOM 内存的 TOML 文档，可移动不可拷贝
 */
class NEFORCE_API toml_document {
public:
    toml_document() noexcept = default;
    ~toml_document() = default;

    toml_document(toml_document&&) noexcept = default;
    toml_document& operator=(toml_document&&) noexcept = default;

    toml_document(const toml_document&) = delete;
    toml_document& operator=(const toml_document&) = delete;

    /**
     * @brief 根值访问
     * @return 根值引用
     */
    NEFORCE_NODISCARD toml_value& root() noexcept { return root_; }

    /**
     * @brief 根值访问
     * @return 根值引用
     */
    NEFORCE_NODISCARD const toml_value& root() const noexcept { return root_; }

    /**
     * @brief 兼容旧写法的成员访问运算符
     * @return 根值指针
     */
    const toml_value* operator->() const noexcept { return &root_; }

    /**
     * @brief 兼容旧写法的成员访问运算符
     * @return 根值指针
     */
    toml_value* operator->() noexcept { return &root_; }

    /**
     * @brief 是否为空文档
     * @return 根值是否为空表格
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        return root_.kind() != toml_kind::table || root_.element_count() == 0;
    }

    /**
     * @brief 获取根值指针
     * @return 根值指针
     */
    NEFORCE_NODISCARD const toml_value* get() const noexcept { return &root_; }

    /**
     * @brief 获取根值指针
     * @return 根值指针
     */
    NEFORCE_NODISCARD toml_value* get() noexcept { return &root_; }

    /**
     * @brief 转换到布尔值
     * @return 根值是否为表格
     */
    explicit operator bool() const noexcept { return root_.is_table(); }

    /**
     * @brief 把根值构造为空表格
     * @param is_inline 是否为内联表格
     * @return 表格可变视图
     * @throws memory_exception 分配失败时抛出
     */
    toml_table_mut make_table(bool is_inline = false);

    /**
     * @brief 把根值构造为空数组
     * @return 数组可变视图
     * @throws memory_exception 分配失败时抛出
     */
    toml_array_mut make_array();

    /**
     * @brief 新建一个空表格值
     * @param is_inline 是否为内联表格
     * @return 表格可变视图
     * @throws memory_exception 分配失败时抛出
     */
    toml_table_mut new_table(bool is_inline = false);

    /**
     * @brief 新建一个空数组值
     * @return 数组可变视图
     * @throws memory_exception 分配失败时抛出
     */
    toml_array_mut new_array();

    /**
     * @brief 在 arena 中新建空容器节点
     * @param is_table true 新建表格，false 新建数组
     * @param is_inline 新建表格时是否为内联表格
     * @param reserve 预分配的元素/成员个数，0 表示按默认规则分配
     * @return 节点指针
     * @throws memory_exception 分配失败时抛出
     */
    toml_value* new_container(bool is_table, bool is_inline = false, size_t reserve = 0);

    /**
     * @brief 向表格节点追加成员
     * @param node 表格节点
     * @param key 键
     * @param value 成员值
     * @param copy_key 键是否需要复制进 arena
     * @throws memory_exception 分配失败时抛出
     */
    void table_append(toml_value& node, string_view key, toml_value value, bool copy_key = true);

    /**
     * @brief 向数组节点追加元素
     * @param node 数组节点
     * @param value 元素值
     * @throws memory_exception 分配失败时抛出
     */
    void array_append(toml_value& node, toml_value value);

    /**
     * @brief 一次性预分配表格的成员数组
     * @param node 表格节点
     * @param count 预期成员个数
     * @throws memory_exception 分配失败时抛出
     */
    void table_reserve(toml_value& node, size_t count);

    /**
     * @brief 一次性预分配数组的元素数组
     * @param node 数组节点
     * @param count 预期元素个数
     * @throws memory_exception 分配失败时抛出
     */
    void array_reserve(toml_value& node, size_t count);

    /**
     * @brief 构造字符串值，长内容复制进文档 arena
     * @param text 字符串视图
     * @param type 引号类型
     * @return 字符串值
     * @throws memory_exception 分配失败时抛出
     */
    NEFORCE_NODISCARD toml_value make_string_value(string_view text, toml_string_type type = toml_basic_string);

    /**
     * @brief 从文本解析并构造日期时间值
     * @param text 日期时间文本
     * @param type 日期时间类型
     * @return 日期时间值
     */
    NEFORCE_NODISCARD toml_value make_datetime_value(string_view text, toml_datetime_type type) noexcept;

    /**
     * @brief 在文档 arena 中分配一段字符存储并拷贝
     * @param text 源文本
     * @return 字符串视图
     * @throws memory_exception 分配失败时抛出
     */
    NEFORCE_NODISCARD string_view store_string(string_view text);

    /**
     * @brief 预留文档内存块
     * @param bytes 期望的可用字节数
     * @throws memory_exception 分配失败时抛出
     */
    void reserve(const size_t bytes) { arena_.reserve(bytes); }

private:
    friend class toml_parser;
    friend class toml_builder;
    friend class toml_table_mut;
    friend class toml_array_mut;

    bump_arena arena_; ///< 文档内存池
    toml_value root_;  ///< 根值
};

/**
 * @class toml_table_mut
 * @brief 表格可变视图，用于构建 DOM
 */
class NEFORCE_API toml_table_mut {
public:
    /**
     * @brief 追加一个成员
     * @param key 键
     * @param value 成员值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    toml_table_mut& add_member(string_view key, toml_value value);

    /**
     * @brief 追加一个成员
     * @param key 键
     * @param value 成员值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    toml_table_mut& add_member(const string& key, const toml_value value) {
        return this->add_member(string_view(key.data(), key.size()), value);
    }

    /**
     * @brief 追加一个成员
     * @param key 键
     * @param value 成员值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    toml_table_mut& add_member(const char* key, const toml_value value) {
        return this->add_member(string_view(key), value);
    }

    /**
     * @brief 取底层值
     * @return 值拷贝
     */
    NEFORCE_NODISCARD toml_value to_value() const noexcept { return *node_; }

    /**
     * @brief 以只读视图访问
     * @return 表格视图
     */
    NEFORCE_NODISCARD toml_table as_table() const noexcept { return node_->as_table(); }

    /**
     * @brief 成员个数
     * @return 成员个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return node_ == nullptr ? 0 : node_->element_count(); }

private:
    friend class toml_document;
    friend class toml_value;

    toml_table_mut(toml_document* doc, toml_value* node) noexcept :
    doc_(doc),
    node_(node) {}

    toml_document* doc_ = nullptr;
    toml_value* node_ = nullptr;
};

/**
 * @class toml_array_mut
 * @brief 数组可变视图，用于构建 DOM
 */
class NEFORCE_API toml_array_mut {
public:
    /**
     * @brief 追加一个元素
     * @param value 元素值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    toml_array_mut& add_element(toml_value value);

    /**
     * @brief 取底层值
     * @return 值拷贝
     */
    NEFORCE_NODISCARD toml_value to_value() const noexcept { return *node_; }

    /**
     * @brief 以只读视图访问
     * @return 数组视图
     */
    NEFORCE_NODISCARD toml_array as_array() const noexcept { return node_->as_array(); }

    /**
     * @brief 元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return node_ == nullptr ? 0 : node_->element_count(); }

private:
    friend class toml_document;
    friend class toml_value;

    toml_array_mut(toml_document* doc, toml_value* node) noexcept :
    doc_(doc),
    node_(node) {}

    toml_document* doc_ = nullptr;
    toml_value* node_ = nullptr;
};


inline toml_boolean toml_value::as_boolean() const noexcept { return toml_boolean(storage_.ext_.payload_.boolean_); }

inline toml_integer toml_value::as_integer() const noexcept { return toml_integer(storage_.ext_.payload_.integer_); }

inline toml_float toml_value::as_float() const noexcept { return toml_float(storage_.ext_.payload_.real_); }

inline toml_string toml_value::as_string() const noexcept {
    if (is_inline_string()) {
        return toml_string(storage_.inl_.chars_, storage_.ext_.flags_ & mask_inline_length,
                           static_cast<toml_string::string_type>(inline_string_type()));
    }
    return toml_string(
            storage_.ext_.payload_.chars_, storage_.ext_.count_,
            static_cast<toml_string::string_type>((storage_.ext_.flags_ & mask_string_type) >> string_type_shift));
}

inline toml_datetime toml_value::as_datetime() const noexcept {
    return toml_datetime(storage_.ext_.payload_.datetime_,
                         static_cast<toml_datetime::datetime_type>(storage_.ext_.flags_ & mask_datetime_type));
}

inline toml_array toml_value::as_array() const noexcept {
    if (!is_array()) {
        return toml_array(nullptr, 0);
    }
    const toml_container* container = storage_.ext_.payload_.container_;
    return toml_array(container->data.elements, container->count);
}

inline toml_table toml_value::as_table() const noexcept {
    return toml_table(is_table() ? storage_.ext_.payload_.container_ : empty_container());
}

/** @} */ // TomlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TOML_TOML_VALUE_HPP__
