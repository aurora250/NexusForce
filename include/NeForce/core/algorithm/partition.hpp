#ifndef NEFORCE_CORE_ALGORITHM_PARTITION_HPP__
#define NEFORCE_CORE_ALGORITHM_PARTITION_HPP__

/**
 * @file partition.hpp
 * @brief 分区算法
 *
 * 此文件提供了分区算法实现，
 * 用于根据谓词或基准值将序列划分为满足条件和不满足条件的两部分。
 */

#include "NeForce/core/algorithm/shift.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup PartitionAlgorithms 分区算法
 * @brief 分区算法的实现
 * @{
 */

/**
 * @brief 分区算法
 * @tparam Iterator 迭代器类型
 * @tparam Predicate 谓词类型
 * @param first 范围起始
 * @param last 范围结束
 * @param pred 一元谓词
 * @return 指向第二部分第一个元素的迭代器
 *
 * 将范围 [first, last) 重新排列，使得所有满足谓词 pred 的元素都出现在不满足谓词的元素之前。
 * 不保证相同类别的元素保持原始相对顺序。
 *
 * 算法使用双向迭代器，从两端向中间扫描：
 * 1. 从前向后找到第一个不满足谓词的元素
 * 2. 从后向前找到第一个满足谓词的元素
 * 3. 交换这两个元素
 * 4. 重复直到两个扫描指针相遇
 */
template <typename Iterator, typename Predicate>
constexpr Iterator partition(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_bid_iter_v<Iterator>, "Iterator must be bidirectional_iterator");
    static_assert(is_invocable_v<Predicate, decltype(*first)>, "Predicate must be invocable");

    while (true) {
        while (true) {
            if (first == last) {
                return first;
            }

            if (pred(*first)) {
                ++first;
            } else {
                break;
            }
        }
        --last;
        while (true) {
            if (first == last) {
                return first;
            }

            if (!pred(*last)) {
                --last;
            } else {
                break;
            }
        }
        _NEFORCE iter_swap(first, last);
        ++first;
    }
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename Iterator, typename Compare>
constexpr Iterator median_iter(Iterator a, Iterator b, Iterator c, Compare comp) {
    if (comp(*a, *b)) {
        if (comp(*b, *c)) {
            return b;
        } else if (comp(*a, *c)) {
            return c;
        } else {
            return a;
        }
    } else {
        if (comp(*a, *c)) {
            return a;
        } else if (comp(*b, *c)) {
            return c;
        } else {
            return b;
        }
    }
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief Lomuto 分区算法（基准值归位）
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 基准值最终的迭代器位置
 *
 * 选择 first、mid、last-1 的中位数作为基准值，将其交换到 first，
 * 使用 Lomuto 方案分区，最后将基准值放置到正确位置并返回。
 */
template <typename Iterator, typename Compare>
constexpr Iterator lomuto_partition(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");

    if (last - first < 2) {
        return first;
    }

    Iterator mid = first + (last - first) / 2;
    Iterator piv_iter = inner::median_iter(first, mid, last - 1, comp);
    _NEFORCE iter_swap(first, piv_iter);
    auto pivot = *first;

    Iterator i = first + 1;
    Iterator j = first;

    while (i != last) {
        if (comp(*i, pivot)) {
            ++j;
            if (i != j) {
                _NEFORCE iter_swap(i, j);
            }
        }
        ++i;
    }
    _NEFORCE iter_swap(first, j);
    return j;
}

/**
 * @brief Lomuto 分区算法（基准值归位）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 基准值最终的迭代器位置
 */
template <typename Iterator>
constexpr Iterator lomuto_partition(Iterator first, Iterator last) {
    return _NEFORCE lomuto_partition(first, last, less<>{});
}

/** @} */ // PartitionAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_PARTITION_HPP__
