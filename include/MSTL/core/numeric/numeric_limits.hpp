#ifndef MSTL_CORE_NUMERIC_NUMERIC_LIMITS_HPP__
#define MSTL_CORE_NUMERIC_NUMERIC_LIMITS_HPP__
#include "../typeinfo/types.hpp"
#include "../typeinfo/type_traits.hpp"
#include "../config/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

enum class FLOAT_DENORM_TYPE {
    INDETERMINATE = -1, ABSENT, PRESENT
};

enum class FLOAT_ROUND_TYPE {
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
    // traps
};

struct __numeric_int_base : __numeric_base {
    static constexpr bool is_bounded     = true;
    static constexpr bool is_exact       = true;
    static constexpr bool is_integer     = true;
    static constexpr bool is_specialized = true;
    static constexpr int radix           = 2;
#ifdef MSTL_COMPILER_GNUC__
    static constexpr bool traps          = true;
#endif
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
class numeric_limits<int8_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int8_t min() noexcept { return -128; }
    MSTL_NODISCARD static constexpr int8_t max() noexcept { return 127; }

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
class numeric_limits<int16_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int16_t min() noexcept { return -32768; }
    MSTL_NODISCARD static constexpr int16_t max() noexcept { return 32767; }

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
    MSTL_NODISCARD static constexpr int32_t min() noexcept { return -2147483647 - 1; }
    MSTL_NODISCARD static constexpr int32_t max() noexcept { return 2147483647; }

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
class numeric_limits<int64_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr int64_t min() noexcept { return -9223372036854775807LL - 1; }
    MSTL_NODISCARD static constexpr int64_t max() noexcept { return 9223372036854775807LL; }

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

#ifdef MSTL_PLATFORM_LINUX64__
template <>
class numeric_limits<long long> : public numeric_limits<int64_t> {};
#else
template <>
class numeric_limits<long> : public numeric_limits<int32_t> {};
#endif

template <>
class numeric_limits<uint8_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint8_t min() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint8_t max() noexcept { return 0xffU; }

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

template <>
class numeric_limits<uint16_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint16_t min() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint16_t max() noexcept { return 0xffffU; }

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
    MSTL_NODISCARD static constexpr uint32_t min() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint32_t max() noexcept { return 0xffffffffU; }

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
class numeric_limits<uint64_t> : public _INNER __numeric_int_base {
public:
    MSTL_NODISCARD static constexpr uint64_t min() noexcept { return 0; }
    MSTL_NODISCARD static constexpr uint64_t max() noexcept { return 0xffffffffffffffffULL; }

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

#ifdef MSTL_PLATFORM_LINUX64__
template <>
class numeric_limits<unsigned long long> : public numeric_limits<uint64_t> {};
#else
template <>
class numeric_limits<unsigned long> : public numeric_limits<uint32_t> {};
#endif


template <>
class numeric_limits<char> : public numeric_limits<
    conditional_t<static_cast<char>(128) < 0, int8_t, uint8_t>
> {};

#ifdef MSTL_STANDARD_20__
template <>
class numeric_limits<char8_t> : public numeric_limits<uint8_t> {};
#endif

template <>
class numeric_limits<char16_t> : public numeric_limits<uint16_t> {};
template <>
class numeric_limits<char32_t> : public numeric_limits<uint32_t> {};

#ifdef MSTL_PLATFORM_WINDOWS__
template <>
class numeric_limits<wchar_t> : public numeric_limits<uint16_t> {};
#elif defined(MSTL_PLATFORM_LINUX__)
template <>
class numeric_limits<wchar_t> : public numeric_limits<int32_t> {};
#endif


template <>
class numeric_limits<float32_t> : public _INNER __numeric_float_base {
public:
    MSTL_NODISCARD static constexpr float32_t min_posi() noexcept { return 1.175494351e-38f; }
    MSTL_NODISCARD static constexpr float32_t max_posi() noexcept { return 3.402823466e+38f; }
    MSTL_NODISCARD static constexpr float32_t min_nega() noexcept { return -3.402823466e+38f; }
    MSTL_NODISCARD static constexpr float32_t max_nega() noexcept { return -1.175494351e-38f; }

    MSTL_NODISCARD static constexpr float32_t min() noexcept { return min_posi(); }
    MSTL_NODISCARD static constexpr float32_t max() noexcept { return max_posi(); }

    MSTL_NODISCARD static constexpr float32_t lowest() noexcept { return min_nega(); }
    MSTL_NODISCARD static constexpr float32_t epsilon() noexcept { return 1.192092896e-07f; }
    MSTL_NODISCARD static constexpr float32_t round_error() noexcept { return 0.5F; }
    MSTL_NODISCARD static constexpr float32_t denorm_min() noexcept { return 1.401298464e-45f; }

    MSTL_NODISCARD static constexpr float32_t infinity() noexcept { return __builtin_huge_valf(); }
    MSTL_NODISCARD static constexpr float32_t quiet_nan() noexcept {
#ifdef MSTL_COMPILER_GCC__
        return __builtin_nanf("");
#else
        return __builtin_nan("0");
#endif
    }
    MSTL_NODISCARD static constexpr float32_t signaling_nan() noexcept {
#ifdef MSTL_COMPILER_GCC__
        return __builtin_nansf("");
#else
        return __builtin_nans("1");
#endif
    }

