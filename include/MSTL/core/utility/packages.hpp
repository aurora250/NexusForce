#ifndef MSTL_CORE_UTILITY_PACKAGES_HPP__
#define MSTL_CORE_UTILITY_PACKAGES_HPP__
#include "../string/serialize.hpp"
#include "../string/to_numerics.hpp"
MSTL_BEGIN_NAMESPACE__

struct boolean : iserialize<boolean>, ibinary<boolean> {
    using value_type = bool;
    using self = boolean;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr boolean() = default;
    constexpr boolean(const self&) noexcept = default;
    constexpr boolean(const value_type& val) noexcept : value_(val) {}
    constexpr boolean& operator=(const self&) noexcept = default;
    constexpr boolean& operator=(const value_type& other) noexcept {
        value_ = other;
        return *this;
    }

    constexpr boolean(self&& other) noexcept {
        this->swap(other);
    }
    constexpr boolean(value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        this->swap(other);
        other.value_ = _MSTL initialize<value_type>();
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other;
        return *this;
    }

    MSTL_CONSTEXPR20 ~boolean() = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return value_ ? "true" : "false";
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view lower) {
        self obj;
        string str(lower.trim());
        try {
            obj = static_cast<bool>(to_int32(str.view(), nullptr, 10));
        } catch (...) {
            _MSTL transform(str.begin(), str.end(), str.begin(), [](byte_t c) {
                return _MSTL to_lowercase(c);
            });
            if (str == "true" || str == "yes" || str == "y") {
                obj = true;
            } else if (str == "false" || str == "no" || str == "n") {
                obj = false;
            } else {
                throw_exception(typecast_exception("Convert from string to boolean failed."));
            }
        }
        return obj;
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        try {
            *this = self::parse(str);
            return true;
        }
        catch (...) {
            value_ = false;
            return false;
        }
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    constexpr bool operator ==(const self& other) const noexcept { return value_ == other.value_; }
    constexpr bool operator !=(const self& other) const noexcept { return value_ != other.value_; }
    constexpr bool operator <(const self& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator <=(const self& other) const noexcept { return value_ <= other.value_; }
    constexpr bool operator >(const self& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator >=(const self& other) const noexcept { return value_ >= other.value_; }

    constexpr self operator !() const noexcept { return self(!value_); }
    constexpr self operator &(const self& other) const noexcept { return self{static_cast<value_type>(value_ & other.value_)}; }
    constexpr self operator |(const self& other) const noexcept { return self{static_cast<value_type>(value_ | other.value_)}; }
    constexpr self operator ^(const self& other) const noexcept { return self{static_cast<value_type>(value_ ^ other.value_)}; }

    constexpr self& operator &=(const self& other) noexcept { value_ &= other.value_; return *this; }
    constexpr self& operator |=(const self& other) noexcept { value_ |= other.value_; return *this; }
    constexpr self& operator ^=(const self& other) noexcept { value_ ^= other.value_; return *this; }
};

template <>
struct package_base<bool> {
    using type = boolean;
};
template <>
struct unpackage_base<boolean> {
    using type = bool;
};


#define __MSTL_BUILD_INTEGER_STRUCT(SIGN, UPPER, BYTE) \
struct SIGN## integer## BYTE : iserialize<SIGN## integer## BYTE>, iarithmetic<SIGN## integer## BYTE>, ibinary<SIGN## integer## BYTE> { \
    using value_type = SIGN## int## BYTE## _t; \
    using self = SIGN## integer## BYTE; \
    \
private: \
    value_type value_ = _MSTL initialize<value_type>(); \
    \
public: \
    constexpr SIGN## integer## BYTE () = default; \
    constexpr SIGN## integer## BYTE (const self&) noexcept = default; \
    constexpr SIGN## integer## BYTE (const value_type& val) noexcept : value_(val) {} \
    constexpr SIGN## integer## BYTE & operator=(const self&) noexcept = default; \
    constexpr SIGN## integer## BYTE & operator=(const value_type& other) noexcept { value_ = other; return *this; } \
    \
    constexpr SIGN## integer## BYTE (self&& other) noexcept : value_(other.value_) { \
        other.value_ = _MSTL initialize<value_type>(); \
    } \
    constexpr SIGN## integer## BYTE (value_type&& other) noexcept : value_(other) {} \
    \
    constexpr self& operator=(self&& other) noexcept { \
        if (this != &other) { \
            value_ = other.value_; \
            other.value_ = 0; \
        } \
        return *this; \
    } \
    constexpr self& operator=(value_type&& other) noexcept { \
        value_ = other; return *this; \
    } \
    \
    MSTL_CONSTEXPR20 ~SIGN## integer## BYTE () = default; \
    \
    MSTL_NODISCARD constexpr explicit operator bool() const noexcept { \
        return value_ != _MSTL initialize<value_type>(); \
    } \
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; } \
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; } \
    static constexpr size_t bytes() noexcept { return sizeof(value_type); } \
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; } \
    \
    MSTL_NODISCARD constexpr size_t to_hash() const noexcept { \
        return _MSTL hash<value_type>()(value_); \
    } \
    \
    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) { \
        return _INNER __int_to_string_dispatch(value); \
    } \
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const { \
        return _INNER __int_to_string_dispatch(value_); \
    } \
    \
    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view str) { \
        return self{_MSTL to_## SIGN## int## BYTE (str)}; \
    } \
    \
    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept { \
        try { \
            *this = self::parse(str); \
            return true; \
        } catch (...) { \
            value_ = _MSTL initialize<value_type>(); \
            return false; \
        } \
    } \
    \
    constexpr void swap(self& other) noexcept { \
        _MSTL swap(value_, other.value_); \
    } \
    \
    constexpr bool operator==(const self& other) const noexcept { \
        return value_ == other.value_; \
    } \
    constexpr bool operator!=(const self& other) const noexcept { \
        return value_ != other.value_; \
    } \
    constexpr bool operator<(const self& other) const noexcept { \
        return value_ < other.value_; \
    } \
    constexpr bool operator<=(const self& other) const noexcept { \
        return value_ <= other.value_; \
    } \
    constexpr bool operator>(const self& other) const noexcept { \
        return value_ > other.value_; \
    } \
    constexpr bool operator>=(const self& other) const noexcept { \
        return value_ >= other.value_; \
    } \
    \
    constexpr self operator+(const self& other) const { \
        return self(value_ + other.value_); \
    } \
    constexpr self operator-(const self& other) const { \
        return self(value_ - other.value_); \
    } \
    constexpr self operator*(const self& other) const { \
        return self(value_ * other.value_); \
    } \
    constexpr self operator-() const { \
        return self(0 - value_); \
    } \
    \
    MSTL_NODISCARD constexpr self operator/(const self& other) const { \
        if (other.value_ == 0) throw_exception(math_exception("Division by zero")); \
        return self(value_ / other.value_); \
    } \
    MSTL_NODISCARD constexpr self operator%(const self& other) const { \
        if (other.value_ == 0) throw_exception(math_exception("Division by zero")); \
        return self(value_ % other.value_); \
    } \
    \
    constexpr self& operator+=(const self& other) { \
        value_ += other.value_; \
        return *this; \
    } \
    constexpr self& operator-=(const self& other) { \
        value_ -= other.value_; \
        return *this; \
    } \
    constexpr self& operator*=(const self& other) { \
        value_ *= other.value_; \
        return *this; \
    } \
    constexpr self& operator/=(const self& other) { \
        *this = *this / other; \
        return *this; \
    } \
    constexpr self& operator%=(const self& other) { \
        *this = *this % other; \
        return *this; \
    } \
    \
    constexpr self operator&(const self& other) const { \
        return self(value_ & other.value_); \
    } \
    constexpr self operator|(const self& other) const { \
        return self(value_ | other.value_); \
    } \
    constexpr self operator^(const self& other) const { \
        return self(value_ ^ other.value_); \
    } \
    constexpr self operator~() const { \
        return self(~value_); \
    } \
    \
    MSTL_NODISCARD constexpr self operator<<(const uint32_t shift) const { \
        if (shift >= static_cast<uint32_t>(bits())) throw_exception(value_exception("Shift out of size.")); \
        return self(value_ << shift); \
    } \
    MSTL_NODISCARD constexpr self operator>>(const uint32_t shift) const { \
        if (shift >= static_cast<uint32_t>(bits())) throw_exception(value_exception("Shift out of size.")); \
        return self(value_ >> shift); \
    } \
    \
    constexpr self& operator&=(const self& other) { \
        value_ &= other.value_; \
        return *this; \
    } \
    constexpr self& operator|=(const self& other) { \
        value_ |= other.value_; \
        return *this; \
    } \
    constexpr self& operator^=(const self& other) { \
        value_ ^= other.value_; \
        return *this; \
    } \
    constexpr self& operator<<=(const uint32_t shift) { \
        *this = *this << shift; \
        return *this; \
    } \
    constexpr self& operator>>=(const uint32_t shift) { \
        *this = *this >> shift; \
        return *this; \
    } \
    \
    constexpr self& operator++() { \
        ++value_; \
        return *this; \
    } \
    constexpr self operator++(int) { \
        self temp(*this); \
        ++value_; \
        return temp; \
    } \
    constexpr self& operator--() { \
        --value_; \
        return *this; \
    } \
    constexpr self operator--(int) { \
        self temp(*this); \
        --value_; \
        return temp; \
    } \
}; \
template <> \
struct package_base<SIGN## int## BYTE## _t> { \
    using type = SIGN## integer## BYTE; \
}; \
template <> \
struct unpackage_base<SIGN## integer## BYTE> { \
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
struct package_base<long long> {
    using type = integer64;
};
template <>
struct package_base<unsigned long long> {
    using type = uinteger64;
};
#else
template <>
struct package_base<long> {
    using type = integer32;
};
template <>
struct package_base<unsigned long> {
    using type = uinteger32;
};
#endif


