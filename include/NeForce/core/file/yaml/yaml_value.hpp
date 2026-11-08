#ifndef NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__

/**
 * @file yaml_value.hpp
 * @brief YAML配置格式值类型
 *
 * 此文件提供了 YAML 1.2 配置格式的扁平化值表示、视图以及文档容器。
 */

#include "NeForce/core/memory/bump_arena.hpp"
#include "NeForce/core/time/datetime.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct yaml_exception
 * @brief YAML格式操作失败
 */
struct yaml_exception final : value_exception {
    explicit yaml_exception(const char* info = "YAML Operation Failed.") noexcept :
    value_exception(info) {}

    explicit yaml_exception(const exception& e) :
    value_exception(e) {}

    ~yaml_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "yaml_exception"; }
};

/** @} */ // Exceptions

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup YamlConfig YAML配置
 * @{
 */

/**
 * @enum yaml_kind
 * @brief YAML值类型标签
 */
enum class yaml_kind : uint8_t {
    null_,     ///< 空值
    boolean,   ///< 布尔值
    integer,   ///< 64位有符号整数
    floating,  ///< IEEE 754 双精度浮点数
    string,    ///< 字符串标量
    timestamp, ///< 时间戳
    sequence,  ///< 序列
    mapping    ///< 映射
};

class yaml_value;
class yaml_null;
class yaml_boolean;
class yaml_integer;
class yaml_float;
class yaml_string;
class yaml_timestamp;
class yaml_sequence;
class yaml_mapping;
struct yaml_member;

/**
 * @struct yaml_meta
 * @brief 锚点与标签元数据
 */
struct yaml_meta {
    string_view anchor; ///< 锚点名
    string_view tag;    ///< 标签名
};

/**
 * @struct yaml_container
 * @brief 序列或映射的载荷
 */
struct yaml_container {
    uint32_t count = 0;    ///< 元素/成员数量
    uint32_t capacity = 0; ///< 已分配容量，2 的幂

    union payload {
        yaml_value* elements; ///< 序列元素数组
        yaml_member* members; ///< 映射成员数组
    } data{};

    uint32_t* index = nullptr;   ///< 开放寻址索引，惰性构建
    uint32_t index_capacity = 0; ///< 索引槽位数量，2 的幂
    uint32_t flags = 0;          ///< 容器状态标志，bit0 表示排序序已缓存
};

static_assert(sizeof(yaml_container) == 32, "yaml_container must stay 32 bytes");


/**
 * @class yaml_value
 * @brief 扁平化 YAML 值
 *
 * 24 字节固定大小，短字符串内联存储。
 */
class NEFORCE_API yaml_value {
public:
    /**
     * @struct inline_form
     * @brief 短字符串内联形式
     */
    struct inline_form {
        yaml_kind tag;   ///< 值类型
        uint8_t flags;   ///< 标志位
        uint16_t size;   ///< 字符串长度
        char chars[12];  ///< 内联字符
        yaml_meta* meta; ///< 锚点/标签
    };

    /**
     * @struct extended_form
     * @brief 非内联形式
     */
    struct extended_form {
        yaml_kind tag;     ///< 值类型
        uint8_t flags;     ///< 标志位
        uint16_t reserved; ///< 保留
        uint32_t count;    ///< 字符串长度

        union payload {
            bool boolean;              ///< 布尔值
            int64_t integer;           ///< 整数
            double floating;           ///< 浮点数
            uint64_t timestamp;        ///< 打包的时间戳
            const char* chars;         ///< 字符串指针
            yaml_value* elements;      ///< 序列元素
            yaml_member* members;      ///< 映射成员
            yaml_container* container; ///< 容器指针
        };
        payload data;    ///< 载荷
        yaml_meta* meta; ///< 锚点/标签
    };

    /**
     * @struct storage_form
     * @brief 存储联合体
     */
    union storage_form {
        inline_form inlined;    ///< 内联形式
        extended_form extended; ///< 扩展形式
    };

private:
    static constexpr uint8_t inline_flag = 0x80;  ///< 内联标志
    static constexpr uint8_t style_mask = 0x07;   ///< 标量样式掩码
    static constexpr uint8_t flow_flag = 0x08;    ///< 流式集合标志
    static constexpr uint8_t time_mask = 0x30;    ///< 时间戳类型掩码
    static constexpr uint8_t time_shift = 4;      ///< 时间戳类型位移
    static constexpr size_t inline_capacity = 12; ///< 内联容量

    storage_form form_; ///< 存储

    NEFORCE_NODISCARD extended_form& ext() noexcept { return form_.extended; }
    NEFORCE_NODISCARD const extended_form& ext() const noexcept { return form_.extended; }
    NEFORCE_NODISCARD inline_form& inl() noexcept { return form_.inlined; }
    NEFORCE_NODISCARD const inline_form& inl() const noexcept { return form_.inlined; }
    NEFORCE_NODISCARD bool is_inline_string() const noexcept { return (form_.extended.flags & inline_flag) != 0; }

    friend class yaml_document;
    friend class yaml_mapping;
    friend class yaml_sequence;
    friend class yaml_string;
    friend class yaml_timestamp;

public:
    /**
     * @brief 构造空值
     */
    yaml_value() noexcept {
        form_ = storage_form();
        form_.extended.tag = yaml_kind::null_;
    }

    /**
     * @brief 获取值类型
     * @return 类型标签
     */
    NEFORCE_NODISCARD yaml_kind kind() const noexcept { return form_.extended.tag; }

    /**
     * @brief 判断是否为空值
     * @return 是否为空值
     */
    NEFORCE_NODISCARD bool is_null() const noexcept { return kind() == yaml_kind::null_; }

    /**
     * @brief 判断是否为布尔值
     * @return 是否为布尔值
     */
    NEFORCE_NODISCARD bool is_boolean() const noexcept { return kind() == yaml_kind::boolean; }

    /**
     * @brief 判断是否为整数
     * @return 是否为整数
     */
    NEFORCE_NODISCARD bool is_integer() const noexcept { return kind() == yaml_kind::integer; }

    /**
     * @brief 判断是否为浮点数
     * @return 是否为浮点数
     */
    NEFORCE_NODISCARD bool is_float() const noexcept { return kind() == yaml_kind::floating; }

    /**
     * @brief 判断是否为字符串
     * @return 是否为字符串
     */
    NEFORCE_NODISCARD bool is_string() const noexcept { return kind() == yaml_kind::string; }

    /**
     * @brief 判断是否为时间戳
     * @return 是否为时间戳
     */
    NEFORCE_NODISCARD bool is_timestamp() const noexcept { return kind() == yaml_kind::timestamp; }

    /**
     * @brief 判断是否为序列
     * @return 是否为序列
     */
    NEFORCE_NODISCARD bool is_sequence() const noexcept { return kind() == yaml_kind::sequence; }

    /**
     * @brief 判断是否为映射
     * @return 是否为映射
     */
    NEFORCE_NODISCARD bool is_mapping() const noexcept { return kind() == yaml_kind::mapping; }

    /**
     * @brief 判断是否存在锚点或标签
     * @return 是否存在元数据
     */
    NEFORCE_NODISCARD bool has_meta() const noexcept {
        return (is_inline_string() ? form_.inlined.meta : form_.extended.meta) != nullptr;
    }

    /**
     * @brief 获取锚点名
     * @return 锚点视图，无锚点返回空视图
     */
    NEFORCE_NODISCARD string_view get_anchor() const noexcept;

    /**
     * @brief 获取标签名
     * @return 标签视图，无标签返回空视图
     */
    NEFORCE_NODISCARD string_view get_tag() const noexcept;

    /**
     * @brief 获取空值视图
     * @return 空值视图
     */
    NEFORCE_NODISCARD yaml_null as_null() const noexcept;

    /**
     * @brief 获取布尔值视图
     * @return 布尔值视图
     */
    NEFORCE_NODISCARD yaml_boolean as_boolean() const noexcept;

    /**
     * @brief 获取整数视图
     * @return 整数视图
     */
    NEFORCE_NODISCARD yaml_integer as_integer() const noexcept;

    /**
     * @brief 获取浮点数视图
     * @return 浮点数视图
     */
    NEFORCE_NODISCARD yaml_float as_float() const noexcept;

    /**
     * @brief 获取字符串视图
     * @return 字符串视图
     */
    NEFORCE_NODISCARD yaml_string as_string() const noexcept;

    /**
     * @brief 获取时间戳视图
     * @return 时间戳视图
     */
    NEFORCE_NODISCARD yaml_timestamp as_timestamp() const noexcept;

    /**
     * @brief 获取序列视图
     * @return 序列视图
     */
    NEFORCE_NODISCARD yaml_sequence as_sequence() const noexcept;

    /**
     * @brief 获取映射视图
     * @return 映射视图
     */
    NEFORCE_NODISCARD yaml_mapping as_mapping() const noexcept;

    /**
     * @brief 序列化为单行字符串
     * @return 单行 YAML 文本
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 序列化为多行文档
     * @return YAML 文档文本
     */
    NEFORCE_NODISCARD string to_document() const;
};

static_assert(sizeof(yaml_value) == 24, "yaml_value must stay 24 bytes");


/**
 * @struct yaml_member
 * @brief 映射成员
 */
struct yaml_member {
    const char* key_data; ///< 键名数据
    uint32_t key_size;    ///< 键名长度
    uint32_t key_hash;    ///< 键名哈希
    yaml_value value;     ///< 成员值

    /**
     * @brief 获取键名
     * @return 键名视图
     */
    NEFORCE_NODISCARD string_view key() const noexcept { return string_view(key_data, key_size); }
};

static_assert(sizeof(yaml_member) == 40, "yaml_member must stay 40 bytes");


/**
 * @class yaml_null
 * @brief 空值视图
 */
class yaml_null {
public:
    yaml_null() = default;

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_null* operator->() const noexcept { return this; }
};

/**
 * @class yaml_boolean
 * @brief 布尔值视图
 */
class yaml_boolean {
private:
    bool value_ = false;

public:
    yaml_boolean() = default;

    /**
     * @brief 构造布尔值视图
     * @param value 布尔值
     */
    explicit yaml_boolean(const bool value) noexcept :
    value_(value) {}

    /**
     * @brief 获取布尔值
     * @return 布尔值
     */
    NEFORCE_NODISCARD bool get_value() const noexcept { return value_; }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_boolean* operator->() const noexcept { return this; }
};

/**
 * @class yaml_integer
 * @brief 整数值视图
 */
class yaml_integer {
private:
    int64_t value_ = 0;

public:
    yaml_integer() = default;

    /**
     * @brief 构造整数值视图
     * @param value 整数值
     */
    explicit yaml_integer(const int64_t value) noexcept :
    value_(value) {}

    /**
     * @brief 获取整数值
     * @return 整数值
     */
    NEFORCE_NODISCARD int64_t get_value() const noexcept { return value_; }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_integer* operator->() const noexcept { return this; }
};

/**
 * @class yaml_float
 * @brief 浮点数值视图
 */
class yaml_float {
private:
    double value_ = 0;

public:
    yaml_float() = default;

    /**
     * @brief 构造浮点数值视图
     * @param value 浮点数值
     */
    explicit yaml_float(const double value) noexcept :
    value_(value) {}

    /**
     * @brief 获取浮点数值
     * @return 浮点数值
     */
    NEFORCE_NODISCARD double get_value() const noexcept { return value_; }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_float* operator->() const noexcept { return this; }
};

/**
 * @class yaml_string
 * @brief 字符串视图
 */
class yaml_string {
public:
    /**
     * @enum string_style
     * @brief 字符串标量样式
     */
    enum string_style : uint8_t {
        Plain,        ///< 纯文本
        SingleQuoted, ///< 单引号
        DoubleQuoted, ///< 双引号
        Literal,      ///< 字面量块（|）
        Folded        ///< 折叠块（>）
    };

private:
    const char* data_ = nullptr;
    uint32_t size_ = 0;
    string_style style_ = Plain;

public:
    yaml_string() = default;

    /**
     * @brief 构造字符串视图
     * @param data 字符数据
     * @param size 长度
     * @param style 标量样式
     */
    yaml_string(const char* data, const size_t size, const string_style style) noexcept :
    data_(data),
    size_(static_cast<uint32_t>(size)),
    style_(style) {}

    /**
     * @brief 获取字符串值
     * @return 字符串视图
     */
    NEFORCE_NODISCARD string_view get_value() const noexcept { return string_view(data_, size_); }

    /**
     * @brief 获取标量样式
     * @return 标量样式
     */
    NEFORCE_NODISCARD string_style get_style() const noexcept { return style_; }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 视图是否无效
     */
    NEFORCE_NODISCARD bool operator==(nullptr_t np) const noexcept {
        (void) np;
        return data_ == nullptr;
    }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 视图是否有效
     */
    NEFORCE_NODISCARD bool operator!=(nullptr_t np) const noexcept {
        (void) np;
        return data_ != nullptr;
    }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_string* operator->() const noexcept { return this; }
};

/**
 * @class yaml_timestamp
 * @brief 时间戳视图
 */
class yaml_timestamp {
public:
    /**
     * @enum timestamp_type
     * @brief 时间戳类型
     */
    enum timestamp_type : uint8_t {
        DateTime,   ///< 日期时间
        DateTimeTz, ///< 带时区的日期时间
        Date,       ///< 仅日期
        Time        ///< 仅时间
    };

private:
    uint64_t packed_ = 0;
    timestamp_type type_ = DateTime;

public:
    yaml_timestamp() = default;

    /**
     * @brief 构造时间戳视图
     * @param packed 打包的时间戳
     * @param type 时间戳类型
     */
    yaml_timestamp(const uint64_t packed, const timestamp_type type) noexcept :
    packed_(packed),
    type_(type) {}

    /**
     * @brief 获取日期时间值
     * @return 日期时间对象
     */
    NEFORCE_NODISCARD datetime get_value() const noexcept;

    /**
     * @brief 获取时间戳类型
     * @return 时间戳类型
     */
    NEFORCE_NODISCARD timestamp_type get_type() const noexcept { return type_; }

    /**
     * @brief 获取 RFC 3339 格式字符串
     * @return 格式化字符串
     */
    NEFORCE_NODISCARD string get_string_value() const { return get_value().to_RFC3339(); }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_timestamp* operator->() const noexcept { return this; }
};

/**
 * @class yaml_sequence
 * @brief 序列视图
 */
class yaml_sequence {
public:
    /**
     * @enum sequence_style
     * @brief 序列集合样式
     */
    enum sequence_style : uint8_t {
        Block, ///< 块样式
        Flow   ///< 流样式
    };

private:
    yaml_value* node_ = nullptr;

    friend class yaml_value;
    explicit yaml_sequence(yaml_value* node) noexcept :
    node_(node) {}

public:
    yaml_sequence() = default;

    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept;

    /**
     * @brief 判断序列是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0; }

    /**
     * @brief 获取指定索引的元素
     * @param index 元素索引
     * @return 元素指针，越界返回 nullptr
     */
    NEFORCE_NODISCARD const yaml_value* get_element(size_t index) const noexcept;

    /**
     * @brief 获取所有元素
     * @return 元素数组指针
     */
    NEFORCE_NODISCARD const yaml_value* data() const noexcept;

    /**
     * @brief 获取集合样式
     * @return 集合样式
     */
    NEFORCE_NODISCARD sequence_style get_style() const noexcept;

    /**
     * @brief 获取锚点名
     * @return 锚点视图
     */
    NEFORCE_NODISCARD string_view get_anchor() const noexcept {
        return node_ == nullptr ? string_view() : node_->get_anchor();
    }

    /**
     * @brief 获取标签名
     * @return 标签视图
     */
    NEFORCE_NODISCARD string_view get_tag() const noexcept {
        return node_ == nullptr ? string_view() : node_->get_tag();
    }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 视图是否无效
     */
    NEFORCE_NODISCARD bool operator==(nullptr_t np) const noexcept {
        (void) np;
        return node_ == nullptr;
    }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 视图是否有效
     */
    NEFORCE_NODISCARD bool operator!=(nullptr_t np) const noexcept {
        (void) np;
        return node_ != nullptr;
    }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_sequence* operator->() const noexcept { return this; }
};

/**
 * @class yaml_mapping
 * @brief 映射视图
 */
class yaml_mapping {
public:
    /**
     * @enum mapping_style
     * @brief 映射集合样式
     */
    enum mapping_style : uint8_t {
        Block, ///< 块样式
        Flow   ///< 流样式
    };

private:
    yaml_value* node_ = nullptr;

    friend class yaml_value;
    explicit yaml_mapping(yaml_value* node) noexcept :
    node_(node) {}

public:
    yaml_mapping() = default;

    /**
     * @brief 获取成员数量
     * @return 成员数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept;

    /**
     * @brief 判断映射是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0; }

    /**
     * @brief 获取指定键名的成员
     * @param key 键名
     * @return 成员值指针，键不存在返回 nullptr
     */
    NEFORCE_NODISCARD const yaml_value* get_member(string_view key) const noexcept;

    /**
     * @brief 获取指定键名的成员
     * @param key 键名
     * @return 成员值指针，键不存在返回 nullptr
     */
    NEFORCE_NODISCARD const yaml_value* get_member(const string& key) const noexcept {
        return get_member(string_view(key.data(), key.size()));
    }

    /**
     * @brief 获取指定键名的成员
     * @param key 键名
     * @return 成员值指针，键不存在返回 nullptr
     */
    NEFORCE_NODISCARD const yaml_value* get_member(const char* key) const noexcept {
        return get_member(string_view(key));
    }

    /**
     * @brief 检查键名是否存在
     * @param key 键名
     * @return 键是否存在
     */
    NEFORCE_NODISCARD bool has_member(string_view key) const noexcept { return get_member(key) != nullptr; }

    /**
     * @brief 获取成员数组起始指针
     * @return 成员数组指针
     */
    NEFORCE_NODISCARD const yaml_member* data() const noexcept;

    /**
     * @brief 获取成员数组结束指针
     * @return 结束指针
     */
    NEFORCE_NODISCARD const yaml_member* end_data() const noexcept;

    /**
     * @brief 获取成员数组的常量引用
     * @return 视图自身
     */
    NEFORCE_NODISCARD const yaml_mapping& get_members() const noexcept { return *this; }

    /**
     * @brief 获取按键名排序的成员索引
     * @return 排序索引数组，成员数不大于 1 时返回 nullptr
     */
    NEFORCE_NODISCARD const uint32_t* get_sorted_indices() const noexcept;

    /**
     * @brief 获取集合样式
     * @return 集合样式
     */
    NEFORCE_NODISCARD mapping_style get_style() const noexcept;

    /**
     * @brief 获取锚点名
     * @return 锚点视图
     */
    NEFORCE_NODISCARD string_view get_anchor() const noexcept {
        return node_ == nullptr ? string_view() : node_->get_anchor();
    }

    /**
     * @brief 获取标签名
     * @return 标签视图
     */
    NEFORCE_NODISCARD string_view get_tag() const noexcept {
        return node_ == nullptr ? string_view() : node_->get_tag();
    }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 视图是否无效
     */
    NEFORCE_NODISCARD bool operator==(nullptr_t np) const noexcept {
        (void) np;
        return node_ == nullptr;
    }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 视图是否有效
     */
    NEFORCE_NODISCARD bool operator!=(nullptr_t np) const noexcept {
        (void) np;
        return node_ != nullptr;
    }

    /**
     * @brief 访问视图自身
     * @return 视图指针
     */
    NEFORCE_NODISCARD const yaml_mapping* operator->() const noexcept { return this; }

    /**
     * @brief 成员迭代器起始
     * @return 起始指针
     */
    NEFORCE_NODISCARD const yaml_member* begin() const noexcept { return data(); }

    /**
     * @brief 成员迭代器结束
     * @return 结束指针
     */
    NEFORCE_NODISCARD const yaml_member* end() const noexcept { return end_data(); }
};

/**
 * @class yaml_document
 * @brief YAML 文档
 */
class NEFORCE_API yaml_document {
private:
    bump_arena arena_; ///< 分配器
    yaml_value root_;  ///< 根值

private:
    NEFORCE_NODISCARD yaml_value* new_sequence(bool flow);
    NEFORCE_NODISCARD yaml_value* new_mapping(bool flow);

    void mapping_append(yaml_value& node, string_view key, yaml_value value, bool copy_key);
    void sequence_append(yaml_value& node, yaml_value value);

    void set_meta(yaml_value& value, string_view anchor, string_view tag, bool copy);

    NEFORCE_NODISCARD yaml_value make_string_value(const string_view text, const uint8_t style, const bool copy) {
        yaml_value value;
        value.form_ = yaml_value::storage_form();
        if (text.size() <= yaml_value::inline_capacity) {
            yaml_value::inline_form& form = value.form_.inlined;
            form.tag = yaml_kind::string;
            form.flags = static_cast<uint8_t>(style | yaml_value::inline_flag);
            form.size = static_cast<uint16_t>(text.size());
            form.meta = nullptr;
            memory_copy(form.chars, text.data(), text.size());
            return value;
        }
        yaml_value::extended_form& form = value.form_.extended;
        form.tag = yaml_kind::string;
        form.flags = style;
        form.count = static_cast<uint32_t>(text.size());
        if (copy) {
            auto* data = static_cast<char*>(arena_.allocate(text.size(), 1, 1));
            memory_copy(data, text.data(), text.size());
            form.data.chars = data;
        } else {
            form.data.chars = text.data();
        }
        form.meta = nullptr;
        return value;
    }

    NEFORCE_NODISCARD yaml_value make_null_value() const noexcept { return yaml_value(); }

    NEFORCE_NODISCARD yaml_value make_boolean_value(const bool value) const noexcept {
        yaml_value result;
        result.form_ = yaml_value::storage_form();
        result.form_.extended.tag = yaml_kind::boolean;
        result.form_.extended.data.boolean = value;
        return result;
    }

    NEFORCE_NODISCARD yaml_value make_integer_value(const int64_t value) const noexcept {
        yaml_value result;
        result.form_ = yaml_value::storage_form();
        result.form_.extended.tag = yaml_kind::integer;
        result.form_.extended.data.integer = value;
        return result;
    }

    NEFORCE_NODISCARD yaml_value make_float_value(const double value) const noexcept {
        yaml_value result;
        result.form_ = yaml_value::storage_form();
        result.form_.extended.tag = yaml_kind::floating;
        result.form_.extended.data.floating = value;
        return result;
    }

    NEFORCE_NODISCARD yaml_value make_timestamp_value(const datetime& value) noexcept;
    NEFORCE_NODISCARD string_view store_string(string_view text, bool copy);
    void append_missing(yaml_value& target, const yaml_mapping& source);
    void set_size_hint(yaml_value& root, size_t size) noexcept;

    friend class yaml_parser;
    friend class yaml_builder;

public:
    /**
     * @brief 构造空文档
     */
    yaml_document() noexcept = default;

    yaml_document(const yaml_document&) = delete;
    yaml_document& operator=(const yaml_document&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源文档
     */
    yaml_document(yaml_document&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源文档
     * @return 自身引用
     */
    yaml_document& operator=(yaml_document&& other) noexcept = default;

    ~yaml_document() = default;

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 始终为 false
     */
    NEFORCE_NODISCARD bool operator==(nullptr_t np) const noexcept {
        (void) np;
        return false;
    }

    /**
     * @brief 与空指针比较
     * @param np 空指针
     * @return 始终为 true
     */
    NEFORCE_NODISCARD bool operator!=(nullptr_t np) const noexcept {
        (void) np;
        return true;
    }

    /**
     * @brief 获取根值
     * @return 根值引用
     */
    NEFORCE_NODISCARD yaml_value& root() noexcept { return root_; }

    /**
     * @brief 获取根值
     * @return 根值常量引用
     */
    NEFORCE_NODISCARD const yaml_value& root() const noexcept { return root_; }

    /**
     * @brief 访问根值
     * @return 根值指针
     */
    NEFORCE_NODISCARD yaml_value* operator->() noexcept { return &root_; }

    /**
     * @brief 访问根值
     * @return 根值常量指针
     */
    NEFORCE_NODISCARD const yaml_value* operator->() const noexcept { return &root_; }

    /**
     * @brief 访问根值
     * @return 根值引用
     */
    NEFORCE_NODISCARD yaml_value& operator*() noexcept { return root_; }

    /**
     * @brief 访问根值
     * @return 根值常量引用
     */
    NEFORCE_NODISCARD const yaml_value& operator*() const noexcept { return root_; }
};

/** @} */ // YamlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_VALUE_HPP__
