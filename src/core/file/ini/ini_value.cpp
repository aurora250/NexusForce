#include <NeForce/core/file/ini/ini_value.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string ini_value_to_string(const ini_value* value) {
        switch (value->type()) {
            case ini_value::Property: {
                return value->as_property()->get_value();
            }
            case ini_value::Section: {
                const ini_section* section = value->as_section();
                string result;
                if (!section->get_name().empty()) {
                    result += "[" + section->get_name() + "]\n";
                }
                for (const auto& prop: section->get_properties()) {
                    result += prop.first + " = " + prop.second->get_value() + "\n";
                }
                return result;
            }
            default: {
                return "";
            }
        }
    }
} // namespace


int ini_property::get_int(const int default_value) const noexcept {
    try {
        return integer32::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

double ini_property::get_double(const double default_value) const noexcept {
    try {
        return float64::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

bool ini_property::get_bool(const bool default_value) const noexcept {
    try {
        return boolean::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

string ini_value::to_string() const { return ini_value_to_string(this); }

string ini_value::to_document() const { return ini_value_to_string(this); }

string ini_document::to_string() const {
    string result;
    const ini_section* global = get_global_section();
    if (global != nullptr && !global->get_properties().empty()) {
        for (const auto& prop: global->get_properties()) {
            result += prop.first + " = " + prop.second->get_value() + "\n";
        }
        result += "\n";
    }

    for (const auto& sec: get_sections()) {
        result += "[" + sec.first + "]\n";
        for (const auto& prop: sec.second->get_properties()) {
            result += prop.first + " = " + prop.second->get_value() + "\n";
        }
        result += "\n";
    }
    return result;
}

NEFORCE_END_NAMESPACE__
