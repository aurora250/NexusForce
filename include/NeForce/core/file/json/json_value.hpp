#ifndef NEFORCE_CORE_FILE_JSON_JSON_DOM_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_DOM_HPP__

/**
 * @file json_value.hpp
 * @brief JSON DOM 值、视图与文档
 */

#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/file/json/json_arena.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct json_exception
 * @brief JSON 操作异常
 */
struct json_exception final : value_exception {
    explicit json_exception(const char* info = "JSON Operation Failed.") noexcept :
    value_exception(info) {}

    explicit json_exception(const exception& e) :
    value_exception(e) {}

    ~json_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "json_exception"; }
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
class json_document;
class json_builder;
class json_bool;
class json_number;
class json_string;
class json_array;
class json_object;
class json_object_mut;
class json_array_mut;

struct json_member;

/**
 * @enum json_kind
 * @brief JSON 值类型
 */
enum class json_kind : uint8_t {
    null_,    ///< 空值
    boolean,  ///< 布尔
    integer,  ///< 有符号整数
    uinteger, ///< 无符号整数
    real,     ///< 浮点数
    string,   ///< 字符串
    array,    ///< 数组
    object    ///< 对象
};


#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

/**
 * @class json_value
 * @brief JSON DOM 值，16 字节、平凡可析构、无虚函数
 */
class NEFORCE_API json_value {
public:
    /**
     * @brief 构造空值
     * @return 空值
     */
    NEFORCE_NODISCARD static json_value make_null() noexcept { return json_value(); }

    /**
     * @brief 构造布尔值
     * @param value 布尔值
     * @return 布尔值
     */
    NEFORCE_NODISCARD static json_value make_bool(const bool value) noexcept {
        json_value result;
        result.storage_.ext_.tag_ = json_kind::boolean;
        result.storage_.ext_.payload_.boolean_ = value;
        return result;
    }

    /**
     * @brief 构造有符号整数
     * @param value 整数值
     * @return 整数值
     */
    NEFORCE_NODISCARD static json_value make_integer(const int64_t value) noexcept {
        json_value result;
        result.storage_.ext_.tag_ = json_kind::integer;
        result.storage_.ext_.payload_.integer_ = value;
        return result;
    }

    /**
     * @brief 构造无符号整数
     * @param value 无符号整数值
     * @return 无符号整数值
     */
    NEFORCE_NODISCARD static json_value make_uinteger(const uint64_t value) noexcept {
        json_value result;
        result.storage_.ext_.tag_ = json_kind::uinteger;
        result.storage_.ext_.payload_.uinteger_ = value;
        return result;
    }

    /**
     * @brief 构造字符串值
     * @param text 字符串视图，调用方需保证其生命周期不短于使用期
     * @return 字符串值
     * @note 本函数不复制内容；需要自包含文档时使用 `json_document::make_string_value`
     */
    NEFORCE_NODISCARD static json_value make_string(const string_view text) noexcept {
        json_value result;
        result.storage_.ext_.tag_ = json_kind::string;
        result.storage_.ext_.count_ = static_cast<uint32_t>(text.size());
        result.storage_.ext_.payload_.chars_ = text.data();
        return result;
    }

    /**
     * @brief 构造浮点值
     * @param value 浮点值
     * @return 浮点值
     */
    NEFORCE_NODISCARD static json_value make_real(const double value) noexcept {
        json_value result;
        result.storage_.ext_.tag_ = json_kind::real;
        result.storage_.ext_.payload_.real_ = value;
        return result;
    }

    /**
     * @brief 获取值类型
     * @return 类型标签
     */
    NEFORCE_NODISCARD json_kind kind() const noexcept { return storage_.ext_.tag_; }

    NEFORCE_NODISCARD bool is_null() const noexcept { return storage_.ext_.tag_ == json_kind::null_; }

    NEFORCE_NODISCARD bool is_bool() const noexcept { return storage_.ext_.tag_ == json_kind::boolean; }

    NEFORCE_NODISCARD bool is_number() const noexcept {
        return storage_.ext_.tag_ == json_kind::integer || storage_.ext_.tag_ == json_kind::uinteger ||
               storage_.ext_.tag_ == json_kind::real;
    }

