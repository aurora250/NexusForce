#include <MSTL/core/file/env/env_builder.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

env_builder::env_builder() {
    root_ = make_unique<env_document>();
}

env_builder& env_builder::key(const string& k) {
    current_key_ = k;
    return *this;
}

env_builder& env_builder::unquoted() noexcept {
    current_quote_type_ = env_variable::None;
    return *this;
}

env_builder& env_builder::single_quoted() noexcept {
    current_quote_type_ = env_variable::Single;
    return *this;
}

env_builder& env_builder::double_quoted() noexcept {
    current_quote_type_ = env_variable::Double;
    return *this;
}

env_builder& env_builder::exported(const bool exp) noexcept {
    current_exported_ = exp;
    return *this;
}

env_builder& env_builder::value(const string& v) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, v,
        current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const int v) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(v),
        current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const int64_t v) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(v),
        current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const double v) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(v),
        current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const bool v) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(v),
        current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const double v, const int precision) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }
    root_->set_variable(current_key_, _MSTL to_string_with_precision(v, precision),
        current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::comment(const string& text) {
    root_->add_comment(text);
    return *this;
}

env_builder& env_builder::blank_line() {
    root_->add_comment("");
    return *this;
}

env_builder& env_builder::add(const string& key, const string& value) {
    return this->key(key).value(value);
}

env_builder& env_builder::add(const string& key, const int value) {
    return this->key(key).value(value);
}

env_builder& env_builder::add(const string& key, const int64_t value) {
    return this->key(key).value(value);
}

env_builder& env_builder::add(const string& key, const double value) {
    return this->key(key).value(value);
}

env_builder& env_builder::add(const string& key, const bool value) {
    return this->key(key).value(value);
}

env_builder& env_builder::add_export(const string& key, const string& value) {
    return this->exported().key(key).value(value);
}

unique_ptr<env_document> env_builder::build() {
    return _MSTL move(root_);
}

MSTL_END_NAMESPACE__
