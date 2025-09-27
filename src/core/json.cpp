#include <MSTL/core/json.hpp>
MSTL_BEGIN_NAMESPACE__

const json_null* json_value::as_null() const noexcept { return nullptr; }
const json_bool* json_value::as_bool() const noexcept { return nullptr; }
const json_number* json_value::as_number() const noexcept { return nullptr; }
const json_string* json_value::as_string() const noexcept { return nullptr; }
const json_object* json_value::as_object() const noexcept { return nullptr; }
const json_array* json_value::as_array() const noexcept { return nullptr; }


MSTL_NODISCARD bool json_value::is_null() const noexcept { return type() == Null; }
MSTL_NODISCARD bool json_value::is_bool() const noexcept { return type() == Bool; }
MSTL_NODISCARD bool json_value::is_number() const noexcept { return type() == Number; }
MSTL_NODISCARD bool json_value::is_string() const noexcept { return type() == String; }
MSTL_NODISCARD bool json_value::is_object() const noexcept { return type() == Object; }
MSTL_NODISCARD bool json_value::is_array() const noexcept { return type() == Array; }


json_null::types json_null::type() const noexcept { return Null; }
const json_null* json_null::as_null() const noexcept { return this; }


json_bool::json_bool(const bool v) noexcept : value(v) {}
json_bool::types json_bool::type() const noexcept { return Bool; }
const json_bool* json_bool::as_bool() const noexcept { return this; }
bool json_bool::get_value() const noexcept { return value; }


json_number::json_number(const double v) noexcept : value(v) {}
json_number::types json_number::type() const noexcept { return Number; }
const json_number* json_number::as_number() const noexcept { return this; }
double json_number::get_value() const noexcept { return value; }


json_string::json_string(string v) noexcept : value(_MSTL move(v)) {}
json_string::types json_string::type() const noexcept { return String; }
const json_string* json_string::as_string() const noexcept { return this; }
const string& json_string::get_value() const noexcept { return value; }


json_object::types json_object::type() const noexcept { return Object; }
const json_object* json_object::as_object() const noexcept { return this; }

void json_object::add_member(const string& key, unique_ptr<json_value> value) {
    members[key] = _MSTL move(value);
}

const json_value* json_object::get_member(const string& key) const {
    auto it = members.find(key);
    if (it != members.end()) {
        return it->second.get();
    }
    return nullptr;
}

const unordered_map<string, unique_ptr<json_value>>&
json_object::get_members() const noexcept {
    return members;
}


json_array::types json_array::type() const noexcept { return Array; }
const json_array* json_array::as_array() const noexcept { return this; }

void json_array::add_element(unique_ptr<json_value> value) {
    elements.emplace_back(_MSTL move(value));
}

const json_value* json_array::get_element(const size_t index) const noexcept {
    if (index < elements.size()) {
        return elements[index].get();
    }
    return nullptr;
}

size_t json_array::size() const noexcept { return elements.size(); }

const vector<unique_ptr<json_value>>& json_array::get_elements() const noexcept {
    return elements;
}


void json_parser::skip_space() noexcept {
    while (pos < length && _MSTL is_space(json[pos])) {
        pos++;
    }
}

char json_parser::current() const noexcept {
    if (pos < length) {
        return json[pos];
    }
    return '\0';
}

bool json_parser::eof() const noexcept {
    return pos >= length;
}

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
            if (pos >= length || !isdigit(json[pos])) {
                Exception(JsonOperateError("Invalid decimal part"));
            }
            while (pos < length && isdigit(json[pos])) {
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
        if (pos >= length || !isdigit(json[pos])) {
            Exception(JsonOperateError("Invalid exponent part"));
        }
        while (pos < length && isdigit(json[pos])) {
            pos++;
        }
    }

    string num_str = json.substr(start, pos - start);
    try {
        double value = _MSTL to_float64(num_str.data());
        return make_unique<json_number>(value);
    } catch (...) {
        Exception(JsonOperateError("Invalid number value"));
    }
    return make_unique<json_number>(FLOAT64_MAX_POSI_SIZE);
}