    NEFORCE_NODISCARD bool is_string() const noexcept { return storage_.ext_.tag_ == json_kind::string; }

    NEFORCE_NODISCARD bool is_array() const noexcept { return storage_.ext_.tag_ == json_kind::array; }

    NEFORCE_NODISCARD bool is_object() const noexcept { return storage_.ext_.tag_ == json_kind::object; }

    /**
     * @brief 获取字符串长度或元素/成员个数
     * @return 个数
     */
    NEFORCE_NODISCARD uint32_t element_count() const noexcept {
        if (is_inline_string()) {
            return storage_.ext_.flags_ & mask_inline_length;
        }
        return storage_.ext_.count_;
    }

    /**
     * @brief 以布尔视图读取
     * @return 布尔视图
     */
    NEFORCE_NODISCARD json_bool as_bool() const noexcept;

    /**
     * @brief 以数值视图读取
     * @return 数值视图
     */
    NEFORCE_NODISCARD json_number as_number() const noexcept;

    /**
     * @brief 以字符串视图读取
     * @return 字符串视图
     */
    NEFORCE_NODISCARD json_string as_string() const noexcept;

    /**
     * @brief 以数组视图读取
     * @return 数组视图
     */
    NEFORCE_NODISCARD json_array as_array() const noexcept;

    /**
     * @brief 以对象视图读取
     * @return 对象视图
     */
    NEFORCE_NODISCARD json_object as_object() const noexcept;

    /**
     * @brief 序列化为紧凑文本
     * @return JSON 文本
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 序列化为带缩进的文本
     * @param indent 每层缩进空格数
     * @return JSON 文本
     */
    NEFORCE_NODISCARD string to_indented_string(size_t indent = 2) const;

private:
    friend class json_document;
    friend class json_builder;
    friend class json_parser;
    friend class json_object_mut;
    friend class json_array_mut;

    static constexpr uint32_t inline_string_capacity = 14;
    static constexpr uint8_t flag_inline_string = 0x80;
    static constexpr uint8_t mask_inline_length = 0x0F;

    NEFORCE_NODISCARD static json_value make_inline_string(string_view text);
    NEFORCE_NODISCARD static json_value make_inline_string_unchecked(string_view text) noexcept;

    NEFORCE_NODISCARD bool is_inline_string() const noexcept {
        return storage_.ext_.tag_ == json_kind::string && (storage_.ext_.flags_ & flag_inline_string) != 0;
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
            json_kind tag_;     ///< 类型标签
            uint8_t flags_;     ///< 内部标记
            uint16_t reserved_; ///< 保留
            uint32_t count_;    ///< 字符串长度或元素/成员个数

            /**
             * @brief 载荷
             */
            union payload {
                bool boolean_;
                int64_t integer_;
                uint64_t uinteger_;
                double real_;
                const char* chars_;
                json_value* elements_;
                json_member* members_;
            } payload_; ///< 载荷

        } ext_; ///< 统一形态

        /**
         * @brief 内联字符串形态
         */
        struct inline_form {
            json_kind tag_;  ///< 类型标签
            uint8_t flags_;  ///< 内部标记与长度
            char chars_[14]; ///< 内联字符

        } inl_; ///< 内联形态

    } storage_{}; ///< 值存储

    static_assert(sizeof(storage_form) == 16, "storage_form must stay 16 bytes");
};

#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic pop
#endif


/**
 * @struct json_member
 * @brief JSON 对象的一个成员
 */
struct NEFORCE_API json_member {
    /// 内联键最大字节数
    static constexpr uint32_t inline_key_capacity = 8;

    /// 键内联标记位
    static constexpr uint32_t flag_inline_key = 0x80000000U;

    /// 键前缀覆盖的字节数
    static constexpr uint32_t key_prefix_size = 4;

    const char* key_data = nullptr; ///< 键起始地址，或不超过 8 字节的内联键本体
    uint32_t key_size = 0;          ///< 键长度，最高位为内联标记
    uint32_t key_prefix = 0;        ///< 键前 4 字节，不足 4 字节时高位补 0
    json_value value{};             ///< 成员值

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
 * @class json_bool
 * @brief 布尔视图
 */
class NEFORCE_API json_bool {
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
    const json_bool* operator->() const noexcept { return this; }

private:
    friend class json_value;

