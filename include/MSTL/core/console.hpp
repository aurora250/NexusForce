#ifndef MSTL_CONSOLE_HPP__
#define MSTL_CONSOLE_HPP__
#include "color.hpp"
#include "mutex.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename = void>
struct io_base;


class MSTL_API sys_console {
#ifdef MSTL_PLATFORM_WINDOWS__
    ::HANDLE out_;
    ::HANDLE in_;
#else
    int out_;
    int in_;
#endif
    mutex mutex_{};

    friend sys_console& get_console();

private:
    void write_string_unsafe(const string& str) const { write_string_unsafe(str.view()); }
    void write_string_unsafe(string_view str) const;
    void write_string_unsafe(const char* str) const { write_string_unsafe(string_view{str}); }

    string readln_string_unsafe() const;
    char read_char_unsafe() const;

    void flush_unsafe() const;

    void init_console();

private:
    sys_console() { this->init_console(); }

    explicit sys_console(const color& color, bool use_256_color = true)  {
        this->init_console();
        this->set_color(color, use_256_color);
    }

public:
    sys_console(const sys_console&) = delete;
    sys_console& operator=(const sys_console&) = delete;
    sys_console(sys_console&&) = delete;
    sys_console& operator=(sys_console&&) = delete;

    ~sys_console() = default;

    void flush() {
        lock_guard<mutex> lock(mutex_);
        this->flush_unsafe();
    }

    void write_string(const string& str) {
        lock_guard<mutex> lock(mutex_);
        this->write_string_unsafe(str.view());
    }
    void write_string(const string_view& str) {
        lock_guard<mutex> lock(mutex_);
        this->write_string_unsafe(str);
    }
    void write_string(const char* str) {
        lock_guard<mutex> lock(mutex_);
        this->write_string_unsafe(str);
    }


    string readln_string()  {
        lock_guard<mutex> lock(mutex_);
        return this->readln_string_unsafe();
    }

    char read_char() {
        lock_guard<mutex> lock(mutex_);
        return this->read_char_unsafe();
    }


    template <typename T>
    void print(const T& value) {
        lock_guard<mutex> lock(mutex_);
        io_base<T>::write(*this, value);
    }
    template <typename... Args>
    void printf(const string_view fmt, Args&&... args) {
        this->write_string(_MSTL format(fmt, _MSTL forward<Args>(args)...));
    }
    template <typename T>
    void printc(const color& color, const T& value) {
        lock_guard<mutex> lock(mutex_);
        this->write_string_unsafe("\033[" + _MSTL to_string(color.to_ansi_basic(false)) + "m");
        io_base<T>::write(*this, value);
        this->write_string_unsafe("\033[0m");
    }

