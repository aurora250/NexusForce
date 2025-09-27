#ifndef MSTL_ITERATOR_HPP__
#define MSTL_ITERATOR_HPP__
#include "concepts.hpp"
#include "exception.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator>
MSTL_TRAITS_DEPRECATE MSTL_NODISCARD constexpr iter_cat_t<Iterator>
iterator_category(const Iterator&) noexcept {
    return iter_cat_t<Iterator>();
}

template <typename Iterator>
MSTL_TRAITS_DEPRECATE MSTL_NODISCARD constexpr iter_dif_t<Iterator>*
distance_type(const Iterator&) noexcept {
    return nullptr;
}

template <typename Iterator>
MSTL_TRAITS_DEPRECATE MSTL_NODISCARD constexpr
iter_val_t<Iterator>* value_type(const Iterator&) noexcept {
    return nullptr;
}


#ifndef MSTL_VERSION_17__
MSTL_BEGIN_INNER__
template <typename Ptr, enable_if_t<is_pointer_v<Ptr>, int> = 0>
constexpr iter_ptr_t<Ptr> __to_pointer_aux(Ptr iter) {
    return iter;
}
template <typename Iterator, enable_if_t<!is_pointer_v<Iterator>, int> = 0>
constexpr iter_ptr_t<Iterator> __to_pointer_aux(Iterator iter) {
    return iter.operator->();
}
MSTL_END_INNER__

template <typename Iterator>
constexpr iter_ptr_t<Iterator> to_pointer(Iterator iter) {
    return _INNER __to_pointer_aux(iter);
}
#else
template <typename Iterator>
constexpr iter_ptr_t<Iterator> to_pointer(Iterator tmp) {
    if constexpr (is_pointer_v<Iterator>)
        return tmp;
    else
        return tmp.operator->();
}
#endif // MSTL_VERSION_17__


#ifndef MSTL_VERSION_17__
MSTL_BEGIN_INNER__
template <typename Iterator, typename Distance, enable_if_t<is_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 void __advance_aux(Iterator& i, Distance n) {
    i += n;
}
template <typename Iterator, typename Distance, enable_if_t<!is_rnd_iter_v<Iterator> && is_ranges_bid_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 void __advance_aux(Iterator& i, Distance n) {
    for (; n < 0; ++n) --i;
    for (; 0 < n; --n) ++i;
}
template <typename Iterator, typename Distance, enable_if_t<!is_rnd_iter_v<Iterator> && !is_ranges_bid_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 void __advance_aux(Iterator& i, Distance n) {
    MSTL_DEBUG_VERIFY__(is_signed_v<Distance> && n >= 0, "negative advance of non-bidirectional iterator");
    for (; 0 < n; --n) ++i;
}
MSTL_END_INNER__

template <typename Iterator, typename Distance, enable_if_t<is_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 void advance(Iterator& i, Distance n) {
    _INNER __advance_aux(i, n);
}
#else
template <typename Iterator, typename Distance, enable_if_t<is_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 void advance(Iterator& i, Distance n) {
    if constexpr (is_rnd_iter_v<Iterator>) {
        i += n;
    }
    else {
        if constexpr (is_signed<Distance>::value && !is_bid_iter_v<Iterator>) {
            MSTL_DEBUG_VERIFY(n >= 0, "negative advance of non-bidirectional iterator");
        }
        if constexpr (is_signed<Distance>::value && is_bid_iter_v<Iterator>) {
            for (; n < 0; ++n)
                --i;
        }
        for (; 0 < n; --n)
            ++i;
    }
}
#endif // MSTL_VERSION_17__

template <typename Iterator>
MSTL_CONSTEXPR17 Iterator prev(Iterator iter, iter_dif_t<Iterator> n = -1) {
    MSTL_DEBUG_VERIFY(n <= 0, "negative advance in previous operation function.");
    _MSTL advance(iter, n);
    return iter;
}

template <typename Iterator>
MSTL_CONSTEXPR17 Iterator next(Iterator iter, iter_dif_t<Iterator> n = 1) {
    MSTL_DEBUG_VERIFY(n >= 0, "positive advance in next operation function.");
    _MSTL advance(iter, n);
    return iter;
}


#ifndef MSTL_VERSION_17__
MSTL_BEGIN_INNER__
template <typename Iterator, enable_if_t<is_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 iter_dif_t<Iterator> __distance_aux(Iterator first, Iterator last) {
    return last - first;
}
template <typename Iterator, enable_if_t<!is_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 iter_dif_t<Iterator> __distance_aux(Iterator first, Iterator last) {
    iter_dif_t<Iterator> n = 0;
    while (first != last) { ++first; ++n; }
    return n;
}
MSTL_END_INNER__

