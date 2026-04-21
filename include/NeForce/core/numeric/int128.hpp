#ifndef NEFORCE_CORE_NUMERIC_INT128_HPP__
#define NEFORCE_CORE_NUMERIC_INT128_HPP__
#include "NeForce/core/string/to_numerics.hpp"
#include "NeForce/core/string/to_string.hpp"
#include "NeForce/core/interface/iobject.hpp"
#include "NeForce/core/interface/inumeric.hpp"
#ifdef NEFORCE_SUPPORT_INT128
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API int128_t;


struct NEFORCE_API uint128_t : icommon<uint128_t>, iarithmetic<uint128_t>, ibinary<uint128_t>, iobject<uint128_t> {
public:
    uint64_t lo{0};
    uint64_t hi{0};

    constexpr uint128_t() noexcept = default;

    constexpr uint128_t(const int32_t low) noexcept :
    lo(static_cast<uint64_t>(static_cast<int64_t>(low))),
    hi(low < 0 ? ~static_cast<uint64_t>(0) : 0) {}

    constexpr uint128_t(const uint32_t low) noexcept :
    lo(low) {}

    constexpr uint128_t(const uint64_t low) noexcept :
    lo(low) {}

    constexpr uint128_t(const uint64_t high, const uint64_t low) noexcept :
    lo(low),
    hi(high) {}

    constexpr uint128_t(const uint128_t&) noexcept = default;
    constexpr uint128_t& operator=(const uint128_t&) noexcept = default;

    explicit NEFORCE_CONSTEXPR20 uint128_t(const string& str, int base = 10) :
    uint128_t(str.view(), base) {}

    explicit constexpr uint128_t(string_view str, int base = 10);

    explicit constexpr uint128_t(const char* str, int base = 10) :
    uint128_t(string_view{str}, base) {}

    constexpr int128_t to_int128() const noexcept;

    explicit constexpr operator bool() const noexcept { return lo || hi; }
    explicit constexpr operator char() const noexcept { return static_cast<char>(lo); }
    explicit constexpr operator int8_t() const noexcept { return static_cast<int8_t>(lo); }
    explicit constexpr operator uint8_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint16_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint64_t() const noexcept { return lo; }
    explicit constexpr operator int128_t() const noexcept;

    constexpr bool operator==(const uint128_t& rhs) const noexcept { return hi == rhs.hi && lo == rhs.lo; }
    constexpr bool operator<(const uint128_t& rhs) const noexcept {
        return hi < rhs.hi || (hi == rhs.hi && lo < rhs.lo);
    }

    constexpr uint128_t operator-() const noexcept {
        const uint64_t new_lo = ~lo + 1ULL;
        const uint64_t new_hi = ~hi + (lo == 0ULL ? 1ULL : 0ULL);
        return uint128_t(new_hi, new_lo);
    }

    uint128_t& operator+=(const uint128_t& other);
    uint128_t& operator-=(const uint128_t& other);
    uint128_t& operator*=(const uint128_t& other);
    uint128_t& operator/=(const uint128_t& other);
    uint128_t& operator%=(const uint128_t& other);

    uint128_t& operator++() noexcept {
        *this += 1;
        return *this;
    }
    uint128_t& operator--() noexcept {
        *this -= 1;
        return *this;
    }

    constexpr uint128_t operator~() const noexcept { return uint128_t(~hi, ~lo); }

