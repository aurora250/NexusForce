#ifndef MSTL_CORE_ALGORITHM_SEARCH_HPP__
#define MSTL_CORE_ALGORITHM_SEARCH_HPP__
#include "../iterator/reverse_iterator.hpp"
#include "bound.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename Predicate, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr bool all_of(Iterator first, Iterator last, Predicate pred) {
	for (; first != last; ++first) {
		if (!pred(*first)) return false;
	}
	return true;
}

template <typename Iterator, typename Predicate, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr bool any_of(Iterator first, Iterator last, Predicate pred) {
	for (; first != last; ++first) {
		if (pred(*first)) return true;
	}
	return false;
}

template <typename Iterator, typename Predicate, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr bool none_of(Iterator first, Iterator last, Predicate pred) {
	for (; first != last; ++first) {
		if (pred(*first)) return false;
	}
	return true;
}

template <typename Iterator, typename BinaryPredicate, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator adjacent_find(Iterator first, Iterator last, BinaryPredicate binary_pred) {
	if (first == last) return last;
	Iterator next = first;
	while (++next != last) {
		if (binary_pred(*first, *next)) return first;
		first = next;
	}
	return last;
}

template <typename Iterator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator adjacent_find(Iterator first, Iterator last) {
	return _MSTL adjacent_find(first, last, _MSTL equal_to<iter_value_t<Iterator>>());
}

template <typename Iterator, typename T, typename BinaryPredicate,
	enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr iter_difference_t<Iterator> count_if(Iterator first, Iterator last, const T& value, BinaryPredicate pred) {
	iter_difference_t<Iterator> n = 0;
	for (; first != last; ++first) {
		if (pred(*first, value)) ++n;
	}
	return n;
}

template <typename Iterator, typename T>
constexpr iter_difference_t<Iterator> count(Iterator first, Iterator last, const T& value) {
	return _MSTL count_if(first, last, value, _MSTL equal_to<iter_value_t<Iterator>>());
}

template <typename Iterator, typename T>
MSTL_NODISCARD constexpr Iterator find(Iterator first, Iterator last, const T& value) {
	while (first != last && *first != value) ++first;
	return first;
}

template <typename Iterator, typename Predicate,
	enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator find_if(Iterator first, Iterator last, Predicate pred) {
	while (first != last && !pred(*first)) ++first;
	return first;
}

template <typename Iterator, typename Predicate,
	enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator find_if_not(Iterator first, Iterator last, Predicate pred) {
	while (first != last && pred(*first)) ++first;
	return first;
}

template <typename Iterator1, typename Iterator2, typename BinaryPredicate,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator1 search(Iterator1 first1, Iterator1 last1, Iterator2 first2,
	Iterator2 last2, BinaryPredicate binary_pred) {
	iter_difference_t<Iterator1> d1 = _MSTL distance(first1, last1);
	iter_difference_t<Iterator2> d2 = _MSTL distance(first2, last2);
	if (d1 < d2) return last1;

	Iterator1 current1 = first1;
	Iterator2 current2 = first2;

	while (current2 != last2) {
		if (binary_pred(*current1, *current2)) {
			++current1;
			++current2;
		} else {
			if (d1 == d2) return last1;

            current1 = ++first1;
            current2 = first2;
            --d1;
        }
	}
	return first1;
}

template <typename Iterator1, typename Iterator2>
constexpr Iterator1 search(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _MSTL search(first1, last1, first2, last2, _MSTL equal_to<iter_value_t<Iterator1>>());
}

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator search_n(Iterator first, Iterator last, const size_t count, const T& value) {
	first = _MSTL find(first, last, value);
	while (first != last) {
		size_t n = count - 1;
		Iterator i = first;
		++i;
		while (i != last && n != 0 && *i == value) {
			++i;
			--n;
		}
		if (n == 0) return first;

        first = _MSTL find(i, last, value);
    }
	return last;
}

