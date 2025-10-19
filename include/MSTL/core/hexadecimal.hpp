#ifndef MSTL_HEXADECIMAL_HPP__
#define MSTL_HEXADECIMAL_HPP__
#include "serialize.hpp"
MSTL_BEGIN_NAMESPACE__

struct hexadecimal : iserialize<hexadecimal>, iarithmetic<hexadecimal>, ibinary<hexadecimal> {
public:
    using self = hexadecimal;
    using value_type = int64_t;

private:
    value_type value_ = 0;

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
                Exception(ValueError("Invalid hexadecimal character"));
            }
        }

        if (hex_digits.empty()) return 0;

        size_t pos = 0;
        const uint64_t raw = _MSTL to_uint64(hex_digits.data(), &pos, 16);
        if (pos != hex_digits.size()) {
            Exception(ValueError("Invalid hexadecimal format"));
        }

        if (negative) {
            if (raw > static_cast<uint64_t>(numeric_limits<int64_t>::max()) + 1) {
                Exception(ValueError("Hexadecimal value out of range"));
            }
            return -static_cast<int64_t>(raw);
        }
        if (raw > static_cast<uint64_t>(numeric_limits<int64_t>::max())) {
            Exception(ValueError("Hexadecimal value out of range"));
        }
        return static_cast<value_type>(raw);
    }

public:
    constexpr hexadecimal() noexcept = default;
    explicit constexpr hexadecimal(const short v) noexcept : value_(v) {}
    explicit constexpr hexadecimal(const int v) noexcept : value_(v) {}
    explicit constexpr hexadecimal(const long v) noexcept : value_(v) {}
    explicit constexpr hexadecimal(const long long v) noexcept : value_(v) {}
    explicit constexpr hexadecimal(const integer64& v) noexcept : value_(v.value()) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const string_view s) : value_(parse_hex(s)) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const char* s) : hexadecimal(string_view(s)) {}
    MSTL_CONSTEXPR20 explicit hexadecimal(const string& s) : hexadecimal(s.view()) {}

    constexpr hexadecimal(const hexadecimal&) noexcept = default;
    constexpr hexadecimal& operator =(const hexadecimal&) noexcept = default;
    constexpr hexadecimal(hexadecimal&&) noexcept = default;
    constexpr hexadecimal& operator =(hexadecimal&&) noexcept = default;

    MSTL_CONSTEXPR20 ~hexadecimal() = default;

    MSTL_NODISCARD explicit constexpr operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }

    constexpr self operator +(const self& other) const noexcept { return self{value_ + other.value_}; }
    constexpr self operator -(const self& other) const noexcept { return self{value_ - other.value_}; }
    constexpr self operator -() const noexcept { return self{-value_}; }
    constexpr self operator *(const self& other) const noexcept { return self{value_ * other.value_}; }
    constexpr self operator /(const self& other) const {
        if (other.value_ == 0) Exception(MathError("Division by zero"));
        return self{value_ / other.value_};
    }
    constexpr self operator %(const self& other) const {
        if (other.value_ == 0) Exception(MathError("Modulo by zero"));
        return self{value_ % other.value_};
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr self& operator+=(const self& other) noexcept {
        value_ += other. value_;
        return *this;
    }
    constexpr self& operator-=(const self& other) noexcept {
        value_ -= other. value_;
        return *this;
    }
    constexpr self& operator*=(const self& other) noexcept {
        value_ *= other. value_;
        return *this;
    }
    constexpr self& operator/=(const self& other) {
        *this = *this / other;
        return *this;
    }
    constexpr self& operator%=(const self& other) {
        *this = *this % other;
        return *this;
    }

    constexpr self& operator++() {
        ++value_;
        return *this;
    }
    constexpr self operator++(int) {
        const self temp(*this);
        ++value_;
        return temp;
    }
    constexpr self& operator--() {
        --value_;
        return *this;
    }
    constexpr self operator--(int) {
        const self temp(*this);
        --value_;
        return temp;
    }

    constexpr self operator &(const self& other) const noexcept { return self{value_ & other.value_}; }
    constexpr self operator |(const self& other) const noexcept { return self{value_ | other.value_}; }
    constexpr self operator ^(const self& other) const noexcept { return self{value_ ^ other.value_}; }
    constexpr self operator ~() const noexcept { return self{~value_}; }
    constexpr self operator <<(const uint32_t shift) const {
        if (shift >= 64) Exception(ValueError("Shift count out of range"));
        return self{value_ << shift};
    }
    constexpr self operator >>(const uint32_t shift) const {
        if (shift >= 64) Exception(ValueError("Shift count out of range"));
        return self{value_ >> shift};
    }

    constexpr self& operator &=(const self& other) noexcept { value_ &= other.value_; return *this; }
    constexpr self& operator |=(const self& other) noexcept { value_ |= other.value_; return *this; }
    constexpr self& operator ^=(const self& other) noexcept { value_ ^= other.value_; return *this; }
    constexpr self& operator <<=(const uint32_t shift) { *this = *this << shift; return *this; }
    constexpr self& operator >>=(const uint32_t shift) { *this = *this >> shift; return *this; }


    bool get_bit(const size_t position) const {
        if (position >= 64) Exception(ValueError("Bit position out of range"));
        return (value_ >> position) & 1;
    }
    constexpr hexadecimal& set_bit(const size_t position, const bool bit_value_ = true) {
        if (position >= 64) Exception(ValueError("Bit position out of range"));
        if (bit_value_) {
            value_ |= (1ULL << position);
        } else {
            value_ &= ~(1ULL << position);
        }
        return *this;
    }
    constexpr hexadecimal& flip_bit(const size_t position) {
        if (position >= 64) Exception(ValueError("Bit position out of range"));
        value_ ^= (1ULL << position);
        return *this;
    }

    MSTL_NODISCARD int popcount() const { return _MSTL popcountll(static_cast<size_t>(value_)); }
    MSTL_NODISCARD int clz() const { return _MSTL clzll(static_cast<size_t>(value_)); }

    MSTL_NODISCARD constexpr int64_t to_int64() const noexcept { return value_; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept { return hash<value_type>()(value_); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const;

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view str) { return self(str); }
    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        try {
            value_ = parse_hex(str);
            return true;
        } catch (...) {
            return false;
        }
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }
};

