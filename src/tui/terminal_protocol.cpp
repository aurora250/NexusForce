#include <NeForce/tui/terminal_protocol.hpp>
#include <NeForce/core/string/to_string.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    char ascii_lower(const char c) noexcept { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

    bool equals_ignore_case(const string_view lhs, const string_view rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (ascii_lower(lhs[i]) != ascii_lower(rhs[i])) {
                return false;
            }
        }
        return true;
    }

    bool contains_ignore_case(const string_view haystack, const string_view needle) noexcept {
        if (needle.empty()) {
            return true;
        }
        if (needle.size() > haystack.size()) {
            return false;
        }
        for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
            bool matched = true;
            for (size_t j = 0; j < needle.size(); ++j) {
                if (ascii_lower(haystack[i + j]) != ascii_lower(needle[j])) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return true;
            }
        }
        return false;
    }

    bool starts_with_ignore_case(const string_view text, const string_view prefix) noexcept {
        return text.size() >= prefix.size() && equals_ignore_case(text.head(prefix.size()), prefix);
    }

    int hex_digit_value(const char c) noexcept {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    }

    // Parse 1-4 hex digits and scale them to 8 bits.
    // Four digits use the high byte, matching the 16-bit component form emitted by xterm.
    bool parse_hex_component(const string_view text, int& value) noexcept {
        if (text.empty() || text.size() > 4) {
            return false;
        }
        int raw = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            const int digit = hex_digit_value(text[i]);
            if (digit < 0) {
                return false;
            }
            raw = raw * 16 + digit;
        }
        switch (text.size()) {
            case 1:
                raw *= 17;
                break;
            case 2:
                break;
            case 3:
                raw >>= 4;
                break;
            case 4:
                raw >>= 8;
                break;
            default:
                return false;
        }
        value = raw;
        return true;
    }

    bool parse_hex_byte(const string_view text, int& value) noexcept {
        if (text.size() != 2) {
            return false;
        }
        const int high = hex_digit_value(text[0]);
        const int low = hex_digit_value(text[1]);
        if (high < 0 || low < 0) {
            return false;
        }
        value = high * 16 + low;
        return true;
    }

    optional<_NEFORCE color> parse_rgb_reply(const string_view payload) {
        const size_t first = payload.find('/');
        if (first == string_view::npos) {
            return none;
        }
        const size_t second = payload.find('/', first + 1);
        if (second == string_view::npos || payload.find('/', second + 1) != string_view::npos) {
            return none;
        }

        int red = 0;
        int green = 0;
        int blue = 0;
        if (!parse_hex_component(payload.head(first), red) ||
            !parse_hex_component(payload.view(first + 1, second - first - 1), green) ||
            !parse_hex_component(payload.tail(second + 1), blue)) {
            return none;
        }
        return _NEFORCE color(red, green, blue);
    }

    optional<_NEFORCE color> parse_hash_reply(const string_view payload) {
        if (payload.size() != 7 || payload[0] != '#') {
            return none;
        }

        int red = 0;
        int green = 0;
        int blue = 0;
        if (!parse_hex_byte(payload.view(1, 2), red) || !parse_hex_byte(payload.view(3, 2), green) ||
            !parse_hex_byte(payload.view(5, 2), blue)) {
            return none;
        }
        return _NEFORCE color(red, green, blue);
    }

    string build_osc9_notification(const string_view title, const string_view body) {
        string result = "\033]9;";
        if (!title.empty()) {
            result.append(title);
            result.append(": ");
        }
        result.append(body);
        result.append("\a");
        return result;
    }
} // namespace


