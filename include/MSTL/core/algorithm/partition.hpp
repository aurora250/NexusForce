#ifndef MSTL_CORE_ALGORITHM_PARTITION_HPP__
#define MSTL_CORE_ALGORITHM_PARTITION_HPP__
#include "shift.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename Predicate, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr Iterator partition(Iterator first, Iterator last, Predicate pred) {
	while (true) {
		while (true) {
			if (first == last) return first;

			if (pred(*first)) ++first;
			else break;
		}
		--last;
		while (true) {
			if (first == last) return first;

			if (!pred(*last)) --last;
			else break;
		}
		_MSTL iter_swap(first, last);
		++first;
	}
}

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
constexpr Iterator lomuto_partition(Iterator first, Iterator last, const T& pivot, Compare comp) {
	while (first < last) {
		while (comp(*first, pivot)) ++first;
		--last;
		while (comp(pivot, *last)) --last;
		if (!(first < last)) break;
		_MSTL iter_swap(first, last);
		++first;
	}
	return first;
}

template <typename Iterator, typename T>
constexpr Iterator lomuto_partition(Iterator first, Iterator last, const T& pivot) {
	return _MSTL lomuto_partition(first, last, pivot);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_PARTITION_HPP__
