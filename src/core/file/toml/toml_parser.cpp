#include "NeForce/core/file/toml/toml_parser.hpp"
#include "NeForce/core/memory/bit.hpp"
#include "NeForce/core/string/to_numerics.hpp"
#include "NeForce/core/string/to_string.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    NEFORCE_NODISCARD bool digits_at(const char* cursor, const char* end, const size_t count) noexcept {
        if (static_cast<size_t>(end - cursor) < count) {
            return false;
        }
        for (size_t i = 0; i < count; ++i) {
            if (!is_digit(cursor[i])) {
                return false;
            }
        }
        return true;
    }

    NEFORCE_NODISCARD bool date_at(const char* cursor, const char* end) noexcept {
        return static_cast<size_t>(end - cursor) >= 10 && digits_at(cursor, end, 4) && cursor[4] == '-' &&
               digits_at(cursor + 5, end, 2) && cursor[7] == '-' && digits_at(cursor + 8, end, 2);
    }

    NEFORCE_NODISCARD bool time_at(const char* cursor, const char* end) noexcept {
        return static_cast<size_t>(end - cursor) >= 8 && digits_at(cursor, end, 2) && cursor[2] == ':' &&
               digits_at(cursor + 3, end, 2) && cursor[5] == ':' && digits_at(cursor + 6, end, 2);
    }

    NEFORCE_NODISCARD bool offset_at(const char* cursor, const char* end) noexcept {
        return static_cast<size_t>(end - cursor) >= 5 && digits_at(cursor, end, 2) && cursor[2] == ':' &&
               digits_at(cursor + 3, end, 2);
    }

    NEFORCE_NODISCARD bool decimal_integer(const string_view text, int64_t& value) noexcept {
        size_t index = 0;
        bool negative = false;
        if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
            negative = text[index] == '-';
            ++index;
        }
        if (index == text.size()) {
            return false;
        }
        const uint64_t limit = negative ? static_cast<uint64_t>(9223372036854775807LL) + 1U
                                        : static_cast<uint64_t>(9223372036854775807LL);
        uint64_t magnitude = 0;
        for (; index < text.size(); ++index) {
            const char c = text[index];
            if (c < '0' || c > '9') {
                return false;
            }
            const auto digit = static_cast<uint64_t>(c - '0');
            if (magnitude > (limit - digit) / 10U) {
                return false;
            }
            magnitude = magnitude * 10U + digit;
        }
        if (negative) {
            value = magnitude == static_cast<uint64_t>(9223372036854775807LL) + 1U
                            ? static_cast<int64_t>(-9223372036854775807LL - 1)
                            : -static_cast<int64_t>(magnitude);
        } else {
            value = static_cast<int64_t>(magnitude);
        }
        return true;
    }

    NEFORCE_NODISCARD bool based_integer(const string_view text, const int base, int64_t& value) noexcept {
        uint64_t magnitude = 0;
        for (const char c: text) {
            uint64_t digit = 0;
            if (c >= '0' && c <= '9') {
                digit = static_cast<uint64_t>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                digit = static_cast<uint64_t>(c - 'a') + 10U;
            } else if (c >= 'A' && c <= 'F') {
                digit = static_cast<uint64_t>(c - 'A') + 10U;
            } else {
                return false;
            }
            if (digit >= static_cast<uint64_t>(base)) {
                return false;
            }
            if (magnitude > (static_cast<uint64_t>(9223372036854775807LL) - digit) / static_cast<uint64_t>(base)) {
                return false;
            }
            magnitude = magnitude * static_cast<uint64_t>(base) + digit;
        }
        value = static_cast<int64_t>(magnitude);
        return true;
    }

    NEFORCE_NODISCARD string text_of(const string_view text) { return string(text.data(), text.size()); }

    constexpr uint64_t k_byte_ones = 0x0101010101010101ULL;
    constexpr uint64_t k_byte_highs = 0x8080808080808080ULL;
    constexpr uint64_t k_newline_word = 0x0A0A0A0A0A0A0A0AULL;
    constexpr uint64_t k_backslash_word = 0x5C5C5C5C5C5C5C5CULL;

    NEFORCE_NODISCARD constexpr uint64_t byte_match(const uint64_t word, const uint64_t target) noexcept {
        return (word ^ target) - k_byte_ones & ~(word ^ target) & k_byte_highs;
    }

    NEFORCE_NODISCARD const char* scan_single_line(const char* cursor, const char* end, const char separator,
                                                   const bool backslash) noexcept {
        if (cursor == nullptr || end == nullptr) {
            return cursor;
        }
        const uint64_t separator_word = static_cast<uint64_t>(static_cast<unsigned char>(separator)) * k_byte_ones;
        while (end - cursor >= 8) {
            uint64_t word = 0;
            memory_copy(&word, cursor, sizeof(word));
            uint64_t mask = byte_match(word, separator_word) | byte_match(word, k_newline_word);
            if (backslash) {
                mask |= byte_match(word, k_backslash_word);
            }
            if (mask != 0) {
                return cursor + (countr_zero(static_cast<uintptr_t>(mask)) >> 3);
            }
            cursor += 8;
        }
        while (cursor < end) {
            const char c = *cursor;
            if (c == separator || c == '\n' || (backslash && c == '\\')) {
                return cursor;
            }
            ++cursor;
        }
        return cursor;
    }
} // namespace


