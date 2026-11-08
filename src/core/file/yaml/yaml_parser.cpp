#include <NeForce/core/file/yaml/yaml_parser.hpp>
#include <NeForce/core/memory/bit.hpp>
#include <NeForce/core/string/to_numerics.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    constexpr bool is_space(const char ch) noexcept { return ch == ' ' || ch == '\t'; }
    constexpr bool is_break(const char ch) noexcept { return ch == '\n' || ch == '\r'; }

    constexpr bool is_flow_char(const char ch) noexcept {
        return ch == ',' || ch == '[' || ch == ']' || ch == '{' || ch == '}';
    }

    constexpr bool is_indicator_char(const char ch) noexcept {
        return ch == '-' || ch == '?' || ch == ':' || ch == ',' || ch == '[' || ch == ']' || ch == '{' || ch == '}' ||
               ch == '#' || ch == '&' || ch == '*' || ch == '!' || ch == '|' || ch == '>' || ch == '\'' || ch == '"' ||
               ch == '%' || ch == '@' || ch == '`';
    }

    NEFORCE_NODISCARD bool equal_ignore_case(const string_view text, const char* lower) noexcept {
        size_t i = 0;
        for (; i < text.size(); ++i) {
            if (lower[i] == '\0') {
                return false;
            }
            char ch = text[i];
            if (ch >= 'A' && ch <= 'Z') {
                ch = static_cast<char>(ch - 'A' + 'a');
            }
            if (ch != lower[i]) {
                return false;
            }
        }
        return lower[i] == '\0';
    }

    constexpr uint64_t swar_ones = 0x0101010101010101ULL;
    constexpr uint64_t swar_high = 0x8080808080808080ULL;

    NEFORCE_NODISCARD constexpr uint64_t zero_byte_mask(const uint64_t word) noexcept {
        return (word - swar_ones) & ~word & swar_high;
    }

    NEFORCE_NODISCARD const char* find_plain_special(const char* position, const char* end, const bool flow) noexcept {
        constexpr uint64_t newline = 0x0A0A0A0A0A0A0A0AULL;
        constexpr uint64_t carriage = 0x0D0D0D0D0D0D0D0DULL;
        constexpr uint64_t hash = 0x2323232323232323ULL;
        constexpr uint64_t colon = 0x3A3A3A3A3A3A3A3AULL;
        constexpr uint64_t comma = 0x2C2C2C2C2C2C2C2CULL;
        constexpr uint64_t lbracket = 0x5B5B5B5B5B5B5B5BULL;
        constexpr uint64_t rbracket = 0x5D5D5D5D5D5D5D5DULL;
        constexpr uint64_t lbrace = 0x7B7B7B7B7B7B7B7BULL;
        constexpr uint64_t rbrace = 0x7D7D7D7D7D7D7D7DULL;
        while (position + 8 <= end) {
            uint64_t word = 0;
            memory_copy(&word, position, sizeof(word));
            uint64_t mask = zero_byte_mask(word ^ newline) | zero_byte_mask(word ^ carriage) |
                            zero_byte_mask(word ^ hash) | zero_byte_mask(word ^ colon);
            if (flow) {
                mask |= zero_byte_mask(word ^ comma) | zero_byte_mask(word ^ lbracket) |
                        zero_byte_mask(word ^ rbracket) | zero_byte_mask(word ^ lbrace) | zero_byte_mask(word ^ rbrace);
            }
            if (mask != 0) {
                return position + (countr_zero(static_cast<uintptr_t>(mask)) >> 3);
            }
            position += 8;
        }
        while (position < end) {
            const char ch = *position;
            if (is_break(ch) || ch == '#' || ch == ':') {
                break;
            }
            if (flow && (ch == ',' || ch == '[' || ch == ']' || ch == '{' || ch == '}')) {
                break;
            }
            ++position;
        }
        return position;
    }

    NEFORCE_NODISCARD bool is_terminator(const char ch) noexcept {
        return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\0' || ch == ':' || ch == ',' ||
               ch == ']' || ch == '}' || ch == '#';
    }
} // namespace


yaml_parser::yaml_parser(string yaml_str) :
owned_(move(yaml_str)),
text_(owned_.data(), owned_.size()),
begin_(text_.data() != nullptr ? text_.data() : ""),
cursor_(begin_),
end_(begin_ + text_.size()),
owning_(true) {}

yaml_parser::yaml_parser(const string_view yaml_view) noexcept :
text_(yaml_view),
begin_(yaml_view.data() != nullptr ? yaml_view.data() : ""),
cursor_(begin_),
end_(begin_ + yaml_view.size()) {}

yaml_parser::yaml_parser(const char* yaml_str) noexcept :
text_(yaml_str == nullptr ? string_view() : string_view(yaml_str)),
begin_(yaml_str != nullptr ? yaml_str : ""),
cursor_(begin_),
end_(begin_ + text_.size()) {}

bool yaml_parser::expect(const char ch) {
    if (!match(ch)) {
        throw_parse_error(string("Expected '") + ch + "'");
    }
    return true;
}

NEFORCE_ALWAYS_INLINE_INLINE void yaml_parser::skip_whitespace_inline() noexcept {
    while (cursor_ < end_ && is_space(*cursor_)) {
        ++cursor_;
    }
}

NEFORCE_ALWAYS_INLINE_INLINE void yaml_parser::skip_comment() noexcept {
    if (cursor_ < end_ && *cursor_ == '#') {
        while (cursor_ < end_ && !is_break(*cursor_)) {
            ++cursor_;
        }
    }
}

NEFORCE_ALWAYS_INLINE_INLINE void yaml_parser::skip_to_next_line() noexcept {
    while (cursor_ < end_ && !is_break(*cursor_)) {
        ++cursor_;
    }
    if (cursor_ < end_ && *cursor_ == '\r') {
        ++cursor_;
    }
    if (cursor_ < end_ && *cursor_ == '\n') {
        ++cursor_;
    }
}

void yaml_parser::skip_blank_lines() noexcept {
    for (;;) {
        const char* position = cursor_;
        while (position < end_ && is_space(*position)) {
            ++position;
        }
        if (position < end_ && *position == '#') {
            cursor_ = position;
            skip_comment();
            continue;
        }
        if (position < end_ && is_break(*position)) {
            cursor_ = position;
            skip_to_next_line();
            continue;
        }
        break;
    }
}

