#ifndef NEFORCE_CORE_ALGORITHM_SORT_HPP__
#define NEFORCE_CORE_ALGORITHM_SORT_HPP__

/**
 * @file sort.hpp
 * @brief 排序算法
 *
 * 此文件提供了标准排序算法实现，
 * 包括多种排序算法、排序检查和部分排序功能。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/algorithm/heap.hpp"
#include "NeForce/core/algorithm/merge.hpp"
#include "NeForce/core/algorithm/partition.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup SortAlgorithms 排序算法
 * @brief 排序算法的实现
 *
 * 本模块提供了多种排序算法的完整实现，涵盖基础排序、高级排序、线性时间排序以及
 * 具有特殊用途或教学价值的排序算法。
 *
 * @section standards 遵循的国际标准与学术参考
 * 本模块中的算法实现参考以下学术文献：
 *
 * **经典排序算法学术文献：**
 * - **C.A.R. Hoare (1961)**：Algorithm 64 — Quicksort
 *   Communications of the ACM, 4(7): 321
 * - **Donald E. Knuth (1998)**：The Art of Computer Programming, Volume 3 — Sorting and Searching
 *   ISBN: 978-0201896855
 * - **Robert Sedgewick (1978)**：Implementing Quicksort programs
 *   Communications of the ACM, 21(10): 847-857
 *
 * **现代混合排序算法文献：**
 * - **David R. Musser (1997)**：Introspective Sorting and Selection Algorithms
 *   Software: Practice and Experience, 27(8): 983-993
 *   https://doi.org/10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-%23
 * - **Tim Peters (2002)**：Timsort — Python list sort
 *   https://svn.python.org/projects/python/trunk/Objects/listsort.txt
 *
 * **平滑排序学术文献：**
 * - **Edsger W. Dijkstra (1981)**：Smoothsort, an alternative for sorting in situ
 *   EWD796a, Burroughs Corporation
 *   https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html
 *
 * @section algorithm_classification 算法分类
 * | 类别               | 算法                                   | 时间复杂度（平均） | 空间复杂度 | 稳定性 |
 * |--------------------|----------------------------------------|--------------------|------------|--------|
 * | 基础排序           | bubble_sort, cocktail_sort, select_sort | O(N²)              | O(1)       | 部分稳定 |
 * | 插入排序           | insertion_sort, shell_sort             | O(N²) / O(N log N) | O(1)       | 稳定/不稳定 |
 * | 分治排序           | merge_sort, quick_sort                 | O(N log N)         | O(N) / O(log N) | 稳定/不稳定 |
 * | 混合排序           | introspective_sort, tim_sort, smooth_sort | O(N log N)      | O(log N) / O(N) | 不稳定/稳定 |
 * | 部分排序           | partial_sort, nth_element              | O(N log k) / O(N)  | O(1)       | 不稳定 |
 * | 娱乐/教学排序      | monkey_sort                            | O((N+1)!)          | O(1)       | 不稳定 |
 *
 * @section hybrid_algorithms 混合排序算法详解
 * **内省排序（Introspective Sort）**：
 * - 结合快速排序、堆排序和插入排序
 * - 默认递归深度限制：2 × ⌊log₂(N)⌋
 * - 超过深度限制时切换到堆排序，避免 O(N²) 最坏情况
 * - 小规模子序列（≤阈值）使用插入排序
 *
 * **TimSort**：
 * - 结合归并排序和插入排序
 * - 识别并利用数据中的自然有序片段（run）
 * - 最小归并片段大小（minrun）：32
 * - Python 和 Java 默认排序算法
 *
 * **平滑排序（Smoothsort）**：
 * - 基于莱昂纳多堆（Leonardo Heap）
 * - 在接近有序的序列上达到 O(N) 时间复杂度
 * - 由 Edsger W. Dijkstra 设计
 *
 * @section partial_sorting 部分排序
 * **partial_sort**：
 * - 找出前 k 个最小（或最大）元素并排序
 * - 使用堆排序实现，时间复杂度 O(N log k)
 * - 适用于"Top K"问题
 *
 * **nth_element**：
 * - 找出第 n 个顺序统计量
 * - 使用快速选择算法，平均时间复杂度 O(N)
 * - nth 之前元素 ≤ nth ≤ nth 之后元素
 *
 * @section stability_notes 稳定性说明
 * | 算法              | 稳定性 | 说明                                           |
 * |-------------------|--------|------------------------------------------------|
 * | bubble_sort       | 稳定   | 相邻元素相等时不交换                           |
 * | cocktail_sort     | 稳定   | 双向冒泡，保持相等元素相对顺序                 |
 * | select_sort       | 不稳定 | 交换可能破坏相等元素的相对顺序                 |
 * | insertion_sort    | 稳定   | 相等元素不移动                                 |
 * | merge_sort        | 稳定   | 合并时优先取左侧元素                           |
 * | quick_sort        | 不稳定 | 分区交换破坏相对顺序                           |
 * | shell_sort        | 不稳定 | 间隔插入排序破坏顺序                           |
 * | heap_sort         | 不稳定 | 堆操作不保持相对顺序                           |
 * | introspective_sort| 不稳定 | 结合快速排序和堆排序，均不稳定                 |
 * | tim_sort          | 稳定   | 插入排序和归并排序均稳定                       |
 * | smooth_sort       | 不稳定 | 基于堆结构，不保持相对顺序                     |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | 迭代器要求        | 各算法要求不同，见各函数文档              |
 * | 比较器要求        | 严格弱序（Strict Weak Ordering）          |
 * | 小序列优化        | 默认阈值 MEMORY_ALIGN_THRESHHOLD          |
 * | 递归深度限制      | 2 × ⌊log₂(N)⌋（内省排序）                 |
 *
 * @note 对于大多数应用场景，推荐使用标准排序 `sort`（内省排序），
 *       它在平均和最坏情况下均表现良好，且经过了充分测试。
 *
 * @see https://en.cppreference.com/w/cpp/algorithm/sort
 * @see https://en.wikipedia.org/wiki/Sorting_algorithm
 * @see https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html
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

    if (first == last) {
        return true;
    }
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
    return is_sorted(first, last, _NEFORCE less<>());
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

    if (first == last) {
        return last;
    }
    Iterator next = _NEFORCE next(first);
    for (; next != last; ++first, ++next) {
        if (comp(*next, *first)) {
            return next;
        }
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
    return is_sorted_until(first, last, _NEFORCE less<>());
}

/**
 * @brief 冒泡排序
 * @tparam Iterator 双向迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O(N²)，最优O(N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：稳定
 *
 * 通过重复交换相邻的逆序元素将最大元素冒泡到末尾。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void bubble_sort(Iterator first, Iterator last, Compare comp) {
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    if (first == last) {
        return;
    }
    Iterator end = last;
    --end;
    auto revend = _NEFORCE make_reverse_iterator(first);
    auto revstart = _NEFORCE make_reverse_iterator(end);
    for (auto iter = revstart; iter != revend; ++iter) {
        bool not_finished = false;
        Iterator curend = iter.base();
        for (Iterator it = first; it != curend; ++it) {
            Iterator next = it;
            ++next;
            if (comp(*next, *it)) {
                _NEFORCE iter_swap(it, next);
                not_finished = true;
            }
        }
        if (!not_finished) {
            break;
        }
    }
}

/**
 * @brief 冒泡排序（默认升序）
 * @tparam Iterator 双向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void bubble_sort(Iterator first, Iterator last) {
    return _NEFORCE bubble_sort(first, last, _NEFORCE less<>());
}

/**
 * @brief 鸡尾酒排序（双向冒泡排序）
 * @tparam Iterator 双向迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O(N²)，最优O(N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：稳定
 *
 * 冒泡排序的改进版本，同时从两端进行冒泡，减少循环次数。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void cocktail_sort(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_bid_iter_v<Iterator>, "Iterator must be bidirectional_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    if (first == last) {
        return;
    }
    bool swapped = true;
    Iterator left = first;
    Iterator right = last;
    --right;
    while (swapped) {
        swapped = false;
        for (Iterator i = left; i != right; ++i) {
            Iterator next = i;
            ++next;
            if (comp(*next, *i)) {
                _NEFORCE iter_swap(i, next);
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
        --right;
        swapped = false;
        for (Iterator i = right; i != left; --i) {
            Iterator prev = i;
            --prev;
            if (comp(*i, *prev)) {
                _NEFORCE iter_swap(i, prev);
                swapped = true;
            }
        }
        ++left;
    }
}

/**
 * @brief 鸡尾酒排序（默认升序）
 * @tparam Iterator 双向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void cocktail_sort(Iterator first, Iterator last) {
    return _NEFORCE cocktail_sort(first, last, _NEFORCE less<>());
}

/**
 * @brief 选择排序
 * @tparam Iterator 前向迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：O(N²)
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 每次从未排序部分选择元素放到已排序部分的末尾。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void select_sort(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    if (first == last) {
        return;
    }
    for (Iterator i = first; i != last; ++i) {
        Iterator min = i;
        Iterator j = i;
        ++j;
        for (; j != last; ++j) {
            if (comp(*j, *min)) {
                min = j;
            }
        }
        _NEFORCE iter_swap(i, min);
    }
}

/**
 * @brief 选择排序（默认升序）
 * @tparam Iterator 前向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void select_sort(Iterator first, Iterator last) {
    return _NEFORCE select_sort(first, last, _NEFORCE less<>());
}

/**
 * @brief 希尔排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O(N log N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 插入排序的改进版本，通过比较相距一定间隔的元素来工作。
 */
