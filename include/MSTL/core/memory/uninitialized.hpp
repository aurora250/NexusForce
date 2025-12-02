#ifndef MSTL_CORE_MEMORY_UNINITIALIZED_HPP__
#define MSTL_CORE_MEMORY_UNINITIALIZED_HPP__
#include "../algorithm/shift.hpp"
#include "construct.hpp"
MSTL_BEGIN_NAMESPACE__


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _MSTL copy(first, last, result);
}

template <typename Iterator1, typename Iterator2, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    Iterator2 cur = result;
    try {
        for (; first != last; ++first, ++cur)
            _MSTL construct(&*cur, *first);
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        throw_exception(memory_exception("uninitialized copy failed."));
    }
    return cur;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 uninitialized_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
    if (first == last) return result;
    return _INNER __uninitialized_copy_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_copy_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator2 cur = result;
    try {
        for (; count > 0; --count, ++first, ++cur)
            _MSTL construct(&*cur, *first);
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        throw_exception(memory_exception("uninitialized copy n failed."));
    }
    return pair<Iterator1, Iterator2>(first, cur);
}

template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_copy_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator1 last = first + count;
    return _MSTL make_pair(last, _MSTL uninitialized_copy(first, last, result));
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> uninitialized_copy_n(
    Iterator1 first, size_t count, Iterator2 result) {
    return _INNER __uninitialized_copy_n_aux(first, count, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator, typename T, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 void __uninitialized_fill_aux(Iterator first, Iterator last, const T& x) {
    _MSTL fill(first, last, x);
}

template <typename Iterator, typename T, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 void __uninitialized_fill_aux(Iterator first, Iterator last, const T& x) {
    Iterator cur = first;
    try {
        for (; cur != last; ++cur)
            _MSTL construct(&*cur, x);
    }
    catch (...) {
        for (; cur != first; --cur)
            _MSTL destroy(&*cur);
        throw_exception(memory_exception("uninitialized fill failed."));
    }
}
MSTL_END_INNER__

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void uninitialized_fill(Iterator first, Iterator last, const T& x) {
    if (first == last) return;
    _INNER __uninitialized_fill_aux(first, last, x);
}


MSTL_BEGIN_INNER__
template <typename Iterator, typename T, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 Iterator __uninitialized_fill_n_aux(Iterator first, size_t n, const T& x) {
    return _MSTL fill_n(first, n, x);
}

template <typename Iterator, typename T, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 Iterator __uninitialized_fill_n_aux(Iterator first, size_t n, const T& x) {
    Iterator cur = first;
    try{
        for (; n > 0; --n, ++cur)
            _MSTL construct(&*cur, x);
    }
    catch (...) {
        for (; cur != first; --cur)
            _MSTL destroy(&*cur);
        throw_exception(memory_exception("uninitialized fill n failed."));
    }
    return cur;
}
MSTL_END_INNER__

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 Iterator uninitialized_fill_n(Iterator first, size_t n, const T& x) {
    return _INNER __uninitialized_fill_n_aux(first, n, x);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _MSTL move(first, last, result);
}

template <typename Iterator1, typename Iterator2, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    Iterator2 cur = result;
    try{
        for (; first != last; ++first, ++cur)
            _MSTL construct(&*cur, _MSTL move(*first));
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        throw_exception(memory_exception("uninitialized move failed."));
    }
    return cur;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 uninitialized_move(Iterator1 first, Iterator1 last, Iterator2 result) {
    if (first == last) return result;
    return _INNER __uninitialized_move_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_move_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator2 cur = result;
    try{
        for (; count > 0; --count, ++first, ++cur)
            _MSTL construct(&*cur, _MSTL move(*first));
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        throw_exception(memory_exception("uninitialized move n failed."));
    }
    return pair<Iterator1, Iterator2>(first, cur);
}

template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_move_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator1 last = first + count;
    return _MSTL make_pair(last, _MSTL uninitialized_move(first, last, result));
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> uninitialized_move_n(
    Iterator1 first, size_t count, Iterator2 result) {
    return _INNER __uninitialized_move_n_aux(first, count, result);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_UNINITIALIZED_HPP__
