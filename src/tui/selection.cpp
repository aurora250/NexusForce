#include <NeForce/tui/selection.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    // Width in columns of the single glyph stored in a cell.
    // Wide glyphs occupy one column themselves and leave their remaining columns empty,
    // which the text extractor must skip.
    int glyph_width(const string& glyph) {
        if (glyph.empty()) {
            return 1;
        }
        size_t index = 0;
        const codepoint cp = codepoint::decode_utf8(reinterpret_cast<const byte_t*>(glyph.data()), index, glyph.size());
        return max(1, cp.display_width());
    }
} // namespace


void screen_selection::start(const int x, const int y) noexcept {
    active = true;
    anchor_x = x;
    anchor_y = y;
    caret_x = x;
    caret_y = y;
}

void screen_selection::extend_to(const int x, const int y) noexcept {
    caret_x = x;
    caret_y = y;
}

void screen_selection::clear() noexcept {
    active = false;
    anchor_x = 0;
    anchor_y = 0;
    caret_x = 0;
    caret_y = 0;
}

void screen_selection::bounds(int& x1, int& y1, int& x2, int& y2) const noexcept {
    const bool forward = anchor_y < caret_y || (anchor_y == caret_y && anchor_x <= caret_x);
    if (forward) {
        x1 = anchor_x;
        y1 = anchor_y;
        x2 = caret_x;
        y2 = caret_y;
    } else {
        x1 = caret_x;
        y1 = caret_y;
        x2 = anchor_x;
        y2 = anchor_y;
    }
}

bool screen_selection::contains(const int x, const int y) const noexcept {
    if (!active) {
        return false;
    }

    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    bounds(x1, y1, x2, y2);

    if (y < y1 || y > y2) {
        return false;
    }
    if (y1 == y2) {
        return x >= x1 && x <= x2;
    }
    if (y == y1) {
        return x >= x1;
    }
    if (y == y2) {
        return x <= x2;
    }
    return true;
}

void paint_selection(surface& target, const screen_selection& selection) {
    if (selection.empty()) {
        return;
    }

    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    selection.bounds(x1, y1, x2, y2);

    const int top = max(y1, 0);
    const int bottom = min(y2, target.dimy() - 1);
    for (int y = top; y <= bottom; ++y) {
        const int from = (y == top) ? max(x1, 0) : 0;
        const int to = (y == bottom) ? min(x2, target.dimx() - 1) : target.dimx() - 1;
        for (int x = from; x <= to; ++x) {
            target.fast_cell_at(x, y).inverted = true;
        }
    }
}

string selection_text(const surface& source, const screen_selection& selection) {
    string text;
    if (selection.empty()) {
        return text;
    }

    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    selection.bounds(x1, y1, x2, y2);

    const int top = max(y1, 0);
    const int bottom = min(y2, source.dimy() - 1);

    for (int y = top; y <= bottom; ++y) {
        const int from = (y == top) ? max(x1, 0) : 0;
        const int to = (y == bottom) ? min(x2, source.dimx() - 1) : source.dimx() - 1;

        string line;
        int skip = 0;
        for (int x = from; x <= to; ++x) {
            const cell& current = source.fast_cell_at(x, y);
            if (skip > 0) {
                --skip;
                continue;
            }
            if (current.character.empty()) {
                line.push_back(' ');
                continue;
            }
            line.append(current.character);
            skip = glyph_width(current.character) - 1;
        }

        size_t end = line.size();
        while (end > 0 && line[end - 1] == ' ') {
            --end;
        }
        if (end != line.size()) {
            line.resize(end);
        }

        if (!text.empty()) {
            text.push_back('\n');
        }
        text.append(line);
    }

    return text;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
