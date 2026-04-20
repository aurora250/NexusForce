#ifndef NEFORCE_CORE_NUMERIC_INT128_HPP__
#define NEFORCE_CORE_NUMERIC_INT128_HPP__
#include "NeForce/core/interface/istringify.hpp"
#ifdef NEFORCE_SUPPORT_INT128
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API uint128_t : icommon<uint128_t> {
public:
    uint64_t lo;
    uint64_t hi;

    constexpr uint128_t() noexcept :
    lo(0),
    hi(0) {}

    constexpr uint128_t(uint64_t low) noexcept :
    lo(low),
    hi(0) {}

    constexpr uint128_t(uint64_t high, uint64_t low) noexcept :
    lo(low),
    hi(high) {}

    constexpr uint128_t(const uint128_t&) noexcept = default;
    constexpr uint128_t& operator=(const uint128_t&) noexcept = default;

    explicit uint128_t(const string& str, int base = 10);
    explicit uint128_t(const char* str, int base = 10);

    explicit operator bool() const noexcept { return lo || hi; }
    explicit operator uint64_t() const noexcept { return lo; }
    explicit operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }

    bool operator==(const uint128_t& rhs) const noexcept {
        return hi == rhs.hi && lo == rhs.lo;
    }
    bool operator<(const uint128_t& rhs) const noexcept {
        return hi < rhs.hi || (hi == rhs.hi && lo < rhs.lo);
    }

    uint128_t operator+(const uint128_t& other) const;
    uint128_t operator-(const uint128_t& other) const;
    uint128_t operator*(const uint128_t& other) const;
    uint128_t operator/(const uint128_t& other) const;
    uint128_t operator%(const uint128_t& other) const;

    uint128_t& operator+=(const uint128_t& other);
    uint128_t& operator-=(const uint128_t& other);
    uint128_t& operator*=(const uint128_t& other);
    uint128_t& operator/=(const uint128_t& other);
    uint128_t& operator%=(const uint128_t& other);

    uint128_t& operator++() noexcept;
    uint128_t operator++(int) noexcept;
    uint128_t& operator--() noexcept;
    uint128_t operator--(int) noexcept;

    uint128_t operator~() const noexcept;
    uint128_t operator&(const uint128_t& other) const noexcept;
    uint128_t operator|(const uint128_t& other) const noexcept;
    uint128_t operator^(const uint128_t& other) const noexcept;
    uint128_t operator<<(unsigned int shift) const noexcept;
    uint128_t operator>>(unsigned int shift) const noexcept;

    uint128_t& operator&=(const uint128_t& other) noexcept;
    uint128_t& operator|=(const uint128_t& other) noexcept;
    uint128_t& operator^=(const uint128_t& other) noexcept;
    uint128_t& operator<<=(unsigned int shift) noexcept;
    uint128_t& operator>>=(unsigned int shift) noexcept;

    static uint128_t mul64(uint64_t a, uint64_t b) noexcept;
    uint64_t div64(uint64_t divisor, uint64_t* remainder = nullptr) const noexcept;

    size_t to_hash() const noexcept {
        return hash<uint64_t>()(lo) ^ (hash<uint64_t>()(hi) << 1);
    }

    string to_string(int base = 10) const;
    string to_hex_string(bool prefix = false) const;

    static constexpr uint128_t min() noexcept { return uint128_t(0, 0); }
    static constexpr uint128_t max() noexcept { return uint128_t(~uint64_t(0), ~uint64_t(0)); }
};


class NEFORCE_API int128_t : icommon<int128_t> {
public:
    uint64_t lo;
    uint64_t hi;

    constexpr int128_t() noexcept :
    lo(0),
    hi(0) {}

    constexpr int128_t(int64_t value) noexcept;
    constexpr int128_t(uint64_t low, bool negative = false) noexcept;

    constexpr int128_t(uint64_t high, uint64_t low) noexcept :
    lo(low),
    hi(high) {}

    constexpr int128_t(const int128_t&) noexcept = default;
    constexpr int128_t& operator=(const int128_t&) noexcept = default;

    explicit int128_t(const string& str, int base = 10);
    explicit int128_t(const char* str, int base = 10);

    uint128_t to_uint128() const noexcept;

    explicit operator bool() const noexcept { return lo || hi; }
    explicit operator int64_t() const noexcept;
    explicit operator uint64_t() const noexcept { return lo; }

    bool is_negative() const noexcept { return static_cast<int64_t>(hi) < 0; }

