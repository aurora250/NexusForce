#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/file/env/env_parser.hpp>
MSTL_BEGIN_NAMESPACE__

void env_parser::skip_whitespace() noexcept {
    while (pos_ < len_ && (env_[pos_] == ' ' || env_[pos_] == '	')) {
        advance();
    }
}

void env_parser::skip_line() noexcept {
    while (!eof() && current() != '\n') {
        advance();
    }
    if (current() == '\n') {
        advance();
    }
}

char env_parser::current() const noexcept {
    if (pos_ < len_) return env_[pos_];
    return '\0';
}

char env_parser::peek(const size_t offset) const noexcept {
    if (pos_ + offset < len_) return env_[pos_ + offset];
    return '\0';
}

bool env_parser::eof() const noexcept {
    return pos_ >= len_;
}

void env_parser::advance() noexcept {
    if (pos_ < len_) {
        if (env_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

bool env_parser::is_comment_line(const string& line) const noexcept {
    string trimmed = line;
    trimmed.trim();
    return trimmed.starts_with('#');
}

bool env_parser::is_blank_line(const string& line) const noexcept {
    string trimmed = line;
    trimmed.trim();
    return trimmed.empty();
}

string env_parser::parse_unquoted_value(const string& line, size_t& pos) const {
    string result;
    while (pos < line.size()) {
        char c = line[pos];
        if (c == ' ' || c == '	' || c == '#') {
            break;
        }
        result += c;
        pos++;
    }
    return result;
}

string env_parser::parse_single_quoted_value(const string& line, size_t& pos) const {
    pos++;
    string result;

    while (pos < line.size()) {
        char c = line[pos];
        if (c == '\'') {
            if (pos + 3 < line.size() &&
                line[pos + 1] == '\\' &&
                line[pos + 2] == '\'' &&
                line[pos + 3] == '\'') {
                result += '\'';
                pos += 4;
            } else {
                pos++;
                break;
            }
        } else {
            result += c;
            pos++;
        }
    }

    return result;
}

string env_parser::parse_double_quoted_value(const string& line, size_t& pos) const {
    pos++;
    string result;

    while (pos < line.size()) {
        const char c = line[pos];
        if (c == '\\' && pos + 1 < line.size()) {
            pos++;
            const char next = line[pos];
            switch (next) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case '$': result += '$'; break;
                case '`': result += '`'; break;
                default:
                    result += '\\';
                    result += next;
                    break;
            }
            pos++;
        } else if (c == '"') {
            pos++;
            break;
        } else {
            result += c;
            pos++;
        }
    }

    return result;
}

bool env_parser::parse_variable_line(const string& line, string& name,
    unique_ptr<env_variable> &variable)const {
    size_t pos = 0;
    string trimmed = line;
    trimmed.trim();

    bool is_exported = false;
    if (trimmed.starts_with("export ")) {
        is_exported = true;
        trimmed = trimmed.substr(7);
        trimmed.trim();
    }
    const size_t eq_pos = trimmed.find('=');
    if (eq_pos == string::npos) {
        return false;
    }
    name = trimmed.substr(0, eq_pos);
    name.trim();
    if (name.empty()) {
        return false;
    }

    pos = eq_pos + 1;
    while (pos < trimmed.size() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
        pos++;
    }
    if (pos >= trimmed.size()) {
        variable = make_unique<env_variable>("", env_variable::None, is_exported);
        return true;
    }

    string value;
    env_variable::quote_type qt = env_variable::None;

    const char first_char = trimmed[pos];
    if (first_char == '\'') {
        qt = env_variable::Single;
        value = parse_single_quoted_value(trimmed, pos);
    } else if (first_char == '"') {
        qt = env_variable::Double;
        value = parse_double_quoted_value(trimmed, pos);
    } else {
        qt = env_variable::None;
        value = parse_unquoted_value(trimmed, pos);
    }

    variable = make_unique<env_variable>(value, qt, is_exported);
    return true;
}

void env_parser::parse_line(const string& line) const {
    if (is_blank_line(line)) {
        return;
    }
    if (is_comment_line(line)) {
        string comment = line;
        comment.trim();
        if (comment.size() > 1) {
            root_->add_comment(comment.substr(1).trim());
        }
        return;
    }

    string name;
    unique_ptr<env_variable> variable;
    if (parse_variable_line(line, name, variable)) {
        root_->add_variable(name, _MSTL move(variable));
    }
}

unique_ptr<env_document> env_parser::parse() {
    string line;

    while (!eof()) {
        line.clear();
        while (!eof() && current() != '\n') {
            line += current();
            advance();
        }
        if (current() == '\n') {
            advance();
        }

        try {
            parse_line(line);
        } catch (...) {
            throw;
        }
    }

    return _MSTL move(root_);
}

optional<unique_ptr<env_document>> env_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

MSTL_END_NAMESPACE__
