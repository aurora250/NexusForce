#ifndef NEFORCE_CORE_FILE_TOML_TOML_PARSER_HPP__
#define NEFORCE_CORE_FILE_TOML_TOML_PARSER_HPP__

/**
 * @file toml_parser.hpp
 * @brief toml配置文件解析器
 *
 * 此文件提供了toml格式配置的解析器实现。
 * 提供完整的语法分析和错误处理机制。
 *
 * 支持toml v1.0.0规范的所有特性：
 * - 键值对（Key/Value pairs）
 * - 字符串（Basic, Literal, Multi-line）
 * - 数字（Integer, Float, Special floats）
 * - 布尔值（true/false）
 * - 日期时间（Offset, Local, Date, Time）
 * - 数组（Arrays）
 * - 内联表格（Inline tables）
 * - 标准表格（Standard tables）
 * - 表格数组（Arrays of tables）
 * - 注释和空白处理
 */

#include "NeForce/core/file/toml/toml_value.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup TomlConfig toml配置
 * @brief toml配置格式管理
 * @{
 */

/**
 * @class toml_parser
 * @brief toml配置解析器
 *
 * 采用递归下降解析算法。
 * 解析过程会维护行号和列号信息，便于错误定位。
 */
class NEFORCE_API toml_parser {
private:
    /**
     * @struct context
     * @brief 解析上下文结构
     *
     * 保存表格解析的上下文信息，用于表格数组的嵌套管理。
     */
    struct context {
        toml_table* table;      ///< 当前表格指针
        vector<string> path;    ///< 当前路径
    };

    vector<context> context_stack_;  ///< 上下文栈
    bool is_in_array_table_ = false; ///< 是否在表格数组中

    string text_;        ///< 待解析的toml文本
    size_t len_;         ///< 文本长度
    size_t pos_ = 0;     ///< 当前解析位置
    size_t line_ = 1;    ///< 当前行号
    size_t column_ = 1;  ///< 当前列号

    unique_ptr<toml_table> root_;  ///< 解析结果根表格
    toml_table* ctb_ = nullptr;    ///< 当前表格指针
    vector<string> ctp_;           ///< 当前表格路径

    /**
     * @brief 跳过空白字符
     *
     * 跳过空格和制表符。
     */
    void skip_whitespace() noexcept;

    /**
     * @brief 跳过注释
     *
     * 跳过以#开头的注释直到行尾。
     */
    void skip_comment() noexcept;

    /**
     * @brief 跳过空白和注释
     *
     * 连续跳过空白字符、注释和换行符。
     */
    void skip_whitespace_and_comments() noexcept;

    /**
     * @brief 跳过换行符
     *
     * 跳过连续的换行符。
     */
    void skip_newlines() noexcept;

    /**
     * @brief 跳过空白但不包括换行符
     *
     * 只跳过空格和制表符，不跳过换行符。
     */
    void skip_whitespace_no_newline() noexcept;

    /**
     * @brief 获取当前字符
     * @return 当前位置的字符，若已到末尾返回'\0'
     */
    char current() const noexcept;

    /**
     * @brief 预取字符
     * @param offset 向前偏移量，默认为1
     * @return 指定偏移位置的字符
     */
    char peek(size_t offset = 1) const noexcept;

    /**
     * @brief 检查是否已到末尾
     * @return 是否已到文件末尾
     */
    bool eof() const noexcept;

    /**
     * @brief 前进一个字符
     *
     * 更新位置信息，处理换行符的行列计数。
     */
    void advance() noexcept;

    /**
     * @brief 期望指定字符
     * @param ch 期望的字符
     * @return 是否成功匹配
     * @throws toml_exception 当字符不匹配时抛出
     *
     * 如果当前字符与期望字符匹配则前进并返回true，否则抛出异常。
     */
    bool expect(char ch);

    /**
     * @brief 尝试匹配指定字符
     * @param ch 要匹配的字符
     * @return 是否匹配成功
     *
     * 如果当前字符与指定字符匹配则前进并返回true，否则返回false。
     */
    bool match(char ch) noexcept;

    /**
     * @brief 抛出解析错误
     * @param message 错误信息
     * @throws toml_exception 始终抛出异常
     *
     * 构造包含行号和列号的错误信息并抛出异常。
     */
    void throw_parse_error(string message) const;

    /**
     * @brief 解析Unicode转义序列
     * @param digits 十六进制位数（4或8）
     * @return Unicode码点
     *
     * 解析Unicode转义序列。
     *
     * @throws toml_exception 当转义序列无效时抛出
     */
    char32_t parse_unicode_escape(size_t digits);

    /**
     * @brief 解析字符串
     * @return 解析得到的字符串值
     *
     * 根据引号类型分派到具体的解析方法。
     */
    unique_ptr<toml_string> parse_string();

    /**
     * @brief 解析基本字符串
     * @return 解析得到的字符串值
     *
     * 格式："string"，支持转义序列。
     */
    unique_ptr<toml_string> parse_basic_string();

    /**
     * @brief 解析字面量字符串
     * @return 解析得到的字符串值
     *
     * 格式：'string'，不支持转义。
     */
    unique_ptr<toml_string> parse_literal_string();

    /**
     * @brief 解析多行基本字符串
     * @return 解析得到的字符串值
     *
     * 格式："""string""" ，支持转义和行延续。
     */
    unique_ptr<toml_string> parse_multiline_basic_string();

    /**
     * @brief 解析多行字面量字符串
     * @return 解析得到的字符串值
     *
     * 格式：'''string'''，不支持转义。
     */
    unique_ptr<toml_string> parse_multiline_literal_string();

    /**
     * @brief 解析数字
     * @return 解析得到的数字值（整数或浮点数）
     *
     * 支持整数、浮点数和特殊浮点数（inf, nan）。
     */
    unique_ptr<toml_value> parse_number();

    /**
     * @brief 解析整数
     * @param base 进制基数（2, 8, 10, 16）
     * @return 解析得到的整数值
     */
    unique_ptr<toml_integer> parse_integer(int base = 10);

    /**
     * @brief 解析布尔值
     * @return 解析得到的布尔值
     */
    unique_ptr<toml_boolean> parse_boolean();

    /**
     * @brief 解析日期时间
     * @return 解析得到的日期时间值
     *
     * 支持四种日期时间格式的自动识别。
     */
    unique_ptr<toml_datetime> parse_datetime();

    /**
     * @brief 解析数组
     * @return 解析得到的数组
     *
     * 格式：[value1, value2, ...]
     * 要求所有元素类型一致。
     */
    unique_ptr<toml_array> parse_array();

    /**
     * @brief 解析内联表格
     * @return 解析得到的内联表格
     *
     * 格式：{ key = value, key2 = value2 }
     * 内联表格不能包含换行符。
     */
    unique_ptr<toml_table> parse_inline_table();

    /**
     * @brief 解析键名
     * @return 键名字符串
     *
     * 根据当前字符决定解析裸键或引号键。
     */
    string parse_key();

    /**
     * @brief 解析裸键
     * @return 裸键字符串
     *
     * 格式：[A-Za-z0-9_-]+
     */
    string parse_bare_key();

    /**
     * @brief 解析引号键
     * @return 引号键字符串
     *
     * 使用字符串解析规则。
     */
    string parse_quoted_key();

    /**
     * @brief 解析点分隔键路径
     * @return 键路径向量
     *
     * 格式：key1.key2.key3
     */
    vector<string> parse_dotted_key();

    /**
     * @brief 解析任意值
     * @return 解析得到的值
     *
     * 根据当前字符决定解析的具体类型。
     */
    unique_ptr<toml_value> parse_value();

    /**
     * @brief 解析键值对
     *
     * 解析格式：key = value
     * 支持点分隔键路径。
     */
    void parse_key_value();

    /**
     * @brief 解析表格头
     *
     * 格式：[table.path]
     */
    void parse_table_header();

    /**
     * @brief 解析表格数组头
     *
     * 格式：[[array.table.path]]
     */
    void parse_array_table_header();

    /**
     * @brief 获取或创建表格
     * @param path 表格路径
     * @return 表格指针
     *
     * 沿路径获取表格，如果不存在则创建。
     */
    toml_table* get_or_create_table(const vector<string>& path) const;

    /**
     * @brief 导航到表格
     * @param path 表格路径
     * @return 表格指针，不存在返回nullptr
     */
    toml_table* navigate_to_table(const vector<string>& path) const;

    /**
     * @brief 设置当前表格
     * @param path 表格路径
     *
     * 将当前表格设置为指定路径的表格，必要时创建。
     */
    void set_current_table(const vector<string>& path);

public:
    /**
     * @brief 构造函数
     * @param text 待解析的toml文本
     *
     * 初始化解析器，创建空的根表格。
     */
    explicit toml_parser(string text) noexcept
    : text_(_NEFORCE move(text)), len_(text_.size()) {
        root_ = make_unique<toml_table>();
        ctb_ = root_.get();
    }

    /**
     * @brief 执行解析
     * @return 解析完成的根表格
     * @throws toml_exception 当解析过程中遇到语法错误时抛出
     *
     * 解析整个toml文档，构建完整的表格结构。
     */
    unique_ptr<toml_table> parse();

    /**
     * @brief 尝试执行解析
     * @return 解析结果的可选对象
     *
     * 如果解析成功返回包含根表格的optional，
     * 如果解析失败返回空的optional。
     */
    optional<unique_ptr<toml_table>> try_parse();
};

/** @} */ // TomlConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TOML_TOML_PARSER_HPP__
