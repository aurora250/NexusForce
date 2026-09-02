#ifndef NEFORCE_CORE_ITERATOR_MOVE_ITERATOR_HPP__
#define NEFORCE_CORE_ITERATOR_MOVE_ITERATOR_HPP__
#include "NeForce/core/iterator/iterator_traits.hpp"
#include "NeForce/core/typeinfo/concepts.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename Iterator>
class move_iterator {
public:
    using iterator_category = iter_category_t<Iterator>;
    using value_type = iter_value_t<Iterator>;
    using difference_type = iter_difference_t<Iterator>;
    using pointer = Iterator;
    using reference = conditional_t<is_reference_v<iter_reference_t<Iterator>>,
                                    remove_reference_t<iter_reference_t<Iterator>>&&, iter_reference_t<Iterator>>;

private:
    template <typename Iterator2>
    static constexpr bool convertible_with =
            !is_same_v<Iterator2, Iterator> && is_convertible_v<const Iterator2&, Iterator>;

    Iterator current_{};

    template <typename Iterator2>
    friend class move_iterator;

public:
    constexpr move_iterator() noexcept(is_nothrow_default_constructible_v<Iterator>) = default;

    explicit constexpr move_iterator(Iterator other) noexcept(is_nothrow_move_constructible_v<Iterator>) :
    current_(_NEFORCE move(other)) {}

    template <typename U>
#if NEFORCE_STANDARD_20
        requires convertible_with<U>
#endif
    constexpr move_iterator(const move_iterator<U>& other) :
    current_(other.current_) {
    }

    template <typename U>
#if NEFORCE_STANDARD_20
        requires convertible_with<U> && assignable_from<Iterator&, const U&>
#endif
    constexpr move_iterator& operator=(const move_iterator<U>& other) {
        current_ = other.current_;
        return *this;
    }

    NEFORCE_NODISCARD constexpr const Iterator& base() const& noexcept { return current_; }

    NEFORCE_NODISCARD constexpr Iterator base() && noexcept(is_nothrow_move_constructible_v<Iterator>) {
        return _NEFORCE move(current_);
    }

    NEFORCE_NODISCARD constexpr reference operator*() const noexcept(is_nothrow_copy_constructible_v<reference>) {
        return static_cast<reference>(*current_);
    }

    NEFORCE_NODISCARD constexpr pointer operator->() const noexcept(is_nothrow_copy_constructible_v<pointer>) {
        return current_;
    }

    constexpr move_iterator& operator++() noexcept(noexcept(++current_)) {
        ++current_;
        return *this;
    }

    constexpr move_iterator operator++(int) noexcept(is_nothrow_copy_constructible_v<Iterator> &&
                                                     noexcept(++current_)) {
        move_iterator tmp = *this;
        ++current_;
        return tmp;
    }

    constexpr move_iterator& operator--() noexcept(noexcept(--current_)) {
        --current_;
        return *this;
    }

    constexpr move_iterator operator--(int) noexcept(is_nothrow_copy_constructible_v<Iterator> &&
                                                     noexcept(++current_)) {
        move_iterator tmp = *this;
        --current_;
        return tmp;
    }

    NEFORCE_NODISCARD constexpr move_iterator operator+(difference_type n) const { return move_iterator(current_ + n); }

    constexpr move_iterator& operator+=(difference_type n) {
        current_ += n;
        return *this;
    }

    NEFORCE_NODISCARD constexpr move_iterator operator-(difference_type n) const { return move_iterator(current_ - n); }

    constexpr move_iterator& operator-=(difference_type n) {
        current_ -= n;
        return *this;
    }

    NEFORCE_NODISCARD constexpr reference operator[](difference_type n) const
            noexcept(is_nothrow_move_constructible_v<reference>) {
        return _NEFORCE move(current_[n]);
    }
};


template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool
operator==(const move_iterator<Iterator1>& x,
           const move_iterator<Iterator2>& y) noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() == y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires {
        { x.base() == y.base() } -> convertible_to<bool>;
    }
#endif
{
    return x.base() == y.base();
}

template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool
operator!=(const move_iterator<Iterator1>& x,
           const move_iterator<Iterator2>& y) noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() != y.base()))) {
    return x != y;
}

template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool
operator<(const move_iterator<Iterator1>& x,
          const move_iterator<Iterator2>& y) noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() < y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires {
        { x.base() < y.base() } -> convertible_to<bool>;
    }
#endif
{
    return x.base() < y.base();
}

template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool
operator<=(const move_iterator<Iterator1>& x,
           const move_iterator<Iterator2>& y) noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() <= y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires {
        { y.base() < x.base() } -> convertible_to<bool>;
    }
#endif
{
    return !(y < x);
}

template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool
operator>(const move_iterator<Iterator1>& x,
          const move_iterator<Iterator2>& y) noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() > y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires {
        { y.base() < x.base() } -> convertible_to<bool>;
    }
#endif
{
    return y < x;
}

template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool
operator>=(const move_iterator<Iterator1>& x,
           const move_iterator<Iterator2>& y) noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() >= y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires {
        { x.base() < y.base() } -> convertible_to<bool>;
    }
#endif
{
    return !(x < y);
}

template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr decltype(auto)
operator-(const move_iterator<Iterator1>& x,
          const move_iterator<Iterator2>& y) noexcept(noexcept(y.base() - x.base())) {
    return x.base() - y.base();
}

template <typename Iterator>
NEFORCE_NODISCARD constexpr move_iterator<Iterator>
operator+(iter_difference_t<move_iterator<Iterator>> n, const move_iterator<Iterator>& x) noexcept(noexcept(x + n)) {
    return x + n;
}

template <typename Iterator>
NEFORCE_NODISCARD constexpr move_iterator<Iterator>
make_move_iterator(Iterator i) noexcept(is_nothrow_move_constructible_v<Iterator>) {
    return move_iterator<Iterator>(_NEFORCE move(i));
}


NEFORCE_END_NAMESPACE__
#endif //NEFORCE_CORE_ITERATOR_MOVE_ITERATOR_HPP__
