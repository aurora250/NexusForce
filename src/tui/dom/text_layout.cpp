#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/dom/text_layout.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    int codepoint_width(const codepoint& cp) {
        const auto value = cp.value();
        if (value < 0x20 || value == 0x7F) {
            return 1;
        }
        return cp.display_width();
    }

    size_t codepoint_bytes(const byte_t lead) {
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

    codepoint decode_at(const byte_t* data, const size_t len, const size_t pos) {
        const auto lead = data[pos];
        const size_t bytes = min(codepoint_bytes(lead), len - pos);
        char32_t value = 0;
        if (bytes == 1) {
            value = lead;
        } else if (bytes == 2) {
            value = static_cast<char32_t>(lead & 0x1F) << 6;
            value |= static_cast<byte_t>(data[pos + 1]) & 0x3F;
        } else if (bytes == 3) {
            value = static_cast<char32_t>(lead & 0x0F) << 12;
            value |= (static_cast<byte_t>(data[pos + 1]) & 0x3F) << 6;
            value |= static_cast<byte_t>(data[pos + 2]) & 0x3F;
        } else {
            value = static_cast<char32_t>(lead & 0x07) << 18;
            value |= (static_cast<byte_t>(data[pos + 1]) & 0x3F) << 12;
            value |= (static_cast<byte_t>(data[pos + 2]) & 0x3F) << 6;
            value |= static_cast<byte_t>(data[pos + 3]) & 0x3F;
        }
        return codepoint{value};
    }

    text_layout layout_impl(const string_view text, const int width, const style::wrap_mode wm, const int hang_cols) {
        text_layout result;

        const auto* data = reinterpret_cast<const byte_t*>(text.data());
        const size_t len = text.size();
        if (len == 0) {
            result.lines.push_back(text_line{0, 0, false});
            return result;
        }

        // wrap_mode::none never consults the width,
        // so the layout degenerates to splitting on hard breaks:
        // skip the per-codepoint width accounting entirely.
        if (wm == style::wrap_mode::none) {
            size_t begin = 0;
            for (size_t i = 0; i < len; ++i) {
                if (text[i] == '\n') {
                    result.lines.push_back(text_line{begin, i, false});
                    begin = i + 1;
                }
            }
            result.lines.push_back(text_line{begin, len, false});
            return result;
        }

        size_t pos = 0;
        size_t line_begin = 0;
        int line_width = 0;
        size_t last_space = string_view::npos;
        bool continuation = false;
        int limit = max(1, width - (continuation ? hang_cols : 0));

        while (pos < len) {
            if (data[pos] == '\n') {
                result.lines.push_back(text_line{line_begin, pos, continuation});
                ++pos;
                line_begin = pos;
                line_width = 0;
                last_space = string_view::npos;
                continuation = false;
                limit = max(1, width);
                continue;
            }

            const size_t bytes = min(codepoint_bytes(data[pos]), len - pos);
            const codepoint cp = decode_at(data, len, pos);
            const int cw = codepoint_width(cp);

            if (wm != style::wrap_mode::none && line_width + cw > limit) {
                if (cp.value() == ' ') {
                    // The space itself overflows:
                    // break here and drop it together with any run of spaces that follows.
                    result.lines.push_back(text_line{line_begin, pos, continuation});
                    size_t next = pos + 1;
                    while (next < len && data[next] == ' ') {
                        ++next;
                    }
                    pos = next;
                    line_begin = next;
                    line_width = 0;
                    last_space = string_view::npos;
                    continuation = true;
                    limit = max(1, width - (hang_cols > 0 ? hang_cols : 0));
                    continue;
                }
                if (wm == style::wrap_mode::word && last_space != string_view::npos && last_space > line_begin) {
                    result.lines.push_back(text_line{line_begin, last_space, continuation});
                    size_t next = last_space + 1;
                    while (next < len && data[next] == ' ') {
                        ++next;
                    }
                    pos = next;
                    line_begin = next;
                    line_width = 0;
                    last_space = string_view::npos;
                    continuation = true;
                    limit = max(1, width - (hang_cols > 0 ? hang_cols : 0));
                    continue;
                }
                if (pos > line_begin) {
                    result.lines.push_back(text_line{line_begin, pos, continuation});
                    line_begin = pos;
                    line_width = 0;
                    last_space = string_view::npos;
                    continuation = true;
                    limit = max(1, width - (hang_cols > 0 ? hang_cols : 0));
                    continue;
                }
            }

            if (cp.value() == ' ') {
                last_space = pos;
            }
            line_width += cw;
            pos += bytes;
        }

        result.lines.push_back(text_line{line_begin, len, continuation});
        return result;
    }
} // namespace


text_layout layout_text(const string_view text, const int width, const style::wrap_mode wm) {
    return layout_impl(text, width, wm, 0);
}

text_layout layout_text_hang(const string_view text, const int width, const style::wrap_mode wm, const int hang_cols) {
    if (hang_cols <= 0) {
        return layout_impl(text, width, wm, 0);
    }
    return layout_impl(text, width, wm, hang_cols);
}

void caret_position(const text_layout& layout, const string_view text, const size_t offset, int& row, int& col) {
    const size_t clamped = min(offset, text.size());
    row = 0;
    col = 0;

    for (size_t i = 0; i < layout.lines.size(); ++i) {
        const auto& line = layout.lines[i];
        if (clamped >= line.begin && clamped <= line.end) {
            row = static_cast<int>(i);
            col = _NEFORCE text_display_width(text.substr(line.begin, clamped - line.begin));
            return;
        }
        if (clamped < line.begin) {
            row = (i == 0) ? 0 : static_cast<int>(i - 1);
            col = _NEFORCE text_display_width(
                    text.substr(layout.lines[i - 1].begin, layout.lines[i - 1].end - layout.lines[i - 1].begin));
            return;
        }
    }

    const auto& last = layout.lines.back();
    row = static_cast<int>(layout.lines.size()) - 1;
    col = _NEFORCE text_display_width(text.substr(last.begin, last.end - last.begin));
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
