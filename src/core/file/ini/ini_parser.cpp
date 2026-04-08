#include <NeForce/core/file/ini/ini_parser.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__

void ini_parser::skip_whitespace() noexcept {
    while (pos_ < len_ && is_space(current())) {
        advance();
    }
}

void ini_parser::skip_line() noexcept {
    while (!eof() && current() != '\n') {
        advance();
    }
    if (current() == '\n') {
        advance();
    }
}

char ini_parser::current() const noexcept {
    if (pos_ < len_) {
        return text_[pos_];
    }
    return '\0';
}

char ini_parser::peek(const size_t offset) const noexcept {
    if (pos_ + offset < len_) {
        return text_[pos_ + offset];
    }
    return '\0';
}

bool ini_parser::eof() const noexcept { return pos_ >= len_; }

void ini_parser::advance() noexcept {
    if (pos_ < len_) {
        if (text_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

bool ini_parser::is_comment_line(const string& line) const {
    string trimmed = line;
    trimmed.trim();
    return trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#';
}

bool ini_parser::is_section_line(const string& line, string& section_name) const {
    string trimmed = line;
    trimmed.trim();
    if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
        section_name = trimmed.substr(1, trimmed.size() - 2).trim();
        return true;
    }
    return false;
}

bool ini_parser::parse_key_value(const string& line, string& key, string& value) const {
    const size_t pos = line.find('=');
    if (pos == string::npos) {
        return false;
    }

    key = line.substr(0, pos).trim();
    value = line.substr(pos + 1).trim();

    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
    }

    return !key.empty();
}

void ini_parser::parse_line(const string& line) {
    if (is_comment_line(line)) {
        return;
    }

    string section_name;
    if (is_section_line(line, section_name)) {
        auto new_section = make_unique<ini_section>(section_name);
        current_section_ = new_section.get();
        root_->add_section(section_name, move(new_section));
        return;
    }

    string key, value;
    if (parse_key_value(line, key, value)) {
        if (current_section_ == nullptr) {
            current_section_ = root_->get_global_section();
        }
        current_section_->set_property(key, value);
    }
}

unique_ptr<ini_document> ini_parser::parse() {
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

        parse_line(line);
    }

    return move(root_);
}

optional<unique_ptr<ini_document>> ini_parser::try_parse() {
    try {
        return parse();
    } catch (...) {
        return {};
    }
}

NEFORCE_END_NAMESPACE__
