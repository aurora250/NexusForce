#ifndef MSTL_CORE_FILE_YAML_YAML_PARSER_HPP__
#define MSTL_CORE_FILE_YAML_YAML_PARSER_HPP__
#include "MSTL/core/compound/optional.hpp"
#include "yaml_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API yaml_parser {
private:
    struct indent_context {
        size_t level;
        yaml_mapping* mapping;
        yaml_sequence* sequence;
        string key;
        bool is_sequence;
    };

    string yaml_;
    size_t len_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    vector<indent_context> indent_stack_;
    yaml_ptr root_;
    size_t current_indent_ = 0;
    bool in_flow_context_ = false;

    unordered_map<string, yaml_ptr> anchors_;

    char current() const noexcept;
    char peek(size_t offset = 1) const noexcept;
    bool eof() const noexcept;
    void advance() noexcept;
    bool match(char ch) noexcept;
    bool expect(char ch);

    void skip_whitespace_inline() noexcept;
    void skip_comment() noexcept;
    void skip_to_next_line() noexcept;
    void skip_blank_lines() noexcept;
    void skip_whitespace_and_comments();
    bool is_whitespace(char ch) const noexcept;
    bool is_newline(char ch) const noexcept;

    size_t peek_indent() const noexcept;
    size_t skip_indent();
    void handle_indent_change(size_t new_indent);

    void parse_directive();

    string parse_anchor();
    void register_anchor(const string& anchor, const shared_ptr<yaml_value>& value);
    yaml_ptr parse_alias();
    void skip_tag() noexcept;
    string parse_tag();
    bool has_anchor() const noexcept;
    bool has_alias() const noexcept;

    shared_ptr<yaml_string> parse_plain_string();
    shared_ptr<yaml_string> parse_single_quoted_string();
    shared_ptr<yaml_string> parse_double_quoted_string();
    shared_ptr<yaml_string> parse_literal_string();
    shared_ptr<yaml_string> parse_folded_string();
    string parse_multiline_string(bool is_literal);

    shared_ptr<yaml_value> parse_scalar();
    shared_ptr<yaml_value> parse_number();
    shared_ptr<yaml_boolean> parse_boolean();
    shared_ptr<yaml_null> parse_null();
    shared_ptr<yaml_timestamp> parse_timestamp(string_view str) const;

    shared_ptr<yaml_sequence> parse_flow_sequence();
    shared_ptr<yaml_mapping> parse_flow_mapping();
    shared_ptr<yaml_sequence> parse_block_sequence();
    shared_ptr<yaml_mapping> parse_block_mapping(bool parent_skipped_indent);

    string parse_key();
    string parse_plain_key();
    string parse_quoted_key();

    shared_ptr<yaml_value> parse_value();
    shared_ptr<yaml_value> parse_block_value();
    shared_ptr<yaml_value> parse_inline_value();
    shared_ptr<yaml_value> parse_single_document();

    void parse_document_start();
    void parse_document_end();
    bool is_document_start() const noexcept;
    bool is_document_end() const noexcept;

    bool is_plain_safe(char ch) const noexcept;
    bool is_key_char(char ch) const noexcept;
    bool is_indicator(char ch) const noexcept;
    bool is_flow_indicator(char ch) const noexcept;

    char32_t parse_unicode_escape(size_t digits);
    string unescape_string(const string& str) const;

    void throw_parse_error(const string& message) const;

public:
    explicit yaml_parser(string yaml_str) noexcept
    : yaml_(_MSTL move(yaml_str)), len_(yaml_.size()) {}

    yaml_ptr parse();
    optional<yaml_ptr> try_parse();

    vector<yaml_ptr> parse_documents();
    optional<vector<yaml_ptr>> try_parse_documents();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_YAML_YAML_PARSER_HPP__
