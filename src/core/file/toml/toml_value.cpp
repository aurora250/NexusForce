#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/file/toml/toml_value.hpp>
#include <MSTL/core/numeric/numeric_types.hpp>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

string MSTL_API toml_value_to_string(const toml_value* value) {
    if (!value) return "";

    switch (value->type()) {
        case toml_value::Boolean: {
            return _MSTL to_string(value->as_boolean()->get_value());
        }
        case toml_value::Integer: {
            return _MSTL to_string(value->as_integer()->get_value());
        }
        case toml_value::Float: {
            return _MSTL to_string(value->as_float()->get_value());
        }
        case toml_value::String: {
            const toml_string* str_val = value->as_string();
            const string& str = str_val->get_value();
            const toml_string::string_type str_type = str_val->get_string_type();

            switch (str_type) {
                case toml_string::Basic: {
                    return "\"" + _MSTL escape(str) + "\"";
                }
                case toml_string::Literal: {
                    string lit;
                    lit += '\'';
                    for (const char c : str) {
                        if (c == '\'') lit += "''";
                        else lit += c;
                    }
                    lit += '\'';
                    return lit;
                }
                case toml_string::MultiBasic: {
                    string escaped = _MSTL escape(str);
                    if (!escaped.empty() && escaped.back() == '"') {
                        escaped.pop_back();
                        escaped += "\\\"";
                    }
                    return "\"\"\"" + escaped + "\"\"\"";
                }
                case toml_string::MultiLiteral: {
                    return "'''" + str + "'''";
                }
                default: {
                    return "\"" + _MSTL escape(str) + "\"";
                }
            }
        }
        case toml_value::DateTime: {
            return value->as_datetime()->get_string_value();
        }
        case toml_value::Array: {
            const toml_array* arr = value->as_array();
            string result = "[";
            for (size_t i = 0; i < arr->size(); ++i) {
                if (i > 0) result += ", ";
                result += toml_value_to_string(arr->get_element(i));
            }
            result += "]";
            return result;
        }
        case toml_value::Table: {
            const toml_table* table = value->as_table();
            if (table->is_inline()) {
                string result = "{ ";
                bool first = true;
                for (const auto& elm : table->get_members()) {
                    const auto& key = elm.first;
                    const auto& val = elm.second;
                    if (!first) result += ", ";
                    else first = false;

                    bool needs_quotes = false;
                    for (const char c : key) {
                        if (!is_alpha_or_digit(c) && c != '_' && c != '-') {
                            needs_quotes = true;
                            break;
                        }
                    }

                    if (needs_quotes) {
                        result += "\"" + _MSTL escape(key) + "\"";
                    } else {
                        result += key;
                    }
                    result += " = " + toml_value_to_string(val.get());
                }
                result += " }";
                return result;
            } else {
                return "{ /* non-inline table */ }";
            }
        }
        default: {
            return "unknown";
        }
    }
}

static MSTL_ALWAYS_INLINE_INLINE string toml_quote_key_if_needed(const string& key) {
    bool needs_quotes = false;
    for (const char c : key) {
        if (!is_alpha_or_digit(c) && c != '_' && c != '-') {
            needs_quotes = true;
            break;
        }
    }

    if (needs_quotes) {
        return "\"" + _MSTL escape(key) + "\"";
    }
    return key;
}

