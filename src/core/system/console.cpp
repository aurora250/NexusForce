#include <MSTL/core/system/console.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <MSTL/core/system/environment.hpp>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <cstdlib>
#endif
MSTL_BEGIN_NAMESPACE__

void sys_console::print_string_unsafe(const string_view str) const {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD written;
    ::WriteConsoleA(out_, str.data(), str.length(), &written, nullptr);
#elif defined(MSTL_PLATFORM_LINUX__)
    size_t total = 0;
    while (total < str.length()) {
        const ssize_t written = ::write(out_, str.data() + total, str.length() - total);
        if (written < 0) {
            if (errno == EINTR) continue;
            break;
        }
        total += written;
    }
#endif
}

void sys_console::set_color_unsafe(const color& color, const bool use_256_color) const {
    if (use_256_color) {
        this->print_string_unsafe("\033[38;5;" + _MSTL to_string(color.to_ansi_256()) + "m");
    } else {
        this->print_string_unsafe("\033[" + _MSTL to_string(color.to_ansi_basic(false)) + "m");
    }
}

void sys_console::typewriter_print_unsafe(const string_view text,
    const milliseconds delay_per_char, const bool with_sound) const {
    if (delay_per_char.count() <= 0) {
        print_string_unsafe(text);
        return;
    }

    for (const char ch : text) {
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
#ifdef MSTL_PLATFORM_WINDOWS__
    string result;
    char buffer[1024];
    ::DWORD read;

    do {
        if (!::ReadConsoleA(in_, buffer, sizeof(buffer) - 1, &read, nullptr)) {
            break;
        }

        size_t actual_read = read;
        if (actual_read > 1 && buffer[actual_read-2] == '\r' && buffer[actual_read-1] == '\n') {
            actual_read -= 2;
        } else if (actual_read > 0 && (buffer[actual_read-1] == '\r' || buffer[actual_read-1] == '\n')) {
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
#elif defined(MSTL_PLATFORM_LINUX__)
    string line;
    while (true) {
        char ch;
        const ssize_t n = ::read(in_, &ch, 1);
        if (n <= 0) break;
        if (ch == '\n') break;
        if (ch == '\r') continue;
        line.push_back(ch);
    }
    return line;
#endif
}

string sys_console::read_unsafe() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    string result;
    char ch;
    ::DWORD read;

    while (true) {
        if (!::ReadConsoleA(in_, &ch, 1, &read, nullptr) || read == 0) {
            break;
        }
        if (_MSTL is_space(ch)) {
            if (result.empty()) {
                continue;
            }
            break;
        }
        result.push_back(ch);
    }
    return result;
#elif defined(MSTL_PLATFORM_LINUX__)
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
            char ch;
            const ssize_t n = ::read(in_, &ch, 1);

            if (n <= 0) {
                if (n < 0 && errno == EINTR) {
                    continue;
                }
                break;
            }
            if (_MSTL is_space(ch)) {
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
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD original_mode = 0;
    ::GetConsoleMode(in_, &original_mode);
    ::SetConsoleMode(in_, original_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
    char ch = '\0';
    ::DWORD read = 0;
    try {
        if (::ReadConsoleA(in_, &ch, 1, &read, nullptr) && read > 0) {
            ::SetConsoleMode(in_, original_mode);
            if (ch == '\r') {
                char next_ch = '\0';
                if (::ReadConsoleA(in_, &next_ch, 1, &read, nullptr) && read > 0) {
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
#elif defined(MSTL_PLATFORM_LINUX__)
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

void sys_console::flush_unsafe() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::FlushConsoleInputBuffer(in_);
#elif defined(MSTL_PLATFORM_LINUX__)
    ::tcflush(in_, TCIFLUSH);
#endif
}

void sys_console::beep_unsafe() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::Beep(800, 200);
#elif defined(MSTL_PLATFORM_LINUX__)
    if (is_interactive()) {
        print_string_unsafe("\a");
        flush_unsafe();
    } else {
        ::system("echo -e '\a' > /dev/tty 2>/dev/null");
    }
#endif
}

void sys_console::flash_screen_unsafe() const {
    if (!is_interactive() || !supports_colors()) {
        return;
    }
#ifdef MSTL_PLATFORM_WINDOWS__

    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    ::GetConsoleScreenBufferInfo(out_, &csbi);

    const ::WORD inverted_attr = (csbi.wAttributes & 0xF0) >> 4 | (csbi.wAttributes & 0x0F) << 4;
    ::SetConsoleTextAttribute(out_, inverted_attr);

    this_thread::sleep_for(milliseconds(100));
    ::SetConsoleTextAttribute(out_, csbi.wAttributes);

#elif defined(MSTL_PLATFORM_LINUX__)

    print_string_unsafe("\033[7m");
    flush_unsafe();

    this_thread::sleep_for(milliseconds(100));
    print_string_unsafe("\033[0m");

    flush_unsafe();

#endif
}

void sys_console::fade_effect_unsafe(
    const string_view text, const color& from, const color& to,
    const milliseconds duration, const bool is_fade_in) const {
    if (!supports_colors() || duration.count() <= 0 || text.empty()) {
        print_string_unsafe(text);
        return;
    }

    constexpr int steps = 50;
    const auto step_duration = duration / steps;

#ifdef MSTL_PLATFORM_WINDOWS__
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
            t = 1.0f - t;
        }
        color current_color = color::lerp(from, to, t);
        print_string_unsafe("\r\033[2K");
        set_color_unsafe(current_color, supports_truecolor());
        print_string_unsafe(text);
        flush_unsafe();
        this_thread::sleep_for(step_duration);
    }

    print_string_unsafe("\033[0m");
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetConsoleCursorPosition(out_, csbi.dwCursorPosition);
#else
    print_string_unsafe("\033[u");
#endif
    flush_unsafe();
}

sys_console::sys_console() {
#ifdef MSTL_PLATFORM_WINDOWS__
    out_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
    in_ = ::GetStdHandle(STD_INPUT_HANDLE);

    if (out_ == INVALID_HANDLE_VALUE || in_ == INVALID_HANDLE_VALUE) {
        throw_exception(device_exception("Failed to get console handles"));
    }

    ::SetConsoleOutputCP(CP_UTF8);
    ::SetConsoleCP(CP_UTF8);

    ::DWORD mode;
    ::GetConsoleMode(out_, &mode);
    mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    ::SetConsoleMode(out_, mode);
#elif defined(MSTL_PLATFORM_LINUX__)
    out_ = STDOUT_FILENO;
    in_ = STDIN_FILENO;
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
#ifdef MSTL_PLATFORM_WINDOWS__
    constexpr ::COORD top_left = { 0, 0 };
    ::CONSOLE_SCREEN_BUFFER_INFO screen;
    ::DWORD written;

    ::GetConsoleScreenBufferInfo(out_, &screen);
    const ::DWORD length = screen.dwSize.X * screen.dwSize.Y;
    ::FillConsoleOutputCharacterA(out_, ' ', length, top_left, &written);
    ::FillConsoleOutputAttribute(out_, screen.wAttributes, length, top_left, &written);
    ::SetConsoleCursorPosition(out_, top_left);
#elif defined(MSTL_PLATFORM_LINUX__)
    this->print_string_unsafe("\033[2J\033[1;1H");
#endif
}

void sys_console::pause(const string_view msg) {
    lock<mutex> lock(mutex_);
    this->flush_unsafe();
    this->print_string_unsafe(msg);
    MSTL_IGNORE this->readln_unsafe();
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
        } else if (to_uppercase(input) == to_uppercase(no)) {
            return false;
        } else {
            const string error_msg = "Please enter '"_s + yes + "' or '" + no + "'.\n";
            this->print_string_unsafe(error_msg);
        }
    }
}

string sys_console::password(const string_view prompt, const char mask, const bool show_length) {
    lock<mutex> lock(mutex_);
    if (!is_interactive()) {
        return readln_unsafe();
    }
    string password;

#ifdef MSTL_PLATFORM_WINDOWS__
    print_string_unsafe(prompt);
    flush_unsafe();

    ::DWORD original_mode = 0;
    if (!::GetConsoleMode(in_, &original_mode)) {
        throw_exception(device_exception("Failed to get console mode"));
    }
    ::DWORD new_mode = original_mode;
    new_mode &= ~ENABLE_ECHO_INPUT;
    new_mode &= ~ENABLE_LINE_INPUT;
    new_mode |= ENABLE_PROCESSED_INPUT;
    if (!::SetConsoleMode(in_, new_mode)) {
        throw_exception(device_exception("Failed to set console mode"));
    }

    try {
        while (true) {
            char ch;
            ::DWORD read;
            if (!::ReadConsoleA(in_, &ch, 1, &read, nullptr) || read == 0) {
                break;
            }

            if (ch == '\r' || ch == '\n') {
                print_string_unsafe("\n");
                break;
            } else if (ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();

                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    } else if (show_length) {
                        size_t display_length = password.length() + 1;
                        if (password.length() > 9) ++display_length;
                        if (password.length() > 99) ++display_length;

                        print_string_unsafe("\r");
                        print_string_unsafe(prompt);

                        if (!password.empty()) {
                            string length_display = " [" + to_string(password.length()) + "]";
                            print_string_unsafe(length_display);
                        }
                    }
                }
            } else if (ch == '\x03') {
                print_string_unsafe("^C\n");
                ::SetConsoleMode(in_, original_mode);
                throw_exception(system_exception("Interrupted by user"));
            } else if (ch == '\x00' || ch == '\xe0') {
                ::ReadConsoleA(in_, &ch, 1, &read, nullptr);
                continue;
            } else if (ch >= 32 && ch <= 126) {
                password.push_back(ch);

                if (mask != '\0') {
                    print_string_unsafe(string(1, mask));
                } else if (show_length) {
                    print_string_unsafe("\r");
                    print_string_unsafe(prompt);
                    string length_display = " [" + to_string(password.length()) + "]";
                    print_string_unsafe(length_display);
                }
            }
            flush_unsafe();
        }
    } catch (...) {
        ::SetConsoleMode(in_, original_mode);
        throw;
    }
    ::SetConsoleMode(in_, original_mode);

#elif defined(MSTL_PLATFORM_LINUX__)
    print_string_unsafe(prompt);
    flush_unsafe();

    ::termios original_termios;
    ::tcgetattr(in_, &original_termios);
    ::termios new_termios = original_termios;
    new_termios.c_lflag &= ~(ECHO | ICANON);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    ::tcsetattr(in_, TCSANOW, &new_termios);

    try {
        while (true) {
            char ch;
            const ssize_t n = ::read(in_, &ch, 1);

            if (n <= 0) {
                if (n < 0 && errno == EINTR) continue;
                break;
            }

            if (ch == '\n' || ch == '\r') {
                print_string_unsafe("\n");
                break;
            } else if (ch == '\x7f' || ch == '\b') {
                if (!password.empty()) {
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                    if (show_length) {
                        string length_display = " [" + to_string(password.length()) + "]";
                        print_string_unsafe(length_display);
                        for (size_t i = 0; i < length_display.length(); ++i) {
                            print_string_unsafe("\b");
                        }
                    }
                }
            } else if (ch == '\x03') {
                print_string_unsafe("^C\n");
                throw_exception(system_exception("Interrupted by user"));
            } else if (ch == '\x15') {
                while (!password.empty()) {
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                }
                if (show_length) {
                    string length_display = " [" + to_string(password.length()) + "]";
                    print_string_unsafe(length_display);
                    for (size_t i = 0; i < length_display.length(); ++i) {
                        print_string_unsafe("\b");
                    }
                }
            } else if (ch == '\x17') {
                while (!password.empty() && password.back() != ' ') {
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                }
                if (!password.empty() && password.back() == ' ') {
                    password.pop_back();
                    if (mask != '\0') {
                        print_string_unsafe("\b \b");
                    }
                }
                if (show_length) {
                    string length_display = " [" + to_string(password.length()) + "]";
                    print_string_unsafe(length_display);
                    for (size_t i = 0; i < length_display.length(); ++i) {
                        print_string_unsafe("\b");
                    }
                }
            } else if (ch >= 32 && ch <= 126) {
                password.push_back(ch);
                if (mask != '\0') {
                    print_string_unsafe(string(1, mask));
                }
                if (show_length) {
                    string length_display = " [" + to_string(password.length()) + "]";
                    print_string_unsafe(length_display);
                    for (size_t i = 0; i < length_display.length(); ++i) {
                        print_string_unsafe("\b");
                    }
                }
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
        this->print_string_unsafe("\033[48;5;" + _MSTL to_string(color.to_ansi_256()) + "m");
    } else {
        this->print_string_unsafe("\033[" + _MSTL to_string(color.to_ansi_basic(true)) + "m");
    }
}

void sys_console::reset_color() {
    lock<mutex> lock(mutex_);
    this->print_string_unsafe("\033[0m");
}

void sys_console::progress_bar(double percentage, const int width,
    const bool show_percentage, const char fill_char, const char empty_char) {
    lock<mutex> lock(mutex_);

    double display_percentage = percentage;
    if (percentage > 1.0 && percentage <= 100.0) {
        display_percentage = percentage / 100.0;
    }
    if (display_percentage < 0.0) display_percentage = 0.0;
    else if (display_percentage > 1.0) display_percentage = 1.0;

    const int filled = static_cast<int>(display_percentage * width);
    string bar;
    bar.reserve(width + 20);
    bar.push_back('[');
    for (int i = 0; i < width; ++i) {
        if (i < filled) bar.push_back(fill_char);
        else bar.push_back(empty_char);
    }
    bar.push_back(']');
    if (show_percentage) {
        bar.append(" ");
        bar.append(_MSTL to_string(static_cast<int>(display_percentage * 100)));
        bar.append("%");
    }

    print_string_unsafe("\r");
    print_string_unsafe(bar);
    flush_unsafe();
}

void sys_console::set_cursor_position(int row, int column) {
    lock<mutex> lock(mutex_);
#ifdef MSTL_PLATFORM_WINDOWS__
    const ::COORD pos{static_cast<::SHORT>(column), static_cast<::SHORT>(row)};
    ::SetConsoleCursorPosition(out_, pos);
#else
    this->print_string_unsafe("\033[" + _MSTL to_string(row) + ";" + _MSTL to_string(column) + "H");
#endif
}

void sys_console::save_cursor_position() {
    lock<mutex> lock(mutex_);
#ifdef MSTL_PLATFORM_WINDOWS__
    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    ::GetConsoleScreenBufferInfo(out_, &csbi);
    saved_cursor_pos_ = csbi.dwCursorPosition;
#else
    this->print_string_unsafe("\033[s");
#endif
}

void sys_console::restore_cursor_position() {
    lock<mutex> lock(mutex_);
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetConsoleCursorPosition(out_, saved_cursor_pos_);
#else
    this->print_string_unsafe("\033[u");
#endif
}

void sys_console::hide_cursor() {
    lock<mutex> lock(mutex_);
#ifdef MSTL_PLATFORM_WINDOWS__
    ::CONSOLE_CURSOR_INFO cursorInfo{};
    ::GetConsoleCursorInfo(out_, &cursorInfo);
    cursorInfo.bVisible = false;
    ::SetConsoleCursorInfo(out_, &cursorInfo);
#else
    this->print_string_unsafe("\033[?25l");
#endif
}

void sys_console::show_cursor() {
    lock<mutex> lock(mutex_);
#ifdef MSTL_PLATFORM_WINDOWS__
    ::CONSOLE_CURSOR_INFO cursorInfo{};
    ::GetConsoleCursorInfo(out_, &cursorInfo);
    cursorInfo.bVisible = true;
    ::SetConsoleCursorInfo(out_, &cursorInfo);
#else
    this->print_string_unsafe("\033[?25h");
#endif
}

sys_console::console_size sys_console::get_console_size() const {
    lock<mutex> lock(mutex_);

#ifdef MSTL_PLATFORM_WINDOWS__
    ::CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (::GetConsoleScreenBufferInfo(out_, &csbi)) {
        return console_size{
            csbi.srWindow.Right - csbi.srWindow.Left + 1,
            csbi.srWindow.Bottom - csbi.srWindow.Top + 1
        };
    }
    return console_size{80, 24};

#elif defined(MSTL_PLATFORM_LINUX__)
    ::winsize ws;
    if (::ioctl(out_, TIOCGWINSZ, &ws) == 0) {
        return console_size{ws.ws_col, ws.ws_row};
    }
    const string cols = environment::get("COLUMNS");
    const string rows = environment::get("LINES");
    if (!cols.empty() && !rows.empty()) {
        return console_size{
            _MSTL to_int32(cols.view()),
            _MSTL to_int32(rows.view())
        };
    }
    return console_size{80, 24};
#endif
}

bool sys_console::is_terminal_resized() {
    lock<mutex> lock(mutex_);
    const console_size current = get_console_size();
    if (current != last_size_) {
        last_size_ = current;
        return true;
    }
    return false;
}

bool sys_console::supports_colors() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD mode;
    ::GetConsoleMode(out_, &mode);
    return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#elif defined(MSTL_PLATFORM_LINUX__)
    const string term = environment::get("TERM");
    if (term.empty()) return false;
    return (::isatty(out_) && (
        term.find("xterm") != string::npos ||
        term.find("screen") != string::npos ||
        term.find("tmux") != string::npos ||
        term.find("rxvt") != string::npos ||
        term.find("color") != string::npos
    ));
#endif
}

bool sys_console::supports_truecolor() const {
    if (!supports_colors()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    return true;
#elif defined(MSTL_PLATFORM_LINUX__)
    const string colorterm = environment::get("COLORTERM");
    return !colorterm.empty() && (
        colorterm.find("truecolor") != string::npos ||
        colorterm.find("24bit") != string::npos
    );
#endif
}

bool sys_console::supports_unicode() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    return ::GetConsoleOutputCP() == CP_UTF8;
#elif defined(MSTL_PLATFORM_LINUX__)
    const string lang = environment::get("LANG");
    const string lc_all = environment::get("LC_ALL");
    const string lc_ctype = environment::get("LC_CTYPE");

    const string encoding = lc_ctype ? lc_ctype : (lc_all ? lc_all : lang);
    if (encoding.empty()) return false;

    return string_find_pattern(encoding.data(), "UTF-8") != nullptr ||
           string_find_pattern(encoding.data(), "utf8") != nullptr;
#endif
}

bool sys_console::is_interactive() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    return out_ != INVALID_HANDLE_VALUE && ::GetFileType(out_) == FILE_TYPE_CHAR;
#elif defined(MSTL_PLATFORM_LINUX__)
    return ::isatty(out_);
#endif
}

string sys_console::console_type() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    return "windows_console";
#elif defined(MSTL_PLATFORM_LINUX__)
    const string term = environment::get("TERM");
    return !term.empty() ? term : "unknown";
#endif
}

