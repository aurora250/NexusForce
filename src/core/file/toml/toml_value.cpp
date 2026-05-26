#include <NeForce/core/file/toml/toml_value.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string toml_value_to_string(const toml_value* value) {
        if (value == nullptr) {
            return "";
        }

        switch (value->type()) {
            //  Scalar directly converts to strings
            case toml_value::Boolean: {
                return to_string(value->as_boolean()->get_value());
            }
            case toml_value::Integer: {
                return to_string(value->as_integer()->get_value());
            }
            case toml_value::Float: {
                return to_string(value->as_float()->get_value());
            }
            // String types are escaped wrapped according to the four TOML string formats
            case toml_value::String: {
                const toml_string* str_val = value->as_string();
                const string& str = str_val->get_value();
                const toml_string::string_type str_type = str_val->get_string_type();

                switch (str_type) {
                    case toml_string::Basic: {
                        return "\"" + escape(str) + "\"";
                    }
                    case toml_string::Literal: {
                        string lit;
                        lit += '\'';
                        for (const char c: str) {
                            if (c == '\'') {
                                lit += "''";
                            } else {
                                lit += c;
                            }
                        }
                        lit += '\'';
                        return lit;
                    }
                    case toml_string::MultiBasic: {
                        string escaped = escape(str);
                        if (!escaped.empty() && escaped.back() == '"') {
                            escaped.pop_back();
                            escaped += "\\\"";
                        }
                        return R"(""")" + escaped + R"(""")";
                    }
                    case toml_string::MultiLiteral: {
                        return "'''" + str + "'''";
                    }
                    default: {
                        return "\"" + escape(str) + "\"";
                    }
                }
            }
            // Date uses its preformatted string value
            case toml_value::DateTime: {
                return value->as_datetime()->get_string_value();
            }
            // Arrays reprocess each element recursively
            case toml_value::Array: {
                const toml_array* arr = value->as_array();
                string result = "[";
                for (size_t i = 0; i < arr->size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += toml_value_to_string(arr->get_element(i));
                }
                result += "]";
                return result;
            }
            case toml_value::Table: {
                const toml_table* table = value->as_table();
                // Inline table
                if (table->is_inline()) {
                    string result = "{ ";
                    bool first = true;
                    for (const auto& elm: table->get_members()) {
                        const auto& key = elm.first;
                        const auto& val = elm.second;
                        if (!first) {
                            result += ", ";
                        } else {
                            first = false;
                        }

                        bool needs_quotes = false;
                        for (const char c: key) {
                            if (!is_alpha_or_digit(c) && c != '_' && c != '-') {
                                needs_quotes = true;
                                break;
                            }
                        }

                        if (needs_quotes) {
                            result += "\"" + escape(key) + "\"";
                        } else {
                            result += key;
                        }
                        result += " = " + toml_value_to_string(val.get());
                    }
                    result += " }";
                    return result;
                }
                // Non-inline tables are not elaborated in this context
                return "{ /* non-inline table */ }";
            }
            default: {
                return "unknown";
            }
        }
    }

    string toml_quote_key_if_needed(const string& key) {
        bool needs_quotes = false;
        for (const char c: key) {
            if (!is_alpha_or_digit(c) && c != '_' && c != '-') {
                needs_quotes = true;
                break;
            }
        }

        if (needs_quotes) {
            return "\"" + escape(key) + "\"";
        }
        return key;
    }

    string toml_value_document(const toml_value* value);

    string toml_table_to_string_with_path(const toml_table* table, const string& path_prefix = "") {
        if (table == nullptr) {
            return "";
        }

        string result;
        vector<pair<string, const toml_value*>> ordinary_members;
        vector<pair<string, const toml_table*>> nested_tables;
        vector<pair<string, const toml_array*>> array_tables;

        for (const auto& elm: table->get_members()) {
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

        for (const auto& elm: ordinary_members) {
            const auto& key = elm.first;
            const auto& val = elm.second;
            result += toml_quote_key_if_needed(key);
            result += " = ";
            result += toml_value_document(val);
            result += "\n";
        }

        for (const auto& elm: nested_tables) {
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

        for (const auto& elm: array_tables) {
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

    string toml_value_document(const toml_value* value) {
        if (value == nullptr) {
            return "";
        }

        switch (value->type()) {
            case toml_value::Boolean: {
                return to_string(value->as_boolean()->get_value());
            }
            case toml_value::Integer: {
                return to_string(value->as_integer()->get_value());
            }
            case toml_value::Float: {
                return to_string(value->as_float()->get_value());
            }
            case toml_value::String: {
                const toml_string* str_val = value->as_string();
                const string& str = str_val->get_value();
                const toml_string::string_type str_type = str_val->get_string_type();

                switch (str_type) {
                    case toml_string::Basic: {
                        return "\"" + escape(str) + "\"";
                    }
                    case toml_string::Literal: {
                        string lit = "'";
                        for (char c: str) {
                            if (c == '\'') {
                                lit += "''";
                            } else {
                                lit += c;
                            }
                        }
                        lit += "'";
                        return lit;
                    }
                    case toml_string::MultiBasic: {
                        string escaped = escape(str);
                        // Prevent the double quotes at the end from being confused with multi-line terminators
                        if (!escaped.empty() && escaped.back() == '"') {
                            escaped.pop_back();
                            escaped += R"(\")";
                        }
                        return R"(""")" + escaped + R"(""")";
                    }
                    case toml_string::MultiLiteral: {
                        return "'''" + str + "'''";
                    }
                    default: {
                        return "\"" + escape(str) + "\"";
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
                    if (i > 0) {
                        result += ", ";
                    }
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
                    for (const auto& elm: table->get_members()) {
                        const auto& key = elm.first;
                        const auto& val = elm.second;
                        if (!first) {
                            result += ", ";
                        } else {
                            first = false;
                        }

                        result += toml_quote_key_if_needed(key);
                        result += " = " + toml_value_document(val.get());
                    }
                    result += " }";
                    return result;
                }
                // Non-inline tables are delegated to recursive expansion
                return toml_table_to_string_with_path(table);
            }
            default: {
                return "unknown";
            }
        }
    }
} // namespace


string toml_value::to_string() const { return toml_value_to_string(this); }

string toml_value::to_document() const { return toml_value_document(this); }

NEFORCE_END_NAMESPACE__
