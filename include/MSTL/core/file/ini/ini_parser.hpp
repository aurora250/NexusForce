#ifndef MSTL_CORE_FILE_INI_INI_PARSER_HPP__
#define MSTL_CORE_FILE_INI_INI_PARSER_HPP__

/**
 * @file ini_parser.hpp
 * @brief ini配置解析器
 *
 * 此文件提供了ini配置格式的解析器实现。
 * 支持解析ini格式，包括节(section)、键值对、注释等元素。
 * 提供完整的语法分析和错误处理机制。
 */

#include "MSTL/core/file/ini/ini_value.hpp"
#include "MSTL/core/utility/optional.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup IniConfig ini配置
 * @brief ini配置格式管理
 * @{
 */

/**
 * @class ini_parser
 * @brief ini配置解析器
 *
 * 解析ini格式，支持以下特性：
 * - 节定义（[section]格式）
 * - 键值对（key=value格式）
 * - 注释（支持;和#开头的行）
 * - 空白行处理
 * - 引号包裹的值
 * - 值前后的空白字符修剪
 *
 * 解析过程会维护行号和列号信息，便于错误定位。
 */
class MSTL_API ini_parser {
private:
    string text_;           ///< 待解析的文本内容
    size_t len_;            ///< 文本长度
    size_t pos_ = 0;        ///< 当前解析位置
    size_t line_ = 1;       ///< 当前行号
    size_t column_ = 1;     ///< 当前列号

    unique_ptr<ini_document> root_;  ///< 解析结果文档
    ini_section* current_section_ = nullptr;  ///< 当前正在解析的节

    /**
     * @brief 跳过空白字符
     *
     * 跳过空格、制表符等空白字符，更新位置信息。
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
     * @return 是否为注释行（空行或以;、#开头的行）
     */
    bool is_comment_line(const string& line) const noexcept;

    /**
     * @brief 判断是否为节定义行
     * @param line 要检查的行内容
     * @param section_name [输出] 解析出的节名称
     * @return 是否为节定义行
     *
     * 节定义格式：[section_name]
     * 节名称会去除首尾空白字符。
     */
    bool is_section_line(const string& line, string& section_name) const;

    /**
     * @brief 解析键值对
     * @param line 要解析的行内容
     * @param key [输出] 解析出的键名
     * @param value [输出] 解析出的值
     * @return 解析是否成功
     *
     * 解析格式：key = value
     * 键和值都会去除首尾空白字符。
     * 如果值被引号包裹（单引号或双引号），会自动去除引号。
     */
    bool parse_key_value(const string& line, string& key, string& value) const;

    /**
     * @brief 解析单行内容
     * @param line 要解析的行
     * @throws ini_exception 当解析遇到语法错误时抛出
     *
     * 根据行类型（注释、节定义、键值对）进行相应处理。
     * 如果是节定义，创建新节并切换当前节；
     * 如果是键值对，添加到当前节中。
     */
    void parse_line(const string& line);

public:
    /**
     * @brief 构造函数
     * @param text 待解析的文本内容
     *
     * 初始化解析器，创建空的文档对象，并将当前节设置为全局节。
     */
    explicit ini_parser(string text)
    : text_(_MSTL move(text)), len_(text_.size()) {
        root_ = make_unique<ini_document>();
        current_section_ = root_->get_global_section();
    }

    /**
     * @brief 执行解析
     * @return 解析完成的文档对象
     * @throws ini_exception 当解析遇到语法错误时抛出
     *
     * 逐行解析整个文本内容，构建ini文档结构。
     * 解析过程中遇到错误会抛出异常。
     */
    unique_ptr<ini_document> parse();

    /**
     * @brief 尝试执行解析
     * @return 解析结果的可选对象
     *
     * 如果解析成功返回包含文档对象的optional，
     * 如果解析失败返回空的optional。
     */
    optional<unique_ptr<ini_document>> try_parse();
};

/** @} */ // IniConfig

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_INI_INI_PARSER_HPP__
