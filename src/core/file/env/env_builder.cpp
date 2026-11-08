#include "NeForce/core/file/env/env_builder.hpp"
#include "NeForce/core/string/to_string.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

void env_builder::test_exception() const {
    if (current_key_.empty()) {
        NEFORCE_THROW_EXCEPTION(env_exception("No key set for value"));
    }
}

env_builder::env_builder() = default;

env_builder& env_builder::key(string key) noexcept {
    current_key_ = move(key);
    return *this;
}

env_builder& env_builder::unquoted() noexcept {
    quote_ = env_variable::None;
    return *this;
}

env_builder& env_builder::single_quoted() noexcept {
    quote_ = env_variable::Single;
    return *this;
}

env_builder& env_builder::double_quoted() noexcept {
    quote_ = env_variable::Double;
    return *this;
}

env_builder& env_builder::exported(const bool exported) noexcept {
    exported_ = exported;
    return *this;
}

env_builder& env_builder::assign(string value) {
    test_exception();
    document_.set_variable(string_view(current_key_.data(), current_key_.size()),
                           string_view(value.data(), value.size()), quote_, exported_);
    current_key_.clear();
    quote_ = env_variable::None;
    exported_ = false;
    return *this;
}

env_builder& env_builder::value(string value) { return assign(move(value)); }

env_builder& env_builder::value(const int value) { return assign(to_string(value)); }

env_builder& env_builder::value(const int64_t value) { return assign(to_string(value)); }

env_builder& env_builder::value(const double value) { return assign(to_string(value)); }

env_builder& env_builder::value(const bool value) { return assign(to_string(value)); }

env_builder& env_builder::value(const double value, const int precision) {
    return assign(to_string_with_precision(value, precision));
}

env_builder& env_builder::comment(string text) {
    document_.add_comment(string_view(text.data(), text.size()));
    return *this;
}

env_builder& env_builder::blank_line() {
    document_.add_comment(string_view());
    return *this;
}

env_builder& env_builder::add(string key, string value) { return this->key(move(key)).value(move(value)); }

env_builder& env_builder::add(string key, const int value) { return this->key(move(key)).value(value); }

env_builder& env_builder::add(string key, const int64_t value) { return this->key(move(key)).value(value); }

env_builder& env_builder::add(string key, const double value) { return this->key(move(key)).value(value); }

env_builder& env_builder::add(string key, const bool value) { return this->key(move(key)).value(value); }

env_builder& env_builder::add_export(string key, string value) {
    return this->exported().key(move(key)).value(move(value));
}

env_document env_builder::build() noexcept { return move(document_); }

NEFORCE_END_NAMESPACE__
