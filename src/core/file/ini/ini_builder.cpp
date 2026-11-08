#include "NeForce/core/file/ini/ini_builder.hpp"
#include "NeForce/core/string/to_string.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

void ini_builder::test_exception() const {
    if (current_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ini_exception("No section context for value"));
    }
    if (current_key_.empty()) {
        NEFORCE_THROW_EXCEPTION(ini_exception("No key set for value"));
    }
}

ini_builder::ini_builder() :
current_(document_.root_.storage_.ext_.payload_.section_) {}

ini_builder& ini_builder::begin_section(const string& name) {
    current_ = document_.new_section(string_view(name.data(), name.size())).data();
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::end_section() {
    current_ = document_.global_section_mut().data();
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::key(const string& key) {
    current_key_ = key;
    return *this;
}

ini_builder& ini_builder::value(string value) {
    test_exception();
    document_.append_property(current_, string_view(current_key_.data(), current_key_.size()),
                              string_view(value.data(), value.size()), true);
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const int value) { return this->value(to_string(value)); }

ini_builder& ini_builder::value(const int64_t value) { return this->value(to_string(value)); }

ini_builder& ini_builder::value(const double value) { return this->value(to_string(value)); }

ini_builder& ini_builder::value(const bool value) { return this->value(to_string(value)); }

ini_builder& ini_builder::value(const double value, const int precision) {
    return this->value(to_string_with_precision(value, precision));
}

ini_builder& ini_builder::value_section(const string& name, function<void(ini_builder&)> func) {
    begin_section(name);
    func(*this);
    end_section();
    return *this;
}

ini_document ini_builder::build() noexcept { return move(document_); }

NEFORCE_END_NAMESPACE__
