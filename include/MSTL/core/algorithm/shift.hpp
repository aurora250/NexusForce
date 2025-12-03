#ifndef MSTL_CORE_ALGORITHM_SHIFT_HPP__
#define MSTL_CORE_ALGORITHM_SHIFT_HPP__
#include "../numeric/math.hpp"
#include "../memory/memory.hpp"
#include "../compound/pair.hpp"
#include "../functional/functor.hpp"
#include "search.hpp"
#include "iterator.hpp"
MSTL_BEGIN_NAMESPACE__

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

template <typename Iterator, typename Function, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Function for_each(Iterator first, Iterator last, Function f) {
	for (; first != last; ++first) f(*first);
	return f;
}

template<typename Iterator, typename Function, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator for_each_n(Iterator first, const size_t n, Function f) {
	for (size_t i = 0; i < n; i++) {
		f(*first);
		++first;
	}
	return first;
}

template <typename Iterator, typename Generator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr void generate(Iterator first, Iterator last, Generator gen) {
	for (; first != last; ++first) {
		*first = gen();
	}
}

template <typename Iterator, typename Generator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator generate_n(Iterator first, size_t n, Generator gen) {
	for (; n > 0; --n, ++first) {
		*first = gen();
	}
	return first;
}

template <typename Iterator1, typename Iterator2, typename T,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 replace_copy(Iterator1 first, Iterator1 last, Iterator2 result,
	const T& old_value, const T& new_value) {
	for (; first != last; ++first, ++result) {
		*result = *first == old_value ? new_value : *first;
	}
	return result;
}

template <typename Iterator1, typename Iterator2, typename Predicate, typename T,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 replace_copy_if(Iterator1 first, Iterator1 last, Iterator2 result,
	Predicate pred, const T& new_value) {
	for (; first != last; ++first, ++result) {
		*result = pred(*first) ? new_value : *first;
	}
	return result;
}

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr void replace(Iterator first, Iterator last, const T& old_value, const T& new_value) {
	for (; first != last; ++first) {
		if (*first == old_value) *first = new_value;
	}
}

template <typename Iterator, typename Predicate, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr void replace_if(Iterator first, Iterator last, Predicate pred, const T& new_value) {
	for (; first != last; ++first) {
		if (pred(*first)) *first = new_value;
	}
}

#ifndef MSTL_STANDARD_17__
MSTL_BEGIN_INNER__
template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void __reverse_aux(Iterator first, Iterator last) {
	while (first < last) {
		--last;
		_MSTL iter_swap(first, last);
		++first;
	}
}
template <typename Iterator, enable_if_t<!is_ranges_rnd_iter_v<Iterator>, int> = 0>
void __reverse_aux(Iterator first, Iterator last) {
	while (true) {
		if (first == last || first == --last) return;
		--last;
		_MSTL iter_swap(first, last);
		++first;
	}
}
MSTL_END_INNER__

template <typename Iterator, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
void reverse(Iterator first, Iterator last) {
	_INNER __reverse_aux(first, last);
}
#else
template <typename Iterator, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr void reverse(Iterator first, Iterator last) {
	if constexpr (is_ranges_rnd_iter_v<Iterator>) {
		while (first < last) {
			--last;
			_MSTL iter_swap(first, last);
			++first;
		}
	}
	else {
		while (true) {
			if (first == last || first == --last) return;
			--last;
			_MSTL iter_swap(first, last);
			++first;
		}
	}
}
#endif // MSTL_STANDARD_17__

MSTL_BEGIN_INNER__

#ifndef MSTL_STANDARD_17__
template <typename Iterator, enable_if_t<!is_ranges_bid_iter_v<Iterator>, int> = 0>
void __rotate_aux_dispatch(Iterator first, Iterator middle, Iterator last) {
	for (Iterator i = middle; ;) {
		_MSTL iter_swap(first, i);
		++first;
		++i;
		if (first == middle) {
			if (i == last) return;
			middle = i;
		}
		else if (i == last)
			i = middle;
	}
}
template <typename Iterator, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
void __rotate_aux_dispatch(Iterator first, Iterator middle, Iterator last) {
	_MSTL reverse(first, middle);
	_MSTL reverse(middle, last);
	_MSTL reverse(first, last);
}

template <typename Iterator, enable_if_t<!is_ranges_rnd_iter_v<Iterator>, int> = 0>
void __rotate_aux(Iterator first, Iterator middle, Iterator last) {
	_INNER __rotate_aux_dispatch(first, middle, last);
}
#else
template <typename Iterator, enable_if_t<!is_ranges_rnd_iter_v<Iterator>, int> = 0>
constexpr void __rotate_aux(Iterator first, Iterator middle, Iterator last) {
	if (first == middle || middle == last) return;
	if constexpr (is_ranges_bid_iter_v<Iterator>) {
		_MSTL reverse(first, middle);
		_MSTL reverse(middle, last);
		_MSTL reverse(first, last);
	}
	else {
		for (Iterator i = middle; ;) {
			_MSTL iter_swap(first, i);
			++first;
			++i;
			if (first == middle) {
				if (i == last) return;
				middle = i;
			} else if (i == last) {
				i = middle;
			}
		}
	}
}
#endif // MSTL_STANDARD_17__