void yaml_parser::skip_whitespace_and_comments() {
    skip_whitespace_inline();
    while (current() == '#') {
        skip_comment();
        skip_to_next_line();
        skip_whitespace_inline();
    }
}

NEFORCE_ALWAYS_INLINE_INLINE size_t yaml_parser::peek_indent() const noexcept {
    size_t indent = 0;
    const char* position = cursor_;
    while (position < end_ && *position == ' ') {
        ++indent;
        ++position;
    }
    return indent;
}

NEFORCE_ALWAYS_INLINE_INLINE size_t yaml_parser::skip_indent() {
    size_t indent = 0;
    while (cursor_ < end_ && *cursor_ == ' ') {
        ++indent;
        ++cursor_;
    }
    if (cursor_ < end_ && *cursor_ == '\t') {
        throw_parse_error("Tabs are not allowed for indentation in YAML");
    }
    return indent;
}

void yaml_parser::handle_indent_change(const size_t new_indent) { current_indent_ = new_indent; }

void yaml_parser::parse_directive() {
    if (current() != '%') {
        return;
    }
    skip_to_next_line();
}

string_view yaml_parser::parse_anchor() {
    if (current() != '&') {
        return string_view();
    }
    advance();
    const char* start = cursor_;
    while (cursor_ < end_ && !is_space(*cursor_) && !is_break(*cursor_) && *cursor_ != ',' && *cursor_ != ']' &&
           *cursor_ != '}' && *cursor_ != '#' && *cursor_ != ':') {
        ++cursor_;
    }
    return string_view(start, static_cast<size_t>(cursor_ - start));
}

void yaml_parser::register_anchor(const string_view anchor, const yaml_value& value) {
    for (const anchor_entry& entry: anchors_) {
        if (entry.name == anchor) {
            throw_parse_error("Duplicate anchor");
        }
    }
    anchors_.push_back(anchor_entry{anchor, value});
}

yaml_value yaml_parser::parse_alias() {
    expect('*');
    const char* start = cursor_;
    while (cursor_ < end_ && !is_space(*cursor_) && !is_break(*cursor_) && !is_flow_char(*cursor_) && *cursor_ != '#') {
        ++cursor_;
    }
    const string_view name(start, static_cast<size_t>(cursor_ - start));
    for (const anchor_entry& entry: anchors_) {
        if (entry.name == name) {
            return entry.value;
        }
    }
    throw_parse_error("Undefined alias");
}

void yaml_parser::skip_tag() noexcept {
    if (current() != '!') {
        return;
    }
    advance();
    if (current() == '!') {
        advance();
    }
    while (cursor_ < end_ && !is_space(*cursor_) && !is_break(*cursor_) && *cursor_ != ',') {
        ++cursor_;
    }
    skip_whitespace_inline();
}

string_view yaml_parser::parse_tag() {
    if (current() != '!') {
        return string_view();
    }
    advance();
    if (current() == '<') {
        advance();
        const char* content = cursor_;
        while (cursor_ < end_ && *cursor_ != '>') {
            ++cursor_;
        }
        const string_view tag(content, static_cast<size_t>(cursor_ - content));
        if (current() == '>') {
            advance();
        } else {
            throw_parse_error("Unclosed tag");
        }
        return tag;
    }
    if (current() == '!') {
        advance();
        const char* body = cursor_;
        while (cursor_ < end_ && !is_space(*cursor_) && !is_break(*cursor_) && *cursor_ != ':' && *cursor_ != ',' &&
               *cursor_ != '[' && *cursor_ != ']' && *cursor_ != '{' && *cursor_ != '}' && *cursor_ != '#') {
            ++cursor_;
        }
        return string_view(body, static_cast<size_t>(cursor_ - body));
    }
    const char* start = cursor_;
    while (cursor_ < end_ && !is_space(*cursor_) && !is_break(*cursor_) && *cursor_ != ':' && *cursor_ != ',' &&
           *cursor_ != '[' && *cursor_ != ']' && *cursor_ != '{' && *cursor_ != '}' && *cursor_ != '#') {
        ++cursor_;
    }
    return string_view(start, static_cast<size_t>(cursor_ - start));
}

NEFORCE_ALWAYS_INLINE_INLINE bool yaml_parser::has_anchor() const noexcept { return current() == '&'; }

NEFORCE_ALWAYS_INLINE_INLINE bool yaml_parser::has_alias() const noexcept { return current() == '*'; }

NEFORCE_ALWAYS_INLINE_INLINE bool yaml_parser::is_document_start() const noexcept {
    return cursor_ + 2 < end_ && cursor_[0] == '-' && cursor_[1] == '-' && cursor_[2] == '-';
}

NEFORCE_ALWAYS_INLINE_INLINE bool yaml_parser::is_document_end() const noexcept {
    return cursor_ + 2 < end_ && cursor_[0] == '.' && cursor_[1] == '.' && cursor_[2] == '.';
}

void yaml_parser::parse_document_start() {
    if (is_document_start()) {
        cursor_ += 3;
    }
}

void yaml_parser::parse_document_end() {
    if (is_document_end()) {
        cursor_ += 3;
    }
}

codepoint yaml_parser::parse_unicode_escape(const size_t digits) {
    uint32_t code_point = 0;
    for (size_t i = 0; i < digits; ++i) {
        const char ch = current();
        if (!is_xdigit(ch)) {
            throw_parse_error("Invalid unicode escape sequence");
        }
        code_point = code_point * 16 +
                     (ch <= '9' ? static_cast<uint32_t>(ch - '0') : static_cast<uint32_t>((ch | 0x20) - 'a' + 10));
        advance();
    }
    if (code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF)) {
        throw_parse_error("Unicode code point out of range");
    }
    return codepoint(code_point);
}

yaml_value yaml_parser::parse_plain_string() {
    if (is_indicator_char(current()) && current() != '-') {
        throw_parse_error("Plain string cannot start with indicator");
    }
    const char* start = cursor_;
    for (;;) {
        cursor_ = find_plain_special(cursor_, end_, in_flow_context_);
        if (cursor_ >= end_) {
            break;
        }
        const char ch = *cursor_;
        if (is_break(ch) || ch == '#') {
            break;
        }
        if (ch == ':' && (peek() == ' ' || is_break(peek()) || peek() == '\0')) {
            break;
        }
        if (in_flow_context_ && (ch == ',' || is_flow_char(ch))) {
            break;
        }
        ++cursor_;
    }
    const char* finish = cursor_;
    while (finish > start && finish[-1] == ' ') {
        --finish;
    }
    return document_.make_string_value(string_view(start, static_cast<size_t>(finish - start)), yaml_string::Plain,
                                       owning_);
}