    constexpr uint128_t& operator&=(const uint128_t& other) noexcept {
        hi &= other.hi;
        lo &= other.lo;
        return *this;
    }
    constexpr uint128_t& operator|=(const uint128_t& other) noexcept {
        hi |= other.hi;
        lo |= other.lo;
        return *this;
    }
    constexpr uint128_t& operator^=(const uint128_t& other) noexcept {
        hi ^= other.hi;
        lo ^= other.lo;
        return *this;
    }
    constexpr uint128_t& operator<<=(const uint32_t shift) noexcept {
        if (shift == 0) {
            return *this;
        }
        if (shift >= 128) {
            hi = 0;
            lo = 0;
            return *this;
        }
        if (shift >= 64) {
            hi = lo << (shift - 64);
            lo = 0;
            return *this;
        }
        hi = (hi << shift) | (lo >> (64 - shift));
        lo = lo << shift;
        return *this;
    }
    constexpr uint128_t& operator>>=(const uint32_t shift) noexcept {
        if (shift == 0) {
            return *this;
        }
        if (shift >= 128) {
            hi = 0;
            lo = 0;
            return *this;
        }
        if (shift >= 64) {
            lo = hi >> (shift - 64);
            hi = 0;
            return *this;
        }
        lo = (lo >> shift) | (hi << (64 - shift));
        hi = hi >> shift;
        return *this;
    }

    static uint128_t mul64(uint64_t a, uint64_t b) noexcept;
    uint64_t div64(uint64_t divisor, uint64_t* remainder = nullptr) const noexcept;

    constexpr size_t to_hash() const noexcept {
        constexpr uint64_t GOLDEN = 0x9E3779B97F4A7C15ULL;
        size_t seed = hash<uint64_t>()(lo);
        seed ^= hash<uint64_t>()(hi) + GOLDEN + (seed << 6) + (seed >> 2);
        return seed;
    }

    static constexpr uint128_t parse(const string_view view) { return uint128_t{view}; }
    NEFORCE_CONSTEXPR20 string to_string() const;

    static constexpr uint128_t min() noexcept { return uint128_t(0ULL, 0ULL); }
    static constexpr uint128_t max() noexcept {
        return uint128_t(~static_cast<uint64_t>(0), ~static_cast<uint64_t>(0));
    }
};


struct NEFORCE_API int128_t : icommon<int128_t>, iarithmetic<int128_t>, ibinary<int128_t>, iobject<int128_t> {
public:
    uint64_t lo{0};
    uint64_t hi{0};

    constexpr int128_t() noexcept = default;

    constexpr int128_t(const int32_t value) noexcept :
    int128_t(static_cast<int64_t>(value)) {}

    constexpr int128_t(const int64_t value) noexcept :
    lo(static_cast<uint64_t>(value)),
    hi(value < 0 ? ~static_cast<uint64_t>(0) : 0) {}

    constexpr int128_t(const uint64_t low, const bool negative = false) noexcept :
    lo(low),
    hi(negative ? ~static_cast<uint64_t>(0) : 0) {}

    constexpr int128_t(const uint64_t high, const uint64_t low) noexcept :
    lo(low),
    hi(high) {}

    constexpr int128_t(const int128_t&) noexcept = default;
    constexpr int128_t& operator=(const int128_t&) noexcept = default;

    constexpr int128_t& operator=(const uint128_t& other) noexcept {
        lo = other.lo;
        hi = other.hi;
        return *this;
    }

    explicit NEFORCE_CONSTEXPR20 int128_t(const string& str, int base = 10) :
    int128_t(str.view(), base) {}

    explicit constexpr int128_t(string_view str, int base = 10);

    explicit constexpr int128_t(const char* str, int base = 10) :
    int128_t(string_view{str}, base) {}

    constexpr uint128_t to_uint128() const noexcept { return uint128_t(hi, lo); }

    explicit constexpr operator bool() const noexcept { return lo || hi; }
    explicit constexpr operator char() const noexcept { return static_cast<char>(lo); }
    explicit constexpr operator int8_t() const noexcept { return static_cast<int8_t>(lo); }
    explicit constexpr operator int16_t() const noexcept { return static_cast<int16_t>(lo); }
    explicit constexpr operator int32_t() const noexcept { return static_cast<int32_t>(lo); }
    explicit constexpr operator int64_t() const noexcept { return static_cast<int64_t>(lo); }
    explicit constexpr operator uint8_t() const noexcept { return static_cast<uint8_t>(lo); }
    explicit constexpr operator uint16_t() const noexcept { return static_cast<uint16_t>(lo); }
    explicit constexpr operator uint32_t() const noexcept { return static_cast<uint32_t>(lo); }
    explicit constexpr operator uint64_t() const noexcept { return lo; }
    explicit constexpr operator uint128_t() const noexcept { return to_uint128(); }

