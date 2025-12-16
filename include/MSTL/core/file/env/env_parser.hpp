#ifndef MSTL_CORE_FILE_ENV_ENV_PARSER_HPP__
#define MSTL_CORE_FILE_ENV_ENV_PARSER_HPP__
#include "../../utility/optional.hpp"
#include "env_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API env_parser {
private:
    string env_;
    size_t len_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    unique_ptr<env_document> root_;

    void skip_whitespace() noexcept;
    void skip_line() noexcept;

    char current() const noexcept;
    char peek(size_t offset = 1) const noexcept;
    bool eof() const noexcept;
    void advance() noexcept;

    bool is_comment_line(const string& line) const noexcept;
    bool is_blank_line(const string& line) const noexcept;
    bool parse_variable_line(const string& line, string& name,
        unique_ptr<env_variable>& variable) const;

    string parse_unquoted_value(const string& line, size_t& pos) const;
    string parse_single_quoted_value(const string& line, size_t& pos) const;
    string parse_double_quoted_value(const string& line, size_t& pos) const;

    void parse_line(const string& line) const;

public:
    explicit env_parser(string env_str) noexcept
        : env_(_MSTL move(env_str)), len_(env_.size()) {
        root_ = make_unique<env_document>();
    }

    unique_ptr<env_document> parse();
    optional<unique_ptr<env_document>> try_parse();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_ENV_ENV_PARSER_HPP__
