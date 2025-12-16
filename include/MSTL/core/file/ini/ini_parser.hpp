#ifndef MSTL_CORE_FILE_INI_INI_PARSER_HPP__
#define MSTL_CORE_FILE_INI_INI_PARSER_HPP__
#include "../../utility/optional.hpp"
#include "ini_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API ini_parser {
private:
    string ini_;
    size_t len_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    unique_ptr<ini_document> root_;
    ini_section* current_section_ = nullptr;

    void skip_whitespace() noexcept;
    void skip_line() noexcept;

    char current() const noexcept;
    char peek(size_t offset = 1) const noexcept;
    bool eof() const noexcept;
    void advance() noexcept;

    bool is_comment_line(const string& line) const noexcept;
    bool is_section_line(const string& line, string& section_name) const;
    bool parse_key_value(const string& line, string& key, string& value) const;

    void parse_line(const string& line);

public:
    explicit ini_parser(string ini_str) noexcept
        : ini_(_MSTL move(ini_str)), len_(ini_.size()) {
        root_ = make_unique<ini_document>();
        current_section_ = root_->get_global_section();
    }

    unique_ptr<ini_document> parse();
    optional<unique_ptr<ini_document>> try_parse();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_INI_INI_PARSER_HPP__
