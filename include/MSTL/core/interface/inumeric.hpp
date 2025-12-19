#ifndef MSTL_CORE_INTERFACE_INUMERIC_HPP__
#define MSTL_CORE_INTERFACE_INUMERIC_HPP__
#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct iarithmetic {
private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }
    constexpr T& derived() noexcept {
        return static_cast<T&>(*this);
    }

public:
    MSTL_NODISCARD constexpr T operator +(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator+=(other))) {
        T tmp(derived());
        tmp += other;
        return tmp;
    }
    
    MSTL_NODISCARD constexpr T operator -(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator-=(other))) {
        T tmp(derived());
        tmp -= other;
        return tmp;
    }
    
    MSTL_NODISCARD constexpr T operator *(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator*=(other))) {
        T tmp(derived());
        tmp *= other;
        return tmp;
    }
    
    MSTL_NODISCARD constexpr T operator /(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator/=(other))) {
        T tmp(derived());
        tmp /= other;
        return tmp;
    }
    
    MSTL_NODISCARD constexpr T operator %(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator%=(other))) {
        T tmp(derived());
        tmp %= other;
        return tmp;
    }

    MSTL_NODISCARD constexpr T operator -() const
    noexcept(noexcept(derived().operator-())) {
        return derived().operator-();
    }

    constexpr T& operator +=(const T& other)
    noexcept(noexcept(derived().operator+=(other))) {
        return derived().operator+=(other);
    }
    
    constexpr T& operator -=(const T& other)
    noexcept(noexcept(derived().operator-=(other))) {
        return derived().operator-=(other);
    }
    
    constexpr T& operator *=(const T& other)
    noexcept(noexcept(derived().operator*=(other))) {
        return derived().operator*=(other);
    }
    
    constexpr T& operator /=(const T& other) {
        return derived().operator/=(other);
    }
    
    constexpr T& operator %=(const T& other) {
        return derived().operator%=(other);
    }

    constexpr T& operator ++()
    noexcept(noexcept(derived().operator++())) {
        return derived().operator++();
    }
    
    constexpr T operator ++(int)
    noexcept(noexcept(derived().operator++())) {
        T tmp(derived());
        ++derived();
        return tmp;
    }
    
    constexpr T& operator --()
    noexcept(noexcept(derived().operator--())) {
        return derived().operator--();
    }
    
    constexpr T operator --(int)
    noexcept(noexcept(derived().operator--())) {
        T tmp(derived());
        --derived();
        return tmp;
    }
};


template <typename T>
struct ibinary {
private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }
    constexpr T& derived() noexcept {
        return static_cast<T&>(*this);
    }

public:
    MSTL_NODISCARD constexpr T operator &(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator&=(other))) {
        T tmp(derived());
        tmp &= other;
        return tmp;
    }

    MSTL_NODISCARD constexpr T operator |(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator|=(other))) {
        T tmp(derived());
        tmp |= other;
        return tmp;
    }

    MSTL_NODISCARD constexpr T operator ^(const T& other) const
    noexcept(noexcept(const_cast<T&>(derived()).operator^=(other))) {
        T tmp(derived());
        tmp ^= other;
        return tmp;
    }

    MSTL_NODISCARD constexpr T operator ~() const
    noexcept(noexcept(derived().operator~())) {
        return derived().operator~();
    }

    MSTL_NODISCARD constexpr T operator <<(const uint32_t shift) const {
        T tmp(derived());
        tmp <<= shift;
        return tmp;
    }

    MSTL_NODISCARD constexpr T operator >>(const uint32_t shift) const {
        T tmp(derived());
        tmp >>= shift;
        return tmp;
    }

    constexpr T& operator &=(const T& other)
    noexcept(noexcept(derived().operator&=(other))) {
        return derived().operator&=(other);
    }

    constexpr T& operator |=(const T& other)
    noexcept(noexcept(derived().operator|=(other))) {
        return derived().operator|=(other);
    }

    constexpr T& operator ^=(const T& other)
    noexcept(noexcept(derived().operator^=(other))) {
        return derived().operator^=(other);
    }

    constexpr T& operator <<=(const uint32_t shift) {
        return derived().operator<<=(shift);
    }

    constexpr T& operator >>=(const uint32_t shift) {
        return derived().operator>>=(shift);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_INUMERIC_HPP__
