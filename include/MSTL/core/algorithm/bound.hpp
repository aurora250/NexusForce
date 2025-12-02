#ifndef MSTL_CORE_ALGORITHM_BOUND_HPP__
#define MSTL_CORE_ALGORITHM_BOUND_HPP__
#include "../typeinfo/concepts.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator lower_bound(Iterator first, Iterator last, const T& value, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	Distance len = _MSTL distance(first, last);
	Distance half;
	Iterator middle;
	while (len > 0) {
		half = len >> 1;
		middle = first;
		_MSTL advance(middle, half);
		if (comp(*middle, value)) {
			first = middle;
			++first;
			len = len - half - 1;
		}
		else len = half;
	}
	return first;
}

template <typename Iterator, typename T>
constexpr Iterator lower_bound(Iterator first, Iterator last, const T& value) {
	return _MSTL lower_bound(first, last, value, _MSTL less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator upper_bound(Iterator first, Iterator last, const T& value, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	Distance len = _MSTL distance(first, last);
	Distance half;
	Iterator middle;
	while (len > 0) {
		half = len >> 1;
		middle = first;
		_MSTL advance(middle, half);
		if (comp(value, *middle)) {
			first = middle;
			++first;
			len = len - half - 1;
		}
		else len = half;
	}
	return first;
}

template <typename Iterator, typename T>
constexpr Iterator upper_bound(Iterator first, Iterator last, const T& value) {
	return _MSTL upper_bound(first, last, value, _MSTL greater<iter_value_t<Iterator>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_BOUND_HPP__
