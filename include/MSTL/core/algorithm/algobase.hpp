#ifndef MSTL_CORE_ALGORITHM_ALGOBASE_HPP__
#define MSTL_CORE_ALGORITHM_ALGOBASE_HPP__
#include "../compound/pair.hpp"
#include "../functional/functor.hpp"
#include "../iterator/iterator.hpp"
#include "../memory/memory.hpp"
#include "../config/undef_cmacro.hpp"
#include <initializer_list>
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

template <typename Iterator, typename T, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr void fill(Iterator first, Iterator last, const T& value) {
	for (; first != last; ++first) *first = value;
}

template <typename Iterator, typename T, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator fill_n(Iterator first, size_t n, const T& value) {
	for (; n > 0; --n, ++first) *first = value;
	return first;
}

template <typename Iterator1, typename Iterator2, enable_if_t<
	is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
constexpr void iter_swap(Iterator1 a, Iterator2 b)
noexcept(noexcept(_MSTL swap(*a, *b))) {
	_MSTL swap(*a, *b);
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


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	iter_difference_t<Iterator1> n = _MSTL distance(first, last);
	for (; n > 0; --n, ++first, ++result)
		*result = *first;
	return result;
}
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	const auto n = static_cast<size_t>(last - first);
	const auto bytes = n * sizeof(iter_value_t<Iterator1>);
	_MSTL memory_move(_MSTL addressof(*result), _MSTL addressof(*first), bytes);
	return result + n;
}
MSTL_END_INNER__


template <typename Iterator1, typename Iterator2, enable_if_t<
	is_iter_v<Iterator1> && is_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 copy(Iterator1 first, Iterator1 last, Iterator2 result) {
	if (first == last) return result;
	return _INNER __copy_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
constexpr pair<Iterator1, Iterator2> __copy_n_aux(Iterator1 first, size_t count, Iterator2 result) {
	Iterator1 last = first + count;
	return pair<Iterator1, Iterator2>(last, _MSTL copy(first, last, result));
}
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
constexpr pair<Iterator1, Iterator2> __copy_n_aux(Iterator1 first, size_t count, Iterator2 result) {
	for (; count > 0; --count, ++first, ++result)
		*result = *first;
	return pair<Iterator1, Iterator2>(first, result);
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
constexpr pair<Iterator1, Iterator2> copy_n(Iterator1 first, size_t count, Iterator2 result) {
	return _INNER __copy_n_aux(first, count, result);
}

template <typename Iterator1, typename Iterator2, typename UnaryPredicate>
constexpr Iterator2 copy_if(Iterator1 first, Iterator1 last, Iterator2 result, UnaryPredicate unary_pred) {
	for (; first != last; ++first) {
		if (unary_pred(*first))
			*result++ = *first;
	}
	return result;
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __copy_backward_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	iter_difference_t<Iterator1> n = _MSTL distance(first, last);
	for (; n > 0; --n)
		*--result = *--last;
	return result;
}
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __copy_backward_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	const auto n = static_cast<size_t>(last - first);
	last -= n;
	_MSTL memory_move(_MSTL addressof(*result), _MSTL addressof(*first), n * sizeof(iter_value_t<Iterator1>));
	return result;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
	is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 copy_backward(Iterator1 first, Iterator1 last, Iterator2 result) {
	if (first == last) return result;
	return _INNER __copy_backward_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	iter_difference_t<Iterator1> n = _MSTL distance(first, last);
	for (; n > 0; --n, ++first, ++result)
		*result = _MSTL move(*first);
	return result;
}
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	const auto n = static_cast<size_t>(last - first);
	_MSTL memory_move(_MSTL addressof(*result), _MSTL addressof(*first), n * sizeof(iter_value_t<Iterator1>));
	return result + n;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
	is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 move(Iterator1 first, Iterator1 last, Iterator2 result) {
	if (first == last) return result;
	return _INNER __move_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __move_backward_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	for (size_t n = _MSTL distance(first, last); n > 0; --n)
		*--result = _MSTL move(*--last);
	return result;
}
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_cot_iter_v<Iterator1>, int> = 0>
constexpr Iterator2 __move_backward_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
	const auto n = static_cast<size_t>(last - first);
	last -= n;
	_MSTL memory_move(_MSTL addressof(*result), _MSTL addressof(*first), n * sizeof(iter_value_t<Iterator1>));
	return result;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
	is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 move_backward(Iterator1 first, Iterator1 last, Iterator2 result) {
	if (first == last) return result;
	return _INNER __move_backward_aux(first, last, result);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_ALGOBASE_HPP__
