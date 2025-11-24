#ifndef MSTL_CORE_ALGORITHM_HEAP_HPP__
#define MSTL_CORE_ALGORITHM_HEAP_HPP__
#include "../iterator/iterator.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 Iterator is_heap_until(Iterator first, Iterator last, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	Distance n = last - first;
	for (Distance child = 1; child < n; ++child) {
		Distance parent = (child - 1) / 2;
		if (comp(*(first + parent), *(first + child))) {
			return first + child;
		}
	}
	return last;
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 Iterator is_heap_until(Iterator first, Iterator last) {
	return _MSTL is_heap_until(first, last, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 bool is_heap(Iterator first, Iterator last, Compare comp) {
	return _MSTL is_heap_until(first, last, comp) == last;
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 bool is_heap(Iterator first, Iterator last) {
	return _MSTL is_heap_until(first, last) == last;
}

template <typename Iterator, typename Distance, typename T, typename Compare,
	enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap_aux(Iterator first, Distance hole_index, Distance top_index, T value, Compare comp) {
	Distance parent = (hole_index - 1) / 2;
	while (hole_index > top_index && comp(*(first + parent), value)) {
		*(first + hole_index) = *(first + parent);
		hole_index = parent;
		parent = (hole_index - 1) / 2;
	}
	*(first + hole_index) = value;
}

template <typename Iterator, typename Distance, typename T, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap_aux(Iterator first, Distance hole_index, Distance top_index, T value) {
	_MSTL push_heap_aux(first, hole_index, top_index, value, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap(Iterator first, Iterator last, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	if (last - first < 2) return;
	_MSTL push_heap_aux(first, Distance(last - first - 1), Distance(0), *(last - 1), comp);
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap(Iterator first, Iterator last) {
	_MSTL push_heap(first, last, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Distance, typename T, typename Compare,
	enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void adjust_heap(Iterator first, Distance hole_index, Distance len, T value, Compare comp) {
	Distance top_index = hole_index;
	Distance child = 2 * hole_index + 1;
	while (child < len) {
		if (child + 1 < len && comp(*(first + child), *(first + child + 1))) {
			++child;
		}
		if (!comp(value, *(first + child))) {
			break;
		}
		*(first + hole_index) = *(first + child);
		hole_index = child;
		child = 2 * hole_index + 1;
	}
	_MSTL push_heap_aux(first, hole_index, top_index, value, comp);
}

template <typename Iterator, typename Distance, typename T,
	enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void adjust_heap(Iterator first, Distance hole_index, Distance len, T value) {
	_MSTL adjust_heap(first, hole_index, len, value, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap_aux(Iterator first, Iterator last, Iterator result, T value, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	*result = *first;
	_MSTL adjust_heap(first, Distance(0), Distance(last - first), value, comp);
}

template <typename Iterator, typename T, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap_aux(Iterator first, Iterator last, Iterator result, T value) {
	_MSTL pop_heap_aux(first, last, result, value, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap(Iterator first, Iterator last, Compare comp) {
	if (last - first < 2) return;
	--last;
	_MSTL pop_heap_aux(first, last, last, *last, comp);
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap(Iterator first, Iterator last) {
	_MSTL pop_heap(first, last, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void sort_heap(Iterator first, Iterator last, Compare comp) {
	while (last - first > 1) {
		_MSTL pop_heap(first, last--, comp);
	}
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void sort_heap(Iterator first, Iterator last) {
	_MSTL sort_heap(first, last, less<iter_value_t<Iterator>>());
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void make_heap(Iterator first, Iterator last, Compare comp) {
	if (last - first < 2) return;
	using Distance = iter_difference_t<Iterator>;
	Distance len = last - first;
	if (len < 2) return;

	Distance parent = (len - 2) / 2;
	while (true) {
		_MSTL adjust_heap(first, parent, len, *(first + parent), comp);
		if (parent == 0) return;
		--parent;
	}
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void make_heap(Iterator first, Iterator last) {
	_MSTL make_heap(first, last, less<iter_value_t<Iterator>>());
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_HEAP_HPP__
