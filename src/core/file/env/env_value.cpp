#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/file/env/env_value.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string escape_env_value(const string& value, env_variable::quote_type qt) {
        if (qt == env_variable::None) {
            return value;
        } else if (qt == env_variable::Single) {
            string result;
            for (const char c : value) {
                if (c == '\'') {
                    result += "'\\''";
                } else {
                    result += c;
                }
            }
            return result;
        } else {
            return escape(value);
        }
    }

    string env_value_to_string(const env_value* value, const string& key = "") {
        switch (value->type()) {
            case env_value::Variable: {
                const env_variable* var = value->as_variable();
                const string& val = var->get_value();
                env_variable::quote_type qt = var->get_quote_type();

                string result;
                if (var->is_exported()) {
                    result += "export ";
                }
                if (!key.empty()) {
                    result += key + "=";
                }

                switch (qt) {
                    case env_variable::None: {
                        result += val;
                        break;
                    }
                    case env_variable::Single: {
                        result += "'" + escape_env_value(val, qt) + "'";
                        break;
                    }
                    case env_variable::Double: {
                        result += "\"" + escape_env_value(val, qt) + "\"";
                        break;
                    }
                }
                return result;
            }
            default: {
                return "";
            }
        }
    }
}


int env_variable::get_int(const int default_value) const noexcept {
    try {
        return integer32::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

int64_t env_variable::get_int64(const int64_t default_value) const noexcept {
    try {
        return integer64::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

double env_variable::get_double(const double default_value) const noexcept {
    try {
        return float64::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

bool env_variable::get_bool(const bool default_value) const noexcept {
    try {
        return boolean::parse(value_.view()).value();
    } catch (...) {
        return default_value;
    }
}

string env_value::to_string() const {
    return env_value_to_string(this);
}

string env_value::to_document() const {
    return env_value_to_string(this);
}

string env_document::to_string() const {
    string result;
    for (const auto& comment : get_comments()) {
        result += "# " + comment + "\n";
    }
    if (!get_comments().empty()) {
        result += "\n";
    }
    for (const auto& var : get_variables()) {
        result += env_value_to_string(var.second.get(), var.first) + "\n";
    }
    return result;
}

NEFORCE_END_NAMESPACE__
