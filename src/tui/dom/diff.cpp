#include <NeForce/core/string/codepoint.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/string/width.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/dom/diff.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    bool parse_range(const string_view text, const size_t from, int& first, int& count) {
        size_t pos = from;
        int value = 0;
        bool has_digits = false;
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
            value = value * 10 + (text[pos] - '0');
            has_digits = true;
            ++pos;
        }
        if (!has_digits) {
            return false;
        }
        first = value;
        count = 1;
        if (pos < text.size() && text[pos] == ',') {
            ++pos;
            int number = 0;
            bool digits = false;
            while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
                number = number * 10 + (text[pos] - '0');
                digits = true;
                ++pos;
            }
            if (digits) {
                count = number;
            }
        }
        return true;
    }

    vector<string_view> split_lines(const string_view text) {
        vector<string_view> lines;
        size_t start = 0;
        for (size_t i = 0; i <= text.size(); ++i) {
            if (i == text.size() || text[i] == '\n') {
                lines.push_back(text.substr(start, i - start));
                start = i + 1;
            }
        }
        if (!lines.empty() && lines.back().empty() && !text.empty() && text.back() == '\n') {
            lines.pop_back();
        }
        return lines;
    }

    int cell_width(const codepoint& cp) noexcept {
        const uint32_t value = cp.value();
        if (value < 0x20 || value == 0x7F) {
            return 1;
        }
        return cp.display_width();
    }

    struct codepoint_step {
        uint32_t value = 0;
        size_t next = 0;
        int width = 0;
    };

    codepoint_step decode_step(const string_view text, const size_t pos) {
        size_t cursor = pos;
        const codepoint cp = codepoint::decode_utf8(reinterpret_cast<const byte_t*>(text.data()), cursor, text.size());
        codepoint_step step;
        step.value = cp.value();
        step.next = cursor;
        step.width = cell_width(cp);
        return step;
    }

    string make_spaces(const int count) {
        if (count <= 0) {
            return string();
        }
        return string(static_cast<size_t>(count), ' ');
    }

    string_view width_prefix(const string_view text, const int budget) {
        if (budget <= 0) {
            return string_view{};
        }
        size_t pos = 0;
        int used = 0;
        while (pos < text.size()) {
            const codepoint_step step = decode_step(text, pos);
            if (step.width > budget - used) {
                break;
            }
            used += step.width;
            pos = step.next;
        }
        return text.substr(0, pos);
    }

    string fit_text(const string_view text, const int width) {
        if (width <= 0) {
            return string();
        }
        const int measured = text_display_width(text);
        if (measured <= width) {
            string result(text);
            result += make_spaces(width - measured);
            return result;
        }
        if (width == 1) {
            return string("\xe2\x80\xa6");
        }
        string result(width_prefix(text, width - 1));
        result += "\xe2\x80\xa6";
        const int remaining = width - text_display_width(result.view());
        if (remaining > 0) {
            result += make_spaces(remaining);
        }
        return result;
    }

    vector<text_span> fit_spans(vector<text_span> spans, const int width, const tui::style& padding_style) {
        if (width <= 0) {
            return {};
        }

        int total = 0;
        for (const auto& span: spans) {
            total += text_display_width(span.content.view());
        }
        if (total <= width) {
            if (total < width) {
                spans.push_back(text_span{make_spaces(width - total), padding_style, string()});
            }
            return spans;
        }

        // The content does not fit, so reserve one column for the ellipsis.
        const int budget = max(0, width - 1);
        vector<text_span> result;
        int used = 0;
        for (const auto& span: spans) {
            if (used >= budget) {
                break;
            }
            const string_view kept = width_prefix(span.content.view(), budget - used);
            if (!kept.empty()) {
                result.push_back(text_span{string(kept), span.style, span.link});
            }
            used += text_display_width(kept);
            if (kept.size() < span.content.size()) {
                break;
            }
        }
        result.push_back(text_span{string("\xe2\x80\xa6"), padding_style, string()});
        ++used;
        if (used < width) {
            result.push_back(text_span{make_spaces(width - used), padding_style, string()});
        }
        return result;
    }

    struct diff_styles {
        tui::style base;
        tui::style gutter;
        tui::style added;
        tui::style removed;
        tui::style hunk;
        tui::style meta;
        tui::style word_added;
        tui::style word_removed;
    };

    diff_styles make_styles(const diff_view_option& opt) {
        diff_styles styles;
        styles.base = opt.style;

        styles.gutter = opt.gutter_style;
        if (!styles.gutter.dim.has_value()) {
            styles.gutter.dim = true;
        }

        styles.added = opt.added_style;
        if (!styles.added.fg.has_value()) {
            styles.added.fg = _NEFORCE color::green();
        }
        styles.removed = opt.removed_style;
        if (!styles.removed.fg.has_value()) {
            styles.removed.fg = _NEFORCE color::red();
        }

        styles.hunk = styles.base;
        styles.hunk.fg = _NEFORCE color::cyan();
        styles.meta = styles.base;
        styles.meta.dim = true;

        styles.word_added = tui::style::merge(styles.added, opt.word_added_style);
        if (!styles.word_added.bold.has_value()) {
            styles.word_added.bold = true;
        }
        styles.word_removed = tui::style::merge(styles.removed, opt.word_removed_style);
        if (!styles.word_removed.bold.has_value()) {
            styles.word_removed.bold = true;
        }
        return styles;
    }

    const tui::style& style_for(const diff_styles& styles, const diff_kind kind) {
        switch (kind) {
            case diff_kind::added:
                return styles.added;
            case diff_kind::removed:
                return styles.removed;
            case diff_kind::hunk:
                return styles.hunk;
            case diff_kind::meta:
                return styles.meta;
            case diff_kind::context:
            default:
                return styles.base;
        }
    }

    const tui::style& word_style_for(const diff_styles& styles, const diff_kind kind) {
        if (kind == diff_kind::added) {
            return styles.word_added;
        }
        if (kind == diff_kind::removed) {
            return styles.word_removed;
        }
        return styles.base;
    }

    struct decoded_text {
        vector<uint32_t> values;
        vector<size_t> offsets;
    };

    decoded_text decode_codepoints(const string_view text) {
        decoded_text result;
        result.offsets.push_back(0);
        size_t pos = 0;
        while (pos < text.size()) {
            const codepoint_step step = decode_step(text, pos);
            result.values.push_back(step.value);
            pos = step.next;
            result.offsets.push_back(pos);
        }
        return result;
    }

    bool is_zero_width(const uint32_t value) { return codepoint(value).display_width() == 0; }

    struct word_pieces {
        size_t middle_begin = 0;
        size_t middle_end = 0;

        NEFORCE_NODISCARD bool has_middle() const noexcept { return middle_end > middle_begin; }
    };

    word_pieces locate_word_pieces(const string_view text, const string_view peer) {
        const decoded_text self = decode_codepoints(text);
        const decoded_text other = decode_codepoints(peer);
        const size_t self_count = self.values.size();
        const size_t other_count = other.values.size();

        size_t prefix = 0;
        while (prefix < self_count && prefix < other_count && self.values[prefix] == other.values[prefix]) {
            ++prefix;
        }
        size_t suffix = 0;
        while (suffix + prefix < self_count && suffix + prefix < other_count &&
               self.values[self_count - 1 - suffix] == other.values[other_count - 1 - suffix]) {
            ++suffix;
        }

        size_t begin = prefix;
        size_t end = self_count - suffix;
        if (begin < end) {
            // Keep a combining mark in the same segment as the base character it follows.
            while (begin > 0 && is_zero_width(self.values[begin])) {
                --begin;
            }
            while (end > begin && end < self_count && is_zero_width(self.values[end])) {
                --end;
            }
        }

        word_pieces pieces;
        pieces.middle_begin = self.offsets[begin];
        pieces.middle_end = end > begin ? self.offsets[end] : self.offsets[begin];
        return pieces;
    }

    vector<text_span> word_line_spans(const diff_line& line, const diff_line& peer, const tui::style& base,
                                      const tui::style& word, bool& highlighted) {
        const word_pieces pieces = locate_word_pieces(line.text.view(), peer.text.view());
        const string_view raw = line.text.view();
        highlighted = pieces.has_middle();

        vector<text_span> spans;
        if (pieces.middle_begin > 0) {
            spans.push_back(text_span{string(raw.substr(0, pieces.middle_begin)), base, string()});
        }
        if (pieces.has_middle()) {
            spans.push_back(text_span{string(raw.substr(pieces.middle_begin, pieces.middle_end - pieces.middle_begin)),
                                      word, string()});
        }
        if (pieces.middle_end < raw.size()) {
            spans.push_back(text_span{string(raw.substr(pieces.middle_end)), base, string()});
        }
        if (spans.empty()) {
            spans.push_back(text_span{string(), base, string()});
        }
        return spans;
    }

    int measure_gutter(const vector<diff_line>& lines) {
        int gutter_width = 1;
        for (const auto& line: lines) {
            gutter_width = max(gutter_width, static_cast<int>(to_string(max(line.old_number, line.new_number)).size()));
        }
        return gutter_width;
    }

    bool split_active(const diff_view_option& opt) {
        return opt.side_by_side && opt.width > 0 && opt.width >= opt.split_min_width;
    }

    bool can_split(const diff_view_option& opt, const int gutter_width) {
        if (!split_active(opt)) {
            return false;
        }
        const int side_fixed = (opt.show_gutter ? gutter_width + 1 : 0) + (opt.show_markers ? 1 : 0);
        return opt.width - side_fixed * 2 - 1 >= 2;
    }

    struct unified_entry {
        const diff_line* line = nullptr;
        const diff_line* pair = nullptr;
    };

    vector<unified_entry> build_unified_entries(const vector<diff_line>& lines, const bool word_level) {
        vector<unified_entry> entries;
        size_t i = 0;
        while (i < lines.size()) {
            if (lines[i].kind == diff_kind::removed) {
                const size_t removed_begin = i;
                while (i < lines.size() && lines[i].kind == diff_kind::removed) {
                    ++i;
                }
                const size_t removed_end = i;
                const size_t added_begin = i;
                while (i < lines.size() && lines[i].kind == diff_kind::added) {
                    ++i;
                }
                const size_t added_end = i;
                const size_t removed_count = removed_end - removed_begin;
                const size_t added_count = added_end - added_begin;
                for (size_t k = 0; k < removed_count; ++k) {
                    unified_entry entry;
                    entry.line = &lines[removed_begin + k];
                    if (word_level && k < added_count) {
                        entry.pair = &lines[added_begin + k];
                    }
                    entries.push_back(entry);
                }
                for (size_t k = 0; k < added_count; ++k) {
                    unified_entry entry;
                    entry.line = &lines[added_begin + k];
                    if (word_level && k < removed_count) {
                        entry.pair = &lines[removed_begin + k];
                    }
                    entries.push_back(entry);
                }
                continue;
            }

            unified_entry entry;
            entry.line = &lines[i];
            entries.push_back(entry);
            ++i;
        }
        return entries;
    }

    struct split_row {
        const diff_line* full = nullptr;
        const diff_line* left = nullptr;
        const diff_line* right = nullptr;
        bool pair = false;
    };

    vector<split_row> build_split_rows(const vector<diff_line>& lines) {
        vector<split_row> rows;
        size_t i = 0;
        while (i < lines.size()) {
            if (lines[i].kind == diff_kind::removed) {
                const size_t removed_begin = i;
                while (i < lines.size() && lines[i].kind == diff_kind::removed) {
                    ++i;
                }
                const size_t removed_end = i;
                const size_t added_begin = i;
                while (i < lines.size() && lines[i].kind == diff_kind::added) {
                    ++i;
                }
                const size_t added_end = i;
                const size_t removed_count = removed_end - removed_begin;
                const size_t added_count = added_end - added_begin;
                const size_t count = max(removed_count, added_count);
                for (size_t k = 0; k < count; ++k) {
                    split_row row;
                    if (k < removed_count) {
                        row.left = &lines[removed_begin + k];
                    }
                    if (k < added_count) {
                        row.right = &lines[added_begin + k];
                    }
                    row.pair = row.left != nullptr && row.right != nullptr;
                    rows.push_back(row);
                }
                continue;
            }

            if (lines[i].kind == diff_kind::added) {
                while (i < lines.size() && lines[i].kind == diff_kind::added) {
                    split_row row;
                    row.right = &lines[i];
                    rows.push_back(row);
                    ++i;
                }
                continue;
            }

            split_row row;
            if (lines[i].kind == diff_kind::context) {
                row.left = &lines[i];
                row.right = &lines[i];
            } else {
                row.full = &lines[i];
            }
            rows.push_back(row);
            ++i;
        }
        return rows;
    }

    void append_split_side(elements& cells, const diff_line* line, const bool right_side, const int content_width,
                           const diff_view_option& opt, const diff_styles& styles, const int gutter_width,
                           const diff_line* word_peer) {
        if (opt.show_gutter) {
            const int number = line == nullptr ? 0 : (right_side ? line->new_number : line->old_number);
            string number_text = number > 0 ? to_string(static_cast<int64_t>(number)) : string();
            while (compare_less(number_text.size(), static_cast<size_t>(gutter_width))) {
                number_text = " " + number_text;
            }
            cells.push_back(element::text(number_text + " ", styles.gutter, style::wrap_mode::none));
        }

        const diff_kind kind = line == nullptr ? diff_kind::context : line->kind;
        const tui::style& base = style_for(styles, kind);
        if (opt.show_markers) {
            const char marker = line == nullptr                      ? ' '
                                : (line->kind == diff_kind::added)   ? '+'
                                : (line->kind == diff_kind::removed) ? '-'
                                                                     : ' ';
            cells.push_back(element::text(string(1, marker), base, style::wrap_mode::none));
        }

        vector<text_span> spans;
        if (line == nullptr) {
            spans.push_back(text_span{string(), base, string()});
        } else {
            const bool word_pair = opt.word_level && word_peer != nullptr &&
                                   ((line->kind == diff_kind::removed && word_peer->kind == diff_kind::added) ||
                                    (line->kind == diff_kind::added && word_peer->kind == diff_kind::removed));
            if (word_pair) {
                bool highlighted = false;
                spans = word_line_spans(*line, *word_peer, base, word_style_for(styles, line->kind), highlighted);
            } else {
                spans.push_back(text_span{line->text, base, string()});
            }
        }

        spans = fit_spans(move(spans), content_width, base);
        cells.push_back(element::rich_text(move(spans), base, style::wrap_mode::none));
    }

    element render_side_by_side(const vector<diff_line>& lines, const diff_view_option& opt, const diff_styles& styles,
                                const int gutter_width) {
        const int side_fixed = (opt.show_gutter ? gutter_width + 1 : 0) + (opt.show_markers ? 1 : 0);
        const int remaining = opt.width - side_fixed * 2 - 1;
        const int left_content = remaining / 2;
        const int right_content = remaining - left_content;

        elements rows;
        for (const auto& row: build_split_rows(lines)) {
            elements cells;
            if (row.full != nullptr) {
                append_split_side(cells, row.full, false, opt.width - side_fixed, opt, styles, gutter_width, nullptr);
            } else {
                append_split_side(cells, row.left, false, left_content, opt, styles, gutter_width,
                                  row.pair ? row.right : nullptr);
                cells.push_back(element::text("\xe2\x94\x82", styles.gutter, style::wrap_mode::none));
                append_split_side(cells, row.right, true, right_content, opt, styles, gutter_width,
                                  row.pair ? row.left : nullptr);
            }
            rows.push_back(element::hbox(move(cells)));
        }

        if (rows.empty()) {
            rows.push_back(element::text(make_spaces(opt.width)));
        }
        return element::vbox(move(rows));
    }

    element render_unified(const vector<diff_line>& lines, const diff_view_option& opt, const diff_styles& styles,
                           const int gutter_width) {
        elements rows;
        for (const auto& entry: build_unified_entries(lines, opt.word_level)) {
            const diff_line& line = *entry.line;
            elements cells;

            if (opt.show_gutter) {
                string old_text = line.old_number > 0 ? to_string(static_cast<int64_t>(line.old_number)) : string();
                string new_text = line.new_number > 0 ? to_string(static_cast<int64_t>(line.new_number)) : string();
                while (compare_less(old_text.size(), static_cast<size_t>(gutter_width))) {
                    old_text = " " + old_text;
                }
                while (compare_less(new_text.size(), static_cast<size_t>(gutter_width))) {
                    new_text = " " + new_text;
                }
                cells.push_back(element::text(old_text + " " + new_text + " ", styles.gutter, style::wrap_mode::none));
            }

            const tui::style& line_style = style_for(styles, line.kind);

            if (opt.show_markers) {
                const char marker = (line.kind == diff_kind::added)     ? '+'
                                    : (line.kind == diff_kind::removed) ? '-'
                                                                        : ' ';
                cells.push_back(element::text(string(1, marker), line_style, style::wrap_mode::none));
            }

            bool highlighted = false;
            vector<text_span> spans;
            if (opt.word_level && entry.pair != nullptr) {
                spans = word_line_spans(line, *entry.pair, line_style, word_style_for(styles, line.kind), highlighted);
            }
            if (highlighted) {
                cells.push_back(element::rich_text(move(spans), line_style, style::wrap_mode::none));
            } else {
                cells.push_back(element::text(line.text, line_style, style::wrap_mode::none));
            }
            rows.push_back(element::hbox(move(cells)));
        }

        if (rows.empty()) {
            rows.push_back(element::text(" "));
        }
        return element::vbox(move(rows));
    }

    element render_diff_lines(const vector<diff_line>& lines, const diff_view_option& opt) {
        const diff_styles styles = make_styles(opt);
        const int gutter_width = measure_gutter(lines);
        if (can_split(opt, gutter_width)) {
            return render_side_by_side(lines, opt, styles, gutter_width);
        }
        return render_unified(lines, opt, styles, gutter_width);
    }
} // namespace


