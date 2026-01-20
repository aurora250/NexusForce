#ifndef MSTL_CORE_ALGORITHM_PERMUTATION_HPP__
#define MSTL_CORE_ALGORITHM_PERMUTATION_HPP__

/**
 * @file permutation.hpp
 * @brief MSTL排列算法
 *
 * 此文件提供了排列算法实现，
 * 包括排列检查、下一个排列和上一个排列的生成。
 */

#include "shift.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup PermutationAlgorithms 排列算法
 * @brief MSTL排列算法的实现
 * @{
 */

/**
 * @brief 检查两个序列是否为排列关系
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @tparam BinaryPred 二元谓词类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param pred 相等性谓词
 * @return 如果两个范围互为排列则返回true，否则返回false
 *
 * 检查两个范围是否包含相同的元素（忽略顺序），但每个元素出现的次数必须相同。
 * 算法首先检查长度是否相等，然后：
 * 1. 如果两个范围完全相同（元素顺序和值都相同），立即返回true
 * 2. 否则，对于第一个范围中的每个元素，统计在两个范围中出现的次数
 * 3. 如果任何元素的计数不同，返回false
 */
template <typename Iterator1, typename Iterator2, typename BinaryPred,
	enable_if_t<is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>, int> = 0>
constexpr bool is_permutation(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, BinaryPred pred) {
	iter_difference_t<Iterator1> len1 = _MSTL distance(first1, last1);
	iter_difference_t<Iterator2> len2 = _MSTL distance(first2, last2);
	if (len1 != len2) return false;

	for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
		if (!pred(*first1, *first2)) break;
	}
	if (first1 == last1) return true;

	for (Iterator1 i = first1; i != last1; ++i) {
		bool is_repeated = false;
		for (Iterator1 j = first1; j != i; ++j) {
			if (pred(*j, *i)) {
				is_repeated = true;
				break;
			}
		}
		if (!is_repeated) {
			size_t c2 = 0;
			for (Iterator2 j = first2; j != last2; ++j) {
				if (pred(*i, *j)) ++c2;
			}
			if (c2 == 0) return false;
			size_t c1 = 1;
			Iterator1 j = i;
			for (++j; j != last1; ++j) {
				if (pred(*i, *j)) ++c1;
			}
			if (c1 != c2) return false;
		}
	}
	return true;
}

/**
 * @brief 检查两个序列是否为排列关系
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @return 如果两个范围互为排列则返回true，否则返回false
 */
template <typename Iterator1, typename Iterator2>
constexpr bool is_permutation(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _MSTL is_permutation(first1, last1, first2, last2, _MSTL equal_to<iter_value_t<Iterator1>>());
}

/**
 * @brief 生成下一个字典序排列
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 如果存在下一个排列则返回true，否则返回false（已经是最后一个排列）
 *
 * 将范围 [first, last) 变换为下一个字典序排列。
 * 算法步骤（已知排列 P）：
 * 1. 从右向左找到第一个满足 P[i] < P[i+1] 的位置 i
 * 2. 从右向左找到第一个满足 P[i] < P[j] 的位置 j
 * 3. 交换 P[i] 和 P[j]
 * 4. 反转从 i+1 到结尾的部分
 *
 * 如果已经是最后一个排列（完全降序），则变换为第一个排列（完全升序）并返回false。
 */
template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr bool next_permutation(Iterator first, Iterator last, Compare comp) {
	if (first == last) return false;
	Iterator i = first;
	++i;
	if (i == last) return false;
	i = last;
	--i;
	for (;;) {
		Iterator ii = i;
		--i;
		if (comp(*i, *ii)) {
			Iterator j = last;
			while (!comp(*i, *--j)) {}
			_MSTL iter_swap(i, j);
			_MSTL reverse(ii, last);
			return true;
		}
		if (i == first) {
			_MSTL reverse(first, last);
			return false;
		}
	}
}

/**
 * @brief 生成下一个字典序排列
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 如果存在下一个排列则返回true，否则返回false
 */
template <typename Iterator>
constexpr bool next_permutation(Iterator first, Iterator last) {
	return _MSTL next_permutation(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/**
 * @brief 生成上一个字典序排列
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 如果存在上一个排列则返回true，否则返回false（已经是第一个排列）
 *
 * 将范围 [first, last) 变换为上一个字典序排列。
 * 算法步骤（已知排列 P）：
 * 1. 从右向左找到第一个满足 P[i] > P[i+1] 的位置 i
 * 2. 从右向左找到第一个满足 P[j] < P[i] 的位置 j
 * 3. 交换 P[i] 和 P[j]
 * 4. 反转从 i+1 到结尾的部分
 *
 * 如果已经是第一个排列（完全升序），则变换为最后一个排列（完全降序）并返回false。
 */
template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
constexpr bool prev_permutation(Iterator first, Iterator last, Compare comp) {
	if (first == last) return false;
	Iterator i = first;
	++i;
	if (i == last) return false;
	i = last;
	--i;
	for (;;) {
		Iterator ii = i;
		--i;
		if (comp(*ii, *i)) {
			Iterator j = last;
			while (!comp(*--j, *i)) {}
			_MSTL iter_swap(i, j);
			_MSTL reverse(ii, last);
			return true;
		}
		if (i == first) {
			_MSTL reverse(first, last);
			return false;
		}
	}
}

/**
 * @brief 生成上一个字典序排列
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 如果存在上一个排列则返回true，否则返回false
 */
template <typename Iterator>
constexpr bool prev_permutation(Iterator first, Iterator last) {
	return _MSTL prev_permutation(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/** @} */ // PermutationAlgorithms

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_PERMUTATION_HPP__
