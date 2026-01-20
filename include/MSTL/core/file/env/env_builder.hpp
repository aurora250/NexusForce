#ifndef MSTL_CORE_FILE_ENV_ENV_BUILDER_HPP__
#define MSTL_CORE_FILE_ENV_ENV_BUILDER_HPP__
#include "env_value.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API env_builder {
private:
    unique_ptr<env_document> root_;
    string current_key_;
    env_variable::quote_type current_quote_type_ = env_variable::None;
    bool current_exported_ = false;

public:
    env_builder();
    env_builder(const env_builder&) = delete;
    env_builder& operator =(const env_builder&) = delete;
    env_builder(env_builder&&) = default;
    env_builder& operator =(env_builder&&) = default;

    env_builder& key(const string& k);

    env_builder& unquoted() noexcept;
    env_builder& single_quoted() noexcept;
    env_builder& double_quoted() noexcept;
    env_builder& exported(bool exp = true) noexcept;

    env_builder& value(const string& v);
    env_builder& value(const char* v) { return value(string(v)); }
    env_builder& value(const string_view v) { return value(string(v)); }
    env_builder& value(int v);
    env_builder& value(int64_t v);
    env_builder& value(double v);
    env_builder& value(bool v);

    env_builder& value(double v, int precision);

    env_builder& comment(const string& text);
    env_builder& blank_line();

    env_builder& add(const string& key, const string& value);
    env_builder& add(const string& key, const string_view value) { return add(key, string(value)); }
    env_builder& add(const string& key, const char* value) { return add(key, string(value)); }
    env_builder& add(const string& key, int value);
    env_builder& add(const string& key, int64_t value);
    env_builder& add(const string& key, double value);
    env_builder& add(const string& key, bool value);

    env_builder& add_export(const string& key, const string& value);

    unique_ptr<env_document> build();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_ENV_ENV_BUILDER_HPP__
