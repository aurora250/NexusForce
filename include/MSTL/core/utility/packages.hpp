#ifndef MSTL_CORE_UTILITY_PACKAGES_HPP__
#define MSTL_CORE_UTILITY_PACKAGES_HPP__
#include "../interface/iobject.hpp"
#include "../interface/ipackage.hpp"
#include "../string/to_numerics.hpp"
#include "../string/to_string.hpp"
MSTL_BEGIN_NAMESPACE__

struct boolean : ipackage<boolean, bool>, iobject<boolean> {
    using value_type = bool;
    using base = ipackage<boolean, bool>;

    MSTL_BUILD_PACKAGE_CONSTRUCTOR(boolean)

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return boolean(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return value_ ? "true" : "false";
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 boolean parse(const string_view lower) {
        boolean obj;
        string str(lower.trim());
        try {
            str.lowercase();
            if (str == "true" || str == "yes" || str == "y") {
                obj = true;
            } else if (str == "false" || str == "no" || str == "n") {
                obj = false;
            } else {
                obj = static_cast<bool>(to_int32(str.view(), nullptr, 10));
            }
        } catch (...) {
            throw_exception(typecast_exception("Convert from string to boolean failed."));
        }
        return obj;
    }

    constexpr boolean operator !() const noexcept {
        return boolean(!value_);
    }
};

template <>
struct package<bool> {
    using type = boolean;
};
template <>
struct unpackage<boolean> {
    using type = bool;
};


#define __MSTL_BUILD_INTEGER_STRUCT(SIGN, UPPER, BYTE) \
struct SIGN## integer## BYTE : iobject<SIGN## integer## BYTE>, ipackage<SIGN## integer## BYTE, SIGN## int## BYTE## _t> { \
    using value_type = SIGN## int## BYTE## _t; \
    using base = ipackage<SIGN## integer## BYTE, SIGN## int## BYTE## _t>; \
    \
    MSTL_BUILD_PACKAGE_CONSTRUCTOR(SIGN## integer## BYTE) \
    \
    MSTL_NODISCARD constexpr explicit operator bool() const noexcept { \
        return value_ != _MSTL initialize<value_type>(); \
    } \
    \
    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) { \
        return _INNER __int_to_string_dispatch(value); \
    } \
    \
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const { \
        return _INNER __int_to_string_dispatch(value_); \
    } \
    \
    MSTL_NODISCARD static constexpr SIGN## integer## BYTE parse(const string_view str) { \
        return SIGN## integer## BYTE{_MSTL to_## SIGN## int## BYTE (str)}; \
    } \
}; \
template <> \
struct package<SIGN## int## BYTE## _t> { \
    using type = SIGN## integer## BYTE; \
}; \
template <> \
struct unpackage<SIGN## integer## BYTE> { \
    using type = SIGN## int## BYTE## _t; \
};

__MSTL_BUILD_INTEGER_STRUCT(,,16)
__MSTL_BUILD_INTEGER_STRUCT(,,32)
__MSTL_BUILD_INTEGER_STRUCT(,,64)
__MSTL_BUILD_INTEGER_STRUCT(u,U,16)
__MSTL_BUILD_INTEGER_STRUCT(u,U,32)
__MSTL_BUILD_INTEGER_STRUCT(u,U,64)
#undef __MSTL_BUILD_INTEGER_STRUCT


#ifdef MSTL_PLATFORM_LINUX64__
template <>
struct package<long long> {
    using type = integer64;
};
template <>
struct package<unsigned long long> {
    using type = uinteger64;
};
#else
template <>
struct package<long> {
    using type = integer32;
};
template <>
struct package<unsigned long> {
    using type = uinteger32;
};
#endif


struct float32 : iobject<float32>, ipackage<float32, float32_t> {
    using value_type = float32_t;
    using base = ipackage<float32, float32_t>;

    MSTL_BUILD_PACKAGE_CONSTRUCTOR(float32)

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return float32(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static constexpr float32 parse(const string_view str) {
        return float32{_MSTL to_float32(str)};
    }
};

template <>
struct package<float32_t> {
    using type = float32;
};
template <>
struct unpackage<float32> {
    using type = float32_t;
};


struct float64 : iobject<float64>, ipackage<float64, float64_t> {
    using value_type = float64_t;
    using base = ipackage<float64, float64_t>;

    MSTL_BUILD_PACKAGE_CONSTRUCTOR(float64)

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return float64(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static constexpr float64 parse(const string_view str) {
        return float64{_MSTL to_float64(str)};
    }
};

template <>
struct package<float64_t> {
    using type = float64;
};
template <>
struct unpackage<float64> {
    using type = float64_t;
};


struct decimal : iobject<decimal>, ipackage<decimal, decimal_t> {
    using value_type = decimal_t;
    using base = ipackage<decimal, decimal_t>;

    MSTL_BUILD_PACKAGE_CONSTRUCTOR(decimal)

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return decimal(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static constexpr decimal parse(const string_view str) {
        return decimal{_MSTL to_decimal(str)};
    }
};

template <>
struct package<decimal_t> {
    using type = decimal;
};
template <>
struct unpackage<decimal> {
    using type = decimal_t;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_PACKAGES_HPP__
