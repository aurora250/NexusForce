#ifndef NEFORCE_CORE_ALGORITHM_PARALLEL_HPP__
#define NEFORCE_CORE_ALGORITHM_PARALLEL_HPP__

/**
 * @file parallel.hpp
 * @brief 并行算法
 *
 * 此文件提供了并行版本的算法，利用多线程加速计算。
 */

#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/algorithm/iterator.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ParallelAlgorithms 并行算法
 * @brief 并行计算算法
 * @{
 */

/**
 * @brief 并行归约操作
 * @tparam Iterator 迭代器类型
 * @tparam BinaryOperation 二元操作类型
 * @tparam Result 结果类型
 * @tparam Threshhold 并行阈值，小于等于此值时使用串行算法
 * @param first 范围的起始迭代器
 * @param last 范围的结束迭代器
 * @param op 归约操作的二元函数
 * @param res 归约结果的引用
 *
 * 使用分治法的并行归约算法。将范围分成两半，分别在不同线程中计算，最后合并结果。
 * 当元素数量小于阈值时，使用串行算法。
 */
template <typename Iterator, typename BinaryOperation, typename Result,
    size_t Threshhold = 10, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
void reduce(Iterator first, Iterator last, BinaryOperation op, Result& res) {
    const size_t dist = _NEFORCE distance(first, last);
    if (dist <= Threshhold) {
        for (Iterator it = first; it != last; ++it)
            res = op(res, *it);
    }
    else {
        Iterator mid = _NEFORCE next(first, dist / 2);
        Result l_res = res, r_res = res;
        _NEFORCE thread r_thd(reduce<Iterator, BinaryOperation, Result, Threshhold>, mid, last, op, _NEFORCE ref(r_res));
        _NEFORCE reduce(first, mid, op, l_res);
        r_thd.join();
        res = op(l_res, r_res);
    }
}

/**
 * @brief 并行变换归约操作
 * @tparam Iterator 迭代器类型
 * @tparam UnaryOperation 一元变换操作类型
 * @tparam BinaryOp 二元归约操作类型
 * @tparam Result 结果类型
 * @tparam Threshhold 并行阈值，小于等于此值时使用串行算法
 * @param first 范围的起始迭代器
 * @param last 范围的结束迭代器
 * @param transform 变换操作的一元函数
 * @param reduce 归约操作的二元函数
 * @param res 归约结果的引用
 *
 * 先对每个元素应用变换操作，然后进行归约。使用分治法的并行算法。
 * 当元素数量小于阈值时，使用串行算法。
 */
template <typename Iterator, typename UnaryOperation, typename BinaryOp, typename Result,
    size_t Threshhold = 10, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
void transform_reduce(Iterator first, Iterator last, UnaryOperation transform, BinaryOp reduce, Result& res) {
    const size_t dist = _NEFORCE distance(first, last);
    if (dist <= Threshhold) {
        for (Iterator it = first; it != last; ++it)
            res = reduce(res, transform(*it));
    }
    else {
        Iterator mid = _NEFORCE next(first, dist / 2);
        Result l_res = _NEFORCE initialize<Result>(), r_res = _NEFORCE initialize<Result>();
        _NEFORCE thread r_thd(transform_reduce<Iterator, UnaryOperation, BinaryOp, Result, Threshhold>,
            mid, last, transform, reduce, _NEFORCE ref(r_res));
        _NEFORCE transform_reduce(first, mid, transform, reduce, l_res);
        r_thd.join();
        res = reduce(res, reduce(l_res, r_res));
    }
}

/** @} */ // ParallelAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_PARALLEL_HPP__
