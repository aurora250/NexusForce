#ifndef NEFORCE_CORE_STRING_REGEX_HPP__
#define NEFORCE_CORE_STRING_REGEX_HPP__

/**
 * @file regex.hpp
 * @brief 正则表达式引擎封装
 *
 * 此文件提供了内置正则引擎的封装。
 * 支持正则表达式的编译、匹配、查找、替换、分割等操作，
 * 并提供迭代器接口用于遍历匹配结果。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/core/string/string.hpp"
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
NEFORCE_ERROR_BUILD_FINAL_CLASS(regex_exception, value_exception, "Regex Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup Regex 正则表达式
 * @brief 正则表达式操作实现
 *
 * 提供一个自包含的回溯式正则引擎。
 *
 * 模式在构造时编译为字节码，匹配在显式栈上执行。
 * 引擎直接处理 UTF-8 字节序列，提供编译、匹配、查找、替换、分割与遍历迭代器。
 *
 * @section standards 参照的国际标准
 *
 * **正则语法产生式：**
 * - **ECMA-262 §22.2**：ECMAScript Pattern（Disjunction / Alternative / Term / Assertion / Quantifier / CharacterClass / AtomEscape）
 *   https://tc39.es/ecma262/#sec-patterns
 * - **IEEE POSIX.1-2017 (ISO/IEC 9945) §9**：BRE / ERE 的元字符、括号表达式与区间
 *   https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html
 *
 * **Unicode 支持：**
 * - **Unicode UTS #18**：Unicode Regular Expressions（Level 1 字符类与 \\p{...}）
 *   https://www.unicode.org/reports/tr18/
 * - **Unicode UAX #29**：Unicode Text Segmentation（\\X 扩展字素簇）
 *   https://www.unicode.org/reports/tr29/
 * - **Unicode UAX #44**：Unicode Character Database（通用类别）
 *   https://www.unicode.org/reports/tr44/
 *
 * **文本编码：**
 * - **IETF RFC 3629**：UTF-8 的字节序列与合法性约束
 *   https://www.rfc-editor.org/rfc/rfc3629.html
 *
 * @section support 支持的语法
 *
 * **ECMAScript Pattern（ECMA-262 §22.2）**
 * - 字面量、`.`、`^ $`、`\\b` `\\B`、分支 `|`
 * - 字符类 `[...]`：区间、取反、`]` 作为首字符、`\\d \\D \\w \\W \\s \\S`
 * - 捕获组 `(...)`、非捕获组 `(?:...)`、命名组 `(?<name>...)`
 * - 前瞻 `(?=...)` `(?!...)`、后顾 `(?<=...)` `(?<!...)`
 * - 量词 `* + ? {n} {n,} {n,m}` 及其惰性形式（尾部 `?`）
 * - 反向引用 `\\1`..`\\9`、`\\k<name>`
 * - 属性转义 `\\p{Lu}` `\\P{L}`（Unicode 通用类别与少量脚本）
 * - 转义 `\\n \\r \\t \\f \\v \\0nn \\xHH \\uHHHH \\cX`
 *
 * **POSIX ERE（ISO/IEC 9945 §9.4）**
 * - 括号表达式与区间、`[:alpha:]` `[:digit:]` 等字符类、`^ $` 锚点、`|` 分支
 * - 差异：启用 `utf` 后 `.` 与字符类按码位而不是字节匹配
 *
 * **Unicode（UTS #18 Level 1 / UAX #29）**
 * - `\\p{L}` 等 30 个通用类别；`\\p{Han}` `\\p{Hiragana}` `\\p{Katakana}` `\\p{Hangul}`
 * - 属性别名 `\\p{Any}` `\\p{ASCII}` `\\p{Alnum}` `\\p{Word}` `\\p{Space}` 等
 * - `\\X` 扩展字素簇：组合符、ZWJ 序列、CRLF、Hangul 音节
 * - 字符数据版本 Unicode 13.0.0，由 `scripts/generate_regex_ucd.py` 生成
 *
 * **Perl 扩展（不属于标准）**
 * - `(?|...)` 分支重置、`(?>...)` 原子组、`(?R)` / `(?N)` 递归、`(?&name)` 子程序
 * - 占有量词 `*+ ++ ?+ {n,m}+`、`\\Q...\\E` 字面量引用
 * - 内联选项 `(?imsxU)`、`(?i:...)`、`(?-i)`
 * - `\\A \\z \\Z` 锚点、`\\G`（搜索起点锚点）、`\\K`（重置匹配起点）
 * - `\\R` 通用换行：CRLF 作为一个整体，或 LF / VT / FF / CR / NEL / LS / PS
 * - 条件组 `(?(1)yes|no)`、`(?(name)yes|no)`、`(?(DEFINE)...)` 子程序定义块
 *
 * @section unsupported 未实现的构造
 *
 * 下列语法在编译期抛出 `regex_exception`，不会静默降级为其他语义：
 *
 * - 回溯控制动词 `(*SKIP)`、`(*PRUNE)`、`(*THEN)`、`(*COMMIT)`
 * - `\\C`（单字节）、`\\N{U+XXXX}`（按码位字面量）
 * - callout `(?C...)`
 * - 断言作为条件的条件组 `(?(?=...)yes|no)`
 * - 长度无界的后顾断言
 *
 * @section limits 运行期约束
 *
 * - 单次匹配尝试的回溯上限 1,000,000 步、整次调用的总上限 20,000,000 步、递归深度上限 1000 层，超出时抛出 `regex_exception`
 * - 编译期自动占有化：贪婪量词后接的字符集与量词原子不相交时，转换为原子量词
 * - 无法占有化的嵌套量词会触发指数级回溯，优先使用锚点与具体字符类缩窄起点，或改写为等价的无嵌套形式，可用 regex::analysis() 在注册期识别这类模式
 * - 后顾断言的匹配窗口上限 32767 字节
 * - 位置与长度以 UTF-8 **字节**为单位，不是码位索引
 * @{
 */

