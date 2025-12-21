#ifndef MSTL_CORE_ITERATOR_REVERSE_ITERATOR_HPP
#define MSTL_CORE_ITERATOR_REVERSE_ITERATOR_HPP
#include "../algorithm/iterator.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator>
class reverse_iterator {
    static_assert(is_bid_iter_v<Iterator>, "reverse iterator requires bidirectional iterator.");

public:
    using iterator_category = iter_category_t<Iterator>;
    using value_type        = iter_value_t<Iterator>;
    using difference_type   = iter_difference_t<Iterator>;
    using pointer           = iter_pointer_t<Iterator>;
    using reference         = iter_reference_t<Iterator>;

private:
    Iterator current;

public:
    constexpr reverse_iterator() = default;

    constexpr explicit reverse_iterator(Iterator x)
    noexcept(is_nothrow_move_constructible_v<Iterator>)
    : current(_MSTL move(x)) {}

    template <typename U>
#ifdef MSTL_STANDARD_20__
        requires(!same_as<U, Iterator> && convertible_to<const U&, Iterator>)
#endif // MSTL_STANDARD_20__
    constexpr explicit reverse_iterator(const reverse_iterator<U>& x)
        noexcept(is_nothrow_constructible_v<Iterator, const U&>)
        : current(x.current) {}

    template <typename U>
#ifdef MSTL_STANDARD_20__
        requires(!same_as<U, Iterator> && convertible_to<const U&, Iterator>
    && assignable_from<Iterator&, const U&>)
#endif // MSTL_STANDARD_20__
    constexpr reverse_iterator& operator =(const reverse_iterator<U>& x)
        noexcept(is_nothrow_assignable_v<reverse_iterator&, const U&>) {
        current = x.current;
        return *this;
    }

    MSTL_CONSTEXPR20 ~reverse_iterator() noexcept = default;

    MSTL_NODISCARD constexpr reference operator *() const
        noexcept(is_nothrow_copy_assignable_v<Iterator> && noexcept(*--(_MSTL declval<Iterator&>()))) {
        Iterator iter = current;
        return *--iter;
    }

    MSTL_NODISCARD constexpr pointer operator ->() const
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(--(_MSTL declval<Iterator&>()))
            && is_nothrow_arrow<Iterator&, pointer>)
#ifdef MSTL_STANDARD_20__
        requires (is_pointer_v<Iterator> || requires(const Iterator it) { it.operator->(); })
#endif // MSTL_STANDARD_20__
    {
        Iterator tmp = current;
        --tmp;
        return _MSTL to_pointer(tmp);
    }

    constexpr reverse_iterator& operator ++()
         noexcept(noexcept(--current)) {
        --current;
        return *this;
    }

    constexpr reverse_iterator operator ++(int)
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(--current)) {
        reverse_iterator tmp = *this;
        --current;
        return tmp;
    }

    constexpr reverse_iterator& operator --()
        noexcept(noexcept(++current)) {
        ++current;
        return *this;
    }

    constexpr reverse_iterator operator --(int)
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(++current)) {
        reverse_iterator tmp = *this;
        ++current;
        return tmp;
    }

    constexpr reverse_iterator operator +(const difference_type n) const
        noexcept(noexcept(reverse_iterator(current - n))) {
        return reverse_iterator(current - n);
    }
    constexpr reverse_iterator& operator +=(const difference_type n)
        noexcept(noexcept(current -= n)) {
        current -= n;
        return *this;
    }
    constexpr reverse_iterator operator -(const difference_type n) const
        noexcept(noexcept(reverse_iterator(current + n))) {
        return reverse_iterator(current + n);
    }
    constexpr reverse_iterator& operator -=(const difference_type n)
        noexcept(noexcept(current += n)) {
        current += n;
        return *this;
    }

    constexpr reference operator [](const difference_type n) const
        noexcept(noexcept(_MSTL declcopy<reference>(reverse_iterator(current - n)))) {
        return *(*this + n);
    }

    
    MSTL_NODISCARD constexpr const Iterator& base() const noexcept {
        return current;
    }
};
template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator ==(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.base() == y.base())))
#ifdef MSTL_STANDARD_20__
    requires requires { { x.base() == y.base() } -> convertible_to<bool>; }
#endif // MSTL_STANDARD_20__
{
    return x.base() == y.base();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator !=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.base() != y.base())))
#ifdef MSTL_STANDARD_20__
    requires requires { { x.base() != y.base() } -> convertible_to<bool>; }
#endif // MSTL_STANDARD_20__
{
    return x.base() != y.base();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator <(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.base() > y.base())))
#ifdef MSTL_STANDARD_20__
    requires requires { { x.base() > y.base() } -> convertible_to<bool>; }
#endif
{
    return x.base() > y.base();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator >(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.base() < y.base())))
#ifdef MSTL_STANDARD_20__
    requires requires { { x.base() < y.base() } -> convertible_to<bool>; }
#endif // MSTL_STANDARD_20__
{
    return x.base() < y.base();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator <=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.base() >= y.base())))
#ifdef MSTL_STANDARD_20__
    requires requires { { x.base() >= y.base() } -> convertible_to<bool>; }
#endif // MSTL_STANDARD_20__
{
    return x.base() >= y.base();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator >=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.base() <= y.base())))
#ifdef MSTL_STANDARD_20__
    requires requires { { x.base() <= y.base() } -> convertible_to<bool>; }
#endif // MSTL_STANDARD_20__
{
    return x.base() <= y.base();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr decltype(auto) operator -(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(y.base() - x.base())) {
    return y.base() - x.base();
}

template <typename Iterator>
constexpr reverse_iterator<Iterator> operator +(
    iter_difference_t<Iterator> n, const reverse_iterator<Iterator>& x)
    noexcept(noexcept(x + n)) {
    return x + n;
}

template <typename Iterator>
MSTL_NODISCARD constexpr reverse_iterator<Iterator>
make_reverse_iterator(Iterator it) noexcept(is_nothrow_move_constructible_v<Iterator>) {
    return reverse_iterator<Iterator>(_MSTL move(it));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ITERATOR_REVERSE_ITERATOR_HPP
