#include <NeForce/tui/input.hpp>
#include <NeForce/tui/reconciler.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <termios.h>
#    include <unistd.h>
#    include <csignal>
#elif defined(NEFORCE_PLATFORM_WINDOWS)
#    ifdef NEFORCE_COMPILER_MSVC
#        include <consoleapi.h>
#        include <consoleapi2.h>
#    endif
#    include <WinBase.h>
#    include <WinNls.h>
#    include <conio.h>
#    include <synchapi.h>
#    include <processthreadsapi.h>
#endif
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
#ifdef NEFORCE_PLATFORM_LINUX
    volatile ::sig_atomic_t winsize_changed = 0;

    void sigwinch_handler(int /*unused*/) {
        winsize_changed = 1;
        ::signal(SIGWINCH, sigwinch_handler);
    }
#endif
} // namespace

bool input_driver::check_resize_flag() {
#ifdef NEFORCE_PLATFORM_LINUX
    if (winsize_changed != 0) {
        winsize_changed = 0;
        return true;
    }
#endif
    return false;
}

input_driver::input_driver(io_context& ctx, reconciler& r, strand& s) :
ctx_(ctx),
reconiler_(r),
strand_(s),
console_(sys_console::instance()) {}

input_driver::~input_driver() { stop(); }

void input_driver::start() {
    if (listening_) {
        return;
    }

#ifdef NEFORCE_PLATFORM_LINUX
    ::tcgetattr(STDIN_FILENO, &old_termios_);
    term_saved_ = true;
    ::termios raw = old_termios_;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    ::tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    ::signal(SIGWINCH, sigwinch_handler);

    ctx_.add_fd(
            STDIN_FILENO, epoll_in, [this](int fd, uint32_t events, error_code ec) { drain_stdin(fd, events, ec); },
            false);

#else
    stdin_handle_ = ::GetStdHandle(STD_INPUT_HANDLE);

    ::GetConsoleMode(stdin_handle_, &old_in_mode_);
    term_saved_ = true;

    constexpr ::DWORD new_in_mode = ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
    ::SetConsoleMode(stdin_handle_, new_in_mode);

    input_thread_ = ::CreateThread(nullptr, 0, input_thread_proc, this, 0, nullptr);
#endif

    listening_ = true;
}

void input_driver::stop() {
    if (!listening_) {
        return;
    }
    listening_ = false;

#ifdef NEFORCE_PLATFORM_LINUX
    if (term_saved_) {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &old_termios_);
        term_saved_ = false;
    }
#else
    if (stdin_handle_ != nullptr) {
        ::CancelIoEx(stdin_handle_, nullptr);
    }

    if (input_thread_ != nullptr) {
        ::WaitForSingleObject(input_thread_, 5000);
        ::CloseHandle(input_thread_);
        input_thread_ = nullptr;
    }

    if (term_saved_) {
        ::SetConsoleMode(stdin_handle_, old_in_mode_);
        term_saved_ = false;
    }
#endif
}

#ifdef NEFORCE_PLATFORM_LINUX
void input_driver::drain_stdin(int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
    while (true) {
        byte_t ch = 0;
        const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n <= 0) {
            break;
        }
        process_byte(static_cast<byte_t>(ch));
    }

    if (esc_active_ && !accum_.empty() && accum_[0] == '\x1B' && accum_.size() == 1) {
        key_event ke;
        ke.key = key_event::key::escape;
        dispatch_key(ke);
    }

    strand_.post([this] { reconiler_.flush(); });
}
#else
unsigned long WINAPI input_driver::input_thread_proc(void* selfp) {
    auto* self = static_cast<input_driver*>(selfp);
    byte_t buf[256];
    ::DWORD nread = 0;

    while (self->listening_) {
        if (::ReadFile(self->stdin_handle_, buf, sizeof(buf), &nread, nullptr) == 0) {
            break;
        }
        if (nread == 0) {
            continue;
        }
        self->process_accumulated_bytes(buf, nread);
        self->strand_.post([self] {
            if (self->listening_) {
                self->reconiler_.flush();
            }
        });
    }
    return 0;
}
#endif

void input_driver::process_accumulated_bytes(const byte_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        process_byte(data[i]);
    }
    if (esc_active_ && !accum_.empty() && accum_[0] == '\x1B' && accum_.size() == 1) {
        key_event ke;
        ke.key = key_event::type::escape;
        dispatch_key(ke);
    }
}

