#include <NeForce/core/system/console.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    ifdef NEFORCE_COMPILER_MSVC
#        include <consoleapi.h>
#        include <consoleapi2.h>
#    endif
#    ifdef NEFORCE_COMPILER_MINGW
#        include <windef.h>
#        include <wincon.h>
#        include <wingdi.h>
#    endif
#    include <WinBase.h>
#    include <WinNls.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/system/environment.hpp>
#    include <cerrno>
#    include <cstdlib>
#    include <fcntl.h>
#    include <sys/ioctl.h>
#    include <termios.h>
#    include <unistd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

void sys_console::print_string_unsafe(const string_view str) const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD written = 0;
    ::WriteConsoleA(out_, str.data(), str.length(), &written, nullptr);
#elif defined(NEFORCE_PLATFORM_LINUX)
    size_t total = 0;
    while (total < str.length()) {
        const ssize_t written = ::write(out_, str.data() + total, str.length() - total);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        total += written;
    }
#endif
}

void sys_console::set_color_unsafe(const color& color, const bool use_256_color) const {
    if (use_256_color) {
        this->print_string_unsafe("\033[38;5;" + _NEFORCE to_string(color.to_ansi_256()) + "m");
    } else {
        this->print_string_unsafe("\033[" + _NEFORCE to_string(color.to_ansi_basic(false)) + "m");
    }
}

void sys_console::typewriter_print_unsafe(const string_view text, const milliseconds delay_per_char,
                                          const bool with_sound) const {
    if (delay_per_char.count() <= 0) {
        print_string_unsafe(text);
        return;
    }

    for (const char ch: text) {
        print_string_unsafe(string(1, ch));
        flush_unsafe();

        if (with_sound && is_interactive()) {
            if (ch != ' ' && ch != '\n' && ch != '\t') {
                beep_unsafe();
            }
        }

        if (ch == '.' || ch == '!' || ch == '?') {
            this_thread::sleep_for(delay_per_char * 3);
        } else if (ch == ',' || ch == ';' || ch == ':') {
            this_thread::sleep_for(delay_per_char * 2);
        } else {
            this_thread::sleep_for(delay_per_char);
        }
    }
}

string sys_console::readln_unsafe() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    string result;
    char buffer[1024];
    ::DWORD read = 0;

    do {
        if (::ReadConsoleA(in_, buffer, sizeof(buffer) - 1, &read, nullptr) == FALSE) {
            break;
        }

        size_t actual_read = read;
        if (actual_read > 1 && buffer[actual_read - 2] == '\r' && buffer[actual_read - 1] == '\n') {
            actual_read -= 2;
        } else if (actual_read > 0 && (buffer[actual_read - 1] == '\r' || buffer[actual_read - 1] == '\n')) {
            actual_read--;
        }

        if (actual_read > 0) {
            buffer[actual_read] = '\0';
            result.append(buffer, actual_read);
        }

        if (read < sizeof(buffer) - 1) {
            break;
        }
    } while (true);

    return result;
#elif defined(NEFORCE_PLATFORM_LINUX)
    string line;
    while (true) {
        char ch = 0;
        const ssize_t n = ::read(in_, &ch, 1);
        if (n <= 0) {
            break;
        }
        if (ch == '\n') {
            break;
        }
        if (ch == '\r') {
            continue;
        }
        line.push_back(ch);
    }
    return line;
#endif
}

string sys_console::read_unsafe() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    string result;
    char ch = 0;
    ::DWORD read = 0;

    while (true) {
        if (::ReadConsoleA(in_, &ch, 1, &read, nullptr) == FALSE || read == 0) {
            break;
        }
        if (_NEFORCE is_space(ch)) {
            if (result.empty()) {
                continue;
            }
            break;
        }
        result.push_back(ch);
    }
    return result;
#elif defined(NEFORCE_PLATFORM_LINUX)
    string result;

    ::termios old_tio, new_tio;
    ::tcgetattr(in_, &old_tio);
    new_tio = old_tio;

    new_tio.c_lflag &= ~(ICANON | ECHO);
    new_tio.c_cc[VMIN] = 1;
    new_tio.c_cc[VTIME] = 0;
    ::tcsetattr(in_, TCSANOW, &new_tio);

    try {
        while (true) {
            char ch = 0;
            const ssize_t n = ::read(in_, &ch, 1);

            if (n <= 0) {
                if (n < 0 && errno == EINTR) {
                    continue;
                }
                break;
            }
            if (_NEFORCE is_space(ch)) {
                if (result.empty()) {
                    continue;
                }
                break;
            }
            result.push_back(ch);
        }
    } catch (...) {
        ::tcsetattr(in_, TCSANOW, &old_tio);
        throw;
    }
    ::tcsetattr(in_, TCSANOW, &old_tio);
    return result;
