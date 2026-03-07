#ifndef NEFORCE_CORE_ALGORITHM_COMPARE_HPP__
#define NEFORCE_CORE_ALGORITHM_COMPARE_HPP__

/**
 * @file compare.hpp
 * @brief 比较算法
 *
 * 此文件提供了比较算法实现，
 * 包括相等性比较、范围比较、极值查找、字典序比较等功能。
 */

#include "NeForce/core/memory/memory.hpp"
#include "NeForce/core/utility/pair.hpp"
#include "NeForce/core/algorithm/search.hpp"
#include <initializer_list>
#ifdef NEFORCE_PLATFORM_WINDOWS
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CompareAlgorithms 比较算法
 * @brief 比较算法的实现
 * @{
 */

/**
 * @brief 比较两个范围是否相等
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @tparam BinaryPredicate 二元谓词类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param binary_pred 相等性谓词
 * @return 如果两个范围相等则返回true，否则返回false
 *
 * 比较范围 [first1, last1) 和以 first2 开始的范围是否相等。
 * 使用二元谓词 binary_pred 判断两个元素是否相等。
 * 要求第二个范围至少与第一个范围一样长。
 */
template <typename Iterator1, typename Iterator2, typename BinaryPredicate>
NEFORCE_NODISCARD constexpr bool equal(Iterator1 first1, Iterator1 last1, Iterator2 first2, BinaryPredicate binary_pred)
noexcept(noexcept(++first1) && noexcept(++first2) && noexcept(binary_pred(*first1, *first2))) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, "Iterator must be input_iterator");
	for (; first1 != last1; ++first1, ++first2) {
		if (!binary_pred(*first1, *first2)) return false;
	}
	return true;
}

/**
 * @brief 比较两个范围是否相等
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @return 如果两个范围相等则返回true，否则返回false
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool equal(Iterator1 first1, Iterator1 last1, Iterator2 first2)
noexcept(noexcept(_NEFORCE equal(first1, last1, first2, _NEFORCE equal_to<iter_value_t<Iterator1>>()))) {
	return _NEFORCE equal(first1, last1, first2, _NEFORCE equal_to<iter_value_t<Iterator1>>());
}

/**
 * @brief 查找值的相等范围
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param value 要查找的值
 * @param comp 比较函数对象
 * @return 包含相等范围的pair<起始迭代器, 结束迭代器>
 *
 * 在已排序的范围 [first, last) 中查找所有等于 value 的元素。
 * 返回的pair包含相等范围的 [起始, 结束) 迭代器。
 */
template <typename Iterator, typename T, typename Compare>
constexpr pair<Iterator, Iterator> equal_range(Iterator first, Iterator last, const T& value, Compare comp) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

	auto len = _NEFORCE distance(first, last);
	auto half = len;
	Iterator middle, left, right;

	while (len > 0) {
		half = len >> 1;
		middle = first;
		_NEFORCE advance(middle, half);

		if (comp(*middle, value)) {
			first = middle;
			++first;
			len = len - half - 1;
		} else if (comp(value, *middle)) {
			len = half;
		} else {
			left = _NEFORCE lower_bound(first, middle, value, comp);
			_NEFORCE advance(first, len);
			right = _NEFORCE upper_bound(++middle, first, value, comp);
			return pair<Iterator, Iterator>(left, right);
		}
	}
	return pair<Iterator, Iterator>(first, first);
}

/**
 * @brief 查找值的相等范围
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param value 要查找的值
 * @return 包含相等范围的pair<起始迭代器, 结束迭代器>
 */
template <typename Iterator, typename T>
constexpr pair<Iterator, Iterator> equal_range(Iterator first, Iterator last, const T& value) {
	return _NEFORCE equal_range(first, last, value, _NEFORCE less<iter_value_t<Iterator>>());
}


/**
 * @brief 返回两个值中的较大者
 * @tparam T 值类型
 * @tparam Compare 比较函数类型
 * @param a 第一个值
 * @param b 第二个值
 * @param comp 比较函数对象
 * @return a和b中的较大者
 *
 * 使用比较函数 comp 判断大小：
 * - 如果 comp(a, b) 返回 true，则 b 较大
 * - 否则 a 较大
 */
