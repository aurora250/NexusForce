#include <NeForce/tui/screen.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <Windows.h>
#else
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    void append_cursor_move(string& ss, const int x, const int y) {
        ss.append("\033[");
        ss.append(to_string(y + 1));
        ss.append(";");
        ss.append(to_string(x + 1));
        ss.append("H");
    }
} // namespace


surface::surface(const int dimx, const int dimy) :
dimx_(dimx),
dimy_(dimy) {
    const size_t count = static_cast<size_t>(dimx) * dimy;
    cells_.resize(count);
    surface::clear();
}

string& surface::at(const int x, const int y) {
    if (x < 0 || x >= dimx_ || y < 0 || y >= dimy_) {
        static string empty;
        return empty;
    }
    return fast_cell_at(x, y).character;
}

cell& surface::cell_at(const int x, const int y) {
    if (x < 0 || x >= dimx_ || y < 0 || y >= dimy_) {
        static cell empty_cell;
        return empty_cell;
    }
    return fast_cell_at(x, y);
}

void surface::clear() {
    for (auto& cell: cells_) {
        cell.reset();
        cell.character = " ";
    }
}

screen::screen(const int dimx, const int dimy) :
surface(dimx, dimy) {}

void screen::resize(const int dimx, const int dimy) {
    dimx_ = dimx;
    dimy_ = dimy;
    const size_t count = static_cast<size_t>(dimx) * dimy;
    cells_.resize(count);
    clear();
}

uint8_t screen::register_hyperlink(const string& link) {
    hyperlinks_.push_back(link);
    if (hyperlinks_.size() > 255) {
        return 0;
    }
    return static_cast<uint8_t>(hyperlinks_.size());
}

const string& screen::hyperlink(const uint8_t id) const {
    static const string empty;
    const size_t idx = static_cast<size_t>(id) - 1;
    if (id == 0 || idx >= hyperlinks_.size()) {
        return empty;
    }
    return hyperlinks_[idx];
}

void screen::clear() {
    surface::clear();
    cursor_ = tui::cursor{};
    hyperlinks_.clear();
}

void screen::reset_position(const bool clear) {
    if (clear) {
        this->clear();
    }
    cursor_ = tui::cursor{};
}

void screen::update_cell_style(string& ss, const cell& prev, const cell& next) const {
    if (next.hyperlink != prev.hyperlink) {
        ss += "\033]8;;";
        ss += hyperlink(next.hyperlink);
        ss += "\033\\";
    }

    if (((next.bold ^ prev.bold) | (next.dim ^ prev.dim)) != 0) {
        if ((prev.bold && !next.bold) || (prev.dim && !next.dim)) {
            ss += "\033[22m";
        }
        if (next.bold) {
            ss += "\033[1m";
        }
        if (next.dim) {
            ss += "\033[2m";
        }
    }

    if (next.italic ^ prev.italic) {
        ss += next.italic ? "\033[3m" : "\033[23m";
    }

    if (((next.underlined ^ prev.underlined) | (next.underlined_double ^ prev.underlined_double)) != 0) {
        if ((prev.underlined && !next.underlined) || (prev.underlined_double && !next.underlined_double)) {
            ss += "\033[24m";
        }
        if (next.underlined_double) {
            ss += "\033[21m";
        } else if (next.underlined) {
            ss += "\033[4m";
        }
    }

    if (next.blink ^ prev.blink) {
        ss += next.blink ? "\033[5m" : "\033[25m";
    }

    if (next.inverted ^ prev.inverted) {
        ss += next.inverted ? "\033[7m" : "\033[27m";
    }

    if (next.strikethrough ^ prev.strikethrough) {
        ss += next.strikethrough ? "\033[9m" : "\033[29m";
    }

    if (!next.foreground.equal_to(prev.foreground)) {
        if (next.foreground.A() == 0) {
            ss += "\033[39m";
        } else {
            ss += "\033[38;5;";
            ss += _NEFORCE to_string(next.foreground.to_ansi_256());
            ss += "m";
        }
    }

    if (!next.background.equal_to(prev.background)) {
        if (next.background.A() == 0) {
            ss += "\033[49m";
        } else {
            ss += "\033[48;5;";
            ss += _NEFORCE to_string(next.background.to_ansi_256());
            ss += "m";
        }
    }
}

string screen::to_string(const screen& prev) const {
    string result;
    result.reserve(static_cast<size_t>(dimx_) * dimy_ * 4);
    result += "\033[?25l";

    int last_x = -1;
    int last_y = -1;

    tui::cell term_state;
    term_state.reset();

    for (int y = 0; y < dimy_ && y < prev.dimy_; ++y) {
        for (int x = 0; x < dimx_ && x < prev.dimx_; ++x) {
            const size_t idx = static_cast<size_t>(y) * dimx_ + x;
            const tui::cell& cell = cells_[idx];

            if (cell.automerge) {
                continue;
            }

            const tui::cell& prev_cell = prev.cells_[idx];

            const bool content_same = cell.visually_equal(prev_cell);
            const bool style_match = cell.visually_equal(term_state);
            if (content_same && style_match) {
                continue;
            }

            if (last_y != y || last_x + 1 != x) {
                append_cursor_move(result, x, y);
            }
            last_x = x;
            last_y = y;

            update_cell_style(result, term_state, cell);

            term_state.foreground = cell.foreground;
            term_state.background = cell.background;
            term_state.bold = cell.bold;
            term_state.dim = cell.dim;
            term_state.italic = cell.italic;
            term_state.inverted = cell.inverted;
            term_state.underlined = cell.underlined;
            term_state.underlined_double = cell.underlined_double;
            term_state.blink = cell.blink;
            term_state.strikethrough = cell.strikethrough;
            term_state.hyperlink = cell.hyperlink;

            result.append(cell.character);
        }
    }

    result += "\033[0m";

    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