#endif
}

char sys_console::read_char_unsafe() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD original_mode = 0;
    ::GetConsoleMode(in_, &original_mode);
    ::SetConsoleMode(in_, original_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
    char ch = '\0';
    ::DWORD read = 0;
    try {
        if (::ReadConsoleA(in_, &ch, 1, &read, nullptr) == TRUE && read > 0) {
            ::SetConsoleMode(in_, original_mode);
            if (ch == '\r') {
                char next_ch = '\0';
                if (::ReadConsoleA(in_, &next_ch, 1, &read, nullptr) == TRUE && read > 0) {
                    if (next_ch == '\n') {
                        return '\n';
                    }
                }
                return '\n';
            }
            return ch;
        }
    } catch (...) {
        ::SetConsoleMode(in_, original_mode);
        throw;
    }
    ::SetConsoleMode(in_, original_mode);
    return '\0';
#elif defined(NEFORCE_PLATFORM_LINUX)
    ::termios old_tio, new_tio;
    ::tcgetattr(in_, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    new_tio.c_cc[VMIN] = 1;
    new_tio.c_cc[VTIME] = 0;
    ::tcsetattr(in_, TCSANOW, &new_tio);

    char ch = '\0';
    const ssize_t n = ::read(in_, &ch, 1);
    ::tcsetattr(in_, TCSANOW, &old_tio);

    return (n > 0) ? ch : '\0';
#endif
}

sys_console::console_size sys_console::get_console_size_unsafe() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (::GetConsoleScreenBufferInfo(out_, &csbi) == TRUE) {
        return console_size{csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1};
    }
    return console_size{80, 24};
#else
    ::winsize ws;
    if (::ioctl(out_, TIOCGWINSZ, &ws) == 0) {
        return console_size{ws.ws_col, ws.ws_row};
    }
    const string cols = environment::get("COLUMNS");
    const string rows = environment::get("LINES");
    if (!cols.empty() && !rows.empty()) {
        return console_size{_NEFORCE to_int32(cols.view()), _NEFORCE to_int32(rows.view())};
    }
    return console_size{80, 24};
#endif
}

void sys_console::flush_unsafe() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::GetFileType(out_) != FILE_TYPE_CHAR) {
        ::FlushFileBuffers(out_);
    }
#elif defined(NEFORCE_PLATFORM_LINUX)
    if (::isatty(out_) == 0) {
        ::fsync(out_);
    }
#endif
}

void sys_console::beep_unsafe() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::Beep(800, 200);
#elif defined(NEFORCE_PLATFORM_LINUX)
    if (is_interactive()) {
        print_string_unsafe("\a");
        flush_unsafe();
    } else {
        const int fd = ::open("/dev/tty", O_WRONLY);
        if (fd != -1) {
            ::write(fd, "\a", 1);
            ::close(fd);
        } else {
            ::write(STDERR_FILENO, "\a", 1);
        }
    }
#endif
}

void sys_console::flash_screen_unsafe() const {
    if (!is_interactive() || !supports_colors()) {
        return;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS

    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    ::GetConsoleScreenBufferInfo(out_, &csbi);

    const ::WORD inverted_attr = (csbi.wAttributes & 0xF0) >> 4 | (csbi.wAttributes & 0x0F) << 4;
    ::SetConsoleTextAttribute(out_, inverted_attr);

    this_thread::sleep_for(milliseconds(100));
    ::SetConsoleTextAttribute(out_, csbi.wAttributes);

#elif defined(NEFORCE_PLATFORM_LINUX)

    print_string_unsafe("\033[7m");
    flush_unsafe();

    this_thread::sleep_for(milliseconds(100));
    print_string_unsafe("\033[0m");

    flush_unsafe();

#endif
}

void sys_console::fade_effect_unsafe(const string_view text, const color& from, const color& to,
                                     const milliseconds duration, const bool is_fade_in) const {
    if (!supports_colors() || duration.count() <= 0 || text.empty()) {
        print_string_unsafe(text);
        return;
    }

    constexpr int steps = 50;
    const auto step_duration = duration / steps;

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    ::GetConsoleScreenBufferInfo(out_, &csbi);
#else
    print_string_unsafe("\033[s");
#endif
    print_string_unsafe("\033[2K");

    if (!is_fade_in) {
        set_color_unsafe(to, supports_truecolor());
        print_string_unsafe(text);
        flush_unsafe();
    }

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;
        if (!is_fade_in) {
            t = 1.0F - t;
        }
        color current_color = color::lerp(from, to, t);
        print_string_unsafe("\r\033[2K");
        set_color_unsafe(current_color, supports_truecolor());
        print_string_unsafe(text);
        flush_unsafe();
        this_thread::sleep_for(step_duration);
    }

    print_string_unsafe("\033[0m");
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCursorPosition(out_, csbi.dwCursorPosition);
#else
    print_string_unsafe("\033[u");
#endif
    flush_unsafe();
}

