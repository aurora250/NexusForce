#ifndef MSTL_CORE_ALGORITHM_SET_HPP__
#define MSTL_CORE_ALGORITHM_SET_HPP__

/**
 * @file set.hpp
 * @brief MSTL集合算法
 *
 * 此文件提供了集合算法实现，
 * 用于在已排序的序列上执行集合操作，如并集、交集、差集和对称差集。
 */

#include "shift.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup SetAlgorithms 集合算法
 * @brief MSTL集合算法的实现
 * @{
 */

/**
 * @brief 计算两个已排序范围的并集
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 计算两个已排序范围 [first1, last1) 和 [first2, last2) 的并集。
 * 结果包含所有出现在任一输入范围中的元素，重复元素只出现一次。
 *
 * 前提条件：
 * 1. 两个输入范围都已按升序排序
 * 2. 输出范围不与输入范围重叠
 */
template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_union(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			*result = *first1;
			++first1;
		} else if (*first2 < *first1) {
			*result = *first2;
			++first2;
		} else {
			*result = *first1;
			++first1; ++first2;
		}
		++result;
	}
	return _MSTL copy(first2, last2, _MSTL copy(first1, last1, result));
}

/**
 * @brief 计算两个已排序范围的交集
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 计算两个已排序范围 [first1, last1) 和 [first2, last2) 的交集。
 * 结果包含同时出现在两个输入范围中的元素。
 */
template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_intersection(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			++first1;
		} else if (*first2 < first1) {
			++first2;
		} else {
			*result = *first1;
			++first1; ++first2;
			++result;
		}
	}
	return result;
}

/**
 * @brief 计算两个已排序范围的差集
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 计算两个已排序范围 [first1, last1) 和 [first2, last2) 的差集。
 * 结果包含出现在第一个范围但不出现在第二个范围中的元素。
 */
template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_difference(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			*result = *first1;
			++first1;
			++result;
		} else if (*first2 < first1) {
			++first2;
		} else {
			++first1; ++first2;
		}
	}
	return _MSTL copy(first1, last1, result);
}

/**
 * @brief 计算两个已排序范围的对称差集
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 计算两个已排序范围 [first1, last1) 和 [first2, last2) 的对称差集。
 * 结果包含出现在任一输入范围但不同时出现在两个范围中的元素。
 * 即：并集减去交集。
 */
template <typename Iterator1, typename Iterator2, typename Iterator3,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>, int> = 0>
constexpr Iterator3 set_symmetric_difference(Iterator1 first1, Iterator1 last1,
	Iterator2 first2, Iterator2 last2, Iterator3 result) {
	while (first1 != last1 && first2 != last2) {
		if (*first1 < *first2) {
			*result = *first1;
			++first1;
			++result;
		} else if (*first2 < first1) {
			*result = *first2;
			++first1;
			++result;
		} else {
			++first1; ++first2;
		}
	}
	return _MSTL copy(first2, last2, _MSTL copy(first1, last1, result));
}

/** @} */ // SetAlgorithms

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_SET_HPP__
