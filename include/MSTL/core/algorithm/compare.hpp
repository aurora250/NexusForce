#ifndef MSTL_CORE_ALGORITHM_COMPARE_HPP__
#define MSTL_CORE_ALGORITHM_COMPARE_HPP__
#include <initializer_list>
#include "../config/undef_cmacro.hpp"
#include "../functional/functor.hpp"
#include "../memory/memory.hpp"
#include "../utility/pair.hpp"
#include "bound.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator1, typename Iterator2, typename BinaryPredicate, enable_if_t<
	is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
MSTL_NODISCARD constexpr bool equal(Iterator1 first1, Iterator1 last1, Iterator2 first2, BinaryPredicate binary_pred)
noexcept(noexcept(++first1) && noexcept(++first2) && noexcept(binary_pred(*first1, *first2))) {
	for (; first1 != last1; ++first1, ++first2) {
		if (!binary_pred(*first1, *first2)) return false;
	}
	return true;
}

template <typename Iterator1, typename Iterator2>
MSTL_NODISCARD constexpr bool equal(Iterator1 first1, Iterator1 last1, Iterator2 first2)
noexcept(noexcept(_MSTL equal(first1, last1, first2, _MSTL equal_to<iter_value_t<Iterator1>>()))) {
	return _MSTL equal(first1, last1, first2, _MSTL equal_to<iter_value_t<Iterator1>>());
}

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr pair<Iterator, Iterator> equal_range(Iterator first, Iterator last, const T& value, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	Distance len = _MSTL distance(first, last);
	Distance half;
	Iterator middle, left, right;
	while (len > 0) {
		half = len >> 1;
		middle = first;
		_MSTL advance(middle, half);
		if (comp(*middle, value)) {
			first = middle;
			++first;
			len = len - half - 1;
		} else if (comp(value, *middle)) {
			len = half;
		} else {
			left = _MSTL lower_bound(first, middle, value, comp);
			_MSTL advance(first, len);
			right = _MSTL upper_bound(++middle, first, value, comp);
			return pair<Iterator, Iterator>(left, right);
		}
	}
	return pair<Iterator, Iterator>(first, first);
}

template <typename Iterator, typename T>
constexpr pair<Iterator, Iterator> equal_range(Iterator first, Iterator last, const T& value) {
	return _MSTL equal_range(first, last, value, _MSTL less<iter_value_t<Iterator>>());
}


template <typename T, typename Compare>
constexpr const T& max(const T& a, const T& b, Compare comp)
noexcept(noexcept(comp(a, b))) {
	return comp(a, b) ? b : a;
}

template <typename T>
constexpr const T& max(const T& a, const T& b)
noexcept(noexcept(a < b)) {
	return a < b ? b : a;
}

template <typename T, typename Compare>
constexpr const T& min(const T& a, const T& b, Compare comp)
noexcept(noexcept(comp(b, a))) {
	return comp(b, a) ? b : a;
}

