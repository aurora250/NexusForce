#ifndef MSTL_CORE_ALGORITHM_SET_HPP__
#define MSTL_CORE_ALGORITHM_SET_HPP__
#include "../typeinfo/concepts.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_union(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			*result = *first1;
			++first1;
		} else if (*first2 < *first1) {
			*result = *first2;
			++first2;
		} else {
			*result = *first1;
			++first1; ++first2;
		}
		++result;
	}
	return _MSTL copy(first2, last2, _MSTL copy(first1, last1, result));
}

template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_intersection(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			++first1;
		} else if (*first2 < first1) {
			++first2;
		} else {
			*result = *first1;
			++first1; ++first2;
			++result;
		}
	}
	return result;
}

template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_difference(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			*result = *first1;
			++first1;
			++result;
		} else if (*first2 < first1) {
			++first2;
		} else {
			++first1; ++first2;
		}
	}
	return _MSTL copy(first1, last1, result);
}

template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_symmetric_difference(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			*result = *first1;
			++first1;
			++result;
		} else if (*first2 < first1) {
			*result = *first2;
			++first1;
			++result;
		} else {
			++first1; ++first2;
		}
	}
	return _MSTL copy(first2, last2, _MSTL copy(first1, last1, result));
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_SET_HPP__