    constexpr bool is_negative() const noexcept { return static_cast<int64_t>(hi) < 0; }

    constexpr bool operator==(const int128_t& rhs) const noexcept { return hi == rhs.hi && lo == rhs.lo; }
    constexpr bool operator<(const int128_t& rhs) const noexcept {
        const bool a_neg = is_negative();
        const bool b_neg = rhs.is_negative();
        if (a_neg != b_neg) {
            return a_neg;
        }
        return a_neg ? (hi > rhs.hi || (hi == rhs.hi && lo > rhs.lo)) : (hi < rhs.hi || (hi == rhs.hi && lo < rhs.lo));
    }

    constexpr int128_t operator+() const noexcept { return *this; }
    constexpr int128_t operator-() const noexcept {
        const uint64_t new_lo = ~lo + 1ULL;
        const uint64_t new_hi = ~hi + (lo == 0ULL ? 1ULL : 0ULL);
        return int128_t(new_hi, new_lo);
    }

    int128_t& operator+=(const int128_t& other);
    int128_t& operator-=(const int128_t& other);
    int128_t& operator*=(const int128_t& other);
    int128_t& operator/=(const int128_t& other);
    int128_t& operator%=(const int128_t& other);

    int128_t& operator++() noexcept {
        *this += 1;
        return *this;
    }
    int128_t& operator--() noexcept {
        *this -= 1;
        return *this;
    }

    constexpr int128_t operator~() const noexcept { return int128_t(~hi, ~lo); }

    constexpr int128_t& operator&=(const int128_t& other) noexcept {
        hi &= other.hi;
        lo &= other.lo;
        return *this;
    }
    constexpr int128_t& operator|=(const int128_t& other) noexcept {
        hi |= other.hi;
        lo |= other.lo;
        return *this;
    }
    constexpr int128_t& operator^=(const int128_t& other) noexcept {
        hi ^= other.hi;
        lo ^= other.lo;
        return *this;
    }
    constexpr int128_t& operator<<=(const uint32_t shift) noexcept {
        *this = to_uint128() << shift;
        return *this;
    }
    constexpr int128_t& operator>>=(const uint32_t shift) noexcept {
        if (shift == 0) {
            return *this;
        }
        const bool neg = is_negative();
        if (shift >= 128) {
            *this = neg ? int128_t(~0ULL, ~0ULL) : int128_t(0);
            return *this;
        }
        if (shift >= 64) {
            lo = static_cast<uint64_t>(static_cast<int64_t>(hi) >> (shift - 64));
            hi = neg ? ~0ULL : 0ULL;
        } else {
            lo = (lo >> shift) | (hi << (64 - shift));
            hi = static_cast<uint64_t>(static_cast<int64_t>(hi) >> shift);
        }
        return *this;
    }

    constexpr size_t to_hash() const noexcept {
        constexpr uint64_t GOLDEN = 0x9E3779B97F4A7C15ULL;
        size_t seed = hash<uint64_t>()(lo);
        seed ^= hash<uint64_t>()(hi) + GOLDEN + (seed << 6) + (seed >> 2);
        return seed;
    }

    static constexpr int128_t parse(const string_view view) { return int128_t{view}; }
    NEFORCE_CONSTEXPR20 string to_string() const;

    static constexpr int128_t min() noexcept { return int128_t(0x8000000000000000ULL, 0ULL); }
    static constexpr int128_t max() noexcept { return int128_t(0x7FFFFFFFFFFFFFFFULL, ~static_cast<uint64_t>(0)); }
};

template <>
struct make_signed<uint128_t> {
    using type = int128_t;
};
template <>
struct make_unsigned<int128_t> {
    using type = uint128_t;
};

