#ifndef MSTL_CORE_FILE_INI_INI_BUILDER_HPP__
#define MSTL_CORE_FILE_INI_INI_BUILDER_HPP__
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API ini_builder {
private:
    enum class entry_type {
        SECTION,
        KEY_VALUE,
        COMMENT
    };

    struct entry {
        entry_type type;
        string data1;
        string data2;

        entry(const entry_type type, string data1, string data2)
        : type(type), data1(_MSTL move(data1)), data2(_MSTL move(data2)) {}
    };

    vector<entry> entries_;

public:
    ini_builder& add_comment(string comment);
    ini_builder& add_blank_line();

    ini_builder& section(string section_name);

    ini_builder& add(string key, string value);
    ini_builder& add(string key, const char* value);

    ini_builder& add(string key, int32_t value);
    ini_builder& add(string key, int64_t value);
    ini_builder& add(string key, double value);
    ini_builder& add(string key, double value, int precision);
    ini_builder& add(string key, bool value);

    MSTL_NODISCARD string to_string() const;

    ini_builder& clear();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_INI_INI_BUILDER_HPP__
