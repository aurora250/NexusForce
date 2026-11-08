#include "NeForce/core/file/env/env_parser.hpp"
#include "NeForce/core/utility/packages.hpp"
NEFORCE_BEGIN_NAMESPACE__

namespace {
    NEFORCE_NODISCARD string_view trimmed(const char* begin, const char* end) noexcept {
        return string_view(begin, static_cast<size_t>(end - begin)).trim();
    }

    NEFORCE_NODISCARD bool starts_with_export(const string_view text) noexcept {
        return text.size() >= 7 && text[0] == 'e' && text[1] == 'x' && text[2] == 'p' && text[3] == 'o' &&
               text[4] == 'r' && text[5] == 't' && text[6] == ' ';
    }

    NEFORCE_NODISCARD char escaped_char(const char next) noexcept {
        switch (next) {
            case 'n':
                return '\n';
            case 'r':
                return '\r';
            case 't':
                return '\t';
            case '\\':
                return '\\';
            case '"':
                return '"';
            case '$':
                return '$';
            case '`':
                return '`';
            default:
                return '\0';
        }
    }
} // namespace


env_parser::env_parser(const string_view text) noexcept :
text_(text),
cursor_(text.data()),
end_(text.data() == nullptr ? nullptr : text.data() + text.size()) {}

env_parser::env_parser(string text) :
owned_(move(text)),
text_(owned_.data(), owned_.size()),
cursor_(owned_.data()),
end_(owned_.data() + owned_.size()),
owning_(true) {}

env_parser::env_parser(const char* text) :
env_parser(string(text)) {}

string_view env_parser::parse_quoted(const char* begin, const char* end, env_quote& quote, bool& decoded) {
    const char delimiter = *begin;
    quote = delimiter == '\'' ? env_quote::single : env_quote::double_quote;
    const char* cursor = begin + 1;
    const char* content_begin = cursor;
    const char* content_end = cursor;
    bool escaped = false;
    if (delimiter == '\'') {
        while (cursor < end) {
            if (*cursor == '\'') {
                if (cursor + 3 < end && cursor[1] == '\\' && cursor[2] == '\'' && cursor[3] == '\'') {
                    escaped = true;
                    cursor += 4;
                    continue;
                }
                content_end = cursor;
                break;
            }
            ++cursor;
        }
        if (cursor >= end) {
            content_end = end;
        }
        if (!escaped) {
            decoded = false;
            return string_view(content_begin, static_cast<size_t>(content_end - content_begin));
        }
        decoded = true;
        scratch_.clear();
        cursor = content_begin;
        while (cursor < content_end) {
            if (*cursor == '\'' && cursor + 3 < content_end && cursor[1] == '\\' && cursor[2] == '\'' &&
                cursor[3] == '\'') {
                scratch_ += '\'';
                cursor += 4;
            } else {
                scratch_ += *cursor;
                ++cursor;
            }
        }
        return string_view(scratch_.data(), scratch_.size());
    }
    while (cursor < end) {
        if (*cursor == '\\' && cursor + 1 < end) {
            escaped = true;
            cursor += 2;
            continue;
        }
        if (*cursor == '"') {
            content_end = cursor;
            break;
        }
        ++cursor;
    }
    if (cursor >= end) {
        content_end = end;
    }
    if (!escaped) {
        decoded = false;
        return string_view(content_begin, static_cast<size_t>(content_end - content_begin));
    }
    decoded = true;
    scratch_.clear();
    cursor = content_begin;
    while (cursor < content_end) {
        if (*cursor == '\\' && cursor + 1 < content_end) {
            const char next = cursor[1];
            const char ch = escaped_char(next);
            if (static_cast<int>(decoded) != '\0') {
                scratch_ += ch;
            } else {
                scratch_ += '\\';
                scratch_ += next;
            }
            cursor += 2;
        } else {
            scratch_ += *cursor;
            ++cursor;
        }
    }
    return string_view(scratch_.data(), scratch_.size());
}

void env_parser::parse_line(const char* begin, const char* end) {
    const string_view line = trimmed(begin, end);
    if (line.empty()) {
        return;
    }
    if (line.front() == '#') {
        if (line.size() > 1) {
            document_.add_comment_impl(trimmed(line.data() + 1, line.data() + line.size()), owning_);
        }
        return;
    }

    string_view content = line;
    bool exported = false;
    if (starts_with_export(content)) {
        exported = true;
        content = trimmed(content.data() + 7, content.data() + content.size());
    }

    const char* equal = content.data();
    const char* limit = content.data() + content.size();
    while (equal < limit && *equal != '=') {
        ++equal;
    }
    if (equal == limit) {
        return;
    }
    const string_view name = trimmed(content.data(), equal);
    if (name.empty()) {
        return;
    }

    const char* value_begin = equal + 1;
    while (value_begin < limit && (*value_begin == ' ' || *value_begin == '\t')) {
        ++value_begin;
    }

    env_quote quote = env_quote::none;
    string_view value;
    bool decoded = false;
    if (value_begin >= limit) {
        value = string_view();
    } else if (*value_begin == '\'' || *value_begin == '"') {
        value = parse_quoted(value_begin, limit, quote, decoded);
    } else {
        const char* value_end = value_begin;
        while (value_end < limit && *value_end != ' ' && *value_end != '\t' && *value_end != '#') {
            ++value_end;
        }
        value = string_view(value_begin, static_cast<size_t>(value_end - value_begin));
    }

    if (decoded) {
        value = document_.store_string(value);
    }
    document_.set_variable_impl(name, value, quote, exported, owning_);
}

env_document env_parser::parse() {
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

optional<env_document> env_parser::try_parse() noexcept {
    try {
        return optional<env_document>(parse());
    } catch (...) {
        return optional<env_document>();
    }
}

NEFORCE_END_NAMESPACE__