void input_driver::process_byte(byte_t byte) {
    if (mouse_active_) {
        accum_ += static_cast<char>(byte);
        if (parse_mouse_sequence()) {
            return;
        }
        return;
    }

    if (esc_active_) {
        accum_ += static_cast<char>(byte);
        parse_escape_sequence();
        return;
    }

    if (byte == 0x1B) {
        accum_.clear();
        accum_ += static_cast<char>(byte);
        esc_active_ = true;
        return;
    }

    if (byte >= 0x80) {
        if (!utf8_active_) {
            accum_.clear();
            utf8_active_ = true;
        }
        accum_ += static_cast<char>(byte);
        parse_utf8_sequence();
        return;
    }

    utf8_active_ = false;

    key_event ke;
    if (byte == '\r' || byte == '\n') {
        ke.key = key_event::type::enter;
    } else if (byte == '\t') {
        ke.key = key_event::type::tab;
    } else if (byte == 0x7F || byte == '\b') {
        ke.key = key_event::type::backspace;
    } else if (byte < 0x20) {
        ke.key = key_event::type::printable;
        ke.cp = codepoint{byte + 0x60};
        ke.mods = key_modifier::ctrl;
    } else {
        ke.key = key_event::type::printable;
        ke.cp = codepoint{byte};
    }
    dispatch_key(ke);
}

bool input_driver::parse_escape_sequence() {
    const auto& s = accum_;
    if (s.size() < 2) {
        return false;
    }

    if (s.size() >= 3 && s[0] == '\x1B' && s[1] == 'O') {
        key_event ke;
        switch (s[2]) {
            case 'P':
                ke.key = key_event::type::F1;
                break;
            case 'Q':
                ke.key = key_event::type::F2;
                break;
            case 'R':
                ke.key = key_event::type::F3;
                break;
            case 'S':
                ke.key = key_event::type::F4;
                break;
            default:
                break;
        }
        if (ke.key != key_event::type::unknown) {
            dispatch_key(ke);
            return true;
        }
        ke.key = key_event::type::escape;
        dispatch_key(ke);
        return true;
    }

    if (s.size() >= 3 && s[0] == '\x1B' && s[1] == '[') {
        const char terminator = s.back();

        if (s.size() == 3 && terminator >= 'A' && terminator <= 'D') {
            key_event ke;
            switch (terminator) {
                case 'A':
                    ke.key = key_event::type::up;
                    break;
                case 'B':
                    ke.key = key_event::type::down;
                    break;
                case 'C':
                    ke.key = key_event::type::right;
                    break;
                case 'D':
                    ke.key = key_event::type::left;
                    break;
                default:
                    break;
            }
            dispatch_key(ke);
            return true;
        }

        if (s.size() == 3 && terminator == 'H') {
            key_event ke;
            ke.key = key_event::type::home;
            dispatch_key(ke);
            return true;
        }
        if (s.size() == 3 && terminator == 'F') {
            key_event ke;
            ke.key = key_event::type::end;
            dispatch_key(ke);
            return true;
        }

        if (s.size() == 3 && terminator == 'Z') {
            key_event ke;
            ke.key = key_event::type::tab_reverse;
            dispatch_key(ke);
            return true;
        }

        if (terminator == '~') {
            const string_view num = s.view(2, s.size() - 3);
            key_event ke;

            if (num == "1" || num == "7") {
                ke.key = key_event::type::home;
            } else if (num == "2") {
                ke.key = key_event::type::insert;
            } else if (num == "3") {
                ke.key = key_event::type::delete_;
            } else if (num == "4" || num == "8") {
                ke.key = key_event::type::end;
            } else if (num == "5") {
                ke.key = key_event::type::page_up;
            } else if (num == "6") {
                ke.key = key_event::type::page_down;
            } else if (num == "11") {
                ke.key = key_event::type::F1;
            } else if (num == "12") {
                ke.key = key_event::type::F2;
            } else if (num == "13") {
                ke.key = key_event::type::F3;
            } else if (num == "14") {
                ke.key = key_event::type::F4;
            } else if (num == "15") {
                ke.key = key_event::type::F5;
            } else if (num == "17") {
                ke.key = key_event::type::F6;
            } else if (num == "18") {
                ke.key = key_event::type::F7;
            } else if (num == "19") {
                ke.key = key_event::type::F8;
            } else if (num == "20") {
                ke.key = key_event::type::F9;
            } else if (num == "21") {
                ke.key = key_event::type::F10;
            } else if (num == "23") {
                ke.key = key_event::type::F11;
            } else if (num == "24") {
                ke.key = key_event::type::F12;
            }

            if (ke.key != key_event::type::unknown) {
                dispatch_key(ke);
                return true;
            }

            ke.key = key_event::type::escape;
            dispatch_key(ke);
            return true;
        }

        if (s.size() >= 6 && s[1] == '[' && s[2] == '<') {
            if (terminator == 'M' || terminator == 'm') {
                return parse_mouse_sequence();
            }
            if (s.size() < 16) {
                mouse_active_ = true;
                return false;
            }
        }
    }

    if (s.size() > 32) {
        key_event ke;
        ke.key = key_event::type::escape;
        dispatch_key(ke);
        return true;
    }

    return false;
}

