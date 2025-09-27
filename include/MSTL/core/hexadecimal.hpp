#ifndef MSTL_HEXADECIMAL_HPP__
#define MSTL_HEXADECIMAL_HPP__
#include "string.hpp"
#include "vector.hpp"
#include "optional.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__
class hexadecimal {
private:
    int64_t value;

    MSTL_CONSTEXPR20 static int64_t parse_hex(const string& str) {
        if (str.empty()) return 0;

        string s = str;
        bool negative = false;
        size_t start = 0;

        while (start < s.size() && _MSTL is_space(s[start])) ++start;
        if (start == s.size()) return 0;

        if (s[start] == '-') {
            negative = true;
            ++start;
        } else if (s[start] == '+') {
            ++start;
        }

        if (start + 1 < s.size() && s[start] == '0' &&
            (s[start+1] == 'x' || s[start+1] == 'X')) {
            start += 2;
        }

        string hex_digits;
        while (start < s.size()) {
            char c = s[start++];
            if (_MSTL is_xdigit(c)) {
                hex_digits += c;
            } else if (!_MSTL is_space(c)) {
                Exception(ValueError("Invalid hexadecimal character"));
            }
        }

        if (hex_digits.empty()) return 0;

        size_t pos = 0;
        const uint64_t raw = to_uint64(hex_digits.data(), &pos, 16);
        if (pos != hex_digits.size()) {
            Exception(ValueError("Invalid hexadecimal format"));
        }

        if (negative) {
            if (raw > static_cast<uint64_t>(INT64_MAX_SIZE) + 1) {
                Exception(ValueError("Hexadecimal value out of range"));
            }
            return -static_cast<long long>(raw);
        }
        if (raw > static_cast<uint64_t>(INT64_MAX_SIZE)) {
            Exception(ValueError("Hexadecimal value out of range"));
        }
        return static_cast<int64_t>(raw);
    }

public:
    constexpr hexadecimal() : value(0) {}
    constexpr hexadecimal(const int64_t v) : value(v) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const string& s) : value(parse_hex(s)) {}

    constexpr hexadecimal(const hexadecimal&) noexcept = default;
    constexpr hexadecimal& operator =(const hexadecimal&) noexcept = default;
    constexpr hexadecimal(hexadecimal&&) noexcept = default;
    constexpr hexadecimal& operator =(hexadecimal&&) noexcept = default;

    constexpr int64_t to_decimal() const { return value; }

    constexpr hexadecimal operator +(const hexadecimal& other) const { return {value + other.value}; }
    constexpr hexadecimal operator -(const hexadecimal& other) const { return {value - other.value}; }
    friend constexpr hexadecimal operator -(const hexadecimal& other) { return {-other.value}; }
    constexpr hexadecimal operator *(const hexadecimal& other) const { return {value * other.value}; }
    hexadecimal operator /(const hexadecimal& other) const {
        if (other.value == 0) Exception(MathError("Division by zero"));
        return {value / other.value};
    }
    hexadecimal operator %(const hexadecimal& other) const {
        if (other.value == 0) Exception(MathError("Modulo by zero"));
        return {value % other.value};
    }

    constexpr bool operator ==(const hexadecimal& other) const { return value == other.value; }
    constexpr bool operator !=(const hexadecimal& other) const { return value != other.value; }
    constexpr bool operator <(const hexadecimal& other) const { return value < other.value; }
    constexpr bool operator <=(const hexadecimal& other) const { return value <= other.value; }
    constexpr bool operator >(const hexadecimal& other) const { return value > other.value; }
    constexpr bool operator >=(const hexadecimal& other) const { return value >= other.value; }


    constexpr hexadecimal operator &(const hexadecimal& other) const { return {value & other.value}; }
    constexpr hexadecimal operator |(const hexadecimal& other) const { return {value | other.value}; }
    constexpr hexadecimal operator ^(const hexadecimal& other) const { return {value ^ other.value}; }
    constexpr hexadecimal operator ~() const { return {~value}; }

    hexadecimal operator <<(const int shift) const {
        if (shift < 0 || shift >= 64) Exception(ValueError("Shift count out of range"));
        return {value << shift};
    }
    hexadecimal operator >>(const int shift) const {
        if (shift < 0 || shift >= 64) Exception(ValueError("Shift count out of range"));
        return {value >> shift};
    }

    constexpr hexadecimal& operator &=(const hexadecimal& other) { value &= other.value; return *this; }
    constexpr hexadecimal& operator |=(const hexadecimal& other) { value |= other.value; return *this; }
    constexpr hexadecimal& operator ^=(const hexadecimal& other) { value ^= other.value; return *this; }
    hexadecimal& operator <<=(const int shift) { *this = *this << shift; return *this; }
    hexadecimal& operator >>=(const int shift) { *this = *this >> shift; return *this; }


    bool get_bit(const size_t position) const {
        if (position >= 64) Exception(ValueError("Bit position out of range"));
        return (value >> position) & 1;
    }

    hexadecimal& set_bit(const size_t position, bool bit_value = true) {
        if (position >= 64) Exception(ValueError("Bit position out of range"));
        if (bit_value) {
            value |= (1ULL << position);
        } else {
            value &= ~(1ULL << position);
        }
        return *this;
    }

    hexadecimal& flip_bit(const size_t position) {
        if (position >= 64) Exception(ValueError("Bit position out of range"));
        value ^= (1ULL << position);
        return *this;
    }

    int popcount() const {
        return _MSTL popcountll(static_cast<uint64_t>(value));
    }

    int clz() const {
        return _MSTL clzll(static_cast<uint64_t>(value));
    }

    static _MSTL optional<hexadecimal> try_parse(const string& str) noexcept {
        try {
            optional<hexadecimal> res;
            res.emplace(str);
            return res;
        } catch (...) {
            return nullopt;
        }
    }

    MSTL_CONSTEXPR20 _MSTL vector<uint8_t> to_bytes(const bool little_endian = true) const {
        vector<uint8_t> bytes;
        uint64_t val = static_cast<uint64_t>(value);

        if (little_endian) {
            for (int i = 0; i < 8; ++i) {
                bytes.push_back(static_cast<uint8_t>(val & 0xFF));
                val >>= 8;
            }
        } else {
            for (int i = 7; i >= 0; --i) {
                bytes.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
            }
        }
        return bytes;
    }

    static hexadecimal from_bytes(const uint8_t* bytes, size_t count, bool little_endian = true) {
        if (count > 8) Exception(ValueError("Too many bytes for int64_t"));

        int64_t result = 0;
        if (little_endian) {
            for (size_t i = 0; i < count; ++i) {
                result |= static_cast<int64_t>(bytes[i]) << (i * 8);
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                result = (result << 8) | bytes[i];
            }
        }
        return hexadecimal(result);
    }

    MSTL_CONSTEXPR20 string to_string() const;
};
MSTL_END_INNER__

