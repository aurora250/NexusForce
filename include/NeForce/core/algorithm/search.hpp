#ifndef NEFORCE_CORE_ALGORITHM_SEARCH_HPP__
#define NEFORCE_CORE_ALGORITHM_SEARCH_HPP__

/**
 * @file search.hpp
 * @brief 查找和搜索算法
 *
 * 此文件提供了各种查找和搜索算法的实现，
 * 包括区间查询、元素查找、模式匹配等常用算法。
 */

#include "NeForce/core/iterator/reverse_iterator.hpp"
#include "NeForce/core/functional/functor.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup BoundAlgorithms 边界查找算法
 * @brief 在有序范围内查找边界的二分查找算法
 * @{
 */

/**
 * @brief 查找有序范围中第一个不小于指定值的元素位置
 * @tparam Iterator 迭代器类型，需要满足前向迭代器要求
 * @tparam T 查找值的类型
 * @tparam Compare 比较函数对象类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @param comp 比较函数对象
 * @return 指向第一个不小于value的元素的迭代器，或last如果未找到
 *
 * 在有序范围[first, last)中执行二分查找，返回第一个满足!comp(*it, value)的
 * 元素位置。要求范围已按照comp排序。
 */
template <typename Iterator, typename T, typename Compare>
constexpr Iterator lower_bound(Iterator first, Iterator last, const T& value, Compare comp) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

	using Distance = iter_difference_t<Iterator>;
	Distance len = _NEFORCE distance(first, last);
	Distance half;
	Iterator middle;
	while (len > 0) {
		half = len >> 1;
		middle = first;
		_NEFORCE advance(middle, half);
		if (comp(*middle, value)) {
			first = middle;
			++first;
			len = len - half - 1;
		}
		else len = half;
	}
	return first;
}

/**
 * @brief lower_bound的默认比较版本
 * @tparam Iterator 迭代器类型
 * @tparam T 查找值的类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @return 指向第一个不小于value的元素的迭代器，或last如果未找到
 *
 * 使用默认的less比较器执行lower_bound查找。
 */
template <typename Iterator, typename T>
constexpr Iterator lower_bound(Iterator first, Iterator last, const T& value) {
	return _NEFORCE lower_bound(first, last, value, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 查找有序范围中第一个大于指定值的元素位置
 * @tparam Iterator 迭代器类型，需要满足前向迭代器要求
 * @tparam T 查找值的类型
 * @tparam Compare 比较函数对象类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @param comp 比较函数对象
 * @return 指向第一个大于value的元素的迭代器，或last如果未找到
 *
 * 在有序范围[first, last)中执行二分查找，返回第一个满足comp(value, *it)的
 * 元素位置。要求范围已按照comp排序。
 */
template <typename Iterator, typename T, typename Compare>
constexpr Iterator upper_bound(Iterator first, Iterator last, const T& value, Compare comp) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

	using Distance = iter_difference_t<Iterator>;
	Distance len = _NEFORCE distance(first, last);
	Distance half;
	Iterator middle;
	while (len > 0) {
		half = len >> 1;
		middle = first;
		_NEFORCE advance(middle, half);
		if (comp(value, *middle)) {
			first = middle;
			++first;
			len = len - half - 1;
		}
		else len = half;
	}
	return first;
}

/**
 * @brief upper_bound的默认比较版本
 * @tparam Iterator 迭代器类型
 * @tparam T 查找值的类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @return 指向第一个大于value的元素的迭代器，或last如果未找到
 *
 * 使用默认的greater比较器执行upper_bound查找。
 */
template <typename Iterator, typename T>
constexpr Iterator upper_bound(Iterator first, Iterator last, const T& value) {
	return _NEFORCE upper_bound(first, last, value, _NEFORCE greater<iter_value_t<Iterator>>());
}

/**
 * @brief 在有序范围内进行二分查找
 * @tparam Iterator 迭代器类型，需要满足前向迭代器要求
 * @tparam T 查找值的类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @return 如果找到value则返回true，否则返回false
 */
template <typename Iterator, typename T>
constexpr bool binary_search(Iterator first, Iterator last, const T& value) {
	Iterator i = _NEFORCE lower_bound(first, last, value);
	return i != last && !(value < *i);
}

/**
 * @brief binary_search的谓词版本
 * @tparam Iterator 迭代器类型，需要满足前向迭代器要求
 * @tparam T 查找值的类型
 * @tparam Compare 比较函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @param comp 比较函数
 * @return 如果找到value则返回true，否则返回false
 */
template <typename Iterator, typename T, typename Compare>
constexpr bool binary_search(Iterator first, Iterator last, const T& value, Compare comp) {
	Iterator i = _NEFORCE lower_bound(first, last, value, comp);
	return i != last && !comp(value, *i);
}

/**
 * @brief 检查一个有序范围是否包含另一个有序范围的所有元素
 * @tparam Iterator1 主序列迭代器类型，需要满足输入迭代器要求
 * @tparam Iterator2 子序列迭代器类型，需要满足输入迭代器要求
 * @tparam Compare 比较函数类型
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 子序列起始迭代器
 * @param last2 子序列终止迭代器
 * @param comp 比较函数
 * @return 如果主序列包含子序列的所有元素则返回true，否则返回false
 */
template <typename Iterator1, typename Iterator2, typename Compare>
constexpr bool includes(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, "Iterator must be input_iterator");

	while (first1 != last1 && first2 != last2) {
		if (comp(*first2, *first1)) return false;

		if (comp(*first1, *first2)) ++first1;
		else ++first1, ++first2;
	}
	return first2 == last2;
}