template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 iter_dif_t<Iterator> distance(Iterator first, Iterator last) {
    return _INNER __distance_aux(first, last);
}
#else
template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR17 iter_dif_t<Iterator> distance(Iterator first, Iterator last) {
    if constexpr (is_rnd_iter_v<Iterator>)
        return last - first;
    else {
        iter_dif_t<Iterator> n = 0;
        while (first != last) { ++first; ++n; }
        return n;
    }
}
#endif // MSTL_VERSION_17__


template <typename Container>
class back_insert_iterator {
public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;
    using self              = back_insert_iterator<Container>;

    constexpr explicit back_insert_iterator(Container& x) noexcept
        : container(_MSTL addressof(x)) {}
    constexpr self& operator =(const typename Container::value_type& value) {
        container->push_back(value);
        return *this;
    }
    constexpr self& operator =(typename Container::value_type&& value) {
        container->push_back(_MSTL move(value));
        return *this;
    }
    MSTL_NODISCARD constexpr self& operator *() noexcept { return *this; }
    constexpr self& operator ++() noexcept { return *this; }
    constexpr self& operator ++(int) noexcept { return *this; }

private:
    Container* container;
};
template <typename Container>
MSTL_NODISCARD constexpr back_insert_iterator<Container> back_inserter(Container& x) noexcept {
    return back_insert_iterator<Container>(x);
}

template <typename Container>
class front_insert_iterator {
public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;
    using self              = front_insert_iterator<Container>;

    constexpr explicit front_insert_iterator(Container& x) noexcept
        : container(_MSTL addressof(x)) {}
    constexpr self& operator =(const typename Container::value_type& value) {
        container->push_front(value);
        return *this;
    }
    constexpr self& operator =(typename Container::value_type&& value) {
        container->push_front(_MSTL move(value));
        return *this;
    }
    MSTL_NODISCARD constexpr self& operator *() noexcept { return *this; }
    constexpr self& operator ++() noexcept { return *this; }
    constexpr self& operator ++(int) noexcept { return *this; }

private:
    Container* container;
};

template <typename Container>
MSTL_NODISCARD constexpr front_insert_iterator<Container> front_inserter(Container& x) noexcept {
    return front_insert_iterator<Container>(x);
}

template <typename Container>
class insert_iterator {
public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = void;
    using pointer           = void;
    using reference         = void;
    using self              = insert_iterator<Container>;

    constexpr insert_iterator(Container& x, typename Container::iterator it) noexcept
        : container(_MSTL addressof(x)), iter(_MSTL move(it)) {}
    constexpr self& operator =(const typename Container::value_type& value) {
        iter = container->insert(iter, value);
        ++iter;
        return *this;
    }
    constexpr self& operator =(typename Container::value_type&& value) {
        iter = container->insert(iter, _MSTL move(value));
        ++iter;
        return *this;
    }
    MSTL_NODISCARD constexpr self& operator *() noexcept { return *this; }
    constexpr self& operator ++() noexcept { return *this; }
    constexpr self& operator ++(int) noexcept { return *this; }

private:
    Container* container;
    typename Container::iterator iter;
};

template <typename Container>
MSTL_NODISCARD constexpr insert_iterator<Container> 
inserter(Container& x, typename Container::iterator it) noexcept {
    return insert_iterator<Container>(x, it);
}


template <typename Iterator>
class reverse_iterator {
    static_assert(is_bid_iter_v<Iterator>, "reverse iterator requires bidirectional iterator.");

public:
    using iterator_category = iter_cat_t<Iterator>;
    using value_type        = iter_val_t<Iterator>;
    using difference_type   = iter_dif_t<Iterator>;
    using pointer           = iter_ptr_t<Iterator>;
    using reference         = iter_ref_t<Iterator>;
    using self              = reverse_iterator<Iterator>;

private:
    Iterator current;

public:
    constexpr reverse_iterator() = default;

    constexpr explicit reverse_iterator(Iterator x)
        noexcept(is_nothrow_move_constructible_v<Iterator>)
        : current(_MSTL move(x)) {}

    template <typename U>
#ifdef MSTL_VERSION_20__
        requires(!same_as<U, Iterator> && convertible_to<const U&, Iterator>)
#endif // MSTL_VERSION_20__
    constexpr explicit reverse_iterator(const reverse_iterator<U>& x)
        noexcept(is_nothrow_constructible_v<Iterator, const U&>)
        : current(x.current) {}

    template <typename U>
#ifdef MSTL_VERSION_20__
        requires(!same_as<U, Iterator> && convertible_to<const U&, Iterator> 
    && assignable_from<Iterator&, const U&>)
#endif // MSTL_VERSION_20__
    constexpr self& operator =(const reverse_iterator<U>& x)
        noexcept(is_nothrow_assignable_v<self&, const U&>) {
        current = x.current;
        return *this;
    }

