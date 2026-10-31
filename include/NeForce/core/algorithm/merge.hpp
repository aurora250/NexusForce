#ifndef NEFORCE_CORE_ALGORITHM_MERGE_HPP__
#define NEFORCE_CORE_ALGORITHM_MERGE_HPP__

/**
 * @file merge.hpp
 * @brief 合并算法
 *
 * 此文件提供了合并算法实现，
 * 用于将两个已排序的序列合并成一个有序序列。
 */

#include "NeForce/core/algorithm/shift.hpp"
#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup MergeAlgorithms 合并算法
 * @brief 合并算法的实现
 * @{
 */

/**
 * @brief 合并两个已排序序列
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @tparam Compare 比较函数类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param result 输出范围起始
 * @param comp 比较函数对象
 * @return 输出范围结束迭代器
 *
 * 将两个已排序的范围 [first1, last1) 和 [first2, last2) 合并到以 result 开始的范围。
 * 结果范围包含来自两个输入范围的所有元素，并保持排序顺序。
 *
 * 前提条件：
 * 1. 两个输入范围都已按照 comp 排序
 * 2. 输出范围不与任一输入范围重叠
 * 3. 输出范围有足够的空间容纳所有元素
 */
template <typename Iterator1, typename Iterator2, typename Iterator3, typename Compare>
constexpr Iterator3 merge(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Iterator3 result,
                          Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2> &&
                          is_ranges_input_iter_v<Iterator3>,
                  "Iterator must be input_iterator");

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
    return _NEFORCE copy(first2, last2, _NEFORCE copy(first1, last1, result));
}

/**
 * @brief 合并两个已排序序列
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 */
template <typename Iterator1, typename Iterator2, typename Iterator3>
constexpr Iterator3 merge(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Iterator3 result) {
    return _NEFORCE merge(first1, last1, first2, last2, result, _NEFORCE less<>());
}

/**
 * @brief 原地合并两个已排序的连续范围
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param middle 范围中间分割点
 * @param last 范围结束
 * @param comp 比较函数对象
 *
 * 将两个已排序的连续子范围 [first, middle) 和 [middle, last) 原地合并，
 * 使得整个范围 [first, last) 成为有序的。
 *
 * 使用临时缓冲区逐个移动元素完成稳定归并，空间复杂度 O(N)。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void inplace_merge(Iterator first, Iterator middle, Iterator last, Compare comp) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be a forward_iterator");

    if (first == middle || middle == last) {
        return;
    }

    const auto total = _NEFORCE distance(first, last);
    vector<iter_value_t<Iterator>> buffer;
    buffer.reserve(static_cast<size_t>(total));

    Iterator left = first;
    Iterator right = middle;
    while (left != middle && right != last) {
        if (comp(*right, *left)) {
            buffer.push_back(_NEFORCE move(*right));
            ++right;
        } else {
            buffer.push_back(_NEFORCE move(*left));
            ++left;
        }
    }
    while (left != middle) {
        buffer.push_back(_NEFORCE move(*left));
        ++left;
    }
    while (right != last) {
        buffer.push_back(_NEFORCE move(*right));
        ++right;
    }

    Iterator output = first;
    for (auto& value: buffer) {
        *output = _NEFORCE move(value);
        ++output;
    }
}

/**
 * @brief 原地合并两个已排序的连续范围
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param middle 范围中间分割点
 * @param last 范围结束
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 void inplace_merge(Iterator first, Iterator middle, Iterator last) {
    return _NEFORCE inplace_merge(first, middle, last, _NEFORCE less<>());
}

/** @} */ // MergeAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_MERGE_HPP__