sys_console::sys_console() noexcept
#ifdef NEFORCE_PLATFORM_WINDOWS
:
out_(::GetStdHandle(STD_OUTPUT_HANDLE)),
in_(::GetStdHandle(STD_INPUT_HANDLE))
#else
:
out_(STDOUT_FILENO),
in_(STDIN_FILENO)
#endif
{
#ifdef NEFORCE_PLATFORM_WINDOWS
    try {
        if (out_ == INVALID_HANDLE_VALUE || in_ == INVALID_HANDLE_VALUE) {
            return;
        }

        ::SetConsoleOutputCP(CP_UTF8);
        ::SetConsoleCP(CP_UTF8);

        ::DWORD mode = 0;
        ::GetConsoleMode(out_, &mode);
        mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        ::SetConsoleMode(out_, mode);
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
#endif
}

void sys_console::flush() {
    lock<mutex> lock(mutex_);
    this->flush_unsafe();
}

void sys_console::print_string(const string& str) {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe(str.view());
}

void sys_console::print_string(const string_view& view) {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe(view);
}

void sys_console::print_string(const char* str) {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe(str);
}

string sys_console::read() {
    lock<mutex> lock(mutex_);
    return this->read_unsafe();
}

string sys_console::readln() {
    lock<mutex> lock(mutex_);
    return this->readln_unsafe();
}

char sys_console::read_char() {
    lock<mutex> lock(mutex_);
    return this->read_char_unsafe();
}

void sys_console::println() {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe("\n");
}

void sys_console::clear() {
    lock<mutex> lock(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    constexpr ::COORD top_left = {0, 0};
    ::CONSOLE_SCREEN_BUFFER_INFO screen{};
    ::DWORD written = 0;

    ::GetConsoleScreenBufferInfo(out_, &screen);
    const ::DWORD length = screen.dwSize.X * screen.dwSize.Y;
    ::FillConsoleOutputCharacterA(out_, ' ', length, top_left, &written);
    ::FillConsoleOutputAttribute(out_, screen.wAttributes, length, top_left, &written);
    ::SetConsoleCursorPosition(out_, top_left);
#elif defined(NEFORCE_PLATFORM_LINUX)
    this->print_string_unsafe("\033[2J\033[1;1H");
#endif
}

void sys_console::pause(const string_view msg) {
    lock<mutex> lock(mutex_);
    this->flush_unsafe();
    this->print_string_unsafe(msg);
    ignore = this->readln_unsafe();
    this->flush_unsafe();
}

bool sys_console::confirmation(const string_view prompt, const char yes, const char no) {
    lock<mutex> lock(mutex_);
    while (true) {
        this->print_string_unsafe(prompt);
        this->flush_unsafe();

        const char input = read_char_unsafe();
        this->print_string_unsafe("\n");

        if (to_uppercase(input) == to_uppercase(yes)) {
            return true;
        }
        if (to_uppercase(input) == to_uppercase(no)) {
            return false;
        }
        const string error_msg = "Please enter '"_s + yes + "' or '" + no + "'.\n";
        this->print_string_unsafe(error_msg);
    }
}

string sys_console::password(const string_view prompt, const char mask, const bool show_length) {
    lock<mutex> lock(mutex_);
    if (!is_interactive()) {
        return readln_unsafe();
    }
    string password;

    auto erase_chars = [&](const size_t n) {
        for (size_t i = 0; i < n; ++i) {
            print_string_unsafe("\b \b");
        }
    };

#ifdef NEFORCE_PLATFORM_WINDOWS
    print_string_unsafe(prompt);
    flush_unsafe();

    ::DWORD original_mode = 0;
    if (::GetConsoleMode(in_, &original_mode) == FALSE) {
        NEFORCE_THROW_EXCEPTION(console_exception("Failed to get console mode"));
    }

    ::DWORD new_mode = original_mode;
    new_mode &= ~ENABLE_ECHO_INPUT;
    new_mode &= ~ENABLE_LINE_INPUT;
    new_mode |= ENABLE_PROCESSED_INPUT;
    if (::SetConsoleMode(in_, new_mode) == FALSE) {
        NEFORCE_THROW_EXCEPTION(console_exception("Failed to set console mode"));
    }

    try {
        while (true) {
            char ch = 0;
            ::DWORD read = 0;

            if (::ReadConsoleA(in_, &ch, 1, &read, nullptr) == FALSE || read == 0) {
                break;
            }

            if (ch == '\r' || ch == '\n') {
                print_string_unsafe("\n");
                break;
            }

            if (ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();

                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                    if (show_length) {
                        const string old_display = " [" + to_string(password.length() + 1) + "]";
                        erase_chars(old_display.length());

                        if (!password.empty()) {
                            const string new_display = " [" + to_string(password.length()) + "]";
                            print_string_unsafe(new_display);
                            erase_chars(new_display.length());
                        }
                    }
                }
            } else if (ch == '\x03') {
                print_string_unsafe("^C\n");
                ::SetConsoleMode(in_, original_mode);
                NEFORCE_THROW_EXCEPTION(console_exception("Interrupted by user"));
            } else if (ch == '\x00' || ch == '\xe0') {
                ::ReadConsoleA(in_, &ch, 1, &read, nullptr);
                continue;
            } else if (ch >= 32 && ch <= 126) {
                password.push_back(ch);

                if (mask != '\0') {
                    print_string_unsafe(string(1, mask));
                }
                if (show_length) {
                    const string old_display =
                            password.length() > 1 ? " [" + to_string(password.length() - 1) + "]" : "";
                    erase_chars(old_display.length());
                    const string new_display = " [" + to_string(password.length()) + "]";
                    print_string_unsafe(new_display);
                    erase_chars(new_display.length());
                }
            }
            flush_unsafe();
        }
    } catch (...) {
        ::SetConsoleMode(in_, original_mode);
        throw;
    }
    ::SetConsoleMode(in_, original_mode);

#elif defined(NEFORCE_PLATFORM_LINUX)
    print_string_unsafe(prompt);
    flush_unsafe();

    auto refresh_length_display = [&](size_t old_len, size_t new_len) {
        if (!show_length) {
            return;
        }
        const string old_display = old_len > 0 ? " [" + to_string(old_len) + "]" : "";
        erase_chars(old_display.length());
        if (new_len > 0) {
            const string new_display = " [" + to_string(new_len) + "]";
            print_string_unsafe(new_display);
            erase_chars(new_display.length());
        }
    };

    ::termios original_termios;
    ::tcgetattr(in_, &original_termios);
    ::termios new_termios = original_termios;
    new_termios.c_lflag &= ~(ECHO | ICANON);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    ::tcsetattr(in_, TCSANOW, &new_termios);

    try {
        while (true) {
            char ch = 0;
            const ssize_t n = ::read(in_, &ch, 1);

            if (n <= 0) {
                if (n < 0 && errno == EINTR) {
                    continue;
                }
                break;
            }

            if (ch == '\n' || ch == '\r') {
                print_string_unsafe("\n");
                break;
            } else if (ch == '\x7f' || ch == '\b') {
                if (!password.empty()) {
                    const size_t old_len = password.length();
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                    refresh_length_display(old_len, password.length());
                }
            } else if (ch == '\x03') { // Ctrl-C
                print_string_unsafe("^C\n");
                NEFORCE_THROW_EXCEPTION(console_exception("Interrupted by user"));
            } else if (ch == '\x15') { // Ctrl-U
                const size_t old_len = password.length();
                if (mask != '\0') {
                    erase_chars(old_len);
                }
                refresh_length_display(old_len, 0);
                password.clear();
            } else if (ch == '\x17') { // Ctrl-W
                const size_t old_len = password.length();
                while (!password.empty() && password.back() == ' ') {
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                }
                while (!password.empty() && password.back() != ' ') {
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                }
                refresh_length_display(old_len, password.length());
            } else if (ch >= 32 && ch <= 126) {
                const size_t old_len = password.length();
                password.push_back(ch);
                if (mask != '\0') {
                    print_string_unsafe(string(1, mask));
                }
                refresh_length_display(old_len, password.length());
            }
            flush_unsafe();
        }
    } catch (...) {
        ::tcsetattr(in_, TCSANOW, &original_termios);
        throw;
    }
    ::tcsetattr(in_, TCSANOW, &original_termios);
#endif

    return password;
}

void sys_console::set_color(const integer32& color) {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe("\033[" + color.to_string() + "m");
}

void sys_console::set_color(const color& color, const bool use_256_color) {
    lock<mutex> lock(mutex_);
    set_color_unsafe(color, use_256_color);
}

void sys_console::set_background_color(const color& color, const bool use_256_color) {
    lock<mutex> lock(mutex_);
    if (use_256_color) {
        this->print_string_unsafe("\033[48;5;" + _NEFORCE to_string(color.to_ansi_256()) + "m");
    } else {
        this->print_string_unsafe("\033[" + _NEFORCE to_string(color.to_ansi_basic(true)) + "m");
    }
}

void sys_console::reset_color() {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe("\033[0m");
}

void sys_console::progress_bar(double percentage, const int width, const bool show_percentage, const char fill_char,
                               const char empty_char) {
    lock<mutex> lock(mutex_);

    double display_percentage = percentage;
    if (percentage > 1.0 && percentage <= 100.0) {
        display_percentage = percentage / 100.0;
    }
    if (display_percentage < 0.0) {
        display_percentage = 0.0;
    } else if (display_percentage > 1.0) {
        display_percentage = 1.0;
    }

    const int filled = static_cast<int>(display_percentage * width);
    string bar;
    bar.reserve(width + 20);
    bar.push_back('[');
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            bar.push_back(fill_char);
        } else {
            bar.push_back(empty_char);
        }
    }
    bar.push_back(']');
    if (show_percentage) {
        bar.append(" ");
        bar.append(_NEFORCE to_string(static_cast<int>(display_percentage * 100)));
        bar.append("%");
    }

    print_string_unsafe("\r");
    print_string_unsafe(bar);
    flush_unsafe();
}

