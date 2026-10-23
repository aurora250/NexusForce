#include <NeForce/tui/input_parser.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    constexpr size_t max_sequence_size = 32;
    constexpr size_t max_paste_size = 1024ULL * 1024ULL;
    constexpr string_view paste_end = "\033[201~";

    key_modifier modifier_of(const int param) {
        if (param < 1) {
            return key_modifier::none;
        }
        const int bits = param - 1;
        auto mods = key_modifier::none;
        if ((bits & 0x01) != 0) {
            mods = mods | key_modifier::shift;
        }
        if ((bits & 0x02) != 0) {
            mods = mods | key_modifier::alt;
        }
        if ((bits & 0x04) != 0) {
            mods = mods | key_modifier::ctrl;
        }
        return mods;
    }

    vector<int> split_params(const string_view params) {
        vector<int> values;
        int current = 0;
        bool has_digit = false;
        for (size_t i = 0; i < params.size(); ++i) {
            const char c = params[i];
            if (c == ';') {
                values.push_back(has_digit ? current : 0);
                current = 0;
                has_digit = false;
                continue;
            }
            if (c >= '0' && c <= '9') {
                current = current * 10 + (c - '0');
                has_digit = true;
            }
        }
        values.push_back(has_digit ? current : 0);
        return values;
    }

    key_event control_key(const uint8_t byte) {
        key_event ke;
        switch (byte) {
            case '\r':
            case '\n': {
                ke.key = key_event::type::enter;
                return ke;
            }
            case '\t': {
                ke.key = key_event::type::tab;
                return ke;
            }
            case 0x7F:
            case '\b': {
                ke.key = key_event::type::backspace;
                return ke;
            }
            default: {
                break;
            }
        }

        ke.key = key_event::type::printable;
        ke.mods = key_modifier::ctrl;
        if (byte == 0) {
            ke.cp = codepoint{static_cast<char32_t>(0x40)};
        } else if (byte <= 26) {
            ke.cp = codepoint{static_cast<char32_t>(byte) + 0x60};
        } else {
            ke.cp = codepoint{static_cast<char32_t>(byte) + 0x40};
        }
        return ke;
    }

    key_event printable_key(const char32_t cp) {
        key_event ke;
        ke.key = key_event::type::printable;
        ke.cp = codepoint{cp};
        return ke;
    }

    key_event function_key(const int number) {
        key_event ke;
        if (number >= 1 && number <= 24) {
            ke.key = static_cast<key_event::type>(static_cast<int>(key_event::type::F1) + number - 1);
        }
        return ke;
    }

    vector<string_view> split_view(const string_view text, const char sep) {
        vector<string_view> parts;
        size_t start = 0;
        for (size_t i = 0; i <= text.size(); ++i) {
            if (i == text.size() || text[i] == sep) {
                parts.push_back(text.substr(start, i - start));
                start = i + 1;
            }
        }
        return parts;
    }

    int parse_int(const string_view text, const int fallback) noexcept {
        if (text.empty()) {
            return fallback;
        }
        int value = 0;
        for (const char c: text) {
            if (c < '0' || c > '9') {
                return fallback;
            }
            value = value * 10 + (c - '0');
            if (value > 10000000) {
                return fallback;
            }
        }
        return value;
    }

    void map_functional_key(const int code, key_event& ke) noexcept {
        switch (code) {
            case 57344:
                ke.key = key_event::type::escape;
                return;
            case 57345:
                ke.key = key_event::type::enter;
                return;
            case 57346:
                ke.key = key_event::type::tab;
                return;
            case 57347:
                ke.key = key_event::type::backspace;
                return;
            case 57348:
                ke.key = key_event::type::insert;
                return;
            case 57349:
                ke.key = key_event::type::delete_;
                return;
            case 57350:
                ke.key = key_event::type::left;
                return;
            case 57351:
                ke.key = key_event::type::right;
                return;
            case 57352:
                ke.key = key_event::type::up;
                return;
            case 57353:
                ke.key = key_event::type::down;
                return;
            case 57354:
                ke.key = key_event::type::page_up;
                return;
            case 57355:
                ke.key = key_event::type::page_down;
                return;
            case 57356:
                ke.key = key_event::type::home;
                return;
            case 57357:
                ke.key = key_event::type::end;
                return;
            default:
                break;
        }
        if (code >= 57364 && code <= 57387) {
            const int index = code - 57364;
            const auto type = static_cast<key_event::type>(static_cast<int>(key_event::type::F1) + index);
            ke.key = type;
        }
    }

    void map_kitty_code(const int code, key_event& ke) noexcept {
        map_functional_key(code, ke);
        if (ke.key != key_event::type::unknown) {
            return;
        }
        switch (code) {
            case 27:
                ke.key = key_event::type::escape;
                return;
            case 13:
                ke.key = key_event::type::enter;
                return;
            case 9:
                ke.key = key_event::type::tab;
                return;
            case 127:
                ke.key = key_event::type::backspace;
                return;
            default:
                break;
        }
        if (code >= 32 && code <= 0x10FFFF) {
            ke.key = key_event::type::printable;
            ke.cp = codepoint{static_cast<char32_t>(code)};
        }
    }
} // namespace


