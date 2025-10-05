#ifndef MSTL_OBJECT_HPP__
#define MSTL_OBJECT_HPP__
#include "algo.hpp"
#include "format.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct object : icommon<T> {
public:
    using self = object<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_CONSTEXPR20 ~object() = default;

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return self::to_template(this)->to_string();
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 child_type parse(const string& str) {
        return child_type::parse(str);
    }
    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept {
        return self::to_template(this)->try_parse(str);
    }
};

template <typename T, enable_if_t<is_base_of_v<object<T>, T>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const object<T>& obj) {
    return obj.to_string();
}
template <typename T, enable_if_t<is_packaged_v<T>, int> = 0>
MSTL_CONSTEXPR20 string to_string(const T& value) {
    return package_t<T>(value).to_string();
}


template <typename T, enable_if_t<is_packaged_v<T>, int> = 0>
constexpr package_t<T> make_package(T&& value) noexcept {
    return package_t<T>(_MSTL forward<T>(value));
}

template <typename T, enable_if_t<is_unpackaged_v<T>, int> = 0>
constexpr unpackage_t<T> make_unpackage(T&& value) noexcept {
    return static_cast<unpackage_t<T>>(_MSTL forward<T>(value));
}


struct boolean : object<boolean>, ibinary<boolean> {
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
    static MSTL_CONSTEVAL size_t bytes() noexcept { return sizeof(value_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return value_ ? "true" : "false";
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string& lower) {
        self obj;
        _MSTL transform(lower.begin(), lower.end(), lower.begin(), [](const byte_t c) {
            return _INNER to_lowercase(c);
        });

        if (lower == "true" || lower == "1" || lower == "yes" || lower == "y" || lower == "on") {
            obj.value_ = true;
        } else if (lower == "false" || lower == "0" || lower == "no" || lower == "n" || lower == "off") {
            obj.value_ = false;
        } else {
            Exception(TypeCastError("Convert from string to boolean failed."));
        }
        return obj;
    }

    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept {
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
    constexpr self operator &(const self& other) const noexcept { return {static_cast<value_type>(value_ & other.value_)}; }
    constexpr self operator |(const self& other) const noexcept { return {static_cast<value_type>(value_ | other.value_)}; }
    constexpr self operator ^(const self& other) const noexcept { return {static_cast<value_type>(value_ ^ other.value_)}; }

    constexpr self& operator &=(const self& other) noexcept { value_ &= other.value_; return *this; }
    constexpr self& operator |=(const self& other) noexcept { value_ |= other.value_; return *this; }
    constexpr self& operator ^=(const self& other) noexcept { value_ ^= other.value_; return *this; }
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
struct SIGN##integer##BYTE : object<SIGN##integer##BYTE>, iarithmetic<SIGN##integer##BYTE>, ibinary<SIGN##integer##BYTE> { \
    using value_type = SIGN##int##BYTE##_t;\
    using self = SIGN##integer##BYTE; \
    using base = object<SIGN##integer##BYTE>; \
    \
private: \
    value_type value_ = _MSTL initialize<value_type>(); \
    \
public: \
    constexpr SIGN##integer##BYTE () = default; \
    constexpr SIGN##integer##BYTE (const self&) noexcept = default; \
    constexpr SIGN##integer##BYTE (const value_type& val) noexcept : value_(val) {} \
    constexpr SIGN##integer##BYTE & operator=(const self&) noexcept = default; \
    constexpr SIGN##integer##BYTE & operator=(const value_type& other) noexcept { value_ = other; return *this; } \
    \
    constexpr SIGN##integer##BYTE (self&& other) noexcept : value_(other.value_) { \
        other.value_ = _MSTL initialize<value_type>(); \
    } \
    constexpr SIGN##integer##BYTE (value_type&& other) noexcept : value_(other) {} \
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
    MSTL_CONSTEXPR20 ~SIGN##integer##BYTE () = default; \
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
        return self(value).to_string(); \
    } \
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const { \
        return _INNER __int_to_string_dispatch(value_); \
    } \
    \
    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string& str) { \
        return self{_INNER to_##SIGN##int##BYTE (str.c_str())}; \
    } \
    \
    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept { \
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
        if (other.value_ == 0) Exception(MathError("Division by zero")); \
        return self(value_ / other.value_); \
    } \
    MSTL_NODISCARD constexpr self operator%(const self& other) const { \
        if (other.value_ == 0) Exception(MathError("Division by zero")); \
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
        if (shift >= static_cast<uint32_t>(bits())) Exception(ValueError("Shift out of size.")); \
        return self(value_ << shift); \
    } \
    MSTL_NODISCARD constexpr self operator>>(const uint32_t shift) const { \
        if (shift >= static_cast<uint32_t>(bits())) Exception(ValueError("Shift out of size.")); \
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
struct package<SIGN##int##BYTE##_t> { \
    using type = SIGN##integer##BYTE; \
}; \
template <> \
struct unpackage<SIGN##integer##BYTE> { \
    using type = SIGN##int##BYTE##_t; \
};

