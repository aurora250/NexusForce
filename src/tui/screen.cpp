#include <NeForce/tui/screen.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    void append_color(string& ss, const color& c, const bool background, const color_depth depth) {
        if (c.A() == 0) {
            ss += background ? "\033[49m" : "\033[39m";
            return;
        }
        if (depth == color_depth::ansi16) {
            ss += "\033[";
            ss += _NEFORCE to_string(c.to_ansi_basic(background));
            ss += "m";
            return;
        }

        ss += "\033[";
        ss += _NEFORCE to_string(background ? 48 : 38);
        if (depth == color_depth::truecolor) {
            ss += ";2;";
            ss += _NEFORCE to_string(c.R());
            ss += ";";
            ss += _NEFORCE to_string(c.G());
            ss += ";";
            ss += _NEFORCE to_string(c.B());
        } else {
            ss += ";5;";
            ss += _NEFORCE to_string(c.to_ansi_256());
        }
        ss += "m";
    }

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
    const auto& raw = link.view();
    for (size_t i = 0; i < hyperlinks_.size(); ++i) {
        if (hyperlinks_[i] == raw) {
            return static_cast<uint8_t>(i + 1);
        }
    }
    if (hyperlinks_.size() >= 255) {
        return 0;
    }
    hyperlinks_.push_back(link);
    return static_cast<uint8_t>(hyperlinks_.size());
}

string_view screen::hyperlink(const uint8_t id) const noexcept {
    const size_t idx = static_cast<size_t>(id) - 1;
    if (id == 0 || idx >= hyperlinks_.size()) {
        return {};
    }
    return hyperlinks_[idx].view();
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

string screen::to_plaintext() const {
    string result;
    result.reserve(static_cast<size_t>(dimx_ + 1) * static_cast<size_t>(dimy_));
    for (int y = 0; y < dimy_; ++y) {
        if (y > 0) {
            result += '\n';
        }
        int last_non_space = -1;
        for (int x = 0; x < dimx_; ++x) {
            const auto& c = cells_[static_cast<size_t>(y) * static_cast<size_t>(dimx_) + static_cast<size_t>(x)];
            if (c.character != " " || c.automerge) {
                last_non_space = x;
            }
        }
        for (int x = 0; x <= last_non_space; ++x) {
            const auto& c = cells_[static_cast<size_t>(y) * static_cast<size_t>(dimx_) + static_cast<size_t>(x)];
            if (!c.character.empty() && !c.automerge) {
                result += c.character;
            }
        }
    }
    while (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
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
        append_color(ss, next.foreground, false, depth_);
    }

    if (!next.background.equal_to(prev.background)) {
        append_color(ss, next.background, true, depth_);
    }
}

void screen::append_cursor(string& ss) const {
    if (cursor_.shape == tui::cursor::shape::hidden) {
        ss += "\033[?25l";
        return;
    }

    int cx = cursor_.x;
    int cy = cursor_.y;
    cx = (cx < 0) ? 0 : ((cx >= dimx_) ? dimx_ - 1 : cx);
    cy = (cy < 0) ? 0 : ((cy >= dimy_) ? dimy_ - 1 : cy);
    append_cursor_move(ss, cx, cy);

    switch (cursor_.shape) {
        case tui::cursor::shape::block_blinking:
            ss += "\033[1 q";
            break;
        case tui::cursor::shape::block:
            ss += "\033[2 q";
            break;
        case tui::cursor::shape::underline_blinking:
            ss += "\033[3 q";
            break;
        case tui::cursor::shape::underline:
            ss += "\033[4 q";
            break;
        case tui::cursor::shape::bar_blinking:
            ss += "\033[5 q";
            break;
        case tui::cursor::shape::bar:
            ss += "\033[6 q";
            break;
        case tui::cursor::shape::hidden:
        default:
            break;
    }
    ss += "\033[?25h";
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
    append_cursor(result);

    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
