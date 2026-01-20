#ifndef MSTL_CORE_FILE_INI_INI_BUILDER_HPP__
#define MSTL_CORE_FILE_INI_INI_BUILDER_HPP__
#include "MSTL/core/functional/function.hpp"
#include "ini_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API ini_builder {
private:
    unique_ptr<ini_document> root_;
    ini_section* current_section_ = nullptr;
    string current_key_;

    void test_exception() const;

public:
    ini_builder();
    ini_builder(const ini_builder&) = delete;
    ini_builder& operator =(const ini_builder&) = delete;
    ini_builder(ini_builder&&) = default;
    ini_builder& operator =(ini_builder&&) = default;

    ini_builder& begin_section(const string& section_name);
    ini_builder& end_section();

    ini_builder& key(const string& k);

    ini_builder& value(const string& v);
    ini_builder& value(const char* v) { return value(string(v)); }
    ini_builder& value(const string_view v) { return value(string(v)); }

    ini_builder& value(int v);
    ini_builder& value(int64_t v);
    ini_builder& value(double v);
    ini_builder& value(bool v);

    ini_builder& value(double v, int precision);

    ini_builder& value_section(const string& section_name,
        _MSTL function<void(ini_builder&)>&& build_func);

    unique_ptr<ini_document> build();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_INI_INI_BUILDER_HPP__
