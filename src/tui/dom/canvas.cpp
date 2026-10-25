#include <NeForce/core/string/utf_iterator.hpp>
#include <NeForce/core/string/width.hpp>
#include <NeForce/tui/dom/canvas.hpp>
#include <NeForce/tui/screen.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    void apply_style(cell& target, const style& s) {
        if (s.fg.has_value()) {
            target.foreground = s.fg.value();
        }
        if (s.bg.has_value()) {
            target.background = s.bg.value();
        }
        if (s.bold.has_value()) {
            target.bold = s.bold.value();
        }
        if (s.dim.has_value()) {
            target.dim = s.dim.value();
        }
        if (s.underline.has_value()) {
            target.underlined = s.underline.value();
        }
        if (s.underlined_double.has_value()) {
            target.underlined_double = s.underlined_double.value();
        }
        if (s.italic.has_value()) {
            target.italic = s.italic.value();
        }
        if (s.reverse.has_value()) {
            target.inverted = s.reverse.value();
        }
        if (s.blink.has_value()) {
            target.blink = s.blink.value();
        }
        if (s.strikethrough.has_value()) {
            target.strikethrough = s.strikethrough.value();
        }
    }
} // namespace


canvas_painter::canvas_painter(screen& target, const theme& t, const int x, const int y, const int w, const int h,
                               style base, const int clip_x, const int clip_y, const int clip_w, const int clip_h) :
screen_(target),
theme_(t),
base_(move(base)),
origin_x_(x),
origin_y_(y),
w_(max(0, w)),
h_(max(0, h)),
clip_x_(max(0, clip_x)),
clip_y_(max(0, clip_y)),
clip_w_(max(0, clip_w)),
clip_h_(max(0, clip_h)) {}

bool canvas_painter::visible(const int x, const int y) const noexcept {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) {
        return false;
    }
    if (x < clip_x_ || y < clip_y_ || x >= clip_x_ + clip_w_ || y >= clip_y_ + clip_h_) {
        return false;
    }
    const int gx = origin_x_ + x;
    const int gy = origin_y_ + y;
    return gx >= 0 && gy >= 0 && gx < screen_.dimx() && gy < screen_.dimy();
}

bool canvas_painter::contains(const int x, const int y) const noexcept { return visible(x, y); }

int canvas_painter::available_width(const int x) const noexcept {
    const int right = min(w_, clip_x_ + clip_w_);
    return max(0, right - max(x, clip_x_));
}

void canvas_painter::write_cell(const int x, const int y, const codepoint& cp, const style& over) {
    const int cw = max(1, cp.display_width());
    if (!visible(x, y)) {
        return;
    }
    {
        auto& target = screen_.fast_cell_at(origin_x_ + x, origin_y_ + y);
        target.reset();
        const uint32_t value = cp.value();
        if (value < 0x20 || value == 0x7F) {
            target.character = " ";
        } else {
            cp.append_to(target.character);
        }
        apply_style(target, style::merge(base_, over));
    }
    for (int i = 1; i < cw; ++i) {
        if (visible(x + i, y)) {
            auto& next = screen_.fast_cell_at(origin_x_ + x + i, origin_y_ + y);
            next.reset();
            next.automerge = true;
        }
    }
}

void canvas_painter::put(const int x, const int y, const string_view glyph, const style& over) {
    const auto& raw = glyph;
    for (utf8_iterator it(reinterpret_cast<const byte_t*>(raw.data()), raw.size()); it != utf8_iterator(); ++it) {
        write_cell(x, y, *it, over);
        return;
    }
}

void canvas_painter::put(const int x, const int y, const codepoint& cp, const style& over) {
    write_cell(x, y, cp, over);
}

void canvas_painter::fill(const int x, const int y, const int w, const int h, const string_view glyph,
                          const style& over) {
    codepoint cp;
    bool valid = false;
    for (utf8_iterator it(reinterpret_cast<const byte_t*>(glyph.data()), glyph.size()); it != utf8_iterator(); ++it) {
        cp = *it;
        valid = true;
        break;
    }
    if (!valid) {
        return;
    }
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            write_cell(x + col, y + row, cp, over);
        }
    }
}

void canvas_painter::hline(const int x, const int y, const int w, const string_view glyph, const style& over) {
    for (int i = 0; i < w; ++i) {
        put(x + i, y, glyph, over);
    }
}

void canvas_painter::vline(const int x, const int y, const int h, const string_view glyph, const style& over) {
    for (int i = 0; i < h; ++i) {
        put(x, y + i, glyph, over);
    }
}

int canvas_painter::text(const int x, const int y, const string_view content, const style& over) {
    int cx = x;
    const int limit = min(w_, clip_x_ + clip_w_);
    int advanced = 0;
    for (utf8_iterator it(reinterpret_cast<const byte_t*>(content.data()), content.size()); it != utf8_iterator();
         ++it) {
        if (cx >= limit) {
            break;
        }
        const int cw = max(1, it->display_width());
        if (cx >= clip_x_) {
            write_cell(cx, y, *it, over);
            advanced += cw;
        }
        cx += cw;
    }
    return advanced;
}

int canvas_painter::text_right(const int x, const int y, const int w, const string_view content, const style& over) {
    const int text_w = text_display_width(content);
    const int start = x + max(0, w - text_w);
    text(start, y, content, over);
    return start;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