template <typename T, typename Compare>
constexpr const T& max(const T& a, const T& b, Compare comp)
noexcept(noexcept(comp(a, b))) {
	return comp(a, b) ? b : a;
}

/**
 * @brief 返回两个值中的较大者
 * @tparam T 值类型
 * @param a 第一个值
 * @param b 第二个值
 * @return a和b中的较大者
 */
template <typename T>
constexpr const T& max(const T& a, const T& b)
noexcept(noexcept(a < b)) {
	return a < b ? b : a;
}

/**
 * @brief 返回两个值中的较小者
 * @tparam T 值类型
 * @tparam Compare 比较函数类型
 * @param a 第一个值
 * @param b 第二个值
 * @param comp 比较函数对象
 * @return a和b中的较小者
 *
 * 使用比较函数 comp 判断大小：
 * - 如果 comp(b, a) 返回 true，则 b 较小
 * - 否则 a 较小
 */
template <typename T, typename Compare>
constexpr const T& min(const T& a, const T& b, Compare comp)
noexcept(noexcept(comp(b, a))) {
	return comp(b, a) ? b : a;
}

/**
 * @brief 返回两个值中的较小者
 * @tparam T 值类型
 * @param a 第一个值
 * @param b 第二个值
 * @return a和b中的较小者
 */
template <typename T>
constexpr const T& min(const T& a, const T& b)
noexcept(noexcept(b < a)) {
	return b < a ? b : a;
}

/**
 * @brief 返回三个值的中位数
 * @tparam T 值类型
 * @tparam Compare 比较函数类型
 * @param a 第一个值
 * @param b 第二个值
 * @param c 第三个值
 * @param comp 比较函数对象
 * @return a、b、c中的中位数
 *
 * 使用比较函数 comp 确定三个值的中位数。
 * 最多进行3次比较。
 */
template <typename T, typename Compare>
constexpr const T& median(const T& a, const T& b, const T& c, Compare comp)
noexcept(noexcept(comp(a, b))) {
	if (comp(a, b)) {
		if (comp(b, c))
			return b;
		else if (comp(a, c))
			return c;
		else
			return a;
	}
	else if (comp(a, c))
		return a;
	else if (comp(b, c))
		return c;
	else
		return b;
}

/**
 * @brief 返回三个值的中位数
 * @tparam T 值类型
 * @param a 第一个值
 * @param b 第二个值
 * @param c 第三个值
 * @return a、b、c中的中位数
 */
template <typename T>
constexpr const T& median(const T& a, const T& b, const T& c)
noexcept(noexcept(_NEFORCE median(a, b, c, _NEFORCE less<T>()))) {
	return _NEFORCE median(a, b, c, _NEFORCE less<T>());
}


/**
 * @brief 查找范围中的最小值和最大值
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 包含最小值和最大值的pair
 *
 * 同时查找范围 [first, last) 中的最小值和最大值。
 * 如果范围为空，返回默认构造的pair。
 */
template <typename Iterator, typename Compare>
pair<iter_value_t<Iterator>, iter_value_t<Iterator>>
constexpr minmax(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
	using T = iter_value_t<Iterator>;
	if (first == last) {
		return _NEFORCE make_pair(T(), T());
	}
	T min_val = *first;
	T max_val = *first;

	++first;
	for (; first != last; ++first) {
		if (comp(*first, min_val))
			min_val = *first;
		else if (!comp(*first, max_val))
			max_val = *first;
	}
	return _NEFORCE make_pair(min_val, max_val);
}

/**
 * @brief 查找范围中的最小值和最大值
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 包含最小值和最大值的pair
 */