void input_parser::reset() {
    state_ = state::ground;
    seq_.clear();
    utf8_.clear();
    paste_.clear();
    osc_.clear();
    alt_ = key_modifier::none;
}

void input_parser::emit_key(parsed_input& out, const key_event& ke) {
    out.type = parsed_input::kind::key;
    out.key = ke;
    state_ = state::ground;
    seq_.clear();
    utf8_.clear();
    alt_ = key_modifier::none;
}

bool input_parser::feed(const byte_t byte, parsed_input& out) {
    out.type = parsed_input::kind::none;
    out.paste_text.clear();

    switch (state_) {
        case state::paste: {
            return feed_paste(byte, out);
        }
        case state::osc: {
            return feed_osc(byte, out);
        }
        case state::utf8: {
            return feed_utf8(byte, out);
        }
        case state::csi: {
            return feed_csi(byte, out);
        }
        case state::ss3: {
            return feed_ss3(byte, out);
        }
        case state::escape: {
            return feed_escape(byte, out);
        }
        case state::ground:
        default: {
            return feed_ground(byte, out);
        }
    }
}

bool input_parser::flush(parsed_input& out) {
    out.type = parsed_input::kind::none;
    out.paste_text.clear();

    if (state_ == state::escape) {
        key_event ke;
        ke.key = key_event::type::escape;
        emit_key(out, ke);
        return true;
    }
    if (state_ == state::utf8 || state_ == state::csi || state_ == state::ss3 || state_ == state::osc) {
        reset();
    }
    return false;
}

bool input_parser::feed_ground(const byte_t byte, parsed_input& out) {
    if (byte == 0x1B) {
        state_ = state::escape;
        seq_.clear();
        return false;
    }
    if (byte >= 0x80) {
        state_ = state::utf8;
        utf8_.clear();
        utf8_ += static_cast<char>(byte);
        return complete_utf8(out);
    }
    if (byte < 0x20 || byte == 0x7F) {
        emit_key(out, control_key(byte));
        return true;
    }

    emit_key(out, printable_key(static_cast<char32_t>(byte)));
    return true;
}

bool input_parser::feed_escape(const byte_t byte, parsed_input& out) {
    if (byte == '[') {
        state_ = state::csi;
        seq_.clear();
        return false;
    }
    if (byte == ']') {
        state_ = state::osc;
        osc_.clear();
        osc_ += '\033';
        osc_ += ']';
        return false;
    }
    if (byte == 'O') {
        state_ = state::ss3;
        seq_.clear();
        return false;
    }
    if (byte == 0x1B) {
        key_event ke;
        ke.key = key_event::type::escape;
        out.type = parsed_input::kind::key;
        out.key = ke;
        seq_.clear();
        alt_ = key_modifier::none;
        return true;
    }
    if (byte >= 0x80) {
        state_ = state::utf8;
        alt_ = key_modifier::alt;
        utf8_.clear();
        utf8_ += static_cast<char>(byte);
        return complete_utf8(out);
    }

    if (byte < 0x20 || byte == 0x7F) {
        key_event ke = control_key(byte);
        ke.mods = ke.mods | key_modifier::alt;
        emit_key(out, ke);
        return true;
    }

    key_event ke = printable_key(static_cast<char32_t>(byte));
    ke.mods = key_modifier::alt;
    emit_key(out, ke);
    return true;
}

