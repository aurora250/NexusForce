#ifndef MSTL_CORE_ALGORITHM_LEONARDO_HEAP_HPP__
#define MSTL_CORE_ALGORITHM_LEONARDO_HEAP_HPP__
#include "../container/vector.hpp"
#include "../numeric/math.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void adjust_leonardo_heap(Iterator first, size_t current_heap, int level_index, vector<int>& levels) {
	size_t child_heap1;
	size_t child_heap2;
	while (level_index > 0) {
		size_t prev_heap = current_heap - leonardo(levels[level_index]);
		if (*(first + current_heap) < *(first + prev_heap)) {
			if (levels[level_index] > 1) {
				child_heap1 = current_heap - 1 - leonardo(levels[level_index] - 2);
				child_heap2 = current_heap - 1;
				if (*(first + prev_heap) < *(first + child_heap1)) break;
				if (*(first + prev_heap) < *(first + child_heap2)) break;
			}
			_MSTL iter_swap(first + current_heap, first + prev_heap);
			current_heap = prev_heap;
			--level_index;
		}
		else {
			break;
		}
	}
	int current_level = levels[level_index];
	while (current_level > 1) {
		size_t max_child = current_heap;
		child_heap1 = current_heap - 1 - leonardo(current_level - 2);
		child_heap2 = current_heap - 1;

		if (*(first + max_child) < *(first + child_heap1)) max_child = child_heap1;
		if (*(first + max_child) < *(first + child_heap2)) max_child = child_heap2;

		if (max_child == child_heap1) {
			_MSTL iter_swap(first + current_heap, first + child_heap1);
			current_heap = child_heap1;
			--current_level;
		}
		else if (max_child == child_heap2) {
			_MSTL iter_swap(first + current_heap, first + child_heap2);
			current_heap = child_heap2;
			current_level -= 2;
		}
		else {
			break;
		}
	}
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void push_leonardo_heap(Iterator first, Iterator last) {
	if (first == last) return;
	const size_t size = _MSTL distance(first, last);
	vector<int> levels = { 1 };
	int toplevel = 0;
	for (size_t i = 1; i < size - 1; ++i) {
		if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
			--toplevel;
			++levels[toplevel];
		}
		else if (levels[toplevel] != 1) {
			++toplevel;
			levels.push_back(1);
		}
		else {
			++toplevel;
			levels.push_back(0);
		}
	}
	if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
		--toplevel;
		++levels[toplevel];
	}
	else if (levels[toplevel] != 1) {
		++toplevel;
		levels.push_back(1);
	}
	else {
		++toplevel;
		levels.push_back(0);
	}
	_MSTL adjust_leonardo_heap(first, size - 1, toplevel, levels);
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void pop_leonardo_heap(Iterator first, Iterator last) {
	if (first == last) return;
	const size_t size = _MSTL distance(first, last);
	vector<int> levels = { 1 };
	int toplevel = 0;
	for (size_t i = 1; i < size; ++i) {
		if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
			--toplevel;
			++levels[toplevel];
		}
		else if (levels[toplevel] != 1) {
			++toplevel;
			levels.push_back(1);
		}
		else {
			++toplevel;
			levels.push_back(0);
		}
		_MSTL adjust_leonardo_heap(first, i, toplevel, levels);
	}
	if (levels[toplevel] <= 1) {
		--toplevel;
	}
	else {
		--levels[toplevel];
		levels.push_back(levels[toplevel] - 1);
		++toplevel;
		_MSTL adjust_leonardo_heap(first, size - 2 - leonardo(levels[toplevel]), toplevel - 1, levels);
		_MSTL adjust_leonardo_heap(first, size - 2, toplevel, levels);
	}
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void sort_leonardo_heap(Iterator first, Iterator last) {
	if (first == last) return;
	const size_t size = _MSTL distance(first, last);
	vector<int> levels = { 1 };
	int toplevel = 0;
	for (size_t i = 1; i < size; ++i) {
		if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
			--toplevel;
			++levels[toplevel];
		}
		else if (levels[toplevel] != 1) {
			++toplevel;
			levels.push_back(1);
		}
		else {
			++toplevel;
			levels.push_back(0);
		}
		_MSTL adjust_leonardo_heap(first, i, toplevel, levels);
	}
	for (size_t i = size - 2; i > 0; --i) {
		if (levels[toplevel] <= 1) {
			--toplevel;
		}
		else {
			--levels[toplevel];
			levels.push_back(levels[toplevel] - 1);
			++toplevel;

			_MSTL adjust_leonardo_heap(first, i - leonardo(levels[toplevel]), toplevel - 1, levels);
			_MSTL adjust_leonardo_heap(first, i, toplevel, levels);
		}
	}
}

template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void make_leonardo_heap(Iterator first, Iterator last) {
	if (first == last) return;
	const size_t size = _MSTL distance(first, last);
	vector<int> levels = { 1 };
	int toplevel = 0;

	for (size_t i = 1; i < size; ++i) {
		if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
			--toplevel;
			++levels[toplevel];
		}
		else if (levels[toplevel] != 1) {
			++toplevel;
			levels.push_back(1);
		}
		else {
			++toplevel;
			levels.push_back(0);
		}
		_MSTL adjust_leonardo_heap(first, i, toplevel, levels);
	}
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_LEONARDO_HEAP_HPP__
