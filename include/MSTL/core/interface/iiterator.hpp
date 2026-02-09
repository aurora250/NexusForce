#ifndef MSTL_CORE_INTERFACE_IITERATOR_HPP__
#define MSTL_CORE_INTERFACE_IITERATOR_HPP__
#include "MSTL/core/typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator>
struct iiterator {
public:
private:
    constexpr Iterator& derived() noexcept {
        return static_cast<Iterator&>(*this);
    }

    constexpr const Iterator& derived() const noexcept {
        return static_cast<const Iterator&>(*this);
    }

public:
    MSTL_NODISCARD constexpr decltype(auto) operator *() const noexcept {
        return derived().dereference();
    }

    MSTL_NODISCARD constexpr decltype(auto) operator ->() const noexcept {
        return &(derived().dereference());
    }

    constexpr Iterator& operator ++() noexcept {
        derived().increment();
        return derived();
    }

    constexpr Iterator operator ++(int) noexcept {
        Iterator temp = derived();
        derived().increment();
        return temp;
    }

    constexpr Iterator& operator --() noexcept {
        derived().decrement();
        return derived();
    }

    constexpr Iterator operator --(int) noexcept {
        Iterator temp = derived();
        derived().decrement();
        return temp;
    }

    constexpr Iterator& operator +=(auto&& n) noexcept {
        derived().advance(n);
        return derived();
    }

    MSTL_NODISCARD constexpr Iterator operator +(auto&& n) const noexcept {
        Iterator temp = derived();
        temp.advance(n);
        return temp;
    }

    MSTL_NODISCARD friend constexpr Iterator operator +(auto&& n, const iiterator& it) noexcept {
        return it.derived() + n;
    }

    constexpr Iterator& operator -=(auto&& n) noexcept {
        derived().advance(-n);
        return derived();
    }

    template <typename T>
    MSTL_NODISCARD constexpr enable_if_t<!is_same_v<T, Iterator>, Iterator>
    operator -(const T n) const noexcept {
        Iterator temp = derived();
        temp.advance(-n);
        return temp;
    }

    MSTL_NODISCARD constexpr decltype(auto)
    operator -(const Iterator& other) const noexcept {
        return derived().distance_to(other);
    }

    MSTL_NODISCARD constexpr bool operator ==(const Iterator& rhs) const noexcept {
        return derived().equal(rhs);
    }

    MSTL_NODISCARD constexpr bool operator !=(const Iterator& rhs) const noexcept {
        return !(*this == rhs);
    }

    MSTL_NODISCARD constexpr bool operator <(const Iterator& rhs) const noexcept {
        return derived().less_than(rhs);
    }

    MSTL_NODISCARD constexpr bool operator >(const Iterator& rhs) const noexcept {
        return rhs < derived();
    }

    MSTL_NODISCARD constexpr bool operator <=(const Iterator& rhs) const noexcept {
        return !(derived() > rhs);
    }

    MSTL_NODISCARD constexpr bool operator >=(const Iterator& rhs) const noexcept {
        return !(derived() < rhs);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_IITERATOR_HPP__
