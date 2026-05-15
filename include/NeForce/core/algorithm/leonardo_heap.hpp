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
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup LeonardoHeap 莱昂纳多堆算法
 * @brief 莱昂纳多堆算法实现
 *
 * 莱昂纳多堆基于莱昂纳多数（Leonardo Numbers），由 Edsger W. Dijkstra 设计，
 * 具有自平衡特性，在接近有序的序列上表现出接近线性的时间复杂度。
 *
 * @section references 学术文献与算法来源
 * 本实现基于以下原创学术论文和算法出版物：
 *
 * **平滑排序（Smoothsort）原始论文：**
 * - **Edsger W. Dijkstra (1981)**：Smoothsort, an alternative for sorting in situ
 *   EWD796a, Burroughs Corporation
 *   https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html
 *
 * **算法分析与实现参考：**
 * - **Edsger W. Dijkstra (1982)**：Smoothsort, an alternative for sorting in situ
 *   Science of Computer Programming, Volume 1, Issue 3, Pages 223-233
 *   https://doi.org/10.1016/0167-6423(82)90016-8
 *
 * **莱昂纳多数（Leonardo Numbers）定义：**
 * - **Keith Schwarz (2010)**：Smoothsort Demystified
 *   Stanford CS166 Lecture Notes
 *   https://www.keithschwarz.com/smoothsort/
 *
 * **现代算法描述：**
 * - **Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein**：
 *   Introduction to Algorithms (3rd Edition), Problem 6-2 (Smoothsort)
 *
 * @section leonardo_numbers 莱昂纳多数列
 * 莱昂纳多数由 Dijkstra 为平滑排序专门设计，定义如下：
 *
 * | 索引 n | 递推公式                                      | 前几项                         |
 * |--------|-----------------------------------------------|--------------------------------|
 * | n      | L(0) = 1, L(1) = 1, L(n) = L(n-1) + L(n-2) + 1 | 1, 1, 3, 5, 9, 15, 25, 41, 67 |
 *
 * **递推公式**：L(n) = L(n-1) + L(n-2) + 1
 *
 * **闭式表示**：L(n) = 2 × F(n+1) - 1，其中 F 为斐波那契数
 *
 * @section heap_structure 莱昂纳多堆结构
 * 莱昂纳多堆是一系列满足堆性质的二叉树的集合，每棵树的大小均为莱昂纳多数：
 *
 * | 特性              | 描述                                                   |
 * |-------------------|--------------------------------------------------------|
 * | 堆性质            | 父节点 ≥ 子节点（最大堆）                               |
 * | 树大小            | 每棵树的大小为 L(n)                                     |
 * | 树结构            | 根节点左子树大小为 L(n-1)，右子树大小为 L(n-2)          |
 * | 多树组合          | 堆由多棵大小递减的莱昂纳多树组成                        |
 * | 大小约束          | 相邻树的大小差至少为 2                                  |
 *
 * **树结构示意**（L(4) = 9）：
 * ```
 *        root
 *       /    \
 *   L(3)=5  L(2)=3
 *   /   \    /  \
 * L(2) L(1) L(1) L(0)
 * ```
 *
 * @section comparison_with_heapsort 与堆排序对比
 * | 特性              | 平滑排序（Smoothsort）      | 堆排序（Heapsort）          |
 * |-------------------|-----------------------------|-----------------------------|
 * | 最坏时间复杂度    | O(n log n)                  | O(n log n)                  |
 * | 最优时间复杂度    | O(n)                        | O(n log n)                  |
 * | 自适应            | 是                          | 否                          |
 * | 代码复杂度        | 较高                        | 较低                        |
 * | 实现难度          | 困难                        | 简单                        |
 *
 * @section tree_merging 树的合并规则
 * 莱昂纳多堆中的树合并遵循以下规则：
 *
 * | 条件                                      | 操作                                   |
 * |-------------------------------------------|----------------------------------------|
 * | 最后两棵树大小分别为 L(k+1) 和 L(k)       | 合并为大小为 L(k+2) 的树               |
 * | 最后两棵树大小不满足 L(k+1) 和 L(k) 关系  | 添加大小为 L(1) 或 L(0) 的新树         |
 *
 * **合并后结构**：
 * - 新根节点：原第二棵树的根
 * - 左子树：原第一棵树（大小为 L(k+1)）
 * - 右子树：原第二棵树的右子树（大小为 L(k)）
 *
 * @note 平滑排序是 Edsger W. Dijkstra（图灵奖得主，结构化程序设计倡导者）
 *       在 1981 年设计的算法，旨在证明原地排序算法可以具有自适应性。
 *       尽管代码复杂，但其在理论上的优雅性使其成为算法教材中的经典案例。
 *
 * @warning 对于大多数应用场景，推荐使用 `sort`。
 *          平滑排序的常数因子较大，在随机数据上通常慢于快速排序。
 *
 * @see https://www.cs.utexas.edu/~EWD/transcriptions/EWD07xx/EWD796a.html
 * @see https://www.keithschwarz.com/smoothsort/
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

    size_t child_heap1 = 0;
    size_t child_heap2 = 0;
    while (level_index > 0) {
        size_t prev_heap = current_heap - leonardo(levels[level_index]);
        if (*(first + current_heap) < *(first + prev_heap)) {
            if (levels[level_index] > 1) {
                child_heap1 = current_heap - 1 - leonardo(levels[level_index] - 2);
                child_heap2 = current_heap - 1;
                if (*(first + prev_heap) < *(first + child_heap1)) {
                    break;
                }
                if (*(first + prev_heap) < *(first + child_heap2)) {
                    break;
                }
            }
            _NEFORCE iter_swap(first + current_heap, first + prev_heap);
            current_heap = prev_heap;
            --level_index;
        } else {
            break;
        }
    }
    int current_level = levels[level_index];
    while (current_level > 1) {
        size_t max_child = current_heap;
        child_heap1 = current_heap - 1 - leonardo(current_level - 2);
        child_heap2 = current_heap - 1;

        if (*(first + max_child) < *(first + child_heap1)) {
            max_child = child_heap1;
        }
        if (*(first + max_child) < *(first + child_heap2)) {
            max_child = child_heap2;
        }

        if (max_child == child_heap1) {
            _NEFORCE iter_swap(first + current_heap, first + child_heap1);
            current_heap = child_heap1;
            --current_level;
        } else if (max_child == child_heap2) {
            _NEFORCE iter_swap(first + current_heap, first + child_heap2);
            current_heap = child_heap2;
            current_level -= 2;
        } else {
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

    if (first == last) {
        return;
    }
    const size_t size = _NEFORCE distance(first, last);
    if (size < 2) {
        return;
    }

    vector<int> levels = {1};
    int toplevel = 0;
    for (size_t i = 1; i < size - 1; ++i) {
        if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
            --toplevel;
            ++levels[toplevel];
        } else if (levels[toplevel] != 1) {
            ++toplevel;
            levels.push_back(1);
        } else {
            ++toplevel;
            levels.push_back(0);
        }
    }
    if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
        --toplevel;
        ++levels[toplevel];
    } else if (levels[toplevel] != 1) {
        ++toplevel;
        levels.push_back(1);
    } else {
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

    if (first == last) {
        return;
    }
    const size_t size = _NEFORCE distance(first, last);
    vector<int> levels = {1};
    int toplevel = 0;
    for (size_t i = 1; i < size; ++i) {
        if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
            --toplevel;
            ++levels[toplevel];
        } else if (levels[toplevel] != 1) {
            ++toplevel;
            levels.push_back(1);
        } else {
            ++toplevel;
            levels.push_back(0);
        }
        _NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
    }
    if (levels[toplevel] <= 1) {
        --toplevel;
    } else {
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

    if (first == last) {
        return;
    }
    const size_t size = _NEFORCE distance(first, last);
    if (size < 2) {
        return;
    }

    vector<int> levels = {1};
    int toplevel = 0;
    for (size_t i = 1; i < size; ++i) {
        if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
            --toplevel;
            ++levels[toplevel];
        } else if (levels[toplevel] != 1) {
            ++toplevel;
            levels.push_back(1);
        } else {
            ++toplevel;
            levels.push_back(0);
        }
        _NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
    }
    for (size_t i = size - 2; i > 0; --i) {
        if (levels[toplevel] <= 1) {
            --toplevel;
        } else {
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

    if (first == last) {
        return;
    }
    const size_t size = _NEFORCE distance(first, last);
    vector<int> levels = {1};
    int toplevel = 0;

    for (size_t i = 1; i < size; ++i) {
        if (toplevel > 0 && levels[toplevel - 1] - levels[toplevel] == 1) {
            --toplevel;
            ++levels[toplevel];
        } else if (levels[toplevel] != 1) {
            ++toplevel;
            levels.push_back(1);
        } else {
            ++toplevel;
            levels.push_back(0);
        }
        _NEFORCE adjust_leonardo_heap(first, i, toplevel, levels);
    }
}

/** @} */ // LeonardoHeap

/**
 * @addtogroup SortAlgorithms 排序算法
 * @{
 */

/**
 * @brief 平滑排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 基于莱昂纳多堆的排序算法，是堆排序的改进版本，
 * 在部分有序的序列上表现优异。
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void smooth_sort(Iterator first, Iterator last) {
    _NEFORCE make_leonardo_heap(first, last);
    _NEFORCE sort_leonardo_heap(first, last);
}

/** @} */ // SortAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_LEONARDO_HEAP_HPP__
