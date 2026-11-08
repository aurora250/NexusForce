#ifndef NEFORCE_CORE_FILE_INI_INI_VALUE_HPP__
#define NEFORCE_CORE_FILE_INI_INI_VALUE_HPP__

/**
 * @file ini_value.hpp
 * @brief INI 值、视图与文档
 */

#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/bump_arena.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct ini_exception
 * @brief INI格式操作失败
 */
struct ini_exception final : value_exception {
    explicit ini_exception(const char* info = "INI Operation Failed.") noexcept :
    value_exception(info) {}

    explicit ini_exception(const exception& e) :
    value_exception(e) {}

    ~ini_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "ini_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup ConfigFormat 配置格式操作
 * @brief 配置格式管理
 * @{
 */

/**
 * @defgroup IniConfig INI配置
 * @brief INI配置格式管理
 * @{
 */

class ini_value;
class ini_document;
class ini_builder;
class ini_parser;
class ini_section;
class ini_property;
class ini_section_mut;
class ini_sections;
class ini_section_iterator;

struct ini_member;
struct ini_section_data;

/**
 * @enum ini_kind
 * @brief INI 值类型
 */
enum class ini_kind : uint8_t {
    property, ///< 属性
    section   ///< 节
};

/**
 * @struct ini_section_data
 * @brief 节的共享载荷
 */
struct NEFORCE_API ini_section_data {
    string_view name;              ///< 节名称，全局节为空
    ini_member* members = nullptr; ///< 成员数组
    uint32_t count = 0;            ///< 成员个数
    uint16_t reserved = 0;         ///< 成员数组预分配容量
    uint32_t* index = nullptr;     ///< 大节的开放寻址索引
    uint32_t index_capacity = 0;   ///< 索引槽位个数，2 的幂
};


#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

/**
 * @class ini_value
 * @brief INI DOM 值
 */
class NEFORCE_API ini_value : public istringify<ini_value> {
public:
    /**
     * @brief 构造空属性
     */
    ini_value() noexcept = default;

    /**
     * @brief 构造属性字符串
     * @param text 字符串视图，调用方需保证其生命周期不短于使用期
     * @return 属性值
     */
    NEFORCE_NODISCARD static ini_value make_property(const string_view text) noexcept {
        if (text.size() <= inline_string_capacity) {
            return make_inline_property_unchecked(text);
        }
        ini_value result;
        result.storage_.ext_.tag_ = ini_kind::property;
        result.storage_.ext_.flags_ = 0;
        result.storage_.ext_.count_ = static_cast<uint32_t>(text.size());
        result.storage_.ext_.payload_.chars_ = text.data();
        return result;
    }

    /**
     * @brief 获取值类型
     * @return 类型标签
     */
    NEFORCE_NODISCARD ini_kind kind() const noexcept { return storage_.ext_.tag_; }

    /**
     * @brief 判断是否为属性
     * @return 是否为属性
     */
    NEFORCE_NODISCARD bool is_property() const noexcept { return storage_.ext_.tag_ == ini_kind::property; }

    /**
     * @brief 判断是否为节
     * @return 是否为节
     */
    NEFORCE_NODISCARD bool is_section() const noexcept { return storage_.ext_.tag_ == ini_kind::section; }

    /**
     * @brief 获取字符串长度或节成员个数
     * @return 个数
     */
    NEFORCE_NODISCARD uint32_t element_count() const noexcept {
        if (is_inline_property()) {
            return storage_.ext_.flags_ & mask_inline_length;
        }
        if (is_section()) {
            return storage_.ext_.payload_.section_->count;
        }
        return storage_.ext_.count_;
    }

    /**
     * @brief 以属性视图读取
     * @return 属性视图
     */
    NEFORCE_NODISCARD ini_property as_property() const noexcept;

    /**
     * @brief 以节视图读取
     * @return 节视图
     */
    NEFORCE_NODISCARD ini_section as_section() const noexcept;

    /**
     * @brief 转换为字符串
     * @return 属性返回原值，节返回 [name] 与属性行
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档格式字符串
     * @return 与 to_string() 相同
     */
    NEFORCE_NODISCARD string to_document() const;

private:
    friend class ini_document;
    friend class ini_builder;
    friend class ini_parser;
    friend class ini_section;
    friend class ini_property;
    friend class ini_section_mut;

    static constexpr uint32_t inline_string_capacity = 14;
    static constexpr uint8_t flag_inline_property = 0x80;
    static constexpr uint8_t mask_inline_length = 0x0F;

    NEFORCE_NODISCARD static ini_value make_inline_property_unchecked(string_view text) noexcept;

    NEFORCE_NODISCARD bool is_inline_property() const noexcept {
        return storage_.ext_.tag_ == ini_kind::property && (storage_.ext_.flags_ & flag_inline_property) != 0;
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
            ini_kind tag_ = ini_kind::property;    ///< 类型标签
            uint8_t flags_ = flag_inline_property; ///< 内联标记
            uint16_t reserved_ = 0;                ///< 预留
            uint32_t count_ = 0;                   ///< 字符串长度或成员个数

            /**
             * @brief 载荷
             */
            union payload {
                const char* chars_;         ///< 属性字符串起始地址
                ini_section_data* section_; ///< 节载荷

            } payload_{};                   ///< 载荷

        } ext_; ///< 统一形态

        /**
         * @brief 内联属性形态
         */
        struct inline_form {
            ini_kind tag_ = ini_kind::property;    ///< 类型标签
            uint8_t flags_ = flag_inline_property; ///< 内联标记与长度
            char chars_[14];                       ///< 内联字符

        } inl_; ///< 内联形态

    } storage_{}; ///< 值存储

    static_assert(sizeof(storage_form) == 16, "storage_form must stay 16 bytes");
};

#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic pop
#endif


/**
 * @struct ini_member
 * @brief INI 节的一个成员
 */
struct NEFORCE_API ini_member {
    /// 内联键最大字节数
    static constexpr uint32_t inline_key_capacity = 8;