struct float32 : iserialize<float32>, iarithmetic<float32> {
    using value_type = float32_t;
    using self = float32;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr float32 () = default;
    constexpr float32 (const self&) noexcept = default;
    constexpr float32 (const value_type& val) noexcept : value_(val) {}
    constexpr float32 & operator=(const self&) noexcept = default;
    constexpr float32 & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr float32 (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr float32 (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~float32 () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view str) {
        return self{_MSTL to_float32(str)};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        try {
            *this = self::parse(str);
            return true;
        } catch (...) {
            value_ = _MSTL initialize<value_type>();
            return false;
        }
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other. value_);
    }

    constexpr bool operator==(const self& other) const noexcept {
        return value_ == other. value_;
    }
    constexpr bool operator!=(const self& other) const noexcept {
        return value_ != other. value_;
    }
    constexpr bool operator<(const self& other) const noexcept {
        return value_ < other. value_;
    }
    constexpr bool operator<=(const self& other) const noexcept {
        return value_ <= other. value_;
    }
    constexpr bool operator>(const self& other) const noexcept {
        return value_ > other. value_;
    }
    constexpr bool operator>=(const self& other) const noexcept {
        return value_ >= other. value_;
    }

    constexpr self operator+(const self& other) const {
        return self(value_ + other. value_);
    }
    constexpr self operator-(const self& other) const {
        return self(value_ - other. value_);
    }
    constexpr self operator*(const self& other) const {
        return self(value_ * other. value_);
    }
    constexpr self operator-() const {
        return self(-value_);
    }