    void println() {
        this->write_string("\n");
    }
    template <typename T>
    void println(const T& value) {
        this->print(value);
        this->println();
    }
    template <typename T>
    void printcln(const color& color, const T& value) {
        this->printc(color, value);
        this->println();
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

    template <typename T, enable_if_t<!is_packaged_v<T> && !is_base_of_v<iserialize<T>, T>, int> = 0>
    T readln() {
        return T(this->readln_string());
    }
    template <typename T, enable_if_t<!is_packaged_v<T> && is_base_of_v<iserialize<T>, T>, int> = 0>
    T readln() {
        T obj;
        obj.try_parse(this->readln_string().view());
        return _MSTL move(obj);
    }
    template <typename T, enable_if_t<is_packaged_v<T>, int> = 0>
    T readln() {
        package_t<T> obj;
        obj.try_parse(this->readln_string().view());
        return static_cast<T>(obj);
    }

    template <typename T>
    void readln(T& value) {
        value = _MSTL move(this->readln<T>());
    }

    void clear();
    void pause(string_view msg = "press any key to continue...");

    void set_color(const integer32& color);
    void set_color(const color& color, bool use_256_color = true);
    void set_background_color(const color& color, bool use_256_color = true);
    void reset_color();
};


template <typename T>
struct io_base<T, enable_if_t<is_base_of_v<_MSTL_RANGES view_base<T>, T>>> {
    static void write(sys_console& console, const T& value) {
        string result;
        if (value.begin() == value.end()) {
            result = "[]";
        } else {
            result += "[ ";
            for (auto iter = value.begin(); iter != value.end(); ++iter) {
                if (iter != value.begin()) result += ", ";
                result += to_string(*iter);
            }
            result += " ]";
        }
        console.write_string(result.view());
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_base_of_v<istringify<T>, T> && !is_base_of_v<iserialize<T>, T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(value.to_string());
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_base_of_v<iserialize<T>, T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(value.to_string());
    }
    static void read(sys_console& console, T& value) {
        value = iserialize<T>::parse(console.readln_string().view());
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_packaged_v<T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(_MSTL to_string(value));
    }
    static void read(sys_console& console, T& value) {
        value = package_t<T>::parse(console.readln_string().view());
    }
};


template <typename T>
struct io_base<T, enable_if_t<is_null_pointer_v<T>>> {
    static void write(sys_console& console, nullptr_t) {
        console.write_string(_MSTL to_string(nullptr));
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_pointer_v<T> && !is_cstring_v<T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename T, size_t N>
struct io_base<T[N], enable_if_t<is_character_v<T>>> {
    static void write(sys_console& console, const T(& value)[N]) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename T, size_t N>
struct io_base<const T[N], enable_if_t<is_character_v<T>>> {
    static void write(sys_console& console, const T(& value)[N]) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename T>
struct io_base<const T*, enable_if_t<is_character_v<T>>> {
    static void write(sys_console& console, const T* value) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename CharT, typename Traits>
struct io_base<basic_string_view<CharT, Traits>> {
    static void write(sys_console& console, const basic_string_view<CharT, Traits>& value) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename CharT, typename Traits, typename Alloc>
struct io_base<basic_string<CharT, Traits, Alloc>> {
    static void write(sys_console& console, const basic_string<CharT, Traits, Alloc>& value) {
        console.write_string(_MSTL to_string(value));
    }
    static void read(sys_console& console, basic_string<CharT, Traits, Alloc>& value) {
        value = _MSTL move(console.readln_string());
    }
};


template <typename T>
struct io_base<T, enable_if_t<is_union_v<T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(_MSTL to_string(value));
    }
};


template <typename T>
struct io_base<T, enable_if_t<is_unbounded_array_v<T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_bounded_array_v<T> && !is_cstring_v<T>>> {
    static void write(sys_console& console, const T& value) {
        console.write_string(_MSTL to_string(value));
    }
};

template <typename T>
struct io_base<T, enable_if_t<is_base_of_v<Error,T>>> {
    static void write(sys_console& console, const T& value) {
        io_base<string>::write(console, _MSTL to_string(value));
    }
};

template <typename IfEmpty, typename T, bool Compressed>
struct io_base<compressed_pair<IfEmpty, T, Compressed>> {
    static void write(sys_console& console, const compressed_pair<IfEmpty, T, Compressed>& value) {
        io_base<string>::write(console, _MSTL to_string(value));
    }
};

template <typename T1, typename T2>
struct io_base<pair<T1, T2>> {
    static void write(sys_console& console, const pair<T1, T2>& value) {
        io_base<string>::write(console, _MSTL to_string(value));
    }
};

template<typename... Args> struct io_base<tuple<Args...>> {
    static void write(sys_console &console, const tuple<Args...> &value) {
        io_base<string>::write(console, _MSTL to_string(value));
    }
};


inline sys_console& get_console() {
    static sys_console console;
    return console;
}

static sys_console& console = get_console();


#ifndef MSTL_STANDARD_17__

MSTL_BEGIN_INNER__
MSTL_ALWAYS_INLINE_INLINE void print_rests() {}

template <typename First, typename... Rests>
void print_rests(const First& first, const Rests&... rests) {
    console.print(" ");
    console.print<remove_cvref_t<First>>(first);
    _INNER print_rests(rests...);
}

MSTL_ALWAYS_INLINE_INLINE void printc_rests(const color&) {}

template <typename First, typename... Rests>
void printc_rests(const color& color, const First& first, const Rests&... rests) {
    console.print(" ");
    console.printc<remove_cvref_t<First>>(color, first);
    _INNER printc_rests(color, rests...);
}
MSTL_END_INNER__

template <typename This>
void print(const This& t) {
    console.print<remove_cvref_t<This>>(t);
}

template <typename This, typename... Rests>
void print(const This& t, const Rests&... rests) {
    console.print<remove_cvref_t<This>>(t);
    _INNER print_rests(rests...);
}

template <typename This>
void printc(const color& color, const This& t) {
    console.print<remove_cvref_t<This>>(color, t);
}

template <typename This, typename... Rests>
void printc(const color& color, const This& t, const Rests&... rests) {
    console.printc<remove_cvref_t<This>>(color, t);
    _INNER printc_rests(color, rests...);
}

MSTL_ALWAYS_INLINE_INLINE void println() {
    console.println();
}

template <typename This>
void println(const This& t) {
    console.print<remove_cvref_t<This>>(t);
    println();
}

template <typename This, typename... Rests>
void println(const This& t, const Rests&... rests) {
    console.print<remove_cvref_t<This>>(t);
    _INNER print_rests(rests...);
    println();
}

template <typename This, typename ...Rests>
void printcln(const color& color, const This& t, const Rests&... rests) {
    console.printc<remove_cvref_t<This>>(color, t);
    _INNER printc_rests(color, rests...);
    println();
}

#else

template <typename This, typename ...Rests>
void print(const This& t, const Rests&... r) {
    console.print<remove_cvref_t<This>>(t);
    ((console.print(" "), console.print<remove_cvref_t<Rests>>(r)), ...);
}

template <typename This, typename ...Rests>
void printc(const color& color, const This& t, const Rests&... r) {
    console.printc<remove_cvref_t<This>>(color, t);
    ((console.print(" "), console.printc<remove_cvref_t<Rests>>(color, r)), ...);
}

MSTL_ALWAYS_INLINE_INLINE void println() {
    console.println();
}

template <typename This, typename ...Rests>
void println(const This& t, const Rests&... r) {
    console.print<remove_cvref_t<This>>(t);
    ((console.print(" "), console.print<remove_cvref_t<Rests>>(r)), ...);
    println();
}

template <typename This, typename ...Rests>
void printcln(const color& color, const This& t, const Rests&... r) {
    console.printc<remove_cvref_t<This>>(color, t);
    ((console.print(" "), console.printc<remove_cvref_t<Rests>>(color, r)), ...);
    println();
}

#endif

template <typename... Args>
void printf(const string_view fmt, Args&&... args) {
    console.printf(fmt, _MSTL forward<Args>(args)...);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CONSOLE_HPP__