__MSTL_BUILD_INTEGER_STRUCT(,,16)
__MSTL_BUILD_INTEGER_STRUCT(,,32)
__MSTL_BUILD_INTEGER_STRUCT(,,64)
__MSTL_BUILD_INTEGER_STRUCT(u,U,16)
__MSTL_BUILD_INTEGER_STRUCT(u,U,32)
__MSTL_BUILD_INTEGER_STRUCT(u,U,64)
#undef __MSTL_BUILD_INTEGER_STRUCT


template <>
struct package<long> {
#ifdef MSTL_PLATFORM_WINDOWS__
    using type = integer32;
#elif defined(MSTL_PLATFORM_LINUX__)
    using type = integer64;
#endif
};

template <>
struct package<unsigned long> {
#ifdef MSTL_PLATFORM_WINDOWS__
    using type = uinteger32;
#elif defined(MSTL_PLATFORM_LINUX__)
    using type = uinteger64;
#endif
};


struct float32 : object<float32>, iarithmetic<float32> {
    using value_type = float32_t;
    using self = float32;
    using base = object<float32>;

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

    static constexpr value_type min_positive_value() noexcept { return FLOAT32_MIN_POSI_VALUE; }
    static constexpr value_type max_positive_value() noexcept { return FLOAT32_MAX_POSI_VALUE; }
    static constexpr value_type min_negative_value() noexcept { return FLOAT32_MIN_NEGA_VALUE; }
    static constexpr value_type max_negative_value() noexcept { return FLOAT32_MAX_NEGA_VALUE; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string& str) {
        return self{_INNER to_float32(str.c_str())};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept {
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
        if (other. value_ == 0) Exception(MathError("Division by zero"));
        return self(value_ / other. value_);
    }
    MSTL_NODISCARD constexpr self operator%(const self& other) const {
        if (other. value_ == 0) Exception(MathError("Division by zero"));
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
struct package<float32_t> {
    using type = float32;
};
template <>
struct unpackage<float32> {
    using type = float32_t;
};


struct float64 : object<float64>, iarithmetic<float64> {
    using value_type = float64_t;
    using self = float64;
    using base = object<float64>;

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

    static constexpr value_type min_positive_value() noexcept { return FLOAT64_MIN_POSI_VALUE; }
    static constexpr value_type max_positive_value() noexcept { return FLOAT64_MAX_POSI_VALUE; }
    static constexpr value_type min_negative_value() noexcept { return FLOAT64_MIN_NEGA_VALUE; }
    static constexpr value_type max_negative_value() noexcept { return FLOAT64_MAX_NEGA_VALUE; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string& str) {
        return self{_INNER to_float64(str. c_str())};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept {
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
        if (other. value_ == 0) Exception(MathError("Division by zero"));
        return self(value_ / other. value_);
    }
    MSTL_NODISCARD constexpr self operator%(const self& other) const {
        if (other. value_ == 0) Exception(MathError("Division by zero"));
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
struct package<float64_t> {
    using type = float64;
};
template <>
struct unpackage<float64> {
    using type = float64_t;
};


struct decimal : object<decimal>, iarithmetic<decimal> {
    using value_type = decimal_t;
    using self = decimal;
    using base = object<decimal>;

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

    static constexpr value_type min_positive_value() noexcept { return DECIMAL_MIN_POSI_VALUE; }
    static constexpr value_type max_positive_value() noexcept { return DECIMAL_MAX_POSI_VALUE; }
    static constexpr value_type min_negative_value() noexcept { return DECIMAL_MIN_NEGA_VALUE; }
    static constexpr value_type max_negative_value() noexcept { return DECIMAL_MAX_NEGA_VALUE; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<value_type>()(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 string to_string(const value_type value) {
        return self(value).to_string();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return _INNER __float_to_string<char>(value_);
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string& str) {
        return self{_INNER to_decimal(str.c_str())};
    }

    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept {
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
        if (other. value_ == 0) Exception(MathError("Division by zero"));
        return self(value_ / other. value_);
    }
    MSTL_NODISCARD constexpr self operator%(const self& other) const {
        if (other. value_ == 0) Exception(MathError("Division by zero"));
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
struct package<decimal_t> {
    using type = decimal;
};
template <>
struct unpackage<decimal> {
    using type = decimal_t;
};


template <typename T>
struct icharacter : object<T> {
public:
    using base_type = object<T>;
    using self = icharacter<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return base_type::to_template(o);
    }

public:
    MSTL_CONSTEXPR20 ~icharacter() = default;

    constexpr bool is_space() const
    noexcept(noexcept(self::to_template(this)->is_space())) {
        return self::to_template(this)->is_space();
    }

    constexpr bool is_alpha() const
    noexcept(noexcept(self::to_template(this)->is_alpha())) {
        return self::to_template(this)->is_alpha();
    }

    constexpr bool is_digit() const
    noexcept(noexcept(self::to_template(this)->is_digit())) {
        return self::to_template(this)->is_digit();
    }

    constexpr bool is_xdigit() const
    noexcept(noexcept(self::to_template(this)->is_xdigit())) {
        return self::to_template(this)->is_xdigit();
    }

    constexpr bool is_alpha_or_digit() const
    noexcept(noexcept(self::to_template(this)->is_alpha_or_digit())) {
        return self::to_template(this)->is_alpha_or_digit();
    }

    constexpr bool is_digit_or_alpha() const
    noexcept(noexcept(self::to_template(this)->is_digit_or_alpha())) {
        return self::to_template(this)->is_digit_or_alpha();
    }

    constexpr void to_lowercase()
    noexcept(noexcept(self::to_template(this)->to_lowercase())) {
        return self::to_template(this)->to_lowercase();
    }

    constexpr void to_uppercase()
    noexcept(noexcept(self::to_template(this)->to_uppercase())) {
        return self::to_template(this)->to_uppercase();
    }

    static constexpr T to_lowercase(icharacter value)
    noexcept(noexcept(value.to_lowercase())) {
        return value.to_lowercase();
    }
    static constexpr T to_uppercase(icharacter value)
    noexcept(noexcept(value.to_uppercase())) {
        return value.to_uppercase();
    }
};

template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr bool is_space(const icharacter<T>& obj)
noexcept(noexcept(obj.to_space())) {
    return obj.is_space();
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr bool is_alpha(const icharacter<T>& obj)
noexcept(noexcept(obj.is_alpha())) {
    return obj.is_alpha();
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr bool is_digit(const icharacter<T>& obj)
noexcept(noexcept(obj.is_digit())) {
    return obj.is_digit();
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr bool is_xdigit(const icharacter<T>& obj)
noexcept(noexcept(obj.is_xdigit())) {
    return obj.is_xdigit();
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr bool is_alpha_or_digit(const icharacter<T>& obj)
noexcept(noexcept(obj.is_alpha_or_digit())) {
    return obj.is_alpha_or_digit();
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr bool is_digit_or_alpha(const icharacter<T>& obj)
noexcept(noexcept(obj.is_digit_or_alpha())) {
    return obj.is_digit_or_alpha();
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr T to_lowercase(const icharacter<T>& obj)
noexcept(noexcept(icharacter<T>::to_lowercase(obj))) {
    return icharacter<T>::to_lowercase(obj);
}
template <typename T, enable_if_t<is_base_of_v<icharacter<T>, T>, int> = 0>
constexpr T to_uppercase(const icharacter<T>& obj)
noexcept(noexcept(icharacter<T>::to_uppercase(obj))) {
    return icharacter<T>::to_uppercase(obj);
}


struct character : icharacter<char> {
    using value_type = char;
    using self = character;
    using base = icharacter<char>;

private:
    value_type value_ = _MSTL initialize<value_type>();

public:
    constexpr character () = default;
    constexpr character (const self&) noexcept = default;
    constexpr character (const value_type& val) noexcept : value_(val) {}
    constexpr character & operator=(const self&) noexcept = default;
    constexpr character & operator=(const value_type& other) noexcept { value_ = other; return *this; }

    constexpr character (self&& other) noexcept : value_(other. value_) {
        other. value_ = _MSTL initialize<value_type>();
    }
    constexpr character (value_type&& other) noexcept : value_(other) {}

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

    MSTL_CONSTEXPR20 ~character () = default;

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
        return {1, value_};
    }

    MSTL_NODISCARD static MSTL_CONSTEXPR20 self parse(const string& str) {
        if (str.empty()) {
            Exception(TypeCastError("Cannot convert empty string to character8."));
        }
        if (str.size() > 1) {
            Exception(TypeCastError("String too long to convert to character8."));
        }
        return self(str[0]);
    }

    MSTL_CONSTEXPR20 bool try_parse(const string& str) noexcept {
        try {
            *this = self::parse(str);
            return true;
        } catch (...) {
            value_ = _MSTL initialize<value_type>();
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

    constexpr bool is_space() const noexcept { return _INNER is_space(value_); }
    constexpr bool is_alpha() const noexcept { return _INNER is_alpha(value_); }
    constexpr bool is_digit() const noexcept { return _INNER is_digit(value_); }
    constexpr bool is_xdigit() const noexcept { return _INNER is_xdigit(value_); }
    constexpr bool is_alpha_or_digit() const noexcept { return _INNER is_alpha_or_digit(value_); }
    constexpr bool is_digit_or_alpha() const noexcept { return _INNER is_digit_or_alpha(value_); }

    constexpr void to_lowercase() noexcept { value_ = _INNER to_lowercase(value_); }
    constexpr void to_uppercase() noexcept { value_ = _INNER to_uppercase(value_); }
};

template <>
struct package<char> {
    using type = character;
};

template <>
struct unpackage<character> {
    using type = char;
};


template <typename Collector, typename ValueType, typename Iterator, typename ConstIterator>
struct collector : object<Collector> {
private:
    using collector_type    = Collector;
    using self              = collector;
    using base_type         = object<Collector>;

    static constexpr collector_type* to_template(const collector_type* o) noexcept {
        return base_type::to_template(o);
    }

protected:


public:
    MSTL_BUILD_TYPE_ALIAS(ValueType)
    using iterator					= Iterator;
    using const_iterator			= ConstIterator;
    using reverse_iterator			= _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator	= _MSTL reverse_iterator<const_iterator>;


    MSTL_CONSTEXPR20 ~collector() = default;

    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator begin()
    noexcept(noexcept(self::to_template(this)->begin())) {
        return self::to_template(this)->begin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator end()
    noexcept(noexcept(self::to_template(this)->end())) {
        return self::to_template(this)->end();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator begin() const
    noexcept(noexcept(self::to_template(this)->begin())) {
        return self::to_template(this)->begin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator end() const
    noexcept(noexcept(self::to_template(this)->end())) {
        return self::to_template(this)->end();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cbegin() const
    noexcept(noexcept(self::to_template(this)->cbegin())) {
        return self::to_template(this)->cbegin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cend() const
    noexcept(noexcept(self::to_template(this)->cend())) {
        return self::to_template(this)->cend();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rbegin()
    noexcept(noexcept(self::to_template(this)->rbegin())) {
        return self::to_template(this)->rbegin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rend()
    noexcept(noexcept(self::to_template(this)->rend())) {
        return self::to_template(this)->rend();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rbegin() const
    noexcept(noexcept(self::to_template(this)->rbegin())) {
        return self::to_template(this)->rbegin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rend() const
    noexcept(noexcept(self::to_template(this)->rend())) {
        return self::to_template(this)->rend();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crbegin() const
    noexcept(noexcept(self::to_template(this)->crbegin())) {
        return self::to_template(this)->crbegin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crend() const
    noexcept(noexcept(self::to_template(this)->crend())) {
        return self::to_template(this)->crend();
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const
    noexcept(noexcept(self::to_template(this)->size())) {
        return self::to_template(this)->size();
    }
    MSTL_NODISCARD static constexpr size_type max_size()
    noexcept(noexcept(collector_type::max_size())) {
        return collector_type::max_size();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type capacity() const
    noexcept(noexcept(self::to_template(this)->capacity())) {
        return self::to_template(this)->capacity();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const
    noexcept(noexcept(self::to_template(this)->empty())) {
        return self::to_template(this)->empty();
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer data()
    noexcept(noexcept(self::to_template(this)->data())) {
        return self::to_template(this)->data();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer data() const
    noexcept(noexcept(self::to_template(this)->data())) {
        return self::to_template(this)->data();
    }

    MSTL_CONSTEXPR20 void reserve(const size_type n) {
        self::to_template(this)->reserve(n);
    }
    MSTL_CONSTEXPR20 void resize(size_type new_size, const value_type& x) {
        self::to_template(this)->resize(new_size, x);
    }
    MSTL_CONSTEXPR20 void resize(const size_type new_size) {
        self::to_template(this)->resize(new_size);
    }

    MSTL_CONSTEXPR20 void assign(size_type n, const value_type& value) {
        self::to_template(this)->assign(n, value);
    }

    MSTL_CONSTEXPR20 iterator insert(iterator position, const value_type& x) {
        return self::to_template(this)->insert(position, x);
    }
    MSTL_CONSTEXPR20 iterator insert(iterator position, value_type&& x) {
        return self::to_template(this)->insert(position, _MSTL move(x));
    }
    MSTL_CONSTEXPR20 iterator insert(iterator position) {
        return to_template(this)->insert(position);
    }
    MSTL_CONSTEXPR20 void insert(iterator position, size_type n, const value_type& x) {
        self::to_template(this)->insert(position, n, x);
    }

    MSTL_CONSTEXPR20 iterator erase(iterator first, iterator last)
    noexcept(noexcept(self::to_template(this)->erase(first, last))) {
        return self::to_template(this)->erase(first, last);
    }
    MSTL_CONSTEXPR20 iterator erase(iterator position)
    noexcept(noexcept(self::to_template(this)->erase(position))) {
        return self::to_template(this)->erase(position);
    }

    MSTL_CONSTEXPR20 void shrink_to_fit()
    noexcept(noexcept(self::to_template(this)->shrink_to_fit())) {
        self::to_template(this)->shrink_to_fit();
    }
    MSTL_CONSTEXPR20 void clear()
    noexcept(noexcept(self::to_template(this)->clear())) {
        self::to_template(this)->clear();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_OBJECT_HPP__
