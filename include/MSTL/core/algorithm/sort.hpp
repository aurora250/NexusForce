#ifndef MSTL_CORE_ALGORITHM_SORT_HPP__
#define MSTL_CORE_ALGORITHM_SORT_HPP__
#include "merge.hpp"
#include "heap.hpp"
#include "partition.hpp"
#include "compare.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename Compare, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
bool is_sorted(Iterator first, Iterator last, Compare comp) {
	if (first == last) return true;
	Iterator next = _MSTL next(first);
	for (; next != last; ++first, ++next) {
		if (comp(*next, *first)) {
			return false;
		}
	}
	return true;
}

template <typename Iterator>
bool is_sorted(Iterator first, Iterator last) {
	return is_sorted(first, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
Iterator is_sorted_until(Iterator first, Iterator last, Compare comp) {
	if (first == last) return last;
	Iterator next = _MSTL next(first);
	for (; next != last; ++first, ++next) {
		if (comp(*next, *first))
			return next;
	}
	return last;
}

template <typename Iterator>
Iterator is_sorted_until(Iterator first, Iterator last) {
	return is_sorted_until(first, last, _MSTL less<iter_value_t<Iterator>>());
}


// merge sort : Ot(NlogN) Om(logN) stable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void merge_sort(Iterator first, Iterator last, Compare comp) {
    using Distance = typename iterator_traits<Iterator>::difference_type;
    Distance n = _MSTL distance(first, last);
    if (n < 2) return;
    Iterator mid = first + n / 2;
    _MSTL merge_sort(first, mid);
    _MSTL merge_sort(mid, last);
    _MSTL inplace_merge(first, mid, last, comp);
}

template <typename Iterator>
void merge_sort(Iterator first, Iterator last) {
    return _MSTL merge_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

// partial sort / heap sort : Ot(NlogN) Om(1) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void partial_sort(Iterator first, Iterator middle, Iterator last, Compare comp) {
    if (first == middle) return;
    _MSTL make_heap(first, middle, comp);
    for (Iterator i = middle; i < last; ++i) {
        if (comp(*i, *first)) {
	        _MSTL pop_heap_aux(first, middle, i, *i, comp);
        }
    }
    _MSTL sort_heap(first, middle, comp);
}

template <typename Iterator>
void partial_sort(Iterator first, Iterator middle, Iterator last) {
    return _MSTL partial_sort(first, middle, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename Iterator1, typename Iterator2, typename Compare, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_rnd_iter_v<Iterator2>, int> = 0>
Iterator2 partial_sort_copy(Iterator1 first, Iterator1 last,
	Iterator2 result_first, Iterator2 result_last, Compare comp) {
	if (result_first == result_last) return result_last;
    using Distance = typename iterator_traits<Iterator1>::difference_type;
	Iterator2 result_real_last = result_first;
	while (first != last && result_real_last != result_last) {
		*result_real_last = *first;
		++result_real_last;
		++first;
	}
    _MSTL make_heap(result_first, result_real_last, comp);
	while (first != last) {
		if (comp(*first, *result_first)) {
            _MSTL adjust_heap(result_first, Distance(0),
            	Distance(result_real_last - result_first), *first, comp);
        }
		++first;
	}
    _MSTL sort_heap(result_first, result_real_last, comp);
	return result_real_last;
}

template <typename Iterator1, typename Iterator2>
Iterator2 partial_sort_copy(
    Iterator1 first, Iterator1 last, Iterator2 result_first, Iterator2 result_last) {
    return _MSTL partial_sort_copy(first, result_first, result_last, _MSTL less<iter_value_t<Iterator1>>());
}

MSTL_BEGIN_INNER__
template <typename Iterator, typename T, typename Compare>
void __insertion_sort_aux(Iterator last, T value, Compare comp) {
    Iterator next = last;
    --next;
    while (comp(value, *next)) {
        *last = *next;
        last = next;
        --next;
    }
    *last = value;
}
MSTL_END_INNER__

// insertion sort : Ot(N)~(N^2) Om(1) stable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void insertion_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    using T = typename iterator_traits<Iterator>::value_type;
    for (Iterator i = first + 1; i != last; ++i) {
        T value = *i;
        if (comp(value, *first)) {
            _MSTL copy_backward(first, i, i + 1);
            *first = value;
        } else {
	        _INNER __insertion_sort_aux(i, value, comp);
        }
    }
}

template <typename Iterator>
void insertion_sort(Iterator first, Iterator last) {
    return _MSTL insertion_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}


static constexpr size_t SORT_DISPATCH_THRESHOLD = 16;

// introspective sort : Ot(NlogN) Om(logN) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void introspective_sort(Iterator first, Iterator last, int depth_limit, Compare comp) {
    while (first < last) {
        if (depth_limit == 0) {
            _MSTL partial_sort(first, last, last, comp);
            return;
        }
        --depth_limit;
        Iterator cut = _MSTL lomuto_partition(
            first, last, _MSTL median(*first, *(first + (last - first) / 2), *(last - 1), comp), comp);
        _MSTL introspective_sort(cut, last, depth_limit, comp);
        last = cut;
    }
}

template <typename Iterator>
void introspective_sort(Iterator first, Iterator last, int depth_limit) {
    return _MSTL introspective_sort(first, last, depth_limit, _MSTL less<iter_value_t<Iterator>>());
}

// quick sort : Ot(NlogN) Om(1) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void quick_sort(Iterator first, Iterator last, Compare comp) {
    if (first < last) {
        Iterator pov = last - 1;
        Iterator cut = _MSTL lomuto_partition(first, last, *pov, comp);
        _MSTL iter_swap(cut, pov);
        _MSTL quick_sort(first, cut, comp);
        _MSTL quick_sort(cut + 1, last, comp);
    }
}