template <typename Iterator>
constexpr pair<iter_value_t<Iterator>, iter_value_t<Iterator>> minmax(Iterator first, Iterator last) {
	return _NEFORCE minmax(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}


/**
 * @brief 查找范围中的最大元素
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 指向最大元素的迭代器，如果范围为空则返回first
 *
 * 查找范围 [first, last) 中的最大元素。
 * 如果有多个最大元素，返回第一个遇到的。
 */
template <typename Iterator, typename Compare>
constexpr Iterator max_element(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
	if (first == last) return first;
	Iterator result = first;
	while (++first != last)
		if (comp(*result, *first)) result = first;
	return result;
}

/**
 * @brief 查找范围中的最大元素
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 指向最大元素的迭代器
 */
template <typename Iterator>
constexpr Iterator max_element(Iterator first, Iterator last) {
	return _NEFORCE max_element(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 返回初始化列表中的最大值
 * @tparam T 值类型
 * @param list 初始化列表
 * @return 列表中的最大值
 */
template <typename T>
constexpr const T& max(std::initializer_list<T> list) {
	auto iter = _NEFORCE max_element(list.begin(), list.end());
	return *iter;
}

/**
 * @brief 查找范围中的最小元素
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return 指向最小元素的迭代器，如果范围为空则返回first
 *
 * 查找范围 [first, last) 中的最小元素。
 * 如果有多个最小元素，返回第一个遇到的。
 */
template <typename Iterator, typename Compare>
constexpr Iterator min_element(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
	if (first == last) return first;
	Iterator result = first;
	while (++first != last)
		if (comp(*first, *result)) result = first;
	return result;
}

/**
 * @brief 查找范围中的最小元素
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 指向最小元素的迭代器
 */
template <typename Iterator>
constexpr Iterator min_element(Iterator first, Iterator last) {
	return _NEFORCE min_element(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}

/**
 * @brief 返回初始化列表中的最小值
 * @tparam T 值类型
 * @param list 初始化列表
 * @return 列表中的最小值
 */
template <typename T>
constexpr const T& min(std::initializer_list<T> list) {
	return *_NEFORCE min_element(list.begin(), list.end());
}

/**
 * @brief 同时查找范围中的最小和最大元素
 * @tparam Iterator 迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param comp 比较函数对象
 * @return pair<指向最小元素的迭代器, 指向最大元素的迭代器>
 *
 * 同时查找范围 [first, last) 中的最小和最大元素。
 * 如果范围为空，两个迭代器都指向first。
 */
template <typename Iterator, typename Compare>
constexpr pair<Iterator, Iterator> minmax_element(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
	Iterator min = _NEFORCE min_element(first, last, comp);
	Iterator max = _NEFORCE max_element(first, last, comp);
	return _NEFORCE make_pair(min, max);
}

/**
 * @brief 同时查找范围中的最小和最大元素
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return pair<指向最小元素的迭代器, 指向最大元素的迭代器>
 */
template <typename Iterator>
constexpr pair<Iterator, Iterator> minmax_element(Iterator first, Iterator last) {
	return _NEFORCE minmax_element(first, last, _NEFORCE less<iter_value_t<Iterator>>());
}


/**
 * @brief 将值限制在指定范围内
 * @tparam T 值类型
 * @tparam Compare 比较函数类型
 * @param value 要限制的值
 * @param lower 下限值
 * @param upper 上限值
 * @param comp 比较函数对象
 * @return 限制后的值
 *
 * 将 value 限制在 [lower, upper] 范围内：
 * - 如果 value < lower，返回 lower
 * - 如果 value > upper，返回 upper
 * - 否则返回 value
 *
 * 要求 lower <= upper
 */
template <typename T, typename Compare>
constexpr const T& clamp(const T& value, const T& lower, const T& upper, Compare comp)
noexcept(noexcept(comp(value, lower))) {
	if (comp(value, lower))
		return lower;
	else if (comp(upper, value))
		return upper;
	return value;
}

/**
 * @brief 将值限制在指定范围内
 * @tparam T 值类型
 * @param value 要限制的值
 * @param lower 下限值
 * @param upper 上限值
 * @return 限制后的值
 */
template <typename T>
constexpr const T& clamp(const T& value, const T& lower, const T& upper)
noexcept(noexcept(_NEFORCE clamp(value, lower, upper, _NEFORCE less<T>()))) {
	return _NEFORCE clamp(value, lower, upper, _NEFORCE less<T>());
}


/**
 * @brief 字典序比较两个范围
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @tparam Compare 比较函数类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @param comp 比较函数对象
 * @return 如果第一个范围字典序小于第二个范围则返回true，否则返回false
 *
 * 比较范围 [first1, last1) 和 [first2, last2) 的字典序：
 * 1. 比较对应元素，直到找到不相等的元素
 * 2. 第一个范围的元素小于第二个范围的对应元素时返回true
 * 3. 所有对应元素都相等，但第一个范围较短时返回true
 */
template <typename Iterator1, typename Iterator2, typename Compare>
NEFORCE_NODISCARD constexpr bool lexicographical_compare(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2, Compare comp)
noexcept(noexcept(++first1) && noexcept(++first2) && noexcept(comp(*first1, *first2)) && noexcept(first1 == last1 && first2 != last2)) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, "Iterator must be input_iterator");
	for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
		if (comp(*first1, *first2)) return true;
		if (comp(*first2, *first1)) return false;
	}
	return first1 == last1 && first2 != last2;
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 字典序比较辅助函数（连续迭代器版本）
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @return 比较结果
 *
 * 对连续迭代器使用内存比较优化。
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr
enable_if_t<is_ranges_cot_iter_v<Iterator1> && is_ranges_cot_iter_v<Iterator2>, bool>
__lexicographical_compare_aux(
	Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2) noexcept {
	const auto len1 = static_cast<size_t>(last1 - first1);
	const auto len2 = static_cast<size_t>(last2 - first2);
	const size_t clp = _NEFORCE min(len1, len2);

	const int result = _NEFORCE memory_compare(
		_NEFORCE addressof(*first1), _NEFORCE addressof(*first2),
		clp * sizeof(iter_value_t<Iterator1>));
	return result != 0 ? result < 0 : len1 < len2;
}

/**
 * @brief 字典序比较辅助函数（非连续迭代器版本）
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @return 比较结果
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr
enable_if_t<!(is_ranges_cot_iter_v<Iterator1> && is_ranges_cot_iter_v<Iterator2>), bool>
__lexicographical_compare_aux(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2)
noexcept(noexcept(_NEFORCE lexicographical_compare(first1, last1, first2, last2, _NEFORCE less<iter_value_t<Iterator1>>()))) {
	return _NEFORCE lexicographical_compare(first1, last1, first2, last2, _NEFORCE less<iter_value_t<Iterator1>>());
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 字典序比较两个范围
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param last2 第二个范围结束
 * @return 如果第一个范围字典序小于第二个范围则返回true，否则返回false
 *
 * 对连续迭代器进行优化，使用内存比较提高性能。
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool lexicographical_compare(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2)
noexcept(noexcept(_INNER __lexicographical_compare_aux(first1, last1, first2, last2))) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, "Iterator must be input_iterator");
	return _INNER __lexicographical_compare_aux(first1, last1, first2, last2);
}


/**
 * @brief 查找两个范围中首个不匹配的元素
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @tparam Compare 比较函数类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @param comp 比较函数对象
 * @return pair<第一个范围中不匹配的位置, 第二个范围中对应的位置>
 *
 * 比较范围 [first1, last1) 和以 first2 开始的范围，
 * 返回第一个不满足 comp(*it1, *it2) 的位置。
 * 如果所有对应元素都满足谓词，返回 pair<last1, first2 + (last1 - first1)>
 */
template <typename Iterator1, typename Iterator2, typename Compare>
pair<Iterator1, Iterator2> constexpr mismatch(Iterator1 first1, Iterator1 last1, Iterator2 first2, Compare comp) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>, "Iterator must be input_iterator");
	while (first1 != last1 && comp(*first1, *first2)) {
		++first1; ++first2;
	}
	return _NEFORCE make_pair<Iterator1, Iterator2>(first1, first2);
}

/**
 * @brief 查找两个范围中首个不匹配的元素
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @return pair<第一个范围中不匹配的位置, 第二个范围中对应的位置>
 */
template <typename Iterator1, typename Iterator2>
constexpr pair<Iterator1, Iterator2> mismatch(Iterator1 first1, Iterator1 last1, Iterator2 first2) {
	return _NEFORCE mismatch(first1, last1, first2, _NEFORCE equal_to<iter_value_t<Iterator1>>());
}

/** @} */ // CompareAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_COMPARE_HPP__