unique_ptr<json_value> json_parser::parse_keyword() {
    const size_t start = pos;
    while (pos < length && isalpha(json[pos])) {
        pos++;
    }

    string keyword = json.substr(start, pos - start);
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
        auto key_obj = parse_string();
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

json_parser::json_parser(string json_str) noexcept
    : json(_MSTL move(json_str)), pos(0), length(json.size()) {}

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


json_builder::frame::frame(const types t, json_object* obj) : type(t), object_ptr(obj) {}
json_builder::frame::frame(const types t, json_array* arr) : type(t), array_ptr(arr) {}

json_builder& json_builder::begin_object() {
    auto new_object = make_unique<json_object>();
    json_object* obj_ptr = new_object.get();

    if (contexts.empty()) {
        if (root) {
            Exception(JsonOperateError("Root value already set"));
        }
        root = _MSTL move(new_object);
    } else {
        auto& current = contexts.top();
        if (current.type == Array) {
            current.array_ptr->add_element(_MSTL move(new_object));
        } else if (current.type == Object) {
            if (current_key.empty()) {
                Exception(JsonOperateError("No key set for object value"));
            }
            current.object_ptr->add_member(current_key, _MSTL move(new_object));
            current_key.clear();
        }
    }

    contexts.push(frame(Object, obj_ptr));
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
        auto& current = contexts.top();
        if (current.type == Array) {
            current.array_ptr->add_element(_MSTL move(new_array));
        } else if (current.type == Object) {
            if (current_key.empty()) {
                Exception(JsonOperateError("No key set for array value"));
            }
            current.object_ptr->add_member(current_key, _MSTL move(new_array));
            current_key.clear();
        }
    }

    contexts.push(frame(Array, arr_ptr));
    return *this;
}

json_builder& json_builder::end_object() {
    if (contexts.empty() || contexts.top().type != Object) {
        Exception(JsonOperateError("No object to close or context mismatch"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Incomplete key-value pair in object"));
    }
    contexts.pop();
    return *this;
}

json_builder& json_builder::end_array() {
    if (contexts.empty() || contexts.top().type != Array) {
        Exception(JsonOperateError("No array to close or context mismatch"));
    }
    contexts.pop();
    return *this;
}

json_builder& json_builder::key(const string& k) {
    if (contexts.empty() || contexts.top().type != Object) {
        Exception(JsonOperateError("Key can only be set inside an object"));
    }
    if (!current_key.empty()) {
        Exception(JsonOperateError("Key already set without corresponding value"));
    }
    current_key = k;
    return *this;
}

json_builder& json_builder::value(nullptr_t) {
    return value_impl(make_unique<json_null>());
}
json_builder& json_builder::value(const string& v) {
    return value_impl(make_unique<json_string>(v));
}
json_builder& json_builder::value(const char* v) {
    return value(string(v));
}
json_builder& json_builder::value(const string_view& v) {
    return value(string(v));
}
json_builder& json_builder::value(const double v) {
    return value_impl(make_unique<json_number>(v));
}
json_builder& json_builder::value(const int v) {
    return value_impl(make_unique<json_number>(static_cast<double>(v)));
}
json_builder& json_builder::value(const bool v) {
    return value_impl(make_unique<json_bool>(v));
}
json_builder& json_builder::value(unique_ptr<json_value>&& v) {
    return value_impl(_MSTL move(v));
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


static string json_value_to_string(const json_value* value) {
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
                val >= static_cast<double>(INT64_MIN_SIZE) &&
                val <= static_cast<double>(INT64_MAX_SIZE)) {
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
            return "\"" + escape_string(str_val->get_value()) + "\"";
        }
        case json_value::Array: {
            const json_array* arr_val = value->as_array();
            string result = "[";

            const auto& elements = arr_val->get_elements();
            for (size_t i = 0; i < elements.size(); ++i) {
                if (i > 0) {
                    result += ",";
                }
                result += json_value_to_string(elements[i].get());
            }

            result += "]";
            return result;
        }
        case json_value::Object: {
            const json_object* obj_val = value->as_object();
            string result = "{";

            const auto& members = obj_val->get_members();
            bool first = true;

            for (const auto& pair : members) {
                if (!first) {
                    result += ",";
                }
                first = false;

                result += "\"" + escape_string(pair.first) + "\":";
                result += json_value_to_string(pair.second.get());
            }

            result += "}";
            return result;
        }
        default: return "null";
    }
}

string json_to_string(const unique_ptr<json_value>& value) {
    return json_value_to_string(value.get());
}
string json_to_string(unique_ptr<json_value>&& value) {
    string result = json_value_to_string(value.get());
    return _MSTL move(result);
}
string json_to_string(const json_value* value) {
    return json_value_to_string(value);
}

MSTL_END_NAMESPACE__
