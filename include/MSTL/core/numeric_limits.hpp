#ifndef MSTL_NUMERIC_LIMITS_HPP__
#define MSTL_NUMERIC_LIMITS_HPP__
#include "type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

enum class MSTL_API FLOAT_DENORM_TYPE {
    INDETERMINATE = -1, ABSENT, PRESENT
};

enum class MSTL_API FLOAT_ROUND_TYPE {
    INDETERMINATE = -1, TOWARD_ZERO, TO_NEAREST, TOWARD_INFINITY, TOWARD_NEG_INFINITY
};


MSTL_BEGIN_INNER__

struct __numeric_base {
    static constexpr auto has_denorm        = FLOAT_DENORM_TYPE::ABSENT;
    static constexpr bool has_denorm_loss   = false;
    static constexpr bool has_infinity      = false;
    static constexpr bool has_quiet_nan     = false;
    static constexpr bool has_signaling_nan = false;
    static constexpr bool is_bounded        = false;
    static constexpr bool is_exact          = false;
    static constexpr bool is_iec559         = false;
    static constexpr bool is_integer        = false;
    static constexpr bool is_modulo         = false;
    static constexpr bool is_signed         = false;
    static constexpr bool is_specialized    = false;
    static constexpr bool tinyness_before   = false;
    static constexpr bool traps             = false;
    static constexpr auto round_style       = FLOAT_ROUND_TYPE::TOWARD_ZERO;
    static constexpr int digits             = 0;
    static constexpr int digits10           = 0;
    static constexpr int max_digits10       = 0;
    static constexpr int max_exponent       = 0;
    static constexpr int max_exponent10     = 0;
    static constexpr int min_exponent       = 0;
    static constexpr int min_exponent10     = 0;
    static constexpr int radix              = 0;
};

struct __numeric_int_base : __numeric_base {
    static constexpr bool is_bounded     = true;
    static constexpr bool is_exact       = true;
    static constexpr bool is_integer     = true;
    static constexpr bool is_specialized = true;
    static constexpr int radix           = 2;
};

struct __numeric_float_base : __numeric_base {
    static constexpr auto has_denorm            = FLOAT_DENORM_TYPE::PRESENT;
    static constexpr bool has_infinity          = true;
    static constexpr bool has_quiet_nan         = true;
    static constexpr bool has_signaling_nan     = true;
    static constexpr bool is_bounded            = true;
    static constexpr bool is_iec559             = true;
    static constexpr bool is_signed             = true;
    static constexpr bool is_specialized        = true;
    static constexpr auto round_style           = FLOAT_ROUND_TYPE::TO_NEAREST;
    static constexpr int radix                  = 2;
};

MSTL_END_INNER__


template <typename T, typename = void>
class numeric_limits : public _INNER __numeric_base {
public:
    MSTL_NODISCARD static constexpr T min() noexcept { return T(); }
    MSTL_NODISCARD static constexpr T max() noexcept { return T(); }

    MSTL_NODISCARD static constexpr T lowest() noexcept { return T(); }
    MSTL_NODISCARD static constexpr T epsilon() noexcept { return T(); }
    MSTL_NODISCARD static constexpr T round_error() noexcept { return T(); }
    MSTL_NODISCARD static constexpr T denorm_min() noexcept { return T(); }

    MSTL_NODISCARD static constexpr T infinity() noexcept { return T(); }
    MSTL_NODISCARD static constexpr T quiet_nan() noexcept { return T(); }
    MSTL_NODISCARD static constexpr T signaling_nan() noexcept { return T(); }
};

template <typename T>
class numeric_limits<const T> : public numeric_limits<T> {};

template <typename T>
class numeric_limits<volatile T> : public numeric_limits<T> {};

template <typename T>
class numeric_limits<const volatile T> : public numeric_limits<T> {};


template <>
class numeric_limits<bool> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr bool min() noexcept { return false; }
    MSTL_NODISCARD static constexpr bool max() noexcept { return true; }
    
    MSTL_NODISCARD static constexpr bool lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr bool epsilon() noexcept { return false; }
    MSTL_NODISCARD static constexpr bool round_error() noexcept { return false; }
    MSTL_NODISCARD static constexpr bool denorm_min() noexcept { return false; }
    
    MSTL_NODISCARD static constexpr bool infinity() noexcept { return false; }
    MSTL_NODISCARD static constexpr bool quiet_nan() noexcept { return false; }
    MSTL_NODISCARD static constexpr bool signaling_nan() noexcept { return false; }

    static constexpr int digits = 1;
};