    MSTL_NODISCARD constexpr self operator/(const self& other) const {
        if (other. value_ == 0) throw_exception(math_exception("Division by zero"));
        return self(value_ / other. value_);
    }
    MSTL_NODISCARD constexpr self operator%(const self& other) const {
        if (other. value_ == 0) throw_exception(math_exception("Division by zero"));
        return self(_MSTL float_mod(value_, other.value_));
    }

    constexpr self& operator+=(const self& other) {
        value_ += other. value_;
        return *this;
    }
    constexpr self& operator-=(const self& other) {
        value_ -= other. value_;
        return *this;
    }
    constexpr self& operator*=(const self& other) {
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
        self temp(*this);
        ++value_;
        return temp;
    }
    constexpr self& operator--() {
        --value_;
        return *this;
    }
    constexpr self operator--(int) {
        self temp(*this);
        --value_;
        return temp;
    }
};

template <>
struct package_base<float32_t> {
    using type = float32;
};
template <>
struct unpackage_base<float32> {
    using type = float32_t;
};


struct float64 : iserialize<float64>, iarithmetic<float64> {
    using value_type = float64_t;
    using self = float64;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr float64 () = default;
    constexpr float64 (const self&) noexcept = default;
    constexpr float64 (const value_type& val) noexcept : value_(val) {}
    constexpr float64 & operator=(const self&) noexcept = default;
    constexpr float64 & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr float64 (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr float64 (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~float64 () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view str) {
        return self{_MSTL to_float64(str)};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        try {
            *this = self::parse(str);
            return true;
        } catch (...) {
            value_ = _MSTL initialize<value_type>();
            return false;
        }
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other. value_);
    }

    constexpr bool operator==(const self& other) const noexcept {
        return value_ == other. value_;
    }
    constexpr bool operator!=(const self& other) const noexcept {
        return value_ != other. value_;
    }
    constexpr bool operator<(const self& other) const noexcept {
        return value_ < other. value_;
    }
    constexpr bool operator<=(const self& other) const noexcept {
        return value_ <= other. value_;
    }
    constexpr bool operator>(const self& other) const noexcept {
        return value_ > other. value_;
    }
    constexpr bool operator>=(const self& other) const noexcept {
        return value_ >= other. value_;
    }

    constexpr self operator+(const self& other) const {
        return self(value_ + other. value_);
    }
    constexpr self operator-(const self& other) const {
        return self(value_ - other. value_);
    }
    constexpr self operator*(const self& other) const {
        return self(value_ * other. value_);
    }
    constexpr self operator-() const {
        return self(0 - value_);
    }