bool input_driver::parse_utf8_sequence() {
    if (accum_.empty()) {
        return false;
    }
    const auto first = static_cast<byte_t>(accum_[0]);
    size_t expected = 0;

    if ((first & 0xE0) == 0xC0) {
        expected = 2;
    } else if ((first & 0xF0) == 0xE0) {
        expected = 3;
    } else if ((first & 0xF8) == 0xF0) {
        expected = 4;
    } else {
        key_event ke;
        ke.key = key_event::type::printable;
        ke.cp = codepoint{0xFFFD};
        dispatch_key(ke);
        return true;
    }

    if (accum_.size() < expected) {
        return false;
    }

    char32_t cp = 0;
    if (expected == 2) {
        cp = static_cast<char32_t>(first & 0x1F) << 6;
        cp |= static_cast<byte_t>(accum_[1]) & 0x3F;
    } else if (expected == 3) {
        cp = static_cast<char32_t>(first & 0x0F) << 12;
        cp |= (static_cast<byte_t>(accum_[1]) & 0x3F) << 6;
        cp |= static_cast<byte_t>(accum_[2]) & 0x3F;
    } else if (expected == 4) {
        cp = static_cast<char32_t>(first & 0x07) << 18;
        cp |= (static_cast<byte_t>(accum_[1]) & 0x3F) << 12;
        cp |= (static_cast<byte_t>(accum_[2]) & 0x3F) << 6;
        cp |= static_cast<byte_t>(accum_[3]) & 0x3F;
    }

    key_event ke;
    ke.key = key_event::type::printable;
    ke.cp = codepoint{cp};
    dispatch_key(ke);
    return true;
}

bool input_driver::parse_mouse_sequence() {
    const auto& s = accum_;
    if (s.size() < 6) {
        return false;
    }

    const size_t lt = s.find('<');
    const size_t last_M = s.rfind('M');
    const size_t last_m = s.rfind('m');

    if (lt == string::npos || (last_M == string::npos && last_m == string::npos)) {
        return s.size() < 16;
    }

    const size_t term_pos = (last_M != string::npos) ? last_M : last_m;
    if (lt == string::npos || term_pos == string::npos || term_pos <= lt) {
        return false;
    }

    const bool is_release = (last_m != string::npos && (last_M == string::npos || last_m > last_M));

    const string_view inner = s.view(lt + 1, term_pos - lt - 1);
    const auto sc1 = inner.find(';');
    const auto sc2 = inner.find(';', sc1 + 1);

    if (sc1 == string::npos || sc2 == string::npos) {
        return false;
    }

    int btn_value = 0;
    int mx = 0;
    int my = 0;
    try {
        btn_value = to_int32(inner.view(0, sc1));
        mx = to_int32(inner.view(sc1 + 1, sc2 - sc1 - 1));
        my = to_int32(inner.view(sc2 + 1));
    } catch (...) {
        return false;
    }

    mouse_event me;
    me.x = mx - 1;
    me.y = my - 1;

    const int btn_code = btn_value & 0x03;
    const bool move = (btn_value & 0x20) != 0;
    const bool wheel = (btn_value & 0x40) != 0;

    if (wheel) {
        me.action = mouse_action::wheel;
        me.button = (btn_code == 0)   ? mouse_button::wheelup
                    : (btn_code == 1) ? mouse_button::wheeldown
                                     : mouse_button::none;
    } else if (is_release) {
        me.action = mouse_action::release;
        switch (btn_code) {
            case 0:
                me.button = mouse_button::left;
                break;
            case 1:
                me.button = mouse_button::middle;
                break;
            case 2:
                me.button = mouse_button::right;
                break;
            default:
                break;
        }
    } else if (move) {
        me.action = mouse_action::move;
        me.button = mouse_button::none;
    } else {
        me.action = mouse_action::press;
        switch (btn_code) {
            case 0:
                me.button = mouse_button::left;
                break;
            case 1:
                me.button = mouse_button::middle;
                break;
            case 2:
                me.button = mouse_button::right;
                break;
            default:
                break;
        }
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

    mouse_active_ = false;
    esc_active_ = false;
    dispatch_mouse(me);
    return true;
}

void input_driver::dispatch_key(const key_event& e) {
    accum_.clear();
    esc_active_ = false;
    mouse_active_ = false;
    utf8_active_ = false;
    strand_.dispatch([this, e] { reconiler_.dispatch_key(e); });
}

void input_driver::dispatch_mouse(const mouse_event& e) {
    accum_.clear();
    esc_active_ = false;
    mouse_active_ = false;
    utf8_active_ = false;
    strand_.dispatch([this, e] { reconiler_.dispatch_mouse(e); });
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
