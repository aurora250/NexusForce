#ifndef MSTL_CORE_UTILITY_HEXADECIMAL_HPP__
#define MSTL_CORE_UTILITY_HEXADECIMAL_HPP__
#include "../config/undef_cmacro.hpp"
#include "../interface/inumeric.hpp"
#include "../interface/iobject.hpp"
#include "../string/format.hpp"
#include "../string/to_numerics.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API hexadecimal : iobject<hexadecimal>, ipackage<hexadecimal, int64_t> {
public:
    using value_type = int64_t;
    using base = ipackage<hexadecimal, int64_t>;

private:

    static MSTL_CONSTEXPR20 value_type parse_hex(const string_view s) {
        if (s.empty()) return 0;

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
            const char c = s[start++];
            if (_MSTL is_xdigit(c)) {
                hex_digits += c;
            } else if (!_MSTL is_space(c)) {
                throw_exception(value_exception("Invalid hexadecimal character"));
            }
        }

        if (hex_digits.empty()) return 0;

        size_t pos = 0;
        const uint64_t raw = _MSTL to_uint64(hex_digits.view(), &pos, 16);
        if (pos != hex_digits.size()) {
            throw_exception(value_exception("Invalid hexadecimal format"));
        }

        if (negative) {
            if (raw > static_cast<uint64_t>(numeric_traits<int64_t>::max()) + 1) {
                throw_exception(value_exception("Hexadecimal value out of range"));
            }
            return -static_cast<int64_t>(raw);
        }
        if (raw > static_cast<uint64_t>(numeric_traits<int64_t>::max())) {
            throw_exception(value_exception("Hexadecimal value out of range"));
        }
        return static_cast<value_type>(raw);
    }

public:
    explicit constexpr hexadecimal(const int16_t v) noexcept : base(v) {}
    explicit constexpr hexadecimal(const int32_t v) noexcept : base(v) {}
    explicit constexpr hexadecimal(const uint16_t v) noexcept : base(v) {}
    explicit constexpr hexadecimal(const uint32_t v) noexcept : base(v) {}
    explicit constexpr hexadecimal(const uint64_t v) noexcept : base(v) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const string_view s) : base(parse_hex(s)) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const char* s) : hexadecimal(string_view(s)) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const string& s) : hexadecimal(s.view()) {}

    MSTL_BUILD_PACKAGE_CONSTRUCTOR(hexadecimal)

    MSTL_NODISCARD explicit constexpr operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }

    MSTL_NODISCARD constexpr bool get_bit(const size_t position) const {
        if (position >= 64) throw_exception(value_exception("Bit position out of range"));
        return (value_ >> position) & 1;
    }

    constexpr hexadecimal& set_bit(const size_t position, const bool bit_value_ = true) {
        if (position >= 64) throw_exception(value_exception("Bit position out of range"));
        if (bit_value_) {
            value_ |= (1ULL << position);
        } else {
            value_ &= ~(1ULL << position);
        }
        return *this;
    }

    constexpr hexadecimal& flip_bit(const size_t position) {
        if (position >= 64) throw_exception(value_exception("Bit position out of range"));
        value_ ^= (1ULL << position);
        return *this;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const;

    MSTL_NODISCARD static MSTL_CONSTEXPR20 hexadecimal parse(const string_view str) {
        return hexadecimal(str);
    }
};

template <>
struct unpackage<hexadecimal> {
    using type = int64_t;
};


template <>
struct formatter<hexadecimal> {
    MSTL_CONSTEXPR20 string operator ()(const hexadecimal& value, const format_options& options) const {
        return formatter<int64_t>()(value.value(), options);
    }
};

MSTL_CONSTEXPR20 string hexadecimal::to_string() const {
    return _MSTL format("{#x}", *this);
}


MSTL_CONSTEXPR20 hexadecimal to_hexadecimal(const string_view sv) {
    return hexadecimal::parse(sv);
}


MSTL_BEGIN_LITERALS__

MSTL_CONSTEXPR20 hexadecimal operator ""_hex(const char* str, const size_t len) {
    return hexadecimal{string_view(str, len)};
}
constexpr hexadecimal operator ""_hex(const unsigned long long value) {
    return {static_cast<int64_t>(value)};
}

MSTL_END_LITERALS__

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_HEXADECIMAL_HPP__
