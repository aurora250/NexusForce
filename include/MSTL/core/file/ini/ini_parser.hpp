#ifndef MSTL_CORE_FILE_INI_INI_PARSER_HPP__
#define MSTL_CORE_FILE_INI_INI_PARSER_HPP__
#include "MSTL/core/container/map.hpp"
#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API ini_parser {
private:
    map<string, map<string, string>> data_;

    bool is_comment(const string& line) const;
    bool is_section(const string& line, string& section_name) const;

    bool parse_key_value(const string& line, string& key, string& value) const;

public:
    bool try_parse(const string& content);

    MSTL_NODISCARD string get_string(const string& section, const string& key, const string& defaultv = "") const;
    MSTL_NODISCARD int get_int(const string& section, const string& key, int defaultv = 0) const;
    MSTL_NODISCARD double get_double(const string& section, const string& key, double defaultv = 0.0) const;
    MSTL_NODISCARD bool get_bool(const string& section, const string& key, bool defaultv = false) const;

    void set_value(const string& section, const string& key, const string& value);

    MSTL_NODISCARD bool has_section(const string& section) const;
    MSTL_NODISCARD bool has_key(const string& section, const string& key) const;

    MSTL_NODISCARD string to_string() const;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_INI_INI_PARSER_HPP__
