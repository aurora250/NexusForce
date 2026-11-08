#include "NeForce/core/file/ini/ini_parser.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    NEFORCE_NODISCARD string_view trimmed(const char* begin, const char* end) noexcept {
        return string_view(begin, static_cast<size_t>(end - begin)).trim();
    }
} // namespace


ini_parser::ini_parser(const string_view text) noexcept :
text_(text),
cursor_(text.data()),
end_(text.data() == nullptr ? nullptr : text.data() + text.size()) {
    current_ = document_.root_.storage_.ext_.payload_.section_;
}

ini_parser::ini_parser(string text) :
owned_(move(text)),
text_(owned_.data(), owned_.size()),
cursor_(owned_.data()),
end_(owned_.data() + owned_.size()),
owning_(true) {
    current_ = document_.root_.storage_.ext_.payload_.section_;
}

ini_parser::ini_parser(const char* text) :
ini_parser(string(text)) {}

void ini_parser::parse_line(const char* begin, const char* end) {
    const string_view line = trimmed(begin, end);
    if (line.empty() || line.front() == ';' || line.front() == '#') {
        return;
    }
    if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
        const string_view name = trimmed(line.data() + 1, line.data() + line.size() - 1);
        current_ = document_.new_section(name, owning_).data();
        return;
    }
    const char* equal = line.data();
    const char* limit = line.data() + line.size();
    while (equal < limit && *equal != '=') {
        ++equal;
    }
    if (equal == limit) {
        return;
    }
    const string_view key = trimmed(line.data(), equal);
    if (key.empty()) {
        return;
    }
    string_view value = trimmed(equal + 1, limit);
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
        value = string_view(value.data() + 1, value.size() - 2);
    }
    if (current_ == nullptr) {
        current_ = document_.root_.storage_.ext_.payload_.section_;
    }
    document_.append_property(current_, key, value, owning_);
}

ini_document ini_parser::parse() {
    if (cursor_ == nullptr) {
        return move(document_);
    }
    while (cursor_ < end_) {
        const char* line_begin = cursor_;
        while (cursor_ < end_ && *cursor_ != '\n') {
            ++cursor_;
        }
        const char* line_end = cursor_;
        if (cursor_ < end_) {
            ++cursor_;
        }
        if (line_end > line_begin && line_end[-1] == '\r') {
            --line_end;
        }
        parse_line(line_begin, line_end);
    }
    return move(document_);
}

optional<ini_document> ini_parser::try_parse() noexcept {
    try {
        return optional<ini_document>(parse());
    } catch (...) {
        return optional<ini_document>();
    }
}

NEFORCE_END_NAMESPACE__
