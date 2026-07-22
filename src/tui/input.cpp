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
    volatile ::sig_atomic_t g_winsizeChanged = 0;

    void sigwinchHandler(int /*unused*/) {
        g_winsizeChanged = 1;
        ::signal(SIGWINCH, sigwinchHandler);
    }
#else
    ::DWORD WINAPI inputThreadProc(::LPVOID lpParam) {
        auto* self = static_cast<InputDriver*>(lpParam);
        ::HANDLE hStdin = static_cast<::HANDLE>(self->stdinHandle_);
        unsigned char buf[256];
        ::DWORD nread = 0;

        while (self->listening_) {
            if (::ReadFile(hStdin, buf, sizeof(buf), &nread, nullptr) == 0) {
                break;
            }
            if (nread == 0) {
                continue;
            }
            self->processAccumulatedBytes(buf, nread);
            self->strand_.post([self] {
                if (self->listening_) {
                    self->reconiler_.flush();
                }
            });
        }
        return 0;
    }
#endif
} // namespace

bool InputDriver::checkResizeFlag() {
#ifdef NEFORCE_PLATFORM_LINUX
    if (g_winsizeChanged != 0) {
        g_winsizeChanged = 0;
        return true;
    }
#endif
    return false;
}

InputDriver::InputDriver(io_context& ctx, Reconciler& r, strand& s) :
ctx_(ctx),
reconiler_(r),
strand_(s),
console_(sys_console::instance()),
oldTermios_() {}

InputDriver::~InputDriver() { stop(); }

void InputDriver::start() {
    if (listening_) {
        return;
    }

#ifdef NEFORCE_PLATFORM_LINUX
    ::tcgetattr(STDIN_FILENO, &oldTermios_);
    termSaved_ = true;
    ::termios raw = oldTermios_;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    ::tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    ::signal(SIGWINCH, sigwinchHandler);

    ctx_.add_fd(
            STDIN_FILENO, epoll_in, [this](int fd, uint32_t events, error_code ec) { drainStdin(fd, events, ec); },
            false);

#else
    stdinHandle_ = ::GetStdHandle(STD_INPUT_HANDLE);

    ::GetConsoleMode(stdinHandle_, &oldInMode_);
    termSaved_ = true;

    ::DWORD newInMode = ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
    ::SetConsoleMode(stdinHandle_, newInMode);

    inputThread_ = ::CreateThread(nullptr, 0, inputThreadProc, this, 0, nullptr);
#endif

    listening_ = true;
}

void InputDriver::stop() {
    if (!listening_) {
        return;
    }
    listening_ = false;

#ifdef NEFORCE_PLATFORM_LINUX
    if (termSaved_) {
        ::tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios_);
        termSaved_ = false;
    }
#else
    if (stdinHandle_ != nullptr) {
        ::CancelIoEx(stdinHandle_, nullptr);
    }

    if (inputThread_ != nullptr) {
        ::WaitForSingleObject(static_cast<::HANDLE>(inputThread_), 5000);
        ::CloseHandle(static_cast<::HANDLE>(inputThread_));
        inputThread_ = nullptr;
    }

    if (termSaved_) {
        ::SetConsoleMode(stdinHandle_, oldInMode_);
        termSaved_ = false;
    }
#endif
}

#ifdef NEFORCE_PLATFORM_LINUX
void InputDriver::drainStdin(int /*fd*/, uint32_t /*events*/, error_code /*ec*/) {
    while (true) {
        unsigned char ch = 0;
        const ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n <= 0) {
            break;
        }
        processByte(static_cast<byte_t>(ch));
    }

    if (escActive_ && !accum_.empty() && accum_[0] == '\x1B' && accum_.size() == 1) {
        KeyEvent ke;
        ke.key = KeyEvent::Key::Escape;
        dispatchKey(ke);
    }

    strand_.post([this] { reconiler_.flush(); });
}
#endif

void InputDriver::processAccumulatedBytes(const byte_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        processByte(data[i]);
    }
    if (escActive_ && !accum_.empty() && accum_[0] == '\x1B' && accum_.size() == 1) {
        KeyEvent ke;
        ke.key = KeyEvent::Key::Escape;
        dispatchKey(ke);
    }
}

void InputDriver::processByte(byte_t byte) {
    if (mouseActive_) {
        accum_ += static_cast<char>(byte);
        if (parseMouseSequence()) {
            return;
        }
        return;
    }

    if (escActive_) {
        accum_ += static_cast<char>(byte);
        parseEscapeSequence();
        return;
    }

    if (byte == 0x1B) {
        accum_.clear();
        accum_ += static_cast<char>(byte);
        escActive_ = true;
        return;
    }

    if (byte >= 0x80) {
        accum_.clear();
        accum_ += static_cast<char>(byte);
        parseUtf8Sequence();
        return;
    }

    KeyEvent ke;
    if (byte == '\r' || byte == '\n') {
        ke.key = KeyEvent::Key::Enter;
    } else if (byte == '\t') {
        ke.key = KeyEvent::Key::Tab;
    } else if (byte == 0x7F || byte == '\b') {
        ke.key = KeyEvent::Key::Backspace;
    } else if (byte < 0x20) {
        ke.key = KeyEvent::Key::Printable;
        ke.ch = static_cast<char32_t>(byte + 0x60);
        ke.mods = Modifier::Ctrl;
    } else {
        ke.key = KeyEvent::Key::Printable;
        ke.ch = static_cast<char32_t>(byte);
    }
    dispatchKey(ke);
}

