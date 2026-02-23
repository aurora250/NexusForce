#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/file/env/env_builder.hpp>
MSTL_BEGIN_NAMESPACE__

env_builder::env_builder() {
    root_ = make_unique<env_document>();
}

env_builder& env_builder::key(string key) noexcept {
    current_key_ = _MSTL move(key);
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

env_builder& env_builder::exported(const bool exported) noexcept {
    current_exported_ = exported;
    return *this;
}

env_builder& env_builder::value(string value) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL move(value), current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const int value) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(value), current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const int64_t value) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(value), current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const double value) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(value), current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const bool value) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(current_key_, _MSTL to_string(value), current_quote_type_, current_exported_);
    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::value(const double value, const int precision) {
    if (current_key_.empty()) {
        throw_exception(env_exception("No key set for value"));
    }

    root_->set_variable(
        current_key_, _MSTL to_string_with_precision(value, precision),
        current_quote_type_, current_exported_);

    current_key_.clear();
    current_quote_type_ = env_variable::None;
    current_exported_ = false;
    return *this;
}

env_builder& env_builder::comment(string text) noexcept {
    root_->add_comment(move(text));
    return *this;
}

env_builder& env_builder::blank_line() noexcept {
    static string empty{};
    root_->add_comment(empty);
    return *this;
}

env_builder& env_builder::add(string key, string value) {
    return this->key(move(key)).value(_MSTL move(value));
}

env_builder& env_builder::add(string key, const int value) {
    return this->key(move(key)).value(value);
}

env_builder& env_builder::add(string key, const int64_t value) {
    return this->key(move(key)).value(value);
}

env_builder& env_builder::add(string key, const double value) {
    return this->key(move(key)).value(value);
}

env_builder& env_builder::add(string key, const bool value) {
    return this->key(move(key)).value(value);
}

env_builder& env_builder::add_export(string key, string value) {
    return this->exported().key(move(key)).value(_MSTL move(value));
}

unique_ptr<env_document> env_builder::build() noexcept {
    return _MSTL move(root_);
}

MSTL_END_NAMESPACE__