/**
 * @brief includes的默认比较版本
 * @tparam Iterator1 主序列迭代器类型
 * @tparam Iterator2 子序列迭代器类型
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 子序列起始迭代器
 * @param last2 子序列终止迭代器
 * @return 如果主序列包含子序列的所有元素则返回true，否则返回false
 */
template <typename Iterator1, typename Iterator2>
constexpr bool includes(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _NEFORCE includes(first1, last1, first2, last2, _NEFORCE less<iter_value_t<Iterator1>>());
}

/** @} */ // BoundAlgorithms

/**
 * @defgroup QuantifierAlgorithms 量词算法
 * @brief 检查范围内元素是否满足特定条件的算法
 * @{
 */

/**
 * @brief 检查所有元素是否都满足谓词
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam Predicate 谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param pred 谓词函数
 * @return 如果所有元素都满足谓词则返回true，否则返回false
 */
template <typename Iterator, typename Predicate>
constexpr bool all_of(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	for (; first != last; ++first) {
		if (!pred(*first)) return false;
	}
	return true;
}

/**
 * @brief 检查是否有任意元素满足谓词
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam Predicate 谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param pred 谓词函数
 * @return 如果有任意元素满足谓词则返回true，否则返回false
 */
template <typename Iterator, typename Predicate>
constexpr bool any_of(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	for (; first != last; ++first) {
		if (pred(*first)) return true;
	}
	return false;
}

/**
 * @brief 检查是否没有元素满足谓词
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam Predicate 谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param pred 谓词函数
 * @return 如果没有元素满足谓词则返回true，否则返回false
 */
template <typename Iterator, typename Predicate>
constexpr bool none_of(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	for (; first != last; ++first) {
		if (pred(*first)) return false;
	}
	return true;
}

/** @} */ // QuantifierAlgorithms

/**
 * @defgroup AdjacentAlgorithms 相邻元素算法
 * @brief 处理相邻元素的算法
 * @{
 */

/**
 * @brief 查找第一对满足条件的相邻元素
 * @tparam Iterator 迭代器类型
 * @tparam BinaryPredicate 二元谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param binary_pred 二元谓词函数
 * @return 指向第一对相邻元素中第一个元素的迭代器，或last如果未找到
 */