template <>
struct unpackage<hexadecimal> {
    using type = int64_t;
};


template <>
struct formatter<hexadecimal> {
    MSTL_CONSTEXPR20 string operator()(const hexadecimal& value, const format_options& options) const {
        const int64_t val = value.to_int64();
        const bool is_negative = val < 0;
        uint64_t abs_value = is_negative ? static_cast<uint64_t>(-val) : static_cast<uint64_t>(val);

        const char* digits;
        int base;
        switch (options.type) {
            case NUMBER_TYPE::BINARY: {
                digits = "01";
                base = 2;
                break;
            }
            case NUMBER_TYPE::OCTAL: {
                digits = "01234567";
                base = 8;
                break;
            }
            case NUMBER_TYPE::HEX_UP: {
                digits = "0123456789ABCDEF";
                base = 16;
                break;
            }
            case NUMBER_TYPE::HEX_LOW: default: {
                digits = "0123456789abcdef";
                base = 16;
                break;
            }
        }

        string number_str;
        if (abs_value == 0) {
            number_str = "0";
        } else {
            while (abs_value > 0) {
                number_str = digits[abs_value % base] + number_str;
                abs_value /= base;
            }
        }

        string prefix;
        if (is_negative) {
            prefix += "-";
        } else if (options.sign_mode == '+') {
            prefix += "+";
        } else if (options.sign_mode == ' ') {
            prefix += " ";
        }

        if (options.show_base) {
            switch (options.type) {
                case NUMBER_TYPE::BINARY: prefix += "0b"; break;
                case NUMBER_TYPE::OCTAL: prefix += "0o"; break;
                case NUMBER_TYPE::HEX_LOW: prefix += "0x"; break;
                case NUMBER_TYPE::HEX_UP: prefix += "0X"; break;
                default: break;
            }
        }

        string result = prefix + number_str;
        if (options.width <= 0 || result.size() >= static_cast<size_t>(options.width)) {
            return result;
        }

        const size_t fill_count = options.width - result.size();
        const string fill_str(fill_count, options.fill);

        switch (options.alignment) {
            case NUMBER_ALIGN::LEFT: return result + fill_str;
            case NUMBER_ALIGN::RIGHT: return fill_str + result;
            case NUMBER_ALIGN::INTERNAL: return prefix + fill_str + number_str;
            case NUMBER_ALIGN::CENTER: {
                const size_t left_fill = fill_count / 2;
                const size_t right_fill = fill_count - left_fill;
                return string(left_fill, options.fill) + result + string(right_fill, options.fill);
            }
            default: return fill_str + result;
        }
    }
};


MSTL_CONSTEXPR20 string hexadecimal::to_string() const {
    return _MSTL format("{#x}", *this);
}


MSTL_BEGIN_LITERALS__

MSTL_CONSTEXPR20 hexadecimal operator ""_hex(const char* str, const size_t len) {
    return hexadecimal(string_view(str, len));
}
constexpr hexadecimal operator ""_hex(const unsigned long long value) {
    return hexadecimal(static_cast<long long>(value));
}

MSTL_END_LITERALS__

MSTL_END_NAMESPACE__
#endif // MSTL_HEXADECIMAL_HPP__
