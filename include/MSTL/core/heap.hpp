#ifndef MSTL_HEAP_HPP__
#define MSTL_HEAP_HPP__
#include "concepts.hpp"
MSTL_BEGIN_NAMESPACE__

// is heap
template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 bool is_heap(Iterator first, Iterator last) {
	using Distance = iter_difference_t<Iterator>;
	Distance n = _MSTL distance(first, last);
	Distance parent = 0;
	for (Distance child = 1; child < n; ++child) {
		if (*(first + parent) < *(first + child)) 
			return false;
		if ((child & 1) == 0) ++parent;
	}
	return true;
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 bool is_heap(Iterator first, Iterator last, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	Distance n = _MSTL distance(first, last);
	Distance parent = 0;
	for (Distance child = 1; child < n; ++child) {
		if (comp(*(first + parent), *(first + child)))
			return false;
		if ((child & 1) == 0) ++parent;
	}
	return true;
}

// is heap until
template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 Iterator is_heap_until(Iterator first, Iterator last) {
	using Distance = iter_difference_t<Iterator>;
	Distance n = _MSTL distance(first, last);
	Distance parent = 0;
	for (Distance child = 1; child < n; ++child) {
		if (*(first + parent) < *(first + child)) {
			return first + child;
		}
		if ((child & 1) == 0) ++parent;
	}
	return last;
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 Iterator is_heap_until(Iterator first, Iterator last, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	Distance n = _MSTL distance(first, last);
	Distance parent = 0;
	for (Distance child = 1; child < n; ++child) {
		if (comp(*(first + parent), *(first + child))) {
			return first + child;
		}
		if ((child & 1) == 0) ++parent;
	}
	return last;
}

// push heap
template <typename Iterator, typename Distance, typename T, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap_aux(Iterator first, Distance hole_index, Distance top_index, T value) {
	Distance parent = (hole_index - 1) / 2;
	while (hole_index > top_index && *(first + parent) < value) {
		*(first + hole_index) = *(first + parent);
		hole_index = parent;
		parent = (hole_index - 1) / 2;
	}
	*(first + hole_index) = value;
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap(Iterator first, Iterator last) {
	using Distance = iter_difference_t<Iterator>;
	_MSTL push_heap_aux(first, Distance((last - first) - 1), Distance(0), *(last - 1));
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

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void push_heap(Iterator first, Iterator last, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	_MSTL push_heap_aux(first, Distance(last - first - 1), Distance(0), *(last - 1), comp);
}

// adjust heap
template <typename Iterator, typename Distance, typename T, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void adjust_heap(Iterator first, Distance hole_index, Distance len, T value) {
	Distance top_index = hole_index;
	Distance second_child = 2 * hole_index + 2;
	while (second_child < len) {
		if (*(first + second_child) < *(first + (second_child - 1))) --second_child;
		*(first + hole_index) = *(first + second_child);
		hole_index = second_child;
		second_child = 2 * (second_child + 1);
	}
	if (second_child == len) {
		*(first + hole_index) = *(first + (second_child - 1));
		hole_index = second_child - 1;
	}
	_MSTL push_heap_aux(first, hole_index, top_index, value);
}

template <typename Iterator, typename Distance, typename T, typename Compare,
	enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void adjust_heap(Iterator first, Distance holeIndex, Distance len, T value, Compare comp) {
	Distance topIndex = holeIndex;
	Distance secondChild = 2 * holeIndex + 2;
	while (secondChild < len) {
		if (comp(*(first + secondChild), *(first + (secondChild - 1))))
			--secondChild;
		*(first + holeIndex) = *(first + secondChild);
		holeIndex = secondChild;
		secondChild = 2 * (secondChild + 1);
	}
	if (secondChild == len) {
		*(first + holeIndex) = *(first + (secondChild - 1));
		holeIndex = secondChild - 1;
	}
	_MSTL push_heap_aux(first, holeIndex, topIndex, value, comp);
}

// pop heap
template <typename Iterator, typename T, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap_aux(Iterator first, Iterator last, Iterator result, T value) {
	using Distance = iter_difference_t<Iterator>;
	*result = *first;
	_MSTL adjust_heap(first, Distance(0), Distance(last - first), value);
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap(Iterator first, Iterator last) {
	--last;
	_MSTL pop_heap_aux(first, last, last, *last);
}

template <typename Iterator, typename T, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap_aux(Iterator first, Iterator last, Iterator result, T value, Compare comp) {
	using Distance = iter_difference_t<Iterator>;
	*result = *first;
	_MSTL adjust_heap(first, Distance(0), Distance(last - first), value, comp);
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void pop_heap(Iterator first, Iterator last, Compare comp) {
	--last;
	_MSTL pop_heap_aux(first, last, last, *last, comp);
}

// sort heap
template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void sort_heap(Iterator first, Iterator last) {
	while (last - first > 1) 
		_MSTL pop_heap(first, last--);
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void sort_heap(Iterator first, Iterator last, Compare comp) {
	while (last - first > 1)
		_MSTL pop_heap(first, last--, comp);
}

// make heap
template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void make_heap(Iterator first, Iterator last) {
	if (last - first < 2) return;
	using Distance = iter_difference_t<Iterator>;
	Distance len = last - first;
	Distance parent = (len - 2) / 2;
	while (true) {
		_MSTL adjust_heap(first, parent, len, *(first + parent));
		if (parent == 0) return;
		--parent;
	}
}

template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void make_heap(Iterator first, Iterator last, Compare comp) {
	if (last - first < 2) return;
	using Distance = iter_difference_t<Iterator>;
	Distance len = last - first;
	Distance parent = (len - 2) / 2;
	while (true) {
		_MSTL adjust_heap(first, parent, len, *(first + parent), comp);
		if (parent == 0) return;
		--parent;
	}
}

MSTL_END_NAMESPACE__
#endif // MSTL_HEAP_HPP__
