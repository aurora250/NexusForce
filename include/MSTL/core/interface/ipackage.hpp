#ifndef MSTL_CORE_INTERFACE_IPACKAGE_HPP__
#define MSTL_CORE_INTERFACE_IPACKAGE_HPP__
#include "icommon.hpp"
#include "inumeric.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename PackageT>
struct ipackage : icommon<T>, iarithmetic<T>, ibinary<T> {
    static_assert(is_arithmetic_v<PackageT>, "PackageT must be arithmetic.");

private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }
    constexpr T& derived() noexcept {
        return static_cast<T&>(*this);
    }

public:
    using package_type = PackageT;

protected:
    package_type value_{_MSTL initialize<package_type>()};

public:
    constexpr ipackage() noexcept = default;
    constexpr ipackage(package_type val) noexcept : value_(val) {}
    constexpr ipackage(const ipackage& other) noexcept : value_(other.value_) {}
    constexpr ipackage(ipackage&& other) noexcept : value_(other.value_) {
        other.value_ = initialize<package_type>();
    }
    MSTL_CONSTEXPR20 ~ipackage() = default;

    MSTL_NODISCARD constexpr operator package_type() const noexcept { return value_; }
    MSTL_NODISCARD constexpr package_type value() const noexcept { return value_; }
    MSTL_NODISCARD constexpr int64_t to_int64() const noexcept { return static_cast<int64_t>(value_); }

    MSTL_NODISCARD static constexpr size_t bytes() noexcept { return sizeof(package_type); }
    MSTL_NODISCARD static constexpr size_t bits() noexcept { return sizeof(package_type) * 8; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _MSTL hash<package_type>()(value_);
    }

    constexpr void swap(T& other) noexcept {
        _MSTL swap(value_, other.value_);
    }

    MSTL_NODISCARD constexpr bool operator ==(const T& other) const noexcept {
        return value_ == other.value_;
    }
    MSTL_NODISCARD constexpr bool operator <(const T& other) const noexcept {
        return value_ < other.value_;
    }

    constexpr T& operator +=(const T& other) noexcept {
        value_ += other.value_;
        return derived();
    }
    constexpr T& operator -=(const T& other) noexcept {
        value_ -= other.value_;
        return derived();
    }
    constexpr T& operator *=(const T& other) noexcept {
        value_ *= other.value_;
        return derived();
    }
    constexpr T& operator /=(const T& other) {
        if (other.value_ == 0) throw_exception(math_exception("Division by zero"));
        value_ /= other;
        return derived();
    }
    constexpr T& operator %=(const T& other) {
        value_ = _MSTL float_mod(value_, other.value_);
        return derived();
    }

    MSTL_NODISCARD constexpr T operator -() const noexcept {
        return T(-value_);
    }

    constexpr T& operator ++() noexcept {
        ++value_;
        return derived();
    }
    constexpr T& operator --() noexcept {
        --value_;
        return derived();
    }

    constexpr T operator ~() const noexcept {
        return T{~value_};
    }

    constexpr T& operator &=(const T& other) noexcept {
        value_ &= other.value_;
        return derived();
    }
    constexpr T& operator |=(const T& other) noexcept {
        value_ |= other.value_;
        return derived();
    }
    constexpr T& operator ^=(const T& other) noexcept {
        value_ ^= other.value_;
        return derived();
    }

    constexpr T& operator <<=(const uint32_t shift) {
        if (shift >= 64) throw_exception(value_exception("Shift count out of range"));
        value_ <<= shift;
        return derived();
    }
    constexpr T& operator >>=(const uint32_t shift) {
        if (shift >= 64) throw_exception(value_exception("Shift count out of range"));
        value_ >>= shift;
        return derived();
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_IPACKAGE_HPP__
