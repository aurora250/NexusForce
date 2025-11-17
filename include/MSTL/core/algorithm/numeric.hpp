#ifndef MSTL_NUMERIC_HPP__
#define MSTL_NUMERIC_HPP__
#include "../functional/functor.hpp"
#include "../utility/concepts.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename T, typename BinaryOperation,
	enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 T accumulate(Iterator first, Iterator second, T init, BinaryOperation binary_op) {
	for (; first != second; ++first) init = binary_op(init, *first);
	return init;
}

template <typename Iterator, typename T>
MSTL_CONSTEXPR20 T accumulate(Iterator first, Iterator second, T init) {
	return _MSTL accumulate(first, second, init, _MSTL plus<T>());
}

template <typename Iterator1, typename Iterator2, typename BinaryOperation,
	enable_if_t<is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 adjacent_difference(Iterator1 first, Iterator1 last,
	Iterator2 result, BinaryOperation binary_op) {
	if (first == last) return result;
	using T = iter_value_t<Iterator1>;
	*result = *first;
	T value = *first;
	while (++first != last) {
		T tem = *first;
		*++result = binary_op(tem, value);
		value = tem;
	}
	return ++result;
}

template <typename Iterator1, typename Iterator2>
MSTL_CONSTEXPR20 Iterator2 adjacent_difference(Iterator1 first, Iterator1 last, Iterator2 result) {
	return _MSTL adjacent_difference(first, last, result, _MSTL minus<iter_value_t<Iterator1>>());
}

template <typename Iterator1, typename Iterator2, typename T, typename BinaryOperation1, typename BinaryOperation2,
	enable_if_t<is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 T inner_product(Iterator1 first1, Iterator1 last1, Iterator2 first2, T init,
	BinaryOperation1 binary_op1, BinaryOperation2 binary_op2) {
	for (; first1 != last1; ++first1, ++first2) 
		init = binary_op1(init, binary_op2(*first1, *first2));
	return init;
}

template <typename Iterator1, typename Iterator2, typename T>
MSTL_CONSTEXPR20 T inner_product(Iterator1 first1, Iterator1 last1, Iterator2 first2, T init) {
	return _MSTL inner_product(first1, last1, first2, init, _MSTL plus<T>(), _MSTL multiplies<T>());
}

template <typename Iterator1, typename Iterator2, typename BinaryOperation,
	enable_if_t<is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 partial_sum(Iterator1 first, Iterator1 last, Iterator2 result, BinaryOperation binary_op) {
	if (first == last) return result;
	*result = *first;
	iter_value_t<Iterator1> value = *first;
	while (++first != last) {
		value = binary_op(value, *first);
		*++result = value;
	}
	return ++result;
}

template <typename Iterator1, typename Iterator2>
MSTL_CONSTEXPR20 Iterator2 partial_sum(Iterator1 first, Iterator1 last, Iterator2 result) {
	return _MSTL partial_sum(first, last, result, _MSTL plus<iter_value_t<Iterator1>>());
}

template <typename Iterator, typename T, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void iota(Iterator first, Iterator last, T value) {
	while (first != last) {
		*first++ = value;
		++value;
	}
}

MSTL_END_NAMESPACE__
#endif // MSTL_NUMERIC_HPP__