template <>
class numeric_limits<char> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr char min() noexcept { return CHAR_MIN_VALUE; }
    MSTL_NODISCARD static constexpr char max() noexcept { return CHAR_MAX_VALUE; }

    MSTL_NODISCARD static constexpr char lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr char epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr char infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = IS_CHAR_SIGNED;
    static constexpr bool is_modulo = !IS_CHAR_SIGNED;
    static constexpr int digits     = 8 - static_cast<int>(IS_CHAR_SIGNED);
    static constexpr int digits10   = 2;
};

template <>
class numeric_limits<int8_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int8_t min() noexcept { return INT8_MIN_VALUE; }
    MSTL_NODISCARD static constexpr int8_t max() noexcept { return INT8_MAX_VALUE; }

    MSTL_NODISCARD static constexpr int8_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr int8_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int8_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int8_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr int8_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int8_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int8_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits     = 7;
    static constexpr int digits10   = 2;
};

template <>
class numeric_limits<uint8_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint8_t min() noexcept { return UINT8_MIN_VALUE; }
    MSTL_NODISCARD static constexpr uint8_t max() noexcept { return UINT8_MAX_VALUE; }

    MSTL_NODISCARD static constexpr uint8_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr uint8_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint8_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint8_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr uint8_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint8_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint8_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 8;
    static constexpr int digits10   = 2;
};

#ifdef MSTL_VERSION_20__
template <>
class numeric_limits<char8_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr char8_t min() noexcept { return CHAR8_MIN_VALUE; }
    MSTL_NODISCARD static constexpr char8_t max() noexcept { return CHAR8_MAX_VALUE; }

    MSTL_NODISCARD static constexpr char8_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr char8_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char8_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char8_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr char8_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char8_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char8_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 8;
    static constexpr int digits10   = 2;
};
#endif

template <>
class numeric_limits<char16_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr char16_t min() noexcept { return CHAR16_MIN_VALUE; }
    MSTL_NODISCARD static constexpr char16_t max() noexcept { return CHAR16_MAX_VALUE; }

    MSTL_NODISCARD static constexpr char16_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr char16_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char16_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char16_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr char16_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char16_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char16_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 16;
    static constexpr int digits10   = 4;
};

template <>
class numeric_limits<char32_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr char32_t min() noexcept { return CHAR32_MIN_VALUE; }
    MSTL_NODISCARD static constexpr char32_t max() noexcept { return CHAR32_MAX_VALUE; }

    MSTL_NODISCARD static constexpr char32_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr char32_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char32_t round_error() noexcept { return 0; } 
    MSTL_NODISCARD static constexpr char32_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr char32_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char32_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr char32_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 32;
    static constexpr int digits10   = 9;
};

template <>
class numeric_limits<wchar_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr wchar_t min() noexcept { return WCHAR_MIN_VALUE; }
    MSTL_NODISCARD static constexpr wchar_t max() noexcept { return WCHAR_MAX_VALUE; }

    MSTL_NODISCARD static constexpr wchar_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr wchar_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr wchar_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr wchar_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr wchar_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr wchar_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr wchar_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 16;
    static constexpr int digits10   = 4;
};

template <>
class numeric_limits<int16_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int16_t min() noexcept { return INT16_MIN_VALUE; }
    MSTL_NODISCARD static constexpr int16_t max() noexcept { return INT16_MAX_VALUE; }

    MSTL_NODISCARD static constexpr int16_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr int16_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int16_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int16_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr int16_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int16_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int16_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits     = 15;
    static constexpr int digits10   = 4;
};

template <>
class numeric_limits<int32_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int32_t min() noexcept { return INT32_MIN_VALUE; }
    MSTL_NODISCARD static constexpr int32_t max() noexcept { return INT32_MAX_VALUE; }

    MSTL_NODISCARD static constexpr int32_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr int32_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int32_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int32_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr int32_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int32_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int32_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits     = 31;
    static constexpr int digits10   = 9;
};