void sys_console::set_cursor_position(int row, int column) {
    lock<mutex> lock(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    const ::COORD pos{static_cast<::SHORT>(column), static_cast<::SHORT>(row)};
    ::SetConsoleCursorPosition(out_, pos);
#else
    this->print_string_unsafe("\033[" + _NEFORCE to_string(row) + ";" + _NEFORCE to_string(column) + "H");
#endif
}

void sys_console::save_cursor_position() {
    lock<mutex> lock(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    ::GetConsoleScreenBufferInfo(out_, &csbi);
    const auto pos = csbi.dwCursorPosition;
    saved_cursor_pos_ = console_size{pos.X, pos.Y};
#else
    this->print_string_unsafe("\033[s");
#endif
}

void sys_console::restore_cursor_position() {
    lock<mutex> lock(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCursorPosition(out_, ::COORD{static_cast<::SHORT>(saved_cursor_pos_.width),
                                             static_cast<::SHORT>(saved_cursor_pos_.height)});
#else
    this->print_string_unsafe("\033[u");
#endif
}

void sys_console::hide_cursor() {
    lock<mutex> lock(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CONSOLE_CURSOR_INFO cursor_info{};
    ::GetConsoleCursorInfo(out_, &cursor_info);
    cursor_info.bVisible = FALSE;
    ::SetConsoleCursorInfo(out_, &cursor_info);
#else
    this->print_string_unsafe("\033[?25l");
#endif
}

void sys_console::show_cursor() {
    lock<mutex> lock(mutex_);
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CONSOLE_CURSOR_INFO cursor_info{};
    ::GetConsoleCursorInfo(out_, &cursor_info);
    cursor_info.bVisible = TRUE;
    ::SetConsoleCursorInfo(out_, &cursor_info);
#else
    this->print_string_unsafe("\033[?25h");
#endif
}

sys_console::console_size sys_console::get_console_size() const {
    lock<mutex> lock(mutex_);
    return get_console_size_unsafe();
}

bool sys_console::is_terminal_resized() {
    lock<mutex> lock(mutex_);
    const console_size current = get_console_size_unsafe();
    if (current != last_size_) {
        last_size_ = current;
        return true;
    }
    return false;
}

bool sys_console::supports_colors() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD mode = 0;
    ::GetConsoleMode(out_, &mode);
    return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    const string term = environment::get("TERM");
    if (term.empty()) {
        return false;
    }
    return (::isatty(out_) != 0 && (term.contains("xterm") || term.contains("screen") || term.contains("tmux") ||
                                    term.contains("rxvt") || term.contains("color")));
#endif
}

bool sys_console::supports_truecolor() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return supports_colors();
#else
    if (!supports_colors()) {
        return false;
    }
    const string colorterm = environment::get("COLORTERM");
    return !colorterm.empty() && (colorterm.contains("truecolor") || colorterm.contains("24bit"));
#endif
}

bool sys_console::supports_unicode() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::GetConsoleOutputCP() == CP_UTF8;
#else
    const string lang = environment::get("LANG");
    const string lc_all = environment::get("LC_ALL");
    const string lc_ctype = environment::get("LC_CTYPE");

    const string encoding = !lc_ctype.empty() ? lc_ctype : (!lc_all.empty() ? lc_all : lang);
    if (encoding.empty()) {
        return false;
    }

    return encoding.contains("UTF-8") || encoding.contains("utf8");
#endif
}

bool sys_console::is_interactive() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return out_ != INVALID_HANDLE_VALUE && ::GetFileType(out_) == FILE_TYPE_CHAR;
#else
    return ::isatty(out_) != 0;
#endif
}

