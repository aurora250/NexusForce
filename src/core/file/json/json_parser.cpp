#include <NeForce/core/utility/packages.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
NEFORCE_BEGIN_NAMESPACE__

void json_parser::skip_space() noexcept {
    while (pos_ < len_ && is_space(text_[pos_])) {
        pos_++;
    }
}

char json_parser::current() const noexcept {
    if (pos_ < len_) return text_[pos_];
    return '\0';
}

bool json_parser::eof() const noexcept {
    return pos_ >= len_;
}

unique_ptr<json_string> json_parser::parse_string() {
    pos_++;
    const size_t start_pos = pos_;

    while (pos_ < len_) {
        const char c = text_[pos_++];
        if (c == '\\') {
            if (pos_ >= len_) {
                NEFORCE_THROW_EXCEPTION(json_exception("Unterminated escape sequence in string"));
            }
            pos_++;
        } else if (c == '"') {
            const size_t end_pos = pos_ - 1;
            return make_unique<json_string>(text_.view(start_pos, end_pos - start_pos));
        }
    }
    NEFORCE_THROW_EXCEPTION(json_exception("Unterminated string"));
    unreachable();
}

unique_ptr<json_number> json_parser::parse_number() {
    const size_t start = pos_;
    if (current() == '-') {
        pos_++;
    }
    if (current() == '0') {
        pos_++;
        if (pos_ < len_ && text_[pos_] == '.') {
            pos_++;
            if (pos_ >= len_ || !is_digit(text_[pos_])) {
                NEFORCE_THROW_EXCEPTION(json_exception("Invalid decimal part"));
            }
            while (pos_ < len_ && is_digit(text_[pos_])) {
                pos_++;
            }
        }
    } else if (is_digit(current())) {
        while (pos_ < len_ && is_digit(text_[pos_])) {
            pos_++;
        }
        if (pos_ < len_ && text_[pos_] == '.') {
            pos_++;
            if (pos_ >= len_ || !is_digit(text_[pos_])) {
                NEFORCE_THROW_EXCEPTION(json_exception("Invalid decimal part"));
            }
            while (pos_ < len_ && is_digit(text_[pos_])) {
                pos_++;
            }
        }
    } else {
        NEFORCE_THROW_EXCEPTION(json_exception("Invalid number format"));
    }

    if (pos_ < len_ && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
        pos_++;
        if (pos_ < len_ && (text_[pos_] == '+' || text_[pos_] == '-')) {
            pos_++;
        }
        if (pos_ >= len_ || !is_digit(text_[pos_])) {
            NEFORCE_THROW_EXCEPTION(json_exception("Invalid exponent part"));
        }
        while (pos_ < len_ && is_digit(text_[pos_])) {
            pos_++;
        }
    }

    try {
        double value = float64::parse(text_.view(start, pos_ - start)).value();
        return make_unique<json_number>(value);
    } catch (...) {
        NEFORCE_THROW_EXCEPTION(json_exception("Invalid number value"));
    }
    unreachable();
}

unique_ptr<json_value> json_parser::parse_keyword() {
    const size_t start = pos_;
    while (pos_ < len_ && is_alpha(text_[pos_])) {
        pos_++;
    }

    const string_view keyword = text_.view(start, pos_ - start);
    if (keyword == "true") {
        return make_unique<json_bool>(true);
    } else if (keyword == "false") {
        return make_unique<json_bool>(false);
    } else if (keyword == "null") {
    } else {
        NEFORCE_THROW_EXCEPTION(json_exception("Invalid keyword"));
    }
    return make_unique<json_null>();
}

unique_ptr<json_array> json_parser::parse_array() {
    pos_++;
    auto array = make_unique<json_array>();

    skip_space();
    if (current() == ']') {
        pos_++;
        return array;
    }

    while (true) {
        skip_space();
        auto element = parse_value();
        array->add_element(move(element));
        skip_space();

        if (current() == ']') {
            pos_++;
            break;
        } else if (current() == ',') {
            pos_++;
            skip_space();
        } else {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected comma or closing bracket in array"));
        }
    }
    return array;
}

unique_ptr<json_object> json_parser::parse_object() {
    pos_++;
    auto object = make_unique<json_object>();

    skip_space();
    if (current() == '}') {
        pos_++;
        return object;
    }

    while (true) {
        skip_space();
        if (current() != '"') {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected string key in object"));
        }
        const auto key_obj = parse_string();
        string key = key_obj->get_value();

        skip_space();
        if (current() != ':') {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected colon after key in object"));
        }
        pos_++;
        skip_space();

        auto value = parse_value();
        object->add_member(key, move(value));

        skip_space();
        if (current() == '}') {
            pos_++;
            break;
        } else if (current() == ',') {
            pos_++;
            skip_space();
        } else {
            NEFORCE_THROW_EXCEPTION(json_exception("Expected comma or closing brace in object"));
        }
    }

    return object;
}

unique_ptr<json_value> json_parser::parse_value() {
    skip_space();
    if (eof()) {
        NEFORCE_THROW_EXCEPTION(json_exception("Unexpected end of input"));
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
            NEFORCE_THROW_EXCEPTION(json_exception("Unexpected character"));
    }
    unreachable();
}


unique_ptr<json_value> json_parser::parse() {
    auto value = parse_value();
    skip_space();
    if (!eof()) {
        NEFORCE_THROW_EXCEPTION(json_exception("Unexpected characters after JSON value"));
    }
    return value;
}

optional<unique_ptr<json_value>> json_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

NEFORCE_END_NAMESPACE__
