#ifndef NEFORCE_CORE_ALGORITHM_SORT_HPP__
#define NEFORCE_CORE_ALGORITHM_SORT_HPP__

/**
 * @file sort.hpp
 * @brief 排序算法
 *
 * 此文件提供了标准排序算法实现，
 * 包括多种排序算法、排序检查和部分排序功能。
 */

#include "NeForce/core/algorithm/merge.hpp"
#include "NeForce/core/algorithm/heap.hpp"
#include "NeForce/core/algorithm/partition.hpp"
#include "NeForce/core/algorithm/compare.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup SortAlgorithms 排序算法
 * @brief 排序算法的实现
 * @{
 */

/**
 * @brief 检查范围是否已排序
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 如果范围已按comp排序则返回true，否则返回false
 *
 * 检查范围 [first, last) 是否按照比较函数 comp 排序。
 */
template <typename Iterator, typename Compare>
bool is_sorted(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

	if (first == last) return true;
	Iterator next = _NEFORCE next(first);
	for (; next != last; ++first, ++next) {
		if (comp(*next, *first)) {
			return false;
		}
	}
	return true;
}

/**
 * @brief 检查范围是否已排序
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 如果范围已排序则返回true，否则返回false
 */
template <typename Iterator>
bool is_sorted(Iterator first, Iterator last) {
	return is_sorted(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 查找首个破坏排序的元素
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 指向首个破坏排序的元素的迭代器，如果整个范围已排序则返回last
 *
 * 在范围 [first, last) 中查找第一个使得序列不满足排序条件的位置。
 */
template <typename Iterator, typename Compare>
Iterator is_sorted_until(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

	if (first == last) return last;
	Iterator next = _NEFORCE next(first);
	for (; next != last; ++first, ++next) {
		if (comp(*next, *first))
			return next;
	}
	return last;
}

/**
 * @brief 查找首个破坏排序的元素
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 指向首个破坏排序的元素的迭代器
 */
template <typename Iterator>
Iterator is_sorted_until(Iterator first, Iterator last) {
	return is_sorted_until(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}


/**
 * @brief 归并排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 使用分治策略的稳定排序算法：
 * 1. 递归地将序列分成两半
 * 2. 分别对两半进行排序
 * 3. 合并两个已排序的子序列
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(N)
 * 稳定性：稳定
 */
template <typename Iterator, typename Compare>
void merge_sort(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

    const auto n = _NEFORCE distance(first, last);
    if (n < 2) return;
    Iterator mid = first + n / 2;
    _NEFORCE merge_sort(first, mid);
    _NEFORCE merge_sort(mid, last);
    _NEFORCE inplace_merge(first, mid, last, comp);
}

/**
 * @brief 归并排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 */
template <typename Iterator>
void merge_sort(Iterator first, Iterator last) {
    return _NEFORCE merge_sort(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 部分排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param middle 部分排序的边界
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 对范围 [first, last) 进行部分排序，使得 [first, middle) 包含整个范围中最小的元素，
 * 并且这个子范围是已排序的。使用堆排序算法实现。
 * 适用只需要前k个最小（或最大）元素的情况。
 *
 * 时间复杂度：O(N log k)，其中k = middle - first
 */
template <typename Iterator, typename Compare>
void partial_sort(Iterator first, Iterator middle, Iterator last, Compare comp) {
    if (first == middle) return;
    _NEFORCE make_heap(first, middle, comp);
    for (Iterator i = middle; i < last; ++i) {
        if (comp(*i, *first)) {
	        inner::pop_heap_aux(first, middle, i, *i, comp);
        }
    }
    _NEFORCE sort_heap(first, middle, comp);
}

/**
 * @brief 部分排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param middle 部分排序的边界
 * @param last 范围结束
 */
template <typename Iterator>
void partial_sort(Iterator first, Iterator middle, Iterator last) {
    return _NEFORCE partial_sort(first, middle, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 部分排序并复制到另一个范围
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result_first 输出范围起始
 * @param result_last 输出范围结束
 * @param comp 比较函数对象
 * @return 输出范围的实际结束位置
 *
 * 从输入范围中选取最小的元素，部分排序后复制到输出范围。
 *
 * 时间复杂度：O(N log M)，其中M = result_last - result_first
 */
template <typename Iterator1, typename Iterator2, typename Compare>
Iterator2 partial_sort_copy(Iterator1 first, Iterator1 last, Iterator2 result_first, Iterator2 result_last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator1>, "Iterator must be input_iterator");

	if (result_first == result_last) return result_last;
	Iterator2 result_real_last = result_first;
	while (first != last && result_real_last != result_last) {
		*result_real_last = *first;
		++result_real_last;
		++first;
	}
    _NEFORCE make_heap(result_first, result_real_last, comp);
	while (first != last) {
		if (comp(*first, *result_first)) {
            _NEFORCE adjust_heap(
                result_first,
                0,
            	result_real_last - result_first,
            	*first, comp);
        }
		++first;
	}
    _NEFORCE sort_heap(result_first, result_real_last, comp);
	return result_real_last;
}

/**
 * @brief 部分排序并复制
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 随机访问迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result_first 输出范围起始
 * @param result_last 输出范围结束
 * @return 输出范围的实际结束位置
 */
template <typename Iterator1, typename Iterator2>
Iterator2 partial_sort_copy(Iterator1 first, Iterator1 last, Iterator2 result_first, Iterator2 result_last) {
    return _NEFORCE partial_sort_copy(first, result_first, result_last, _NEFORCE less<iter_value_t<Iterator1>>());
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 插入排序辅助函数
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @tparam Compare 比较函数类型
 * @param last 插入位置
 * @param value 要插入的值
 * @param comp 比较函数对象
 *
 * 在已排序的子序列 [first, last) 中找到合适位置插入新元素。
 */
template <typename Iterator, typename T, typename Compare>
void __insertion_sort_aux(Iterator last, T value, Compare comp) {
    Iterator next = last;
    --next;
    while (comp(value, *next)) {
        *last = *next;
        last = next;
        --next;
    }
    *last = value;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 插入排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 简单的稳定排序算法，适用于小规模或基本有序的数据。
 * 将每个元素插入到前面已排序子序列的正确位置。
 *
 * 时间复杂度：最优O(N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：稳定
 */
template <typename Iterator, typename Compare>
void insertion_sort(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    if (first == last) return;
    for (Iterator i = first + 1; i != last; ++i) {
        iter_value_t<Iterator> value = *i;
        if (comp(value, *first)) {
            _NEFORCE copy_backward(first, i, i + 1);
            *first = value;
        } else {
	        inner::__insertion_sort_aux(i, value, comp);
        }
    }
}

/**
 * @brief 插入排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 */
template <typename Iterator>
void insertion_sort(Iterator first, Iterator last) {
    return _NEFORCE insertion_sort(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 内省排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param depth_limit 递归深度限制
 * @param comp 比较函数对象
 *
 * 结合了快速排序、堆排序和插入排序的混合排序算法：
 * 1. 使用快速排序递归分区
 * 2. 当递归深度过大时切换到堆排序，避免快速排序的最坏情况
 * 3. 对小规模子序列使用插入排序
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(log N)
 * 稳定性：不稳定
 */
template <typename Iterator, typename Compare>
void introspective_sort(Iterator first, Iterator last, int depth_limit, Compare comp) {
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    while (first < last) {
        if (depth_limit == 0) {
            _NEFORCE partial_sort(first, last, last, comp);
            return;
        }
        --depth_limit;
        Iterator cut = _NEFORCE lomuto_partition(
            first, last, _NEFORCE median(*first, *(first + (last - first) / 2), *(last - 1), comp), comp);
        _NEFORCE introspective_sort(cut, last, depth_limit, comp);
        last = cut;
    }
}

/**
 * @brief 内省排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @param depth_limit 递归深度限制
 */
template <typename Iterator>
void introspective_sort(Iterator first, Iterator last, int depth_limit) {
    return _NEFORCE introspective_sort(first, last, depth_limit, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 快速排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 经典的分治排序算法：
 * 1. 选择基准值，这里选择最后一个元素
 * 2. 分区：将小于基准值的放在左边，大于等于的放在右边
 * 3. 递归排序左右两部分
 *
 * 时间复杂度：平均O(N log N)，最差O(N²)
 * 空间复杂度：O(log N)（递归栈）
 * 稳定性：不稳定
 *
 * @note 此实现容易在已排序数据上出现最坏情况。
 */
template <typename Iterator, typename Compare>
void quick_sort(Iterator first, Iterator last, Compare comp) {
    if (first < last) {
        Iterator pov = last - 1;
        Iterator cut = _NEFORCE lomuto_partition(first, last, *pov, comp);
        _NEFORCE iter_swap(cut, pov);
        _NEFORCE quick_sort(first, cut, comp);
        _NEFORCE quick_sort(cut + 1, last, comp);
    }
}

/**
 * @brief 快速排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 */
template <typename Iterator>
void quick_sort(Iterator first, Iterator last) {
    return _NEFORCE quick_sort(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 内省排序分发函数
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param depth_limit 递归深度限制
 * @param comp 比较函数对象
 *
 * 标准排序的内部实现，处理大规模数据的分区递归。
 */
template <typename Iterator, typename Compare>
void __intro_sort_dispatch(Iterator first, Iterator last, int depth_limit, Compare comp) {
    while (last - first > MEMORY_ALIGN_THRESHHOLD) {
        if (depth_limit == 0) {
            _NEFORCE partial_sort(first, last, last, comp);
            return;
        }
        --depth_limit;
        Iterator cut = _NEFORCE lomuto_partition(
            first, last,
            _NEFORCE median(*first, *(first + (last - first) / 2), *(last - 1), comp),
            comp);
        inner::__intro_sort_dispatch(cut, last, depth_limit, comp);
        last = cut;
    }
}

/**
 * @brief 计算以2为底的对数整数部分
 * @param x 正整数
 * @return ⌊log₂(x)⌋
 *
 * 用于计算递归深度限制。
 */
NEFORCE_CONST_FUNCTION constexpr int __log2_int(int x) noexcept {
    int k = 0;
    for (; x > 1; x >>= 1) ++k;
    return k;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 标准排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 标准排序算法，基于内省排序实现：
 * 1. 对大规模数据使用内省排序（快速排序+堆排序）
 * 2. 对阈值内的小规模数据使用插入排序
 * 3. 对大规模数据中的小尾部使用优化的插入排序
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(log N)
 * 稳定性：不稳定
 */
template <typename Iterator, typename Compare>
void sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;

    inner::__intro_sort_dispatch(first, last, inner::__log2_int(last - first) * 2, comp);
    constexpr size_t threshhold = MEMORY_ALIGN_THRESHHOLD;

    if (last - first > threshhold) {
        _NEFORCE insertion_sort(first, first + threshhold, comp);
        for (Iterator i = first + threshhold; i != last; ++i) {
	        inner::__insertion_sort_aux(i, *i, comp);
        }
    }
    else {
	    _NEFORCE insertion_sort(first, last, comp);
    }
}

/**
 * @brief 标准排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 */
template <typename Iterator>
void sort(Iterator first, Iterator last) {
    return _NEFORCE sort(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 第n个元素选择
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param nth 目标位置（0-based）
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 重新排列范围 [first, last)，使得位置 nth 的元素是排序后的正确元素，
 * 并且 nth 之前的元素都不大于它，之后的元素都不小于它。
 * 使用快速选择算法实现。
 */
template <typename Iterator, typename Compare>
void nth_element(Iterator first, Iterator nth, Iterator last, Compare comp) {
    while (last - first > 3) {
        Iterator cut = _NEFORCE lomuto_partition(
            first, last, _NEFORCE median(*first, *(first + (last - first) / 2), *(last - 1), comp), comp);
        if (cut <= nth) first = cut;
        else last = cut;
    }
    _NEFORCE insertion_sort(first, last, comp);
}

/**
 * @brief 第n个元素选择
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param nth 目标位置
 * @param last 范围结束
 */
template <typename Iterator>
void nth_element(Iterator first, Iterator nth, Iterator last) {
    return _NEFORCE nth_element(first, nth, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/** @} */ // SortAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_SORT_HPP__
