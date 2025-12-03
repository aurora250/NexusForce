#include <MSTL/core/json/json_value.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

string json_value_to_string(const json_value* value) {
    if (!value) {
        return "null";
    }
    switch (value->type()) {
        case json_value::Null: {
            return "null";
        }
        case json_value::Bool: {
            const json_bool* bool_val = value->as_bool();
            return bool_val->get_value() ? "true" : "false";
        }
        case json_value::Number: {
            const json_number* num_val = value->as_number();
            const double val = num_val->get_value();

            if (val == static_cast<double>(static_cast<long long>(val)) &&
                val >= static_cast<double>(numeric_limits<int64_t>::min()) &&
                val <= static_cast<double>(numeric_limits<int64_t>::max())) {
                return _MSTL to_string(static_cast<long long>(val));
            }
            string result = _MSTL to_string(val);
            if (result.find('.') != string::npos) {
                while (!result.empty() && result.back() == '0') {
                    result.pop_back();
                }
                if (!result.empty() && result.back() == '.') {
                    result.pop_back();
                }
            }
            return result;
        }
        case json_value::String: {
            const json_string* str_val = value->as_string();
            return "\"" + escape(str_val->get_value()) + "\"";
        }
        case json_value::Array: {
            const json_array* arr_val = value->as_array();
            const auto& elements = arr_val->get_elements();
            if (elements.empty()) return "[]";

            string result = "[";
            for (size_t i = 0; i < elements.size(); ++i) {
                result += json_value_to_string(elements[i].get());
                if (i != elements.size() - 1) result += ",";
            }
            result += "]";
            return result;
        }
        case json_value::Object: {
            const json_object* obj_val = value->as_object();
            const auto& members = obj_val->get_members();
            if (members.empty()) return "{}";

            string result = "{";
            size_t count = 0;
            for (const auto& pair : members) {
                result += "\"" + escape(pair.first) + "\":";
                result += json_value_to_string(pair.second.get());

                if (count != members.size() - 1) result += ",";
                count++;

            }
            result += "}";
            return result;
        }
        default: return "null";
    }
}

static string indent_str(const int indent) {
    return string(indent, ' ');
}

string json_value_to_indent_string(const json_value* value, const int indent) {
    if (!value) {
        return "null";
    }
    switch (value->type()) {
        case json_value::Null: {
            return "null";
        }
        case json_value::Bool: {
            const json_bool* bool_val = value->as_bool();
            return to_string(bool_val->get_value());
        }
        case json_value::Number: {
            const json_number* num_val = value->as_number();
            const double val = num_val->get_value();

            if (val == static_cast<double>(static_cast<long long>(val)) &&
                val >= static_cast<double>(numeric_limits<int64_t>::min()) &&
                val <= static_cast<double>(numeric_limits<int64_t>::max())) {
                return to_string(static_cast<long long>(val));
            }
            string result = _MSTL to_string(val);
            if (result.find('.') != string::npos) {
                while (!result.empty() && result.back() == '0') {
                    result.pop_back();
                }
                if (!result.empty() && result.back() == '.') {
                    result.pop_back();
                }
            }
            return result;
        }
        case json_value::String: {
            const json_string* str_val = value->as_string();
            return "\"" + _MSTL escape(str_val->get_value()) + "\"";
        }
        case json_value::Array: {
            const json_array* arr_val = value->as_array();
            const auto& elements = arr_val->get_elements();
            if (elements.empty()) return "[]";

            string result = "[\n";
            const int child_indent = indent + 2;
            for (size_t i = 0; i < elements.size(); ++i) {
                result += indent_str(child_indent);
                result += json_value_to_indent_string(elements[i].get(), child_indent);
                if (i != elements.size() - 1) result += ",";
                result += "\n";
            }
            result += indent_str(indent) + "]";
            return result;
        }
        case json_value::Object: {
            const json_object* obj_val = value->as_object();
            const auto& members = obj_val->get_members();
            if (members.empty()) return "{}";

            string result = "{\n";
            const int child_indent = indent + 2;
            size_t count = 0;
            for (const auto& pair : members) {
                result += indent_str(child_indent);
                result += "\"" + escape(pair.first) + "\":";
                result += json_value_to_indent_string(pair.second.get(), child_indent);

                if (count != members.size() - 1) result += ",";
                result += "\n";
                count++;
            }
            result += indent_str(indent) + "}";
            return result;
        }
        default: return "null";
    }
}

MSTL_END_INNER__
MSTL_END_NAMESPACE__
