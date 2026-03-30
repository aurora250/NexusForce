#include <NeForce/core/file/yaml/yaml_value.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    bool needs_quotes(const string& str) {
        if (str.empty()) return true;

        if (str == "true" || str == "false" || str == "null" ||
            str == "True" || str == "False" || str == "Null" ||
            str == "TRUE" || str == "FALSE" || str == "NULL" ||
            str == "~" || str == "yes" || str == "no" ||
            str == "on" || str == "off") {
            return true;
            }

        if (is_digit(str[0]) || str[0] == '-' || str[0] == '+' || str[0] == '.') {
            return true;
        }

        for (const char ch : str) {
            if (ch == ':' || ch == '#' || ch == '[' || ch == ']' ||
                ch == '{' || ch == '}' || ch == ',' || ch == '&' ||
                ch == '*' || ch == '!' || ch == '|' || ch == '>' ||
                ch == '\'' || ch == '"' || ch == '%' || ch == '@' ||
                ch == '`' || ch == '\n' || ch == '\r') {
                return true;
                }
        }

        return false;
    }

    void stringify_value(string& result, const yaml_value* value, size_t indent, bool inline_context);

    void stringify_sequence(string& result, const yaml_sequence* seq, const size_t indent, const bool inline_context) {
        if (seq->get_style() == yaml_sequence::Flow || inline_context) {
            result += "[";
            const auto& elements = seq->get_elements();
            for (size_t i = 0; i < elements.size(); ++i) {
                if (i > 0) result += ", ";
                stringify_value(result, elements[i].get(), indent, true);
            }
            result += "]";
        } else {
            const auto& elements = seq->get_elements();
            for (const auto& elem : elements) {
                result += "\n" + string(indent, ' ') + "- ";

                if (elem->is_sequence() || elem->is_mapping()) {
                    stringify_value(result, elem.get(), indent + 2, false);
                } else {
                    stringify_value(result, elem.get(), indent + 2, true);
                }
            }
        }
    }

    void stringify_mapping(string& result, const yaml_mapping* map, const size_t indent, const bool inline_context) {
        if (map->get_style() == yaml_mapping::Flow || inline_context) {
            result += "{";
            const auto& members = map->get_members();
            bool first = true;

            for (const auto& pair : members) {
                if (!first) result += ", ";
                first = false;
                if (needs_quotes(pair.first)) {
                    result += "\"" + escape(pair.first) + "\"";
                } else {
                    result += pair.first;
                }
                result += ": ";
                stringify_value(result, pair.second.get(), indent, true);
            }
            result += "}";
        } else {
            const auto& members = map->get_members();
            bool first = true;

            for (const auto& pair : members) {
                if (!first) {
                    result += "\n";
                }
                first = false;
                result += string(indent, ' ');
                if (needs_quotes(pair.first)) {
                    result += "\"" + escape(pair.first) + "\"";
                } else {
                    result += pair.first;
                }

                result += ": ";

                if (pair.second->is_sequence() || pair.second->is_mapping()) {
                    stringify_value(result, pair.second.get(), indent + 2, false);
                } else {
                    stringify_value(result, pair.second.get(), indent + 2, true);
                }
            }
        }
    }

    void stringify_value(string& result, const yaml_value* value, const size_t indent, const bool inline_context) {
        switch (value->type()) {
            case yaml_value::Null: {
                result += "null";
                break;
            }
            case yaml_value::Boolean: {
                result += to_string(value->as_boolean()->get_value());
                break;
            }
            case yaml_value::Integer: {
                result += to_string(value->as_integer()->get_value());
                break;
            }
            case yaml_value::Float: {
                result += to_string(value->as_float()->get_value());
                break;
            }
            case yaml_value::String: {
                const auto* str = value->as_string();
                const string& val = str->get_value();

                if (str->get_style() == yaml_string::SingleQuoted) {
                    result += "'" + val + "'";
                } else if (str->get_style() == yaml_string::DoubleQuoted || needs_quotes(val)) {
                    result += "\"" + escape(val) + "\"";
                } else if (str->get_style() == yaml_string::Literal) {
                    result += "|\n";
                    string line;
                    size_t pos = 0;
                    while (getline(val, pos, line)) {
                        result += string(indent, ' ') + line + "\n";
                    }
                } else if (str->get_style() == yaml_string::Folded) {
                    result += ">\n";
                    const string indent_str(indent, ' ');
                    result += move(indent_str) + val + "\n";
                } else {
                    result += val;
                }
                break;
            }
            case yaml_value::Timestamp: {
                result += value->as_timestamp()->get_string_value();
                break;
            }
            case yaml_value::Sequence: {
                stringify_sequence(result, value->as_sequence(), indent, inline_context);
                break;
            }
            case yaml_value::Mapping: {
                if (!inline_context) {
                    result += "\n";
                }
                stringify_mapping(result, value->as_mapping(), indent, inline_context);
                break;
            }
        }
    }
}


string yaml_value::to_string() const {
    string result;
    stringify_value(result, this, 0, true);
    return result;
}

string yaml_value::to_document() const {
    string result;

    if (is_sequence() || is_mapping()) {
        stringify_value(result, this, 0, false);
    } else {
        stringify_value(result, this, 0, true);
    }

    result += "\n";
    return result;
}

NEFORCE_END_NAMESPACE__