template <typename Iterator, typename Compare, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
NEFORCE_CONSTEXPR20 void shell_sort(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    if (first == last) {
        return;
    }
    auto dist = _NEFORCE distance(first, last);
    for (auto gap = dist / 2; gap > 0; gap /= 2) {
        for (Iterator i = first + gap; i < last; ++i) {
            iter_value_t<Iterator> temp = *i;
            Iterator j;
            for (j = i; j >= first + gap && comp(temp, *(j - gap)); j -= gap) {
                *j = *(j - gap);
            }
            *j = _NEFORCE move(temp);
        }
    }
}

/**
 * @brief 希尔排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void shell_sort(Iterator first, Iterator last) {
    return _NEFORCE shell_sort(first, last, _NEFORCE less<>());
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
    if (n < 2) {
        return;
    }
    Iterator mid = first + n / 2;
    _NEFORCE merge_sort(first, mid, comp);
    _NEFORCE merge_sort(mid, last, comp);
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
    return _NEFORCE merge_sort(first, last, _NEFORCE less<>());
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
    if (first == middle) {
        return;
    }
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
    return _NEFORCE partial_sort(first, middle, last, _NEFORCE less<>());
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
Iterator2 partial_sort_copy(Iterator1 first, Iterator1 last, Iterator2 result_first, Iterator2 result_last,
                            Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator1>, "Iterator must be input_iterator");

    if (result_first == result_last) {
        return result_last;
    }
    Iterator2 result_real_last = result_first;
    while (first != last && result_real_last != result_last) {
        *result_real_last = *first;
        ++result_real_last;
        ++first;
    }
    _NEFORCE make_heap(result_first, result_real_last, comp);
    while (first != last) {
        if (comp(*first, *result_first)) {
            _NEFORCE adjust_heap(result_first, 0, result_real_last - result_first, *first, comp);
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
    return _NEFORCE partial_sort_copy(first, last, result_first, result_last, _NEFORCE less<>());
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

    if (first == last) {
        return;
    }
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
    return _NEFORCE insertion_sort(first, last, _NEFORCE less<>());
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

        Iterator cut = _NEFORCE lomuto_partition(first, last, comp);
        if (cut - first < last - cut) {
            _NEFORCE introspective_sort(first, cut, depth_limit, comp);
            first = cut + 1;
        } else {
            _NEFORCE introspective_sort(cut + 1, last, depth_limit, comp);
            last = cut;
        }
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
    return _NEFORCE introspective_sort(first, last, depth_limit, _NEFORCE less<>());
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
        Iterator cut = _NEFORCE lomuto_partition(first, last, comp);
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
    return _NEFORCE quick_sort(first, last, _NEFORCE less<>());
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

        Iterator cut = _NEFORCE lomuto_partition(first, last, comp);

        if (cut - first < last - cut) {
            inner::__intro_sort_dispatch(first, cut, depth_limit, comp);
            first = cut + 1;
        } else {
            inner::__intro_sort_dispatch(cut + 1, last, depth_limit, comp);
            last = cut;
        }
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
    for (; x > 1; x >>= 1) {
        ++k;
    }
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
    if (first == last) {
        return;
    }

    inner::__intro_sort_dispatch(first, last, inner::__log2_int(last - first) * 2, comp);
    constexpr size_t threshhold = MEMORY_ALIGN_THRESHHOLD;

    if (last - first > threshhold) {
        _NEFORCE insertion_sort(first, first + threshhold, comp);
        for (Iterator i = first + threshhold; i != last; ++i) {
            inner::__insertion_sort_aux(i, *i, comp);
        }
    } else {
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
    return _NEFORCE sort(first, last, _NEFORCE less<>());
}

/**
 * @brief Tim排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(N)
 * 稳定性：稳定
 *
 * 混合排序算法，结合了归并排序和插入排序。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void tim_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) {
        return;
    }

    constexpr int min_merge = 32;
    const auto n = _NEFORCE distance(first, last);

    if (n > 0) {
        Iterator i = first;
        while (i < last) {
            auto step = _NEFORCE min(static_cast<decltype(n)>(min_merge), last - i);
            Iterator end = _NEFORCE next(i, step);
            _NEFORCE insertion_sort(i, end, comp);
            i = end;
        }
    }

    for (int size = min_merge; size < n; size *= 2) {
        for (Iterator left = first; left < last;) {
            const auto remaining = _NEFORCE distance(left, last);
            if (remaining <= size) {
                break;
            }

            Iterator mid = _NEFORCE next(left, size);
            Iterator right = _NEFORCE next(left, _NEFORCE min(2 * size, static_cast<int>(remaining)));

            if (mid < right) {
                _NEFORCE inplace_merge(left, mid, right, comp);
            }
            left = right;
        }
    }
}

/**
 * @brief Tim排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void tim_sort(Iterator first, Iterator last) {
    return _NEFORCE tim_sort(first, last, _NEFORCE less<>());
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
        Iterator cut = _NEFORCE lomuto_partition(first, last, comp);
        if (cut <= nth) {
            first = cut + 1;
        } else {
            last = cut;
        }
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
    return _NEFORCE nth_element(first, nth, last, _NEFORCE less<>());
}

/** @} */ // SortAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_SORT_HPP__