yaml_value yaml_parser::parse_single_quoted_string() {
    expect('\'');
    const char* content = cursor_;
    const char* finish = content;
    bool escaped = false;
    while (cursor_ < end_) {
        const char ch = *cursor_;
        if (ch == '\'') {
            if (peek() == '\'') {
                escaped = true;
                cursor_ += 2;
                continue;
            }
            finish = cursor_;
            advance();
            break;
        }
        if (is_break(ch)) {
            escaped = true;
        }
        advance();
        if (!escaped) {
            finish = cursor_;
        }
    }
    if (!escaped) {
        return document_.make_string_value(string_view(content, static_cast<size_t>(finish - content)),
                                           yaml_string::SingleQuoted, owning_);
    }

    string result;
    cursor_ = content;
    while (cursor_ < end_) {
        const char ch = *cursor_;
        if (ch == '\'') {
            if (peek() == '\'') {
                result += '\'';
                cursor_ += 2;
                continue;
            }
            advance();
            break;
        }
        if (is_break(ch)) {
            result += ' ';
            skip_to_next_line();
            continue;
        }
        result += ch;
        advance();
    }
    return document_.make_string_value(string_view(result.data(), result.size()), yaml_string::SingleQuoted, true);
}

yaml_value yaml_parser::parse_double_quoted_string() {
    expect('"');
    const char* content = cursor_;
    bool escaped = false;
    while (cursor_ < end_) {
        const char ch = *cursor_;
        if (ch == '"') {
            break;
        }
        if (ch == '\\' || is_break(ch)) {
            escaped = true;
        }
        advance();
    }
    const char* finish = cursor_;
    if (!escaped) {
        expect('"');
        return document_.make_string_value(string_view(content, static_cast<size_t>(finish - content)),
                                           yaml_string::DoubleQuoted, owning_);
    }

    string result;
    cursor_ = content;
    while (cursor_ < end_) {
        const char ch = *cursor_;
        if (ch == '"') {
            advance();
            break;
        }
        if (ch == '\\') {
            advance();
            const char next = current();
            switch (next) {
                case '0':
                    result += '\0';
                    advance();
                    break;
                case 'a':
                    result += '\a';
                    advance();
                    break;
                case 'b':
                    result += '\b';
                    advance();
                    break;
                case 't':
                    result += '\t';
                    advance();
                    break;
                case 'n':
                    result += '\n';
                    advance();
                    break;
                case 'v':
                    result += '\v';
                    advance();
                    break;
                case 'f':
                    result += '\f';
                    advance();
                    break;
                case 'r':
                    result += '\r';
                    advance();
                    break;
                case 'e':
                    result += '\x1B';
                    advance();
                    break;
                case ' ':
                    advance();
                    break;
                case '"':
                    result += '"';
                    advance();
                    break;
                case '/':
                    result += '/';
                    advance();
                    break;
                case '\\':
                    result += '\\';
                    advance();
                    break;
                case 'x': {
                    advance();
                    const codepoint code = parse_unicode_escape(2);
                    code.append_to(result);
                    break;
                }
                case 'u': {
                    advance();
                    const codepoint code = parse_unicode_escape(4);
                    code.append_to(result);
                    break;
                }
                case 'U': {
                    advance();
                    const codepoint code = parse_unicode_escape(8);
                    code.append_to(result);
                    break;
                }
                case '\n':
                    advance();
                    skip_whitespace_inline();
                    break;
                default:
                    throw_parse_error("Invalid escape sequence");
            }
            continue;
        }
        if (is_break(ch)) {
            result += ' ';
            skip_to_next_line();
            skip_whitespace_inline();
            continue;
        }
        result += ch;
        advance();
    }
    return document_.make_string_value(string_view(result.data(), result.size()), yaml_string::DoubleQuoted, true);
}

string yaml_parser::parse_multiline_string(const bool is_literal) {
    advance();
    int explicit_indent = -1;
    char chomping = 'c';
    if (current() >= '1' && current() <= '9') {
        explicit_indent = current() - '0';
        advance();
    }
    if (current() == '+' || current() == '-') {
        chomping = current();
        advance();
        if (explicit_indent < 0 && current() >= '1' && current() <= '9') {
            explicit_indent = current() - '0';
            advance();
        }
    }
    skip_whitespace_inline();
    skip_comment();
    if (!is_break(current()) && !eof()) {
        throw_parse_error("Invalid block scalar header");
    }
    skip_to_next_line();
    const size_t base_indent = current_indent_;
    size_t block_indent = 0;
    while (!eof() && is_break(current())) {
        skip_to_next_line();
    }

    if (explicit_indent > 0) {
        block_indent = base_indent + static_cast<size_t>(explicit_indent);
    } else {
        const char* saved = cursor_;
        size_t first_content_indent = 0;
        bool found = false;
        while (!eof()) {
            size_t indent = 0;
            while (!eof() && current() == ' ') {
                ++indent;
                advance();
            }
            if (!eof() && !is_break(current())) {
                first_content_indent = indent;
                found = true;
                break;
            }
            if (!eof()) {
                skip_to_next_line();
            }
        }
        cursor_ = saved;
        if (!found) {
            return "";
        }
        if (first_content_indent <= base_indent) {
            throw_parse_error("Invalid block scalar indentation");
        }
        block_indent = first_content_indent;
    }

    vector<string> lines;
    while (!eof()) {
        const char* line_start = cursor_;
        const char* content = cursor_;
        size_t line_indent = 0;
        while (content < end_ && *content == ' ' && (explicit_indent <= 0 || line_indent < block_indent)) {
            ++line_indent;
            ++content;
        }
        if (content < end_ && is_break(*content)) {
            cursor_ = content;
            lines.push_back(string());
            skip_to_next_line();
            continue;
        }
        if (line_indent < block_indent) {
            cursor_ = line_start;
            break;
        }
        cursor_ = content;
        const char* line_end = cursor_;
        while (line_end < end_ && !is_break(*line_end)) {
            ++line_end;
        }
        lines.push_back(string(cursor_, static_cast<size_t>(line_end - cursor_)));
        cursor_ = line_end;
        if (cursor_ < end_) {
            skip_to_next_line();
        }
    }

    if (chomping == '-') {
        while (!lines.empty() && lines.back().empty()) {
            lines.pop_back();
        }
    } else if (chomping == 'c') {
        while (lines.size() > 1 && lines.back().empty() && lines[lines.size() - 2].empty()) {
            lines.pop_back();
        }
    }

    string result;
    if (is_literal) {
        for (size_t i = 0; i < lines.size(); ++i) {
            result += lines[i];
            if (i + 1 < lines.size() || chomping == '+') {
                result += '\n';
            }
        }
    } else {
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) {
                if (lines[i].empty() || lines[i - 1].empty()) {
                    result += '\n';
                } else {
                    result += ' ';
                }
            }
            result += lines[i];
        }
        if (chomping == '+' && !lines.empty()) {
            result += '\n';
        }
    }
    return result;
}

