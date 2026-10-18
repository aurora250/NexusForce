#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/tui/dom/paragraph.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    int display_width_of(string_view text) {
        int width = 0;
        const auto* data = reinterpret_cast<const byte_t*>(text.data());
        for (utf8_iterator it(data, text.size()); it != utf8_iterator(); ++it) {
            width += max(1, it->display_width());
        }
        return width;
    }

    int codepoint_width(const byte_t lead) {
        if ((lead & 0x80) == 0) {
            return 1;
        }
        if ((lead & 0xE0) == 0xC0) {
            return 2;
        }
        if ((lead & 0xF0) == 0xE0) {
            return 3;
        }
        if ((lead & 0xF8) == 0xF0) {
            return 4;
        }
        return 1;
    }

    void push_overflowing_word(const string_view word, const int max_width, vector<string>& lines) {
        string chunk;
        int chunk_w = 0;
        const auto* data = reinterpret_cast<const byte_t*>(word.data());
        for (utf8_iterator it(data, word.size()); it != utf8_iterator(); ++it) {
            const int cpw = max(1, it->display_width());
            if (chunk_w + cpw > max_width && !chunk.empty()) {
                lines.push_back(chunk);
                chunk.clear();
                chunk_w = 0;
            }
            it->append_to(chunk);
            chunk_w += cpw;
        }
        if (!chunk.empty()) {
            lines.push_back(chunk);
        }
    }

    vector<string> wrap_text(const string& content, const int max_width) {
        vector<string> lines;
        if (content.empty()) {
            lines.push_back("");
            return lines;
        }
        const auto& raw = content.view();
        const auto* data = reinterpret_cast<const byte_t*>(raw.data());
        const size_t len = raw.size();

        if (max_width <= 0) {
            size_t start = 0;
            for (size_t i = 0; i < len; ++i) {
                if (data[i] == '\n') {
                    lines.push_back(raw.substr(start, i - start));
                    start = i + 1;
                }
            }
            lines.push_back(raw.substr(start));
            return lines;
        }

        string current;
        int current_w = 0;
        size_t pos = 0;
        while (pos < len) {
            while (pos < len && current.empty() && data[pos] == ' ') {
                ++pos;
            }

            size_t word_end = pos;
            while (word_end < len && data[word_end] != ' ' && data[word_end] != '\n') {
                word_end += codepoint_width(data[word_end]);
            }
            word_end = min(word_end, len);

            const bool is_newline = (word_end < len && data[word_end] == '\n');
            const string_view word = raw.view(pos, word_end - pos);
            const int word_w = display_width_of(word);

            if (!current.empty() && current_w + 1 + word_w > max_width) {
                lines.push_back(current);
                current.clear();
                current_w = 0;
            }

            if (word_w > max_width && current.empty()) {
                push_overflowing_word(word, max_width, lines);
            } else if (!word.empty()) {
                if (!current.empty()) {
                    current += ' ';
                    current_w += 1;
                }
                current.append(word.data(), word.size());
                current_w += word_w;
            }

            if (is_newline) {
                lines.push_back(current);
                current.clear();
                current_w = 0;
                pos = word_end + 1;
            } else {
                pos = word_end;
            }
        }
        if (!current.empty()) {
            lines.push_back(current);
        }
        if (lines.empty()) {
            lines.push_back("");
        }
        return lines;
    }

    string pad_line(const string& line, const int max_width, const enum style::align align) {
        const int line_w = display_width_of(line.view());
        if (line_w >= max_width) {
            return line;
        }

        const int padding = max_width - line_w;
        switch (align) {
            case style::align::end:
                return string(static_cast<size_t>(padding), ' ') + line;
            case style::align::center:
                return string(static_cast<size_t>(padding / 2), ' ') + line +
                       string(static_cast<size_t>(padding - padding / 2), ' ');
            default:
                return line + string(static_cast<size_t>(padding), ' ');
        }
    }

    element paragraph_impl(string content, const int max_width, const enum style::align align) {
        auto lines = wrap_text(content, max_width);
        elements children;
        for (auto& line: lines) {
            if (max_width > 0) {
                line = pad_line(line, max_width, align);
            }
            children.push_back(element::text(move(line)));
        }
        return element::vbox(move(children));
    }
} // namespace


element paragraph(string content, const int max_width) {
    return paragraph_impl(move(content), max_width, style::align::start);
}

element paragraph_align_center(string content, const int max_width) {
    return paragraph_impl(move(content), max_width, style::align::center);
}

element paragraph_align_right(string content, const int max_width) {
    return paragraph_impl(move(content), max_width, style::align::end);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
