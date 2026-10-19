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
    const tui::style base_style = opt.style;
    tui::style gutter = opt.gutter_style;
    if (!gutter.dim.has_value()) {
        gutter.dim = true;
    }

    tui::style added_style = base_style;
    added_style.fg = _NEFORCE color::green();
    tui::style removed_style = base_style;
    removed_style.fg = _NEFORCE color::red();
    tui::style hunk_style = base_style;
    hunk_style.fg = _NEFORCE color::cyan();
    tui::style meta_style = base_style;
    meta_style.dim = true;

    int gutter_width = 1;
    for (const auto& line: lines) {
        gutter_width = max(gutter_width, static_cast<int>(to_string(max(line.old_number, line.new_number)).size()));
    }

    elements rows;
    for (const auto& line: lines) {
        elements cells;

        if (opt.show_gutter) {
            string old_text = line.old_number > 0 ? to_string(line.old_number) : string();
            string new_text = line.new_number > 0 ? to_string(line.new_number) : string();
            while (compare_less(old_text.size(), gutter_width)) {
                old_text = " " + old_text;
            }
            while (compare_less(new_text.size(), gutter_width)) {
                new_text = " " + new_text;
            }
            cells.push_back(element::text(old_text + " " + new_text + " ", gutter, style::wrap_mode::none));
        }

        const tui::style& line_style = (line.kind == diff_kind::added)     ? added_style
                                       : (line.kind == diff_kind::removed) ? removed_style
                                       : (line.kind == diff_kind::hunk)    ? hunk_style
                                       : (line.kind == diff_kind::meta)    ? meta_style
                                                                           : base_style;

        if (opt.show_markers) {
            const char marker = (line.kind == diff_kind::added) ? '+' : (line.kind == diff_kind::removed) ? '-' : ' ';
            cells.push_back(element::text(string(1, marker), line_style, style::wrap_mode::none));
        }

        cells.push_back(element::text(line.text, line_style, style::wrap_mode::none));
        rows.push_back(element::hbox(move(cells)));
    }

    if (rows.empty()) {
        rows.push_back(element::text(" "));
    }

    element body = element::vbox(move(rows));
    if (opt.width > 0) {
        tui::style width_style;
        width_style.width = style::size_hint{style::size_hint::fixed, opt.width};
        body = body.with_style(width_style);
    }
    return body;
}

element diff_view(const string_view patch, diff_view_option opt) {
    return diff_view(parse_unified_diff(patch), move(opt));
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
