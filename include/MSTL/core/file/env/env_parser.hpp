#ifndef MSTL_CORE_FILE_ENV_ENV_PARSER_HPP__
#define MSTL_CORE_FILE_ENV_ENV_PARSER_HPP__

/**
 * @file env_parser.hpp
 * @brief env配置格式解析器
 *
 * 此文件提供了env配置格式的解析器实现。
 * 支持解析env格式，包括变量定义、注释、引号处理和转义字符。
 * 提供完整的语法分析和错误处理机制。
 */

#include "MSTL/core/file/env/env_value.hpp"
#include "MSTL/core/utility/optional.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup EnvConfig env配置
 * @brief env配置格式管理
 * @{
 */

/**
 * @class env_parser
 * @brief 环境变量文件解析器
 *
 * 解析env格式，支持以下特性：
 * - 变量定义（KEY=VALUE格式）
 * - 注释（以#开头的行）
 * - 引号处理（单引号、双引号）
 * - 转义字符处理
 * - export关键字支持
 * - 空白行处理
 *
 * 解析过程会维护行号和列号信息，便于错误定位。
 */
class MSTL_API env_parser {
private:
    string text_;       ///< 待解析的文本内容
    size_t len_;        ///< 文本长度
    size_t pos_ = 0;    ///< 当前解析位置
    size_t line_ = 1;   ///< 当前行号
    size_t column_ = 1; ///< 当前列号

    unique_ptr<env_document> root_; ///< 解析结果文档

    /**
     * @brief 跳过空白字符
     *
     * 跳过空格和制表符，更新位置信息。
     */
    void skip_whitespace() noexcept;

    /**
     * @brief 跳过当前行
     *
     * 从当前位置跳过直到行尾，包括换行符。
     */
    void skip_line() noexcept;

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
     * @brief 判断是否为注释行
     * @param line 要检查的行内容
     * @return 是否为注释行
     */
    bool is_comment_line(const string& line) const noexcept;

    /**
     * @brief 判断是否为空白行
     * @param line 要检查的行内容
     * @return 是否为空白行
     */
    bool is_blank_line(const string& line) const noexcept;

    /**
     * @brief 解析变量行
     * @param line 当前行内容
     * @param name [输出] 解析出的变量名
     * @param variable [输出] 解析出的变量对象
     * @return 解析是否成功
     *
     * @throws env_exception 当变量格式错误时抛出
     */
    bool parse_variable_line(
        const string& line, string& name,
        unique_ptr<env_variable>& variable) const;

    /**
     * @brief 解析无引号的值
     * @param line 当前行内容
     * @param pos 解析位置
     * @return 解析出的值字符串
     *
     * 无引号值在遇到空格、制表符或注释符号时结束。
     */
    string parse_unquoted_value(const string& line, size_t& pos) const;

    /**
     * @brief 解析单引号包裹的值
     * @param line 当前行内容
     * @param pos 解析位置
     * @return 解析出的值字符串
     *
     * 支持单引号内的转义：''' 表示一个单引号字符。
     */
    string parse_single_quoted_value(const string& line, size_t& pos) const;

    /**
     * @brief 解析双引号包裹的值
     * @param line 当前行内容
     * @param pos 解析位置
     * @return 解析出的值字符串
     *
     * 支持转义序列
     */
    string parse_double_quoted_value(const string& line, size_t& pos) const;

    /**
     * @brief 解析单行内容
     * @param line 要解析的行
     *
     * 根据行类型进行相应处理。
     */
    void parse_line(const string& line) const;

public:
    /**
     * @brief 构造函数
     * @param text 待解析的文本内容
     *
     * 初始化解析器并创建空的文档对象。
     */
    explicit env_parser(string text) noexcept
    : text_(_MSTL move(text)), len_(text_.size()) {
        root_ = make_unique<env_document>();
    }

    /**
     * @brief 执行解析
     * @return 解析完成的文档对象
     *
     * 逐行解析整个文本内容。
     *
     * @throws env_exception 当解析过程中遇到语法错误时抛出
     */
    unique_ptr<env_document> parse();

    /**
     * @brief 尝试执行解析
     * @return 解析结果的可选对象
     *
     * 如果解析成功返回包含文档对象的optional，
     * 如果解析失败返回空的optional。
     */
    optional<unique_ptr<env_document>> try_parse();
};

/** @} */ // EnvConfig

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_ENV_ENV_PARSER_HPP__
