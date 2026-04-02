#ifndef NEFORCE_CORE_ALGORITHM_MERGE_HPP__
#define NEFORCE_CORE_ALGORITHM_MERGE_HPP__

/**
 * @file merge.hpp
 * @brief 合并算法
 *
 * 此文件提供了合并算法实现，
 * 用于将两个已排序的序列合并成一个有序序列。
 */

#include "NeForce/core/memory/temporary_buffer.hpp"
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
constexpr Iterator3 merge(Iterator1 first1, Iterator1 last1, Iterator2 first2,
	Iterator2 last2, Iterator3 result, Compare comp) {
    static_assert(
        is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>,
        "Iterator must be forward_iterator");

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
	return _NEFORCE merge(first1, last1, first2, last2, result, _NEFORCE less<iter_value_t<Iterator1>>());
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 原地合并辅助函数（不使用缓冲区）
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param middle 范围中间分割点
 * @param last 范围结束
 * @param len1 前半部分长度
 * @param len2 后半部分长度
 * @param comp 比较函数对象
 *
 * 使用旋转和递归在原地合并两个已排序的子范围。
 */
template <typename Iterator, typename Compare>
constexpr void __merge_without_buffer_aux(
    Iterator first, Iterator middle, Iterator last,
    iter_difference_t<Iterator> len1, iter_difference_t<Iterator> len2, Compare comp) {

	if (len1 == 0 || len2 == 0) return;
	if (len1 + len2 == 2) {
		if (comp(*middle, *first)) _NEFORCE iter_swap(first, middle);
		return;
	}
	Iterator first_cut = first;
	Iterator second_cut = middle;
	iter_difference_t<Iterator> len11 = 0;
	auto len22 = len11;
	if (len1 > len2) {
		len11 = len1 / 2;
		_NEFORCE advance(first_cut, len11);
		second_cut = _NEFORCE lower_bound(middle, last, *first_cut, comp);
		len22 = _NEFORCE distance(middle, second_cut);
	} else {
		len22 = len2 / 2;
		_NEFORCE advance(second_cut, len22);
		first_cut = _NEFORCE upper_bound(first, middle, *second_cut, comp);
		len11 = _NEFORCE distance(first, first_cut);
	}
	_NEFORCE rotate(first_cut, middle, second_cut);
	Iterator new_middle = first_cut;
	_NEFORCE advance(new_middle, len22);
	inner::__merge_without_buffer_aux(first, first_cut, new_middle, len11, len22, comp);
	inner::__merge_without_buffer_aux(new_middle, second_cut, last, len1 - len11, len2 - len22, comp);
}

/**
 * @brief 带缓冲区的旋转辅助函数
 * @tparam Iterator1 主迭代器类型
 * @tparam Iterator2 缓冲区迭代器类型
 * @param first 范围起始
 * @param middle 旋转中心
 * @param last 范围结束
 * @param len1 前半部分长度
 * @param len2 后半部分长度
 * @param buffer 缓冲区起始
 * @param buffer_size 缓冲区大小
 * @return 旋转后新的中间位置
 *
 * 使用缓冲区优化旋转操作，选择最小的部分放入缓冲区。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator1 __rotate_with_buffer_aux(
    Iterator1 first, Iterator1 middle, Iterator1 last,
	iter_difference_t<Iterator1> len1, iter_difference_t<Iterator1> len2,
	Iterator2 buffer, iter_difference_t<Iterator2> buffer_size) {

	Iterator2 buffer_end;
	if (len1 > len2 && len2 <= buffer_size) {
		buffer_end = _NEFORCE copy(middle, last, buffer);
		_NEFORCE copy_backward(first, middle, last);
		return _NEFORCE copy(buffer, buffer_end, first);
	}
	if (len1 <= buffer_size) {
		buffer_end = _NEFORCE copy(first, middle, buffer);
		_NEFORCE copy(middle, last, first);
		return _NEFORCE copy_backward(buffer, buffer_end, last);
	}
	_NEFORCE rotate(first, middle, last);
	_NEFORCE advance(first, len2);
	return first;
}

/**
 * @brief 原地合并辅助函数（使用缓冲区）
 * @tparam Iterator 主迭代器类型
 * @tparam Pointer 缓冲区指针类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param middle 范围中间分割点
 * @param last 范围结束
 * @param len1 前半部分长度
 * @param len2 后半部分长度
 * @param buffer 缓冲区指针
 * @param buffer_size 缓冲区大小
 * @param comp 比较函数对象
 *
 * 使用临时缓冲区优化原地合并算法。
 */
template <typename Iterator, typename Pointer, typename Compare>
constexpr void __merge_with_buffer_aux(
    Iterator first, Iterator middle, Iterator last,
	iter_difference_t<Iterator> len1, iter_difference_t<Iterator> len2,
	Pointer buffer, iter_difference_t<Iterator> buffer_size, Compare comp) {

	if (len1 <= len2 && len1 <= buffer_size) {
		Pointer end_buffer = _NEFORCE copy(first, middle, buffer);
		_NEFORCE merge(buffer, end_buffer, middle, last, first, comp);
	}
	else if (len2 <= buffer_size) {
		Pointer end_buffer = _NEFORCE copy(middle, last, buffer);
		if (first == middle) {
			_NEFORCE copy_backward(buffer, end_buffer, last);
			return;
		}
		if (buffer == end_buffer) {
			_NEFORCE copy_backward(first, middle, last);
			return;
		}
		--middle;
		--end_buffer;
		while (true) {
			if (comp(*end_buffer, *middle)) {
				*--last = *middle;
				if (first == middle) {
					_NEFORCE copy_backward(buffer, ++end_buffer, last);
					return;
				}
				--middle;
			}
			else {
				*--last = *end_buffer;
				if (buffer == end_buffer) {
					_NEFORCE copy_backward(first, ++middle, last);
					return;
				}
				--end_buffer;
			}
		}
	}
	else {
		Iterator first_cut = first;
		Iterator second_cut = middle;
		iter_difference_t<Iterator> len11 = 0;
		auto len22 = len11;
		if (len1 > len2) {
			len11 = len1 / 2;
			_NEFORCE advance(first_cut, len11);
			second_cut = _NEFORCE lower_bound(middle, last, *first_cut, comp);
			len22 = _NEFORCE distance(middle, second_cut);
		} else {
			len22 = len2 / 2;
			_NEFORCE advance(second_cut, len22);
			first_cut = _NEFORCE upper_bound(first, middle, *second_cut, comp);
			len11 = _NEFORCE distance(first, first_cut);
		}
		Iterator new_middle = inner::__rotate_with_buffer_aux(
			first_cut, middle, second_cut, len1 - len11, len22, buffer, buffer_size);

		inner::__merge_with_buffer_aux(
			first, first_cut, new_middle, len11, len22, buffer, buffer_size, comp);
		inner::__merge_with_buffer_aux(
			new_middle, second_cut, last, len1 - len11, len2 - len22, buffer, buffer_size, comp);
	}
}

NEFORCE_END_INNER__
/// @endcond

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
 * 算法尝试分配临时缓冲区以提高性能，如果分配失败则使用无缓冲区的算法。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void inplace_merge(Iterator first, Iterator middle, Iterator last, Compare comp) {
    static_assert(is_ranges_bid_iter_v<Iterator>, "Iterator must be a bidirectional_iterator");

	if (first == middle || middle == last) return;
	auto len1 = _NEFORCE distance(first, middle);
	auto len2 = _NEFORCE distance(middle, last);
	try {
		temporary_buffer<Iterator> buffer(first, last);
		inner::__merge_with_buffer_aux(first, middle, last, len1, len2, buffer.begin(), buffer.size(), comp);
	} catch (...) {
		inner::__merge_without_buffer_aux(first, middle, last, len1, len2, comp);
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
	return _NEFORCE inplace_merge(first, middle, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/** @} */ // MergeAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_MERGE_HPP__
