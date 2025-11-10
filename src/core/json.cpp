#include <MSTL/core/json.hpp>
#include <MSTL/core/undef_cmacro.hpp>
MSTL_BEGIN_NAMESPACE__

unique_ptr<json_string> json_parser::parse_string() {
    pos++;
    string result;
    bool escaped = false;

    while (pos < length) {
        const char c = json[pos++];
        if (escaped) {
            escaped = false;
            switch (c) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += c;    break;
            }
        } else if (c == '"') {
            return make_unique<json_string>(result);
        } else if (c == '\\') {
            escaped = true;
        } else {
            result += c;
        }
    }
    Exception(JsonOperateError("Unterminated string"));
    return make_unique<json_string>("");
}

unique_ptr<json_number> json_parser::parse_number() {
    const size_t start = pos;
    if (current() == '-') {
        pos++;
    }
    if (current() == '0') {
        pos++;
        if (pos < length && json[pos] == '.') {
            pos++;
            if (pos >= length || !_MSTL is_digit(json[pos])) {
                Exception(JsonOperateError("Invalid decimal part"));
            }
            while (pos < length && _MSTL is_digit(json[pos])) {
                pos++;
            }
        }
    } else if (_MSTL is_digit(current())) {
        while (pos < length && _MSTL is_digit(json[pos])) {
            pos++;
        }
        if (pos < length && json[pos] == '.') {
            pos++;
            if (pos >= length || !_MSTL is_digit(json[pos])) {
                Exception(JsonOperateError("Invalid decimal part"));
            }
            while (pos < length && _MSTL is_digit(json[pos])) {
                pos++;
            }
        }
    } else {
        Exception(JsonOperateError("Invalid number format"));
    }

    if (pos < length && (json[pos] == 'e' || json[pos] == 'E')) {
        pos++;
        if (pos < length && (json[pos] == '+' || json[pos] == '-')) {
            pos++;
        }
        if (pos >= length || !_MSTL is_digit(json[pos])) {
            Exception(JsonOperateError("Invalid exponent part"));
        }
        while (pos < length && _MSTL is_digit(json[pos])) {
            pos++;
        }
    }

    try {
        double value = float64::parse(json.view(start, pos - start));
        return make_unique<json_number>(value);
    } catch (...) {
        Exception(JsonOperateError("Invalid number value"));
    }
    return make_unique<json_number>(numeric_limits<float64_t>::max());
}

unique_ptr<json_value> json_parser::parse_keyword() {
    const size_t start = pos;
    while (pos < length && _MSTL is_alpha(json[pos])) {
        pos++;
    }

    const string_view keyword = json.view(start, pos - start);
    if (keyword == "true") {
        return make_unique<json_bool>(true);
    } else if (keyword == "false") {
        return make_unique<json_bool>(false);
    } else if (keyword == "null") {
    } else {
        Exception(JsonOperateError("Invalid keyword"));
    }
    return make_unique<json_null>();
}

unique_ptr<json_array> json_parser::parse_array() {
    pos++;
    auto array = make_unique<json_array>();

    skip_space();
    if (current() == ']') {
        pos++;
        return array;
    }

    while (true) {
        skip_space();
        auto element = parse_value();
        array->add_element(_MSTL move(element));
        skip_space();

        if (current() == ']') {
            pos++;
            break;
        } else if (current() == ',') {
            pos++;
            skip_space();
        } else {
            Exception(JsonOperateError("Expected comma or closing bracket in array"));
        }
    }
    return array;
}

unique_ptr<json_object> json_parser::parse_object() {
    pos++;
    auto object = make_unique<json_object>();

    skip_space();
    if (current() == '}') {
        pos++;
        return object;
    }

    while (true) {
        skip_space();
        if (current() != '"') {
            Exception(JsonOperateError("Expected string key in object"));
        }
        const auto key_obj = parse_string();
        string key = key_obj->get_value();

        skip_space();
        if (current() != ':') {
            Exception(JsonOperateError("Expected colon after key in object"));
        }
        pos++;
        skip_space();

        auto value = parse_value();
        object->add_member(key, _MSTL move(value));

        skip_space();
        if (current() == '}') {
            pos++;
            break;
        } else if (current() == ',') {
            pos++;
            skip_space();
        } else {
            Exception(JsonOperateError("Expected comma or closing brace in object"));
        }
    }

    return object;
}