#    define __NEFORCE_DEFINE_MAKE_SIGN(CV)  \
        template <>                         \
        struct make_signed<uint128_t CV> {  \
            using type = int128_t;          \
        };                                  \
        template <>                         \
        struct make_unsigned<int128_t CV> { \
            using type = uint128_t;         \
        };
NEFORCE_MACRO_RANGES_CV_REF(__NEFORCE_DEFINE_MAKE_SIGN)
#    undef __NEFORCE_DEFINE_MAKE_SIGN


template <>
struct is_integral<uint128_t> : true_type {};
template <>
struct is_unsigned<uint128_t> : true_type {};

NEFORCE_CONSTEXPR20 string uint128_t::to_string() const { return inner::__int_to_string_dispatch<uint128_t>(*this); }

constexpr uint128_t operator-(const uint128_t& lhs, const uint128_t& rhs) noexcept {
    const uint64_t new_lo = lhs.lo - rhs.lo;
    const uint64_t new_hi = lhs.hi - rhs.hi - (lhs.lo < rhs.lo);
    return uint128_t(new_hi, new_lo);
}


template <>
struct is_integral<int128_t> : true_type {};
template <>
struct is_signed<int128_t> : true_type {};

NEFORCE_CONSTEXPR20 string int128_t::to_string() const { return inner::__int_to_string_dispatch<int128_t>(*this); }

constexpr int128_t operator-(const int128_t& lhs, const int128_t& rhs) noexcept {
    const uint128_t a = lhs.to_uint128();
    const uint128_t b = rhs.to_uint128();
    const uint128_t c = a - b;
    return int128_t(c.hi, c.lo);
}

constexpr int128_t uint128_t::to_int128() const noexcept { return int128_t(hi, lo); }
constexpr uint128_t::operator int128_t() const noexcept { return int128_t(hi, lo); }


NEFORCE_BEGIN_LITERALS__

constexpr uint128_t operator""_u128(const uint64_t val) noexcept { return uint128_t(val); }
constexpr int128_t operator""_i128(const uint64_t val) noexcept { return int128_t(val); }

constexpr uint128_t operator""_u128(const char* str, const size_t len) { return uint128_t(string_view{str, len}); }
constexpr int128_t operator""_i128(const char* str, const size_t len) { return int128_t(string_view{str, len}); }

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
    static constexpr int128_t epsilon() noexcept { return 0; }
    static constexpr int128_t round_error() noexcept { return 0; }
    static constexpr int128_t infinity() noexcept { return 0; }
    static constexpr int128_t quiet_NaN() noexcept { return 0; }
    static constexpr int128_t signaling_NaN() noexcept { return 0; }
    static constexpr int128_t denorm_min() noexcept { return 0; }
};


/**
 * @brief 将字符串转换为128位无符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的128位无符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr uint128_t to_uint128(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const uint128_t num = inner::str_to_uints<uint128_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) {
        *idx = static_cast<size_t>(endptr - sv.data());
    }
    return num;
}

/**
 * @brief 将字符串转换为128位有符号整数
 * @param sv 要转换的字符串视图
 * @param idx 可选参数，存储转换结束位置索引
 * @param base 进制基数（0表示自动检测）
 * @return 转换后的128位有符号整数
 * @throws typecast_exception 转换失败时
 */
NEFORCE_NODISCARD constexpr int128_t to_int128(const string_view sv, size_t* idx = nullptr, const int base = 10) {
    char* endptr = nullptr;
    const int128_t num = inner::str_to_ints<int128_t>(sv, &endptr, base);
    if (sv.data() == endptr) {
        NEFORCE_THROW_EXCEPTION(typecast_exception("Convert from string failed."));
    }
    if (idx) {
        *idx = static_cast<size_t>(endptr - sv.data());
    }
    return num;
}


constexpr uint128_t::uint128_t(const string_view str, const int base) { *this = to_uint128(str, nullptr, base); }

constexpr int128_t::int128_t(const string_view str, const int base) { *this = to_int128(str, nullptr, base); }

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_CORE_NUMERIC_INT128_HPP__