bool InputDriver::parseEscapeSequence() {
    const auto& s = accum_;
    if (s.size() < 2) {
        return false;
    }

    if (s.size() >= 3 && s[0] == '\x1B' && s[1] == 'O') {
        KeyEvent ke;
        switch (s[2]) {
            case 'P':
                ke.key = KeyEvent::Key::F1;
                break;
            case 'Q':
                ke.key = KeyEvent::Key::F2;
                break;
            case 'R':
                ke.key = KeyEvent::Key::F3;
                break;
            case 'S':
                ke.key = KeyEvent::Key::F4;
                break;
            default:
                break;
        }
        if (ke.key != KeyEvent::Key::Unknown) {
            dispatchKey(ke);
            return true;
        }
        ke.key = KeyEvent::Key::Escape;
        dispatchKey(ke);
        return true;
    }

    if (s.size() >= 3 && s[0] == '\x1B' && s[1] == '[') {
        const char terminator = s.back();

        if (s.size() == 3 && terminator >= 'A' && terminator <= 'D') {
            KeyEvent ke;
            switch (terminator) {
                case 'A':
                    ke.key = KeyEvent::Key::Up;
                    break;
                case 'B':
                    ke.key = KeyEvent::Key::Down;
                    break;
                case 'C':
                    ke.key = KeyEvent::Key::Right;
                    break;
                case 'D':
                    ke.key = KeyEvent::Key::Left;
                    break;
                default:
                    break;
            }
            dispatchKey(ke);
            return true;
        }

        if (s.size() == 3 && terminator == 'H') {
            KeyEvent ke;
            ke.key = KeyEvent::Key::Home;
            dispatchKey(ke);
            return true;
        }
        if (s.size() == 3 && terminator == 'F') {
            KeyEvent ke;
            ke.key = KeyEvent::Key::End;
            dispatchKey(ke);
            return true;
        }

        if (terminator == '~') {
            const string_view numStr = s.view(2, s.size() - 3);
            KeyEvent ke;

            if (numStr == "1" || numStr == "7") {
                ke.key = KeyEvent::Key::Home;
            } else if (numStr == "2") {
                ke.key = KeyEvent::Key::Insert;
            } else if (numStr == "3") {
                ke.key = KeyEvent::Key::Delete;
            } else if (numStr == "4" || numStr == "8") {
                ke.key = KeyEvent::Key::End;
            } else if (numStr == "5") {
                ke.key = KeyEvent::Key::PageUp;
            } else if (numStr == "6") {
                ke.key = KeyEvent::Key::PageDown;
            } else if (numStr == "11") {
                ke.key = KeyEvent::Key::F1;
            } else if (numStr == "12") {
                ke.key = KeyEvent::Key::F2;
            } else if (numStr == "13") {
                ke.key = KeyEvent::Key::F3;
            } else if (numStr == "14") {
                ke.key = KeyEvent::Key::F4;
            } else if (numStr == "15") {
                ke.key = KeyEvent::Key::F5;
            } else if (numStr == "17") {
                ke.key = KeyEvent::Key::F6;
            } else if (numStr == "18") {
                ke.key = KeyEvent::Key::F7;
            } else if (numStr == "19") {
                ke.key = KeyEvent::Key::F8;
            } else if (numStr == "20") {
                ke.key = KeyEvent::Key::F9;
            } else if (numStr == "21") {
                ke.key = KeyEvent::Key::F10;
            } else if (numStr == "23") {
                ke.key = KeyEvent::Key::F11;
            } else if (numStr == "24") {
                ke.key = KeyEvent::Key::F12;
            }

            if (ke.key != KeyEvent::Key::Unknown) {
                dispatchKey(ke);
                return true;
            }

            ke.key = KeyEvent::Key::Escape;
            dispatchKey(ke);
            return true;
        }

        if (s.size() >= 6 && s[1] == '[' && s[2] == '<') {
            if (terminator == 'M' || terminator == 'm') {
                return parseMouseSequence();
            }
            if (s.size() < 16) {
                mouseActive_ = true;
                return false;
            }
        }
    }

    if (s.size() > 32) {
        KeyEvent ke;
        ke.key = KeyEvent::Key::Escape;
        dispatchKey(ke);
        return true;
    }

    return false;
}