yaml_value yaml_parser::parse_literal_string() {
    const string content = parse_multiline_string(true);
    return document_.make_string_value(string_view(content.data(), content.size()), yaml_string::Literal, true);
}

yaml_value yaml_parser::parse_folded_string() {
    const string content = parse_multiline_string(false);
    return document_.make_string_value(string_view(content.data(), content.size()), yaml_string::Folded, true);
}

yaml_value yaml_parser::parse_null() {
    const char* start = cursor_;
    while (cursor_ < end_ && (is_alpha_or_digit(*cursor_) || *cursor_ == '~')) {
        ++cursor_;
    }
    const string_view word(start, static_cast<size_t>(cursor_ - start));
    if (word == "null" || word == "Null" || word == "NULL" || word == "~") {
        return document_.make_null_value();
    }
    cursor_ = start;
    throw_parse_error("Invalid null value");
}

yaml_value yaml_parser::parse_boolean() {
    const char* start = cursor_;
    while (cursor_ < end_ && is_alpha(*cursor_)) {
        ++cursor_;
    }
    const string_view word(start, static_cast<size_t>(cursor_ - start));
    if (equal_ignore_case(word, "true") || equal_ignore_case(word, "yes") || equal_ignore_case(word, "on") ||
        equal_ignore_case(word, "y")) {
        return document_.make_boolean_value(true);
    }
    if (equal_ignore_case(word, "false") || equal_ignore_case(word, "no") || equal_ignore_case(word, "off") ||
        equal_ignore_case(word, "n")) {
        return document_.make_boolean_value(false);
    }
    cursor_ = start;
    throw_parse_error("Invalid boolean value");
}

yaml_value yaml_parser::parse_number() {
    const char* start = cursor_;
    bool negative = false;

    {
        const char* probe = cursor_;
        bool fast_negative = false;
        if (probe < end_ && (*probe == '+' || *probe == '-')) {
            fast_negative = *probe == '-';
            ++probe;
        }
        if (probe < end_ && is_digit(*probe)) {
            if (*probe == '0' && probe + 1 < end_ && is_digit(probe[1])) {
                throw_parse_error("Leading zeros are not allowed in decimal integers");
            }
            uint64_t accumulated = 0;
            bool overflow = false;
            while (probe < end_ && is_digit(*probe)) {
                const auto digit = static_cast<uint64_t>(*probe - '0');
                if (accumulated > (numeric_traits<uint64_t>::max() - digit) / 10) {
                    overflow = true;
                }
                accumulated = accumulated * 10 + digit;
                ++probe;
            }
            const char following = probe < end_ ? *probe : '\0';
            const bool float_like = following == '.' || following == 'e' || following == 'E' || following == '_';
            const bool radix_like = following == 'x' || following == 'X' || following == 'o' || following == 'O' ||
                                    following == 'b' || following == 'B';
            if (!float_like && !radix_like) {
                cursor_ = probe;
                if (!overflow) {
                    if (fast_negative) {
                        if (accumulated == (1ULL << 63)) {
                            return document_.make_integer_value(numeric_traits<int64_t>::min());
                        }
                        if (accumulated <= static_cast<uint64_t>(numeric_traits<int64_t>::max())) {
                            return document_.make_integer_value(-static_cast<int64_t>(accumulated));
                        }
                    } else if (accumulated <= static_cast<uint64_t>(numeric_traits<int64_t>::max())) {
                        return document_.make_integer_value(static_cast<int64_t>(accumulated));
                    }
                }
                try {
                    return document_.make_integer_value(
                            to_int64(string_view(start, static_cast<size_t>(probe - start))));
                } catch (...) {
                    throw_parse_error("Invalid number");
                }
            }
        }
        cursor_ = start;
    }
    bool is_float = false;
    bool is_hex = false;
    bool is_octal = false;
    bool is_binary = false;

    if (current() == '-' || current() == '+') {
        negative = current() == '-';
        advance();
    }

    if (current() == '.') {
        const char* saved = cursor_;
        advance();
        const char* special_start = cursor_;
        while (cursor_ < end_ && is_alpha(*cursor_)) {
            ++cursor_;
        }
        const string_view special(special_start, static_cast<size_t>(cursor_ - special_start));
        if (equal_ignore_case(special, "inf") || equal_ignore_case(special, "infinity")) {
            return document_.make_float_value(negative ? -numeric_traits<double>::infinity()
                                                       : numeric_traits<double>::infinity());
        }
        if (equal_ignore_case(special, "nan")) {
            return document_.make_float_value(numeric_traits<double>::quiet_nan());
        }
        cursor_ = saved;
        is_float = true;
    }

    if (current() == '0' && !eof()) {
        advance();
        if (current() == 'x' || current() == 'X') {
            is_hex = true;
            advance();
        } else if (current() == 'o' || current() == 'O') {
            is_octal = true;
            advance();
        } else if (current() == 'b' || current() == 'B') {
            is_binary = true;
            advance();
        } else {
            const char* probe = cursor_;
            while (probe < end_ && *probe == '_') {
                ++probe;
            }
            if (probe < end_ && is_digit(*probe)) {
                throw_parse_error("Leading zeros are not allowed in decimal integers");
            }
        }
    }

    while (cursor_ < end_) {
        const char ch = *cursor_;
        if ((is_hex && is_xdigit(ch)) || (is_octal && ch >= '0' && ch <= '7') ||
            (is_binary && (ch == '0' || ch == '1')) || (!is_hex && !is_octal && !is_binary && is_digit(ch))) {
            advance();
        } else if (ch == '_') {
            advance();
        } else if (ch == '.') {
            if (is_hex || is_octal || is_binary) {
                throw_parse_error("Float not allowed in this number format");
            }
            if (is_float) {
                break;
            }
            advance();
            is_float = true;
        } else if (ch == 'e' || ch == 'E') {
            if (is_hex || is_octal || is_binary) {
                break;
            }
            is_float = true;
            advance();
            if (current() == '+' || current() == '-') {
                advance();
            }
        } else {
            break;
        }
    }

    string num_str;
    num_str.reserve(static_cast<size_t>(cursor_ - start));
    for (const char* position = start; position < cursor_; ++position) {
        if (*position != '_') {
            num_str += *position;
        }
    }
    if (num_str.empty() || num_str == "." || num_str == "-" || num_str == "+") {
        throw_parse_error("Invalid number");
    }
    try {
        if (is_float) {
            return document_.make_float_value(float64::parse(num_str.view()).value());
        }
        if (is_hex) {
            return document_.make_integer_value(to_int64(num_str.view(), nullptr, 16));
        }
        if (is_octal) {
            return document_.make_integer_value(to_int64(num_str.view(2), nullptr, 8));
        }
        if (is_binary) {
            return document_.make_integer_value(to_int64(num_str.view(2), nullptr, 2));
        }
        return document_.make_integer_value(to_int64(num_str.view()));
    } catch (...) {
        throw_parse_error("Invalid number");
    }
}

