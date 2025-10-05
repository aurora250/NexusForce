#include <MSTL/core/console.hpp>
MSTL_BEGIN_NAMESPACE__

sys_console::sys_console() {
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
}

void sys_console::flush() const {
    ::FlushConsoleInputBuffer(in_);
}

void sys_console::write_string(const string& str) const {
    ::DWORD written;
    ::WriteConsoleA(out_, str.c_str(), static_cast<DWORD>(str.length()), &written, nullptr);
}

void sys_console::write_string(const string_view& str) const {
    ::DWORD written;
    ::WriteConsoleA(out_, str.data(), static_cast<DWORD>(str.length()), &written, nullptr);
}

void sys_console::write_string(const char* str) const {
    if (str == nullptr) return;
    ::DWORD written;
    ::WriteConsoleA(out_, str, static_cast<::DWORD>(char_traits<char>::length(str)), &written, nullptr);
}

string sys_console::readln_string() const {
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
}

char sys_console::read_char() const {
    char ch;
    ::DWORD read;
    if (::ReadConsoleA(in_, &ch, 1, &read, nullptr) && read > 0) {
        return ch;
    }
    return '\0';
}

void sys_console::clear() const {
    constexpr ::COORD topLeft = { 0, 0 };
    ::CONSOLE_SCREEN_BUFFER_INFO screen;
    ::DWORD written;

    ::GetConsoleScreenBufferInfo(out_, &screen);
    const ::DWORD length = screen.dwSize.X * screen.dwSize.Y;
    ::FillConsoleOutputCharacterA(out_, ' ', length, topLeft, &written);
    ::FillConsoleOutputAttribute(out_, screen.wAttributes, length, topLeft, &written);
    ::SetConsoleCursorPosition(out_, topLeft);
}

void sys_console::pause() const {
    write_string("press any key to continue...");
    read_char();
}

void sys_console::set_color(const integer32& color) const {
    write_string("\033[" + color.to_string() + "m");
}

void sys_console::reset_color() const {
    write_string("\033[0m");
}

MSTL_END_NAMESPACE__
