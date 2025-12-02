#ifndef MSTL_CORE_ALGORITHM_PERMUTATION_HPP__
#define MSTL_CORE_ALGORITHM_PERMUTATION_HPP__
#include "shift.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator1, typename Iterator2, typename BinaryPred,
	enable_if_t<is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>, int> = 0>
constexpr bool is_permutation(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, BinaryPred pred) {
	iter_difference_t<Iterator1> len1 = _MSTL distance(first1, last1);
	iter_difference_t<Iterator2> len2 = _MSTL distance(first2, last2);
	if (len1 != len2) return false;

	for (; first1 != last1 && first2 != last2; ++first1, (void) ++first2) {
		if (!pred(*first1, *first2)) break;
	}
	if (first1 == last1) return true;

	for (Iterator1 i = first1; i != last1; ++i) {
		bool is_repeated = false;
		for (Iterator1 j = first1; j != i; ++j) {
			if (pred(*j, *i)) {
				is_repeated = true;
				break;
			}
		}
		if (!is_repeated) {
			size_t c2 = 0;
			for (Iterator2 j = first2; j != last2; ++j) {
				if (pred(*i, *j)) ++c2;
			}
			if (c2 == 0) return false;
			size_t c1 = 1;
			Iterator1 j = i;
			for (++j; j != last1; ++j) {
				if (pred(*i, *j)) ++c1;
			}
			if (c1 != c2) return false;
		}
	}
	return true;
}

template <typename Iterator1, typename Iterator2>
constexpr bool is_permutation(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _MSTL is_permutation(first1, last1, first2, last2, _MSTL equal_to<iter_value_t<Iterator1>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr bool next_permutation(Iterator first, Iterator last, Compare comp) {
	if (first == last) return false;
	Iterator i = first;
	++i;
	if (i == last) return false;
	i = last;
	--i;
	for (;;) {
		Iterator ii = i;
		--i;
		if (comp(*i, *ii)) {
			Iterator j = last;
			while (!comp(*i, *--j)) {}
			_MSTL iter_swap(i, j);
			_MSTL reverse(ii, last);
			return true;
		}
		if (i == first) {
			_MSTL reverse(first, last);
			return false;
		}
	}
}

template <typename Iterator>
constexpr bool next_permutation(Iterator first, Iterator last) {
	return _MSTL next_permutation(first, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr bool prev_permutation(Iterator first, Iterator last, Compare comp) {
	if (first == last) return false;
	Iterator i = first;
	++i;
	if (i == last) return false;
	i = last;
	--i;
	for (;;) {
		Iterator ii = i;
		--i;
		if (comp(*ii, *i)) {
			Iterator j = last;
			while (!comp(*--j, *i)) {}
			_MSTL iter_swap(i, j);
			_MSTL reverse(ii, last);
			return true;
		}
		if (i == first) {
			_MSTL reverse(first, last);
			return false;
		}
	}
}

template <typename Iterator>
constexpr bool prev_permutation(Iterator first, Iterator last) {
	return _MSTL prev_permutation(first, last, _MSTL less<iter_value_t<Iterator>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_PERMUTATION_HPP__