using hexadecimal = _INNER hexadecimal;


enum class hex_alignment {
    left,
    right,
    internal
};

enum class hex_format {
    lowercase = 0x01,
    uppercase = 0x02,
    no_prefix = 0x04,
    with_prefix = 0x08,
    zero_padded = 0x10
};

struct hex_format_options {
    char fill = ' ';
    int width = 0;
    hex_alignment alignment = hex_alignment::right;
    bool show_prefix = true;
    hex_format format_flags = hex_format::lowercase;
    bool zero_padded = false;
    bool show_positive = false;
};

struct setfill_t { char c; };
struct setw_t { int w; };
struct setalignment_t { hex_alignment alignment; };
struct setprefix_t { bool show; };
struct setcase_t { hex_format case_flag; };
struct setzeropad_t { bool on; };
struct setshowpos_t { bool show; };

constexpr setfill_t setfill(const char c) { return {c}; }
constexpr setw_t setw(const int w) { return {w}; }
constexpr setalignment_t setalignment(const hex_alignment a) { return {a}; }
constexpr setprefix_t setprefix(const bool show) { return {show}; }
constexpr setcase_t setcase(const hex_format case_flag) { return {case_flag}; }
constexpr setzeropad_t setzeropad(const bool on) { return {on}; }
constexpr setshowpos_t setshowpos(const bool show) { return {show}; }

constexpr setalignment_t left() { return {hex_alignment::left}; }
constexpr setalignment_t right() { return {hex_alignment::right}; }
constexpr setalignment_t internal() { return {hex_alignment::internal}; }

constexpr setcase_t uppercase() { return {hex_format::uppercase}; }
constexpr setcase_t lowercase() { return {hex_format::lowercase}; }

constexpr setprefix_t noshowbase() { return {false}; }
constexpr setprefix_t showbase() { return {true}; }


