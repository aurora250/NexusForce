#include <MSTL/core/file/ini/ini_builder.hpp>
#include <MSTL/core/string/to_string.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

ini_builder& ini_builder::add_comment(string comment) {
    entries_.emplace_back(entry_type::COMMENT, _MSTL move(comment), "");
    return *this;
}

ini_builder& ini_builder::add_blank_line() {
    return add_comment("");
}

ini_builder& ini_builder::section(string section_name) {
    entries_.emplace_back(entry_type::SECTION, _MSTL move(section_name), "");
    return *this;
}

ini_builder& ini_builder::add(string key, string value) {
    entries_.emplace_back(entry_type::KEY_VALUE, _MSTL move(key),_MSTL move(value));
    return *this;
}

ini_builder& ini_builder::add(string key, const char* value) {
    return add(_MSTL move(key), string(value));
}

ini_builder& ini_builder::add(string key, const int32_t value) {
    return add(_MSTL move(key), _MSTL to_string(value));
}

ini_builder& ini_builder::add(string key, const int64_t value) {
    return add(_MSTL move(key), _MSTL to_string(value));
}

ini_builder& ini_builder::add(string key, const double value) {
    return add(_MSTL move(key), _MSTL to_string(value));
}

ini_builder& ini_builder::add(string key, const double value, const int precision) {
    return add(_MSTL move(key), _MSTL to_string_with_precision(value, precision));
}

ini_builder& ini_builder::add(string key, const bool value) {
    return add(_MSTL move(key), _MSTL to_string(value));
}

string ini_builder::to_string() const {
    string result;
    for (const auto& e : entries_) {
        switch (e.type) {
            case entry_type::COMMENT: {
                if (e.data1.empty()) {
                    result += "\n";
                } else {
                    result += "; " + e.data1 + "\n";
                }
                break;
            }
            case entry_type::SECTION: {
                result += "[" + e.data1 + "]\n";
                break;
            }
            case entry_type::KEY_VALUE: {
                result += e.data1 + " = " + e.data2 + "\n";
                break;
            }
        }
    }
    return result;
}

ini_builder& ini_builder::clear() {
    entries_.clear();
    return *this;
}

MSTL_END_NAMESPACE__
