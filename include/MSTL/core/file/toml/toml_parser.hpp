#ifndef MSTL_CORE_FILE_TOML_TOML_PARSER_HPP__
#define MSTL_CORE_FILE_TOML_TOML_PARSER_HPP__
#include "../../utility/optional.hpp"
#include "toml_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API toml_parser {
private:
    struct context {
        toml_table* table;
        vector<string> path;
    };

    vector<context> context_stack_;
    bool is_in_array_table_ = false;

    string toml_;
    size_t len_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    unique_ptr<toml_table> root_;
    toml_table* ctb_ = nullptr;
    vector<string> ctp_;

    void skip_whitespace() noexcept;
    void skip_comment() noexcept;
    void skip_whitespace_and_comments() noexcept;
    void skip_newlines() noexcept;
    void skip_whitespace_no_newline() noexcept;

    char current() const noexcept;
    char peek(size_t offset = 1) const noexcept;
    bool eof() const noexcept;
    void advance() noexcept;
    bool expect(char ch);
    bool match(char ch) noexcept;

    unique_ptr<toml_string> parse_string();
    unique_ptr<toml_string> parse_basic_string();
    unique_ptr<toml_string> parse_literal_string();
    unique_ptr<toml_string> parse_multiline_basic_string();
    unique_ptr<toml_string> parse_multiline_literal_string();

    char32_t parse_unicode_escape(size_t digits);

    unique_ptr<toml_value> parse_number();
    unique_ptr<toml_integer> parse_integer(int base = 10);

    unique_ptr<toml_boolean> parse_boolean();
    unique_ptr<toml_datetime> parse_datetime();

    unique_ptr<toml_array> parse_array();
    unique_ptr<toml_table> parse_inline_table();

    string parse_key();
    string parse_bare_key();
    string parse_quoted_key();
    vector<string> parse_dotted_key();

    unique_ptr<toml_value> parse_value();
    void parse_key_value();

    void parse_table_header();
    void parse_array_table_header();

    toml_table* get_or_create_table(const vector<string>& path) const;
    toml_table* navigate_to_table(const vector<string>& path) const;
    void set_current_table(const vector<string>& path);

    void throw_parse_error(string message) const;

public:
    explicit toml_parser(string toml_str) noexcept
        : toml_(_MSTL move(toml_str)), len_(toml_.size()) {
        root_ = make_unique<toml_table>();
        ctb_ = root_.get();
    }

    unique_ptr<toml_table> parse();
    optional<unique_ptr<toml_table>> try_parse();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_TOML_TOML_PARSER_HPP__
