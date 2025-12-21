#ifndef MSTL_CORE_INTERFACE_ICOMMON_HPP__
#define MSTL_CORE_INTERFACE_ICOMMON_HPP__
#include "../functional/hash.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct ihashable {
private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }

public:
    MSTL_NODISCARD constexpr size_t to_hash() const
    noexcept(noexcept(derived().to_hash())) {
        return derived().to_hash();
    }
};

template <typename T>
struct hash<T, enable_if_t<is_base_of_v<ihashable<T>, T>>> {
    MSTL_NODISCARD constexpr size_t operator ()(const T& obj) const
    noexcept(noexcept(obj.to_hash())) {
        return obj.to_hash();
    }
};


template <typename T>
struct iswappable {
private:
    constexpr T& derived() noexcept {
        return static_cast<T&>(*this);
    }

public:
    constexpr void swap(T& other)
    noexcept(noexcept(derived().swap(other))) {
        derived().swap(other);
    }
};

template <typename T, enable_if_t<is_base_of_v<iswappable<T>, T>, int> = 0>
constexpr void swap(T& lhs, T& rhs)
noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}


template <typename T>
struct icomparable {
private:
    constexpr const T& derived() const noexcept {
        return static_cast<const T&>(*this);
    }

public:
    MSTL_NODISCARD constexpr bool operator ==(const T& rhs) const
        noexcept(noexcept(derived() == rhs)) {
        return derived() == rhs;
    }

    MSTL_NODISCARD constexpr bool operator !=(const T& rhs) const
        noexcept(noexcept(!(*this == rhs))) {
        return !(*this == rhs);
    }

    MSTL_NODISCARD constexpr bool operator <(const T& rhs) const
        noexcept(noexcept(derived() < rhs)) {
        return derived() < rhs;
    }

    MSTL_NODISCARD constexpr bool operator >(const T& rhs) const
        noexcept(noexcept(rhs < derived())) {
        return rhs < derived();
    }

    MSTL_NODISCARD constexpr bool operator <=(const T& rhs) const
        noexcept(noexcept(!(derived() > rhs))) {
        return !(derived() > rhs);
    }

    MSTL_NODISCARD constexpr bool operator >=(const T& rhs) const
        noexcept(noexcept(!(derived() < rhs))) {
        return !(derived() < rhs);
    }
};


template <typename T>
struct icommon : icomparable<T>, iswappable<T>, ihashable<T> {};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_INTERFACE_ICOMMON_HPP__