template <>
class numeric_limits<long> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr long min() noexcept { return LONG_MIN_VALUE; }
    MSTL_NODISCARD static constexpr long max() noexcept { return LONG_MAX_VALUE; }

    MSTL_NODISCARD static constexpr long lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr long epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr long round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr long denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr long infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr long quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr long signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits     = 31;
    static constexpr int digits10   = 9;
};

template <>
class numeric_limits<int64_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int64_t min() noexcept { return INT64_MIN_VALUE; }
    MSTL_NODISCARD static constexpr int64_t max() noexcept { return INT64_MAX_VALUE; }

    MSTL_NODISCARD static constexpr int64_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr int64_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int64_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int64_t denorm_min() noexcept { return 0; }
    
    MSTL_NODISCARD static constexpr int64_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int64_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr int64_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_signed = true;
    static constexpr int digits     = 63;
    static constexpr int digits10   = 18;
};


template <>
class numeric_limits<uint16_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint16_t min() noexcept { return UINT16_MIN_VALUE; }
    MSTL_NODISCARD static constexpr uint16_t max() noexcept { return UINT16_MAX_VALUE; }

    MSTL_NODISCARD static constexpr uint16_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr uint16_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint16_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint16_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr uint16_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint16_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint16_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 16;
    static constexpr int digits10   = 4;
};

template <>
class numeric_limits<uint32_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint32_t min() noexcept { return UINT32_MAX_VALUE; }
    MSTL_NODISCARD static constexpr uint32_t max() noexcept { return UINT32_MAX_VALUE; }

    MSTL_NODISCARD static constexpr uint32_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr uint32_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint32_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint32_t denorm_min() noexcept { return 0; }
    
    MSTL_NODISCARD static constexpr uint32_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint32_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint32_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 32;
    static constexpr int digits10   = 9;
};

template <>
class numeric_limits<unsigned long> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr unsigned long min() noexcept { return ULONG_MIN_VALUE; }
    MSTL_NODISCARD static constexpr unsigned long max() noexcept { return ULONG_MAX_VALUE; }

    MSTL_NODISCARD static constexpr unsigned long lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr unsigned long epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr unsigned long round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr unsigned long denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr unsigned long infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr unsigned long quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr unsigned long signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 32;
    static constexpr int digits10   = 9;
};

template <>
class numeric_limits<uint64_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint64_t min() noexcept { return UINT64_MIN_VALUE; }
    MSTL_NODISCARD static constexpr uint64_t max() noexcept { return UINT64_MAX_VALUE; }

    MSTL_NODISCARD static constexpr uint64_t lowest() noexcept { return min(); }
    MSTL_NODISCARD static constexpr uint64_t epsilon() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint64_t round_error() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint64_t denorm_min() noexcept { return 0; }

    MSTL_NODISCARD static constexpr uint64_t infinity() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint64_t quiet_nan() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint64_t signaling_nan() noexcept { return 0; }

    static constexpr bool is_modulo = true;
    static constexpr int digits     = 64;
    static constexpr int digits10   = 19;
};

template <>
class numeric_limits<float32_t> : public _INNER __numeric_float_base {
public:
    MSTL_NODISCARD static constexpr float32_t min() noexcept { return FLOAT32_MIN_POSI_VALUE; }
    MSTL_NODISCARD static constexpr float32_t max() noexcept { return FLOAT32_MAX_POSI_VALUE; }

    MSTL_NODISCARD static constexpr float32_t lowest() noexcept { return FLOAT32_MIN_NEGA_VALUE; }
    MSTL_NODISCARD static constexpr float32_t epsilon() noexcept { return FLOAT32_EPSILON; }
    MSTL_NODISCARD static constexpr float32_t round_error() noexcept { return 0.5F; }
    MSTL_NODISCARD static constexpr float32_t denorm_min() noexcept { return FLOAT32_TRUE_MIN_POSITIVE_VALUE; }

    MSTL_NODISCARD static constexpr float32_t infinity() noexcept { return __builtin_huge_valf(); }
    MSTL_NODISCARD static constexpr float32_t quiet_nan() noexcept { return __builtin_nanf("0"); }
    MSTL_NODISCARD static constexpr float32_t signaling_nan() noexcept { return __builtin_nansf("1"); }