bool InputDriver::parseUtf8Sequence() {
    if (accum_.empty()) {
        return false;
    }
    const auto first = static_cast<unsigned char>(accum_[0]);
    size_t expected = 0;

    if ((first & 0xE0) == 0xC0) {
        expected = 2;
    } else if ((first & 0xF0) == 0xE0) {
        expected = 3;
    } else if ((first & 0xF8) == 0xF0) {
        expected = 4;
    } else {
        KeyEvent ke;
        ke.key = KeyEvent::Key::Printable;
        ke.ch = 0xFFFD;
        dispatchKey(ke);
        return true;
    }

    if (accum_.size() < expected) {
        return false;
    }

    char32_t cp = 0;
    if (expected == 2) {
        cp = static_cast<char32_t>(first & 0x1F) << 6;
        cp |= static_cast<unsigned char>(accum_[1]) & 0x3F;
    } else if (expected == 3) {
        cp = static_cast<char32_t>(first & 0x0F) << 12;
        cp |= (static_cast<unsigned char>(accum_[1]) & 0x3F) << 6;
        cp |= static_cast<unsigned char>(accum_[2]) & 0x3F;
    } else if (expected == 4) {
        cp = static_cast<char32_t>(first & 0x07) << 18;
        cp |= (static_cast<unsigned char>(accum_[1]) & 0x3F) << 12;
        cp |= (static_cast<unsigned char>(accum_[2]) & 0x3F) << 6;
        cp |= static_cast<unsigned char>(accum_[3]) & 0x3F;
    }

    KeyEvent ke;
    ke.key = KeyEvent::Key::Printable;
    ke.ch = cp;
    dispatchKey(ke);
    return true;
}

bool InputDriver::parseMouseSequence() {
    const auto& s = accum_;
    if (s.size() < 6) {
        return false;
    }

    const size_t lt = s.find('<');
    const size_t lastM = s.rfind('M');
    const size_t lastm = s.rfind('m');

    if (lt == string::npos || (lastM == string::npos && lastm == string::npos)) {
        return s.size() < 16;
    }

    const size_t termPos = (lastM != string::npos) ? lastM : lastm;
    if (lt == string::npos || termPos == string::npos || termPos <= lt) {
        return false;
    }

    const bool isRelease = (lastm != string::npos && (lastM == string::npos || lastm > lastM));

    const string_view inner = s.view(lt + 1, termPos - lt - 1);
    const auto sc1 = inner.find(';');
    const auto sc2 = inner.find(';', sc1 + 1);

    if (sc1 == string::npos || sc2 == string::npos) {
        return false;
    }

    int btnVal = 0;
    int mx = 0;
    int my = 0;
    try {
        btnVal = to_int32(inner.view(0, sc1));
        mx = to_int32(inner.view(sc1 + 1, sc2 - sc1 - 1));
        my = to_int32(inner.view(sc2 + 1));
    } catch (...) {
        return false;
    }

    MouseEvent me;
    me.x = mx - 1;
    me.y = my - 1;

    const int btnCode = btnVal & 0x03;
    const bool move = (btnVal & 0x20) != 0;
    const bool wheel = (btnVal & 0x40) != 0;

    if (wheel) {
        me.action = MouseAction::Wheel;
        me.button = (btnCode == 0) ? MouseButton::WheelUp : (btnCode == 1) ? MouseButton::WheelDown : MouseButton::None;
    } else if (isRelease) {
        me.action = MouseAction::Release;
        switch (btnCode) {
            case 0:
                me.button = MouseButton::Left;
                break;
            case 1:
                me.button = MouseButton::Middle;
                break;
            case 2:
                me.button = MouseButton::Right;
                break;
            default:
                break;
        }
    } else if (move) {
        me.action = MouseAction::Move;
        me.button = MouseButton::None;
    } else {
        me.action = MouseAction::Press;
        switch (btnCode) {
            case 0:
                me.button = MouseButton::Left;
                break;
            case 1:
                me.button = MouseButton::Middle;
                break;
            case 2:
                me.button = MouseButton::Right;
                break;
            default:
                break;
        }
    }

    if ((btnVal & 0x04) != 0) {
        me.mods = me.mods | Modifier::Shift;
    }
    if ((btnVal & 0x08) != 0) {
        me.mods = me.mods | Modifier::Alt;
    }
    if ((btnVal & 0x10) != 0) {
        me.mods = me.mods | Modifier::Ctrl;
    }

    mouseActive_ = false;
    escActive_ = false;
    dispatchMouse(me);
    return true;
}

void InputDriver::dispatchKey(const KeyEvent& e) {
    accum_.clear();
    escActive_ = false;
    mouseActive_ = false;
    strand_.dispatch([this, e] { reconiler_.dispatchKey(e); });
}

void InputDriver::dispatchMouse(const MouseEvent& e) {
    accum_.clear();
    escActive_ = false;
    mouseActive_ = false;
    strand_.dispatch([this, e] { reconiler_.dispatchMouse(e); });
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
