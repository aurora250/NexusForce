#include <NeForce/core/file/yaml/yaml_parser.hpp>
#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/utility/hexadecimal.hpp>
NEFORCE_BEGIN_NAMESPACE__

char yaml_parser::current() const noexcept { return eof() ? '\0' : yaml_[pos_]; }

char yaml_parser::peek(const size_t offset) const noexcept {
    const size_t peek_pos = pos_ + offset;
    return peek_pos >= len_ ? '\0' : yaml_[peek_pos];
}

bool yaml_parser::eof() const noexcept { return pos_ >= len_; }

void yaml_parser::advance() noexcept {
    if (eof()) {
        return;
    }

    if (current() == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    pos_++;
}

bool yaml_parser::match(const char ch) noexcept {
    if (current() == ch) {
        advance();
        return true;
    }
    return false;
}

bool yaml_parser::expect(const char ch) {
    if (!match(ch)) {
        throw_parse_error(string("Expected '") + ch + "' but got '" + current() + "'");
    }
    return true;
}

bool yaml_parser::is_whitespace(const char ch) const noexcept { return ch == ' ' || ch == '\t'; }

bool yaml_parser::is_newline(const char ch) const noexcept { return ch == '\n' || ch == '\r'; }

void yaml_parser::skip_whitespace_inline() noexcept {
    while (!eof() && is_whitespace(current())) {
        advance();
    }
}

void yaml_parser::skip_comment() noexcept {
    if (current() == '#') {
        while (!eof() && !is_newline(current())) {
            advance();
        }
    }
}

void yaml_parser::skip_to_next_line() noexcept {
    skip_whitespace_inline();
    skip_comment();

    if (current() == '\r') {
        advance();
        if (current() == '\n') {
            advance();
        }
    } else if (current() == '\n') {
        advance();
    }
}

void yaml_parser::skip_blank_lines() noexcept {
    while (!eof()) {
        if (current() == ' ' || is_newline(current())) {
            size_t peek_pos = pos_;
            while (peek_pos < len_ && yaml_[peek_pos] == ' ') {
                peek_pos++;
            }
            const bool is_blank =
                    (peek_pos >= len_) || yaml_[peek_pos] == '\n' || yaml_[peek_pos] == '\r' || yaml_[peek_pos] == '#';
            if (is_blank) {
                skip_to_next_line();
            } else {
                break;
            }
        } else if (current() == '#') {
            skip_comment();
            skip_to_next_line();
        } else {
            break;
        }
    }
}

void yaml_parser::skip_whitespace_and_comments() {
    while (!eof()) {
        if (is_whitespace(current())) {
            advance();
        } else if (current() == '#') {
            skip_comment();
        } else {
            break;
        }
    }
}

size_t yaml_parser::peek_indent() const noexcept {
    size_t indent = 0;
    size_t p = pos_;
    while (p < len_ && yaml_[p] == ' ') {
        ++indent;
        ++p;
    }
    return indent;
}

size_t yaml_parser::skip_indent() {
    size_t indent = 0;
    while (!eof() && current() == ' ') {
        indent++;
        advance();
    }
    if (!eof() && current() == '\t') {
        throw_parse_error("Tabs are not allowed for indentation in YAML");
    }
    return indent;
}

void yaml_parser::handle_indent_change(const size_t new_indent) {
    if (new_indent > current_indent_) {
        current_indent_ = new_indent;
    } else if (new_indent < current_indent_) {
        while (!indent_stack_.empty() && indent_stack_.back().level > new_indent) {
            indent_stack_.pop_back();
        }
        current_indent_ = new_indent;
    }
}

void yaml_parser::parse_directive() {
    if (current() != '%') {
        return;
    }

    advance();

    string name;
    while (!eof() && !is_whitespace(current()) && current() != '\n') {
        name += current();
        advance();
    }
    while (!eof() && current() == ' ') {
        advance();
    }

    string argument;
    while (!eof() && !is_newline(current())) {
        argument += current();
        advance();
    }
    if (!eof() && is_newline(current())) {
        if (current() == '\r' && peek(1) == '\n') {
            advance();
            advance();
        } else {
            advance();
        }
        line_++;
        column_ = 1;
    }
}

bool yaml_parser::has_anchor() const noexcept { return current() == '&'; }

bool yaml_parser::has_alias() const noexcept { return current() == '*'; }

string yaml_parser::parse_anchor() {
    if (current() != '&') {
        return "";
    }
    advance();

    string anchor_name;
    while (!eof() && (is_alpha_or_digit(current()) || current() == '_' || current() == '-')) {
        anchor_name += current();
        advance();
    }
    if (anchor_name.empty()) {
        throw_parse_error("Empty anchor name");
    }
    skip_whitespace_inline();
    return anchor_name;
}

void yaml_parser::register_anchor(const string& anchor, const shared_ptr<yaml_value>& value) {
    if (anchor.empty()) {
        return;
    }

    auto it = anchors_.find(anchor);
    if (it != anchors_.end()) {
        throw_parse_error("Duplicate anchor: &" + anchor);
    }

    anchors_[anchor] = value;
}

shared_ptr<yaml_value> yaml_parser::parse_alias() {
    if (current() != '*') {
        throw_parse_error("Expected '*' for alias");
    }

    advance();
    string alias_name;
    while (!eof() && (is_alpha_or_digit(current()) || current() == '_' || current() == '-')) {
        alias_name += current();
        advance();
    }

    if (alias_name.empty()) {
        throw_parse_error("Empty alias name");
    }
    auto it = anchors_.find(alias_name);
    if (it == anchors_.end()) {
        throw_parse_error("Undefined alias: *" + alias_name);
    }
    return it->second;
}

void yaml_parser::skip_tag() noexcept {
    if (current() != '!') {
        return;
    }
    advance();
    if (current() == '!') {
        advance();
    }
    while (!eof() && !is_whitespace(current()) && !is_newline(current()) && current() != ',') {
        advance();
    }
    skip_whitespace_inline();
}

string yaml_parser::parse_tag() {
    if (current() != '!') {
        return "";
    }

    advance();
    string tag;

    if (current() == '<') {
        advance();
        while (!eof() && current() != '>') {
            tag += current();
            advance();
        }
        if (current() == '>') {
            advance();
        } else {
            throw_parse_error("Unclosed tag");
        }
    } else if (current() == '!') {
        tag += '!';
        advance();
        while (!eof() && !is_whitespace(current()) && !is_newline(current()) && current() != ':' && current() != ',' &&
               current() != '[' && current() != ']' && current() != '{' && current() != '}' && current() != '#') {
            tag += current();
            advance();
        }
    } else {
        while (!eof() && !is_whitespace(current()) && !is_newline(current()) && current() != ':' && current() != ',' &&
               current() != '[' && current() != ']' && current() != '{' && current() != '}' && current() != '#') {
            tag += current();
            advance();
        }
    }
    return tag;
}

bool yaml_parser::is_plain_safe(const char ch) const noexcept {
    return is_alpha_or_digit(ch) || ch == '_' || ch == '-' || ch == '.' || ch == '/' || ch == '+';
}

bool yaml_parser::is_key_char(const char ch) const noexcept { return is_plain_safe(ch) || ch == ' '; }

bool yaml_parser::is_indicator(const char ch) const noexcept {
    return ch == '-' || ch == '?' || ch == ':' || ch == ',' || ch == '[' || ch == ']' || ch == '{' || ch == '}' ||
           ch == '#' || ch == '&' || ch == '*' || ch == '!' || ch == '|' || ch == '>' || ch == '\'' || ch == '"' ||
           ch == '%' || ch == '@' || ch == '`';
}

bool yaml_parser::is_flow_indicator(const char ch) const noexcept {
    return ch == ',' || ch == '[' || ch == ']' || ch == '{' || ch == '}';
}

bool yaml_parser::is_document_start() const noexcept {
    return current() == '-' && peek(1) == '-' && peek(2) == '-' &&
           (peek(3) == ' ' || peek(3) == '\n' || peek(3) == '\t' || peek(3) == '\0');
}

bool yaml_parser::is_document_end() const noexcept {
    return current() == '.' && peek(1) == '.' && peek(2) == '.' &&
           (peek(3) == ' ' || peek(3) == '\n' || peek(3) == '\t' || peek(3) == '\0');
}

void yaml_parser::parse_document_start() {
    if (is_document_start()) {
        advance();
        advance();
        advance();
        skip_to_next_line();
    }
}

void yaml_parser::parse_document_end() {
    if (is_document_end()) {
        advance();
        advance();
        advance();
        skip_to_next_line();
    }
}

codepoint yaml_parser::parse_unicode_escape(const size_t digits) {
    char32_t code_point = 0;

    for (size_t i = 0; i < digits; ++i) {
        const char ch = current();
        if (!is_xdigit(ch)) {
            throw_parse_error("Invalid unicode escape sequence");
        }
        code_point <<= 4;
        if (ch >= '0' && ch <= '9') {
            code_point |= (ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            code_point |= (ch - 'a' + 10);
        } else if (ch >= 'A' && ch <= 'F') {
            code_point |= (ch - 'A' + 10);
        }
        advance();
    }
    if (!codepoint::is_valid_codepoint(static_cast<uint32_t>(code_point))) {
        throw_parse_error("Invalid unicode codepoint");
    }
    return codepoint{code_point};
}

string yaml_parser::unescape_string(const string& str) const {
    string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[++i]) {
                case '0':
                    result += '\0';
                    break;
                case 'a':
                    result += '\a';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 't':
                case '\t':
                    // case '\t' matches the escaped literal tab character, not the escape sequence \t.
                    result += '\t';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'v':
                    result += '\v';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 'e':
                    result += '\x1B';
                    break;
                case ' ':
                    result += ' ';
                    break;
                case '"':
                    result += '"';
                    break;
                case '/':
                    result += '/';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case 'N':
                    result += "\u0085";
                    break;
                case '_':
                    result += "\u00A0";
                    break;
                case 'L':
                    result += "\u2028";
                    break;
                case 'P':
                    result += "\u2029";
                    break;
                case 'x': {
                    if (i + 2 >= str.size()) {
                        // At this time i points to 'x', not '\\'
                        result += str[i];
                        break;
                    }
                    const string_view hex2 = str.view(i + 1, 2);
                    bool valid = true;
                    for (const char c: hex2) {
                        if (!is_xdigit(c)) {
                            valid = false;
                            break;
                        }
                    }
                    if (valid) {
                        const codepoint cp_val{static_cast<uint32_t>(hexadecimal(hex2).value())};
                        cp_val.append_to(result);
                        i += 2;
                    } else {
                        // No extra rollback of i here, switch has already consumed 'x'.
                        result += str[i];
                    }
                    break;
                }
                case 'u': {
                    if (i + 4 >= str.size()) {
                        result += str[i];
                        break;
                    }
                    const string_view hex4 = str.view(i + 1, 4);
                    bool valid = true;
                    for (const char c: hex4) {
                        if (!is_xdigit(c)) {
                            valid = false;
                            break;
                        }
                    }
                    if (valid) {
                        const auto cp_val = static_cast<uint32_t>(hexadecimal(hex4).value());
                        if (codepoint::is_valid_codepoint(cp_val)) {
                            codepoint{cp_val}.append_to(result);
                            i += 4;
                        } else {
                            result += str[i];
                        }
                    } else {
                        result += str[i];
                    }
                    break;
                }
                case 'U': {
                    if (i + 8 >= str.size()) {
                        result += str[i];
                        break;
                    }
                    const string_view hex8 = str.view(i + 1, 8);
                    bool valid = true;
                    for (const char c: hex8) {
                        if (!is_xdigit(c)) {
                            valid = false;
                            break;
                        }
                    }
                    if (valid) {
                        const auto cp_val = static_cast<uint32_t>(hexadecimal(hex8).value());
                        if (codepoint::is_valid_codepoint(cp_val)) {
                            codepoint{cp_val}.append_to(result);
                            i += 8;
                        } else {
                            result += str[i];
                        }
                    } else {
                        result += str[i];
                    }
                    break;
                }
                default:
                    //Unknown escape sequences (such as \q) preserve the backslash and the character itself
                    result += '\\';
                    result += str[i];
                    break;
            }
        } else {
            result += str[i];
        }
    }

    return result;
}

