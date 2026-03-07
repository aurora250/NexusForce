#ifndef NEFORCE_CORE_ALGORITHM_LEONARDO_HEAP_HPP__
#define NEFORCE_CORE_ALGORITHM_LEONARDO_HEAP_HPP__

/**
 * @file leonardo_heap.hpp
 * @brief 莱昂纳多堆算法实现
 *
 * 此文件提供了莱昂纳多堆的实现，这是一种用于平滑排序的数据结构。
 * 莱昂纳多堆基于莱昂纳多数，具有自平衡特性。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/numeric/math.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup LeonardoHeap 莱昂纳多堆
 * @brief 莱昂纳多堆算法实现
 * @{
 */

/**
 * @brief 调整莱昂纳多堆
 * @tparam Iterator 随机访问迭代器类型
 * @param first 指向堆起始的迭代器
 * @param current_heap 当前堆的根位置
 * @param level_index 当前层级索引
 * @param levels 层级数组
 *
 * 调整指定位置的莱昂纳多堆，确保堆性质得以维护。
 * 如果父节点小于子节点，则进行交换并递归调整。
 */
template <typename Iterator>
void adjust_leonardo_heap(Iterator first, size_t current_heap, int level_index, vector<int>& levels) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

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
			_NEFORCE iter_swap(first + current_heap, first + prev_heap);
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
			_NEFORCE iter_swap(first + current_heap, first + child_heap1);
			current_heap = child_heap1;
			--current_level;
		}
		else if (max_child == child_heap2) {
			_NEFORCE iter_swap(first + current_heap, first + child_heap2);
			current_heap = child_heap2;
			current_level -= 2;
		}
		else {
			break;
		}
	}
}

/**
 * @brief 向莱昂纳多堆中推入元素
 * @tparam Iterator 随机访问迭代器类型
 * @param first 指向堆起始的迭代器
 * @param last 指向堆末尾的迭代器
 *
 * 将最后一个元素插入到莱昂纳多堆中，并调整堆以维持堆性质。
 */
template <typename Iterator>
void push_leonardo_heap(Iterator first, Iterator last) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

	if (first == last) return;
	const size_t size = _NEFORCE distance(first, last);
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
	_NEFORCE adjust_leonardo_heap(first, size - 1, toplevel, levels);
}

/**
 * @brief 从莱昂纳多堆中弹出最大元素
 * @tparam Iterator 随机访问迭代器类型
 * @param first 指向堆起始的迭代器
 * @param last 指向堆末尾的迭代器
 *
 * 移除堆顶元素，并重新调整堆。
 */
template <typename Iterator>
void pop_leonardo_heap(Iterator first, Iterator last) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

	if (first == last) return;
	const size_t size = _NEFORCE distance(first, last);
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
		_NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
	}
	if (levels[toplevel] <= 1) {
		--toplevel;
	}
	else {
		--levels[toplevel];
		levels.push_back(levels[toplevel] - 1);
		++toplevel;
		_NEFORCE adjust_leonardo_heap(first, size - 2 - leonardo(levels[toplevel]), toplevel - 1, levels);
		_NEFORCE adjust_leonardo_heap(first, size - 2, toplevel, levels);
	}
}

/**
 * @brief 使用莱昂纳多堆进行排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 指向序列起始的迭代器
 * @param last 指向序列末尾的迭代器
 *
 * 实现平滑排序算法，时间复杂度O(n log n)，在接近有序的序列上表现优异。
 */
template <typename Iterator>
void sort_leonardo_heap(Iterator first, Iterator last) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

	if (first == last) return;
	const size_t size = _NEFORCE distance(first, last);
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
		_NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
	}
	for (size_t i = size - 2; i > 0; --i) {
		if (levels[toplevel] <= 1) {
			--toplevel;
		}
		else {
			--levels[toplevel];
			levels.push_back(levels[toplevel] - 1);
			++toplevel;

			_NEFORCE adjust_leonardo_heap(first, i - leonardo(levels[toplevel]), toplevel - 1, levels);
			_NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
		}
	}
}

/**
 * @brief 构建莱昂纳多堆
 * @tparam Iterator 随机访问迭代器类型
 * @param first 指向序列起始的迭代器
 * @param last 指向序列末尾的迭代器
 *
 * 将指定范围内的元素构建成一个莱昂纳多堆。
 */
template <typename Iterator>
void make_leonardo_heap(Iterator first, Iterator last) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

	if (first == last) return;
	const size_t size = _NEFORCE distance(first, last);
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
		_NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
	}
}

/** @} */ // LeonardoHeap

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_LEONARDO_HEAP_HPP__