vector<diff_line> parse_unified_diff(const string_view patch) {
    vector<diff_line> result;
    const auto raw_lines = split_lines(patch);

    int old_number = 0;
    int new_number = 0;

    for (const auto& line: raw_lines) {
        if (line.starts_with("@@")) {
            diff_line entry;
            entry.kind = diff_kind::hunk;
            entry.text = string(line);
            int first = 0;
            int count = 0;
            const size_t minus = line.find('-');
            if (minus != string_view::npos && parse_range(line, minus + 1, first, count)) {
                old_number = first;
            }
            const size_t plus = line.find('+');
            if (plus != string_view::npos && parse_range(line, plus + 1, first, count)) {
                new_number = first;
            }
            result.push_back(move(entry));
            continue;
        }

        if (line.starts_with("diff --git") || line.starts_with("index ") || line.starts_with("--- ") ||
            line.starts_with("+++ ") || line.starts_with("\\ ")) {
            diff_line entry;
            entry.kind = diff_kind::meta;
            entry.text = string(line);
            result.push_back(move(entry));
            continue;
        }

        if (line.starts_with("+")) {
            diff_line entry;
            entry.kind = diff_kind::added;
            entry.text = string(line.substr(1));
            entry.new_number = new_number++;
            result.push_back(move(entry));
            continue;
        }

        if (line.starts_with("-")) {
            diff_line entry;
            entry.kind = diff_kind::removed;
            entry.text = string(line.substr(1));
            entry.old_number = old_number++;
            result.push_back(move(entry));
            continue;
        }

        if (line.empty()) {
            diff_line entry;
            entry.kind = diff_kind::context;
            entry.text = string();
            entry.old_number = old_number++;
            entry.new_number = new_number++;
            result.push_back(move(entry));
            continue;
        }

        diff_line entry;
        entry.kind = diff_kind::context;
        entry.text = string(line.substr(1));
        entry.old_number = old_number++;
        entry.new_number = new_number++;
        result.push_back(move(entry));
    }

    return result;
}