void sys_console::typewriter_print(const string_view text,
    const milliseconds delay_per_char, const bool with_sound) {
    lock<mutex> lock(mutex_);
    typewriter_print_unsafe(text, delay_per_char, with_sound);
}

void sys_console::typewriter_println(const string_view text,
    const milliseconds delay_per_char, const bool with_sound) {
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

void sys_console::notification(const string_view message,
    const milliseconds duration, const bool play_sound) {
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

void sys_console::fade_in(const string_view text, const milliseconds duration,
    const color& start_color, const color& end_color) {
    lock<mutex> lock(mutex_);
    fade_effect_unsafe(text, start_color, end_color, duration, true);
}

void sys_console::fade_out(const string_view text, const milliseconds duration,
    const color& start_color, const color& end_color) {
    lock<mutex> lock(mutex_);
    fade_effect_unsafe(text, start_color, end_color, duration, false);
}

void sys_console::fade_in_out(const string_view text, const milliseconds in_duration,
    const milliseconds hold_duration, const milliseconds out_duration) {
    lock<mutex> lock(mutex_);

#ifdef MSTL_PLATFORM_WINDOWS__
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
#ifdef MSTL_PLATFORM_WINDOWS__
    ::SetConsoleCursorPosition(out_, csbi.dwCursorPosition);
#else
    print_string_unsafe("\033[u");
#endif
    flush_unsafe();
}

MSTL_END_NAMESPACE__
