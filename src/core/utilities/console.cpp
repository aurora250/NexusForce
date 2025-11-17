#include <MSTL/core/utilities/console.hpp>
#include <MSTL/core/utilities/undef_cmacro.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#endif
MSTL_BEGIN_NAMESPACE__

void sys_console::write_string_unsafe(const string_view str) const {
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

string sys_console::readln_string_unsafe() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    char buffer[4096];
    ::DWORD read;
    if (::ReadConsoleA(in_, buffer, sizeof(buffer) - 1, &read, nullptr)) {
        if (read > 1 && buffer[read-2] == '\r' && buffer[read-1] == '\n') {
            read -= 2;
        } else if (read > 0 && (buffer[read-1] == '\r' || buffer[read-1] == '\n')) {
            read--;
        }
        buffer[read] = '\0';
        return string(buffer, read);
    }
    return "";
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

char sys_console::read_char_unsafe() const {
#ifdef MSTL_PLATFORM_WINDOWS__
    char ch;
    ::DWORD read;
    if (::ReadConsoleA(in_, &ch, 1, &read, nullptr) && read > 0) {
        return ch;
    }
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

void sys_console::init_console() {
#ifdef MSTL_PLATFORM_WINDOWS__
    out_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
    in_ = ::GetStdHandle(STD_INPUT_HANDLE);

    if (out_ == INVALID_HANDLE_VALUE || in_ == INVALID_HANDLE_VALUE) {
        Exception(DeviceOperateError("Failed to get console handles"));
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

    // if (!::isatty(out_) || !::isatty(in_)) {
    //     Exception(DeviceOperateError("Not a terminal device"));
    // }
#endif
}

void sys_console::clear() {
    lock_guard<mutex> lock(mutex_);
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
    this->write_string_unsafe("\033[2J\033[1;1H");
#endif
}

void sys_console::pause(const string_view msg) {
    lock_guard<mutex> lock(mutex_);
    this->flush_unsafe();
    this->write_string_unsafe(msg);
    char c = read_char_unsafe();

#ifdef MSTL_PLATFORM_WINDOWS__
    this->readln_string_unsafe();
#endif
}

void sys_console::set_color(const integer32& color) {
    lock_guard<mutex> lock(mutex_);
    this->write_string_unsafe("\033[" + color.to_string() + "m");
}

void sys_console::set_color(const color& color, const bool use_256_color) {
    lock_guard<mutex> lock(mutex_);
    if (use_256_color) {
        this->write_string_unsafe("\033[38;5;" + _MSTL to_string(color.to_ansi_256()) + "m");
    } else {
        this->write_string_unsafe("\033[" + _MSTL to_string(color.to_ansi_basic(false)) + "m");
    }
}

void sys_console::set_background_color(const color& color, const bool use_256_color) {
    lock_guard<mutex> lock(mutex_);
    if (use_256_color) {
        this->write_string_unsafe("\033[48;5;" + _MSTL to_string(color.to_ansi_256()) + "m");
    } else {
        this->write_string_unsafe("\033[" + _MSTL to_string(color.to_ansi_basic(true)) + "m");
    }
}

void sys_console::reset_color() {
    lock_guard<mutex> lock(mutex_);
    this->write_string_unsafe("\033[0m");
}

MSTL_END_NAMESPACE__