template <typename Iterator, typename BinaryPredicate>
constexpr Iterator adjacent_find(Iterator first, Iterator last, BinaryPredicate binary_pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	if (first == last) return last;
	Iterator next = first;
	while (++next != last) {
		if (binary_pred(*first, *next)) return first;
		first = next;
	}
	return last;
}

/**
 * @brief adjacent_find的默认比较版本
 * @tparam Iterator 迭代器类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @return 指向第一对相等相邻元素中第一个元素的迭代器，或last如果未找到
 */
template <typename Iterator>
constexpr Iterator adjacent_find(Iterator first, Iterator last) {
	return _NEFORCE adjacent_find(first, last, _NEFORCE equal_to<iter_value_t<Iterator>>());
}

/** @} */ // AdjacentAlgorithms

/**
 * @defgroup CountingAlgorithms 计数算法
 * @brief 统计元素数量的算法
 * @{
 */

/**
 * @brief 统计范围内满足二元谓词的元素数量
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam T 值类型
 * @tparam BinaryPredicate 二元谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 比较值
 * @param pred 二元谓词函数
 * @return 满足谓词的元素数量
 */
template <typename Iterator, typename T, typename BinaryPredicate,
	enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr iter_difference_t<Iterator> count_if(Iterator first, Iterator last, const T& value, BinaryPredicate pred) {
	iter_difference_t<Iterator> n = 0;
	for (; first != last; ++first) {
		if (pred(*first, value)) ++n;
	}
	return n;
}

/**
 * @brief 统计范围内满足谓词的元素数量
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam Predicate 谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param pred 谓词函数
 * @return 满足谓词的元素数量
 */
template <typename Iterator, typename Predicate>
constexpr iter_difference_t<Iterator> count_if(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	iter_difference_t<Iterator> n = 0;
	for (; first != last; ++first) {
		if (pred(*first)) ++n;
	}
	return n;
}

/**
 * @brief 统计范围内等于指定值的元素数量
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam T 值类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要统计的值
 * @return 等于value的元素数量
 */
template <typename Iterator, typename T>
constexpr iter_difference_t<Iterator> count(Iterator first, Iterator last, const T& value) {
	return _NEFORCE count_if(first, last, value, _NEFORCE equal_to<iter_value_t<Iterator>>());
}

/** @} */ // CountingAlgorithms

/**
 * @defgroup FindingAlgorithms 查找元素算法
 * @brief 查找特定元素的算法
 * @{
 */

/**
 * @brief 查找范围内第一个等于指定值的元素
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param value 要查找的值
 * @return 指向第一个等于value的元素的迭代器，或last如果未找到
 */
template <typename Iterator, typename T>
NEFORCE_NODISCARD constexpr Iterator find(Iterator first, Iterator last, const T& value) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	while (first != last && *first != value) ++first;
	return first;
}

/**
 * @brief 查找范围内第一个满足谓词的元素
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam Predicate 谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param pred 谓词函数
 * @return 指向第一个满足pred的元素的迭代器，或last如果未找到
 */
template <typename Iterator, typename Predicate>
constexpr Iterator find_if(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	while (first != last && !pred(*first)) ++first;
	return first;
}

/**
 * @brief 查找范围内第一个不满足谓词的元素
 * @tparam Iterator 迭代器类型，需要满足输入迭代器要求
 * @tparam Predicate 谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param pred 谓词函数
 * @return 指向第一个不满足pred的元素的迭代器，或last如果未找到
 */
template <typename Iterator, typename Predicate,
	enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
constexpr Iterator find_if_not(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

	while (first != last && pred(*first)) ++first;
	return first;
}

/** @} */ // FindingAlgorithms

/**
 * @defgroup PatternMatchingAlgorithms 模式匹配算法
 * @brief 在范围内查找子序列的算法
 * @{
 */