bool input_parser::feed_csi(const byte_t byte, parsed_input& out) {
    if (byte == 0x1B) {
        state_ = state::escape;
        seq_.clear();
        return false;
    }
    if (byte >= 0x40 && byte <= 0x7E) {
        seq_ += static_cast<char>(byte);
        state_ = state::ground;
        return finish_csi(out);
    }
    seq_ += static_cast<char>(byte);
    if (seq_.size() > max_sequence_size) {
        reset();
    }
    return false;
}

bool input_parser::finish_csi(parsed_input& out) {
    const string params = seq_;
    const char final = params.empty() ? '\0' : params.back();
    string_view body = params.view();
    if (!body.empty()) {
        body = body.substr(0, body.size() - 1);
    }
    seq_.clear();

    if (body.empty() && final == 'I') {
        out.type = parsed_input::kind::focus;
        out.focused = true;
        return true;
    }
    if (body.empty() && final == 'O') {
        out.type = parsed_input::kind::focus;
        out.focused = false;
        return true;
    }
    if (final == 'u') {
        // Kitty keyboard protocol: code[:alternates][;mods[:event]][;text].
        if (!body.empty() && (body[0] == '?' || body[0] == '>' || body[0] == '<')) {
            return false;
        }
        const vector<string_view> parts = split_view(body, ';');
        const string_view code_part = parts.empty() ? string_view{} : parts[0];
        const size_t colon = code_part.find(string_view{":"});
        const int code = parse_int(colon == string_view::npos ? code_part : code_part.substr(0, colon), -1);
        if (code < 0) {
            return false;
        }
        int modifiers = 1;
        int event_type = 1;
        if (parts.size() > 1) {
            const vector<string_view> mod_part = split_view(parts[1], ':');
            modifiers = parse_int(mod_part.empty() ? string_view{} : mod_part[0], 1);
            if (mod_part.size() > 1) {
                event_type = parse_int(mod_part[1], 1);
            }
        }
        key_event ke;
        map_kitty_code(code, ke);
        if (ke.key == key_event::type::unknown) {
            return false;
        }
        ke.mods = modifier_of(modifiers);
        if (ke.key == key_event::type::tab && (ke.mods & key_modifier::shift) != key_modifier::none) {
            ke.key = key_event::type::tab_reverse;
        }
        if (event_type == 2) {
            ke.action = key_action::repeat;
        } else if (event_type == 3) {
            ke.action = key_action::release;
        }
        if (colon != string_view::npos) {
            const string_view alternate = code_part.substr(colon + 1);
            const size_t next = alternate.find(string_view{":"});
            const int base = parse_int(next == string_view::npos ? alternate : alternate.substr(0, next), 0);
            if (base > 0 && base <= 0x10FFFF) {
                ke.base_cp = codepoint{static_cast<char32_t>(base)};
            }
        }
        emit_key(out, ke);
        return true;
    }

    if (!body.empty() && body[0] == '<') {
        if (final == 'M') {
            return parse_sgr_mouse(body, out, false);
        }
        if (final == 'm') {
            return parse_sgr_mouse(body, out, true);
        }
    }

    const auto values = split_params(body);
    const int primary = values.empty() ? 0 : values[0];
    const key_modifier mods = modifier_of(values.size() > 1 ? values[1] : 1);

    key_event ke;
    switch (final) {
        case 'A': {
            ke.key = key_event::type::up;
            break;
        }
        case 'B': {
            ke.key = key_event::type::down;
            break;
        }
        case 'C': {
            ke.key = key_event::type::right;
            break;
        }
        case 'D': {
            ke.key = key_event::type::left;
            break;
        }
        case 'H': {
            ke.key = key_event::type::home;
            break;
        }
        case 'F': {
            ke.key = key_event::type::end;
            break;
        }
        case 'Z': {
            ke.key = key_event::type::tab_reverse;
            ke.mods = key_modifier::shift;
            break;
        }
        case 'P': {
            ke = function_key(1);
            break;
        }
        case 'Q': {
            ke = function_key(2);
            break;
        }
        case 'R': {
            ke = function_key(3);
            break;
        }
        case 'S': {
            ke = function_key(4);
            break;
        }
        case '~': {
            if (primary == 27 && values.size() >= 3) {
                key_event modified;
                map_kitty_code(values[2], modified);
                if (modified.key == key_event::type::unknown) {
                    return false;
                }
                modified.mods = modifier_of(values[1]);
                emit_key(out, modified);
                return true;
            }
            switch (primary) {
                case 1:
                case 7: {
                    ke.key = key_event::type::home;
                    break;
                }
                case 2: {
                    ke.key = key_event::type::insert;
                    break;
                }
                case 3: {
                    ke.key = key_event::type::delete_;
                    break;
                }
                case 4:
                case 8: {
                    ke.key = key_event::type::end;
                    break;
                }
                case 5: {
                    ke.key = key_event::type::page_up;
                    break;
                }
                case 6: {
                    ke.key = key_event::type::page_down;
                    break;
                }
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                case 17:
                case 18:
                case 19:
                case 20:
                case 21:
                case 23:
                case 24: {
                    const int order[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
                    const int codes[] = {11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 23, 24};
                    for (int i = 0; i < 12; ++i) {
                        if (codes[i] == primary) {
                            ke = function_key(order[i]);
                            break;
                        }
                    }
                    break;
                }
                case 200: {
                    state_ = state::paste;
                    paste_.clear();
                    return false;
                }
                default: {
                    return false;
                }
            }
            break;
        }
        default: {
            return false;
        }
    }

    if (ke.key == key_event::type::unknown) {
        return false;
    }
    ke.mods = ke.mods | mods;
    emit_key(out, ke);
    return true;
}

bool input_parser::feed_ss3(const byte_t byte, parsed_input& out) {
    state_ = state::ground;
    key_event ke;
    switch (byte) {
        case 'A': {
            ke.key = key_event::type::up;
            break;
        }
        case 'B': {
            ke.key = key_event::type::down;
            break;
        }
        case 'C': {
            ke.key = key_event::type::right;
            break;
        }
        case 'D': {
            ke.key = key_event::type::left;
            break;
        }
        case 'H': {
            ke.key = key_event::type::home;
            break;
        }
        case 'F': {
            ke.key = key_event::type::end;
            break;
        }
        case 'P': {
            ke = function_key(1);
            break;
        }
        case 'Q': {
            ke = function_key(2);
            break;
        }
        case 'R': {
            ke = function_key(3);
            break;
        }
        case 'S': {
            ke = function_key(4);
            break;
        }
        default: {
            ke.key = key_event::type::escape;
            break;
        }
    }
    emit_key(out, ke);
    return true;
}

bool input_parser::feed_utf8(const byte_t byte, parsed_input& out) {
    if ((byte & 0xC0) != 0x80) {
        reset();
        return feed_ground(byte, out);
    }
    utf8_ += static_cast<char>(byte);
    return complete_utf8(out);
}

bool input_parser::feed_osc(const byte_t byte, parsed_input& out) {
    osc_ += static_cast<char>(byte);

    const bool terminated_by_bel = (byte == 0x07);
    const bool terminated_by_st = (osc_.size() >= 2 && osc_[osc_.size() - 2] == '\033' && byte == '\\');
    if (!terminated_by_bel && !terminated_by_st) {
        // Guards against a terminal that never terminates the sequence.
        if (osc_.size() > max_paste_size) {
            reset();
        }
        return false;
    }

    return finish_osc(out);
}

bool input_parser::finish_osc(parsed_input& out) {
    const string sequence = osc_;
    reset();

    const string decoded = sys_console::parse_clipboard_response(sequence.view());
    if (decoded.empty()) {
        // "?" answers (terminal refuses the query) and unrelated OSC sequences produce no event.
        return false;
    }

    out.type = parsed_input::kind::clipboard;
    out.paste_text = decoded;
    return true;
}

bool input_parser::complete_utf8(parsed_input& out) {
    if (utf8_.empty()) {
        reset();
        return false;
    }

    const auto first = static_cast<uint8_t>(utf8_[0]);
    size_t expected = 0;
    if ((first & 0xE0) == 0xC0) {
        expected = 2;
    } else if ((first & 0xF0) == 0xE0) {
        expected = 3;
    } else if ((first & 0xF8) == 0xF0) {
        expected = 4;
    } else {
        key_event ke = printable_key(static_cast<char32_t>(0xFFFD));
        ke.mods = alt_;
        emit_key(out, ke);
        return true;
    }

    if (utf8_.size() < expected) {
        return false;
    }
    if (utf8_.size() > expected) {
        reset();
        return false;
    }

    char32_t cp = 0;
    if (expected == 2) {
        cp = static_cast<char32_t>(first & 0x1F) << 6;
        cp |= static_cast<uint8_t>(utf8_[1]) & 0x3F;
    } else if (expected == 3) {
        cp = static_cast<char32_t>(first & 0x0F) << 12;
        cp |= (static_cast<uint8_t>(utf8_[1]) & 0x3F) << 6;
        cp |= static_cast<uint8_t>(utf8_[2]) & 0x3F;
    } else {
        cp = static_cast<char32_t>(first & 0x07) << 18;
        cp |= (static_cast<uint8_t>(utf8_[1]) & 0x3F) << 12;
        cp |= (static_cast<uint8_t>(utf8_[2]) & 0x3F) << 6;
        cp |= static_cast<uint8_t>(utf8_[3]) & 0x3F;
    }

    key_event ke = printable_key(cp);
    ke.mods = alt_;
    emit_key(out, ke);
    return true;
}

bool input_parser::feed_paste(const byte_t byte, parsed_input& out) {
    paste_ += static_cast<char>(byte);
    if (!paste_.ends_with(paste_end)) {
        if (paste_.size() > max_paste_size) {
            paste_.clear();
            state_ = state::ground;
        }
        return false;
    }

    constexpr size_t end_len = paste_end.size();
    string text = paste_.substr(0, paste_.size() - end_len);
    paste_.clear();
    state_ = state::ground;

    string normalized;
    normalized.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '\r') {
            normalized += '\n';
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
        } else {
            normalized += c;
        }
    }

    out.type = parsed_input::kind::paste;
    out.paste_text = move(normalized);
    return true;
}