static string toml_table_to_string_with_path(const toml_table* table, const string& path_prefix = "") {
    if (!table) return "";

    string result;
    vector<pair<string, const toml_value*>> ordinary_members;
    vector<pair<string, const toml_table*>> nested_tables;
    vector<pair<string, const toml_array*>> array_tables;

    for (const auto& elm : table->get_members()) {
        const auto& key = elm.first;
        const auto& val = elm.second;

        if (val->is_array()) {
            const toml_array* arr = val->as_array();
            bool is_array_of_tables = true;

            if (arr->size() > 0) {
                for (size_t i = 0; i < arr->size(); ++i) {
                    if (!arr->get_element(i)->is_table()) {
                        is_array_of_tables = false;
                        break;
                    }
                }
            } else {
                is_array_of_tables = false;
            }

            if (is_array_of_tables) {
                array_tables.emplace_back(key, arr);
                continue;
            }
        }

        if (val->is_table()) {
            const toml_table* nested = val->as_table();
            if (!nested->is_inline()) {
                nested_tables.emplace_back(key, nested);
                continue;
            }
        }
        ordinary_members.emplace_back(key, val.get());
    }

    for (const auto& elm : ordinary_members) {
        const auto& key = elm.first;
        const auto& val = elm.second;
        result += toml_quote_key_if_needed(key);
        result += " = ";
        result += toml_value_document(val);
        result += "\n";
    }

    for (const auto& elm : nested_tables) {
        const auto& key = elm.first;
        const auto& nested = elm.second;
        if (!result.empty() && result.back() != '\n') {
            result += "\n";
        }

        string key_str = toml_quote_key_if_needed(key);
        string full_path = path_prefix.empty() ? key_str : path_prefix + "." + key_str;

        result += "\n[" + full_path + "]\n";
        result += toml_table_to_string_with_path(nested, full_path);
    }

    for (const auto& elm : array_tables) {
        const auto& key = elm.first;
        const auto& arr = elm.second;
        string key_str = toml_quote_key_if_needed(key);
        string full_path = path_prefix.empty() ? key_str : path_prefix + "." + key_str;

        for (size_t i = 0; i < arr->size(); ++i) {
            const toml_table* tbl = arr->get_element(i)->as_table();
            if (!result.empty() && result.back() != '\n') {
                result += "\n";
            }
            result += "\n[[" + full_path + "]]\n";
            result += toml_table_to_string_with_path(tbl, full_path);
        }
    }

    return result;
}

string MSTL_API toml_value_document(const toml_value* value) {
    if (!value) return "";

    switch (value->type()) {
        case toml_value::Boolean: {
            return _MSTL to_string(value->as_boolean()->get_value());
        }
        case toml_value::Integer: {
            return _MSTL to_string(value->as_integer()->get_value());
        }
        case toml_value::Float: {
            const double val = value->as_float()->get_value();
            if (is_nan(val)) {
                return "nan";
            } else if (is_infinity(val)) {
                return _MSTL signbit(val) ? "-inf" : "inf";
            } else {
                return _MSTL to_string(val);
            }
        }
        case toml_value::String: {
            const toml_string* str_val = value->as_string();
            const string& str = str_val->get_value();
            const toml_string::string_type str_type = str_val->get_string_type();

            switch (str_type) {
                case toml_string::Basic: {
                    return "\"" + _MSTL escape(str) + "\"";
                }
                case toml_string::Literal: {
                    string lit = "'";
                    for (char c : str) {
                        if (c == '\'') lit += "''";
                        else lit += c;
                    }
                    lit += "'";
                    return lit;
                }
                case toml_string::MultiBasic: {
                    string escaped = _MSTL escape(str);
                    if (!escaped.empty() && escaped.back() == '"') {
                        escaped.pop_back();
                        escaped += "\\\"";
                    }
                    return "\"\"\"" + escaped + "\"\"\"";
                }
                case toml_string::MultiLiteral: {
                    return "'''" + str + "'''";
                }
                default: {
                    return "\"" + _MSTL escape(str) + "\"";
                }
            }
        }
        case toml_value::DateTime: {
            return value->as_datetime()->get_string_value();
        }
        case toml_value::Array: {
            const toml_array* arr = value->as_array();
            string result = "[";
            for (size_t i = 0; i < arr->size(); ++i) {
                if (i > 0) result += ", ";
                result += toml_value_document(arr->get_element(i));
            }
            result += "]";
            return result;
        }
        case toml_value::Table: {
            const toml_table* table = value->as_table();

            if (table->is_inline()) {
                string result = "{ ";
                bool first = true;
                for (const auto& elm : table->get_members()) {
                    const auto& key = elm.first;
                    const auto& val = elm.second;
                    if (!first) result += ", ";
                    else first = false;

                    result += toml_quote_key_if_needed(key);
                    result += " = " + toml_value_document(val.get());
                }
                result += " }";
                return result;
            } else {
                return toml_table_to_string_with_path(table);
            }
        }
        default: {
            return "unknown";
        }
    }
}

MSTL_END_INNER__

MSTL_END_NAMESPACE__