yaml_value yaml_parser::parse_timestamp(const string_view str) {
    datetime value;
    if (value.try_parse_RFC3339(str) || value.try_parse_ISO8601(str)) {
        return document_.make_timestamp_value(value);
    }
    throw_parse_error("Invalid timestamp");
}

yaml_value yaml_parser::parse_scalar() {
    skip_whitespace_inline();
    if (has_alias()) {
        return parse_alias();
    }

    string_view anchor;
    string_view tag;
    if (current() == '&') {
        anchor = parse_anchor();
        skip_whitespace_inline();
    }
    if (current() == '!') {
        tag = parse_tag();
        skip_whitespace_inline();
    }

    yaml_value value = parse_scalar_body();
    if (!anchor.empty() || !tag.empty()) {
        document_.set_meta(value, anchor, tag, owning_);
    }
    if (!anchor.empty()) {
        register_anchor(anchor, value);
    }
    return value;
}

yaml_value yaml_parser::parse_scalar_body() {
    yaml_value value;
    bool has_value = false;
    const char ch = current();

    if (ch == '~' || is_alpha(ch)) {
        const char* word_start = cursor_;
        if (ch == '~') {
            advance();
        } else {
            while (cursor_ < end_ && is_alpha(*cursor_)) {
                ++cursor_;
            }
        }
        const string_view word(word_start, static_cast<size_t>(cursor_ - word_start));
        const char next = current();
        const bool is_null_word = word == "~" || equal_ignore_case(word, "null");
        const bool is_bool_word = equal_ignore_case(word, "true") || equal_ignore_case(word, "false") ||
                                  equal_ignore_case(word, "yes") || equal_ignore_case(word, "no") ||
                                  equal_ignore_case(word, "on") || equal_ignore_case(word, "off") ||
                                  equal_ignore_case(word, "y") || equal_ignore_case(word, "n");
        if ((is_null_word || is_bool_word) && is_terminator(next)) {
            if (is_null_word) {
                value = document_.make_null_value();
            } else {
                cursor_ = word_start;
                value = parse_boolean();
            }
            has_value = true;
        } else {
            cursor_ = word_start;
        }
    }

    if (!has_value) {
        const char next = current();
        if (next == '-' || next == '+' || is_digit(next) || next == '.') {
            const char* saved = cursor_;
            const char* probe = cursor_;
            bool has_dash = false;
            bool has_colon = false;
            bool has_time_marker = false;
            size_t hyphen_count = 0;
            while (probe < end_ && (is_digit(*probe) || *probe == '-' || *probe == ':' || *probe == '.' ||
                                    *probe == 'T' || *probe == 'Z' || *probe == '+' || *probe == ' ')) {
                if (*probe == '-') {
                    has_dash = true;
                    ++hyphen_count;
                } else if (*probe == ':') {
                    has_colon = true;
                } else if (*probe == 'T') {
                    has_time_marker = true;
                }
                ++probe;
            }
            const string_view candidate(saved, static_cast<size_t>(probe - saved));
            const bool is_timestamp = (has_dash && (has_colon || has_time_marker)) || hyphen_count >= 2;
            if (is_timestamp) {
                try {
                    value = parse_timestamp(candidate);
                    cursor_ = probe;
                } catch (...) {
                    cursor_ = saved;
                    value = parse_plain_string();
                }
            } else {
                value = parse_number();
            }
        } else if (next == '"') {
            value = parse_double_quoted_string();
        } else if (next == '\'') {
            value = parse_single_quoted_string();
        } else if (next == '|') {
            value = parse_literal_string();
        } else if (next == '>') {
            value = parse_folded_string();
        } else {
            value = parse_plain_string();
        }
    }

    return value;
}

yaml_value yaml_parser::parse_flow_sequence() {
    expect('[');
    const bool previous_flow = in_flow_context_;
    in_flow_context_ = true;
    yaml_value* node = document_.new_sequence(true);
    skip_whitespace_inline();
    if (current() == ']') {
        advance();
        in_flow_context_ = previous_flow;
        return *node;
    }
    while (!eof()) {
        skip_whitespace_inline();
        skip_comment();
        if (is_break(current())) {
            skip_to_next_line();
            skip_whitespace_inline();
        }
        if (current() == ']') {
            advance();
            break;
        }
        const char value_start = current();
        yaml_value element;
        if (value_start == '&' || value_start == '!' || value_start == '*' || value_start == '[' ||
            value_start == '{') {
            element = parse_inline_value();
        } else {
            element = parse_scalar_body();
        }
        document_.sequence_append(*node, element);
        skip_whitespace_inline();
        if (is_break(current())) {
            skip_to_next_line();
            skip_whitespace_inline();
        }
        if (current() == ',') {
            advance();
            skip_whitespace_inline();
            if (is_break(current())) {
                skip_to_next_line();
                skip_whitespace_inline();
            }
            if (current() == ']') {
                advance();
                break;
            }
        } else if (current() != ']') {
            throw_parse_error("Expected ',' or ']' in flow sequence");
        }
    }
    in_flow_context_ = previous_flow;
    return *node;
}