shared_ptr<yaml_string> yaml_parser::parse_plain_string() {
    string result;

    if (is_indicator(current()) && current() != '-') {
        throw_parse_error("Plain string cannot start with indicator");
    }

    while (!eof()) {
        const char ch = current();

        if (is_newline(ch) || ch == '#') {
            break;
        }
        if (ch == ':' && (peek(1) == ' ' || is_newline(peek(1)) || eof())) {
            break;
        }
        if (in_flow_context_) {
            if (ch == ',' || is_flow_indicator(ch)) {
                break;
            }
        }

        result += ch;
        advance();
    }

    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    if (result.empty()) {
        throw_parse_error("Empty plain string");
    }
    return make_shared<yaml_string>(move(result), yaml_string::Plain);
}

shared_ptr<yaml_string> yaml_parser::parse_single_quoted_string() {
    expect('\'');
    string result;

    while (!eof()) {
        if (current() == '\'') {
            advance();
            if (current() == '\'') {
                result += '\'';
                advance();
            } else {
                break;
            }
        } else if (is_newline(current())) {
            result += ' ';
            skip_to_next_line();
        } else {
            result += current();
            advance();
        }
    }

    return make_shared<yaml_string>(move(result), yaml_string::SingleQuoted);
}

shared_ptr<yaml_string> yaml_parser::parse_double_quoted_string() {
    expect('"');
    string result;

    while (!eof() && current() != '"') {
        if (current() == '\\') {
            advance();
            if (eof()) {
                throw_parse_error("Unexpected end of string");
            }

            switch (current()) {
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
                    result += ' ';
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
                case 'N':
                    result += "\u0085";
                    advance();
                    break;
                case '_':
                    result += "\u00A0";
                    advance();
                    break;
                case 'L':
                    result += "\u2028";
                    advance();
                    break;
                case 'P':
                    result += "\u2029";
                    advance();
                    break;
                case 'x': {
                    advance();
                    parse_unicode_escape(2).append_to(result);
                    break;
                }
                case 'u': {
                    advance();
                    const codepoint cp = parse_unicode_escape(4);
                    cp.append_to(result);
                    break;
                }
                case 'U': {
                    advance();
                    const codepoint cp = parse_unicode_escape(8);
                    cp.append_to(result);
                    break;
                }
                case '\n': {
                    advance();
                    skip_whitespace_inline();
                    break;
                }
                default: {
                    throw_parse_error(string("Invalid escape sequence: \\") + current());
                }
            }
        } else if (is_newline(current())) {
            result += ' ';
            skip_to_next_line();
            skip_whitespace_inline();
        } else {
            result += current();
            advance();
        }
    }

    expect('"');
    return make_shared<yaml_string>(move(result), yaml_string::DoubleQuoted);
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

    if (!is_newline(current()) && !eof()) {
        throw_parse_error("Invalid block scalar header");
    }
    skip_to_next_line();
    const size_t base_indent = current_indent_;
    size_t block_indent = 0;

    while (!eof() && is_newline(current())) {
        skip_to_next_line();
    }

    if (explicit_indent > 0) {
        block_indent = base_indent + explicit_indent;
    } else {
        const size_t saved_pos = pos_;
        const size_t saved_line = line_;
        const size_t saved_column = column_;

        size_t first_content_indent = 0;
        bool found_content = false;

        while (!eof()) {
            size_t line_indent = 0;
            while (!eof() && current() == ' ') {
                line_indent++;
                advance();
            }
            if (!eof() && !is_newline(current())) {
                first_content_indent = line_indent;
                found_content = true;
                break;
            }
            if (!eof()) {
                skip_to_next_line();
            }
        }

        pos_ = saved_pos;
        line_ = saved_line;
        column_ = saved_column;

        if (!found_content) {
            return "";
        }
        if (first_content_indent <= base_indent) {
            throw_parse_error("Invalid block scalar indentation");
        }
        block_indent = first_content_indent;
    }

    vector<string> lines;

    while (!eof()) {
        size_t line_indent = 0;
        if (explicit_indent > 0) {
            while (!eof() && current() == ' ' && line_indent < block_indent) {
                advance();
                line_indent++;
            }
        } else {
            while (!eof() && current() == ' ') {
                line_indent++;
                advance();
            }
        }

        if (is_newline(current())) {
            lines.push_back("");
            skip_to_next_line();
            continue;
        }
        if (line_indent < block_indent) {
            while (line_indent > 0) {
                pos_--;
                column_--;
                line_indent--;
            }
            break;
        }

        string line;
        while (!eof() && !is_newline(current())) {
            line += current();
            advance();
        }
        lines.push_back(line);
        if (!eof()) {
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

shared_ptr<yaml_string> yaml_parser::parse_literal_string() {
    string content = parse_multiline_string(true);
    return make_shared<yaml_string>(move(content), yaml_string::Literal);
}

shared_ptr<yaml_string> yaml_parser::parse_folded_string() {
    string content = parse_multiline_string(false);
    return make_shared<yaml_string>(move(content), yaml_string::Folded);
}


shared_ptr<yaml_null> yaml_parser::parse_null() {
    if (eof()) {
        return make_shared<yaml_null>();
    }

    string value;
    const size_t temp_pos = pos_;
    while (!eof() && (is_alpha_or_digit(current()) || current() == '~')) {
        value += current();
        advance();
    }

    if (value == "null" || value == "Null" || value == "NULL" || value == "~") {
        return make_shared<yaml_null>();
    }
    pos_ = temp_pos;

    throw_parse_error("Invalid null value: " + value);
}

shared_ptr<yaml_boolean> yaml_parser::parse_boolean() {
    string value;
    while (!eof() && is_alpha(current())) {
        value += current();
        advance();
    }
    string lower_value = value;
    lower_value.lowercase();

    if (lower_value == "true" || lower_value == "yes" || lower_value == "on" || lower_value == "y") {
        return make_shared<yaml_boolean>(true);
    }
    if (lower_value == "false" || lower_value == "no" || lower_value == "off" || lower_value == "n") {
        return make_shared<yaml_boolean>(false);
    }

    throw_parse_error("Invalid boolean value: " + value);
}

shared_ptr<yaml_value> yaml_parser::parse_number() {
    string num_str;
    bool is_negative = false;
    bool is_float = false;
    bool is_hex = false;
    bool is_octal = false;
    bool is_binary = false;

    if (current() == '-' || current() == '+') {
        is_negative = (current() == '-');
        num_str += current();
        advance();
    }

    if (current() == '.') {
        const size_t saved_pos = pos_;
        const size_t saved_line = line_;
        const size_t saved_column = column_;

        string special = ".";
        advance();
        while (!eof() && is_alpha(current())) {
            special += current();
            advance();
        }

        string lower_special = special;
        lower_special.lowercase();

        if (lower_special == ".inf" || lower_special == ".infinity") {
            double val = is_negative ? -numeric_traits<double>::infinity() : numeric_traits<double>::infinity();
            return make_shared<yaml_float>(val);
        }
        if (lower_special == ".nan") {
            return make_shared<yaml_float>(numeric_traits<double>::quiet_nan());
        }
        pos_ = saved_pos;
        line_ = saved_line;
        column_ = saved_column;

        num_str += '.';
        advance();
        is_float = true;
    }

    if (current() == '0' && !eof()) {
        num_str += current();
        advance();

        if (current() == 'x' || current() == 'X') {
            is_hex = true;
            num_str += current();
            advance();
        } else if (current() == 'o' || current() == 'O') {
            is_octal = true;
            num_str += current();
            advance();
        } else if (current() == 'b' || current() == 'B') {
            is_binary = true;
            num_str += current();
            advance();
        } else {
            size_t peek_pos = pos_;
            while (peek_pos < len_ && yaml_[peek_pos] == '_') {
                peek_pos++;
            }
            if (peek_pos < len_ && is_digit(yaml_[peek_pos])) {
                throw_parse_error("Leading zeros are not allowed in decimal integers");
            }
        }
    }

    while (!eof()) {
        const char ch = current();

        if ((is_hex && is_xdigit(ch)) || (is_octal && ch >= '0' && ch <= '7') ||
            (is_binary && (ch == '0' || ch == '1')) || (!is_hex && !is_octal && !is_binary && is_digit(ch))) {
            num_str += ch;
            advance();
        } else if (ch == '_') {
            advance();
            continue;
        } else if (ch == '.') {
            if (is_hex || is_octal || is_binary) {
                throw_parse_error("Float not allowed in this number format");
            }
            if (is_float) {
                break;
            }
            num_str += ch;
            advance();
            is_float = true;
        } else if (ch == 'e' || ch == 'E') {
            if (is_hex || is_octal || is_binary) {
                break;
            }
            is_float = true;
            num_str += ch;
            advance();

            if (current() == '+' || current() == '-') {
                num_str += current();
                advance();
            }
        } else {
            break;
        }
    }

    if (num_str.empty() || num_str == "." || num_str == "-" || num_str == "+") {
        throw_parse_error("Invalid number: " + (num_str.empty() ? "(empty)" : num_str));
    }

    try {
        if (is_float) {
            return make_shared<yaml_float>(float64::parse(num_str.view()).value());
        }
        if (is_hex) {
            return make_shared<yaml_integer>(to_int64(num_str.view(), nullptr, 16));
        }
        if (is_octal) {
            return make_shared<yaml_integer>(to_int64(num_str.view(2), nullptr, 8));
        }
        if (is_binary) {
            return make_shared<yaml_integer>(to_int64(num_str.view(2), nullptr, 2));
        }
        return make_shared<yaml_integer>(to_int64(num_str.view()));
    } catch (...) {
        throw_parse_error("Invalid number: " + num_str);
    }
}

shared_ptr<yaml_timestamp> yaml_parser::parse_timestamp(const string_view str) const {
    try {
        return make_shared<yaml_timestamp>(str);
    } catch (...) {
        throw_parse_error("Invalid timestamp: "_s + str);
    }
}

shared_ptr<yaml_value> yaml_parser::parse_scalar() {
    skip_whitespace_inline();
    if (has_alias()) {
        return parse_alias();
    }

    string anchor;
    string tag;

    if (current() == '&') {
        anchor = parse_anchor();
        skip_whitespace_inline();
    }
    if (current() == '!') {
        tag = parse_tag();
        skip_whitespace_inline();
    }

    shared_ptr<yaml_value> value;
    const char ch = current();

    if (ch == '~' || is_alpha(ch)) {
        string word;
        if (ch == '~') {
            word = "~";
        } else {
            while (!eof() && is_alpha(current())) {
                word += current();
                advance();
            }
        }
        string word_lower = word;
        word_lower.lowercase();
        const bool word_is_null = (word == "~" || word_lower == "null");
        const bool word_is_bool =
                (word_lower == "true" || word_lower == "false" || word_lower == "yes" || word_lower == "no" ||
                 word_lower == "on" || word_lower == "off" || word_lower == "y" || word_lower == "n");
        if (ch != '~') {
            size_t word_end_advance = word.size();
            while (word_end_advance > 0) {
                pos_--;
                column_--;
                word_end_advance--;
            }
        }
        const char next_ch = peek(word.size());
        const bool is_word_terminated =
                (next_ch == ' ' || next_ch == '\t' || next_ch == '\n' || next_ch == '\r' || next_ch == '\0' ||
                 next_ch == ':' || next_ch == ',' || next_ch == ']' || next_ch == '}' || next_ch == '#');
        if ((word_is_null || word_is_bool) && is_word_terminated) {
            if (word_is_null) {
                value = parse_null();
            } else {
                value = parse_boolean();
            }
        }
    }
    if (!value) {
        if (ch == '-' || ch == '+' || is_digit(ch) || ch == '.') {
            const size_t saved_pos = pos_;
            string potential_ts;

            while (!eof() && (is_digit(current()) || current() == '-' || current() == ':' || current() == '.' ||
                              current() == 'T' || current() == 'Z' || current() == '+' || current() == ' ')) {
                potential_ts += current();
                advance();
            }

            bool is_timestamp = false;
            if (potential_ts.contains('-') && (potential_ts.contains(':') || potential_ts.contains('T'))) {
                is_timestamp = true;
            } else if (potential_ts.contains('-')) {
                size_t hyphen_count = 0;
                for (const char c: potential_ts) {
                    if (c == '-') {
                        hyphen_count++;
                    }
                }
                if (hyphen_count >= 2) {
                    is_timestamp = true;
                }
            }

            if (is_timestamp) {
                try {
                    value = parse_timestamp(potential_ts.view());
                } catch (...) {
                    pos_ = saved_pos;
                    value = parse_plain_string();
                }
            } else {
                pos_ = saved_pos;
                value = parse_number();
            }
        } else if (ch == '"') {
            value = parse_double_quoted_string();
        } else if (ch == '\'') {
            value = parse_single_quoted_string();
        } else if (ch == '|') {
            value = parse_literal_string();
        } else if (ch == '>') {
            value = parse_folded_string();
        } else {
            value = parse_plain_string();
        }
    }

    value->set_anchor(anchor);
    value->set_tag(tag);

    if (!anchor.empty()) {
        register_anchor(anchor, value);
    }

    return value;
}

shared_ptr<yaml_sequence> yaml_parser::parse_flow_sequence() {
    expect('[');
    const bool prev_flow = in_flow_context_;
    in_flow_context_ = true;
    auto seq = make_shared<yaml_sequence>(yaml_sequence::Flow);
    skip_whitespace_inline();

    if (current() == ']') {
        advance();
        in_flow_context_ = prev_flow;
        return seq;
    }

    while (!eof()) {
        skip_whitespace_inline();
        skip_comment();

        if (current() == '\n') {
            skip_to_next_line();
            skip_whitespace_inline();
        }

        if (current() == ']') {
            advance();
            break;
        }

        auto value = parse_inline_value();
        seq->add_element(move(value));
        skip_whitespace_inline();
        if (current() == '\n') {
            skip_to_next_line();
            skip_whitespace_inline();
        }

        if (current() == ',') {
            advance();
            skip_whitespace_inline();
            if (current() == '\n') {
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

    in_flow_context_ = prev_flow;
    return seq;
}

shared_ptr<yaml_mapping> yaml_parser::parse_flow_mapping() {
    expect('{');
    const bool prev_flow = in_flow_context_;
    in_flow_context_ = true;
    auto map = make_shared<yaml_mapping>(yaml_mapping::Flow);
    skip_whitespace_inline();

    if (current() == '}') {
        advance();
        in_flow_context_ = prev_flow;
        return map;
    }

    while (!eof()) {
        skip_whitespace_inline();
        skip_comment();

        if (current() == '\n') {
            skip_to_next_line();
            skip_whitespace_inline();
        }

        if (current() == '}') {
            advance();
            break;
        }

        string key = parse_key();

        skip_whitespace_inline();
        expect(':');
        skip_whitespace_inline();

        auto value = parse_inline_value();
        map->add_member(key, move(value));

        skip_whitespace_inline();
        if (current() == '\n') {
            skip_to_next_line();
            skip_whitespace_inline();
        }

        if (current() == ',') {
            advance();
            skip_whitespace_inline();
            if (current() == '\n') {
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

    in_flow_context_ = prev_flow;
    return map;
}

shared_ptr<yaml_sequence> yaml_parser::parse_block_sequence() {
    auto seq = make_shared<yaml_sequence>(yaml_sequence::Block);
    const size_t seq_indent = current_indent_;
    bool at_line_start = true;

    while (!eof()) {
        const size_t line_indent = skip_indent();
        if (!at_line_start && line_indent < seq_indent) {
            break;
        }
        if (line_indent > seq_indent && current() != '-') {
            throw_parse_error("Invalid indentation in block sequence");
        }
        at_line_start = false;

        if (is_newline(current()) || current() == '#') {
            skip_to_next_line();
            continue;
        }

        if (current() != '-') {
            break;
        }
        advance();

        if (!is_whitespace(current()) && !is_newline(current()) && !eof()) {
            throw_parse_error("Expected whitespace after '-' in block sequence");
        }

        skip_whitespace_inline();
        shared_ptr<yaml_value> value;

        if (is_newline(current()) || current() == '#') {
            skip_to_next_line();

            const size_t value_indent = skip_indent();
            if (value_indent <= seq_indent) {
                value = make_shared<yaml_null>();
            } else {
                const size_t saved_indent = current_indent_;
                current_indent_ = value_indent;
                value = parse_block_value();
                current_indent_ = saved_indent;
            }
        } else {
            const size_t saved_pos = pos_;
            bool is_mapping_like = false;

            while (!eof() && !is_newline(current())) {
                if (current() == ':' && (peek(1) == ' ' || is_newline(peek(1)) || eof())) {
                    is_mapping_like = true;
                    break;
                }
                if (is_flow_indicator(current())) {
                    break;
                }
                advance();
            }

            pos_ = saved_pos;

            if (is_mapping_like) {
                const size_t saved_indent = current_indent_;
                current_indent_ = seq_indent + 2;
                value = parse_block_mapping(false);
                current_indent_ = saved_indent;
            } else {
                value = parse_inline_value();
                skip_to_next_line();
            }
        }
        seq->add_element(move(value));
    }
    return seq;
}

shared_ptr<yaml_mapping> yaml_parser::parse_block_mapping(bool parent_skipped_indent) {
    auto map = make_shared<yaml_mapping>(yaml_mapping::Block);
    const size_t map_indent = current_indent_;
    size_t first_key_indent = 0;
    bool is_first_key = true;
    size_t subsequent_key_indent = 0;
    bool first_key_on_same_line = false;

    while (!eof()) {
        while (!eof() && (is_newline(current()) || current() == '#')) {
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
            if (line_indent == 0 && map_indent > 0) {
                if (parent_skipped_indent) {
                    first_key_indent = map_indent;
                    is_first_key = false;
                } else {
                    first_key_on_same_line = true;
                    first_key_indent = 0;
                    is_first_key = false;
                }
            } else if (line_indent < map_indent) {
                break;
            } else {
                first_key_indent = line_indent;
                is_first_key = false;
            }
        } else {
            if (first_key_on_same_line) {
                if (line_indent < map_indent) {
                    break;
                }

                if (line_indent == map_indent) {
                    skip_indent();
                    if (current() == '-' && (peek(1) == ' ' || is_newline(peek(1)))) {
                        break;
                    }
                    for (size_t i = 0; i < line_indent; ++i) {
                        pos_--;
                        column_--;
                    }
                }

                if (subsequent_key_indent == 0) {
                    if (line_indent < map_indent) {
                        throw_parse_error("Subsequent keys must be indented more than the parent context");
                    }
                    subsequent_key_indent = line_indent;
                } else {
                    if (line_indent != subsequent_key_indent) {
                        throw_parse_error("All keys in a block mapping must have the same indentation");
                    }
                }
            } else {
                if (line_indent < map_indent) {
                    break;
                }
                if (line_indent != first_key_indent) {
                    throw_parse_error("All keys in a block mapping must have the same indentation");
                }
            }
        }

        skip_indent();

        if (current() == '.' && peek(1) == '.' && peek(2) == '.') {
            break;
        }
        if (current() == '-' && peek(1) == '-' && peek(2) == '-') {
            break;
        }
        if (current() == '-' && (peek(1) == ' ' || is_newline(peek(1)))) {
            break;
        }

        if (current() == '&') {
            string key_anchor = parse_anchor();
        }
        if (current() == '!') {
            skip_tag();
        }

        if (current() == '?') {
            const size_t complex_key_indent = current_indent_;
            advance();
            skip_whitespace_inline();

            shared_ptr<yaml_value> complex_key;
            string key_anchor;
            string key_tag;

            if (current() == '&') {
                key_anchor = parse_anchor();
                skip_whitespace_inline();
            }
            if (current() == '!') {
                key_tag = parse_tag();
                skip_whitespace_inline();
            }

            if (is_newline(current()) || current() == '#') {
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
                if (!eof() && is_newline(current())) {
                    skip_to_next_line();
                }
            }

            if (!key_anchor.empty()) {
                complex_key->set_anchor(key_anchor);
                register_anchor(key_anchor, complex_key);
            }
            if (!key_tag.empty()) {
                complex_key->set_tag(key_tag);
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

            string value_anchor;
            string value_tag;
            if (current() == '&') {
                value_anchor = parse_anchor();
                skip_whitespace_inline();
            }
            if (current() == '!') {
                value_tag = parse_tag();
                skip_whitespace_inline();
            }

            shared_ptr<yaml_value> value;

            if (is_newline(current()) || current() == '#' || eof()) {
                if (current() == '#') {
                    skip_comment();
                }
                skip_to_next_line();
                if (eof()) {
                    value = make_shared<yaml_null>();
                } else {
                    const size_t value_indent = skip_indent();
                    if (value_indent <= colon_indent) {
                        value = make_shared<yaml_null>();
                    } else {
                        const size_t saved_indent = current_indent_;
                        current_indent_ = value_indent;
                        value = parse_block_value();
                        current_indent_ = saved_indent;
                    }
                }
            } else {
                value = parse_inline_value();
                if (!eof() && !is_newline(current())) {
                    skip_whitespace_inline();
                }
                if (!eof() && current() == '#') {
                    skip_comment();
                }
                if (!eof() && is_newline(current())) {
                    skip_to_next_line();
                }
            }

            if (!value_anchor.empty()) {
                value->set_anchor(value_anchor);
                register_anchor(value_anchor, value);
            }
            if (!value_tag.empty()) {
                value->set_tag(value_tag);
            }

            map->add_member(complex_key->to_string(), move(value));
            continue;
        }

        string key = parse_key();

        skip_whitespace_inline();
        if (current() != ':') {
            throw_parse_error("Expected ':' after key in block mapping");
        }

        advance();
        skip_whitespace_inline();

        string value_anchor;
        string value_tag;
        if (current() == '&') {
            value_anchor = parse_anchor();
            skip_whitespace_inline();
        }
        if (current() == '!') {
            value_tag = parse_tag();
            skip_whitespace_inline();
        }

        shared_ptr<yaml_value> value;

        if (is_newline(current()) || current() == '#' || eof()) {
            if (current() == '#') {
                skip_comment();
            }
            skip_to_next_line();

            if (eof()) {
                value = make_shared<yaml_null>();
            } else {
                const size_t value_indent = peek_indent();

                size_t effective_key_indent = 0;
                if (first_key_on_same_line) {
                    effective_key_indent = (subsequent_key_indent > 0) ? subsequent_key_indent : map_indent;
                } else {
                    effective_key_indent = first_key_indent;
                }

                if (value_indent <= effective_key_indent) {
                    value = make_shared<yaml_null>();
                } else {
                    const size_t saved_indent = current_indent_;
                    current_indent_ = value_indent;
                    value = parse_block_value();
                    current_indent_ = saved_indent;
                }
            }
        } else {
            value = parse_inline_value();
            if (!eof() && !is_newline(current())) {
                skip_whitespace_inline();
            }
            if (!eof() && current() == '#') {
                skip_comment();
            }
            if (!eof() && is_newline(current())) {
                skip_to_next_line();
            }
        }

        value->set_anchor(value_anchor);
        value->set_tag(value_tag);

        if (!value_anchor.empty()) {
            register_anchor(value_anchor, value);
        }

        if (key == "<<") {
            if (value && value->is_mapping()) {
                map->merge_from(value->as_mapping());
            } else if (value && value->is_sequence()) {
                const auto* seq = value->as_sequence();
                for (size_t i = 0; i < seq->size(); ++i) {
                    const auto* elem = seq->get_element(i);
                    if (elem != nullptr && elem->is_mapping()) {
                        map->merge_from(elem->as_mapping());
                    }
                }
            }
        } else {
            map->add_member(key, move(value));
        }
    }
    return map;
}

string yaml_parser::parse_key() {
    skip_whitespace_inline();

    if (current() == '"') {
        return parse_quoted_key();
    }
    if (current() == '\'') {
        const auto str = parse_single_quoted_string();
        return str->get_value();
    }
    return parse_plain_key();
}

string yaml_parser::parse_plain_key() {
    string key;
    while (!eof()) {
        const char ch = current();
        if (ch == ':' && (peek(1) == ' ' || is_newline(peek(1)) || peek(1) == '\0' || is_flow_indicator(peek(1)))) {
            break;
        }
        if (is_newline(ch) || is_flow_indicator(ch)) {
            break;
        }

        if (ch == '#') {
            if (!key.empty() && key.back() == ' ') {
                key.pop_back();
                break;
            }
        }

        key += ch;
        advance();
    }
    while (!key.empty() && key.back() == ' ') {
        key.pop_back();
    }

    if (key.empty()) {
        throw_parse_error("Empty key");
    }
    return key;
}

string yaml_parser::parse_quoted_key() {
    const auto str = parse_double_quoted_string();
    return str->get_value();
}

shared_ptr<yaml_value> yaml_parser::parse_inline_value() {
    skip_whitespace_inline();

    if (has_alias()) {
        return parse_alias();
    }

    string anchor;
    string tag;

    if (current() == '&') {
        anchor = parse_anchor();
        skip_whitespace_inline();
    }
    if (current() == '!') {
        tag = parse_tag();
        skip_whitespace_inline();
    }

    shared_ptr<yaml_value> value;
    const char ch = current();

    if (ch == '[') {
        value = parse_flow_sequence();
    } else if (ch == '{') {
        value = parse_flow_mapping();
    } else {
        value = parse_scalar();
    }

    value->set_anchor(anchor);
    value->set_tag(tag);

    if (!anchor.empty()) {
        register_anchor(anchor, value);
    }

    return value;
}

shared_ptr<yaml_value> yaml_parser::parse_single_document() {
    skip_blank_lines();
    indent_stack_.clear();
    current_indent_ = 0;

    if (eof()) {
        return make_shared<yaml_null>();
    }
    if (is_document_end()) {
        return make_shared<yaml_null>();
    }

    const size_t doc_indent = skip_indent();
    current_indent_ = doc_indent;

    return parse_value();
}

shared_ptr<yaml_value> yaml_parser::parse_block_value() {
    skip_indent();
    if (current() == '?' && (peek(1) == ' ' || is_newline(peek(1)))) {
        return parse_block_mapping(true);
    }
    if (current() == '-' && (peek(1) == ' ' || is_newline(peek(1)))) {
        return parse_block_sequence();
    }
    if (current() == '{') {
        return parse_flow_mapping();
    }
    if (current() == '[') {
        return parse_flow_sequence();
    }
    const size_t saved_pos = pos_;
    const size_t saved_line = line_;
    const size_t saved_column = column_;
    bool looks_like_mapping = false;

    while (!eof() && !is_newline(current())) {
        if (current() == ':' && (peek(1) == ' ' || is_newline(peek(1)) || eof())) {
            looks_like_mapping = true;
            break;
        }
        advance();
    }

    pos_ = saved_pos;
    line_ = saved_line;
    column_ = saved_column;

    if (looks_like_mapping) {
        return parse_block_mapping(true);
    }
    return parse_scalar();
}

shared_ptr<yaml_value> yaml_parser::parse_value() {
    skip_whitespace_inline();
    if (has_alias()) {
        return parse_alias();
    }

    string anchor;
    string tag;

    if (current() == '&') {
        anchor = parse_anchor();
        skip_whitespace_inline();
    }
    if (current() == '!') {
        tag = parse_tag();
        skip_whitespace_inline();
    }

    if (is_newline(current())) {
        skip_to_next_line();
        skip_blank_lines();
    }

    shared_ptr<yaml_value> value;
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
        const size_t saved_pos = pos_;
        const size_t saved_line = line_;
        const size_t saved_column = column_;
        skip_whitespace_inline();

        if (current() == '-' && (peek(1) == ' ' || is_newline(peek(1)))) {
            pos_ = saved_pos;
            line_ = saved_line;
            column_ = saved_column;
            value = parse_block_sequence();
        } else {
            bool found_colon = false;
            while (!eof() && !is_newline(current())) {
                if (current() == ':' && (peek(1) == ' ' || is_newline(peek(1)) || eof())) {
                    found_colon = true;
                    break;
                }
                advance();
            }

            pos_ = saved_pos;
            line_ = saved_line;
            column_ = saved_column;

            if (found_colon) {
                value = parse_block_mapping(false);
            } else {
                value = parse_scalar();
            }
        }
    }

    value->set_anchor(anchor);
    value->set_tag(tag);

    if (!anchor.empty()) {
        register_anchor(anchor, value);
    }

    return value;
}

shared_ptr<yaml_value> yaml_parser::parse() {
    if (len_ >= 3 && static_cast<byte_t>(yaml_[0]) == 0xEF && static_cast<byte_t>(yaml_[1]) == 0xBB &&
        static_cast<byte_t>(yaml_[2]) == 0xBF) {
        pos_ = 3;
    }

    skip_blank_lines();
    parse_document_start();
    skip_blank_lines();
    if (eof()) {
        return make_shared<yaml_null>();
    }
    if (is_document_end()) {
        return make_shared<yaml_null>();
    }

    current_indent_ = skip_indent();
    shared_ptr<yaml_value> result = parse_value();

    while (!eof()) {
        if (is_whitespace(current()) || is_newline(current())) {
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
        if (is_whitespace(current()) || is_newline(current())) {
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

    return result;
}

optional<shared_ptr<yaml_value>> yaml_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

vector<shared_ptr<yaml_value>> yaml_parser::parse_documents() {
    vector<shared_ptr<yaml_value>> documents;

    while (!eof()) {
        anchors_.clear();
        skip_blank_lines();

        while (!eof() && current() == '%') {
            parse_directive();
            skip_blank_lines();
        }
        if (eof()) {
            break;
        }

        if (current() == '-' && peek(1) == '-' && peek(2) == '-') {
            pos_ += 3;
            column_ += 3;
            skip_blank_lines();
        }
        if (eof()) {
            break;
        }

        documents.push_back(parse_single_document());
        skip_blank_lines();

        if (!eof() && current() == '.' && peek(1) == '.' && peek(2) == '.') {
            pos_ += 3;
            column_ += 3;
            skip_blank_lines();
        }
    }
    return documents;
}

optional<vector<shared_ptr<yaml_value>>> yaml_parser::try_parse_documents() {
    try {
        return parse_documents();
    } catch (...) {
        return {};
    }
}

void yaml_parser::throw_parse_error(const string& message) const {
    string error_msg = message;
    error_msg += " at line " + to_string(line_);
    error_msg += ", column " + to_string(column_);

    if (pos_ < len_) {
        error_msg += "\nNear: '";
        const size_t context_start = pos_ > 20 ? pos_ - 20 : 0;
        const size_t context_end = pos_ + 20 < len_ ? pos_ + 20 : len_;

        for (size_t i = context_start; i < context_end; ++i) {
            const char ch = yaml_[i];
            if (ch == '\n') {
                error_msg += "\\n";
            } else if (ch == '\r') {
                error_msg += "\\r";
            } else if (ch == '\t') {
                error_msg += "\\t";
            } else {
                error_msg += ch;
            }
        }
        error_msg += "'";
    }
    NEFORCE_THROW_EXCEPTION(yaml_exception(error_msg.data()));
}

NEFORCE_END_NAMESPACE__