    MSTL_NODISCARD constexpr Iterator base() const
        noexcept(is_nothrow_copy_constructible_v<Iterator>) {
        return current;
    }

    MSTL_NODISCARD constexpr reference operator *() const
        noexcept(is_nothrow_copy_assignable_v<Iterator> && noexcept(*--(_MSTL declval<Iterator&>()))) {
        Iterator iter = current;
        return *--iter;
    }

    MSTL_NODISCARD constexpr pointer operator ->() const
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(--(_MSTL declval<Iterator&>()))
            && is_nothrow_arrow<Iterator&, pointer>)
#ifdef MSTL_VERSION_20__
        requires (is_pointer_v<Iterator> || requires(const Iterator it) { it.operator->(); })
#endif // MSTL_VERSION_20__
    {
        Iterator tmp = current;
        --tmp;
        return _MSTL to_pointer(tmp);
    }

    constexpr self& operator ++()
         noexcept(noexcept(--current)) {
        --current;
        return *this;
    }

    constexpr self operator ++(int)
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(--current)) {
        self tmp = *this;
        --current;
        return tmp;
    }

    constexpr self& operator --()
        noexcept(noexcept(++current)) {
        ++current;
        return *this;
    }

    constexpr self operator --(int)
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(++current)) {
        self tmp = *this;
        ++current;
        return tmp;
    }

    constexpr self operator +(const difference_type n) const 
        noexcept(noexcept(self(current - n))) {
        return self(current - n);
    }
    constexpr self& operator +=(const difference_type n) 
        noexcept(noexcept(current -= n)) {
        current -= n;
        return *this;
    }
    constexpr self operator -(const difference_type n) const 
        noexcept(noexcept(self(current + n))) {
        return self(current + n);
    }
    constexpr self& operator -=(const difference_type n)
        noexcept(noexcept(current += n)) {
        current += n;
        return *this;
    }

    constexpr reference operator[](const difference_type n) const
        noexcept(noexcept(_MSTL declcopy<reference>(self(current - n)))) {
        return *(*this + n);
    }

    MSTL_NODISCARD constexpr const Iterator& get_current() const noexcept {
        return current;
    }
};
template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator ==(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.get_current() == y.get_current())))
#ifdef MSTL_VERSION_20__
    requires requires { { x.get_current() == y.get_current() } -> convertible_to<bool>; }
#endif // MSTL_VERSION_20__
{
    return x.get_current() == y.get_current();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator !=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.get_current() != y.get_current())))
#ifdef MSTL_VERSION_20__
    requires requires { { x.get_current() != y.get_current() } -> convertible_to<bool>; }
#endif // MSTL_VERSION_20__
{
    return x.get_current() != y.get_current();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator <(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.get_current() > y.get_current())))
#ifdef MSTL_VERSION_20__
    requires requires { { x.get_current() > y.get_current() } -> convertible_to<bool>; }
#endif
{
    return x.get_current() > y.get_current();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator >(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.get_current() < y.get_current())))
#ifdef MSTL_VERSION_20__
    requires requires { { x.get_current() < y.get_current() } -> convertible_to<bool>; }
#endif // MSTL_VERSION_20__
{
    return x.get_current() < y.get_current();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator <=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.get_current() >= y.get_current())))
#ifdef MSTL_VERSION_20__
    requires requires { { x.get_current() >= y.get_current() } -> convertible_to<bool>; }
#endif // MSTL_VERSION_20__
{
    return x.get_current() >= y.get_current();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool operator >=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_MSTL declcopy<bool>(x.get_current() <= y.get_current())))
#ifdef MSTL_VERSION_20__
    requires requires { { x.get_current() <= y.get_current() } -> convertible_to<bool>; }
#endif // MSTL_VERSION_20__
{
    return x.get_current() <= y.get_current();
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr decltype(auto) operator -(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(y.get_current() - x.get_current())) {
    return y.get_current() - x.get_current();
}

template <typename Iterator>
constexpr reverse_iterator<Iterator> operator +(
    iter_dif_t<Iterator> n, const reverse_iterator<Iterator>& x)
    noexcept(noexcept(x + n)) {
    return x + n;
}

template <typename Iterator>
MSTL_NODISCARD constexpr reverse_iterator<Iterator> 
make_reverse_iterator(Iterator it) noexcept(is_nothrow_move_constructible_v<Iterator>) {
    return reverse_iterator<Iterator>(_MSTL move(it));
}

MSTL_END_NAMESPACE__
#endif // MSTL_ITERATOR_HPP__