    static constexpr int digits         = 24;
    static constexpr int digits10       = 6;
    static constexpr int max_digits10   = FLOAT32_MAX_DIGITS;
    static constexpr int max_exponent   = 128;
    static constexpr int max_exponent10 = 38;
    static constexpr int min_exponent   = -125;
    static constexpr int min_exponent10 = -37;
};

template <>
class numeric_limits<float64_t> : public _INNER __numeric_float_base {
public:
    MSTL_NODISCARD static constexpr float64_t min() noexcept { return FLOAT64_MIN_POSI_VALUE; }
    MSTL_NODISCARD static constexpr float64_t max() noexcept { return FLOAT64_MAX_POSI_VALUE; }

    MSTL_NODISCARD static constexpr float64_t lowest() noexcept { return FLOAT64_MIN_NEGA_VALUE; }
    MSTL_NODISCARD static constexpr float64_t epsilon() noexcept { return FLOAT64_EPSILON; }
    MSTL_NODISCARD static constexpr float64_t round_error() noexcept { return 0.5; }
    MSTL_NODISCARD static constexpr float64_t denorm_min() noexcept { return FLOAT64_TRUE_MIN_POSITIVE_VALUE; }

    MSTL_NODISCARD static constexpr float64_t infinity() noexcept { return __builtin_huge_val(); }
    MSTL_NODISCARD static constexpr float64_t quiet_nan() noexcept { return __builtin_nan("0"); }
    MSTL_NODISCARD static constexpr float64_t signaling_nan() noexcept { return __builtin_nans("1"); }

    static constexpr int digits         = 53;
    static constexpr int digits10       = 15;
    static constexpr int max_digits10   = FLOAT64_MAX_DIGITS;
    static constexpr int max_exponent   = 1024;
    static constexpr int max_exponent10 = 308;
    static constexpr int min_exponent   = -1021;
    static constexpr int min_exponent10 = -307;
};

template <>
class numeric_limits<decimal_t> : public _INNER __numeric_float_base {
public:
    MSTL_NODISCARD static constexpr decimal_t min() noexcept { return DECIMAL_MIN_POSI_VALUE; }
    MSTL_NODISCARD static constexpr decimal_t max() noexcept { return DECIMAL_MAX_POSI_VALUE; }

    MSTL_NODISCARD static constexpr decimal_t lowest() noexcept { return DECIMAL_MIN_NEGA_VALUE; }
    MSTL_NODISCARD static constexpr decimal_t epsilon() noexcept { return DECIMAL_EPSILON; }
    MSTL_NODISCARD static constexpr decimal_t round_error() noexcept { return 0.5L; }
    MSTL_NODISCARD static constexpr decimal_t denorm_min() noexcept { return DECIMAL_TRUE_MIN_POSITIVE_VALUE; }

    MSTL_NODISCARD static constexpr decimal_t infinity() noexcept { return __builtin_huge_val(); }
    MSTL_NODISCARD static constexpr decimal_t quiet_nan() noexcept { return __builtin_nan("0"); }
    MSTL_NODISCARD static constexpr decimal_t signaling_nan() noexcept { return __builtin_nans("1"); }

#ifdef MSTL_COMPILER_MSVC__
    static constexpr int digits         = numeric_limits<float64_t>::digits;
    static constexpr int digits10       = numeric_limits<float64_t>::digits10;
    static constexpr int max_digits10   = numeric_limits<float64_t>::max_digits10;
    static constexpr int max_exponent   = numeric_limits<float64_t>::max_exponent;
    static constexpr int max_exponent10 = numeric_limits<float64_t>::max_exponent10;
    static constexpr int min_exponent   = numeric_limits<float64_t>::min_exponent;
    static constexpr int min_exponent10 = numeric_limits<float64_t>::min_exponent10;
#elif defined(MSTL_COMPILER_GNUC__)
    static constexpr int digits         = 113;
    static constexpr int digits10       = 33;
    static constexpr int max_digits10   = 35;
    static constexpr int max_exponent   = 16384;
    static constexpr int max_exponent10 = 4932;
    static constexpr int min_exponent   = -16381;
    static constexpr int min_exponent10 = -4931;
#endif
};


template <typename T>
class numeric_limits<T, enable_if_t<is_unpackaged_v<T>>> : public numeric_limits<unpackage_t<T>> {};

MSTL_END_NAMESPACE__
#endif // MSTL_NUMERIC_LIMITS_HPP__
