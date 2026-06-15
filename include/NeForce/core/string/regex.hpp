#ifndef NEFORCE_CORE_STRING_REGEX_HPP__
#define NEFORCE_CORE_STRING_REGEX_HPP__

/**
 * @file regex.hpp
 * @brief 正则表达式引擎封装
 *
 * 此文件提供了基于PCRE2的正则表达式。
 * 支持正则表达式的编译、匹配、查找、替换、分割等操作，
 * 并提供迭代器接口用于遍历匹配结果。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/string/string.hpp"
#include <pcre2.h>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct regex_exception
 * @brief 正则操作异常
 */
struct regex_exception final : value_exception {
    explicit regex_exception(const char* info = "Regex Operation Failed", const char* type = static_type,
                             const int code = 0) noexcept :
    value_exception(info, type, code) {}

    explicit regex_exception(const exception& e) :
    value_exception(e) {}

    ~regex_exception() override = default;

    static constexpr auto static_type = "regex_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup Regex 正则表达式
 * @brief 正则表达式操作实现
 * @{
 */

/**
 * @class match_result
 * @brief 正则表达式匹配结果类
 *
 * 存储一次正则表达式匹配的结果，包括匹配到的文本、捕获组、
 * 匹配位置等信息。支持格式化输出和迭代器访问。
 */
class NEFORCE_API match_result {
public:
    using iterator = vector<string>::const_iterator; ///< 捕获组迭代器类型

private:
    vector<string> groups_;                        ///< 捕获组文本
    vector<pair<size_t, size_t>> group_positions_; ///< 捕获组位置和长度
    size_t position_ = string::npos;               ///< 匹配起始位置
    size_t length_ = 0;                            ///< 匹配长度
    string subject_;                               ///< 原始字符串

public:
    /**
     * @brief 默认构造函数
     */
    match_result() = default;

    /**
     * @brief 构造函数
     * @param subject 原始字符串
     * @param pos 匹配起始位置
     * @param len 匹配长度
     * @param groups 捕获组文本列表
     * @param group_positions 捕获组位置列表
     */
    match_result(string subject, size_t pos, size_t len, const vector<string>& groups,
                 vector<pair<size_t, size_t>> group_positions);

    /**
     * @brief 检查是否匹配成功
     * @return 匹配成功返回true
     */
    NEFORCE_NODISCARD bool matched() const noexcept { return position_ != string::npos; }

    /**
     * @brief 获取匹配起始位置
     * @return 匹配在字符串中的起始索引
     */
    NEFORCE_NODISCARD size_t position() const noexcept { return position_; }

    /**
     * @brief 获取匹配长度
     * @return 匹配的字符长度
     */
    NEFORCE_NODISCARD size_t length() const noexcept { return length_; }

    /**
     * @brief 获取完整匹配的文本
     * @return 匹配到的完整字符串
     */
    NEFORCE_NODISCARD string_view data() const noexcept { return matched() ? groups_[0].view() : ""; }

    /**
     * @brief 获取捕获组数量
     * @return 捕获组总数（包括第0组完整匹配）
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return groups_.size(); }

    /**
     * @brief 获取指定捕获组
     * @param idx 捕获组索引（0表示完整匹配）
     * @return 捕获组文本，越界返回空字符串
     */
    NEFORCE_NODISCARD string_view operator[](const size_t idx) const noexcept {
        return idx < groups_.size() ? groups_[idx].view() : "";
    }

    /**
     * @brief 获取指定捕获组的位置信息
     * @param idx 捕获组索引
     * @return 包含起始位置和长度的pair
     */
    NEFORCE_NODISCARD pair<size_t, size_t> position(const size_t idx) const noexcept {
        return idx < group_positions_.size() ? group_positions_[idx] : pair<size_t, size_t>{string::npos, 0};
    }

    /**
     * @brief 获取匹配前的前缀
     * @return 匹配位置之前的字符串
     */
    NEFORCE_NODISCARD string_view prefix() const noexcept { return matched() ? subject_.view(0, position_) : ""; }

    /**
     * @brief 获取匹配后的后缀
     * @return 匹配位置之后的字符串
     */
    NEFORCE_NODISCARD string_view suffix() const noexcept {
        return matched() ? subject_.view(position_ + length_) : "";
    }

    /**
     * @brief 格式化替换结果
     * @param fmt 格式化字符串
     * @return 格式化后的字符串
     *
     * 支持的转义序列：
     * - $$: 美元符号
     * - $&: 完整匹配
     * - $`: 前缀
     * - $': 后缀
     * - $n: 第n个捕获组（n为0-9）
     * - ${name}: 命名捕获组
     */
    NEFORCE_NODISCARD string format(string_view fmt) const;

    /**
     * @brief 获取捕获组迭代器起始位置
     * @return 捕获组迭代器
     */
    NEFORCE_NODISCARD iterator begin() const noexcept { return groups_.begin(); }

    /**
     * @brief 获取捕获组迭代器结束位置
     * @return 捕获组迭代器
     */
    NEFORCE_NODISCARD iterator end() const noexcept { return groups_.end(); }
};


class NEFORCE_API regex_iterator;

class NEFORCE_API regex_token_iterator;

/**
 * @class regex
 * @brief 正则表达式类
 *
 * PCRE2正则表达式引擎，提供编译、匹配、查找、替换、分割等操作。
 * 支持正则表达式的完整功能，包括捕获组、向后引用、断言等。
 */
class NEFORCE_API regex {
private:
    struct pcre2_code_deleter {
        void operator()(::pcre2_code* code) const noexcept {
            if (code != nullptr) {
                ::pcre2_code_free(code);
            }
        }
    };

    struct pcre2_match_data_deleter {
        void operator()(::pcre2_match_data* data) const noexcept {
            if (data != nullptr) {
                ::pcre2_match_data_free(data);
            }
        }
    };

    unique_ptr<::pcre2_code, pcre2_code_deleter> code_; ///< PCRE2编译后的正则表达式
    string pattern_;                                    ///< 原始正则表达式模式
    uint32_t options_;                                  ///< 编译选项
    int capture_count_ = 0;                             ///< 捕获组数量

    friend class regex_iterator;
    friend class regex_token_iterator;

private:
    void compile(const string& pattern, uint32_t options = 0);

    match_result do_match(::PCRE2_SPTR subject, size_t length, size_t start_offset, uint32_t options,
                          const string& subject_str) const;

public:
    /**
     * @brief 从字符串构造正则表达式
     * @param pattern 正则表达式模式
     * @param options 编译选项（PCRE2选项标志）
     * @throws regex_exception 编译失败时抛出
     */
    explicit regex(const string& pattern, uint32_t options = 0);

    regex(regex&& other) noexcept;
    regex& operator=(regex&& other) noexcept;

    /**
     * @brief 拷贝构造函数
     * @param other 源正则表达式
     * @throws regex_exception 编译失败时抛出
     */
    regex(const regex& other);

    /**
     * @brief 拷贝赋值运算符
     * @param other 源正则表达式
     * @return 自身引用
     * @throws regex_exception 编译失败时抛出
     */
    regex& operator=(const regex& other);

    /**
     * @brief 执行完整匹配
     * @param str 待匹配字符串
     * @return 匹配结果
     *
     * 要求正则表达式完全匹配整个字符串。
     */
    NEFORCE_NODISCARD match_result do_match(const string& str) const;

    /**
     * @brief 检查是否完全匹配
     * @param str 待匹配字符串
     * @return 完全匹配返回true
     */
    NEFORCE_NODISCARD bool match(const string& str) const;

    /**
     * @brief 在字符串中搜索第一个匹配
     * @param str 待搜索字符串
     * @param pos 起始搜索位置
     * @return 第一个匹配结果
     */
    NEFORCE_NODISCARD match_result search(const string& str, size_t pos = 0) const;

    /**
     * @brief 查找所有匹配
     * @param str 待搜索字符串
     * @return 所有匹配结果列表
     */
    NEFORCE_NODISCARD vector<match_result> find_all(const string& str) const;

    /**
     * @brief 替换第一个匹配
     * @param str 原始字符串
     * @param fmt 替换格式字符串
     * @return 替换后的字符串
     */
    NEFORCE_NODISCARD string replace_first(const string& str, string_view fmt) const;

    /**
     * @brief 替换所有匹配
     * @param str 原始字符串
     * @param fmt 替换格式字符串
     * @return 替换后的字符串
     */
    NEFORCE_NODISCARD string replace_all(const string& str, string_view fmt) const;

    /**
     * @brief 使用回调函数替换所有匹配
     * @param str 原始字符串
     * @param callback 回调函数，接收match_result返回替换字符串
     * @return 替换后的字符串
     */
    NEFORCE_NODISCARD string replace_all_callback(const string& str,
                                                  function<string(const match_result&)> callback) const;

    /**
     * @brief 使用正则表达式分割字符串
     * @param str 待分割字符串
     * @param max_splits 最大分割次数，-1表示无限制
     * @return 分割后的字符串列表
     */
    NEFORCE_NODISCARD vector<string> split(const string& str, int max_splits = -1) const;

    /**
     * @brief 获取捕获组数量
     * @return 正则表达式中定义的捕获组数量
     */
    NEFORCE_NODISCARD int capture_count() const noexcept { return capture_count_; }

    /**
     * @brief 获取正则表达式模式
     * @return 原始正则表达式字符串
     */
    NEFORCE_NODISCARD const string& pattern() const noexcept { return pattern_; }

    /**
     * @brief 检查正则表达式是否有效
     * @return 已编译成功返回true
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return code_ != nullptr; }

    /**
     * @brief 获取匹配结果迭代器起始位置
     * @param str 待遍历的字符串
     * @return 匹配迭代器起始位置
     */
    NEFORCE_NODISCARD regex_iterator begin(const string& str) const;

    /**
     * @brief 获取匹配结果迭代器结束位置
     * @param str 待遍历的字符串
     * @return 匹配迭代器结束位置
     */
    NEFORCE_NODISCARD regex_iterator end(const string& str) const;
};

/**
 * @class regex_iterator
 * @brief 正则表达式匹配迭代器
 *
 * 前向迭代器，惰性遍历字符串中的匹配结果。
 */
class NEFORCE_API regex_iterator {
public:
    using iterator_category = forward_iterator_tag; ///< 迭代器类型
    using value_type = match_result;                ///< 元素类型
    using difference_type = ptrdiff_t;              ///< 差值类型
    using pointer = const match_result*;            ///< 指针类型
    using reference = const match_result&;          ///< 引用类型

private:
    const regex* regex_ = nullptr; ///< 关联的正则表达式
    string subject_;               ///< 待遍历的字符串

    match_result current_; ///< 当前匹配结果
    size_t next_pos_ = 0;  ///< 下一次搜索的起始位置
    bool done_ = true;     ///< 是否已到达末尾

    void find_next();

public:
    /**
     * @brief 默认构造函数，构造结束迭代器
     */
    regex_iterator() = default;

    /**
     * @brief 构造函数
     * @param re 正则表达式对象
     * @param str 待遍历的字符串
     * @param pos 起始位置
     */
    regex_iterator(const regex* re, string str, size_t pos = 0);

    /**
     * @brief 解引用操作符
     * @return 当前匹配结果
     */
    NEFORCE_NODISCARD reference operator*() const noexcept { return current_; }

    /**
     * @brief 成员访问操作符
     * @return 当前匹配结果指针
     */
    NEFORCE_NODISCARD pointer operator->() const noexcept { return &current_; }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器
     */
    regex_iterator& operator++();

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器
     */
    regex_iterator operator++(int) {
        regex_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一个迭代器
     * @return 相等返回true
     */
    NEFORCE_NODISCARD bool operator==(const regex_iterator& other) const noexcept;

    /**
     * @brief 不等比较操作符
     * @param other 另一个迭代器
     * @return 不等返回true
     */
    NEFORCE_NODISCARD bool operator!=(const regex_iterator& other) const noexcept { return !(*this == other); }

    /**
     * @brief 获取起始迭代器
     * @param re 正则表达式对象
     * @param str 待遍历的字符串
     * @return 起始迭代器
     */
    static regex_iterator begin(const regex* re, const string& str) { return {re, str, 0}; }

    /**
     * @brief 获取结束迭代器
     * @param re 正则表达式对象
     * @param str 待遍历的字符串
     * @return 结束迭代器
     */
    static regex_iterator end(const regex* re, const string& str) {
        regex_iterator it;
        it.regex_ = re;
        it.subject_ = str;
        it.done_ = true;
        return it;
    }
};

/**
 * @class regex_token_iterator
 * @brief 正则表达式令牌迭代器
 *
 * 用于遍历字符串中被正则表达式分隔的片段。
 * 可以提取匹配的捕获组或分隔符之间的文本。
 */
class NEFORCE_API regex_token_iterator {
public:
    /**
     * @enum state
     * @brief 迭代器状态枚举
     */
    enum class state {
        BEFORE_FIRST,    ///< 第一个匹配之前
        BETWEEN_MATCHES, ///< 匹配之间
        AFTER_LAST,      ///< 最后一个匹配之后
        END              ///< 结束
    };

private:
    const regex* regex_ = nullptr;  ///< 关联的正则表达式
    string subject_;                ///< 待遍历的字符串
    regex_iterator match_iterator_; ///< 匹配迭代器
    regex_iterator end_iterator_;   ///< 结束迭代器
    string current_;                ///< 当前令牌
    int index_ = 0;                 ///< 捕获组索引（负数表示分隔符模式）
    state state_ = state::END;      ///< 当前状态
    size_t last_pos_ = 0;           ///< 最后处理位置

private:
    void find_next();

public:
    /**
     * @brief 默认构造函数
     */
    regex_token_iterator() = default;

    /**
     * @brief 构造函数
     * @param re 正则表达式对象
     * @param str 待遍历的字符串
     * @param index 捕获组索引（-1表示分隔符模式，0表示完整匹配）
     */
    regex_token_iterator(const regex* re, string str, int index = 0);

    /**
     * @brief 解引用操作符
     * @return 当前令牌字符串
     */
    NEFORCE_NODISCARD string_view operator*() const noexcept { return current_.view(); }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器
     */
    regex_token_iterator& operator++();

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器
     */
    regex_token_iterator operator++(int) {
        regex_token_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一个迭代器
     * @return 相等返回true
     */
    NEFORCE_NODISCARD bool operator==(const regex_token_iterator& other) const noexcept;

    /**
     * @brief 不等比较操作符
     * @param other 另一个迭代器
     * @return 不等返回true
     */
    NEFORCE_NODISCARD bool operator!=(const regex_token_iterator& other) const noexcept { return !(*this == other); }
};

/// @cond
inline regex_iterator regex::begin(const string& str) const { return regex_iterator::begin(this, str); }

inline regex_iterator regex::end(const string& str) const { return regex_iterator::end(this, str); }
/// @endcond

/** @} */ // Regex

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_HPP__
