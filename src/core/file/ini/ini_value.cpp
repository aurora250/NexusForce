#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/file/ini/ini_value.hpp>
MSTL_BEGIN_NAMESPACE__

int ini_property::get_int(const int default_value) const noexcept {
    try {
        return integer32::parse(value_.view());
    } catch (...) {
        return default_value;
    }
}

double ini_property::get_double(const double default_value) const noexcept {
    try {
        return float64::parse(value_.view());
    } catch (...) {
        return default_value;
    }
}

bool ini_property::get_bool(const bool default_value) const noexcept {
    try {
        return boolean::parse(value_.view());
    } catch (...) {
        return default_value;
    }
}

MSTL_BEGIN_INNER__

string MSTL_API ini_value_to_string(const ini_value* value) {
    if (!value) return "";

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
            for (const auto& prop : section->get_properties()) {
                result += prop.first + " = " + prop.second->get_value() + "\n";
            }
            return result;
        }
        default: {
            return "";
        }
    }
}

string MSTL_API ini_document_to_string(const ini_document* doc) {
    if (!doc) return "";
    string result;
    const ini_section* global = doc->get_global_section();
    if (global && !global->get_properties().empty()) {
        for (const auto& prop : global->get_properties()) {
            result += prop.first + " = " + prop.second->get_value() + "\n";
        }
        result += "\n";
    }

    for (const auto& sec : doc->get_sections()) {
        result += "[" + sec.first + "]\n";
        for (const auto& prop : sec.second->get_properties()) {
            result += prop.first + " = " + prop.second->get_value() + "\n";
        }
        result += "\n";
    }
    return result;
}

MSTL_END_INNER__

MSTL_END_NAMESPACE__