element diff_view(const vector<diff_line>& lines, diff_view_option opt) {
    element body = render_diff_lines(lines, opt);
    if (opt.background) {
        tui::style band_style = opt.band_style;
        if (!band_style.bg.has_value()) {
            band_style.bg = _NEFORCE color{26, 26, 30};
        }
        body = element::band(move(body), band_style);
    }
    if (opt.width > 0) {
        tui::style width_style;
        width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
        body = body.with_style(width_style);
    }
    return body;
}

vector<diff_file> parse_unified_diff_files(const string_view patch) {
    vector<diff_file> files;

    struct segment {
        string header;
        string path;
        string body;
    };

    vector<segment> segments;
    segment current;

    const auto lines = split_lines(patch);
    for (const auto& line: lines) {
        if (line.starts_with("diff --git")) {
            if (!current.body.empty() || !current.header.empty()) {
                segments.push_back(move(current));
                current = segment{};
            }
            current.header = string(line);
            current.body += string(line);
            current.body += '\n';
            continue;
        }
        if (line.starts_with("+++ ")) {
            const auto path = line.substr(4).trim();
            if (!path.empty() && current.path.empty()) {
                current.path = string(path);
            }
        }
        current.body += string(line);
        current.body += '\n';
    }
    if (!current.body.empty()) {
        segments.push_back(move(current));
    }

    for (auto& item: segments) {
        diff_file file;
        file.header = item.header;
        file.path = item.path.empty() ? item.header : item.path;
        file.lines = parse_unified_diff(item.body.view());
        for (const auto& entry: file.lines) {
            if (entry.kind == diff_kind::added) {
                ++file.added;
            } else if (entry.kind == diff_kind::removed) {
                ++file.removed;
            }
        }
        files.push_back(move(file));
    }

    return files;
}

