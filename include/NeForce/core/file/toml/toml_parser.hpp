#ifndef NEFORCE_CORE_FILE_TOML_TOML_PARSER_HPP__
#define NEFORCE_CORE_FILE_TOML_TOML_PARSER_HPP__

/**
 * @file toml_parser.hpp
 * @brief TOML配置文件解析器
 *
 * 完全支持 TOML v1.0.0 规范：
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
#include "NeForce/core/string/codepoint.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup TomlConfig TOML配置
 * @{
 */

/**
 * @class toml_parser
 * @brief TOML配置解析器
 *
 * 解析错误会携带出错位置的行号与列号。
 */
class NEFORCE_API toml_parser {
public:
    /**
     * @brief 构造零拷贝解析器
     * @param text 输入文本
     * @note text 生命周期需覆盖文档使用期
     */
    explicit toml_parser(const string_view text) noexcept :
    text_(text) {}

    /**
     * @brief 构造自包含解析器
     * @param text 输入文本
     */
    explicit toml_parser(string text) noexcept :
    owned_(_NEFORCE move(text)),
    text_(owned_.data(), owned_.size()),
    owning_(true) {}

    /**
     * @brief 构造零拷贝解析器
     * @param text 以 '\\0' 结尾的输入文本
     * @note text 生命周期需覆盖文档使用期
     */
    explicit toml_parser(const char* text) noexcept :
    text_(string_view(text)) {}

    ~toml_parser() = default;

    toml_parser(const toml_parser&) = delete;
    toml_parser& operator=(const toml_parser&) = delete;

    /**
     * @brief 执行解析
     * @return 解析完成的文档，根值为表格
     * @throws toml_exception 当解析过程中遇到语法错误时抛出
     */
    toml_document parse();

    /**
     * @brief 尝试执行解析
     * @return 成功时返回文档，失败时返回空
     */
    optional<toml_document> try_parse() noexcept;

private:
    NEFORCE_NODISCARD char current() const noexcept { return cursor_ < end_ ? *cursor_ : '\0'; }

    NEFORCE_NODISCARD char peek(const size_t offset = 1) const noexcept {
        return compare_greater(end_ - cursor_, offset) ? cursor_[offset] : '\0';
    }

    NEFORCE_NODISCARD bool eof() const noexcept { return cursor_ >= end_; }

    void advance() noexcept { ++cursor_; }

    void advance_bulk(const size_t count) noexcept { cursor_ += count; }

    void skip_whitespace() noexcept;

    void skip_comment() noexcept;

    void skip_whitespace_and_comments() noexcept;

    void skip_whitespace_no_newline() noexcept;

    bool expect(char ch);

    void throw_parse_error(string message) const;

    NEFORCE_NODISCARD codepoint parse_unicode_escape(size_t digits);

    NEFORCE_NODISCARD toml_value parse_string(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_basic_string(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_basic_escaped(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_multiline_basic_escaped(toml_document& doc);

    NEFORCE_NODISCARD toml_value make_text(toml_document& doc, string_view text, toml_string_type type);

    NEFORCE_NODISCARD toml_value parse_literal_string(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_multiline_basic_string(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_multiline_literal_string(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_number();

    NEFORCE_NODISCARD toml_value parse_integer(int base);

    NEFORCE_NODISCARD toml_value parse_boolean();

    NEFORCE_NODISCARD toml_value parse_datetime(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_array(toml_document& doc);

    NEFORCE_NODISCARD toml_value parse_inline_table(toml_document& doc);

    NEFORCE_NODISCARD string_view parse_key(toml_document& doc);

    NEFORCE_NODISCARD string_view parse_bare_key();

    NEFORCE_NODISCARD string_view parse_quoted_key(toml_document& doc);

    void parse_dotted_key(toml_document& doc, vector<string_view>& path);

    NEFORCE_NODISCARD toml_value parse_value(toml_document& doc);

    void parse_key_value(toml_document& doc);

    void parse_table_header(toml_document& doc);

    void parse_array_table_header(toml_document& doc);

    NEFORCE_NODISCARD toml_value* get_or_create_table(toml_document& doc, toml_value* base, const string_view* path,
                                                      size_t count);

    void set_current_table(toml_document& doc, const vector<string_view>& path);

    NEFORCE_NODISCARD string_view clean_number(string_view raw);

    NEFORCE_NODISCARD static bool same(const char* cursor, const char* end, const char* text, size_t size) noexcept;

    vector<toml_value*> context_stack_; ///< 表格数组的上下文栈
    bool is_in_array_table_ = false;    ///< 是否在表格数组中

    string owned_;          ///< 自包含模式下的输入副本
    string_view text_;      ///< 待解析的TOML文本视图
    string decode_scratch_; ///< 转义解码缓冲区
    string number_scratch_; ///< 数字去下划线缓冲区

    const char* begin_ = nullptr;  ///< 文本起始位置
    const char* cursor_ = nullptr; ///< 当前解析位置
    const char* end_ = nullptr;    ///< 文本结束位置

    toml_value* current_table_ = nullptr; ///< 当前表格指针
    vector<string_view> key_path_;        ///< 键路径复用缓冲
    bool owning_ = false;                 ///< 是否为自包含模式
};

/** @} */ // TomlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TOML_TOML_PARSER_HPP__