    MSTL_NODISCARD constexpr self operator/(const self& other) const {
        if (other. value_ == 0) throw_exception(math_exception("Division by zero"));
        return self(value_ / other. value_);
    }
    MSTL_NODISCARD constexpr self operator%(const self& other) const {
        if (other. value_ == 0) throw_exception(math_exception("Division by zero"));
        return self(_MSTL float_mod(value_, other.value_));
    }

    constexpr self& operator+=(const self& other) {
        value_ += other. value_;
        return *this;
    }
    constexpr self& operator-=(const self& other) {
        value_ -= other. value_;
        return *this;
    }
    constexpr self& operator*=(const self& other) {
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
        self temp(*this);
        ++value_;
        return temp;
    }
    constexpr self& operator--() {
        --value_;
        return *this;
    }
    constexpr self operator--(int) {
        self temp(*this);
        --value_;
        return temp;
    }
};

template <>
struct package_base<float64_t> {
    using type = float64;
};
template <>
struct unpackage_base<float64> {
    using type = float64_t;
};


struct decimal : iserialize<decimal>, iarithmetic<decimal> {
    using value_type = decimal_t;
    using self = decimal;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr decimal () = default;
    constexpr decimal (const self&) noexcept = default;
    constexpr decimal (const value_type& val) noexcept : value_(val) {}
    constexpr decimal & operator=(const self&) noexcept = default;
    constexpr decimal & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr decimal (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr decimal (value_type&& other) noexcept : value_(other) {}

    constexpr self& operator=(self&& other) noexcept {
        if (this != &other) {
            value_ = other. value_;
            other. value_ = 0;
        }
        return *this;
    }
    constexpr self& operator=(value_type&& other) noexcept {
        value_ = other; return *this;
    }

    MSTL_CONSTEXPR20 ~decimal () = default;

    MSTL_NODISCARD constexpr explicit operator bool() const noexcept {
        return value_ != _MSTL initialize<value_type>();
    }
    MSTL_NODISCARD constexpr operator value_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr value_type value() const noexcept { return value_; }
    static constexpr size_t bytes() noexcept { return sizeof(value_type); }
    static constexpr size_t bits() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string_view str) {
        return self{_MSTL to_decimal(str)};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string_view str) noexcept {
        try {
            *this = self::parse(str);
            return true;
        } catch (...) {
            value_ = _MSTL initialize<value_type>();
            return false;
        }
    }

    constexpr void swap(self& other) noexcept {
        _MSTL swap(value_, other. value_);
    }

    constexpr bool operator==(const self& other) const noexcept { return value_ == other. value_; }
    constexpr bool operator!=(const self& other) const noexcept { return value_ != other. value_; }
    constexpr bool operator<(const self& other) const noexcept { return value_ < other. value_; }
    constexpr bool operator<=(const self& other) const noexcept { return value_ <= other. value_; }
    constexpr bool operator>(const self& other) const noexcept { return value_ > other. value_; }
    constexpr bool operator>=(const self& other) const noexcept { return value_ >= other. value_; }

    constexpr self operator+(const self& other) const { return self(value_ + other. value_); }
    constexpr self operator-(const self& other) const { return self(value_ - other. value_); }
    constexpr self operator*(const self& other) const { return self(value_ * other. value_); }
    constexpr self operator-() const { return self(0 - value_); }

    MSTL_NODISCARD constexpr self operator/(const self& other) const {
        if (other. value_ == 0) throw_exception(math_exception("Division by zero"));
        return self(value_ / other. value_);
    }
    MSTL_NODISCARD constexpr self operator%(const self& other) const {
        if (other. value_ == 0) throw_exception(math_exception("Division by zero"));
        return self(_MSTL float_mod(value_, other.value_));
    }

    constexpr self& operator+=(const self& other) { value_ += other. value_; return *this; }
    constexpr self& operator-=(const self& other) { value_ -= other. value_; return *this; }
    constexpr self& operator*=(const self& other) { value_ *= other. value_; return *this; }
    constexpr self& operator/=(const self& other) { *this = *this / other; return *this; }
    constexpr self& operator%=(const self& other) { *this = *this % other; return *this; }

    constexpr self& operator++() { ++value_; return *this; }
    constexpr self operator++(int) {
        self temp(*this);
        ++value_;
        return temp;
    }
    constexpr self& operator--() {
        --value_;
        return *this;
    }
    constexpr self operator--(int) {
        self temp(*this);
        --value_;
        return temp;
    }
};

template <>
struct package_base<decimal_t> {
    using type = decimal;
};
template <>
struct unpackage_base<decimal> {
    using type = decimal_t;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_PACKAGES_HPP__