    explicit json_bool(const bool value) noexcept :
    value_(value) {}
    bool value_ = false;
};

/**
 * @class json_number
 * @brief 数值视图，保留整数/无符号整数/浮点三种来源
 */
class NEFORCE_API json_number {
public:
    /**
     * @brief 以浮点形式获取数值
     * @return 浮点值
     */
    NEFORCE_NODISCARD double get_value() const noexcept { return real_; }

    /**
     * @brief 获取有符号整数
     * @return 整数视图
     */
    NEFORCE_NODISCARD int64_t as_integer() const noexcept { return raw_.integer_; }

    /**
     * @brief 获取无符号整数
     * @return 无符号整数视图
     */
    NEFORCE_NODISCARD uint64_t as_uinteger() const noexcept { return raw_.uinteger_; }

    /**
     * @brief 获取浮点值
     * @return 浮点值
     */
    NEFORCE_NODISCARD double as_real() const noexcept { return real_; }

    /**
     * @brief 判断是否来自整数类型
     * @return 是否为整数
     */
    NEFORCE_NODISCARD bool is_integral() const noexcept { return kind_ != json_kind::real; }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const json_number* operator->() const noexcept { return this; }

private:
    friend class json_value;

    json_kind kind_ = json_kind::real;
    double real_ = 0.0;
    union integer {
        int64_t integer_;
        uint64_t uinteger_;
    } raw_{};
};

/**
 * @class json_string
 * @brief 字符串视图
 */
class NEFORCE_API json_string {
public:
    /**
     * @brief 获取字符串内容
     * @return 字符串视图
     */
    NEFORCE_NODISCARD const string_view& get_value() const noexcept { return text_; }

    /**
     * @brief 获取数据地址
     * @return 数据地址
     */
    NEFORCE_NODISCARD const char* data() const noexcept { return text_.data(); }

    /**
     * @brief 获取长度
     * @return 长度
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return text_.size(); }

    /**
     * @brief 转换为字符串视图
     * @return 字符串视图
     */
    operator string_view() const noexcept { return text_; }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const json_string* operator->() const noexcept { return this; }

private:
    friend class json_value;

    explicit json_string(const string_view text) noexcept :
    text_(text) {}

    string_view text_;
};

/**
 * @class json_array
 * @brief 数组只读视图
 */
class NEFORCE_API json_array {
public:
    /**
     * @brief 兼容旧写法的成员访问运算符
     * @return 自身指针
     */
    const json_array* operator->() const noexcept { return this; }

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
    NEFORCE_NODISCARD const json_value* get_element(const size_t index) const noexcept {
        return index < size_ ? data_ + index : nullptr;
    }