string sys_console::console_type() const {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return "windows_console";
#else
    const string term = environment::get("TERM");
    return !term.empty() ? term : "unknown";
#endif
}

void sys_console::typewriter_print(const string_view text, const milliseconds delay_per_char, const bool with_sound) {
    lock<mutex> lock(mutex_);
    typewriter_print_unsafe(text, delay_per_char, with_sound);
}

void sys_console::typewriter_println(const string_view text, const milliseconds delay_per_char, const bool with_sound) {
    lock<mutex> lock(mutex_);
    typewriter_print_unsafe(text, delay_per_char, with_sound);
    print_string_unsafe("\n");
}

void sys_console::beep() {
    lock<mutex> lock(mutex_);
    beep_unsafe();
}

void sys_console::flash_screen() {
    lock<mutex> lock(mutex_);
    flash_screen_unsafe();
}

void sys_console::notification(const string_view message, const milliseconds duration, const bool play_sound) {
    lock<mutex> lock(mutex_);

    if (play_sound) {
        beep_unsafe();
    }
    flash_screen_unsafe();

    const string notification_text = "\n[NOTIFICATION] "_s + message + "\n";
    print_string_unsafe(notification_text);

    if (duration.count() > 0) {
        this_thread::sleep_for(duration);
        print_string_unsafe("\033[2K\r\033[1A\033[2K\r");
    }
}

