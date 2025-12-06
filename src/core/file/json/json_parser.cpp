#include <MSTL/core/file/json/json_parser.hpp>
#include <MSTL/core/utility/packages.hpp>
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
    throw_exception(json_exception("Unterminated string"));
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
                throw_exception(json_exception("Invalid decimal part"));
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
                throw_exception(json_exception("Invalid decimal part"));
            }
            while (pos < length && _MSTL is_digit(json[pos])) {
                pos++;
            }
        }
    } else {
        throw_exception(json_exception("Invalid number format"));
    }

    if (pos < length && (json[pos] == 'e' || json[pos] == 'E')) {
        pos++;
        if (pos < length && (json[pos] == '+' || json[pos] == '-')) {
            pos++;
        }
        if (pos >= length || !_MSTL is_digit(json[pos])) {
            throw_exception(json_exception("Invalid exponent part"));
        }
        while (pos < length && _MSTL is_digit(json[pos])) {
            pos++;
        }
    }

    try {
        double value = float64::parse(json.view(start, pos - start));
        return make_unique<json_number>(value);
    } catch (...) {
        throw_exception(json_exception("Invalid number value"));
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
        throw_exception(json_exception("Invalid keyword"));
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
            throw_exception(json_exception("Expected comma or closing bracket in array"));
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
            throw_exception(json_exception("Expected string key in object"));
        }
        const auto key_obj = parse_string();
        string key = key_obj->get_value();

        skip_space();
        if (current() != ':') {
            throw_exception(json_exception("Expected colon after key in object"));
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
            throw_exception(json_exception("Expected comma or closing brace in object"));
        }
    }

    return object;
}

unique_ptr<json_value> json_parser::parse_value() {
    skip_space();
    if (eof()) {
        throw_exception(json_exception("Unexpected end of input"));
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
            throw_exception(json_exception("Unexpected character"));
    }
    return parse_object();
}


unique_ptr<json_value> json_parser::parse() {
    auto value = parse_value();
    skip_space();
    if (!eof()) {
        throw_exception(json_exception("Unexpected characters after JSON value"));
    }
    return value;
}

optional<unique_ptr<json_value>> json_parser::try_parse() {
    optional<unique_ptr<json_value>>value (nullopt);
    try {
        value = _MSTL move(parse());
    } catch (const exception&) {
        return value;
    }
    return _MSTL move(value);
}

MSTL_END_NAMESPACE__