template <typename T>
constexpr const T& min(const T& a, const T& b)
noexcept(noexcept(b < a)) {
	return b < a ? b : a;
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
pair<iter_value_t<Iterator>, iter_value_t<Iterator>>
constexpr minmax(Iterator first, Iterator last, Compare comp) {
	using T = iter_value_t<Iterator>;
	if (first == last) {
		return _MSTL make_pair(T(), T());
	}
	T min_val = *first;
	T max_val = *first;

	++first;
	for (; first != last; ++first) {
		if (comp(*first, min_val))
			min_val = *first;
		else if (!comp(*first, max_val))
			max_val = *first;
	}
	return _MSTL make_pair(min_val, max_val);
}

template <typename Iterator>
constexpr pair<iter_value_t<Iterator>, iter_value_t<Iterator>> minmax(Iterator first, Iterator last) {
	return _MSTL minmax(first, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename T, typename Compare>
constexpr const T& median(const T& a, const T& b, const T& c, Compare comp)
noexcept(noexcept(comp(a, b))) {
	if (comp(a, b))
		if (comp(b, c))
			return b;
		else if (comp(a, c))
			return c;
		else
			return a;
	else if (comp(a, c))
		return a;
	else if (comp(b, c))
		return c;
	else
		return b;
}

template <typename T>
constexpr const T& median(const T& a, const T& b, const T& c)
noexcept(noexcept(_MSTL median(a, b, c, _MSTL less<T>()))) {
	return _MSTL median(a, b, c, _MSTL less<T>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator max_element(Iterator first, Iterator last, Compare comp) {
	if (first == last) return first;
	Iterator result = first;
	while (++first != last)
		if (comp(*result, *first)) result = first;
	return result;
}

template <typename Iterator>
constexpr Iterator max_element(Iterator first, Iterator last) {
	return _MSTL max_element(first, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename T>
constexpr const T& max(std::initializer_list<T> list) {
	auto iter = _MSTL max_element(list.begin(), list.end());
	return *iter;
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator min_element(Iterator first, Iterator last, Compare comp) {
	if (first == last) return first;
	Iterator result = first;
	while (++first != last)
		if (comp(*first, *result)) result = first;
	return result;
}

template <typename Iterator>
constexpr Iterator min_element(Iterator first, Iterator last) {
	return _MSTL min_element(first, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename T>
constexpr const T& min(std::initializer_list<T> list) {
	return *_MSTL min_element(list.begin(), list.end());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr pair<Iterator, Iterator> minmax_element(Iterator first, Iterator last, Compare comp) {
	Iterator min = _MSTL min_element(first, last, comp);
	Iterator max = _MSTL max_element(first, last, comp);
	return _MSTL make_pair(min, max);
}

template <typename Iterator>
constexpr pair<Iterator, Iterator> minmax_element(Iterator first, Iterator last) {
	return _MSTL minmax_element(first, last, _MSTL less<iter_value_t<Iterator>>());
}

template <typename T, typename Compare>
constexpr const T& clamp(const T& value, const T& lower, const T& upper, Compare comp)
noexcept(noexcept(comp(value, lower))) {
	if (comp(value, lower))
		return lower;
	else if (comp(upper, value))
		return upper;
	return value;
}

template <typename T>
constexpr const T& clamp(const T& value, const T& lower, const T& upper)
noexcept(noexcept(_MSTL clamp(value, lower, upper, _MSTL less<T>()))) {
	return _MSTL clamp(value, lower, upper, _MSTL less<T>());
}


template <typename Iterator1, typename Iterator2, typename Compare, enable_if_t<
	is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
MSTL_NODISCARD constexpr bool lexicographical_compare(
	Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Compare comp)
noexcept(
	noexcept(++first1) && noexcept(++first2) &&
	noexcept(comp(*first1, *first2)) &&
	noexcept(first1 == last1 && first2 != last2)
	) {
	for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
		if (comp(*first1, *first2)) return true;
		if (comp(*first2, *first1)) return false;
	}
	return first1 == last1 && first2 != last2;
}


MSTL_BEGIN_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
	is_ranges_cot_iter_v<Iterator1> && is_ranges_cot_iter_v<Iterator2>, int> = 0>
MSTL_NODISCARD constexpr bool __lexicographical_compare_aux(
	Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) noexcept {
	const auto len1 = static_cast<size_t>(last1 - first1);
	const auto len2 = static_cast<size_t>(last2 - first2);
	const size_t clp = _MSTL min(len1, len2);

	const int result = _MSTL memory_compare(
		_MSTL addressof(*first1), _MSTL addressof(*first2), clp * sizeof(iter_value_t<Iterator1>));
	return result != 0 ? result < 0 : len1 < len2;
}

template <typename Iterator1, typename Iterator2, enable_if_t<
	!(is_ranges_cot_iter_v<Iterator1> && is_ranges_cot_iter_v<Iterator2>), int> = 0>
MSTL_NODISCARD constexpr bool __lexicographical_compare_aux(
	Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2)
noexcept(noexcept(_MSTL lexicographical_compare(first1, last1, first2, last2, _MSTL less<iter_value_t<Iterator1>>()))) {
	return _MSTL lexicographical_compare(first1, last1, first2, last2, _MSTL less<iter_value_t<Iterator1>>());
}

MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
	is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
MSTL_NODISCARD constexpr bool lexicographical_compare(
	Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2)
noexcept(noexcept(_INNER __lexicographical_compare_aux(first1, last1, first2, last2))) {
	return _INNER __lexicographical_compare_aux(first1, last1, first2, last2);
}


template <typename Iterator1, typename Iterator2, typename Compare, enable_if_t<
	is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
pair<Iterator1, Iterator2>
constexpr mismatch(Iterator1 first1, Iterator1 last1, Iterator2 first2, Compare comp) {
	while (first1 != last1 && comp(*first1, *first2)) {
		++first1; ++first2;
	}
	return _MSTL make_pair<Iterator1, Iterator2>(first1, first2);
}

template <typename Iterator1, typename Iterator2>
constexpr pair<Iterator1, Iterator2> mismatch(Iterator1 first1, Iterator1 last1, Iterator2 first2) {
	return _MSTL mismatch(first1, last1, first2, _MSTL equal_to<iter_value_t<Iterator1>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_COMPARE_HPP__