/**
 * @brief 在范围内查找子序列的第一次出现
 * @tparam Iterator1 主序列迭代器类型，需要满足前向迭代器要求
 * @tparam Iterator2 子序列迭代器类型，需要满足前向迭代器要求
 * @tparam BinaryPredicate 二元谓词函数类型
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 子序列起始迭代器
 * @param last2 子序列终止迭代器
 * @param binary_pred 二元谓词函数
 * @return 指向子序列第一次出现位置的迭代器，或last1如果未找到
 */
template <typename Iterator1, typename Iterator2, typename BinaryPredicate>
constexpr Iterator1 search(
    Iterator1 first1, Iterator1 last1, Iterator2 first2,
	Iterator2 last2, BinaryPredicate binary_pred) {

    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, "Iterator must be forward_iterator");

	const auto d1 = _NEFORCE distance(first1, last1);
	const auto d2 = _NEFORCE distance(first2, last2);
	if (d1 < d2) return last1;

	Iterator1 current1 = first1;
	Iterator2 current2 = first2;

	while (current2 != last2) {
		if (binary_pred(*current1, *current2)) {
			++current1;
			++current2;
		} else {
			if (d1 == d2) return last1;

            current1 = ++first1;
            current2 = first2;
            --d1;
        }
	}
	return first1;
}

/**
 * @brief search的默认比较版本
 * @tparam Iterator1 主序列迭代器类型
 * @tparam Iterator2 子序列迭代器类型
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 子序列起始迭代器
 * @param last2 子序列终止迭代器
 * @return 指向子序列第一次出现位置的迭代器，或last1如果未找到
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator1 search(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _NEFORCE search(first1, last1, first2, last2, _NEFORCE equal_to<iter_value_t<Iterator1>>());
}

/**
 * @brief 查找范围内连续n个等于指定值的子序列
 * @tparam Iterator 迭代器类型，需要满足前向迭代器要求
 * @tparam T 值类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param count 连续出现的次数
 * @param value 要查找的值
 * @return 指向连续n个value的子序列起始位置的迭代器，或last如果未找到
 */
template <typename Iterator, typename T>
constexpr Iterator search_n(Iterator first, Iterator last, const size_t count, const T& value) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

	first = _NEFORCE find(first, last, value);
	while (first != last) {
		size_t n = count - 1;
		Iterator i = first;
		++i;
		while (i != last && n != 0 && *i == value) {
			++i;
			--n;
		}
		if (n == 0) return first;

        first = _NEFORCE find(i, last, value);
    }
	return last;
}

/**
 * @brief search_n的谓词版本
 * @tparam Iterator 迭代器类型，需要满足前向迭代器要求
 * @tparam T 值类型
 * @tparam BinaryPredicate 二元谓词函数类型
 * @param first 范围的起始迭代器
 * @param last 范围的终止迭代器
 * @param count 连续出现的次数
 * @param value 要查找的值
 * @param binary_pred 二元谓词函数
 * @return 指向连续n个满足谓词的子序列起始位置的迭代器，或last如果未找到
 */
template <typename Iterator, typename T, typename BinaryPredicate>
constexpr Iterator search_n(Iterator first, Iterator last, const size_t count, const T& value, BinaryPredicate binary_pred) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

	while (first != last) {
		if (binary_pred(*first, value)) break;
		++first;
	}

	while (first != last) {
		size_t n = count - 1;
		Iterator i = first;
		++i;

		while (i != last && n != 0 && binary_pred(*i, value)) {
			++i;
			--n;
		}
		if (n == 0) return first;

        while (i != last) {
            if (binary_pred(*i, value)) break;
            ++i;
        }
        first = i;
    }
	return last;
}

#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__

