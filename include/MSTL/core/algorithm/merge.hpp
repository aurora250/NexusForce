#ifndef MSTL_CORE_ALGORITHM_MERGE_HPP__
#define MSTL_CORE_ALGORITHM_MERGE_HPP__
#include "../memory/temporary_buffer.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator1, typename Iterator2, typename Iterator3, typename Compare,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 merge(Iterator1 first1, Iterator1 last1, Iterator2 first2,
	Iterator2 last2, Iterator3 result, Compare comp) {
	while (first1 != last1 && first2 != last2) {
		if (comp(*first2, *first1)) {
			*result = *first2;
			++first2;
		} else {
			*result = *first1;
			++first1;
		}
		++result;
	}
	return _MSTL copy(first2, last2, _MSTL copy(first1, last1, result));
}

template <typename Iterator1, typename Iterator2, typename Iterator3>
constexpr Iterator3 merge(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Iterator3 result) {
	return _MSTL merge(first1, last1, first2, last2, result, _MSTL less<iter_value_t<Iterator1>>());
}

MSTL_BEGIN_INNER__

template <typename Iterator, typename Distance, typename Compare>
constexpr void __merge_without_buffer_aux(Iterator first, Iterator middle, Iterator last,
	Distance len1, Distance len2, Compare comp) {
	if (len1 == 0 || len2 == 0) return;
	if (len1 + len2 == 2) {
		if (comp(*middle, *first)) _MSTL iter_swap(first, middle);
		return;
	}
	Iterator first_cut = first;
	Iterator second_cut = middle;
	Distance len11 = 0;
	Distance len22 = 0;
	if (len1 > len2) {
		len11 = len1 / 2;
		_MSTL advance(first_cut, len11);
		second_cut = _MSTL lower_bound(middle, last, *first_cut, comp);
		len22 = _MSTL distance(middle, second_cut);
	} else {
		len22 = len2 / 2;
		_MSTL advance(second_cut, len22);
		first_cut = _MSTL upper_bound(first, middle, *second_cut, comp);
		len11 = _MSTL distance(first, first_cut);
	}
	_MSTL rotate(first_cut, middle, second_cut);
	Iterator new_middle = first_cut;
	_MSTL advance(new_middle, len22);
	_INNER __merge_without_buffer_aux(first, first_cut, new_middle, len11, len22, comp);
	_INNER __merge_without_buffer_aux(new_middle, second_cut, last, len1 - len11, len2 - len22, comp);
}

template <typename Iterator1, typename Iterator2, typename Distance>
constexpr Iterator1 __rotate_with_buffer_aux(Iterator1 first, Iterator1 middle, Iterator1 last,
	Distance len1, Distance len2, Iterator2 buffer, Distance buffer_size) {
	Iterator2 buffer_end;
	if (len1 > len2 && len2 <= buffer_size) {
		buffer_end = _MSTL copy(middle, last, buffer);
		_MSTL copy_backward(first, middle, last);
		return _MSTL copy(buffer, buffer_end, first);
	}
	if (len1 <= buffer_size) {
		buffer_end = _MSTL copy(first, middle, buffer);
		_MSTL copy(middle, last, first);
		return _MSTL copy_backward(buffer, buffer_end, last);
	}
	_MSTL rotate(first, middle, last);
	_MSTL advance(first, len2);
	return first;
}

template <typename Iterator, typename Distance, typename Pointer, typename Compare>
constexpr void __merge_with_buffer_aux(Iterator first, Iterator middle, Iterator last,
	Distance len1, Distance len2, Pointer buffer, Distance buffer_size, Compare comp) {
	if (len1 <= len2 && len1 <= buffer_size) {
		Pointer end_buffer = _MSTL copy(first, middle, buffer);
		_MSTL merge(buffer, end_buffer, middle, last, first, comp);
	}
	else if (len2 <= buffer_size) {
		Pointer end_buffer = _MSTL copy(middle, last, buffer);
		if (first == middle) {
			_MSTL copy_backward(buffer, end_buffer, last);
			return;
		}
		if (buffer == end_buffer) {
			_MSTL copy_backward(first, middle, last);
			return;
		}
		--middle;
		--end_buffer;
		while (true) {
			if (comp(*end_buffer, *middle)) {
				*--last = *middle;
				if (first == middle) {
					_MSTL copy_backward(buffer, ++end_buffer, last);
					return;
				}
				--middle;
			}
			else {
				*--last = *end_buffer;
				if (buffer == end_buffer) {
					_MSTL copy_backward(first, ++middle, last);
					return;
				}
				--end_buffer;
			}
		}
	}
	else {
		Iterator first_cut = first;
		Iterator second_cut = middle;
		Distance len11 = 0;
		Distance len22 = 0;
		if (len1 > len2) {
			len11 = len1 / 2;
			_MSTL advance(first_cut, len11);
			second_cut = _MSTL lower_bound(middle, last, *first_cut, comp);
			len22 = _MSTL distance(middle, second_cut);
		} else {
			len22 = len2 / 2;
			_MSTL advance(second_cut, len22);
			first_cut = _MSTL upper_bound(first, middle, *second_cut, comp);
			len11 = _MSTL distance(first, first_cut);
		}
		Iterator new_middle = _INNER __rotate_with_buffer_aux(
			first_cut, middle, second_cut, len1 - len11, len22, buffer, buffer_size);

		_INNER __merge_with_buffer_aux(
			first, first_cut, new_middle, len11, len22, buffer, buffer_size, comp);
		_INNER __merge_with_buffer_aux(
			new_middle, second_cut, last, len1 - len11, len2 - len22, buffer, buffer_size, comp);
	}
}

MSTL_END_INNER__

template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr void inplace_merge(Iterator first, Iterator middle, Iterator last, Compare comp) {
	if (first == middle || middle == last) return;
	using Distance = iter_difference_t<Iterator>;
	Distance len1 = _MSTL distance(first, middle);
	Distance len2 = _MSTL distance(middle, last);
	temporary_buffer<Iterator> buffer(first, last);
	if (buffer.begin() == 0) {
		_INNER __merge_without_buffer_aux(first, middle, last, len1, len2, comp);
	} else {
		_INNER __merge_with_buffer_aux(first, middle, last, len1, len2,
			buffer.begin(), Distance(buffer.size()), comp);
	}
}

template <typename Iterator>
constexpr void inplace_merge(Iterator first, Iterator middle, Iterator last) {
	return _MSTL inplace_merge(first, middle, last, _MSTL less<iter_value_t<Iterator>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_MERGE_HPP__