element diff_view(const vector<diff_file>& files, diff_view_option opt) {
    elements blocks;

    for (const auto& file: files) {
        if (opt.group_by_file && !file.path.empty()) {
            tui::style header_style = opt.header_style;
            if (!header_style.bold.has_value()) {
                header_style.bold = true;
            }
            string header = file.path;
            header += "  \xe2\x80\xa2 +";
            header += to_string(static_cast<int64_t>(file.added));
            header += " -";
            header += to_string(static_cast<int64_t>(file.removed));
            if (split_active(opt)) {
                header = fit_text(header.view(), opt.width);
            }
            blocks.push_back(
                    element::rich_text({text_span{header, header_style}}, header_style, style::wrap_mode::none));
        }

        vector<diff_line> visible;
        if (opt.collapse_hunk_after > 0) {
            size_t hunk_count = 0;
            for (const auto& line: file.lines) {
                if (line.kind == diff_kind::hunk) {
                    if (hunk_count > opt.collapse_hunk_after && !visible.empty()) {
                        diff_line hint;
                        hint.kind = diff_kind::meta;
                        hint.text = "\xe2\x80\xa6 " + to_string(static_cast<int64_t>(hunk_count)) +
                                    " lines hidden \xc2\xb7 expand to see more";
                        visible.push_back(move(hint));
                        break;
                    }
                    hunk_count = 0;
                }
                ++hunk_count;
                visible.push_back(line);
            }
        } else {
            visible = file.lines;
        }

        vector<diff_line> body_lines;
        body_lines.reserve(visible.size());
        for (const auto& line: visible) {
            if (opt.group_by_file && line.kind == diff_kind::meta) {
                const auto& raw = line.text.view();
                const bool file_header = raw.starts_with("diff --git") || raw.starts_with("index ") ||
                                         raw.starts_with("--- ") || raw.starts_with("+++ ");
                if (file_header) {
                    continue;
                }
            }
            body_lines.push_back(line);
        }

        element body = render_diff_lines(body_lines, opt);
        if (opt.background) {
            tui::style band_style = opt.band_style;
            if (!band_style.bg.has_value()) {
                band_style.bg = _NEFORCE color{26, 26, 30};
            }
            body = element::band(move(body), band_style);
        }
        blocks.push_back(move(body));
    }

    if (blocks.empty()) {
        blocks.push_back(element::text(" "));
    }

    element result = element::vbox(move(blocks));
    if (opt.width > 0) {
        tui::style width_style;
        width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
        result = result.with_style(width_style);
    }
    return result;
}

element diff_view(const string_view patch, diff_view_option opt) {
    if (opt.group_by_file) {
        return diff_view(parse_unified_diff_files(patch), move(opt));
    }
    return diff_view(parse_unified_diff(patch), move(opt));
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
