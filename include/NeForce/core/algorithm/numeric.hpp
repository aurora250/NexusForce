#ifndef NEFORCE_CORE_ALGORITHM_NUMERIC_HPP__
#define NEFORCE_CORE_ALGORITHM_NUMERIC_HPP__

/**
 * @file numeric.hpp
 * @brief 数值算法库
 *
 * 此文件提供了库的数值算法实现，
 * 包括累加、内积、差分、前缀和等数值计算相关的通用算法。
 */

#include "NeForce/core/functional/functor.hpp"
#include "NeForce/core/typeinfo/concepts.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup NumericAlgorithms 数值算法
 * @brief 数值算法的实现
 * @{
 */

/**
 * @brief 累积计算
 * @tparam Iterator 输入迭代器类型
 * @tparam T 初始值类型
 * @tparam BinaryOperation 二元操作类型
 * @param first 范围起始迭代器
 * @param second 范围结束迭代器
 * @param init 初始值
 * @param binary_op 二元操作函数对象
 * @return 累积计算的结果
 *
 * 对范围 [first, second) 中的元素执行累积计算
 */
template <typename Iterator, typename T, typename BinaryOperation>
NEFORCE_CONSTEXPR20 T accumulate(Iterator first, Iterator second, T init, BinaryOperation binary_op) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    for (; first != second; ++first) {
        init = binary_op(init, *first);
    }
    return init;
}

/**
 * @brief 累积计算
 * @tparam Iterator 输入迭代器类型
 * @tparam T 初始值类型
 * @param first 范围起始迭代器
 * @param second 范围结束迭代器
 * @param init 初始值
 * @return 所有元素与初始值的和
 *
 * 默认使用加法运算的累积计算。
 */
template <typename Iterator, typename T>
NEFORCE_CONSTEXPR20 T accumulate(Iterator first, Iterator second, T init) {
    return _NEFORCE accumulate(first, second, init, _NEFORCE plus<T>());
}

/**
 * @brief 相邻差分计算
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam BinaryOperation 二元操作类型
 * @param first 输入范围起始迭代器
 * @param last 输入范围结束迭代器
 * @param result 输出范围起始迭代器
 * @param binary_op 二元操作函数对象
 * @return 输出范围结束迭代器
 *
 * 计算相邻元素的差分并存储到输出范围。
 */
template <typename Iterator1, typename Iterator2, typename BinaryOperation>
NEFORCE_CONSTEXPR20 Iterator2 adjacent_difference(Iterator1 first, Iterator1 last, Iterator2 result,
                                                  BinaryOperation binary_op) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>,
                  "Iterator must be input_iterator");

    if (first == last) {
        return result;
    }

    using T = iter_value_t<Iterator1>;
    *result = *first;
    T value = *first;

    while (++first != last) {
        T tem = *first;
        *++result = binary_op(tem, value);
        value = tem;
    }
    return ++result;
}

/**
 * @brief 相邻差分计算
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始迭代器
 * @param last 输入范围结束迭代器
 * @param result 输出范围起始迭代器
 * @return 输出范围结束迭代器
 *
 * 默认使用减法运算的相邻差分计算。
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_CONSTEXPR20 Iterator2 adjacent_difference(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _NEFORCE adjacent_difference(first, last, result, _NEFORCE minus<iter_value_t<Iterator1>>());
}

/**
 * @brief 内积计算
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam T 初始值类型
 * @tparam BinaryOperation1 第一个二元操作类型
 * @tparam BinaryOperation2 第二个二元操作类型
 * @param first1 第一个范围起始迭代器
 * @param last1 第一个范围结束迭代器
 * @param first2 第二个范围起始迭代器
 * @param init 初始值
 * @param binary_op1 累积操作函数对象
 * @param binary_op2 元素对操作函数对象
 * @return 两个范围的内积结果
 *
 * 计算两个范围的内积。
 */
template <typename Iterator1, typename Iterator2, typename T, typename BinaryOperation1, typename BinaryOperation2>
NEFORCE_CONSTEXPR20 T inner_product(Iterator1 first1, Iterator1 last1, Iterator2 first2, T init,
                                    BinaryOperation1 binary_op1, BinaryOperation2 binary_op2) {

    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>,
                  "Iterator must be input_iterator");

    for (; first1 != last1; ++first1, ++first2) {
        init = binary_op1(init, binary_op2(*first1, *first2));
    }
    return init;
}

/**
 * @brief 内积计算
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam T 初始值类型
 * @param first1 第一个范围起始迭代器
 * @param last1 第一个范围结束迭代器
 * @param first2 第二个范围起始迭代器
 * @param init 初始值
 * @return 两个范围的内积
 *
 * 默认使用乘法和加法运算的内积计算，即点积。
 */
template <typename Iterator1, typename Iterator2, typename T>
NEFORCE_CONSTEXPR20 T inner_product(Iterator1 first1, Iterator1 last1, Iterator2 first2, T init) {
    return _NEFORCE inner_product(first1, last1, first2, init, _NEFORCE plus<T>(), _NEFORCE multiplies<T>());
}

/**
 * @brief 部分和计算
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam BinaryOperation 二元操作类型
 * @param first 输入范围起始迭代器
 * @param last 输入范围结束迭代器
 * @param result 输出范围起始迭代器
 * @param binary_op 二元操作函数对象
 * @return 输出范围结束迭代器
 *
 * 计算前缀和并存储到输出范围。
 */
template <typename Iterator1, typename Iterator2, typename BinaryOperation>
NEFORCE_CONSTEXPR20 Iterator2 partial_sum(Iterator1 first, Iterator1 last, Iterator2 result,
                                          BinaryOperation binary_op) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>,
                  "Iterator must be input_iterator");

    if (first == last) {
        return result;
    }

    *result = *first;
    iter_value_t<Iterator1> value = *first;

    while (++first != last) {
        value = binary_op(value, *first);
        *++result = value;
    }
    return ++result;
}

/**
 * @brief 部分和计算
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始迭代器
 * @param last 输入范围结束迭代器
 * @param result 输出范围起始迭代器
 * @return 输出范围结束迭代器
 *
 * 默认使用加法运算的部分和计算，即前缀和。
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_CONSTEXPR20 Iterator2 partial_sum(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _NEFORCE partial_sum(first, last, result, _NEFORCE plus<iter_value_t<Iterator1>>());
}

/**
 * @brief 顺序填充递增序列
 * @tparam Iterator 输出迭代器类型
 * @tparam T 值类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @param value 起始值
 *
 * 用从value开始的连续值填充范围 [first, last)。
 */
template <typename Iterator, typename T>
NEFORCE_CONSTEXPR20 void sequence_fill(Iterator first, Iterator last, T value) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");

    while (first != last) {
        *first++ = value;
        ++value;
    }
}

/** @} */ // NumericAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_NUMERIC_HPP__