terminal_profile detect_terminal_profile(const string_view term, const string_view colorterm,
                                         const string_view term_program, const string_view tmux,
                                         const string_view kitty_window_id, const string_view wt_session) {
    terminal_profile profile;

    const bool dumb = equals_ignore_case(term, "dumb");
    const bool cons25 = equals_ignore_case(term, "cons25");
    const bool kitty = equals_ignore_case(term, "xterm-kitty");
    const bool iterm = equals_ignore_case(term_program, "iTerm.app");
    const bool wezterm = equals_ignore_case(term_program, "WezTerm");
    const bool vscode = equals_ignore_case(term_program, "vscode");
    const bool ghostty = equals_ignore_case(term_program, "ghostty");
    const bool warp = equals_ignore_case(term_program, "WarpTerminal");
    const bool alacritty = equals_ignore_case(term_program, "alacritty") || contains_ignore_case(term, "alacritty");
    const bool foot = equals_ignore_case(term_program, "foot") || contains_ignore_case(term, "foot");
    const bool windows_terminal = !wt_session.empty();

    profile.tmux = !tmux.empty();
    profile.truecolor = contains_ignore_case(colorterm, "truecolor") || contains_ignore_case(colorterm, "24bit") ||
                        iterm || wezterm || vscode || ghostty || warp || windows_terminal;
    profile.ansi256 = contains_ignore_case(term, "256color");
    profile.unicode = !term.empty() && !dumb;
    profile.kitty_keyboard = kitty;
    profile.focus_events = !dumb && !cons25;
    profile.synchronized_output =
            !profile.tmux && (iterm || wezterm || vscode || ghostty || windows_terminal || kitty || alacritty || foot);
    profile.kitty_graphics = !kitty_window_id.empty();
    profile.sixel = contains_ignore_case(term, "sixel");
    return profile;
}

string build_osc11_query() { return "\033]11;?\a"; }

optional<_NEFORCE color> parse_osc11_reply(const string_view reply) {
    string_view text = reply.trim();
    if (text.size() < 5 || text[0] != '\033' || text[1] != ']') {
        return none;
    }
    text.remove_prefix(2);
    if (!text.starts_with("11;")) {
        return none;
    }
    text.remove_prefix(3);

    if (!text.empty() && text.back() == '\a') {
        text.remove_suffix(1);
    } else if (text.size() >= 2 && text[text.size() - 2] == '\033' && text.back() == '\\') {
        text.remove_suffix(2);
    }

    if (starts_with_ignore_case(text, "rgb:")) {
        return parse_rgb_reply(text.tail(4));
    }
    return parse_hash_reply(text);
}

bool is_light_background(const _NEFORCE color background) noexcept { return background.gray_value() > 140.0; }

string build_notification(const string_view title, const string_view body, const notify_method method) {
    switch (method) {
        case notify_method::osc9:
        case notify_method::auto_:
            return build_osc9_notification(title, body);
        case notify_method::osc777: {
            string result = "\033]777;notify;";
            result.append(title);
            result.append(";");
            result.append(body);
            result.append("\a");
            return result;
        }
        case notify_method::osc99: {
            string result = "\033]99;;";
            result.append(title);
            result.append("\a\033]99;;");
            result.append(body);
            result.append("\a");
            return result;
        }
        case notify_method::bel:
            return string("\a");
        case notify_method::windows:
            // Native Windows toasts require a platform notification backend;
            // this pure sequence builder falls back to BEL.
        default:
            return string("\a");
    }
}

string build_cursor_shape(const cursor_shape shape) {
    auto value = static_cast<uint8_t>(shape);
    if (value > static_cast<uint8_t>(cursor_shape::bar)) {
        value = 0;
    }
    string result = "\033[";
    result.push_back(static_cast<char>('0' + value));
    result.append(" q");
    return result;
}

string build_focus_events(const bool enable) { return enable ? string("\033[?1004h") : string("\033[?1004l"); }

string build_kitty_keyboard(const bool enable, const int flags) {
    if (!enable) {
        return "\033[<u";
    }
    string result = "\033[>";
    result.append(_NEFORCE to_string(flags < 0 ? 0 : flags));
    result.append("u");
    return result;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
