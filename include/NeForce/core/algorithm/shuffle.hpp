#ifndef NEFORCE_CORE_ALGORITHM_SHUFFLE_HPP__
#define NEFORCE_CORE_ALGORITHM_SHUFFLE_HPP__

/**
 * @file shuffle.hpp
 * @brief 随机重排算法
 *
 * 此文件提供了随机重排算法实现，用于将序列中的元素随机重新排列。
 */

#include "NeForce/core/numeric/random.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ShuffleAlgorithms 随机重排算法
 * @brief 随机重排算法的实现
 * @{
*/

/**
 * @brief 随机重排序列
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 *
 * 使用Fisher-Yates洗牌算法（Knuth洗牌）将范围 [first, last) 中的元素随机重排。
 * 算法对每个位置 i（从第二个元素开始），在 [first, i] 范围内随机选择一个位置 j，
 * 然后交换位置 i 和 j 的元素。
 *
 * @note 要求 Iterator 满足随机访问迭代器概念
 * @note 使用默认的 random_lcd 线性同余随机数生成器
 */
template <typename Iterator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void shuffle(Iterator first, Iterator last) {
	if (first == last) return;
	random_lcd rand;
    for (Iterator i = _NEFORCE next(first); i != last; ++i) {
        auto distance = _NEFORCE distance(first, i);
        Iterator j = _NEFORCE next(first, rand.next_int(0, static_cast<int>(distance)));
        _NEFORCE iter_swap(i, j);
    }
}

/**
 * @brief 随机重排序列（使用自定义随机数生成器）
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Generator 随机数生成器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param rand 自定义随机数生成器
 *
 * 使用Fisher-Yates洗牌算法将范围 [first, last) 中的元素随机重排。
 * 允许使用自定义的随机数生成器，提供更大的灵活性。
 *
 * 随机数生成器需要满足以下要求：
 * - 可调用，接受一个整数参数 n
 * - 返回 [0, n) 范围内的随机整数
 *
 * @note 要求 Iterator 满足随机访问迭代器概念
 * @note 随机数生成器必须提供 operator()(integer) -> integer 方法
 */
template <typename Iterator, typename Generator, enable_if_t<is_ranges_rnd_iter_v<Iterator>, int> = 0>
void shuffle(Iterator first, Iterator last, Generator& rand) {
	if (first == last) return;
	for (Iterator i = _NEFORCE next(first); i != last; ++i) {
		Iterator j = _NEFORCE next(first, rand(i - first + 1));
		_NEFORCE iter_swap(i, j);
	}
}

/** @} */ // ShuffleAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_SHUFFLE_HPP__