yaml_value yaml_parser::parse_flow_mapping() {
    expect('{');
    const bool previous_flow = in_flow_context_;
    in_flow_context_ = true;
    yaml_value* node = document_.new_mapping(true);
    skip_whitespace_inline();
    if (current() == '}') {
        advance();
        in_flow_context_ = previous_flow;
        return *node;
    }
    while (!eof()) {
        skip_whitespace_inline();
        skip_comment();
        if (is_break(current())) {
            skip_to_next_line();
            skip_whitespace_inline();
        }
        if (current() == '}') {
            advance();
            break;
        }
        const char key_start = current();
        const string_view key = key_start == '"' || key_start == '\'' ? parse_quoted_key() : parse_plain_key();
        skip_whitespace_inline();
        expect(':');
        skip_whitespace_inline();
        const char value_start = current();
        yaml_value value;
        if (value_start == '&' || value_start == '!' || value_start == '*' || value_start == '[' ||
            value_start == '{') {
            value = parse_inline_value();
        } else {
            value = parse_scalar_body();
        }
        document_.mapping_append(*node, key, value, false);
        skip_whitespace_inline();
        if (is_break(current())) {
            skip_to_next_line();
            skip_whitespace_inline();
        }
        if (current() == ',') {
            advance();
            skip_whitespace_inline();
            if (is_break(current())) {
                skip_to_next_line();
                skip_whitespace_inline();
            }
            if (current() == '}') {
                advance();
                break;
            }
        } else if (current() != '}') {
            throw_parse_error("Expected ',' or '}' in flow mapping");
        }
    }
    in_flow_context_ = previous_flow;
    return *node;
}

yaml_value yaml_parser::parse_block_sequence() {
    yaml_value* node = document_.new_sequence(false);
    const size_t sequence_indent = current_indent_;
    bool at_line_start = true;
    while (!eof()) {
        const size_t line_indent = skip_indent();
        if (!at_line_start && line_indent < sequence_indent) {
            break;
        }
        if (line_indent > sequence_indent && current() != '-') {
            throw_parse_error("Invalid indentation in block sequence");
        }
        at_line_start = false;
        if (is_break(current()) || current() == '#') {
            skip_to_next_line();
            continue;
        }
        if (current() != '-') {
            break;
        }
        advance();
        if (!is_space(current()) && !is_break(current()) && !eof()) {
            throw_parse_error("Expected whitespace after '-' in block sequence");
        }
        skip_whitespace_inline();
        yaml_value value;
        if (is_break(current()) || current() == '#') {
            skip_to_next_line();
            const size_t value_indent = skip_indent();
            if (value_indent <= sequence_indent) {
                value = document_.make_null_value();
            } else {
                const size_t saved_indent = current_indent_;
                current_indent_ = value_indent;
                value = parse_block_value();
                current_indent_ = saved_indent;
            }
        } else {
            const char* saved = cursor_;
            bool mapping_like = false;
            while (cursor_ < end_ && !is_break(*cursor_)) {
                if (*cursor_ == ':' && (peek() == ' ' || is_break(peek()) || peek() == '\0')) {
                    mapping_like = true;
                    break;
                }
                if (is_flow_char(*cursor_)) {
                    break;
                }
                ++cursor_;
            }
            cursor_ = saved;
            if (mapping_like) {
                const size_t saved_indent = current_indent_;
                current_indent_ = sequence_indent + 2;
                value = parse_block_mapping(false);
                current_indent_ = saved_indent;
            } else {
                value = parse_inline_value();
                skip_to_next_line();
            }
        }
        document_.sequence_append(*node, value);
    }
    return *node;
}

