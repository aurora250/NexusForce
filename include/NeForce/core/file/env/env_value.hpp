#ifndef NEFORCE_CORE_FILE_ENV_ENV_VALUE_HPP__
#define NEFORCE_CORE_FILE_ENV_ENV_VALUE_HPP__

/**
 * @file env_value.hpp
 * @brief ENV 值、视图与文档
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
 * @struct env_exception
 * @brief ENV格式操作失败
 */
struct env_exception final : value_exception {
    explicit env_exception(const char* info = "ENV Operation Failed.") noexcept :
    value_exception(info) {}

    explicit env_exception(const exception& e) :
    value_exception(e) {}

    ~env_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "env_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup ConfigFormat 配置格式操作
 * @brief 配置格式管理
 * @{
 */

/**
 * @defgroup EnvConfig ENV配置
 * @brief ENV配置格式管理
 * @{
 */

class env_value;
class env_document;
class env_builder;
class env_parser;
class env_variable;
class env_variables;
class env_comments;

struct env_member;

/**
 * @enum env_quote
 * @brief 值引号类型
 */
enum class env_quote : uint8_t {
    none,        ///< 无引号
    single,      ///< 单引号
    double_quote ///< 双引号
};


#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

/**
 * @class env_value
 * @brief 环境变量值，16 字节、平凡可析构
 */
class NEFORCE_API env_value : public istringify<env_value> {
public:
    /**
     * @enum kind
     * @brief 值类型
     */
    enum class kind : uint8_t {
        variable ///< 变量
    };

    /**
     * @brief 构造空变量
     */
    env_value() noexcept = default;

    /**
     * @brief 构造变量值
     * @param text 值文本
     * @param quote 引号类型
     * @param exported 是否导出
     * @return 变量值
     */
    NEFORCE_NODISCARD static env_value make_variable(const string_view text, env_quote quote, bool exported) noexcept {
        if (text.size() <= inline_value_capacity) {
            return make_inline_variable_unchecked(text, quote, exported);
        }
        env_value result;
        result.storage_.ext_.tag_ = kind::variable;
        result.storage_.ext_.flags_ = pack_flags(quote, exported);
        result.storage_.ext_.count_ = static_cast<uint32_t>(text.size());
        result.storage_.ext_.payload_.chars_ = text.data();
        return result;
    }

    /**
     * @brief 获取值类型
     * @return 类型标签
     */
    NEFORCE_NODISCARD kind type() const noexcept { return storage_.ext_.tag_; }

    /**
     * @brief 判断是否为变量
     * @return 是否为变量
     */
    NEFORCE_NODISCARD bool is_variable() const noexcept { return storage_.ext_.tag_ == kind::variable; }

    /**
     * @brief 获取值长度
     * @return 长度
     */
    NEFORCE_NODISCARD uint32_t element_count() const noexcept {
        if (is_inline()) {
            return storage_.ext_.flags_ & mask_inline_length;
        }
        return storage_.ext_.count_;
    }

    /**
     * @brief 以变量视图读取
     * @return 变量视图
     */
    NEFORCE_NODISCARD env_variable as_variable() const noexcept;

    /**
     * @brief 转换为字符串
     * @return 值的字符串表示
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档字符串
     * @return 与 to_string() 相同
     */
    NEFORCE_NODISCARD string to_document() const;

private:
    friend class env_document;
    friend class env_builder;
    friend class env_parser;
    friend class env_variable;

    static constexpr uint32_t inline_value_capacity = 14;
    static constexpr uint8_t flag_inline_value = 0x80;
    static constexpr uint8_t quote_shift = 4;
    static constexpr uint8_t mask_inline_length = 0x0F;
    static constexpr uint8_t mask_quote = 0x30;
    static constexpr uint8_t flag_exported = 0x40;

    NEFORCE_NODISCARD static env_value make_inline_variable_unchecked(string_view text, env_quote quote,
                                                                      bool exported) noexcept;

    NEFORCE_NODISCARD static uint8_t pack_flags(const env_quote quote, const bool exported) noexcept {
        return static_cast<uint8_t>((static_cast<uint8_t>(quote) << quote_shift) | (exported ? flag_exported : 0));
    }

    NEFORCE_NODISCARD bool is_inline() const noexcept { return (storage_.ext_.flags_ & flag_inline_value) != 0; }

    /**
     * @struct storage_form
     * @brief 值的 16 字节存储
     */
    union storage_form {
        /**
         * @brief 统一形态
         */
        struct extended_form {
            kind tag_ = kind::variable;         ///< 类型标签
            uint8_t flags_ = flag_inline_value; ///< 内联标记、引号与导出标记
            uint16_t reserved_ = 0;             ///< 预留
            uint32_t count_ = 0;                ///< 字符串长度

            /**
             * @brief 载荷
             */
            union payload {
                const char* chars_; ///< 字符串起始地址

            } payload_{}; ///< 载荷

        } ext_; ///< 统一形态

        /**
         * @brief 内联形态
         */
        struct inline_form {
            kind tag_ = kind::variable;         ///< 类型标签
            uint8_t flags_ = flag_inline_value; ///< 内联标记、引号、导出标记与长度
            char chars_[14];                    ///< 内联字符

        } inl_; ///< 内联形态

    } storage_{}; ///< 值存储

    static_assert(sizeof(storage_form) == 16, "storage_form must stay 16 bytes");
};

#ifdef NEFORCE_COMPILER_GCC
#    pragma GCC diagnostic pop
#endif


/**
 * @class env_variable
 * @brief 环境变量只读视图
 */
class NEFORCE_API env_variable {
public:
    /**
     * @enum quote_type
     * @brief 引号类型
     */
    enum quote_type {
        None,   ///< 无引号
        Single, ///< 单引号
        Double  ///< 双引号
    };

    /**
     * @brief 获取变量值
     * @return 变量值视图
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
     * @return 变量值视图
     */
    operator string_view() const noexcept { return get_value(); }

    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const env_variable* operator->() const noexcept { return this; }

    /**
     * @brief 获取引号类型
     * @return 引号类型
     */
    NEFORCE_NODISCARD quote_type get_quote_type() const noexcept;

    /**
     * @brief 判断是否导出
     * @return 是否导出
     */
    NEFORCE_NODISCARD bool is_exported() const noexcept { return (flags_ & env_value::flag_exported) != 0; }

    /**
     * @brief 获取类型
     * @return 变量类型
     */
    NEFORCE_NODISCARD env_value::kind type() const noexcept { return env_value::kind::variable; }

    /**
     * @brief 判断是否为变量
     * @return 恒为 true
     */
    NEFORCE_NODISCARD bool is_variable() const noexcept { return true; }

    /**
     * @brief 获取整数值
     * @param default_value 解析失败时的默认值
     * @return 解析后的整数值
     */
    NEFORCE_NODISCARD int get_int(int default_value = 0) const noexcept;

    /**
     * @brief 获取 64 位整数值
     * @param default_value 解析失败时的默认值
     * @return 解析后的 64 位整数值
     */
    NEFORCE_NODISCARD int64_t get_int64(int64_t default_value = 0) const noexcept;

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
     * @return 变量值
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 转换为文档字符串
     * @return 变量值
     */
    NEFORCE_NODISCARD string to_document() const;

private:
    friend class env_value;

    env_variable(const char* data, const size_t size, const uint8_t flags) noexcept :
    data_(data),
    size_(size),
    flags_(flags) {}

    const char* data_ = nullptr;
    size_t size_ = 0;
    uint8_t flags_ = 0;
};

/**
 * @struct env_member
 * @brief 文档中的一个环境变量
 */
struct NEFORCE_API env_member {
    /// 内联键最大字节数
    static constexpr uint32_t inline_key_capacity = 8;
    /// 键内联标记位
    static constexpr uint32_t flag_inline_key = 0x80000000U;
    /// 键前缀覆盖的字节数
    static constexpr uint32_t key_prefix_size = 4;

    const char* key_data = nullptr; ///< 键起始地址或内联键本体
    uint32_t key_size = 0;          ///< 键长度，最高位为内联标记
    uint32_t key_prefix = 0;        ///< 键前 4 字节
    env_value value{};              ///< 变量值

    /**
     * @brief 判断键是否内联
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
     * @return 键视图
     */
    NEFORCE_NODISCARD string_view key() const noexcept {
        if (key_inlined()) {
            return string_view(reinterpret_cast<const char*>(&key_data), key_length());
        }
        return string_view(key_data, key_length());
    }

    /**
     * @brief 获取变量视图
     * @return 变量视图
     */
    NEFORCE_NODISCARD env_variable variable() const noexcept { return value.as_variable(); }

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
     * @param text 键视图
     */
    void assign_key(string_view text) noexcept;
};

/**
 * @class env_variables
 * @brief 变量列表只读视图
 */
class NEFORCE_API env_variables {
public:
    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const env_variables* operator->() const noexcept { return this; }

    /**
     * @brief 变量个数
     * @return 个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief 按下标访问
     * @param index 下标
     * @return 成员指针，越界返回 nullptr
     */
    NEFORCE_NODISCARD const env_member* get(const size_t index) const noexcept {
        return index < size_ ? data_ + index : nullptr;
    }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const env_member* begin() const noexcept { return data_; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const env_member* end() const noexcept { return data_ == nullptr ? nullptr : data_ + size_; }

private:
    friend class env_document;

    env_variables(const env_member* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const env_member* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @class env_comments
 * @brief 注释列表只读视图
 */
class NEFORCE_API env_comments {
public:
    /**
     * @brief 兼容指针写法的成员访问运算符
     * @return 自身指针
     */
    const env_comments* operator->() const noexcept { return this; }

    /**
     * @brief 注释个数
     * @return 个数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_ == 0; }

    /**
     * @brief 按下标访问
     * @param index 下标
     * @return 注释视图
     */
    NEFORCE_NODISCARD string_view operator[](const size_t index) const noexcept { return data_[index]; }

    /**
     * @brief 起始迭代器
     * @return 起始指针
     */
    NEFORCE_NODISCARD const string_view* begin() const noexcept { return data_; }

    /**
     * @brief 结束迭代器
     * @return 结束指针
     */
    NEFORCE_NODISCARD const string_view* end() const noexcept { return data_ == nullptr ? nullptr : data_ + size_; }

private:
    friend class env_document;

    env_comments(const string_view* data, const size_t size) noexcept :
    data_(data),
    size_(size) {}

    const string_view* data_ = nullptr;
    size_t size_ = 0;
};

/**
 * @class env_document
 * @brief 拥有 DOM 内存的环境变量文档
 */
class NEFORCE_API env_document : public istringify<env_document> {
public:
    env_document() = default;
    ~env_document() = default;

    env_document(env_document&&) noexcept = default;
    env_document& operator=(env_document&&) noexcept = default;

    env_document(const env_document&) = delete;
    env_document& operator=(const env_document&) = delete;

    /**
     * @brief 设置变量，同名变量会被覆盖
     * @param name 变量名
     * @param value 变量值
     * @param quote 引号类型
     * @param exported 是否导出
     */
    void set_variable(string_view name, string_view value, env_variable::quote_type quote = env_variable::None,
                      bool exported = false);

    /**
     * @brief 设置变量，同名变量会被覆盖
     * @param name 变量名
     * @param value 变量值
     * @param quote 引号类型
     * @param exported 是否导出
     */
    void set_variable(const string& name, const string& value,
                      const env_variable::quote_type quote = env_variable::None, const bool exported = false) {
        set_variable(string_view(name.data(), name.size()), string_view(value.data(), value.size()), quote, exported);
    }

    /**
     * @brief 设置变量，同名变量会被覆盖
     * @param name 变量名
     * @param value 变量值
     * @param quote 引号类型
     * @param exported 是否导出
     */
    void set_variable(const char* name, const char* value, const env_variable::quote_type quote = env_variable::None,
                      const bool exported = false) {
        set_variable(string_view(name), string_view(value), quote, exported);
    }

    /**
     * @brief 添加变量，同名变量会被覆盖
     * @param name 变量名
     * @param value 变量值
     * @param quote 引号类型
     * @param exported 是否导出
     */
    void add_variable(string_view name, string_view value, env_variable::quote_type quote = env_variable::None,
                      bool exported = false) {
        set_variable(name, value, quote, exported);
    }

    /**
     * @brief 按名称查找变量
     * @param name 变量名
     * @return 变量值指针，不存在返回 nullptr
     */
    NEFORCE_NODISCARD const env_value* get_variable(string_view name) const noexcept;

    /**
     * @brief 按名称查找变量
     * @param name 变量名
     * @return 变量值指针，不存在返回 nullptr
     */
    NEFORCE_NODISCARD const env_value* get_variable(const string& name) const noexcept {
        return get_variable(string_view(name.data(), name.size()));
    }

    /**
     * @brief 按名称查找变量
     * @param name 变量名
     * @return 变量值指针，不存在返回 nullptr
     */
    NEFORCE_NODISCARD const env_value* get_variable(const char* name) const noexcept {
        return get_variable(string_view(name));
    }

    /**
     * @brief 检查变量是否存在
     * @param name 变量名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_variable(const string_view name) const noexcept { return get_variable(name) != nullptr; }

    /**
     * @brief 检查变量是否存在
     * @param name 变量名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_variable(const string& name) const noexcept {
        return has_variable(string_view(name.data(), name.size()));
    }

    /**
     * @brief 检查变量是否存在
     * @param name 变量名
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool has_variable(const char* name) const noexcept { return has_variable(string_view(name)); }

    /**
     * @brief 删除变量
     * @param name 变量名
     */
    void remove_variable(string_view name) noexcept;

    /**
     * @brief 删除变量
     * @param name 变量名
     */
    void remove_variable(const string& name) noexcept { remove_variable(string_view(name.data(), name.size())); }

    /**
     * @brief 删除变量
     * @param name 变量名
     */
    void remove_variable(const char* name) noexcept { remove_variable(string_view(name)); }

    /**
     * @brief 获取变量列表
     * @return 变量列表视图
     */
    NEFORCE_NODISCARD env_variables get_variables() const noexcept { return env_variables(members_, member_count_); }

    /**
     * @brief 添加注释
     * @param text 注释内容
     */
    void add_comment(string_view text);

    /**
     * @brief 添加注释
     * @param text 注释内容
     */
    void add_comment(const string& text) { add_comment(string_view(text.data(), text.size())); }

    /**
     * @brief 添加注释
     * @param text 注释内容
     */
    void add_comment(const char* text) { add_comment(string_view(text)); }

    /**
     * @brief 获取注释列表
     * @return 注释列表视图
     */
    NEFORCE_NODISCARD env_comments get_comments() const noexcept { return env_comments(comments_, comment_count_); }

    /**
     * @brief 获取字符串值
     * @param name 变量名
     * @param default_value 默认值
     * @return 变量值
     */
    NEFORCE_NODISCARD string get_string(const string_view name, const string_view default_value = "") const {
        const env_value* value = get_variable(name);
        if (value == nullptr) {
            return string(default_value.data(), default_value.size());
        }
        const env_variable variable = value->as_variable();
        return string(variable.data(), variable.size());
    }

    /**
     * @brief 获取整数值
     * @param name 变量名
     * @param default_value 默认值
     * @return 整数值
     */
    NEFORCE_NODISCARD int get_int(const string_view name, const int default_value = 0) const {
        const env_value* value = get_variable(name);
        return value != nullptr ? value->as_variable().get_int(default_value) : default_value;
    }

    /**
     * @brief 获取 64 位整数值
     * @param name 变量名
     * @param default_value 默认值
     * @return 64 位整数值
     */
    NEFORCE_NODISCARD int64_t get_int64(const string_view name, const int64_t default_value = 0) const {
        const env_value* value = get_variable(name);
        return value != nullptr ? value->as_variable().get_int64(default_value) : default_value;
    }

    /**
     * @brief 获取双精度浮点值
     * @param name 变量名
     * @param default_value 默认值
     * @return 双精度浮点值
     */
    NEFORCE_NODISCARD double get_double(const string_view name, const double default_value = 0.0) const {
        const env_value* value = get_variable(name);
        return value != nullptr ? value->as_variable().get_double(default_value) : default_value;
    }

    /**
     * @brief 获取布尔值
     * @param name 变量名
     * @param default_value 默认值
     * @return 布尔值
     */
    NEFORCE_NODISCARD bool get_bool(const string_view name, const bool default_value = false) const {
        const env_value* value = get_variable(name);
        return value != nullptr ? value->as_variable().get_bool(default_value) : default_value;
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
    friend class env_parser;
    friend class env_builder;

    void set_variable_impl(string_view name, string_view value, env_quote quote, bool exported, bool copy);
    void add_comment_impl(string_view text, bool copy);

    NEFORCE_NODISCARD env_member* find_member(string_view name) const noexcept;

    NEFORCE_NODISCARD string_view store_string(string_view text);
    NEFORCE_NODISCARD env_value make_value(string_view text, env_quote quote, bool exported, bool copy);

    bump_arena arena_;                ///< 文档内存池
    env_member* members_ = nullptr;   ///< 变量数组
    uint32_t member_count_ = 0;       ///< 变量个数
    uint32_t member_reserved_ = 0;    ///< 变量数组容量
    uint32_t* index_ = nullptr;       ///< 大文档的开放寻址索引
    uint32_t index_capacity_ = 0;     ///< 索引槽位个数，2 的幂
    string_view* comments_ = nullptr; ///< 注释数组
    uint32_t comment_count_ = 0;      ///< 注释个数
    uint32_t comment_reserved_ = 0;   ///< 注释数组容量
};

/** @} */ // EnvConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_ENV_ENV_VALUE_HPP__