template <typename Iterator, typename Distance>
constexpr void __rotate_cycle_aux(Iterator first, Iterator last, Iterator initial, Distance shift) {
	iter_value_t<Iterator> value = *initial;
	Iterator ptr1 = initial;
	Iterator ptr2 = ptr1 + shift;
	while (ptr2 != initial) {
		*ptr1 = *ptr2;
		ptr1 = ptr2;
		if (last - ptr2 > shift) {
			ptr2 += shift;
		} else {
			ptr2 = first + (shift - (last - ptr2));
		}
	}
	*ptr1 = value;
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
constexpr void __rotate_aux(Iterator first, Iterator middle, Iterator last) {
	iter_difference_t<Iterator> n = _MSTL gcd(last - first, middle - first);
	while (n--) {
		_INNER __rotate_cycle_aux(first, last, first + n, middle - first);
	}
}

MSTL_END_INNER__


template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr void rotate(Iterator first, Iterator middle, Iterator last) {
	if (first == middle || middle == last) return;
	_INNER __rotate_aux(first, middle, last);
}

template <typename Iterator1, typename Iterator2,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 rotate_copy(Iterator1 first, Iterator1 middle, Iterator1 last, Iterator2 result) {
	return _MSTL copy(first, middle, _MSTL copy(middle, last, result));
}

template <typename Iterator,
	enable_if_t<is_ranges_fwd_iter_v<Iterator> && is_default_constructible_v<iter_value_t<Iterator>>, int> = 0>
constexpr void shift_left(Iterator first, Iterator last, size_t n) {
	if (first == last || n == 0) return;
	if (n >= _MSTL distance(first, last)) {
		for (; first != last; ++first) {
			*first = _MSTL initialize<iter_value_t<Iterator>>();
		}
		return;
	}
	Iterator new_first = _MSTL next(first, n);
	_MSTL copy(new_first, last, first);
	Iterator end = _MSTL prev(last, -n);
	for (; end != last; ++end) {
		*end = _MSTL initialize<iter_value_t<Iterator>>();
	}
}

template<typename Iterator,
	enable_if_t<is_ranges_bid_iter_v<Iterator> && is_default_constructible_v<iter_value_t<Iterator>>, int> = 0>
constexpr void shift_right(Iterator first, Iterator last, size_t n) {
	if (first == last || n == 0) return;
	if (n >= _MSTL distance(first, last)) {
		for (; first != last; ++first) {
			*first = _MSTL initialize<iter_value_t<Iterator>>();
		}
		return;
	}
	auto new_last = _MSTL prev(last, -n);
	_MSTL move_backward(first, new_last, last);
	auto end = _MSTL next(first, n);
	for (; first != end; ++first) {
		*first = _MSTL initialize<iter_value_t<Iterator>>();
	}
}

template <typename Iterator1, typename Iterator2,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 swap_ranges(Iterator1 first1, Iterator1 last1, Iterator2 first2) {
	for (; first1 != last1; ++first1, ++first2) {
		_MSTL iter_swap(first1, first2);
	}
	return first2;
}

template <typename Iterator1, typename Iterator2, typename UnaryOperation,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 transform(Iterator1 first, Iterator1 last, Iterator2 result, UnaryOperation op)
noexcept(noexcept(++first) && noexcept(++result) && noexcept(*result = op(*first))) {
	for (; first != last; ++first, ++result) {
		*result = op(*first);
	}
	return result;
}

template <typename Iterator1, typename Iterator2, typename Iterator3, typename BinaryOperation,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 transform(
	Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator3 result, BinaryOperation binary_op
) noexcept(
	noexcept(++first1) && noexcept(first2) &&
	noexcept(++result) && noexcept(*result = binary_op(*first1, *first2))
) {
	for (; first1 != last1; ++first1, ++first2, ++result) {
		*result = binary_op(*first1, *first2);
	}
	return result;
}

template <typename Iterator1, typename Iterator2, typename BinaryPredicate,
	enable_if_t<is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 unique_copy(Iterator1 first, Iterator1 last,
	Iterator2 result, BinaryPredicate binary_pred) {
	if (first == last) return result;
	*result = *first;
	while (++first != last) {
		if (!binary_pred(*result, *first)) *++result = *first;
	}
	return ++result;
}

template <typename Iterator1, typename Iterator2>
constexpr Iterator2 unique_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
	return _MSTL unique_copy(first, last, result, _MSTL equal_to<iter_value_t<Iterator1>>());
}

template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator unique(Iterator first, Iterator last) {
	first = _MSTL adjacent_find(first, last);
	return _MSTL unique_copy(first, last, first);
}

template <typename Iterator, typename BinaryPredicate, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator unique(Iterator first, Iterator last, BinaryPredicate binary_pred) {
	first = _MSTL adjacent_find(first, last, binary_pred);
	return _MSTL unique_copy(first, last, first, binary_pred);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_SHIFT_HPP__