    friend bool operator==(const int128_t& a, const int128_t& b) noexcept;
    friend bool operator!=(const int128_t& a, const int128_t& b) noexcept;
    friend bool operator<(const int128_t& a, const int128_t& b) noexcept;
    friend bool operator<=(const int128_t& a, const int128_t& b) noexcept;
    friend bool operator>(const int128_t& a, const int128_t& b) noexcept;
    friend bool operator>=(const int128_t& a, const int128_t& b) noexcept;

    int128_t operator+() const noexcept { return *this; }
    int128_t operator-() const noexcept;
    int128_t operator+(const int128_t& other) const;
    int128_t operator-(const int128_t& other) const;
    int128_t operator*(const int128_t& other) const;
    int128_t operator/(const int128_t& other) const;
    int128_t operator%(const int128_t& other) const;

    int128_t& operator+=(const int128_t& other);
    int128_t& operator-=(const int128_t& other);
    int128_t& operator*=(const int128_t& other);
    int128_t& operator/=(const int128_t& other);
    int128_t& operator%=(const int128_t& other);

    int128_t& operator++() noexcept;
    int128_t operator++(int) noexcept;
    int128_t& operator--() noexcept;
    int128_t operator--(int) noexcept;

    int128_t operator~() const noexcept;
    int128_t operator&(const int128_t& other) const noexcept;
    int128_t operator|(const int128_t& other) const noexcept;
    int128_t operator^(const int128_t& other) const noexcept;
    int128_t operator<<(unsigned int shift) const noexcept;
    int128_t operator>>(unsigned int shift) const noexcept;

    int128_t& operator&=(const int128_t& other) noexcept;
    int128_t& operator|=(const int128_t& other) noexcept;
    int128_t& operator^=(const int128_t& other) noexcept;
    int128_t& operator<<=(unsigned int shift) noexcept;
    int128_t& operator>>=(unsigned int shift) noexcept;

    size_t to_hash() const noexcept {
        return hash<uint64_t>()(lo) ^ (hash<uint64_t>()(hi) << 1);
    }

    string to_string(int base = 10) const;
    string to_hex_string(bool prefix = false) const;

    static constexpr int128_t min() noexcept { return int128_t(0x8000000000000000ULL, 0ULL); }
    static constexpr int128_t max() noexcept { return int128_t(0x7FFFFFFFFFFFFFFFULL, ~static_cast<uint64_t>(0)); }
};


NEFORCE_BEGIN_LITERALS__

constexpr uint128_t operator"" _u128(unsigned long long val) noexcept { return uint128_t(val); }
constexpr int128_t operator"" _i128(unsigned long long val) noexcept { return int128_t(static_cast<int64_t>(val)); }

NEFORCE_API uint128_t operator"" _u128(const char* str);
NEFORCE_API int128_t operator"" _i128(const char* str);

NEFORCE_END_LITERALS__


template <>
class numeric_traits<uint128_t> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr auto has_denorm = float_denorm_type::ABSENT;
    static constexpr bool has_denorm_loss = false;
    static constexpr auto round_style = float_round_type::TOWARD_ZERO;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = 128;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;
    static constexpr uint128_t min() noexcept { return uint128_t::min(); }
    static constexpr uint128_t lowest() noexcept { return uint128_t::min(); }
    static constexpr uint128_t max() noexcept { return uint128_t::max(); }
    static constexpr uint128_t epsilon() noexcept { return 0; }
    static constexpr uint128_t round_error() noexcept { return 0; }
    static constexpr uint128_t infinity() noexcept { return 0; }
    static constexpr uint128_t quiet_NaN() noexcept { return 0; }
    static constexpr uint128_t signaling_NaN() noexcept { return 0; }
    static constexpr uint128_t denorm_min() noexcept { return 0; }
};

template <>
class numeric_traits<int128_t> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr auto has_denorm = float_denorm_type::ABSENT;
    static constexpr bool has_denorm_loss = false;
    static constexpr auto round_style = float_round_type::TOWARD_ZERO;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = 127;
    static constexpr int digits10 = 38;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;
    static constexpr int128_t min() noexcept { return int128_t::min(); }
    static constexpr int128_t lowest() noexcept { return int128_t::min(); }
    static constexpr int128_t max() noexcept { return int128_t::max(); }
    static constexpr int128_t epsilon() noexcept { return 0LL; }
    static constexpr int128_t round_error() noexcept { return 0LL; }
    static constexpr int128_t infinity() noexcept { return 0LL; }
    static constexpr int128_t quiet_NaN() noexcept { return 0LL; }
    static constexpr int128_t signaling_NaN() noexcept { return 0LL; }
    static constexpr int128_t denorm_min() noexcept { return 0LL; }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_NUMERIC_INT128_HPP__