bool input_parser::parse_sgr_mouse(const string_view body, parsed_input& out, const bool release) {
    seq_.clear();
    if (body.size() < 2) {
        return false;
    }

    const auto values = split_params(body.substr(1));
    if (values.size() < 3) {
        return false;
    }

    const int btn_value = values[0];
    mouse_event me;
    me.x = values[1] - 1;
    me.y = values[2] - 1;

    const int btn_code = btn_value & 0x03;
    const bool is_move = (btn_value & 0x20) != 0;
    const bool is_wheel = (btn_value & 0x40) != 0;

    if (is_wheel) {
        me.action = mouse_action::wheel;
        me.button = (btn_code == 0)   ? mouse_button::wheelup
                    : (btn_code == 1) ? mouse_button::wheeldown
                                      : mouse_button::none;
    } else if (release) {
        me.action = mouse_action::release;
        me.button = (btn_code == 0)   ? mouse_button::left
                    : (btn_code == 1) ? mouse_button::middle
                    : (btn_code == 2) ? mouse_button::right
                                      : mouse_button::none;
    } else if (is_move) {
        me.action = mouse_action::move;
        me.button = (btn_code == 0)   ? mouse_button::left
                    : (btn_code == 1) ? mouse_button::middle
                    : (btn_code == 2) ? mouse_button::right
                                      : mouse_button::none;
    } else {
        me.action = mouse_action::press;
        me.button = (btn_code == 0)   ? mouse_button::left
                    : (btn_code == 1) ? mouse_button::middle
                    : (btn_code == 2) ? mouse_button::right
                                      : mouse_button::none;
    }

    if ((btn_value & 0x04) != 0) {
        me.mods = me.mods | key_modifier::shift;
    }
    if ((btn_value & 0x08) != 0) {
        me.mods = me.mods | key_modifier::alt;
    }
    if ((btn_value & 0x10) != 0) {
        me.mods = me.mods | key_modifier::ctrl;
    }

    out.type = parsed_input::kind::mouse;
    out.mouse = me;
    return true;
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