void sys_console::fade_in(const string_view text, const milliseconds duration, const color& start_color,
                          const color& end_color) {
    lock<mutex> lock(mutex_);
    fade_effect_unsafe(text, start_color, end_color, duration, true);
}

void sys_console::fade_out(const string_view text, const milliseconds duration, const color& start_color,
                           const color& end_color) {
    lock<mutex> lock(mutex_);
    fade_effect_unsafe(text, start_color, end_color, duration, false);
}

void sys_console::fade_in_out(const string_view text, const milliseconds in_duration, const milliseconds hold_duration,
                              const milliseconds out_duration) {
    lock<mutex> lock(mutex_);

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    ::GetConsoleScreenBufferInfo(out_, &csbi);
#else
    print_string_unsafe("\033[s");
#endif
    fade_effect_unsafe(text, color::black(), color::white(), in_duration, true);
    if (hold_duration.count() > 0) {
        this_thread::sleep_for(hold_duration);
    }
    fade_effect_unsafe(text, color::white(), color::black(), out_duration, false);
    print_string_unsafe("\r\033[2K");
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::SetConsoleCursorPosition(out_, csbi.dwCursorPosition);
#else
    print_string_unsafe("\033[u");
#endif
    flush_unsafe();
}

sys_console& console = sys_console::instance();

NEFORCE_END_NAMESPACE__
