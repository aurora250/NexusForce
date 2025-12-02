#ifndef MSTL_CORE_ALGORITHM_ITERATOR_HPP__
#define MSTL_CORE_ALGORITHM_ITERATOR_HPP__
#include "../iterator/iterator_traits.hpp"
#include "../typeinfo/concepts.hpp"
#include "../exception/assertion.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator>
MSTL_TRAITS_DEPRECATE MSTL_NODISCARD constexpr iter_category_t<Iterator>
iterator_category(const Iterator&) noexcept {
    return iter_category_t<Iterator>();
}

template <typename Iterator>
MSTL_TRAITS_DEPRECATE MSTL_NODISCARD constexpr iter_difference_t<Iterator>*
distance_type(const Iterator&) noexcept {
    return nullptr;
}

template <typename Iterator>
MSTL_TRAITS_DEPRECATE MSTL_NODISCARD constexpr
iter_value_t<Iterator>* value_type(const Iterator&) noexcept {
    return nullptr;
}


#ifndef MSTL_STANDARD_17__
MSTL_BEGIN_INNER__
template <typename Ptr, enable_if_t<is_pointer_v<Ptr>, int> = 0>
constexpr iter_pointer_t<Ptr> __to_pointer_aux(Ptr iter) {
    return iter;
}
template <typename Iterator, enable_if_t<!is_pointer_v<Iterator>, int> = 0>
constexpr iter_pointer_t<Iterator> __to_pointer_aux(Iterator iter) {
    return iter.operator->();
}
MSTL_END_INNER__

template <typename Iterator>
constexpr iter_pointer_t<Iterator> to_pointer(Iterator iter) {
    return _INNER __to_pointer_aux(iter);
}
#else
template <typename Iterator>
constexpr iter_pointer_t<Iterator> to_pointer(Iterator tmp) {
    if constexpr (is_pointer_v<Iterator>)
        return tmp;
    else
        return tmp.operator->();
}
#endif // MSTL_STANDARD_17__


#ifndef MSTL_STANDARD_17__
MSTL_BEGIN_INNER__
template <typename Iterator, typename Distance, enable_if_t<is_rnd_iter_v<Iterator>, int> = 0>
constexpr void __advance_aux(Iterator& i, Distance n) {
    i += n;
}
template <typename Iterator, typename Distance, enable_if_t<!is_rnd_iter_v<Iterator> && is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr void __advance_aux(Iterator& i, Distance n) {
    for (; n < 0; ++n) --i;
    for (; 0 < n; --n) ++i;
}
template <typename Iterator, typename Distance, enable_if_t<!is_rnd_iter_v<Iterator> && !is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr void __advance_aux(Iterator& i, Distance n) {
    MSTL_DEBUG_VERIFY__(is_signed_v<Distance> && n >= 0, "negative advance of non-bidirectional iterator");
    for (; 0 < n; --n) ++i;
}
MSTL_END_INNER__

template <typename Iterator, typename Distance, enable_if_t<is_iter_v<Iterator>, int> = 0>
constexpr void advance(Iterator& i, Distance n) {
    _INNER __advance_aux(i, n);
}
#else
template <typename Iterator, typename Distance, enable_if_t<is_iter_v<Iterator>, int> = 0>
constexpr void advance(Iterator& i, Distance n) {
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
#endif // MSTL_STANDARD_17__

template <typename Iterator>
constexpr Iterator prev(Iterator iter, iter_difference_t<Iterator> n = -1) {
    MSTL_DEBUG_VERIFY(n <= 0, "negative advance in previous operation function.");
    _MSTL advance(iter, n);
    return iter;
}

template <typename Iterator>
constexpr Iterator next(Iterator iter, iter_difference_t<Iterator> n = 1) {
    MSTL_DEBUG_VERIFY(n >= 0, "positive advance in next operation function.");
    _MSTL advance(iter, n);
    return iter;
}


#ifndef MSTL_STANDARD_17__
MSTL_BEGIN_INNER__
template <typename Iterator, enable_if_t<is_rnd_iter_v<Iterator>, int> = 0>
constexpr iter_difference_t<Iterator> __distance_aux(Iterator first, Iterator last) {
    return last - first;
}
template <typename Iterator, enable_if_t<!is_rnd_iter_v<Iterator>, int> = 0>
constexpr iter_difference_t<Iterator> __distance_aux(Iterator first, Iterator last) {
    iter_difference_t<Iterator> n = 0;
    while (first != last) { ++first; ++n; }
    return n;
}
MSTL_END_INNER__

template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
constexpr iter_difference_t<Iterator> distance(Iterator first, Iterator last) {
    return _INNER __distance_aux(first, last);
}
#else
template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
constexpr iter_difference_t<Iterator> distance(Iterator first, Iterator last) {
    if constexpr (is_rnd_iter_v<Iterator>)
        return last - first;
    else {
        iter_difference_t<Iterator> n = 0;
        while (first != last) { ++first; ++n; }
        return n;
    }
}
#endif // MSTL_STANDARD_17__

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_ITERATOR_HPP__
