#ifndef NEFORCE_CORE_FILE_YAML_YAML_PARSER_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_PARSER_HPP__

/**
 * @file yaml_parser.hpp
 * @brief YAML 1.2 解析器
 *
 * 此文件提供了 YAML（YAML Ain't Markup Language）格式的递归下降解析器实现。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/yaml/yaml_value.hpp"
#include "NeForce/core/string/codepoint.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup YamlConfig YAML配置
 * @{
 */

/**
 * @class yaml_parser
 * @brief YAML 1.2 递归下降解析器
 *
 * 支持标量类型推断、五种字符串样式、块/流集合、锚点与别名、标签、
 * 合并键、多文档与指令。
 */
class NEFORCE_API yaml_parser {
private:
    /**
     * @struct anchor_entry
     * @brief 锚点表项
     */
    struct anchor_entry {
        string_view name; ///< 锚点名
        yaml_value value; ///< 锚点值
    };

    string owned_;                 ///< 拥有模式下的输入副本
    string_view text_;             ///< 输入视图
    const char* begin_ = nullptr;  ///< 输入起始
    const char* cursor_ = nullptr; ///< 当前读取位置
    const char* end_ = nullptr;    ///< 输入结束
    bool owning_ = false;          ///< 是否拥有输入

    size_t current_indent_ = 0;    ///< 当前缩进
    bool in_flow_context_ = false; ///< 是否处于流上下文

    vector<anchor_entry> anchors_; ///< 锚点表
    yaml_document document_;       ///< 当前文档

private:
    NEFORCE_NODISCARD char current() const noexcept { return cursor_ < end_ ? *cursor_ : '\0'; }

    NEFORCE_NODISCARD char peek(const size_t offset = 1) const noexcept {
        const char* position = cursor_ + offset;
        return position < end_ ? *position : '\0';
    }

    NEFORCE_NODISCARD bool eof() const noexcept { return cursor_ >= end_; }

    void advance() noexcept {
        if (cursor_ < end_) {
            ++cursor_;
        }
    }

    void advance_bulk(const size_t count) noexcept { cursor_ = cursor_ + count < end_ ? cursor_ + count : end_; }

    bool match(const char ch) noexcept {
        if (current() == ch) {
            advance();
            return true;
        }
        return false;
    }
    bool expect(char ch);

    void skip_whitespace_inline() noexcept;
    void skip_comment() noexcept;
    void skip_to_next_line() noexcept;
    void skip_blank_lines() noexcept;
    void skip_whitespace_and_comments();

    NEFORCE_NODISCARD size_t peek_indent() const noexcept;
    size_t skip_indent();
    void handle_indent_change(size_t new_indent);

    void parse_directive();
    NEFORCE_NODISCARD string_view parse_anchor();
    void register_anchor(string_view anchor, const yaml_value& value);
    NEFORCE_NODISCARD yaml_value parse_alias();
    void skip_tag() noexcept;
    NEFORCE_NODISCARD string_view parse_tag();
    NEFORCE_NODISCARD bool has_anchor() const noexcept;
    NEFORCE_NODISCARD bool has_alias() const noexcept;

    NEFORCE_NODISCARD yaml_value parse_plain_string();
    NEFORCE_NODISCARD yaml_value parse_single_quoted_string();
    NEFORCE_NODISCARD yaml_value parse_double_quoted_string();
    NEFORCE_NODISCARD yaml_value parse_literal_string();
    NEFORCE_NODISCARD yaml_value parse_folded_string();
    NEFORCE_NODISCARD string parse_multiline_string(bool is_literal);

    NEFORCE_NODISCARD yaml_value parse_scalar();
    NEFORCE_NODISCARD yaml_value parse_scalar_body();
    NEFORCE_NODISCARD yaml_value parse_number();
    NEFORCE_NODISCARD yaml_value parse_boolean();
    NEFORCE_NODISCARD yaml_value parse_null();
    NEFORCE_NODISCARD yaml_value parse_timestamp(string_view str);

    NEFORCE_NODISCARD yaml_value parse_flow_sequence();
    NEFORCE_NODISCARD yaml_value parse_flow_mapping();
    NEFORCE_NODISCARD yaml_value parse_block_sequence();
    NEFORCE_NODISCARD yaml_value parse_block_mapping(bool parent_skipped_indent);

    NEFORCE_NODISCARD string_view parse_key();
    NEFORCE_NODISCARD string_view parse_plain_key();
    NEFORCE_NODISCARD string_view parse_quoted_key();

    NEFORCE_NODISCARD yaml_value parse_value();
    NEFORCE_NODISCARD yaml_value parse_block_value();
    NEFORCE_NODISCARD yaml_value parse_inline_value();
    NEFORCE_NODISCARD yaml_value parse_single_document();

    void parse_document_start();
    void parse_document_end();
    NEFORCE_NODISCARD bool is_document_start() const noexcept;
    NEFORCE_NODISCARD bool is_document_end() const noexcept;

    NEFORCE_NODISCARD codepoint parse_unicode_escape(size_t digits);
    NEFORCE_NORETURN void throw_parse_error(const string& message) const;

public:
    /**
     * @brief 从字符串构造解析器（拥有输入）
     * @param yaml_str 待解析的 YAML 字符串
     */
    explicit yaml_parser(string yaml_str);

    /**
     * @brief 从字符串视图构造解析器（借用输入）
     * @param yaml_view 待解析的 YAML 视图
     */
    explicit yaml_parser(string_view yaml_view) noexcept;

    /**
     * @brief 从C字符串构造解析器（借用输入）
     * @param yaml_str 待解析的 YAML 字符串
     */
    explicit yaml_parser(const char* yaml_str) noexcept;

    yaml_parser(const yaml_parser&) = delete;
    yaml_parser& operator=(const yaml_parser&) = delete;

    /**
     * @brief 解析单个 YAML 文档
     * @return 解析完成的文档
     * @throws yaml_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD yaml_document parse();

    /**
     * @brief 尝试解析单个 YAML 文档
     * @return 成功时返回文档，失败时返回空 optional
     */
    NEFORCE_NODISCARD optional<yaml_document> try_parse() noexcept;

    /**
     * @brief 解析多个 YAML 文档
     * @return 文档列表
     * @throws yaml_exception 解析失败时抛出
     */
    NEFORCE_NODISCARD vector<yaml_document> parse_documents();

    /**
     * @brief 尝试解析多个 YAML 文档
     * @return 成功时返回文档列表，失败时返回空 optional
     */
    NEFORCE_NODISCARD optional<vector<yaml_document>> try_parse_documents() noexcept;
};

/** @} */ // YamlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_PARSER_HPP__