template <typename Iterator1, typename Iterator2>
constexpr
enable_if_t<is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>, Iterator1>
__find_end_aux(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	using reviter1 = _NEFORCE reverse_iterator<Iterator1>;
	using reviter2 = _NEFORCE reverse_iterator<Iterator2>;
	reviter1 rlast1(first1);
	reviter2 rlast2(first2);
	reviter1 rresult = _NEFORCE search(reviter1(last1), rlast1, reviter2(last2), rlast2);
	if (rresult == rlast1) return last1;
	Iterator1 result = rresult.base();
	_NEFORCE advance(result, -distance(first2, last2));
	return result;
}

template <typename Iterator1, typename Iterator2>
constexpr
enable_if_t<!(is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>), Iterator1>
__find_end_aux(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	Iterator1 result = last1;
	while (true) {
		Iterator1 new_result = _NEFORCE search(first1, last1, first2, last2);
		if (new_result == last1) return result;
		result = new_result;
		first1 = new_result;
		++first1;
	}
}

NEFORCE_END_INNER__
/// @endcond
#endif // NEFORCE_STANDARD_17

/**
 * @brief 在范围内查找子序列的最后一次出现
 * @tparam Iterator1 主序列迭代器类型，需要满足前向迭代器要求
 * @tparam Iterator2 子序列迭代器类型，需要满足前向迭代器要求
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 子序列起始迭代器
 * @param last2 子序列终止迭代器
 * @return 指向子序列最后一次出现位置的迭代器，或last1如果未找到
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator1 find_end(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, "Iterator must be forward_iterator");

	if (first2 == last2) return last1;
#ifdef NEFORCE_STANDARD_17
	if constexpr (is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>) {
		using reviter1 = _NEFORCE reverse_iterator<Iterator1>;
		using reviter2 = _NEFORCE reverse_iterator<Iterator2>;

		reviter1 rlast1(first1);
		reviter2 rlast2(first2);
		reviter1 rresult = _NEFORCE search(reviter1(last1), rlast1, reviter2(last2), rlast2);
		if (rresult == rlast1) return last1;

		Iterator1 result = rresult.base();
		_NEFORCE advance(result, -distance(first2, last2));
		return result;
	}
	else {
		Iterator1 result = last1;
		while (true) {
			Iterator1 new_result = _NEFORCE search(first1, last1, first2, last2);
			if (new_result == last1) return result;
			result = new_result;
			first1 = new_result;
			++first1;
		}
	}
#else
	return inner::__find_end_aux(first1, last1, first2, last2);
#endif
}

/**
 * @brief 查找范围内第一个出现在指定集合中的元素
 * @tparam Iterator1 主序列迭代器类型，需要满足输入迭代器要求
 * @tparam Iterator2 集合序列迭代器类型，需要满足输入迭代器要求
 * @tparam BinaryPredicate 二元谓词函数类型
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 集合序列起始迭代器
 * @param last2 集合序列终止迭代器
 * @param comp 二元谓词函数
 * @return 指向第一个出现在集合中的元素的迭代器，或last1如果未找到
 */
template <typename Iterator1, typename Iterator2, typename BinaryPredicate>
constexpr Iterator1 find_first_of(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, BinaryPredicate comp) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, "Iterator must be input_iterator");

	for (; first1 != last1; ++first1) {
		for (Iterator2 iter = first2; iter != last2; ++iter) {
			if (comp(*first1, *iter)) return first1;
		}
	}
	return last1;
}

/**
 * @brief find_first_of的默认比较版本
 * @tparam Iterator1 主序列迭代器类型
 * @tparam Iterator2 集合序列迭代器类型
 * @param first1 主序列起始迭代器
 * @param last1 主序列终止迭代器
 * @param first2 集合序列起始迭代器
 * @param last2 集合序列终止迭代器
 * @return 指向第一个出现在集合中的元素的迭代器，或last1如果未找到
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator1 find_first_of(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) {
	return _NEFORCE find_first_of(first1, last1, first2, last2, _NEFORCE equal_to<iter_value_t<Iterator1>>());
}

/** @} */ // PatternMatchingAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_SEARCH_HPP__