template <typename Iterator, typename T, typename BinaryPredicate, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator search_n(Iterator first, Iterator last, const size_t count, const T& value, BinaryPredicate binary_pred) {
	while (first != last) {
		if (binary_pred(*first, value)) break;
		++first;
	}
	while (first != last) {
		size_t n = count - 1;
		Iterator i = first;
		++i;
		while (i != last && n != 0 && binary_pred(*i, value)) {
			++i;
			--n;
		}
		if (n == 0) return first;

        while (i != last) {
            if (binary_pred(*i, value)) break;
            ++i;
        }
        first = i;
    }
	return last;
}

#ifndef MSTL_STANDARD_17__
MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2,
	enable_if_t<is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>, int> = 0>
constexpr Iterator1 __find_end_aux(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	using reviter1 = _MSTL reverse_iterator<Iterator1>;
	using reviter2 = _MSTL reverse_iterator<Iterator2>;
	reviter1 rlast1(first1);
	reviter2 rlast2(first2);
	reviter1 rresult = _MSTL search(reviter1(last1), rlast1, reviter2(last2), rlast2);
	if (rresult == rlast1) return last1;
	Iterator1 result = rresult.base();
	_MSTL advance(result, -distance(first2, last2));
	return result;
}
template <typename Iterator1, typename Iterator2,
	enable_if_t<!(is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>), int> = 0>
constexpr Iterator1 __find_end_aux(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	Iterator1 result = last1;
	while (true) {
		Iterator1 new_result = _MSTL search(first1, last1, first2, last2);
		if (new_result == last1) return result;
		result = new_result;
		first1 = new_result;
		++first1;
	}
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator1 find_end(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	if (first2 == last2) return last1;
	return _INNER __find_end_aux(first1, last1, first2, last2);
}
#else
template <typename Iterator1, typename Iterator2,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator1 find_end(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	if (first2 == last2) return last1;
	if constexpr (is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>) {
		using reviter1 = _MSTL reverse_iterator<Iterator1>;
		using reviter2 = _MSTL reverse_iterator<Iterator2>;

		reviter1 rlast1(first1);
		reviter2 rlast2(first2);
		reviter1 rresult = _MSTL search(reviter1(last1), rlast1, reviter2(last2), rlast2);
		if (rresult == rlast1) return last1;

		Iterator1 result = rresult.base();
		_MSTL advance(result, -distance(first2, last2));
		return result;
	}
	else {
		Iterator1 result = last1;
		while (true) {
			Iterator1 new_result = _MSTL search(first1, last1, first2, last2);
			if (new_result == last1) return result;
			result = new_result;
			first1 = new_result;
			++first1;
		}
	}
}
#endif // MSTL_STANDARD_17__

template <typename Iterator1, typename Iterator2, typename BinaryPredicate,
	enable_if_t<is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
constexpr Iterator1 find_first_of(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, BinaryPredicate comp) {
	for (; first1 != last1; ++first1) {
		for (Iterator2 iter = first2; iter != last2; ++iter) {
			if (comp(*first1, *iter)) return first1;
		}
	}
	return last1;
}

template <typename Iterator1, typename Iterator2>
constexpr Iterator1 find_first_of(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _MSTL find_first_of(first1, last1, first2, last2, _MSTL equal_to<iter_value_t<Iterator1>>());
}

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr bool binary_search(Iterator first, Iterator last, const T& value) {
	Iterator i = _MSTL lower_bound(first, last, value);
	return i != last && !(value < *i);
}

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr bool binary_search(Iterator first, Iterator last, const T& value, Compare comp) {
	Iterator i = _MSTL lower_bound(first, last, value, comp);
	return i != last && !comp(value, *i);
}

template <typename Iterator1, typename Iterator2, typename Compare,
	enable_if_t<is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
constexpr bool includes(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Compare comp) {
	while (first1 != last1 && first2 != last2) {
		if (comp(*first2, *first1)) return false;

		if (comp(*first1, *first2)) ++first1;
		else ++first1, ++first2;
	}
	return first2 == last2;
}

template <typename Iterator1, typename Iterator2>
constexpr bool includes(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _MSTL includes(first1, last1, first2, last2, _MSTL less<iter_value_t<Iterator1>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_SEARCH_HPP__