    /// 键内联标记位
    static constexpr uint32_t flag_inline_key = 0x80000000U;

    /// 键前缀覆盖的字节数
    static constexpr uint32_t key_prefix_size = 4;

    const char* key_data = nullptr; ///< 键起始地址，或不超过 8 字节的内联键本体
    uint32_t key_size = 0;          ///< 键长度，最高位为内联标记
    uint32_t key_prefix = 0;        ///< 键前 4 字节，不足 4 字节时高位补 0
    ini_value value{};              ///< 属性值

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
     * @brief 计算键前缀
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
 * @class ini_property
 * @brief 属性只读视图
 */
class NEFORCE_API ini_property {
public:
    /**
     * @brief 获取属性值
     * @return 属性值视图
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
     * @brief 转换为字符串视图
     * @return 属性值视图
     */
    operator string_view() const noexcept { return get_value(); }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const ini_property* operator->() const noexcept { return this; }

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

    /**
     * @brief 转换为字符串
     * @return 属性值
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档字符串
     * @return 属性值
     */
    NEFORCE_NODISCARD string to_document() const;

private:
    friend class ini_value;

    ini_property(const char* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const char* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @class ini_section
 * @brief 节只读视图
 */
class NEFORCE_API ini_section {
public:
    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const ini_section* operator->() const noexcept { return this; }

    /**
     * @brief 判断视图是否指向有效节
     * @return 是否有效
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return data_ != nullptr; }

    /**
     * @brief 获取节名称
     * @return 节名称视图
     */
    NEFORCE_NODISCARD string_view get_name() const noexcept { return data_ == nullptr ? string_view() : data_->name; }

    /**
     * @brief 获取成员个数
     * @return 成员个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return data_ == nullptr ? 0 : data_->count; }

    /**
     * @brief 判断节是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0; }

    /**
     * @brief 按 key 查找属性
     * @param key 属性键名
     * @return 属性值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const ini_value* get_property(const string_view key) const noexcept;

    /**
     * @brief 按 key 查找属性
     * @param key 属性键名
     * @return 属性值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const ini_value* get_property(const string& key) const noexcept {
        return get_property(string_view(key.data(), key.size()));
    }

    /**
     * @brief 按 key 查找属性
     * @param key 属性键名
     * @return 属性值指针，未找到返回 nullptr
     */
    NEFORCE_NODISCARD const ini_value* get_property(const char* key) const noexcept {
        return get_property(string_view(key));
    }

    /**
     * @brief 检查属性是否存在
     * @param key 属性键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_property(const string_view key) const noexcept { return get_property(key) != nullptr; }

    /**
     * @brief 检查属性是否存在
     * @param key 属性键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_property(const string& key) const noexcept {
        return has_property(string_view(key.data(), key.size()));
    }

    /**
     * @brief 检查属性是否存在
     * @param key 属性键名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_property(const char* key) const noexcept { return has_property(string_view(key)); }

    /**
     * @brief 获取成员区间
     * @return 自身引用
     */
    NEFORCE_NODISCARD ini_section get_properties() const noexcept { return *this; }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const ini_member* begin() const noexcept { return data_ == nullptr ? nullptr : data_->members; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const ini_member* end() const noexcept {
        return data_ == nullptr || data_->members == nullptr ? nullptr : data_->members + data_->count;
    }

    /**
     * @brief 获取字符串值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的字符串值
     */
    NEFORCE_NODISCARD string get_string(const string_view key, const string_view default_value = "") const {
        const ini_value* prop = get_property(key);
        if (prop == nullptr) {
            return string(default_value.data(), default_value.size());
        }
        const string_view value = prop->as_property().get_value();
        return string(value.data(), value.size());
    }

    /**
     * @brief 获取整数值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的整数值
     */
    NEFORCE_NODISCARD int get_int(const string_view key, const int default_value = 0) const {
        const ini_value* prop = get_property(key);
        return prop != nullptr ? prop->as_property().get_int(default_value) : default_value;
    }

    /**
     * @brief 获取 64 位整数值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的 64 位整数值
     */
    NEFORCE_NODISCARD int64_t get_int64(string_view key, int64_t default_value = 0) const noexcept;

    /**
     * @brief 获取双精度浮点值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的双精度浮点值
     */
    NEFORCE_NODISCARD double get_double(const string_view key, const double default_value = 0.0) const {
        const ini_value* prop = get_property(key);
        return prop != nullptr ? prop->as_property().get_double(default_value) : default_value;
    }

    /**
     * @brief 获取布尔值
     * @param key 属性键名
     * @param default_value 默认值
     * @return 属性的布尔值
     */
    NEFORCE_NODISCARD bool get_bool(const string_view key, const bool default_value = false) const {
        const ini_value* prop = get_property(key);
        return prop != nullptr ? prop->as_property().get_bool(default_value) : default_value;
    }

    /**
     * @brief 转换为字符串
     * @return 节名称与属性行
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档字符串
     * @return 与 to_string() 相同
     */
    NEFORCE_NODISCARD string to_document() const;

private:
    friend class ini_value;
    friend class ini_document;
    friend class ini_section_mut;
    friend class ini_sections;

    explicit ini_section(const ini_section_data* data) noexcept :
    data_(data) {}

    const ini_section_data* data_ = nullptr;
};

/**
 * @class ini_section_mut
 * @brief 节可变视图，用于构建 DOM
 */
class NEFORCE_API ini_section_mut {
public:
    /**
     * @brief 追加或覆盖一个属性
     * @param key 属性键名
     * @param value 属性值
     * @return 自身引用
     */
    ini_section_mut& set_property(string_view key, string_view value);

    /**
     * @brief 追加或覆盖一个属性
     * @param key 属性键名
     * @param value 属性值
     * @return 自身引用
     */
    ini_section_mut& set_property(const string& key, const string& value) {
        return set_property(string_view(key.data(), key.size()), string_view(value.data(), value.size()));
    }

    /**
     * @brief 追加或覆盖一个属性
     * @param key 属性键名
     * @param value 属性值
     * @return 自身引用
     */
    ini_section_mut& set_property(const char* key, const char* value) {
        return set_property(string_view(key), string_view(value));
    }

    /**
     * @brief 追加或覆盖一个属性
     * @param key 属性键名
     * @param value 属性值
     * @return 自身引用
     */
    ini_section_mut& set_property(const string& key, const char* value) {
        return set_property(string_view(key.data(), key.size()), string_view(value));
    }

    /**
     * @brief 设置节名称
     * @param name 新名称
     * @return 自身引用
     */
    ini_section_mut& set_name(string_view name);

    /**
     * @brief 成员个数
     * @return 成员个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return data_ == nullptr ? 0 : data_->count; }

    /**
     * @brief 以只读视图访问
     * @return 节视图
     */
    NEFORCE_NODISCARD ini_section as_section() const noexcept { return ini_section(data_); }

private:
    friend class ini_document;
    friend class ini_parser;
    friend class ini_builder;

    NEFORCE_NODISCARD ini_section_data* data() const noexcept { return data_; }

    ini_section_mut(ini_document* doc, ini_section_data* data) noexcept :
    doc_(doc),
    data_(data) {}

    ini_document* doc_ = nullptr;
    ini_section_data* data_ = nullptr;
};

/**
 * @class ini_section_iterator
 * @brief 节列表迭代器
 */
class NEFORCE_API ini_section_iterator {
public:
    /**
     * @brief 构造迭代器
     * @param current 当前值指针
     */
    explicit ini_section_iterator(const ini_value* current) noexcept :
    current_(current) {}

    /**
     * @brief 解引用
     * @return 节视图
     */
    NEFORCE_NODISCARD ini_section operator*() const noexcept;

    /**
     * @brief 前进
     * @return 自身引用
     */
    ini_section_iterator& operator++() noexcept {
        ++current_;
        return *this;
    }

    /**
     * @brief 比较
     * @param other 另一迭代器
     * @return 是否不等
     */
    NEFORCE_NODISCARD bool operator!=(const ini_section_iterator& other) const noexcept {
        return current_ != other.current_;
    }

private:
    friend class ini_sections;

    const ini_value* current_ = nullptr;
};

/**
 * @class ini_sections
 * @brief 文档节列表只读视图
 */
class NEFORCE_API ini_sections {
public:
    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const ini_sections* operator->() const noexcept { return this; }

    /**
     * @brief 节个数
     * @return 节个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 判断列表是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief 按下标获取节
     * @param index 下标
     * @return 节视图，越界返回空视图
     */
    NEFORCE_NODISCARD ini_section get(const size_t index) const noexcept {
        return index < size_ ? data_[index].as_section() : ini_section(nullptr);
    }

    /**
     * @brief 起始迭代器
     * @return 起始迭代器
     */
    NEFORCE_NODISCARD ini_section_iterator begin() const noexcept { return ini_section_iterator(data_); }

    /**
     * @brief 结束迭代器
     * @return 结束迭代器
     */
    NEFORCE_NODISCARD ini_section_iterator end() const noexcept {
        return ini_section_iterator(data_ == nullptr ? nullptr : data_ + size_);
    }

private:
    friend class ini_document;

    ini_sections(const ini_value* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const ini_value* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @class ini_document
 * @brief 拥有 DOM 内存的 INI 文档，可移动不可拷贝
 */
class NEFORCE_API ini_document : public istringify<ini_document> {
public:
    /**
     * @brief 构造空文档
     */
    ini_document();

    ~ini_document() = default;

    ini_document(ini_document&&) noexcept = default;
    ini_document& operator=(ini_document&&) noexcept = default;

    ini_document(const ini_document&) = delete;
    ini_document& operator=(const ini_document&) = delete;

    /**
     * @brief 获取全局节
     * @return 全局节视图
     */
    NEFORCE_NODISCARD ini_section get_global_section() const noexcept { return root_.as_section(); }

    /**
     * @brief 按名称查找节
     * @param name 节名称
     * @return 节视图，不存在时返回空视图
     */
    NEFORCE_NODISCARD ini_section get_section(string_view name) const noexcept;

    /**
     * @brief 按名称查找节
     * @param name 节名称
     * @return 节视图，不存在时返回空视图
     */
    NEFORCE_NODISCARD ini_section get_section(const string& name) const noexcept {
        return get_section(string_view(name.data(), name.size()));
    }

    /**
     * @brief 按名称查找节
     * @param name 节名称
     * @return 节视图，不存在时返回空视图
     */
    NEFORCE_NODISCARD ini_section get_section(const char* name) const noexcept {
        return get_section(string_view(name));
    }

    /**
     * @brief 检查节是否存在
     * @param name 节名称
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_section(const string_view name) const noexcept { return get_section(name).valid(); }

    /**
     * @brief 检查节是否存在
     * @param name 节名称
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_section(const string& name) const noexcept {
        return has_section(string_view(name.data(), name.size()));
    }

    /**
     * @brief 检查节是否存在
     * @param name 节名称
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_section(const char* name) const noexcept { return has_section(string_view(name)); }

    /**
     * @brief 获取所有节
     * @return 节列表视图
     */
    NEFORCE_NODISCARD ini_sections get_sections() const noexcept { return ini_sections(sections_, section_count_); }

    /**
     * @brief 获取全局节的可变视图
     * @return 全局节可变视图
     */
    NEFORCE_NODISCARD ini_section_mut global_section_mut() noexcept;

    /**
     * @brief 创建或重置一个命名节
     * @param name 节名称
     * @return 节可变视图
     * @note 同名节会被重置
     */
    NEFORCE_NODISCARD ini_section_mut new_section(string_view name, bool copy = true);

    /**
     * @brief 创建或重置一个命名节
     * @param name 节名称
     * @param copy 是否复制节名称
     * @return 节可变视图
     */
    NEFORCE_NODISCARD ini_section_mut new_section(const string& name, const bool copy = true) {
        return new_section(string_view(name.data(), name.size()), copy);
    }

    /**
     * @brief 创建或重置一个命名节
     * @param name 节名称
     * @return 节可变视图
     */
    NEFORCE_NODISCARD ini_section_mut new_section(const char* name) { return new_section(string_view(name), true); }

    /**
     * @brief 获取字符串值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的字符串值
     */
    NEFORCE_NODISCARD string get_string(const string_view section, const string_view key,
                                        const string_view default_value = "") const {
        return get_section(section).get_string(key, default_value);
    }

    /**
     * @brief 获取整数值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的整数值
     */
    NEFORCE_NODISCARD int get_int(const string_view section, const string_view key, const int default_value = 0) const {
        return get_section(section).get_int(key, default_value);
    }

    /**
     * @brief 获取 64 位整数值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的 64 位整数值
     */
    NEFORCE_NODISCARD int64_t get_int64(string_view section, string_view key, int64_t default_value = 0) const noexcept;

    /**
     * @brief 获取双精度浮点值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的双精度浮点值
     */
    NEFORCE_NODISCARD double get_double(const string_view section, const string_view key,
                                        const double default_value = 0.0) const {
        return get_section(section).get_double(key, default_value);
    }

    /**
     * @brief 获取布尔值
     * @param section 节名称
     * @param key 属性键名
     * @param default_value 默认值
     * @return 指定属性的布尔值
     */
    NEFORCE_NODISCARD bool get_bool(const string_view section, const string_view key,
                                    const bool default_value = false) const {
        return get_section(section).get_bool(key, default_value);
    }

    /**
     * @brief 转换为字符串
     * @return 整个文档的字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 预留文档内存块
     * @param bytes 期望的可用字节数
     */
    void reserve(size_t bytes);

private:
    friend class ini_parser;
    friend class ini_builder;
    friend class ini_section_mut;

    NEFORCE_NODISCARD ini_section_data* new_section_node(string_view name, bool copy, size_t slots);

    NEFORCE_NODISCARD ini_section_data* find_section_data(string_view name) const noexcept;

    ini_section_data* reset_section(ini_section_data* data) noexcept;

    void append_property(ini_section_data* section, string_view key, string_view value, bool copy);

    NEFORCE_NODISCARD string_view store_string(string_view text);

    NEFORCE_NODISCARD ini_value make_property_value(string_view text, bool copy);

    bump_arena arena_;              ///< 文档内存池
    ini_value root_;                ///< 全局节
    ini_value* sections_ = nullptr; ///< 命名节数组
    uint32_t section_count_ = 0;    ///< 命名节个数
    uint32_t section_reserved_ = 0; ///< 命名节数组容量
};

/** @} */ // IniConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_INI_INI_VALUE_HPP__
