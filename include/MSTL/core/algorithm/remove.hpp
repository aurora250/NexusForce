#ifndef MSTL_CORE_ALGORITHM_REMOVE_HPP__
#define MSTL_CORE_ALGORITHM_REMOVE_HPP__

/**
 * @file remove.hpp
 * @brief MSTL删除算法
 *
 * 此文件提供了删除算法实现，
 * 用于从序列中移除满足特定条件的元素。
 */

#include "search.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup RemoveAlgorithms 删除算法
 * @brief MSTL删除算法的实现
 * @{
 */

/**
 * @brief 复制范围中不等于指定值的元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam T 值类型
 * @param first 输入范围起始迭代器
 * @param last 输入范围结束迭代器
 * @param result 输出范围起始迭代器
 * @param value 要排除的值
 * @return 输出范围结束迭代器
 *
 * 将范围 [first, last) 中不等于 value 的所有元素复制到以 result 开始的范围。
 * 原范围保持不变。
 *
 * 如果 pred(*it) 返回 true，则该元素将被排除。
 */
template <typename Iterator1, typename Iterator2, typename T,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 remove_copy(Iterator1 first, Iterator1 last, Iterator2 result, const T& value) {
	for (; first != last; ++first) {
		if (*first != value) {
			*result = *first;
			++result;
		}
	}
	return result;
}

/**
 * @brief 复制范围中不满足谓词的元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam Predicate 谓词类型
 * @param first 输入范围起始迭代器
 * @param last 输入范围结束迭代器
 * @param result 输出范围起始迭代器
 * @param pred 一元谓词
 * @return 输出范围结束迭代器
 *
 * 将范围 [first, last) 中不满足谓词 pred 的所有元素复制到以 result 开始的范围。
 * 原范围保持不变。
 */
template <typename Iterator1, typename Iterator2, typename Predicate,
	enable_if_t<is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
constexpr Iterator2 remove_copy_if(Iterator1 first, Iterator1 last, Iterator2 result, Predicate pred) {
	for (; first != last; ++first) {
		if (!pred(*first)) {
			*result = *first;
			++result;
		}
	}
	return result;
}

/**
 * @brief 移除范围中等于指定值的元素
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @param value 要移除的值
 * @return 新逻辑结束位置的迭代器
 *
 * 从范围 [first, last) 中移除所有等于 value 的元素。
 * 被移除的元素会移动到范围的末尾，但不会被物理删除。
 * 返回的迭代器指向移除操作后范围的新逻辑结束位置。
 *
 * @note 算法保持剩余元素的相对顺序
 */
template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator remove(Iterator first, Iterator last, const T& value) {
	first = _MSTL find(first, last, value);
	Iterator next = first;
	return first == last ? first : _MSTL remove_copy(++next, last, first, value);
}

/**
 * @brief 移除范围中满足谓词的元素
 * @tparam Iterator 迭代器类型
 * @tparam Predicate 谓词类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @param pred 一元谓词
 * @return 新逻辑结束位置的迭代器
 *
 * 从范围 [first, last) 中移除所有满足谓词 pred 的元素。
 * 谓词应接受元素类型的值并返回可转换为 bool 的类型。
 * 如果 pred(*it) 返回 true，则该元素将被移除。
 *
 * @note 算法保持剩余元素的相对顺序
 */
template <typename Iterator, typename Predicate, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
constexpr Iterator remove_if(Iterator first, Iterator last, Predicate pred) {
	first = _MSTL find_if(first, last, pred);
	Iterator next = first;
	return first == last ? first : _MSTL remove_copy_if(++next, last, first, pred);
}

/**
 * @brief 从容器中删除所有等于指定值的元素
 * @tparam Container 容器类型
 * @tparam U 值类型，必须与容器元素类型相同
 * @param cont 容器引用
 * @param value 要删除的值
 * @return 删除的元素数量
 *
 * 从容器的所有元素中删除所有等于 value 的元素。
 * 此操作会实际删除元素，容器大小会改变。
 *
 * @note 要求容器提供 begin(), end(), erase() 方法
 * @note 值类型 U 必须与容器的元素类型相同
 */
template <typename Container, typename U>
constexpr size_t erase(Container& cont, const U& value) {
	using T = decltype(*cont.begin());
	static_assert(
		declval<T>().operator =(declval<U>()),
		"U must be comparable to the value type of Container");

	const auto old_size = cont.size();
	const auto end = cont.end();
	auto removed = _MSTL remove_if(cont.begin(), end,
		[&value](const auto& iter) {
			return *iter == value;
		});
	cont.erase(removed, end);
	return old_size - cont.size();
}

/**
 * @brief 从容器中删除所有满足谓词的元素
 * @tparam Container 容器类型
 * @tparam Predicate 谓词类型
 * @param cont 容器引用
 * @param pred 一元谓词
 * @return 删除的元素数量
 *
 * 从容器中删除所有满足谓词 pred 的元素。
 * 谓词应接受容器元素类型的值并返回可转换为 bool 的类型。
 * 如果 pred(element) 返回 true，则该元素将被删除。
 *
 * @note 要求容器提供 begin(), end(), erase() 方法
 */
template <typename Container, typename Predicate>
constexpr size_t erase_if(Container& cont, Predicate pred) {
	const size_t old_size = cont.size();
	const auto end = cont.end();
	auto removed = _MSTL remove_if(cont.begin(), end,
		[ref_pred = _MSTL ref(pred)](const auto& iter) {
			return ref_pred(*iter);
		});
	cont.erase(removed, end);
	return old_size - cont.size();
}

/** @} */ // RemoveAlgorithms

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_REMOVE_HPP__
