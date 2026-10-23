#include <NeForce/tui/input.hpp>
#include <NeForce/tui/reconciler.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <termios.h>
#    include <unistd.h>
#    include <csignal>
#endif
#ifdef NEFORCE_PLATFORM_WINDOWS
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

    void deliver_guarded(strand& s, function<void()> handler) {
        auto holder = make_shared<function<void()>>(_NEFORCE move(handler));
        s.dispatch([holder] {
            try {
                (*holder)();
                // NOLINTNEXTLINE(bugprone-empty-catch)
            } catch (const exception& e) {
                NEFORCE_REPORT_EXCEPTION(e);
            }
        });
    }
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
    ::cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    ::tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    ::signal(SIGWINCH, sigwinch_handler);

    ctx_.add_fd(
            STDIN_FILENO, epoll_event::in,
            [this](int fd, epoll_event events, error_code ec) { drain_stdin(fd, events, ec); }, false);

#else
    stdin_handle_ = ::GetStdHandle(STD_INPUT_HANDLE);

    ::GetConsoleMode(stdin_handle_, &old_in_mode_);
    term_saved_ = true;

    constexpr ::DWORD new_in_mode = ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
    ::SetConsoleMode(stdin_handle_, new_in_mode);

    input_thread_ = ::CreateThread(nullptr, 0, input_thread_proc, this, 0, nullptr);
#endif

    parser_.reset();
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
void input_driver::drain_stdin(int /*fd*/, epoll_event /*events*/, error_code /*ec*/) {
    byte_t buffer[256];
    while (true) {
        const ssize_t n = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (n <= 0) {
            break;
        }
        process_accumulated_bytes(buffer, static_cast<size_t>(n));
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

void input_driver::process_accumulated_bytes(const byte_t* data, const size_t len) {
    for (size_t i = 0; i < len; ++i) {
        process_byte(data[i]);
    }

    parsed_input pending;
    if (parser_.flush(pending)) {
        deliver(pending);
    }
}

void input_driver::process_byte(const byte_t byte) {
    parsed_input input;
    if (parser_.feed(byte, input)) {
        deliver(input);
    }
}

void input_driver::deliver(const parsed_input& input) {
    switch (input.type) {
        case parsed_input::kind::key: {
            dispatch_key(input.key);
            break;
        }
        case parsed_input::kind::mouse: {
            dispatch_mouse(input.mouse);
            break;
        }
        case parsed_input::kind::focus: {
            dispatch_focus(input);
            break;
        }
        case parsed_input::kind::paste:
        case parsed_input::kind::clipboard: {
            paste_event pe;
            pe.text = input.paste_text;
            dispatch_paste(pe);
            break;
        }
        case parsed_input::kind::none:
        default: {
            break;
        }
    }
}

void input_driver::dispatch_key(const key_event& e) {
    // Key releases are parsed for apps that want them, but the default dispatch
    // only forwards presses: a release would otherwise fire every binding twice.
    if (e.action == key_action::release) {
        return;
    }
    deliver_guarded(strand_, [this, e] { reconiler_.dispatch_key(e); });
}

void input_driver::dispatch_focus(const parsed_input& input) {
    const focus_event event{input.focused};
    deliver_guarded(strand_, [this, event] { reconiler_.dispatch_focus(event); });
}

void input_driver::dispatch_mouse(const mouse_event& e) {
    auto payload = make_shared<mouse_event>(e);
    deliver_guarded(strand_, [this, payload] { reconiler_.dispatch_mouse(*payload); });
}

void input_driver::dispatch_paste(const paste_event& e) {
    auto payload = make_shared<paste_event>(e);
    deliver_guarded(strand_, [this, payload] { reconiler_.dispatch_paste(*payload); });
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