constexpr void set_options(hex_format_options& opt, setfill_t set) {
    opt.fill = set.c;
}

constexpr void set_options(hex_format_options& opt, setw_t set) {
    opt.width = set.w;
}

constexpr void set_options(hex_format_options& opt, const setalignment_t set) {
    opt.alignment = set.alignment;
}

constexpr void set_options(hex_format_options& opt, const setprefix_t set) {
    opt.show_prefix = set.show;
}

constexpr void set_options(hex_format_options& opt, const setcase_t set) {
    opt.format_flags = set.case_flag;
}

constexpr void set_options(hex_format_options& opt, const setzeropad_t set) {
    opt.zero_padded = set.on;
}

constexpr void set_options(hex_format_options& opt, const setshowpos_t set) {
    opt.show_positive = set.show;
}


MSTL_BEGIN_INNER__

MSTL_CONSTEXPR20 string __format_hex_impl(const hexadecimal& hex, hex_format_options options) {
    if (options.zero_padded) {
        options.fill = '0';
        if (options.alignment != hex_alignment::left) {
            options.alignment = hex_alignment::internal;
        }
    }

    const int64_t value = hex.to_decimal();
    const bool is_negative = value < 0;
    uint64_t abs_value = is_negative ? static_cast<uint64_t>(-value) : static_cast<uint64_t>(value);

    const char* hex_digits = (options.format_flags == hex_format::uppercase) ?
        "0123456789ABCDEF" : "0123456789abcdef";

    string digits;
    if (abs_value == 0) {
        digits = "0";
    } else {
        while (abs_value > 0) {
            digits = hex_digits[abs_value % 16] + digits;
            abs_value /= 16;
        }
    }

    string prefix;
    if (is_negative) {
        prefix += "-";
    } else if (options.show_positive) {
        prefix += "+";
    }
    if (options.show_prefix) {
        prefix += (options.format_flags == hex_format::uppercase) ? "0X" : "0x";
    }

    string result = prefix + digits;
    if (result.size() >= static_cast<size_t>(options.width)) {
        return result;
    }
    const size_t fill_count = options.width - result.size();
    const string fill_str(fill_count, options.fill);

    switch (options.alignment) {
        case hex_alignment::left:
            return result + fill_str;
        case hex_alignment::right:
            return fill_str + result;
        case hex_alignment::internal:
            return prefix + fill_str + digits;
        default:
            return fill_str + result;
    }
}
MSTL_END_INNER__

template <typename... Args>
MSTL_CONSTEXPR20 string format_hex(const hexadecimal& hex, Args&&... args) {
    hex_format_options options;
    const int unused[] = {0, (_MSTL set_options(options, std::forward<Args>(args)), 0)...};
    (void) unused;
    return __format_hex_impl(hex, options);
}

template <typename T>
MSTL_CONSTEXPR20 string format_hex(const hexadecimal& hex, T&& arg) {
    hex_format_options options;
    _MSTL set_options(options, arg);
    return __format_hex_impl(hex, options);
}

MSTL_CONSTEXPR20 string format_hex(const hexadecimal& hex) {
    constexpr hex_format_options options;
    return __format_hex_impl(hex, options);
}


MSTL_CONSTEXPR20 string hex_string(const hexadecimal& hex) {
    return format_hex(hex, setprefix(true), lowercase());
}

MSTL_CONSTEXPR20 string HEX_string(const hexadecimal& hex) {
    return format_hex(hex, setprefix(true), uppercase());
}

MSTL_CONSTEXPR20 string hex_fixed(const hexadecimal& hex, int width) {
    return format_hex(hex, setw(width), setzeropad(true), setprefix(true));
}

MSTL_CONSTEXPR20 string hex_plain(const hexadecimal& hex) {
    return format_hex(hex, setprefix(false));
}


MSTL_CONSTEXPR20 string hexadecimal::to_string() const {
    return hex_string(*this);
}

MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string(const hexadecimal& x) {
    return x.to_string();
}


inline namespace literals {
    MSTL_CONSTEXPR20 hexadecimal operator ""_hex(const char* str, const size_t len) {
        return hexadecimal(string(str, len));
    }

    constexpr hexadecimal operator ""_hex(const unsigned long long value) {
        return hexadecimal(static_cast<long long>(value));
    }
}


MSTL_END_NAMESPACE__
#endif // MSTL_HEXADECIMAL_HPP__