yaml_value yaml_parser::parse_block_mapping(const bool parent_skipped_indent) {
    yaml_value* node = document_.new_mapping(false);
    const size_t mapping_indent = current_indent_;
    size_t first_key_indent = 0;
    bool is_first_key = true;
    size_t subsequent_key_indent = 0;
    bool first_key_on_same_line = false;

    while (!eof()) {
        while (!eof() && (is_break(current()) || current() == '#')) {
            if (current() == '#') {
                skip_comment();
            }
            skip_to_next_line();
        }
        if (eof()) {
            break;
        }
        const size_t line_indent = peek_indent();
        if (is_first_key) {
            if (line_indent == 0 && mapping_indent > 0) {
                if (parent_skipped_indent) {
                    first_key_indent = mapping_indent;
                    is_first_key = false;
                } else {
                    first_key_on_same_line = true;
                    first_key_indent = 0;
                    is_first_key = false;
                }
            } else if (line_indent < mapping_indent) {
                break;
            } else {
                first_key_indent = line_indent;
                is_first_key = false;
            }
        } else if (first_key_on_same_line) {
            if (line_indent < mapping_indent) {
                break;
            }
            if (line_indent == mapping_indent) {
                const char* line_begin = cursor_;
                skip_indent();
                if (current() == '-' && (peek() == ' ' || is_break(peek()))) {
                    break;
                }
                cursor_ = line_begin;
            }
            if (subsequent_key_indent == 0) {
                if (line_indent < mapping_indent) {
                    throw_parse_error("Subsequent keys must be indented more than the parent context");
                }
                subsequent_key_indent = line_indent;
            } else if (line_indent != subsequent_key_indent) {
                throw_parse_error("All keys in a block mapping must have the same indentation");
            }
        } else {
            if (line_indent < mapping_indent) {
                break;
            }
            if (line_indent != first_key_indent) {
                throw_parse_error("All keys in a block mapping must have the same indentation");
            }
        }

        skip_indent();
        if (current() == '.' && peek() == '.' && peek(2) == '.') {
            break;
        }
        if (current() == '-' && peek() == '-' && peek(2) == '-') {
            break;
        }
        if (current() == '-' && (peek() == ' ' || is_break(peek()))) {
            break;
        }
        if (current() == '&') {
            (void) parse_anchor();
        }
        if (current() == '!') {
            skip_tag();
        }

        if (current() == '?') {
            const size_t complex_key_indent = current_indent_;
            advance();
            skip_whitespace_inline();
            yaml_value complex_key;
            string_view key_anchor;
            string_view key_tag;
            if (current() == '&') {
                key_anchor = parse_anchor();
                skip_whitespace_inline();
            }
            if (current() == '!') {
                key_tag = parse_tag();
                skip_whitespace_inline();
            }
            if (is_break(current()) || current() == '#') {
                if (current() == '#') {
                    skip_comment();
                }
                skip_to_next_line();
                const size_t key_indent = skip_indent();
                if (key_indent <= complex_key_indent) {
                    throw_parse_error("Complex key must be indented more than '?'");
                }
                const size_t saved_indent = current_indent_;
                current_indent_ = key_indent;
                complex_key = parse_block_value();
                current_indent_ = saved_indent;
            } else {
                complex_key = parse_inline_value();
                if (!eof() && is_break(current())) {
                    skip_to_next_line();
                }
            }
            if (!key_anchor.empty() || !key_tag.empty()) {
                document_.set_meta(complex_key, key_anchor, key_tag, owning_);
            }
            if (!key_anchor.empty()) {
                register_anchor(key_anchor, complex_key);
            }
            skip_blank_lines();
            const size_t colon_indent = skip_indent();
            if (colon_indent < complex_key_indent) {
                throw_parse_error("Expected ':' at same or greater indent after complex key");
            }
            if (current() != ':') {
                throw_parse_error("Expected ':' after complex key");
            }
            advance();
            skip_whitespace_inline();

            string_view value_anchor;
            string_view value_tag;
            if (current() == '&') {
                value_anchor = parse_anchor();
                skip_whitespace_inline();
            }
            if (current() == '!') {
                value_tag = parse_tag();
                skip_whitespace_inline();
            }

            yaml_value value;
            if (is_break(current()) || current() == '#' || eof()) {
                if (current() == '#') {
                    skip_comment();
                }
                skip_to_next_line();
                if (eof()) {
                    value = document_.make_null_value();
                } else {
                    const size_t value_indent = skip_indent();
                    if (value_indent <= colon_indent) {
                        value = document_.make_null_value();
                    } else {
                        const size_t saved_indent = current_indent_;
                        current_indent_ = value_indent;
                        value = parse_block_value();
                        current_indent_ = saved_indent;
                    }
                }
            } else {
                value = parse_inline_value();
                if (!eof() && !is_break(current())) {
                    skip_whitespace_inline();
                }
                if (!eof() && current() == '#') {
                    skip_comment();
                }
                if (!eof() && is_break(current())) {
                    skip_to_next_line();
                }
            }
            if (!value_anchor.empty() || !value_tag.empty()) {
                document_.set_meta(value, value_anchor, value_tag, owning_);
            }
            if (!value_anchor.empty()) {
                register_anchor(value_anchor, value);
            }
            const string key_text = complex_key.to_string();
            document_.mapping_append(*node, string_view(key_text.data(), key_text.size()), value, true);
            continue;
        }

        const string_view key = parse_key();
        skip_whitespace_inline();
        if (current() != ':') {
            throw_parse_error("Expected ':' after key in block mapping");
        }
        advance();
        skip_whitespace_inline();

        string_view value_anchor;
        string_view value_tag;
        if (current() == '&') {
            value_anchor = parse_anchor();
            skip_whitespace_inline();
        }
        if (current() == '!') {
            value_tag = parse_tag();
            skip_whitespace_inline();
        }

        yaml_value value;
        if (is_break(current()) || current() == '#' || eof()) {
            if (current() == '#') {
                skip_comment();
            }
            skip_to_next_line();
            if (eof()) {
                value = document_.make_null_value();
            } else {
                const size_t value_indent = peek_indent();
                const size_t effective_key_indent =
                        first_key_on_same_line ? (subsequent_key_indent > 0 ? subsequent_key_indent : mapping_indent)
                                               : first_key_indent;
                if (value_indent <= effective_key_indent) {
                    value = document_.make_null_value();
                } else {
                    const size_t saved_indent = current_indent_;
                    current_indent_ = value_indent;
                    value = parse_block_value();
                    current_indent_ = saved_indent;
                }
            }
        } else {
            value = parse_inline_value();
            if (!eof() && !is_break(current())) {
                skip_whitespace_inline();
            }
            if (!eof() && current() == '#') {
                skip_comment();
            }
            if (!eof() && is_break(current())) {
                skip_to_next_line();
            }
        }

        if (!value_anchor.empty() || !value_tag.empty()) {
            document_.set_meta(value, value_anchor, value_tag, owning_);
        }
        if (!value_anchor.empty()) {
            register_anchor(value_anchor, value);
        }
        if (key == "<<") {
            if (value.is_mapping()) {
                document_.append_missing(*node, value.as_mapping());
            } else if (value.is_sequence()) {
                const yaml_sequence sequence = value.as_sequence();
                for (size_t i = 0; i < sequence.size(); ++i) {
                    const yaml_value* element = sequence.get_element(i);
                    if (element != nullptr && element->is_mapping()) {
                        document_.append_missing(*node, element->as_mapping());
                    }
                }
            }
        } else {
            document_.mapping_append(*node, key, value, false);
        }
    }
    return *node;
}

string_view yaml_parser::parse_key() {
    skip_whitespace_inline();
    if (current() == '"' || current() == '\'') {
        return parse_quoted_key();
    }
    return parse_plain_key();
}

string_view yaml_parser::parse_plain_key() {
    const char* start = cursor_;
    while (cursor_ < end_) {
        const char ch = *cursor_;
        if (ch == ':' && (peek() == ' ' || is_break(peek()) || peek() == '\0' || is_flow_char(peek()))) {
            break;
        }
        if (is_break(ch) || is_flow_char(ch)) {
            break;
        }
        if (ch == '#') {
            if (cursor_ > start && cursor_[-1] == ' ') {
                --cursor_;
                break;
            }
        }
        ++cursor_;
    }
    const char* finish = cursor_;
    while (finish > start && finish[-1] == ' ') {
        --finish;
    }
    return owning_ ? document_.store_string(string_view(start, static_cast<size_t>(finish - start)), true)
                   : string_view(start, static_cast<size_t>(finish - start));
}

string_view yaml_parser::parse_quoted_key() {
    const yaml_value value = current() == '"' ? parse_double_quoted_string() : parse_single_quoted_string();
    const string_view text = value.as_string().get_value();
    return document_.store_string(text, true);
}