    /**
     * @brief 获取元素区间
     * @return 自身引用
     */
    NEFORCE_NODISCARD json_array get_elements() const noexcept { return *this; }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const json_value* begin() const noexcept { return data_; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const json_value* end() const noexcept { return data_ + size_; }

private:
    friend class json_value;

    json_array(const json_value* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const json_value* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @class json_object
 * @brief 对象只读视图
 */
class NEFORCE_API json_object {
public:
    /**
     * @brief 兼容旧写法的成员访问运算符
     * @return 自身指针
     */
    const json_object* operator->() const noexcept { return this; }

    /**
     * @brief 获取成员个数
     * @return 成员个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 判断对象是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief 按 key 查找成员
     * @param key 键
     * @return 成员值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const json_value* get_member(string_view key) const noexcept;

    /**
     * @brief 按 key 查找成员
     * @param key 键
     * @return 成员值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const json_value* get_member(const string& key) const noexcept {
        return this->get_member(string_view(key.data(), key.size()));
    }

    /**
     * @brief 按 key 查找成员
     * @param key 键
     * @return 成员值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const json_value* get_member(const char* key) const noexcept {
        return this->get_member(string_view(key));
    }

    /**
     * @brief 获取成员区间
     * @return 自身引用
     */
    NEFORCE_NODISCARD json_object get_members() const noexcept { return *this; }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const json_member* begin() const noexcept { return data_; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const json_member* end() const noexcept { return data_ + size_; }

private:
    friend class json_value;

    json_object(const json_member* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const json_member* data_ = nullptr;
    size_t size_ = 0;
};


/**
 * @class json_document
 * @brief 拥有 DOM 内存的 JSON 文档，可移动不可拷贝
 */
class NEFORCE_API json_document {
public:
    json_document() noexcept = default;
    ~json_document() = default;

    json_document(json_document&&) noexcept = default;
    json_document& operator=(json_document&&) noexcept = default;

    json_document(const json_document&) = delete;
    json_document& operator=(const json_document&) = delete;

    /**
     * @brief 根值访问
     * @return 根值引用
     */
    NEFORCE_NODISCARD json_value& root() noexcept { return root_; }

    /**
     * @brief 根值访问
     * @return 根值引用
     */
    NEFORCE_NODISCARD const json_value& root() const noexcept { return root_; }

    /**
     * @brief 兼容旧写法的成员访问运算符
     * @return 根值指针
     */
    const json_value* operator->() const noexcept { return &root_; }

    /**
     * @brief 兼容旧写法的成员访问运算符
     * @return 根值指针
     */
    json_value* operator->() noexcept { return &root_; }

    /**
     * @brief 是否为空文档
     * @return 根值是否为空值
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return root_.is_null(); }

    /**
     * @brief 获取根值指针
     * @return 根值指针
     */
    NEFORCE_NODISCARD const json_value* get() const noexcept { return &root_; }

    /**
     * @brief 获取根值指针
     * @return 根值指针
     */
    NEFORCE_NODISCARD json_value* get() noexcept { return &root_; }

    /**
     * @brief 转换到布尔值
     * @return 根值是否非空
     */
    explicit operator bool() const noexcept { return !root_.is_null(); }

    /**
     * @brief 把根值构造为空对象
     * @return 对象可变视图
     * @throws memory_exception 分配失败时抛出
     */
    json_object_mut make_object();

    /**
     * @brief 把根值构造为空数组
     * @return 数组可变视图
     * @throws memory_exception 分配失败时抛出
     */
    json_array_mut make_array();

    /**
     * @brief 新建一个空对象值
     * @return 对象可变视图
     * @throws memory_exception 分配失败时抛出
     */
    json_object_mut new_object();

    /**
     * @brief 新建一个空数组值
     * @return 数组可变视图
     * @throws memory_exception 分配失败时抛出
     */
    json_array_mut new_array();

    /**
     * @brief 在 arena 中新建空容器节点
     * @param is_object true 新建对象，false 新建数组
     * @param reserve 预分配的元素/成员个数，0 表示按默认规则分配
     * @return 节点指针
     * @throws memory_exception 分配失败时抛出
     */
    json_value* new_container(bool is_object, size_t reserve = 0);

    /**
     * @brief 向对象节点追加成员
     * @param node 对象节点
     * @param key 键
     * @param value 成员值
     * @param copy_key 键是否需要复制进 arena
     * @throws memory_exception 分配失败时抛出
     */
    void object_append(json_value& node, string_view key, json_value value, bool copy_key = true);

    /**
     * @brief 向数组节点追加元素
     * @param node 数组节点
     * @param value 元素值
     * @throws memory_exception 分配失败时抛出
     */
    void array_append(json_value& node, json_value value);

    /**
     * @brief 一次性预分配对象的成员数组
     * @param node 对象节点
     * @param count 预期成员个数
     * @throws memory_exception 分配失败时抛出
     */
    void object_reserve(json_value& node, size_t count);

    /**
     * @brief 一次性预分配数组的元素数组
     * @param node 数组节点
     * @param count 预期元素个数
     * @throws memory_exception 分配失败时抛出
     */
    void array_reserve(json_value& node, size_t count);

    /**
     * @brief 构造字符串值
     * @param text 字符串视图
     * @return 字符串值
     * @throws memory_exception 分配失败时抛出
     */
    NEFORCE_NODISCARD json_value make_string_value(string_view text);

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
    void reserve(size_t bytes) { arena_.reserve(bytes); }

private:
    friend class json_parser;
    friend class json_builder;
    friend class json_object_mut;
    friend class json_array_mut;

    json_arena arena_; ///< 文档内存池
    json_value root_;  ///< 根值
};


inline json_bool json_value::as_bool() const noexcept { return json_bool(storage_.ext_.payload_.boolean_); }

inline json_number json_value::as_number() const noexcept {
    json_number result;
    result.kind_ = storage_.ext_.tag_;
    switch (storage_.ext_.tag_) {
        case json_kind::integer:
            result.real_ = static_cast<double>(storage_.ext_.payload_.integer_);
            result.raw_.integer_ = storage_.ext_.payload_.integer_;
            break;
        case json_kind::uinteger:
            result.real_ = static_cast<double>(storage_.ext_.payload_.uinteger_);
            result.raw_.uinteger_ = storage_.ext_.payload_.uinteger_;
            break;
        default:
            result.real_ = storage_.ext_.payload_.real_;
            break;
    }
    return result;
}

inline json_string json_value::as_string() const noexcept {
    if (is_inline_string()) {
        return json_string(string_view(storage_.inl_.chars_, storage_.ext_.flags_ & mask_inline_length));
    }
    return json_string(string_view(storage_.ext_.payload_.chars_, storage_.ext_.count_));
}

inline json_array json_value::as_array() const noexcept {
    return json_array(storage_.ext_.payload_.elements_, storage_.ext_.count_);
}

inline json_object json_value::as_object() const noexcept {
    return json_object(storage_.ext_.payload_.members_, storage_.ext_.count_);
}

inline const json_value* json_object::get_member(const string_view key) const noexcept {
    if (size_ == 0) {
        return nullptr;
    }
    const uint32_t prefix = json_member::prefix_of(key);
    for (size_t i = 0; i < size_; ++i) {
        if (data_[i].key_equal(prefix, key)) {
            return &data_[i].value;
        }
    }
    return nullptr;
}


/**
 * @class json_object_mut
 * @brief 对象可变视图，用于构建 DOM
 */
class NEFORCE_API json_object_mut {
public:
    /**
     * @brief 追加一个成员
     * @param key 键
     * @param value 成员值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    json_object_mut& add_member(string_view key, json_value value);

    /**
     * @brief 追加一个成员
     * @param key 键
     * @param value 成员值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    json_object_mut& add_member(const string& key, json_value value) {
        return this->add_member(string_view(key.data(), key.size()), value);
    }

    /**
     * @brief 追加一个成员
     * @param key 键
     * @param value 成员值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    json_object_mut& add_member(const char* key, json_value value) { return this->add_member(string_view(key), value); }

    /**
     * @brief 取底层值
     * @return 值拷贝
     */
    NEFORCE_NODISCARD json_value to_value() const noexcept { return *node_; }

    /**
     * @brief 以只读视图访问
     * @return 对象视图
     */
    NEFORCE_NODISCARD json_object as_object() const noexcept { return node_->as_object(); }

    /**
     * @brief 成员个数
     * @return 成员个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return node_ == nullptr ? 0 : node_->element_count(); }

private:
    friend class json_document;
    friend class json_value;

    json_object_mut(json_document* doc, json_value* node) noexcept :
    doc_(doc),
    node_(node) {}

    json_document* doc_ = nullptr;
    json_value* node_ = nullptr;
};

/**
 * @class json_array_mut
 * @brief 数组可变视图，用于构建 DOM
 */
class NEFORCE_API json_array_mut {
public:
    /**
     * @brief 追加一个元素
     * @param value 元素值
     * @return 自身引用
     * @throws memory_exception 分配失败时抛出
     */
    json_array_mut& add_element(json_value value);

    /**
     * @brief 取底层值
     * @return 值拷贝
     */
    NEFORCE_NODISCARD json_value to_value() const noexcept { return *node_; }

    /**
     * @brief 以只读视图访问
     * @return 数组视图
     */
    NEFORCE_NODISCARD json_array as_array() const noexcept { return node_->as_array(); }

    /**
     * @brief 元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return node_ == nullptr ? 0 : node_->element_count(); }

private:
    friend class json_document;
    friend class json_value;

    json_array_mut(json_document* doc, json_value* node) noexcept :
    doc_(doc),
    node_(node) {}

    json_document* doc_ = nullptr;
    json_value* node_ = nullptr;
};

/** @} */ // JsonConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_DOM_HPP__
