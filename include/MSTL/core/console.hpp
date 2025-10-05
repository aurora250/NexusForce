#ifndef MSTL_CONSOLE_HPP__
#define MSTL_CONSOLE_HPP__
#include "object.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename = void>
struct io_base {
    static void write(const class sys_console& console, const T& value);
    static void read(const sys_console& console, T& value);
};


class MSTL_API sys_console {
private:
    ::HANDLE out_;
    ::HANDLE in_;

public:
    sys_console();

    void flush() const;

    void write_string(const string& str) const;
    void write_string(const string_view& str) const;
    void write_string(const char* str) const;

    string readln_string() const;
    char read_char() const;

    template <typename T>
    void print(const T& value) {
        io_base<T>::write(*this, value);
    }
    template <typename... Args>
    void print(const string_view& fmt, Args&&... args) {
        sys_console::print(_MSTL format(fmt, _MSTL forward<Args>(args)...));
    }

    void println() const {
        write_string("\n");
    }
    template <typename T>
    void println(const T& value) {
        sys_console::print(value);
        println();
    }
    template <typename... Args>
    void println(const string_view& fmt, Args&&... args) {
        sys_console::println(_MSTL format(fmt, _MSTL forward<Args>(args)...));
    }

    template <typename T, enable_if_t<!is_packaged_v<T>, int> = 0>
    T read() {
        T value;
        io_base<T>::read(*this, value);
        return _MSTL move(value);
    }

    template <typename T, enable_if_t<is_packaged_v<T>, int> = 0>
    T read() {
        package_t<T> obj;
        io_base<T>::read(*this, obj);
        return obj.value();
    }

    template <typename T>
    void read(T& value) {
        io_base<T>::read(*this, value);
    }

    template <typename T, enable_if_t<!is_packaged_v<T> && !is_base_of_v<object<T>, T>, int> = 0>
        T readln() {
        return T(readln_string());
    }
    template <typename T, enable_if_t<!is_packaged_v<T> && is_base_of_v<object<T>, T>, int> = 0>
    T readln() {
        T obj;
        string line = readln_string();
        obj.try_parse(line);
        return _MSTL move(obj);
    }
    template <typename T, enable_if_t<is_packaged_v<T>, int> = 0>
    T readln() {
        package_t<T> obj;
        string line = readln_string();
        obj.try_parse(line);
        return static_cast<T>(obj);
    }

    template <typename T>
    void readln(T& value) {
        value = _MSTL move(sys_console::readln<T>());
    }

    void clear() const;
    void pause() const;

    void set_color(const integer32& color) const;
    void reset_color() const;
};


template <typename T, typename U>
void io_base<T, U>::write(const sys_console& console, const T& value) {
    console.write_string("@" + _MSTL address_string(&value));
}
template <typename T, typename U>
void io_base<T, U>::read(const sys_console& console, T&) {
    console.write_string("\nio_base::read not specialized for this type");
    assert(false);
}


template <typename T>
struct io_base<T, enable_if_t<is_base_of_v<object<T>, T>>> {
    static void write(const sys_console& console, const object<T>& value) {
        console.write_string(value.to_string());
    }
    static void read(const sys_console& console, object<T>& value) {
        static_cast<T&>(value) = object<T>::parse(console.readln_string());
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_packaged_v<T>>> {
    static void write(const sys_console& console, const T& value) {
        console.write_string(_MSTL to_string(value));
    }
    static void read(const sys_console& console, T& value) {
        auto obj = _MSTL make_package<T>(_MSTL forward<T>(value));
        obj = decltype(obj)::parse(console.readln_string());
        value = static_cast<T>(obj);
    }
};

template <>
struct io_base<void> {
    static void write(const sys_console& console, ...) {
        console.write_string("void");
    }
    static void feature(const sys_console& console, ...) {
        console.write_string("(void)");
    }
};


static sys_console console;


// template <typename This, typename ...Rests>
// void print(const This& t, const Rests&... r) {
//     console.print<remove_cvref_t<This>>(t);
//     ((console.print(" "), console.print<remove_cvref_t<Rests>>(r)), ...);
// }
//
// MSTL_ALWAYS_INLINE inline void print() {}
//
// template <typename This, typename ...Rests>
// void println(const This& t, const Rests&... r) {
//     console.print<remove_cvref_t<This>>(t);
//     ((console.print(" "), console.print<remove_cvref_t<Rests>>(r)), ...);
//     console.println();
// }
//
// MSTL_ALWAYS_INLINE inline void println() {
//     console.println();
// }

MSTL_END_NAMESPACE__
#endif // MSTL_CONSOLE_HPP__