yaml_value yaml_parser::parse_inline_value() {
    skip_whitespace_inline();
    if (has_alias()) {
        return parse_alias();
    }
    string_view anchor;
    string_view tag;
    if (current() == '&') {
        anchor = parse_anchor();
        skip_whitespace_inline();
    }
    if (current() == '!') {
        tag = parse_tag();
        skip_whitespace_inline();
    }

    yaml_value value;
    const char ch = current();
    if (ch == '[') {
        value = parse_flow_sequence();
    } else if (ch == '{') {
        value = parse_flow_mapping();
    } else {
        value = parse_scalar_body();
    }
    if (!anchor.empty() || !tag.empty()) {
        document_.set_meta(value, anchor, tag, owning_);
    }
    if (!anchor.empty()) {
        register_anchor(anchor, value);
    }
    return value;
}

yaml_value yaml_parser::parse_single_document() {
    skip_blank_lines();
    current_indent_ = 0;
    if (eof() || is_document_end()) {
        return document_.make_null_value();
    }
    current_indent_ = skip_indent();
    return parse_value();
}

yaml_value yaml_parser::parse_block_value() {
    skip_indent();
    if (current() == '?' && (peek() == ' ' || is_break(peek()))) {
        return parse_block_mapping(true);
    }
    if (current() == '-' && (peek() == ' ' || is_break(peek()))) {
        return parse_block_sequence();
    }
    if (current() == '{') {
        return parse_flow_mapping();
    }
    if (current() == '[') {
        return parse_flow_sequence();
    }
    const char* saved = cursor_;
    bool looks_like_mapping = false;
    while (cursor_ < end_ && !is_break(*cursor_)) {
        if (*cursor_ == ':' && (peek() == ' ' || is_break(peek()) || peek() == '\0')) {
            looks_like_mapping = true;
            break;
        }
        ++cursor_;
    }
    cursor_ = saved;
    if (looks_like_mapping) {
        return parse_block_mapping(true);
    }
    return parse_scalar();
}

yaml_value yaml_parser::parse_value() {
    skip_whitespace_inline();
    if (has_alias()) {
        return parse_alias();
    }
    string_view anchor;
    string_view tag;
    if (current() == '&') {
        anchor = parse_anchor();
        skip_whitespace_inline();
    }
    if (current() == '!') {
        tag = parse_tag();
        skip_whitespace_inline();
    }
    if (is_break(current())) {
        skip_to_next_line();
        skip_blank_lines();
    }

    yaml_value value;
    const char ch = current();
    if (ch == '[') {
        value = parse_flow_sequence();
    } else if (ch == '{') {
        value = parse_flow_mapping();
    } else if (ch == '|' || ch == '>') {
        value = parse_scalar();
    } else if (ch == '?') {
        value = parse_block_mapping(false);
    } else {
        const char* saved = cursor_;
        skip_whitespace_inline();
        if (current() == '-' && (peek() == ' ' || is_break(peek()))) {
            cursor_ = saved;
            value = parse_block_sequence();
        } else {
            bool found_colon = false;
            while (cursor_ < end_ && !is_break(*cursor_)) {
                if (*cursor_ == ':' && (peek() == ' ' || is_break(peek()) || peek() == '\0')) {
                    found_colon = true;
                    break;
                }
                ++cursor_;
            }
            cursor_ = saved;
            value = found_colon ? parse_block_mapping(false) : parse_scalar();
        }
    }
    if (!anchor.empty() || !tag.empty()) {
        document_.set_meta(value, anchor, tag, owning_);
    }
    if (!anchor.empty()) {
        register_anchor(anchor, value);
    }
    return value;
}

yaml_document yaml_parser::parse() {
    document_ = yaml_document();
    anchors_.clear();
    if (end_ - begin_ >= 3 && static_cast<uint8_t>(begin_[0]) == 0xEF && static_cast<uint8_t>(begin_[1]) == 0xBB &&
        static_cast<uint8_t>(begin_[2]) == 0xBF) {
        cursor_ = begin_ + 3;
    }
    skip_blank_lines();
    parse_document_start();
    skip_blank_lines();
    if (eof() || is_document_end()) {
        document_.root() = document_.make_null_value();
        return move(document_);
    }
    current_indent_ = skip_indent();
    document_.root() = parse_value();
    document_.set_size_hint(document_.root(), text_.size());

    while (!eof()) {
        if (is_space(current()) || is_break(current())) {
            skip_to_next_line();
        } else if (current() == '#') {
            skip_comment();
            skip_to_next_line();
        } else {
            break;
        }
    }
    parse_document_end();
    while (!eof()) {
        if (is_space(current()) || is_break(current())) {
            skip_to_next_line();
        } else if (current() == '#') {
            skip_comment();
            skip_to_next_line();
        } else if (is_document_start()) {
            throw_parse_error("Multiple documents not supported in single parse");
        } else {
            throw_parse_error("Unexpected content after document");
        }
    }
    return move(document_);
}

optional<yaml_document> yaml_parser::try_parse() noexcept {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

vector<yaml_document> yaml_parser::parse_documents() {
    vector<yaml_document> documents;
    while (!eof()) {
        anchors_.clear();
        document_ = yaml_document();
        skip_blank_lines();
        while (!eof() && current() == '%') {
            parse_directive();
            skip_blank_lines();
        }
        if (eof()) {
            break;
        }
        if (is_document_start()) {
            cursor_ += 3;
            skip_blank_lines();
        }
        if (eof()) {
            break;
        }
        document_.root() = parse_single_document();
        document_.set_size_hint(document_.root(), text_.size());
        documents.push_back(move(document_));
        skip_blank_lines();
        if (is_document_end()) {
            cursor_ += 3;
            skip_blank_lines();
        }
    }
    return documents;
}

optional<vector<yaml_document>> yaml_parser::try_parse_documents() noexcept {
    try {
        return parse_documents();
    } catch (...) {
        return {};
    }
}

void yaml_parser::throw_parse_error(const string& message) const {
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
    string error_msg = message;
    error_msg += " at line " + to_string(line);
    error_msg += ", column " + to_string(column);
    if (cursor_ < end_) {
        error_msg += "\nNear: '";
        const char* context_start = cursor_ - begin_ > 20 ? cursor_ - 20 : begin_;
        const char* context_end = cursor_ + 20 < end_ ? cursor_ + 20 : end_;
        for (const char* position = context_start; position < context_end; ++position) {
            if (*position == '\n') {
                error_msg += "\\n";
            } else if (*position == '\r') {
                error_msg += "\\r";
            } else if (*position == '\t') {
                error_msg += "\\t";
            } else {
                error_msg += *position;
            }
        }
        error_msg += "'";
    }
    NEFORCE_THROW_EXCEPTION(yaml_exception(error_msg.data()));
}

NEFORCE_END_NAMESPACE__
