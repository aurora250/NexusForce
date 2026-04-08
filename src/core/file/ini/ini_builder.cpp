#include <NeForce/core/file/ini/ini_builder.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

void ini_builder::test_exception() const {
    if (current_section_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ini_exception("No section context for value"));
    }
    if (current_key_.empty()) {
        NEFORCE_THROW_EXCEPTION(ini_exception("No key set for value"));
    }
}

ini_builder::ini_builder() :
root_(make_unique<ini_document>()),
current_section_(root_->get_global_section()) {}

ini_builder& ini_builder::begin_section(const string& name) {
    auto new_section = make_unique<ini_section>(name);
    current_section_ = new_section.get();
    root_->add_section(name, move(new_section));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::end_section() {
    current_section_ = root_->get_global_section();
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::key(const string& key) {
    if (current_section_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(ini_exception("No section context for key"));
    }
    current_key_ = key;
    return *this;
}

ini_builder& ini_builder::value(string value) {
    test_exception();
    current_section_->set_property(current_key_, move(value));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const int value) {
    test_exception();
    current_section_->set_property(current_key_, _NEFORCE to_string(value));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const int64_t value) {
    test_exception();
    current_section_->set_property(current_key_, _NEFORCE to_string(value));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const double value) {
    test_exception();
    current_section_->set_property(current_key_, _NEFORCE to_string(value));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const bool value) {
    test_exception();
    current_section_->set_property(current_key_, _NEFORCE to_string(value));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const double value, const int precision) {
    test_exception();
    current_section_->set_property(current_key_, to_string_with_precision(value, precision));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value_section(const string& name, function<void(ini_builder&)> func) {
    begin_section(name);
    func(*this);
    end_section();
    return *this;
}

unique_ptr<ini_document> ini_builder::build() noexcept { return move(root_); }

NEFORCE_END_NAMESPACE__