template <typename Iterator>
void quick_sort(Iterator first, Iterator last) {
    return _MSTL quick_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}


MSTL_BEGIN_INNER__
template <typename Iterator, typename Compare>
void __intro_sort_dispatch(Iterator first, Iterator last, int depth_limit, Compare comp) {
    while (last - first > SORT_DISPATCH_THRESHOLD) {
        if (depth_limit == 0) {
            _MSTL partial_sort(first, last, last, comp);
            return;
        }
        --depth_limit;
        Iterator cut = _MSTL lomuto_partition(
            first, last, _MSTL median(*first, *(first + (last - first) / 2), *(last - 1), comp), comp);
        _INNER __intro_sort_dispatch(cut, last, depth_limit, comp);
        last = cut;
    }
}
MSTL_END_INNER__

// standard sort : Ot(NlogN) Om(logN) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    _INNER __intro_sort_dispatch(first, last, static_cast<int>(_MSTL logarithm_2_integer(last - first)) * 2, comp);
    if (last - first > SORT_DISPATCH_THRESHOLD) {
        _MSTL insertion_sort(first, first + SORT_DISPATCH_THRESHOLD, comp);
        for (Iterator i = first + SORT_DISPATCH_THRESHOLD; i != last; ++i) {
	        _INNER __insertion_sort_aux(i, *i, comp);
        }
    }
    else {
	    _MSTL insertion_sort(first, last, comp);
    }
}

template <typename Iterator>
void sort(Iterator first, Iterator last) {
    return _MSTL sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}


// nth element : Ot(N)~(N^2) Om(1) unstable
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void nth_element(Iterator first, Iterator nth, Iterator last, Compare comp) {
    while (last - first > 3) {
        Iterator cut = _MSTL lomuto_partition(
            first, last, _MSTL median(*first, *(first + (last - first) / 2), *(last - 1), comp), comp);
        if (cut <= nth) first = cut;
        else last = cut;
    }
    _MSTL insertion_sort(first, last, comp);
}

template <typename Iterator>
void nth_element(Iterator first, Iterator nth, Iterator last) {
    return _MSTL nth_element(first, nth, last, _MSTL less<iter_value_t<Iterator>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_SORT_HPP__