class NEFORCE_API regex_iterator;
class NEFORCE_API regex_token_iterator;

/**
 * @class match_result
 * @brief 正则表达式匹配结果类
 *
 * 存储一次正则表达式匹配的结果，包括匹配到的文本、捕获组、
 * 匹配位置等信息。支持格式化输出和迭代器访问。
 *
 * @warning 调用方必须保证该字符串在结果使用期间保持存活且未被修改。
 */
class NEFORCE_API match_result {
public:
    using iterator = vector<string>::const_iterator; ///< 捕获组迭代器类型

private:
    friend class regex;
    friend class regex_iterator;
    friend class regex_token_iterator;

    vector<string> groups_;                        ///< 捕获组文本
    vector<pair<size_t, size_t>> group_positions_; ///< 捕获组位置和长度
    size_t position_ = string::npos;               ///< 匹配起始位置
    size_t length_ = 0;                            ///< 匹配长度
    shared_ptr<string> owner_;                     ///< 原始字符串所有者
    string_view subject_;                          ///< 原始字符串视图

public:
    /**
     * @brief 默认构造函数
     */
    match_result() = default;

    /**
     * @brief 原始字符串构造函数
     * @param subject 原始字符串
     * @param pos 匹配起始位置
     * @param len 匹配长度
     * @param groups 捕获组文本列表
     * @param group_positions 捕获组位置列表
     */
    match_result(const string& subject, size_t pos, size_t len, vector<string> groups,
                 vector<pair<size_t, size_t>> group_positions);

