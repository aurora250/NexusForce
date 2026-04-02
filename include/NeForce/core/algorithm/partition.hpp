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

/**
 * @brief Lomuto分区算法
 * @tparam Iterator 随机访问迭代器类型
 * @tparam T 基准值类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param pivot 基准值
 * @param comp 比较函数对象
 * @return 指向基准值最终位置的迭代器
 *
 * Lomuto分区算法，常用于快速排序。
 * 将范围重新排列，使得所有小于基准值的元素出现在基准值之前或等于基准值的元素之前，
 * 所有大于基准值的元素出现在基准值之后。
 *
 * 算法步骤：
 * 1. 从两端向中间扫描
 * 2. 从左找到第一个不小于基准值的元素
 * 3. 从右找到第一个不大于基准值的元素
 * 4. 交换这两个元素
 * 5. 重复直到指针相遇
 */
template <typename Iterator, typename T, typename Compare>
constexpr Iterator lomuto_partition(Iterator first, Iterator last, const T& pivot, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    while (first < last) {
        while (comp(*first, pivot)) {
            ++first;
        }
        --last;
        while (comp(pivot, *last)) {
            --last;
        }
        if (!(first < last)) {
            break;
        }
        _NEFORCE iter_swap(first, last);
        ++first;
    }
    return first;
}

/**
 * @brief Lomuto分区算法
 * @tparam Iterator 随机访问迭代器类型
 * @tparam T 基准值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param pivot 基准值
 * @return 指向第一部分结束位置的迭代器
 */
template <typename Iterator, typename T>
constexpr Iterator lomuto_partition(Iterator first, Iterator last, const T& pivot) {
    return _NEFORCE lomuto_partition(first, last, pivot);
}

/** @} */ // PartitionAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_PARTITION_HPP__
