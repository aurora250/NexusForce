#include <NeForce/tui/dom/paragraph.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    vector<string> wrap_text(const string& content, int max_width) {
        vector<string> lines;
        if (content.empty()) {
            lines.push_back("");
            return lines;
        }
        if (max_width <= 0) {
            return {content};
        }

        string current;
        size_t pos = 0;
        while (pos < content.size()) {
            while (pos < content.size() && current.empty() && content[pos] == ' ') {
                ++pos;
            }

            size_t word_end = pos;
            while (word_end < content.size() && content[word_end] != ' ' && content[word_end] != '\n') {
                ++word_end;
            }

            const size_t word_len = word_end - pos;
            const bool is_newline = (word_end < content.size() && content[word_end] == '\n');

            if (!current.empty() && static_cast<int>(current.size() + 1 + word_len) > max_width) {
                lines.push_back(current);
                current.clear();
                continue;
            }

            if (!current.empty()) {
                current += ' ';
            }
            current.append(content.data() + pos, word_len);

            if (is_newline) {
                lines.push_back(current);
                current.clear();
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

    string pad_line(const string& line, int max_width, enum class style::align align) {
        if (static_cast<int>(line.size()) >= max_width) {
            return line;
        }

        const int padding = max_width - static_cast<int>(line.size());
        switch (align) {
            case style::align::end:
                return string(padding, ' ') + line;
            case style::align::center:
                return string(padding / 2, ' ') + line + string(padding - padding / 2, ' ');
            default:
                return line + string(padding, ' ');
        }
    }

    element paragraph_impl(string content, int max_width, enum class style::align align) {
        auto lines = wrap_text(content, max_width);
        elements elements;
        for (auto& line: lines) {
            if (max_width > 0) {
                line = pad_line(line, max_width, align);
            }
            elements.push_back(element::text(move(line)));
        }
        return element::vbox(move(elements));
    }
} // anonymous namespace


element paragraph(string content, int max_width) {
    return paragraph_impl(move(content), max_width, style::align::start);
}

element paragraph_align_center(string content, int max_width) {
    return paragraph_impl(move(content), max_width, style::align::center);
}

element paragraph_align_right(string content, int max_width) {
    return paragraph_impl(move(content), max_width, style::align::end);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