unique_ptr<json_value> json_parser::parse_value() {
    skip_space();
    if (eof()) {
        Exception(JsonOperateError("Unexpected end of input"));
    }

    const char c = current();
    switch (c) {
        case '{':
            return parse_object();
        case '[':
            return parse_array();
        case '"':
            return parse_string();
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number();
        case 't': case 'f': case 'n':
            return parse_keyword();
        default:
            Exception(JsonOperateError("Unexpected character"));
    }
    return parse_object();
}


unique_ptr<json_value> json_parser::parse() {
    auto value = parse_value();
    skip_space();
    if (!eof()) {
        Exception(JsonOperateError("Unexpected characters after JSON value"));
    }
    return value;
}

optional<unique_ptr<json_value>> json_parser::try_parse() {
    optional<unique_ptr<json_value>>value (nullopt);
    try {
        value = _MSTL move(parse());
    } catch (const Error&) {
        return value;
    }
    return _MSTL move(value);
}


json_builder& json_builder::begin_object() {
    auto new_object = make_unique<json_object>();
    json_object* obj_ptr = new_object.get();

    if (contexts.empty()) {
        if (root) {
            Exception(JsonOperateError("Root value already set"));
        }
        root = _MSTL move(new_object);
    } else {
        const auto & current = contexts.top();
        if (current.type == ARRAY) {
            current.array_ptr->add_element(_MSTL move(new_object));
        } else if (current.type == OBJECT) {
            if (current_key.empty()) {
                Exception(JsonOperateError("No key set for object value"));
            }
            current.object_ptr->add_member(current_key, _MSTL move(new_object));
            current_key.clear();
        }
    }

    contexts.push(frame(OBJECT, obj_ptr));
    return *this;
}

json_builder& json_builder::begin_array() {
    auto new_array = make_unique<json_array>();
    json_array* arr_ptr = new_array.get();

    if (contexts.empty()) {
        if (root) {
            Exception(JsonOperateError("Root value already set"));
        }
        root = _MSTL move(new_array);
    } else {
        const auto & current = contexts.top();
        if (current.type == ARRAY) {
            current.array_ptr->add_element(_MSTL move(new_array));
        } else if (current.type == OBJECT) {
            if (current_key.empty()) {
                Exception(JsonOperateError("No key set for array value"));
            }
            current.object_ptr->add_member(current_key, _MSTL move(new_array));
            current_key.clear();
        }
    }

    contexts.push(frame(ARRAY, arr_ptr));
    return *this;
}

json_builder& json_builder::end_object() {
    if (contexts.empty() || contexts.top().type != OBJECT) {
        Exception(JsonOperateError("No object to close or context mismatch"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Incomplete key-value pair in object"));
    }
    contexts.pop();
    return *this;
}

json_builder& json_builder::end_array() {
    if (contexts.empty() || contexts.top().type != ARRAY) {
        Exception(JsonOperateError("No array to close or context mismatch"));
    }
    contexts.pop();
    return *this;
}

json_builder& json_builder::key(const string& k) {
    if (contexts.empty() || contexts.top().type != OBJECT) {
        Exception(JsonOperateError("Key can only be set inside an object"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Key already set without corresponding value"));
    }
    current_key = k;
    return *this;
}

json_builder& json_builder::value_object(_MSTL function<void(json_builder&)>&& build_func) {
    json_builder inner_builder;
    inner_builder.begin_object();
    build_func(inner_builder);
    inner_builder.end_object();
    auto obj = inner_builder.build();
    return value_impl(_MSTL move(obj));
}

json_builder& json_builder::value_array(_MSTL function<void(json_builder&)>&& build_func) {
    json_builder inner_builder;
    inner_builder.begin_array();
    build_func(inner_builder);
    inner_builder.end_array();
    auto arr = inner_builder.build();
    return value_impl(_MSTL move(arr));
}

unique_ptr<json_value> json_builder::build() {
    if (!contexts.empty()) {
        Exception(JsonOperateError("Incomplete JSON structure - unclosed objects or arrays"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Incomplete key-value pair"));
    }
    if (!root) {
        Exception(JsonOperateError("No JSON value built"));
    }
    return _MSTL move(root);
}


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

string json_value_to_indent_string(const json_value* value, int indent) {
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
