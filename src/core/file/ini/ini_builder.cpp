#include <MSTL/core/file/ini/ini_builder.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

void ini_builder::test_exception() const {
    if (!current_section_) {
        throw_exception(ini_exception("No section context for value"));
    }
    if (current_key_.empty()) {
        throw_exception(ini_exception("No key set for value"));
    }
}

ini_builder::ini_builder() {
    root_ = make_unique<ini_document>();
    current_section_ = root_->get_global_section();
}

ini_builder& ini_builder::begin_section(const string& section_name) {
    auto new_section = make_unique<ini_section>(section_name);
    current_section_ = new_section.get();
    root_->add_section(section_name, _MSTL move(new_section));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::end_section() {
    current_section_ = root_->get_global_section();
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::key(const string& k) {
    if (!current_section_) {
        throw_exception(ini_exception("No section context for key"));
    }
    current_key_ = k;
    return *this;
}

ini_builder& ini_builder::value(const string& v) {
    test_exception();
    current_section_->set_property(current_key_, v);
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const int v) {
    test_exception();
    current_section_->set_property(current_key_, _MSTL to_string(v));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const int64_t v) {
    test_exception();
    current_section_->set_property(current_key_, _MSTL to_string(v));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const double v) {
    test_exception();
    current_section_->set_property(current_key_, _MSTL to_string(v));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const bool v) {
    test_exception();
    current_section_->set_property(current_key_, _MSTL to_string(v));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value(const double v, const int precision) {
    test_exception();
    current_section_->set_property(current_key_, _MSTL to_string_with_precision(v, precision));
    current_key_.clear();
    return *this;
}

ini_builder& ini_builder::value_section(const string& section_name,
    _MSTL function<void(ini_builder&)>&& build_func) {
    begin_section(section_name);
    build_func(*this);
    end_section();
    return *this;
}

unique_ptr<ini_document> ini_builder::build() {
    return _MSTL move(root_);
}

MSTL_END_NAMESPACE__