    /**
     * @brief 字符串视图构造函数
     * @param subject 字符串视图
     * @param pos 匹配起始位置
     * @param len 匹配长度
     * @param groups 捕获组文本列表
     * @param group_positions 捕获组位置列表
     */
    match_result(string_view subject, size_t pos, size_t len, vector<string> groups,
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
    NEFORCE_NODISCARD string_view data() const noexcept { return matched() ? subject_.view(position_, length_) : ""; }

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

private:
    void attach_owner(const string& subject);
    void rebind(string_view subject) noexcept { subject_ = subject; }
};


class NEFORCE_API regex_iterator;

class NEFORCE_API regex_token_iterator;


/**
 * @enum regex_option
 * @brief 正则表达式选项枚举
 */
enum class regex_option : uint32_t {
    none = 0x00000000U,                ///< 无选项
    allow_empty_class = 0x00000001U,   ///< 允许空字符类 []，空类不匹配任何字符
    alt_bsux = 0x00000002U,            ///< 允许 \\uHHHH 形式的 Unicode 转义（ECMAScript 风格）
    caseless = 0x00000008U,            ///< 大小写不敏感匹配
    dollar_endonly = 0x00000010U,      ///< $ 仅匹配字符串末尾，不匹配末尾换行符之前
    dotall = 0x00000020U,              ///< 点号 . 匹配任意字符，包括换行符
    dupnames = 0x00000040U,            ///< 允许重复的命名捕获组
    extended = 0x00000080U,            ///< 扩展模式，忽略空白与 # 注释
    firstline = 0x00000100U,           ///< 仅在第一行内进行匹配
    match_unset_backref = 0x00000200U, ///< 反向引用指向未参与匹配的组时按空串处理
    multiline = 0x00000400U,           ///< 多行模式，^ 和 $ 匹配行首行尾
    no_auto_capture = 0x00002000U,     ///< 禁止自动捕获组，() 不作为捕获组
    ucp = 0x00020000U,                 ///< 启用 Unicode 属性语义（\\d \\w \\s 与大小写折叠）
    ungreedy = 0x00040000U,            ///< 量词默认非贪婪模式
    utf = 0x00080000U,                 ///< 启用 UTF-8 模式，按码位匹配
    alt_circumflex = 0x00200000U,      ///< 多行模式下 ^ 仅匹配字符串开头
    literal = 0x02000000U,             ///< 将整个模式视为字面量字符串
};

NEFORCE_NODISCARD constexpr regex_option operator|(regex_option lhs, regex_option rhs) noexcept {
    return static_cast<regex_option>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

NEFORCE_NODISCARD constexpr regex_option operator&(regex_option lhs, regex_option rhs) noexcept {
    return static_cast<regex_option>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}


/**
 * @class regex
 * @brief 正则表达式类
 *
 * 内置回溯式正则引擎，提供编译、匹配、查找、替换、分割等操作。
 *
 * @note 匹配采用左最先（leftmost-first）回溯语义，不提供 POSIX 的最长匹配（leftmost-longest）语义。
 * @note `$` 默认匹配串尾或串尾换行符之前；需要严格串尾语义时使用 `regex_option::dollar_endonly`。
 */
class NEFORCE_API regex {
public:
    /**
     * @struct analysis_result
     * @brief 正则模式的结构分析结果
     *
     * 在编译期计算，用于调用方决定是否接受不受信任的模式。
     */
    struct analysis_result {
        bool has_nested_quantifier = false;       ///< 是否存在量词嵌套量词的结构
        bool may_backtrack_exponentially = false; ///< 嵌套结构是否可能触发指数级回溯
    };

private:
    void* code_{nullptr};                      ///< 正则表达式
    string pattern_;                           ///< 原始正则表达式模式
    regex_option options_{regex_option::none}; ///< 编译选项
    analysis_result analysis_{};               ///< 编译期结构分析
    int capture_count_ = 0;                    ///< 捕获组数量

    friend class regex_iterator;
    friend class regex_token_iterator;

private:
    void compile(const string& pattern, regex_option options);

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    match_result do_match(const void* subject, size_t length, size_t start_offset, uint32_t options,
                          const string& subject_str) const;

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    match_result do_match_borrowed(const void* subject, size_t length, size_t start_offset, uint32_t options,
                                   string_view subject_view) const;

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    match_result do_match_owned(const shared_ptr<string>& owner, size_t length, size_t start_offset,
                                uint32_t options) const;

public:
    /**
     * @brief 从字符串构造正则表达式
     * @param pattern 正则表达式模式
     * @param options 编译选项
     * @throws regex_exception 编译失败时抛出
     */
    explicit regex(const string& pattern, regex_option options = regex_option::none);

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

    ~regex();

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
     * @brief 获取模式的结构分析结果
     * @return 编译期计算的分析结果
     */
    NEFORCE_NODISCARD const analysis_result& analysis() const noexcept { return analysis_; }

    /**
     * @brief 按名称查找命名捕获组的编号
     * @param name 捕获组名称
     * @return 捕获组编号（从 1 开始），未找到或对象未编译时返回 -1
     */
    NEFORCE_NODISCARD int group_index(string_view name) const noexcept;

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

    /**
     * @brief 重置正则表达式指针
     */
    void reset() noexcept;
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
    shared_ptr<string> subject_;   ///< 待遍历的字符串

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
        it.subject_ = make_shared<string>(str);
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
private:
    /**
     * @enum state
     * @brief 迭代器状态枚举
     */
    enum class state {
        before_first,    ///< 第一个匹配之前
        between_matches, ///< 匹配之间
        after_last,      ///< 最后一个匹配之后
        end              ///< 结束
    };

    const regex* regex_ = nullptr;  ///< 关联的正则表达式
    string subject_;                ///< 待遍历的字符串
    regex_iterator match_iterator_; ///< 匹配迭代器
    regex_iterator end_iterator_;   ///< 结束迭代器
    string current_;                ///< 当前令牌
    int index_ = 0;                 ///< 捕获组索引（负数表示分隔符模式）
    state state_ = state::end;      ///< 当前状态
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