    static constexpr int digits         = 24;
    static constexpr int digits10       = 6;
    static constexpr int max_digits10   = 9;
    static constexpr int max_exponent   = 128;
    static constexpr int max_exponent10 = 38;
    static constexpr int min_exponent   = -125;
    static constexpr int min_exponent10 = -37;
};

template <>
class numeric_limits<float64_t> : public _INNER __numeric_float_base {
public:
    MSTL_NODISCARD static constexpr float64_t min_posi() noexcept { return 2.2250738585072014e-308; }
    MSTL_NODISCARD static constexpr float64_t max_posi() noexcept { return 1.7976931348623157e+308; }
    MSTL_NODISCARD static constexpr float64_t min_nega() noexcept { return -1.7976931348623157e+308; }
    MSTL_NODISCARD static constexpr float64_t max_nega() noexcept { return -2.2250738585072014e-308; }

    MSTL_NODISCARD static constexpr float64_t min() noexcept { return min_posi(); }
    MSTL_NODISCARD static constexpr float64_t max() noexcept { return max_posi(); }

    MSTL_NODISCARD static constexpr float64_t lowest() noexcept { return min_nega(); }
    MSTL_NODISCARD static constexpr float64_t epsilon() noexcept { return 2.2204460492503131e-16; }
    MSTL_NODISCARD static constexpr float64_t round_error() noexcept { return 0.5; }
    MSTL_NODISCARD static constexpr float64_t denorm_min() noexcept { return 4.9406564584124654e-324; }

    MSTL_NODISCARD static constexpr float64_t infinity() noexcept { return __builtin_huge_val(); }
    MSTL_NODISCARD static constexpr float64_t quiet_nan() noexcept {
#ifdef MSTL_COMPILER_GCC__
        return __builtin_nan("");
#else
        return __builtin_nan("0");
#endif
    }
    MSTL_NODISCARD static constexpr float64_t signaling_nan() noexcept {
#ifdef MSTL_COMPILER_GCC__
        return __builtin_nans("");
#else
        return __builtin_nans("1");
#endif
    }

    static constexpr int digits         = 53;
    static constexpr int digits10       = 15;
    static constexpr int max_digits10   = 17;
    static constexpr int max_exponent   = 1024;
    static constexpr int max_exponent10 = 308;
    static constexpr int min_exponent   = -1021;
    static constexpr int min_exponent10 = -307;
};

#ifdef MSTL_COMPILER_MSVC__
template <>
class numeric_limits<decimal_t> : public numeric_limits<float64_t> {};
#else
template <>
class numeric_limits<decimal_t> : public _INNER __numeric_float_base {
public:
    MSTL_NODISCARD static constexpr decimal_t min_posi() noexcept { return 3.36210314311209350626267781732175260e-4932L; }
    MSTL_NODISCARD static constexpr decimal_t max_posi() noexcept { return 1.18973149535723176502126385303097021e+4932L; }
    MSTL_NODISCARD static constexpr decimal_t min_nega() noexcept { return -1.18973149535723176502126385303097021e+4932L; }
    MSTL_NODISCARD static constexpr decimal_t max_nega() noexcept { return -3.36210314311209350626267781732175260e-4932L; }

    MSTL_NODISCARD static constexpr decimal_t min() noexcept { return min_posi(); }
    MSTL_NODISCARD static constexpr decimal_t max() noexcept { return max_posi(); }

    MSTL_NODISCARD static constexpr decimal_t lowest() noexcept { return min_nega(); }
    MSTL_NODISCARD static constexpr decimal_t epsilon() noexcept { return 1.08420217248550443401e-19L; }
    MSTL_NODISCARD static constexpr decimal_t round_error() noexcept { return 0.5L; }
    MSTL_NODISCARD static constexpr decimal_t denorm_min() noexcept { return 3.64519953188247460253e-4951L; }

    MSTL_NODISCARD static constexpr decimal_t infinity() noexcept { return __builtin_huge_val(); }
    MSTL_NODISCARD static constexpr decimal_t quiet_nan() noexcept {
#ifdef MSTL_COMPILER_GCC__
        return __builtin_nanl("");
#else
        return __builtin_nan("0");
#endif
    }
    MSTL_NODISCARD static constexpr decimal_t signaling_nan() noexcept {
#ifdef MSTL_COMPILER_GCC__
        return __builtin_nansl("");
#else
        return __builtin_nans("1");
#endif
    }

    static constexpr int digits         = 64;
    static constexpr int digits10       = 18;
    static constexpr int max_digits10   = 21;
    static constexpr int max_exponent   = 16384;
    static constexpr int max_exponent10 = 4932;
    static constexpr int min_exponent   = -16381;
    static constexpr int min_exponent10 = -4931;
};
#endif


template <typename T>
class numeric_limits<T, enable_if_t<is_unpackaged_v<T>>> : public numeric_limits<unpackage_t<T>> {};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_NUMERIC_NUMERIC_LIMITS_HPP__
