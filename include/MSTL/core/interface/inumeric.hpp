#ifndef MSTL_CORE_INTERFACE_INUMERIC_HPP__
#define MSTL_CORE_INTERFACE_INUMERIC_HPP__
#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct iarithmetic {
public:
    using self = iarithmetic<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_CONSTEXPR20 ~iarithmetic() = default;

    MSTL_NODISCARD constexpr child_type operator +(const child_type& other) const
    noexcept(noexcept(self::to_template(this)->operator+(other))) {
        return self::to_template(this)->operator+(other);
    }
    MSTL_NODISCARD constexpr child_type operator -(const child_type& other) const
    noexcept(noexcept(self::to_template(this)->operator-(other))) {
        return self::to_template(this)->operator-(other);
    }
    MSTL_NODISCARD constexpr child_type operator *(const child_type& other) const
    noexcept(noexcept(self::to_template(this)->operator*(other))) {
        return self::to_template(this)->operator*(other);
    }
    MSTL_NODISCARD constexpr child_type operator /(const child_type& other) const {
        return self::to_template(this)->operator/(other);
    }
    MSTL_NODISCARD constexpr child_type operator %(const child_type& other) const {
        return self::to_template(this)->operator%(other);
    }

    MSTL_NODISCARD constexpr child_type operator -() const
    noexcept(noexcept(self::to_template(this)->operator-())) {
        return self::to_template(this)->operator-();
    }

    constexpr child_type& operator +=(const child_type& other)
    noexcept(noexcept(self::to_template(this)->operator+=(other))) {
        return self::to_template(this)->operator+=(other);
    }
    constexpr child_type& operator -=(const child_type& other)
    noexcept(noexcept(self::to_template(this)->operator-=(other))) {
        return self::to_template(this)->operator-=(other);
    }
    constexpr child_type& operator *=(const child_type& other)
    noexcept(noexcept(self::to_template(this)->operator*=(other))) {
        return self::to_template(this)->operator*=(other);
    }
    constexpr child_type& operator /=(const child_type& other) {
        return self::to_template(this)->operator/=(other);
    }
    constexpr child_type& operator %=(const child_type& other) {
        return self::to_template(this)->operator%=(other);
    }

    constexpr child_type& operator ++()
    noexcept(noexcept(self::to_template(this)->operator++())) {
        return self::to_template(this)->operator++();
    }
    constexpr child_type operator ++(int)
    noexcept(noexcept(self::to_template(this)->operator++(int()))) {
        return self::to_template(this)->operator++(int());
    }
    constexpr child_type& operator --()
    noexcept(noexcept(self::to_template(this)->operator--())) {
        return self::to_template(this)->operator--();
    }
    constexpr child_type operator --(int)
    noexcept(noexcept(self::to_template(this)->operator--(int()))) {
        return self::to_template(this)->operator--(int());
    }
};


template <typename T>
struct ibinary {
public:
    using self = ibinary<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_CONSTEXPR20 ~ibinary() = default;

    MSTL_NODISCARD constexpr child_type operator &(const child_type& other) const
    noexcept(noexcept(self::to_template(this)->operator&(other))) {
        return self::to_template(this)->operator&(other);
    }
    MSTL_NODISCARD constexpr child_type operator |(const child_type& other) const
    noexcept(noexcept(self::to_template(this)->operator|(other))) {
        return self::to_template(this)->operator|(other);
    }
    MSTL_NODISCARD constexpr child_type operator ^(const child_type& other) const
    noexcept(noexcept(self::to_template(this)->operator^(other))) {
        return self::to_template(this)->operator^(other);
    }
    MSTL_NODISCARD constexpr child_type operator ~() const
    noexcept(noexcept(self::to_template(this)->operator~())) {
        return self::to_template(this)->operator~();
    }
    MSTL_NODISCARD constexpr child_type operator <<(const uint32_t shift) const {
        return self::to_template(this)->operator<<(shift);
    }
    MSTL_NODISCARD constexpr child_type operator >>(const uint32_t shift) const {
        return self::to_template(this)->operator>>(shift);
    }

    MSTL_NODISCARD constexpr child_type operator &=(const child_type& other)
    noexcept(noexcept(self::to_template(this)->operator&=(other))) {
        return self::to_template(this)->operator&=(other);
    }
    MSTL_NODISCARD constexpr child_type operator |=(const child_type& other)
    noexcept(noexcept(self::to_template(this)->operator|=(other))) {
        return self::to_template(this)->operator|=(other);
    }
    MSTL_NODISCARD constexpr child_type operator ^=(const child_type& other)
    noexcept(noexcept(self::to_template(this)->operator^=(other))) {
        return self::to_template(this)->operator^=(other);
    }
    MSTL_NODISCARD constexpr child_type operator <<=(const uint32_t shift) {
        return self::to_template(this)->operator<<=(shift);
    }
    MSTL_NODISCARD constexpr child_type operator >>=(const uint32_t shift) {
        return self::to_template(this)->operator>>=(shift);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_INUMERIC_HPP__
