#include <NeForce/core/string/to_string.hpp>
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

    element render_diff_lines(const vector<diff_line>& lines, const diff_view_option& opt) {
        const tui::style base_style = opt.style;
        tui::style gutter = opt.gutter_style;
        if (!gutter.dim.has_value()) {
            gutter.dim = true;
        }

        tui::style added_style = opt.added_style;
        if (!added_style.fg.has_value()) {
            added_style.fg = _NEFORCE color::green();
        }
        tui::style removed_style = opt.removed_style;
        if (!removed_style.fg.has_value()) {
            removed_style.fg = _NEFORCE color::red();
        }
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
                string old_text = line.old_number > 0 ? to_string(static_cast<int64_t>(line.old_number)) : string();
                string new_text = line.new_number > 0 ? to_string(static_cast<int64_t>(line.new_number)) : string();
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
                const char marker = (line.kind == diff_kind::added)     ? '+'
                                    : (line.kind == diff_kind::removed) ? '-'
                                                                        : ' ';
                cells.push_back(element::text(string(1, marker), line_style, style::wrap_mode::none));
            }

            cells.push_back(element::text(line.text, line_style, style::wrap_mode::none));
            rows.push_back(element::hbox(move(cells)));
        }

        if (rows.empty()) {
            rows.push_back(element::text(" "));
        }

        return element::vbox(move(rows));
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
