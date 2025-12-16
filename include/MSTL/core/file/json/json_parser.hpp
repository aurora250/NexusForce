#ifndef MSTL_CORE_FILE_JSON_JSON_PARSER_HPP__
#define MSTL_CORE_FILE_JSON_JSON_PARSER_HPP__
#include "../../utility/optional.hpp"
#include "json_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API json_parser {
private:
    string json;
    size_t length;
    size_t pos = 0;

    void skip_space() noexcept {
        while (pos < length && _MSTL is_space(json[pos])) {
            pos++;
        }
    }

    char current() const noexcept {
        if (pos < length) return json[pos];
        return '\0';
    }

    bool eof() const noexcept { return pos >= length; }


    unique_ptr<json_string> parse_string();
    unique_ptr<json_number> parse_number();
    unique_ptr<json_value> parse_keyword();
    unique_ptr<json_array> parse_array();
    unique_ptr<json_object> parse_object();
    unique_ptr<json_value> parse_value();

public:
    explicit json_parser(string json_str) noexcept
    : json(_MSTL move(json_str)), length(json.size()) {}

    unique_ptr<json_value> parse();
    optional<unique_ptr<json_value>> try_parse();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_JSON_JSON_PARSER_HPP__
