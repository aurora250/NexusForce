#include <MSTL/core/file/toml/toml_parser.hpp>
#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/algorithm/erase.hpp>
#include <MSTL/core/memory/hexadecimal.hpp>
MSTL_BEGIN_NAMESPACE__

void toml_parser::skip_whitespace() noexcept {
    while (pos_ < len_) {
        if (is_space(current())) {
            advance();
        } else {
            break;
        }
    }
}

void toml_parser::skip_comment() noexcept {
    if (current() == '#') {
        while (!eof() && current() != '\n') {
            advance();
        }
    }
}

void toml_parser::skip_whitespace_and_comments() noexcept {
    while (!eof()) {
        skip_whitespace();
        if (current() == '#') {
            skip_comment();
        } else if (current() == '\n' || current() == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void toml_parser::skip_newlines() noexcept {
    while (!eof() && (current() == '\n' || current() == '\r')) {
        advance();
    }
}

void toml_parser::skip_whitespace_no_newline() noexcept {
    while (pos_ < len_) {
        const char ch = toml_[pos_];
        if (ch == ' ' || ch == '\t') {
            advance();
        } else {
            break;
        }
    }
}

char toml_parser::current() const noexcept {
    if (pos_ < len_) return toml_[pos_];
    return '\0';
}

char toml_parser::peek(const size_t offset) const noexcept {
    if (pos_ + offset < len_) return toml_[pos_ + offset];
    return '\0';
}

bool toml_parser::eof() const noexcept {
    return pos_ >= len_;
}

void toml_parser::advance() noexcept {
    if (pos_ < len_) {
        if (toml_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

bool toml_parser::expect(const char ch) {
    if (current() != ch) {
        throw_parse_error("expected '"_s + ch + "' but got '" + escape(string(1, current())) + "'");
        return false;
    }
    advance();
    return true;
}

bool toml_parser::match(const char ch) noexcept {
    if (current() == ch) {
        advance();
        return true;
    }
    return false;
}

void toml_parser::throw_parse_error(string message) const {
    const string error_msg =
        "Line " + _MSTL to_string(line_) +
        ", Column " + _MSTL to_string(column_) + ": " + move(message);
    throw_exception(toml_exception(error_msg));
}

char32_t toml_parser::parse_unicode_escape(const size_t digits) {
    const size_t start_pos = pos_;
    for (size_t i = 0; i < digits; i++) {
        if (eof() || !is_xdigit(current())) {
            throw_parse_error("Invalid unicode escape sequence");
        }
        advance();
    }

    const string_view hex_str = toml_.view(start_pos, digits);
    try {
        const int64_t value = hexadecimal(hex_str).to_int64();
        if (value < 0 || value > 0x10FFFF) {
            throw_parse_error("Unicode codepoint out of range");
        }
        return static_cast<char32_t>(value);
    } catch (...) {
        throw_parse_error("Invalid unicode escape value");
    }
    return 0;
}

unique_ptr<toml_string> toml_parser::parse_string() {
    if (current() == '"') {
        if (peek(1) == '"' && peek(2) == '"') {
            return parse_multiline_basic_string();
        }
        return parse_basic_string();
    } else if (current() == '\'') {
        if (peek(1) == '\'' && peek(2) == '\'') {
            return parse_multiline_literal_string();
        }
        return parse_literal_string();
    }
    throw_parse_error("Expected string");
    return nullptr;
}

unique_ptr<toml_string> toml_parser::parse_basic_string() {
    expect('"');
    string result;

    while (!eof() && current() != '"') {
        if (current() == '\n') {
            throw_parse_error("Unescaped newline in basic string");
        }

        if (current() == '\\') {
            advance();
            if (eof()) throw_parse_error("Unexpected end in string escape");

            switch (current()) {
                case 'b': result += '\b'; break;
                case 't': result += '\t'; break;
                case 'n': result += '\n'; break;
                case 'f': result += '\f'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'u': {
                    advance();
                    if (eof()) {
                        throw_parse_error("Unexpected end after \\u");
                    }
                    const char32_t cp = parse_unicode_escape(4);
                    result += _MSTL to_string(cp);
                    break;
                }
                case 'U': {
                    advance();
                    if (eof()) {
                        throw_parse_error("Unexpected end after \\U");
                    }
                    const char32_t cp = parse_unicode_escape(8);
                    result += _MSTL to_string(cp);
                    break;
                }
                default:
                    throw_parse_error("Invalid escape sequence: \\"_s + current());
            }
            advance();
        } else {
            result += current();
            advance();
        }
    }

    expect('"');
    return make_unique<toml_string>(_MSTL move(result), toml_string::Basic);
}

unique_ptr<toml_string> toml_parser::parse_literal_string() {
    expect('\'');
    string result;

    while (!eof() && current() != '\'') {
        if (current() == '\n') {
            throw_parse_error("Unescaped newline in literal string");
        }
        result += current();
        advance();
    }

    expect('\'');
    return make_unique<toml_string>(_MSTL move(result), toml_string::Literal);
}

unique_ptr<toml_string> toml_parser::parse_multiline_basic_string() {
    expect('"'); expect('"'); expect('"');

    if (current() == '\n') advance();
    else if (current() == '\r' && peek() == '\n') {
        advance(); advance();
    }

    string result;

    while (!eof()) {
        if (current() == '"' && peek() == '"' && peek(2) == '"') {
            advance(); advance(); advance();
            break;
        }

        if (current() == '\\') {
            advance();
            if (eof()) throw_parse_error("Unexpected end in string escape");

            if (current() == '\n') {
                advance();
                skip_whitespace();
                continue;
            } else if (current() == '\r' && peek() == '\n') {
                advance(); advance();
                skip_whitespace();
                continue;
            } else if (current() == ' ' || current() == '\t') {
                skip_whitespace();
                if (current() == '\n') {
                    advance();
                    skip_whitespace();
                    continue;
                } else if (current() == '\r' && peek() == '\n') {
                    advance(); advance();
                    skip_whitespace();
                    continue;
                }
            }

            switch (current()) {
                case 'b': result += '\b'; break;
                case 't': result += '\t'; break;
                case 'n': result += '\n'; break;
                case 'f': result += '\f'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'u': {
                    advance();
                    const char32_t cp = parse_unicode_escape(4);
                    result += _MSTL to_string(cp);
                    break;
                }
                case 'U': {
                    advance();
                    const char32_t cp = parse_unicode_escape(8);
                    result += _MSTL to_string(cp);
                    break;
                }
                default:
                    throw_parse_error("Invalid escape sequence: \\"_s + current());
            }
            advance();
        } else {
            result += current();
            advance();
        }
    }

    return make_unique<toml_string>(_MSTL move(result), toml_string::MultiBasic);
}

unique_ptr<toml_string> toml_parser::parse_multiline_literal_string() {
    expect('\''); expect('\''); expect('\'');

    if (current() == '\n') {
        advance();
    } else if (current() == '\r' && peek() == '\n') {
        advance(); advance();
    }

    string result;

    while (!eof()) {
        if (current() == '\'' && peek() == '\'' && peek(2) == '\'') {
            advance(); advance(); advance();
            break;
        }
        result += current();
        advance();
    }

    return make_unique<toml_string>(_MSTL move(result), toml_string::MultiLiteral);
}

unique_ptr<toml_value> toml_parser::parse_number() {
    const size_t start_pos = pos_;
    bool is_float = false;

    if (current() == '+' || current() == '-') {
        advance();
    }

    if (current() == 'i' || current() == 'n') {
        while (!eof() && is_alpha(current())) {
            advance();
        }
        const string_view num_str = toml_.view(start_pos, pos_ - start_pos);

        try {
            double val = to_float64(num_str);
            return make_unique<toml_float>(val);
        } catch (...) {
            throw_parse_error("Invalid special float value: "_s + num_str);
        }
    }

    if (current() == '0' && !eof()) {
        const char next = peek();
        if (next == 'x' || next == 'X') {
            advance(); advance();
            return parse_integer(16);
        } else if (next == 'o' || next == 'O') {
            advance(); advance();
            return parse_integer(8);
        } else if (next == 'b' || next == 'B') {
            advance(); advance();
            return parse_integer(2);
        }
    }

    bool has_digits = false;
    while (!eof() && (is_digit(current()) || current() == '_')) {
        if (is_digit(current())) has_digits = true;
        advance();
    }

    if (!has_digits) {
        throw_parse_error("Expected digit in number");
    }

    if (current() == '.') {
        is_float = true;
        advance();
        if (!is_digit(current())) {
            throw_parse_error("Expected digit after decimal point");
        }
        while (!eof() && (is_digit(current()) || current() == '_')) {
            advance();
        }
    }

    if (current() == 'e' || current() == 'E') {
        is_float = true;
        advance();
        if (current() == '+' || current() == '-') advance();
        if (!is_digit(current())) {
            throw_parse_error("Expected digit in exponent");
        }
        while (!eof() && (is_digit(current()) || current() == '_')) {
            advance();
        }
    }

    string num_str = toml_.substr(start_pos, pos_ - start_pos);
    num_str.erase(_MSTL remove(num_str.begin(), num_str.end(), '_'), num_str.end());

    try {
        if (is_float) {
            double val = to_float64(num_str.view());
            return make_unique<toml_float>(val);
        } else {
            int64_t val = to_int64(num_str.view(), nullptr, 10);
            return make_unique<toml_integer>(val);
        }
    } catch (...) {
        throw_parse_error("Invalid numeric value: " + num_str);
    }
    return nullptr;
}

unique_ptr<toml_integer> toml_parser::parse_integer(const int base) {
    const size_t start_pos = pos_;
    bool has_digits = false;

    while (!eof()) {
        const char ch = current();
        if (ch == '_') {
            advance();
            continue;
        }

        bool valid = false;
        if (base == 2) {
            valid = (ch == '0' || ch == '1');
        } else if (base == 8) {
            valid = (ch >= '0' && ch <= '7');
        } else if (base == 16) {
            valid = is_xdigit(ch);
        } else {
            valid = is_digit(ch);
        }

        if (valid) {
            has_digits = true;
            advance();
        } else {
            break;
        }
    }

    if (!has_digits) {
        throw_parse_error("Expected digit in integer");
    }

    string num_str = toml_.substr(start_pos, pos_ - start_pos);
    num_str.erase(_MSTL remove(num_str.begin(), num_str.end(), '_'), num_str.end());

    try {
        int64_t val;
        if (base == 2 || base == 8 || base == 16) {
            val = to_int64(num_str.view(), nullptr, base);
        } else {
            val = to_int64(num_str.view(), nullptr, 10);
        }
        return make_unique<toml_integer>(val);
    } catch (...) {
        throw_parse_error("Invalid integer value: " + num_str);
    }
    return nullptr;
}

unique_ptr<toml_boolean> toml_parser::parse_boolean() {
    if (current() == 't' && peek() == 'r' && peek(2) == 'u' && peek(3) == 'e') {
        advance(); advance(); advance(); advance();
        return make_unique<toml_boolean>(true);
    }
    if (current() == 'f' && peek() == 'a' && peek(2) == 'l' &&
        peek(3) == 's' && peek(4) == 'e') {
        advance(); advance(); advance(); advance(); advance();
        return make_unique<toml_boolean>(false);
    }
    throw_parse_error("Expected boolean value");
    return nullptr;
}

unique_ptr<toml_datetime> toml_parser::parse_datetime() {
    const size_t start_pos = pos_;

    while (!eof() && (is_digit(current()) || current() == '-' || current() == ':' ||
        current() == 'T' || current() == 'Z' || current() == '+' ||
        current() == '.' || current() == ' ')) {
        advance();
    }

    const string_view dt_str = toml_.view(start_pos, pos_ - start_pos);

    const bool has_date_sep = dt_str.find('-') != string::npos;
    const bool has_time_sep = dt_str.find(':') != string::npos;
    const bool has_datetime_sep =
        dt_str.find('T') != string::npos ||
        dt_str.find(' ') != string::npos;

    if (!has_date_sep && !has_time_sep) {
        throw_parse_error("Not a valid datetime format");
    }

    toml_datetime::datetime_type dt_type;
    if (has_datetime_sep) {
        if (dt_str.find('Z') != string::npos || dt_str.find('+') != string::npos ||
            dt_str.rfind('-') > 10) {
            dt_type = toml_datetime::OffsetDateTime;
            } else {
                dt_type = toml_datetime::LocalDateTime;
            }
    } else if (has_time_sep) {
        dt_type = toml_datetime::LocalTime;
    } else {
        dt_type = toml_datetime::LocalDate;
    }

    return make_unique<toml_datetime>(dt_str, dt_type);
}

unique_ptr<toml_array> toml_parser::parse_array() {
    expect('[');
    auto arr = make_unique<toml_array>();
    skip_whitespace_and_comments();
    optional<toml_value::types> element_type;

    while (!eof() && current() != ']') {
        auto element = parse_value();

        if (!element_type.has_value()) {
            element_type = element->type();
        } else {
            if (element->type() != element_type.value()) {
                throw_parse_error("Mixed types in array are not allowed");
            }
        }

        arr->add_element(_MSTL move(element));
        skip_whitespace_and_comments();

        if (current() == ',') {
            advance();
            skip_whitespace_and_comments();
            if (current() == ']') break;
        } else if (current() != ']') {
            throw_parse_error("Expected ',' or ']' in array");
        }
    }

    expect(']');
    return arr;
}

unique_ptr<toml_table> toml_parser::parse_inline_table() {
    expect('{');
    auto table = make_unique<toml_table>(true);

    skip_whitespace_no_newline();

    while (!eof() && current() != '}') {
        string key = parse_key();
        skip_whitespace_no_newline();
        expect('=');
        skip_whitespace_no_newline();

        auto value = parse_value();
        if (table->has_member(key)) {
            throw_parse_error("Duplicate key in inline table: " + key);
        }
        table->add_member(key, _MSTL move(value));

        skip_whitespace_no_newline();

        if (current() == ',') {
            advance();
            skip_whitespace_no_newline();
        } else if (current() != '}') {
            throw_parse_error("Expected ',' or '}' in inline table");
        }
    }

    expect('}');
    return table;
}

string toml_parser::parse_key() {
    if (current() == '"' || current() == '\'') {
        return parse_quoted_key();
    }
    return parse_bare_key();
}

string toml_parser::parse_bare_key() {
    const size_t start_pos = pos_;
    while (!eof() && (is_alpha_or_digit(current()) || current() == '_' || current() == '-')) {
        advance();
    }
    if (pos_ == start_pos) {
        throw_parse_error("Expected key");
    }
    return toml_.substr(start_pos, pos_ - start_pos);
}

string toml_parser::parse_quoted_key() {
    const auto str_value = parse_string();
    return str_value->get_value();
}

vector<string> toml_parser::parse_dotted_key() {
    vector<string> keys;
    keys.push_back(parse_key());

    while (current() == '.') {
        advance();
        skip_whitespace();
        keys.push_back(parse_key());
        skip_whitespace();
    }

    return keys;
}

unique_ptr<toml_value> toml_parser::parse_value() {
    skip_whitespace_and_comments();

    if (eof()) {
        throw_parse_error("Unexpected end of input when expecting a value");
    }

    const char ch = current();

    if (ch == '"' || ch == '\'') {
        return parse_string();
    } else if (ch == '[') {
        return parse_array();
    } else if (ch == '{') {
        return parse_inline_table();
    } else if (ch == 't' || ch == 'f') {
        return parse_boolean();
    } else if (ch == '+' || ch == '-' || ch == 'i' || ch == 'n') {
        return parse_number();
    } else if (is_digit(ch)) {
        const size_t saved_pos = pos_;
        const size_t saved_line = line_;
        const size_t saved_column = column_;

        try {
            return parse_datetime();
        } catch (...) {
            pos_ = saved_pos;
            line_ = saved_line;
            column_ = saved_column;
            try {
                return parse_number();
            } catch (...) {
                throw_parse_error("Failed to parse numeric or datetime value");
            }
        }
    }

    throw_parse_error("Unexpected character '"_s + ch + "' when parsing value");
    return nullptr;
}

void toml_parser::parse_key_value() {
    skip_whitespace_and_comments();

    vector<string> key_path = parse_dotted_key();

    skip_whitespace();

    expect('=');
    skip_whitespace();

    unique_ptr<toml_value> val = parse_value();

    skip_whitespace();
    if (current() == '#') {
        skip_comment();
    }

    toml_table* table = ctb_;

    for (size_t i = 0; i + 1 < key_path.size(); i++) {
        const auto& k = key_path[i];
        const toml_value* member = table->get_member(k);
        toml_table* sub_table = nullptr;

        if (member && member->is_table()) {
            sub_table = const_cast<toml_table*>(member->as_table());
        } else if (!member) {
            auto new_table = make_unique<toml_table>();
            sub_table = new_table.get();
            table->add_member(k, _MSTL move(new_table));
        } else {
            throw_parse_error("Key '" + k + "' already exists but is not a table");
        }

        table = sub_table;
    }

    const string& last_key = key_path.back();

    if (table->has_member(last_key)) {
        throw_parse_error("Duplicate key: " + last_key);
    }
    table->add_member(last_key, _MSTL move(val));
}

void toml_parser::parse_table_header() {
    expect('[');

    if (current() == '[') {
        throw_parse_error("Use [[table]] for array of tables");
    }

    const vector<string> path = parse_dotted_key();
    skip_whitespace();
    expect(']');

    set_current_table(path);
    is_in_array_table_ = false;
}

void toml_parser::parse_array_table_header() {
    expect('[');
    expect('[');
    vector<string> path = parse_dotted_key();
    skip_whitespace();
    expect(']');
    expect(']');

    if (path.empty()) {
        throw_parse_error("Empty array table path");
    }

    const vector<string> parent_path(path.begin(), path.end() - 1);
    toml_table* parent = parent_path.empty() ? root_.get() : get_or_create_table(parent_path);

    const string& array_key = path.back();
    const toml_value* existing = parent->get_member(array_key);

    toml_array* arr = nullptr;
    if (existing) {
        if (!existing->is_array()) {
            throw_parse_error("Key '" + array_key + "' already exists and is not an array");
        }
        arr = const_cast<toml_array*>(existing->as_array());
    } else {
        auto new_array = make_unique<toml_array>();
        arr = new_array.get();
        parent->add_member(array_key, _MSTL move(new_array));
    }

    auto new_table = make_unique<toml_table>();
    toml_table* new_table_ptr = new_table.get();

    arr->add_element(_MSTL move(new_table));
    context_stack_.push_back({ctb_, ctp_});
    ctb_ = new_table_ptr;
    ctp_ = path;
    is_in_array_table_ = true;
}

toml_table* toml_parser::get_or_create_table(const vector<string>& path) const {
    toml_table* tbl = root_.get();
    for (const string& key : path) {
        const auto member = tbl->get_member(key);
        if (member && member->is_table()) {
            tbl = const_cast<toml_table*>(member->as_table());
        } else if (!member) {
            auto new_table = make_unique<toml_table>();
            toml_table* new_tbl_ptr = new_table.get();
            tbl->add_member(key, _MSTL move(new_table));
            tbl = new_tbl_ptr;
        } else {
            throw_parse_error("Key '" + key + "' already exists but is not a table");
        }
    }
    return tbl;
}

toml_table* toml_parser::navigate_to_table(const vector<string>& path) const {
    toml_table* tbl = root_.get();
    for (const string& key : path) {
        const toml_value* member = tbl->get_member(key);
        if (!member || !member->is_table()) {
            return nullptr;
        }
        tbl = const_cast<toml_table*>(member->as_table());
    }
    return tbl;
}

void toml_parser::set_current_table(const vector<string>& path) {
    ctp_ = path;
    ctb_ = get_or_create_table(path);
}

unique_ptr<toml_table> toml_parser::parse() {
    while (!eof()) {
        skip_whitespace_and_comments();
        if (eof()) break;

        if (current() == '[') {
            if (is_in_array_table_ && !context_stack_.empty()) {
                const auto& prev_context = context_stack_.back();
                ctb_ = prev_context.table;
                ctp_ = prev_context.path;
                context_stack_.pop_back();
                is_in_array_table_ = false;
            }

            if (peek() == '[') {
                parse_array_table_header();
            } else {
                parse_table_header();
            }
        } else {
            if (ctp_.empty()) {
                ctb_ = root_.get();
            }

            parse_key_value();
        }
    }
    return _MSTL move(root_);
}

optional<unique_ptr<toml_table>> toml_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

MSTL_END_NAMESPACE__
