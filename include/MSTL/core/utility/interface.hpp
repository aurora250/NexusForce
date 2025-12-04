#ifndef MSTL_CORE_UTILITY_INTERFACE_HPP__
#define MSTL_CORE_UTILITY_INTERFACE_HPP__
#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/types.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct ihashable {
public:
    using self = ihashable<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_NODISCARD constexpr size_t to_hash() const
    noexcept(noexcept(self::to_template(this)->to_hash())) {
        return self::to_template(this)->to_hash();
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
public:
    using self = iswappable<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<child_type*>(static_cast<const child_type*>(o));
    }

public:
    constexpr void swap(child_type& other)
    noexcept(noexcept(self::to_template(this)->swap(other))) {
        self::to_template(this)->swap(other);
    }
};

template <typename T, enable_if_t<is_base_of_v<iswappable<T>, T>, int> = 0>
constexpr void swap(T& lh, T& rh)
noexcept(noexcept(lh.swap(rh))) {
    lh.swap(rh);
}


template <typename T>
struct icomparable {
    using self = icomparable<T>;
    using child_type = T;

private:
    static constexpr child_type* to_template(const self* o) noexcept {
        return const_cast<T*>(static_cast<const T*>(o));
    }

public:
    MSTL_NODISCARD constexpr bool operator ==(const child_type& rh) const
    noexcept(noexcept(self::to_template(this)->operator==(rh))) {
        return self::to_template(this)->operator ==(rh);
    }
    MSTL_NODISCARD constexpr bool operator !=(const child_type& rh) const
    noexcept(noexcept(!(*self::to_template(this) == rh))) {
        return !(*this == rh);
    }
    MSTL_NODISCARD constexpr bool operator <(const child_type& rh) const
    noexcept(noexcept(self::to_template(this)->operator<(rh))) {
        return self::to_template(this)->operator <(rh);
    }
    MSTL_NODISCARD constexpr bool operator >(const child_type& rh) const
    noexcept(noexcept(self::to_template(this)->operator>(rh))) {
        return rh < *self::to_template(this);
    }
    MSTL_NODISCARD constexpr bool operator <=(const child_type& rh) const
    noexcept(noexcept(self::to_template(this)->operator<=(rh))) {
        return !(*this > rh);
    }
    MSTL_NODISCARD constexpr bool operator >=(const child_type& rh) const
    noexcept(noexcept(self::to_template(this)->operator>=(rh))) {
        return !(*this < rh);
    }
};

template <typename T, enable_if_t<is_base_of_v<icomparable<T>, T>, int> = 0>
MSTL_NODISCARD constexpr bool operator ==(const T& lh, const T& rh)
noexcept(noexcept(lh.operator==(rh))) {
    return lh.operator==(rh);
}
template <typename T, enable_if_t<is_base_of_v<icomparable<T>, T>, int> = 0>
MSTL_NODISCARD constexpr bool operator !=(const T& lh, const T& rh)
noexcept(noexcept(lh.operator!=(rh))) {
    return lh.operator!=(rh);
}
template <typename T, enable_if_t<is_base_of_v<icomparable<T>, T>, int> = 0>
MSTL_NODISCARD constexpr bool operator <(const T& lh, const T& rh)
noexcept(noexcept(lh.operator<(rh))) {
    return lh.operator<(rh);
}
template <typename T, enable_if_t<is_base_of_v<icomparable<T>, T>, int> = 0>
MSTL_NODISCARD constexpr bool operator >(const T& lh, const T& rh)
noexcept(noexcept(lh.operator>(rh))) {
    return lh.operator>(rh);
}
template <typename T, enable_if_t<is_base_of_v<icomparable<T>, T>, int> = 0>
MSTL_NODISCARD constexpr bool operator <=(const T& lh, const T& rh)
noexcept(noexcept(lh.operator<=(rh))) {
    return lh.operator<=(rh);
}
template <typename T, enable_if_t<is_base_of_v<icomparable<T>, T>, int> = 0>
MSTL_NODISCARD constexpr bool operator >=(const T& lh, const T& rh)
noexcept(noexcept(lh.operator>=(rh))) {
    return lh.operator>=(rh);
}


template <typename T>
struct icommon : icomparable<T>, iswappable<T>, ihashable<T> {};


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


template <
    typename Iterator, typename Category, typename ValueType, typename Reference = ValueType&,
    typename Pointer = ValueType*, typename DifferenceType = ptrdiff_t, typename SizeType = size_t
>
struct iiterator : icomparable<Iterator> {
private:
    using iterator_type     = Iterator;

    static constexpr const iterator_type* to_template(const iterator_type* o) noexcept {
        return static_cast<const iterator_type*>(o);
    }
    static constexpr iterator_type* to_template(iterator_type* o) noexcept {
        return static_cast<iterator_type*>(o);
    }

public:
    using iterator_category = Category;
    using value_type		= ValueType;
    using reference			= Reference;
    using pointer			= Pointer;
    using difference_type	= DifferenceType;
    using size_type			= SizeType;

    using self              = iiterator;

    MSTL_CONSTEXPR20 ~iiterator() = default;

    MSTL_NODISCARD constexpr reference operator *() const
    noexcept(noexcept(iterator_type::operator *())) {
        return iterator_type::operator *();
    }
    MSTL_NODISCARD constexpr pointer operator ->() const
    noexcept(noexcept(iterator_type::operator ->())) {
        return iterator_type::operator ->();
    }

    constexpr iterator_type& operator ++()
    noexcept(noexcept(self::to_template(this)->operator++())) {
        return self::to_template(this)->operator++();
    }
    constexpr iterator_type operator ++(int)
    noexcept(noexcept(self::to_template(this)->operator++(int()))) {
        return self::to_template(this)->operator++(int());
    }
    constexpr iterator_type& operator --()
    noexcept(noexcept(self::to_template(this)->operator--())) {
        return self::to_template(this)->operator--();
    }
    constexpr iterator_type operator --(int)
    noexcept(noexcept(self::to_template(this)->operator--(int()))) {
        return self::to_template(this)->operator--(int());
    }

    constexpr iterator_type& operator +=(difference_type n)
    noexcept(noexcept(self::to_template(this)->operator+=(n))) {
        return self::to_template(this)->operator+=(n);
    }
    MSTL_NODISCARD constexpr iterator_type operator +(difference_type n) const
    noexcept(noexcept(self::to_template(this)->operator+(n))) {
        return self::to_template(this)->operator+(n);
    }
    constexpr iterator_type& operator -=(difference_type n)
    noexcept(noexcept(self::to_template(this)->operator-=(n))) {
        return self::to_template(this)->operator-=(n);
    }
    MSTL_NODISCARD constexpr iterator_type operator -(difference_type n) const
    noexcept(noexcept(self::to_template(this)->operator-(n))) {
        return self::to_template(this)->operator-(n);
    }
    MSTL_NODISCARD constexpr difference_type operator -(const iterator_type& x) const
    noexcept(noexcept(self::to_template(this)->operator-(x))) {
        return self::to_template(this)->operator-(x);
    }

    MSTL_NODISCARD constexpr reference operator [](difference_type n)
    noexcept(noexcept(self::to_template(this)->operator[](n))) {
        return self::to_template(this)->operator[](n);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_INTERFACE_HPP__