void toml_parser::skip_whitespace() noexcept {
    while (!eof() && is_space(current())) {
        advance();
    }
}

void toml_parser::skip_comment() noexcept {
    if (current() != '#') {
        return;
    }
    while (!eof() && current() != '\n') {
        advance();
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

void toml_parser::skip_whitespace_no_newline() noexcept {
    while (!eof() && (current() == ' ' || current() == '\t')) {
        advance();
    }
}

bool toml_parser::expect(const char ch) {
    if (current() != ch) {
        throw_parse_error("expected '"_s + ch + "' but got '" + string(1, current()) + "'");
    }
    advance();
    return true;
}

void toml_parser::throw_parse_error(string message) const {
    size_t line = 1;
    size_t column = 1;
    for (const char* position = begin_; position < cursor_; ++position) {
        if (*position == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    const string error_msg =
            "Line " + to_string(line) + ", Column " + to_string(column) + ": " + _NEFORCE move(message);
    NEFORCE_THROW_EXCEPTION(toml_exception(error_msg.data()));
}

bool toml_parser::same(const char* cursor, const char* end, const char* text, const size_t size) noexcept {
    if (static_cast<size_t>(end - cursor) < size) {
        return false;
    }
    for (size_t i = 0; i < size; ++i) {
        if (cursor[i] != text[i]) {
            return false;
        }
    }
    return true;
}

codepoint toml_parser::parse_unicode_escape(const size_t digits) {
    uint32_t value = 0;
    for (size_t i = 0; i < digits; ++i) {
        if (eof() || !is_xdigit(current())) {
            throw_parse_error("Invalid unicode escape sequence");
        }
        const char c = current();
        uint32_t digit = 0;
        if (c >= '0' && c <= '9') {
            digit = static_cast<uint32_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = static_cast<uint32_t>(c - 'a') + 10;
        } else {
            digit = static_cast<uint32_t>(c - 'A') + 10;
        }
        value = (value << 4) | digit;
        advance();
    }
    if (!codepoint::is_valid_codepoint(value)) {
        throw_parse_error("Invalid unicode codepoint");
    }
    return codepoint{value};
}

toml_value toml_parser::make_text(toml_document& doc, const string_view text, const toml_string_type type) {
    if (owning_) {
        return doc.make_string_value(text, type);
    }
    return toml_value::make_string(text, type);
}

toml_value toml_parser::parse_string(toml_document& doc) {
    if (current() == '"') {
        if (peek() == '"' && peek(2) == '"') {
            return parse_multiline_basic_string(doc);
        }
        return parse_basic_string(doc);
    }
    if (current() == '\'') {
        if (peek() == '\'' && peek(2) == '\'') {
            return parse_multiline_literal_string(doc);
        }
        return parse_literal_string(doc);
    }
    throw_parse_error("Expected string");
    return toml_value();
}

toml_value toml_parser::parse_basic_string(toml_document& doc) {
    expect('"');
    const char* const start = cursor_;
    while (true) {
        cursor_ = scan_single_line(cursor_, end_, '"', true);
        if (cursor_ == end_) {
            throw_parse_error("Unterminated basic string");
        }
        const char c = *cursor_;
        if (c == '"') {
            const string_view raw(start, static_cast<size_t>(cursor_ - start));
            advance();
            return make_text(doc, raw, toml_basic_string);
        }
        if (c == '\n') {
            throw_parse_error("Unescaped newline in basic string");
        }
        cursor_ = start;
        return parse_basic_escaped(doc);
    }
}

toml_value toml_parser::parse_basic_escaped(toml_document& doc) {
    decode_scratch_.clear();
    while (cursor_ < end_) {
        const char c = *cursor_;
        if (c == '"') {
            advance();
            return doc.make_string_value(string_view(decode_scratch_.data(), decode_scratch_.size()),
                                         toml_basic_string);
        }
        if (c == '\n') {
            throw_parse_error("Unescaped newline in basic string");
        }
        if (c != '\\') {
            decode_scratch_ += c;
            advance();
            continue;
        }
        advance();
        if (eof()) {
            throw_parse_error("Unexpected end in string escape");
        }
        switch (current()) {
            case 'b':
                decode_scratch_ += '\b';
                break;
            case 't':
                decode_scratch_ += '\t';
                break;
            case 'n':
                decode_scratch_ += '\n';
                break;
            case 'f':
                decode_scratch_ += '\f';
                break;
            case 'r':
                decode_scratch_ += '\r';
                break;
            case '"':
                decode_scratch_ += '"';
                break;
            case '\\':
                decode_scratch_ += '\\';
                break;
            case 'u': {
                advance();
                if (eof()) {
                    throw_parse_error("Unexpected end after \\u");
                }
                const codepoint cp = parse_unicode_escape(4);
                cp.append_to(decode_scratch_);
                continue;
            }
            case 'U': {
                advance();
                if (eof()) {
                    throw_parse_error("Unexpected end after \\U");
                }
                const codepoint cp = parse_unicode_escape(8);
                cp.append_to(decode_scratch_);
                continue;
            }
            default:
                throw_parse_error(R"(Invalid escape sequence: \)"_s + current());
        }
        advance();
    }
    throw_parse_error("Unterminated basic string");
    return toml_value();
}

toml_value toml_parser::parse_literal_string(toml_document& doc) {
    expect('\'');
    const char* const start = cursor_;
    while (true) {
        cursor_ = scan_single_line(cursor_, end_, '\'', false);
        if (cursor_ == end_) {
            throw_parse_error("Unterminated literal string");
        }
        if (*cursor_ == '\'') {
            const string_view raw(start, static_cast<size_t>(cursor_ - start));
            advance();
            return make_text(doc, raw, toml_literal_string);
        }
        throw_parse_error("Unescaped newline in literal string");
    }
}

toml_value toml_parser::parse_multiline_basic_string(toml_document& doc) {
    expect('"');
    expect('"');
    expect('"');
    if (current() == '\n') {
        advance();
    } else if (current() == '\r' && peek() == '\n') {
        advance();
        advance();
    }
    return parse_multiline_basic_escaped(doc);
}

toml_value toml_parser::parse_multiline_basic_escaped(toml_document& doc) {
    decode_scratch_.clear();
    while (cursor_ < end_) {
        const char c = *cursor_;
        if (c == '"') {
            if (peek() == '"' && peek(2) == '"') {
                advance();
                advance();
                advance();
                return doc.make_string_value(string_view(decode_scratch_.data(), decode_scratch_.size()),
                                             toml_multi_basic_string);
            }
            decode_scratch_ += '"';
            advance();
            continue;
        }
        if (c != '\\') {
            decode_scratch_ += c;
            advance();
            continue;
        }

        advance();
        if (eof()) {
            throw_parse_error("Unexpected end in string escape");
        }
        if (current() == '\n') {
            advance();
            skip_whitespace();
            continue;
        }
        if (current() == '\r' && peek() == '\n') {
            advance();
            advance();
            skip_whitespace();
            continue;
        }
        if (current() == ' ' || current() == '\t') {
            skip_whitespace();
            if (current() == '\n') {
                advance();
                skip_whitespace();
                continue;
            }
            if (current() == '\r' && peek() == '\n') {
                advance();
                advance();
                skip_whitespace();
                continue;
            }
        }

        switch (current()) {
            case 'b':
                decode_scratch_ += '\b';
                break;
            case 't':
                decode_scratch_ += '\t';
                break;
            case 'n':
                decode_scratch_ += '\n';
                break;
            case 'f':
                decode_scratch_ += '\f';
                break;
            case 'r':
                decode_scratch_ += '\r';
                break;
            case '"':
                decode_scratch_ += '"';
                break;
            case '\\':
                decode_scratch_ += '\\';
                break;
            case 'u': {
                advance();
                const codepoint cp = parse_unicode_escape(4);
                cp.append_to(decode_scratch_);
                continue;
            }
            case 'U': {
                advance();
                const codepoint cp = parse_unicode_escape(8);
                cp.append_to(decode_scratch_);
                continue;
            }
            default:
                throw_parse_error(R"(Invalid escape sequence: \)"_s + current());
        }
        advance();
    }
    throw_parse_error("Unterminated multi-line basic string");
    return toml_value();
}

toml_value toml_parser::parse_multiline_literal_string(toml_document& doc) {
    expect('\'');
    expect('\'');
    expect('\'');
    if (current() == '\n') {
        advance();
    } else if (current() == '\r' && peek() == '\n') {
        advance();
        advance();
    }
    const char* const start = cursor_;
    while (cursor_ + 2 < end_) {
        if (cursor_[0] == '\'' && cursor_[1] == '\'' && cursor_[2] == '\'') {
            const string_view raw(start, static_cast<size_t>(cursor_ - start));
            advance();
            advance();
            advance();
            return make_text(doc, raw, toml_multi_literal_string);
        }
        ++cursor_;
    }
    throw_parse_error("Unterminated multi-line literal string");
    return toml_value();
}

string_view toml_parser::clean_number(const string_view raw) {
    if (!raw.contains('_')) {
        return raw;
    }
    number_scratch_.clear();
    number_scratch_.reserve(raw.size());
    for (const char c: raw) {
        if (c != '_') {
            number_scratch_ += c;
        }
    }
    return string_view(number_scratch_.data(), number_scratch_.size());
}

toml_value toml_parser::parse_number() {
    const char* const start = cursor_;
    bool is_float = false;

    const bool has_sign = current() == '+' || current() == '-';
    if (has_sign) {
        advance();
    }

    if (current() == 'i' || current() == 'n') {
        while (!eof() && is_alpha(current())) {
            advance();
        }
        const string_view raw(start, static_cast<size_t>(cursor_ - start));
        try {
            return toml_value::make_float(to_float64(raw));
        } catch (...) {
            throw_parse_error("Invalid special float value: " + text_of(raw));
        }
    }

    if (current() == '0' && !eof()) {
        const char next = peek();
        if (next == 'x' || next == 'X') {
            if (has_sign) {
                throw_parse_error("Sign not allowed for hexadecimal integer");
            }
            advance();
            advance();
            return parse_integer(16);
        }
        if (next == 'o' || next == 'O') {
            if (has_sign) {
                throw_parse_error("Sign not allowed for octal integer");
            }
            advance();
            advance();
            return parse_integer(8);
        }
        if (next == 'b' || next == 'B') {
            if (has_sign) {
                throw_parse_error("Sign not allowed for binary integer");
            }
            advance();
            advance();
            return parse_integer(2);
        }
    }

    bool has_digits = false;
    while (!eof() && (is_digit(current()) || current() == '_')) {
        if (is_digit(current())) {
            has_digits = true;
        }
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
        if (current() == '+' || current() == '-') {
            advance();
        }
        if (!is_digit(current())) {
            throw_parse_error("Expected digit in exponent");
        }
        while (!eof() && (is_digit(current()) || current() == '_')) {
            advance();
        }
    }

    const string_view raw(start, static_cast<size_t>(cursor_ - start));
    const string_view cleaned = clean_number(raw);

    if (!is_float) {
        string_view digits = cleaned;
        if (!digits.empty() && (digits[0] == '+' || digits[0] == '-')) {
            digits = digits.substr(1);
        }
        if (digits.size() > 1 && digits[0] == '0') {
            throw_parse_error("Leading zeros are not allowed in decimal integers");
        }
    }

    if (is_float) {
        string_view probe = cleaned;
        if (!probe.empty() && probe[0] == '+') {
            probe = probe.substr(1);
        }
        double real = 0.0;
        if (inner::parse_double_exact(probe, real)) {
            return toml_value::make_float(real);
        }
        try {
            return toml_value::make_float(to_float64(cleaned));
        } catch (...) {
            throw_parse_error("Invalid numeric value: " + text_of(cleaned));
        }
    }
    int64_t integer = 0;
    if (!decimal_integer(cleaned, integer)) {
        throw_parse_error("Invalid numeric value: " + text_of(cleaned));
    }
    return toml_value::make_integer(integer);
}

toml_value toml_parser::parse_integer(const int base) {
    const char* const start = cursor_;
    bool has_digits = false;
    while (!eof()) {
        const char ch = current();
        if (ch == '_') {
            advance();
            continue;
        }
        bool valid = false;
        if (base == 2) {
            valid = ch == '0' || ch == '1';
        } else if (base == 8) {
            valid = ch >= '0' && ch <= '7';
        } else if (base == 16) {
            valid = is_xdigit(ch);
        } else {
            valid = is_digit(ch);
        }
        if (!valid) {
            break;
        }
        has_digits = true;
        advance();
    }
    if (!has_digits) {
        throw_parse_error("Expected digit in integer");
    }
    const string_view cleaned = clean_number(string_view(start, static_cast<size_t>(cursor_ - start)));
    int64_t integer = 0;
    if (!based_integer(cleaned, base, integer)) {
        throw_parse_error("Invalid integer value: " + text_of(cleaned));
    }
    return toml_value::make_integer(integer);
}

toml_value toml_parser::parse_boolean() {
    if (same(cursor_, end_, "true", 4)) {
        advance_bulk(4);
        return toml_value::make_boolean(true);
    }
    if (same(cursor_, end_, "false", 5)) {
        advance_bulk(5);
        return toml_value::make_boolean(false);
    }
    throw_parse_error("Expected boolean value");
    return toml_value();
}

toml_value toml_parser::parse_datetime(toml_document& doc) {
    const char* const start = cursor_;
    const bool has_date = date_at(cursor_, end_);
    bool has_time = false;
    bool has_offset = false;

    if (has_date) {
        advance_bulk(10);
        if (!eof() && (current() == 'T' || current() == 't' || current() == ' ') && time_at(cursor_ + 1, end_)) {
            advance();
            advance_bulk(8);
            has_time = true;
            if (current() == '.' && is_digit(peek())) {
                advance();
                while (!eof() && is_digit(current())) {
                    advance();
                }
            }
            if (!eof() && (current() == 'Z' || current() == 'z')) {
                advance();
                has_offset = true;
            } else if (!eof() && (current() == '+' || current() == '-') && offset_at(cursor_ + 1, end_)) {
                advance();
                advance_bulk(5);
                has_offset = true;
            }
        }
    } else {
        if (!time_at(cursor_, end_)) {
            throw_parse_error("Not a valid datetime format");
        }
        advance_bulk(8);
        has_time = true;
        if (current() == '.' && is_digit(peek())) {
            advance();
            while (!eof() && is_digit(current())) {
                advance();
            }
        }
    }

    const string_view raw(start, static_cast<size_t>(cursor_ - start));
    toml_datetime_type type = toml_local_date;
    if (!has_date) {
        type = toml_local_time;
    } else if (has_time) {
        type = has_offset ? toml_offset_datetime : toml_local_datetime;
    }
    return doc.make_datetime_value(raw, type);
}

toml_value toml_parser::parse_array(toml_document& doc) {
    expect('[');
    toml_value* node = doc.new_container(false);
    skip_whitespace_and_comments();
    while (!eof() && current() != ']') {
        doc.array_append(*node, parse_value(doc));
        skip_whitespace_and_comments();
        if (current() == ',') {
            advance();
            skip_whitespace_and_comments();
            if (current() == ']') {
                throw_parse_error("Trailing comma is not allowed in array");
            }
        } else if (current() != ']') {
            throw_parse_error("Expected ',' or ']' in array");
        }
    }
    expect(']');
    return *node;
}

toml_value toml_parser::parse_inline_table(toml_document& doc) {
    expect('{');
    toml_value* node = doc.new_container(true, true);
    skip_whitespace_no_newline();
    while (!eof() && current() != '}') {
        const string_view key = parse_key(doc);
        skip_whitespace_no_newline();
        expect('=');
        skip_whitespace_no_newline();
        const toml_value value = parse_value(doc);
        if (node->as_table().has_member(key)) {
            throw_parse_error("Duplicate key in inline table: " + text_of(key));
        }
        doc.table_append(*node, key, value, owning_);
        skip_whitespace_no_newline();
        if (current() == ',') {
            advance();
            skip_whitespace_no_newline();
            if (current() == '}') {
                throw_parse_error("Trailing comma is not allowed in inline table");
            }
        } else if (current() != '}') {
            throw_parse_error("Expected ',' or '}' in inline table");
        }
    }
    expect('}');
    return *node;
}

string_view toml_parser::parse_key(toml_document& doc) {
    if (current() == '"' || current() == '\'') {
        return parse_quoted_key(doc);
    }
    return parse_bare_key();
}

string_view toml_parser::parse_bare_key() {
    const char* const start = cursor_;
    while (!eof() && (is_alpha_or_digit(current()) || current() == '_' || current() == '-')) {
        advance();
    }
    if (cursor_ == start) {
        throw_parse_error("Expected key");
    }
    return string_view(start, static_cast<size_t>(cursor_ - start));
}

string_view toml_parser::parse_quoted_key(toml_document& doc) {
    const toml_value value = parse_string(doc);
    return doc.store_string(value.as_string().get_value());
}

void toml_parser::parse_dotted_key(toml_document& doc, vector<string_view>& path) {
    path.clear();
    path.push_back(parse_key(doc));
    skip_whitespace();
    while (current() == '.') {
        advance();
        skip_whitespace();
        path.push_back(parse_key(doc));
        skip_whitespace();
    }
}

toml_value toml_parser::parse_value(toml_document& doc) {
    if (eof()) {
        throw_parse_error("Unexpected end of input when expecting a value");
    }
    const char ch = current();
    if (ch == '"' || ch == '\'') {
        return parse_string(doc);
    }
    if (ch == '[') {
        return parse_array(doc);
    }
    if (ch == '{') {
        return parse_inline_table(doc);
    }
    if (ch == 't' || ch == 'f') {
        return parse_boolean();
    }
    if (ch == '+' || ch == '-' || ch == 'i' || ch == 'n') {
        return parse_number();
    }
    if (is_digit(ch)) {
        if (date_at(cursor_, end_) || time_at(cursor_, end_)) {
            return parse_datetime(doc);
        }
        return parse_number();
    }
    throw_parse_error("Unexpected character '"_s + ch + "' when parsing value");
    return toml_value();
}

void toml_parser::parse_key_value(toml_document& doc) {
    parse_dotted_key(doc, key_path_);
    expect('=');
    skip_whitespace_and_comments();
    const toml_value value = parse_value(doc);
    skip_whitespace();
    if (current() == '#') {
        skip_comment();
    }

    toml_value* table = current_table_;
    for (size_t i = 0; i + 1 < key_path_.size(); ++i) {
        const string_view key = key_path_[i];
        const toml_value* member = table->as_table().get_member(key);
        toml_value* sub_table = nullptr;
        if (member != nullptr && member->is_table()) {
            sub_table = const_cast<toml_value*>(member);
        } else if (member == nullptr) {
            sub_table = doc.new_container(true);
            doc.table_append(*table, key, *sub_table, owning_);
        } else {
            throw_parse_error("Key '" + text_of(key) + "' already exists but is not a table");
        }
        table = sub_table;
    }

    const string_view last_key = key_path_.back();
    if (table->as_table().has_member(last_key)) {
        throw_parse_error("Duplicate key: " + text_of(last_key));
    }
    doc.table_append(*table, last_key, value, owning_);
}

void toml_parser::parse_table_header(toml_document& doc) {
    expect('[');
    if (current() == '[') {
        throw_parse_error("Use [[table]] for array of tables");
    }
    parse_dotted_key(doc, key_path_);
    skip_whitespace();
    expect(']');
    set_current_table(doc, key_path_);
    is_in_array_table_ = false;
}

void toml_parser::parse_array_table_header(toml_document& doc) {
    expect('[');
    expect('[');
    parse_dotted_key(doc, key_path_);
    skip_whitespace();
    expect(']');
    expect(']');
    if (key_path_.empty()) {
        throw_parse_error("Empty array table path");
    }

    toml_value* parent = get_or_create_table(doc, &doc.root(), key_path_.data(), key_path_.size() - 1);
    const string_view array_key = key_path_.back();
    const toml_value* existing = parent->as_table().get_member(array_key);
    toml_value* array = nullptr;
    if (existing != nullptr) {
        if (!existing->is_array()) {
            throw_parse_error("Key '" + text_of(array_key) + "' already exists and is not an array");
        }
        array = const_cast<toml_value*>(existing);
    } else {
        array = doc.new_container(false);
        doc.table_append(*parent, array_key, *array, owning_);
    }

    toml_value* new_table = doc.new_container(true);
    doc.array_append(*array, *new_table);

    context_stack_.push_back(current_table_);
    current_table_ = new_table;
    is_in_array_table_ = true;
}

toml_value* toml_parser::get_or_create_table(toml_document& doc, toml_value* base, const string_view* path,
                                             const size_t count) {
    toml_value* table = base;
    for (size_t i = 0; i < count; ++i) {
        const string_view key = path[i];
        const toml_value* member = table->as_table().get_member(key);
        if (member != nullptr && member->is_table()) {
            table = const_cast<toml_value*>(member);
        } else if (member == nullptr) {
            toml_value* created = doc.new_container(true);
            doc.table_append(*table, key, *created, owning_);
            table = created;
        } else {
            throw_parse_error("Key '" + text_of(key) + "' already exists but is not a table");
        }
    }
    return table;
}

void toml_parser::set_current_table(toml_document& doc, const vector<string_view>& path) {
    current_table_ = get_or_create_table(doc, &doc.root(), path.data(), path.size());
}

toml_document toml_parser::parse() {
    static constexpr char empty_text[] = "";

    begin_ = text_.data() != nullptr ? text_.data() : empty_text;
    cursor_ = begin_;
    end_ = begin_ + text_.size();

    if (static_cast<size_t>(end_ - cursor_) >= 3 && static_cast<uint8_t>(cursor_[0]) == 0xEF &&
        static_cast<uint8_t>(cursor_[1]) == 0xBB && static_cast<uint8_t>(cursor_[2]) == 0xBF) {
        cursor_ += 3;
    }

    toml_document doc;
    doc.reserve(text_.size() * 2 + 4096);
    doc.make_table();
    current_table_ = &doc.root();
    context_stack_.clear();
    is_in_array_table_ = false;

    while (!eof()) {
        skip_whitespace_and_comments();
        if (eof()) {
            break;
        }
        if (current() == '[') {
            if (is_in_array_table_ && !context_stack_.empty()) {
                current_table_ = context_stack_.back();
                context_stack_.pop_back();
                is_in_array_table_ = false;
            }
            if (peek() == '[') {
                parse_array_table_header(doc);
            } else {
                parse_table_header(doc);
            }
        } else {
            parse_key_value(doc);
        }
    }
    return doc;
}

optional<toml_document> toml_parser::try_parse() noexcept {
    try {
        return optional<toml_document>(parse());
    } catch (...) {
        return optional<toml_document>();
    }
}

NEFORCE_END_NAMESPACE__
