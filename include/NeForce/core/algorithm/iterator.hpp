#ifndef NEFORCE_CORE_ALGORITHM_ITERATOR_HPP__
#define NEFORCE_CORE_ALGORITHM_ITERATOR_HPP__

/**
 * @file iterator.hpp
 * @brief 迭代器操作算法
 *
 * 此文件提供了迭代器的各种操作算法，
 * 包括获取迭代器特性、指针转换、前进/后退、距离计算等辅助函数。
 */

#include "NeForce/core/exception/breakpoint.hpp"
#include "NeForce/core/typeinfo/concepts.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup IteratorOperation 迭代器操作
 * @brief 迭代器操作函数的实现
 * @{
 */

#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__

template <typename Ptr>
constexpr enable_if_t<is_pointer_v<Ptr>, iter_pointer_t<Ptr>> __to_pointer_aux(Ptr iter) {
    return iter;
}

template <typename Iterator>
constexpr enable_if_t<!is_pointer_v<Iterator>, iter_pointer_t<Iterator>> __to_pointer_aux(Iterator iter) {
    return iter.operator->();
}

NEFORCE_END_INNER__
/// @endcond
#endif // NEFORCE_STANDARD_17

/**
 * @brief 将迭代器转换为原始指针
 * @tparam Iterator 迭代器类型
 * @param iter 迭代器
 * @return 原始指针
 */
template <typename Iterator>
constexpr iter_pointer_t<Iterator> to_pointer(Iterator iter) {
#ifdef NEFORCE_STANDARD_17
    if constexpr (is_pointer_v<Iterator>) {
        return iter;
    } else {
        return iter.operator->();
    }
#else
    return inner::__to_pointer_aux(iter);
#endif
}


#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__

template <typename Iterator, typename Distance>
constexpr enable_if_t<is_rnd_iter_v<Iterator>> __advance_aux(Iterator& i, Distance n) {
    i += n;
}

template <typename Iterator, typename Distance>
constexpr enable_if_t<!is_rnd_iter_v<Iterator> && is_ranges_bid_iter_v<Iterator>> __advance_aux(Iterator& i,
                                                                                                Distance n) {
    for (; n < 0; ++n) {
        --i;
    }
    for (; 0 < n; --n) {
        ++i;
    }
}

template <typename Iterator, typename Distance>
constexpr enable_if_t<!is_rnd_iter_v<Iterator> && !is_ranges_bid_iter_v<Iterator>> __advance_aux(Iterator& i,
                                                                                                 Distance n) {
    NEFORCE_DEBUG_VERIFY__(is_signed_v<Distance> && n >= 0, "negative advance of non-bidirectional iterator");
    for (; 0 < n; --n) {
        ++i;
    }
}

NEFORCE_END_INNER__
/// @endcond
#endif // NEFORCE_STANDARD_17

/**
 * @brief 将迭代器前进指定距离
 * @tparam Iterator 迭代器类型
 * @tparam Distance 距离类型
 * @param i 迭代器引用
 * @param n 前进距离
 *
 * 根据迭代器类型使用不同的前进策略：
 * - 随机访问迭代器：直接使用 += 操作
 * - 双向迭代器：支持正负距离
 * - 前向迭代器：只支持非负距离
 */
template <typename Iterator, typename Distance>
constexpr void advance(Iterator& i, Distance n) {
    static_assert(is_iter_v<Iterator>, "Iterator must be iterator");
    static_assert(is_arithmetic_v<Distance>, "Distance must be arithmetic");

#ifdef NEFORCE_STANDARD_17
    if constexpr (is_rnd_iter_v<Iterator>) {
        i += n;
    } else {
        if constexpr (is_signed_v<Distance> && !is_bid_iter_v<Iterator>) {
            NEFORCE_DEBUG_VERIFY(n >= 0, "negative advance of non-bidirectional iterator");
        }
        if constexpr (is_signed_v<Distance> && is_bid_iter_v<Iterator>) {
            for (; n < 0; ++n) {
                --i;
            }
        }
        for (; 0 < n; --n) {
            ++i;
        }
    }
#else
    inner::__advance_aux(i, n);
#endif
}

/**
 * @brief 获取迭代器的前一个位置
 * @tparam Iterator 迭代器类型
 * @param iter 当前迭代器
 * @param n 后退距离，默认为1
 * @return 后退n个位置后的迭代器
 *
 * 将迭代器后退n个位置，n必须为非负数。
 */
template <typename Iterator>
constexpr Iterator prev(Iterator iter, iter_difference_t<Iterator> n = 1) {
    NEFORCE_DEBUG_VERIFY(n >= 0, "negative advance in previous operation function.");
    _NEFORCE advance(iter, -n);
    return iter;
}

/**
 * @brief 获取迭代器的后一个位置
 * @tparam Iterator 迭代器类型
 * @param iter 当前迭代器
 * @param n 前进距离，默认为1
 * @return 前进n个位置后的迭代器
 *
 * 将迭代器前进n个位置，n必须为非负数。
 */
template <typename Iterator>
constexpr Iterator next(Iterator iter, iter_difference_t<Iterator> n = 1) {
    NEFORCE_DEBUG_VERIFY(n >= 0, "positive advance in next operation function.");
    _NEFORCE advance(iter, n);
    return iter;
}


#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__

template <typename Iterator>
constexpr enable_if_t<is_rnd_iter_v<Iterator>, iter_difference_t<Iterator>> __distance_aux(Iterator first,
                                                                                           Iterator last) {
    return last - first;
}

template <typename Iterator>
constexpr enable_if_t<!is_rnd_iter_v<Iterator>, iter_difference_t<Iterator>> __distance_aux(Iterator first,
                                                                                            Iterator last) {
    iter_difference_t<Iterator> n = 0;
    while (first != last) {
        ++first;
        ++n;
    }
    return n;
}

NEFORCE_END_INNER__
/// @endcond
#endif // NEFORCE_STANDARD_17

/**
 * @brief 计算两个迭代器之间的距离
 * @tparam Iterator 迭代器类型
 * @param first 起始迭代器
 * @param last 结束迭代器
 * @return 两个迭代器之间的距离
 *
 * 根据迭代器类型使用不同的计算策略：
 * - 随机访问迭代器：直接使用减法
 * - 其他迭代器：遍历计数
 */
template <typename Iterator>
constexpr iter_difference_t<Iterator> distance(Iterator first, Iterator last) {
    static_assert(is_iter_v<Iterator>, "Iterator must be iterator");

#ifdef NEFORCE_STANDARD_17
    if constexpr (is_ranges_rnd_iter_v<Iterator>) {
        return last - first;
    } else {
        iter_difference_t<Iterator> n = 0;
        while (first != last) {
            ++first;
            ++n;
        }
        return n;
    }
#else
    return inner::__distance_aux(first, last);
#endif
}

/** @} */ // IteratorOperation

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_ITERATOR_HPP__
