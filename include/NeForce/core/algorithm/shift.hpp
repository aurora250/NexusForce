#ifndef NEFORCE_CORE_ALGORITHM_SHIFT_HPP__
#define NEFORCE_CORE_ALGORITHM_SHIFT_HPP__

/**
 * @file shift.hpp
 * @brief 移位和修改算法
 *
 * 此文件提供了移位和修改算法实现，
 * 包括复制、移动、填充、替换、旋转、反转等序列操作。
 */

#include "NeForce/core/algorithm/search.hpp"
#include "NeForce/core/functional/invoke.hpp"
#include "NeForce/core/numeric/math.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup ShiftAlgorithms 修改算法
 * @brief 移位和修改算法的实现
 * @{
 */

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 复制辅助函数（非连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 *
 * 使用循环逐个元素复制。
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<!(is_ranges_cot_iter_v<Iterator1> && is_ranges_cot_iter_v<Iterator2>), Iterator2>
__copy_aux(Iterator1 first, Iterator1 last,
           Iterator2 result) noexcept(is_nothrow_assignable_v<iter_value_t<Iterator2>, iter_value_t<Iterator1>>) {
    auto n = _NEFORCE distance(first, last);
    for (; n > 0; --n, ++first, ++result) {
        *result = *first;
    }
    return result;
}

/**
 * @brief 复制辅助函数（连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 *
 * 使用内存移动优化复制操作。
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<is_ranges_cot_iter_v<Iterator1> && is_ranges_cot_iter_v<Iterator2>, Iterator2>
__copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) noexcept {
    const auto n = static_cast<size_t>(last - first);
    const auto bytes = n * sizeof(iter_value_t<Iterator1>);
    _NEFORCE memory_move(_NEFORCE addressof(*result), _NEFORCE addressof(*first), bytes);
    return _NEFORCE next(result, n);
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 复制范围元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 将范围 [first, last) 的元素复制到以 result 开始的位置。
 * 对连续迭代器使用内存移动优化，非连续迭代器使用循环复制。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 copy(Iterator1 first, Iterator1 last,
                         Iterator2 result) noexcept(noexcept(inner::__copy_aux(first, last, result))) {
    static_assert(is_ranges_input_iter_v<Iterator1>, "Iterator1 must be input_iterator");
    static_assert(is_iter_v<Iterator2>, "Iterator2 must be iterator");
    if (first == last) {
        return result;
    }
    return inner::__copy_aux(first, last, result);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 复制n个元素辅助函数（随机访问迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要复制的元素数量
 * @param result 输出起始
 * @return 输出范围结束迭代器
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<is_ranges_rnd_iter_v<Iterator1>, Iterator2>
__copy_n_aux(Iterator1 first, iter_difference_t<Iterator1> count, Iterator2 result) {
    return _NEFORCE copy(first, _NEFORCE next(first, count), result);
}

/**
 * @brief 复制n个元素辅助函数（非随机访问迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要复制的元素数量
 * @param result 输出起始
 * @return 输出范围结束迭代器
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, Iterator2>
__copy_n_aux(Iterator1 first, iter_difference_t<Iterator1> count, Iterator2 result) {
    for (; count > 0; --count, ++first, ++result) {
        *result = *first;
    }
    return result;
}
NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 复制指定数量的元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始迭代器
 * @param count 要复制的元素数量
 * @param result 输出起始迭代器
 * @return 输出范围结束迭代器
 *
 * 从 first 开始复制 count 个元素到 result。
 * 返回复制后的输入和输出结束迭代器。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 copy_n(Iterator1 first, iter_difference_t<Iterator1> count, Iterator2 result) {
    static_assert(is_ranges_input_iter_v<Iterator1>, "Iterator1 must be input_iterator");
    static_assert(is_iter_v<Iterator2>, "Iterator2 must be iterator");
    return inner::__copy_n_aux(first, count, result);
}

/**
 * @brief 复制满足谓词的元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam Pred 一元谓词类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @param pred 一元谓词
 * @return 输出范围结束迭代器
 *
 * 复制范围 [first, last) 中满足谓词 unary_pred 的所有元素。
 */
template <typename Iterator1, typename Iterator2, typename Pred>
constexpr Iterator2 copy_if(Iterator1 first, Iterator1 last, Iterator2 result, Pred pred) {
    static_assert(is_ranges_input_iter_v<Iterator1>, "Iterator1 must be input_iterator");
    static_assert(is_iter_v<Iterator2>, "Iterator2 must be iterator");

    for (; first != last; ++first) {
        if (pred(*first)) {
            *result++ = *first;
        }
    }
    return result;
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 反向复制辅助函数（非连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围结束
 * @return 输出范围起始
 *
 * 从后向前逐个元素复制。
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<!is_ranges_cot_iter_v<Iterator1>, Iterator2>
__copy_backward_aux(Iterator1 first, Iterator1 last,
                    Iterator2 result) noexcept(is_nothrow_copy_assignable_v<iter_value_t<Iterator1>>) {
    iter_difference_t<Iterator1> n = _NEFORCE distance(first, last);
    for (; n > 0; --n) {
        *--result = *--last;
    }
    return result;
}

/**
 * @brief 反向复制辅助函数（连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围结束
 * @return 输出范围起始
 *
 * 使用内存移动优化反向复制。
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<is_ranges_cot_iter_v<Iterator1>, Iterator2> __copy_backward_aux(Iterator1 first, Iterator1 last,
                                                                                      Iterator2 result) noexcept {
    const auto n = static_cast<size_t>(last - first);
    auto dest = _NEFORCE prev(result, n);
    _NEFORCE memory_move(_NEFORCE addressof(*dest), _NEFORCE addressof(*first), n * sizeof(iter_value_t<Iterator1>));
    return dest;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 反向复制范围元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围结束
 * @return 输出范围起始
 *
 * 将范围 [first, last) 的元素反向复制到以 result-1 开始向前的位置。
 * 用于处理目标范围与源范围重叠的情况。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2
copy_backward(Iterator1 first, Iterator1 last,
              Iterator2 result) noexcept(noexcept(inner::__copy_backward_aux(first, last, result))) {
    static_assert(is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>,
                  "Iterators must be bidirectional_iterator");

    if (first == last) {
        return result;
    }
    return inner::__copy_backward_aux(first, last, result);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 移动辅助函数（非连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<!(is_ranges_cot_iter_v<Iterator1> && is_trivially_copyable_v<iter_value_t<Iterator1>>), Iterator2>
__move_aux(Iterator1 first, Iterator1 last,
           Iterator2 result) noexcept(is_nothrow_move_assignable_v<iter_value_t<Iterator1>>) {
    iter_difference_t<Iterator1> n = _NEFORCE distance(first, last);
    for (; n > 0; --n, ++first, ++result) {
        *result = _NEFORCE move(*first);
    }
    return result;
}

/**
 * @brief 移动辅助函数（连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<is_ranges_cot_iter_v<Iterator1> && is_trivially_copyable_v<iter_value_t<Iterator1>>, Iterator2>
__move_aux(Iterator1 first, Iterator1 last, Iterator2 result) noexcept {
    const auto n = static_cast<size_t>(last - first);
    _NEFORCE memory_move(_NEFORCE addressof(*result), _NEFORCE addressof(*first), n * sizeof(iter_value_t<Iterator1>));
    return result + n;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 移动范围元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 将范围 [first, last) 的元素移动到以 result 开始的位置。
 * 移动后源位置的对象状态是未定义的。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 move(Iterator1 first, Iterator1 last,
                         Iterator2 result) noexcept(noexcept(inner::__move_aux(first, last, result))) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>,
                  "Iterators must be input_iterator");

    if (first == last) {
        return result;
    }
    return inner::__move_aux(first, last, result);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 反向移动辅助函数（非连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围结束
 * @return 输出范围起始
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<!(is_ranges_cot_iter_v<Iterator1> && is_trivially_copyable_v<iter_value_t<Iterator1>>), Iterator2>
__move_backward_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    for (size_t n = _NEFORCE distance(first, last); n > 0; --n) {
        *--result = _NEFORCE move(*--last);
    }
    return result;
}

/**
 * @brief 反向移动辅助函数（连续迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围结束
 * @return 输出范围起始
 */
template <typename Iterator1, typename Iterator2>
constexpr enable_if_t<is_ranges_cot_iter_v<Iterator1> && is_trivially_copyable_v<iter_value_t<Iterator1>>, Iterator2>
__move_backward_aux(Iterator1 first, Iterator1 last, Iterator2 result) noexcept {
    const auto n = static_cast<size_t>(last - first);
    auto dest = result - n;
    _NEFORCE memory_move(_NEFORCE addressof(*dest), _NEFORCE addressof(*first), n * sizeof(iter_value_t<Iterator1>));
    return dest;
}
NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 反向移动范围元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围结束
 * @return 输出范围起始
 *
 * 将范围 [first, last) 的元素反向移动到以 result-1 开始向前的位置。
 * 用于处理目标范围与源范围重叠的情况。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 move_backward(Iterator1 first, Iterator1 last, Iterator2 result) {
    static_assert(is_ranges_bid_iter_v<Iterator1> && is_ranges_bid_iter_v<Iterator2>,
                  "Iterators must be bidirectional_iterator");
    if (first == last) {
        return result;
    }
    return inner::__move_backward_aux(first, last, result);
}


/**
 * @brief 填充范围元素
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param value 要填充的值
 *
 * 将范围 [first, last) 的所有元素设置为 value。
 */
template <typename Iterator, typename T>
constexpr void fill(Iterator first, Iterator last,
                    const T& value) noexcept(is_nothrow_assignable_v<iter_value_t<Iterator>, T>) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterators must be input_iterator");
    static_assert(is_assignable_v<decltype(*first), T>, "Iterators must be assignable from T");

    for (; first != last; ++first) {
        *first = value;
    }
}

/**
 * @brief 填充指定数量的元素
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 起始迭代器
 * @param n 要填充的元素数量
 * @param value 要填充的值
 * @return 填充后的结束迭代器
 */
template <typename Iterator, typename T>
constexpr Iterator fill_n(Iterator first, iter_difference_t<Iterator> n,
                          const T& value) noexcept(is_nothrow_assignable_v<iter_value_t<Iterator>, T>) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterators must be input_iterator");
    static_assert(is_assignable_v<decltype(*first), T>, "Iterators must be assignable from T");

    for (; n > 0; --n, ++first) {
        *first = value;
    }
    return first;
}


/**
 * @brief 交换迭代器指向的元素
 * @tparam Iterator1 第一个迭代器类型
 * @tparam Iterator2 第二个迭代器类型
 * @param a 第一个迭代器
 * @param b 第二个迭代器
 *
 * 交换两个迭代器指向的元素的值。
 */
template <typename Iterator1, typename Iterator2>
constexpr void iter_swap(Iterator1 a, Iterator2 b) noexcept(noexcept(_NEFORCE swap(*a, *b))) {
    static_assert(is_ranges_input_iter_v<Iterator1> && is_ranges_input_iter_v<Iterator2>,
                  "Iterators must be input_iterator");
    _NEFORCE swap(*a, *b);
}

/**
 * @brief 交换两个范围的元素
 * @tparam Iterator1 第一个范围迭代器类型
 * @tparam Iterator2 第二个范围迭代器类型
 * @param first1 第一个范围起始
 * @param last1 第一个范围结束
 * @param first2 第二个范围起始
 * @return 第二个范围结束迭代器
 *
 * 交换范围 [first1, last1) 和以 first2 开始的范围的对应元素。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 swap_ranges(Iterator1 first1, Iterator1 last1, Iterator2 first2) {
    for (; first1 != last1; ++first1, ++first2) {
        _NEFORCE iter_swap(first1, first2);
    }
    return first2;
}


/**
 * @brief 对范围元素应用函数
 * @tparam Iterator 迭代器类型
 * @tparam Function 函数类型
 * @param first 范围起始
 * @param last 范围结束
 * @param f 要应用的函数
 * @return 传入的函数对象
 *
 * 对范围 [first, last) 的每个元素应用函数 f。
 */
template <typename Iterator, typename Function>
constexpr Function for_each(Iterator first, Iterator last, Function f) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    static_assert(is_invocable_v<Function, decltype(*first)>, "Function must be invocable");

    for (; first != last; ++first) {
        f(*first);
    }
    return f;
}

/**
 * @brief 对指定数量的元素应用函数
 * @tparam Iterator 迭代器类型
 * @tparam Function 函数类型
 * @param first 起始迭代器
 * @param n 要处理的元素数量
 * @param f 要应用的函数
 * @return 处理后的迭代器
 */
template <typename Iterator, typename Function>
constexpr Iterator for_each_n(Iterator first, iter_difference_t<Iterator> n, Function f) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    static_assert(is_invocable_v<Function, decltype(*first)>, "Function must be invocable");

    for (iter_difference_t<Iterator> i = 0; i < n; ++i) {
        f(*first);
        ++first;
    }
    return first;
}


/**
 * @brief 用生成器的值填充范围
 * @tparam Iterator 迭代器类型
 * @tparam Generator 生成器类型
 * @param first 范围起始
 * @param last 范围结束
 * @param gen 生成器函数
 *
 * 对范围 [first, last) 的每个元素调用 gen() 并赋值。
 */
template <typename Iterator, typename Generator>
constexpr void generate(Iterator first, Iterator last, Generator gen) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    static_assert(is_invocable_v<Generator>, "Generator must be invocable");
    static_assert(is_assignable_v<decltype(*first), invoke_result_t<Generator>>,
                  "Generator must be assignable from invoke_result");

    for (; first != last; ++first) {
        *first = gen();
    }
}

/**
 * @brief 用生成器的值填充指定数量的元素
 * @tparam Iterator 迭代器类型
 * @tparam Generator 生成器类型
 * @param first 起始迭代器
 * @param n 要填充的元素数量
 * @param gen 生成器函数
 * @return 填充后的迭代器
 */
template <typename Iterator, typename Generator>
constexpr Iterator generate_n(Iterator first, iter_difference_t<Iterator> n, Generator gen) {
    static_assert(is_ranges_input_iter_v<Iterator>, "Iterator must be input_iterator");
    static_assert(is_invocable_v<Generator>, "Generator must be invocable");
    static_assert(is_assignable_v<decltype(*first), invoke_result_t<Generator>>,
                  "Generator must be assignable from invoke_result");

    for (; n > 0; --n, ++first) {
        *first = gen();
    }
    return first;
}


/**
 * @brief 替换并复制元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam T 值类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @param old_value 要替换的旧值
 * @param new_value 要替换的新值
 * @return 输出范围结束
 *
 * 将范围 [first, last) 的元素复制到 result，同时将等于 old_value 的元素替换为 new_value。
 */
template <typename Iterator1, typename Iterator2, typename T>
constexpr Iterator2 replace_copy(Iterator1 first, Iterator1 last, Iterator2 result, const T& old_value,
                                 const T& new_value) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>,
                  "Iterators must be forward_iterator");
    static_assert(is_assignable_v<decltype(*result), const T&>, "*result must be assignable from const T");

    for (; first != last; ++first, ++result) {
        *result = *first == old_value ? new_value : *first;
    }
    return result;
}

/**
 * @brief 根据谓词替换并复制元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam Predicate 谓词类型
 * @tparam T 值类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @param pred 替换谓词
 * @param new_value 要替换的新值
 * @return 输出范围结束
 *
 * 将范围 [first, last) 的元素复制到 result，同时将满足 pred 的元素替换为 new_value。
 */
template <typename Iterator1, typename Iterator2, typename Predicate, typename T>
constexpr Iterator2 replace_copy_if(Iterator1 first, Iterator1 last, Iterator2 result, Predicate pred,
                                    const T& new_value) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>,
                  "Iterators must be forward_iterator");
    static_assert(is_invocable_v<Predicate, decltype(*first)>, "Predicate must be invocable");
    static_assert(is_assignable_v<decltype(*result), const T&>, "*result must be assignable from const T");

    for (; first != last; ++first, ++result) {
        *result = pred(*first) ? new_value : *first;
    }
    return result;
}

/**
 * @brief 替换范围元素
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param old_value 要替换的旧值
 * @param new_value 要替换的新值
 *
 * 将范围 [first, last) 中等于 old_value 的所有元素替换为 new_value。
 */
template <typename Iterator, typename T>
constexpr void replace(Iterator first, Iterator last, const T& old_value, const T& new_value) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterators must be forward_iterator");
    static_assert(is_assignable_v<decltype(*first), const T&>, "*result must be assignable from const T");

    for (; first != last; ++first) {
        if (*first == old_value) {
            *first = new_value;
        }
    }
}

/**
 * @brief 根据谓词替换范围元素
 * @tparam Iterator 迭代器类型
 * @tparam Predicate 谓词类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param pred 替换谓词
 * @param new_value 要替换的新值
 *
 * 将范围 [first, last) 中满足 pred 的所有元素替换为 new_value。
 */
template <typename Iterator, typename Predicate, typename T>
constexpr void replace_if(Iterator first, Iterator last, Predicate pred, const T& new_value) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterators must be forward_iterator");
    static_assert(is_invocable_v<Predicate, decltype(*first)>, "Predicate must be invocable");
    static_assert(is_assignable_v<decltype(*first), const T&>, "*result must be assignable from const T");

    for (; first != last; ++first) {
        if (pred(*first)) {
            *first = new_value;
        }
    }
}

#ifndef NEFORCE_STANDARD_17
/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 反转辅助函数（随机访问迭代器版本）
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 */
template <typename Iterator>
enable_if_t<is_ranges_rnd_iter_v<Iterator>> __reverse_aux(Iterator first, Iterator last) {
    while (first < last) {
        --last;
        _NEFORCE iter_swap(first, last);
        ++first;
    }
    return;
}

/**
 * @brief 反转辅助函数（非随机访问迭代器版本）
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 */
template <typename Iterator>
enable_if_t<!is_ranges_rnd_iter_v<Iterator>> __reverse_aux(Iterator first, Iterator last) {
    while (true) {
        if (first == last || first == --last) {
            return;
        }
        --last;
        _NEFORCE iter_swap(first, last);
        ++first;
    }
    return;
}

NEFORCE_END_INNER__
/// @endcond
#endif // NEFORCE_STANDARD_17

/**
 * @brief 反转范围元素顺序
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 *
 * 将范围 [first, last) 的元素顺序反转。
 */
template <typename Iterator>
constexpr void reverse(Iterator first, Iterator last) {
    static_assert(is_ranges_bid_iter_v<Iterator>, "Iterators must be bidirectional_iterator");

#ifdef NEFORCE_STANDARD_17
    if constexpr (is_ranges_rnd_iter_v<Iterator>) {
        while (first < last) {
            --last;
            _NEFORCE iter_swap(first, last);
            ++first;
        }
    } else {
        while (first != last) {
            --last;
            if (first == last) {
                break;
            }
            _NEFORCE iter_swap(first, last);
            ++first;
        }
    }
#else
    inner::__reverse_aux(first, last);
#endif
}

/// @cond
NEFORCE_BEGIN_INNER__

template <typename Iterator>
constexpr enable_if_t<!is_ranges_bid_iter_v<Iterator>> __rotate_aux_dispatch(Iterator first, Iterator middle,
                                                                             Iterator last) {
    for (Iterator i = middle;;) {
        _NEFORCE iter_swap(first, i);
        ++first;
        ++i;
        if (first == middle) {
            if (i == last) {
                return;
            }
            middle = i;
        } else if (i == last) {
            i = middle;
        }
    }
}

template <typename Iterator>
constexpr enable_if_t<is_ranges_bid_iter_v<Iterator>> __rotate_aux_dispatch(Iterator first, Iterator middle,
                                                                            Iterator last) {
    _NEFORCE reverse(first, middle);
    _NEFORCE reverse(middle, last);
    _NEFORCE reverse(first, last);
}

/**
 * @brief 旋转辅助函数（非随机访问迭代器版本）
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param middle 旋转中心
 * @param last 范围结束
 */
template <typename Iterator>
constexpr enable_if_t<!is_ranges_rnd_iter_v<Iterator>> __rotate_aux(Iterator first, Iterator middle, Iterator last) {
    if (first == middle || middle == last) {
        return;
    }
    inner::__rotate_aux_dispatch(first, middle, last);
}

/**
 * @brief 旋转循环辅助函数
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @param initial 循环起始位置
 * @param shift 旋转步长
 */
template <typename Iterator>
constexpr void __rotate_cycle_aux(Iterator first, Iterator last, Iterator initial, iter_difference_t<Iterator> shift) {
    iter_value_t<Iterator> value = *initial;
    Iterator ptr1 = initial;
    Iterator ptr2 = ptr1 + shift;
    while (ptr2 != initial) {
        *ptr1 = *ptr2;
        ptr1 = ptr2;
        if (last - ptr2 > shift) {
            ptr2 += shift;
        } else {
            ptr2 = first + (shift - (last - ptr2));
        }
    }
    *ptr1 = _NEFORCE move(value);
}

/**
 * @brief 旋转辅助函数（随机访问迭代器版本）
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param middle 旋转中心
 * @param last 范围结束
 *
 * 使用循环旋转算法，基于最大公约数优化。
 */
template <typename Iterator>
constexpr enable_if_t<is_ranges_rnd_iter_v<Iterator>> __rotate_aux(Iterator first, Iterator middle, Iterator last) {
    auto n = _NEFORCE gcd(last - first, middle - first);
    while (n--) {
        inner::__rotate_cycle_aux(first, last, first + n, middle - first);
    }
    return;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 旋转范围元素
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param middle 旋转中心
 * @param last 范围结束
 * @return 旋转后原 first 所在位置的迭代器
 *
 * 将范围 [first, last) 旋转，使 middle 成为新的第一个元素。
 * 旋转后范围变为 [middle, last) + [first, middle)。
 */
template <typename Iterator>
constexpr Iterator rotate(Iterator first, Iterator middle, Iterator last) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterators must be forward_iterator");

    if (first == middle) {
        return last;
    }
    if (middle == last) {
        return first;
    }

    auto dist = _NEFORCE distance(middle, last);
    inner::__rotate_aux(first, middle, last);
    return _NEFORCE next(first, dist);
}

/**
 * @brief 旋转并复制元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param middle 旋转中心
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 *
 * 将旋转后的范围 [first, last) 复制到 result。
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 rotate_copy(Iterator1 first, Iterator1 middle, Iterator1 last, Iterator2 result) {
    return _NEFORCE copy(first, middle, _NEFORCE copy(middle, last, result));
}


/**
 * @brief 向左移位
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @param n 移位数量
 * @return 新逻辑末尾迭代器
 *
 * 将范围 [first, last) 的元素向左移动 n 个位置。
 * 移出的元素被移动，剩余末尾元素处于合法但未指定状态。
 * 如果 n >= distance(first, last)，则返回 first（空范围）。
 */
template <typename Iterator>
constexpr Iterator shift_left(Iterator first, Iterator last, iter_difference_t<Iterator> n) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterators must be forward_iterator");

    if (n <= 0) {
        return last;
    }

    auto dist = _NEFORCE distance(first, last);
    if (static_cast<decltype(dist)>(n) >= dist) {
        return first;
    }

    Iterator new_last = _NEFORCE next(first, dist - n);
    _NEFORCE move(_NEFORCE next(first, n), last, first);
    return new_last;
}


/**
 * @brief 向右移位
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @param n 移位数量
 * @return 新逻辑起始迭代器
 *
 * 将范围 [first, last) 的元素向右移动 n 个位置。
 * 移出的元素被移动，前部元素处于合法但未指定状态。
 * 如果 n >= distance(first, last)，则返回 last（空范围）。
 */
template <typename Iterator>
constexpr Iterator shift_right(Iterator first, Iterator last, iter_difference_t<Iterator> n) {
    static_assert(is_ranges_bid_iter_v<Iterator>, "Iterators must be bidirectional_iterator");

    if (n <= 0) {
        return first;
    }

    auto dist = _NEFORCE distance(first, last);
    if (static_cast<decltype(dist)>(n) >= dist) {
        return last;
    }

    Iterator new_first = _NEFORCE next(first, n);
    _NEFORCE move_backward(first, _NEFORCE next(first, dist - n), last);
    return new_first;
}


/**
 * @brief 对范围元素应用一元变换
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam UnaryOperation 一元操作类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @param op 一元操作函数
 * @return 输出范围结束
 *
 * 对范围 [first, last) 的每个元素应用 op，结果存储到 result。
 */
template <typename Iterator1, typename Iterator2, typename UnaryOperation>
constexpr Iterator2 transform(Iterator1 first, Iterator1 last, Iterator2 result,
                              UnaryOperation op) noexcept(noexcept(++first) && noexcept(++result) &&
                                                          noexcept(*result = op(*first))) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>,
                  "Iterators must be forward_iterator");
    static_assert(is_invocable_v<UnaryOperation, decltype(*first)>, "UnaryOperation must be invocable");
    static_assert(is_assignable_v<decltype(*result), invoke_result_t<UnaryOperation, decltype(*first)>>,
                  "*result must be assignable");

    for (; first != last; ++first, ++result) {
        *result = op(*first);
    }
    return result;
}

/**
 * @brief 对两个范围元素应用二元变换
 * @tparam Iterator1 第一个输入迭代器类型
 * @tparam Iterator2 第二个输入迭代器类型
 * @tparam Iterator3 输出迭代器类型
 * @tparam BinaryOperation 二元操作类型
 * @param first1 第一个输入范围起始
 * @param last1 第一个输入范围结束
 * @param first2 第二个输入范围起始
 * @param result 输出范围起始
 * @param binary_op 二元操作函数
 * @return 输出范围结束
 *
 * 对范围 [first1, last1) 和以 first2 开始的范围的对应元素应用 binary_op，
 * 结果存储到 result。
 */
template <typename Iterator1, typename Iterator2, typename Iterator3, typename BinaryOperation>
constexpr Iterator3 transform(Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator3 result,
                              BinaryOperation binary_op) noexcept(noexcept(++first1) && noexcept(first2) &&
                                                                  noexcept(++result) &&
                                                                  noexcept(*result = binary_op(*first1, *first2))) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2> && is_ranges_fwd_iter_v<Iterator3>,
                  "Iterators must be forward_iterator");
    static_assert(is_invocable_v<BinaryOperation, decltype(*first1), decltype(*first2)>,
                  "UnaryOperation must be invocable");
    static_assert(
            is_assignable_v<decltype(*result), invoke_result_t<BinaryOperation, decltype(*first1), decltype(*first2)>>,
            "*result must be assignable");

    for (; first1 != last1; ++first1, ++first2, ++result) {
        *result = binary_op(*first1, *first2);
    }
    return result;
}


/**
 * @brief 复制唯一元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @tparam BinaryPredicate 二元谓词类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @param binary_pred 相等性谓词
 * @return 输出范围结束
 *
 * 复制范围 [first, last) 中的元素到 result，跳过连续的重复元素。
 * 只有与前一个元素不重复的元素才会被复制。
 */
template <typename Iterator1, typename Iterator2, typename BinaryPredicate>
constexpr Iterator2 unique_copy(Iterator1 first, Iterator1 last, Iterator2 result, BinaryPredicate binary_pred) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>,
                  "Iterators must be forward_iterator");
    static_assert(is_invocable_v<BinaryPredicate, decltype(*result), decltype(*first)>,
                  "BinaryPredicate must be invocable");

    if (first == last) {
        return result;
    }

    *result = *first;

    while (++first != last) {
        if (!binary_pred(*result, *first)) {
            *++result = *first;
        }
    }
    return ++result;
}

/**
 * @brief 复制唯一元素（使用相等比较）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 */
template <typename Iterator1, typename Iterator2>
constexpr Iterator2 unique_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _NEFORCE unique_copy(first, last, result, _NEFORCE equal_to<iter_value_t<Iterator1>>());
}

/**
 * @brief 根据谓词移除连续重复元素
 * @tparam Iterator 迭代器类型
 * @tparam BinaryPredicate 二元谓词类型
 * @param first 范围起始
 * @param last 范围结束
 * @param binary_pred 相等性谓词
 * @return 新逻辑结束迭代器
 *
 * 使用指定的二元谓词判断元素是否相等，移除连续的重复元素。
 */
template <typename Iterator, typename BinaryPredicate>
constexpr Iterator unique(Iterator first, Iterator last, BinaryPredicate binary_pred) {
    first = _NEFORCE adjacent_find(first, last, binary_pred);
    if (first == last) {
        return last;
    }

    Iterator result = first;
    while (++first != last) {
        if (!binary_pred(*result, *first)) {
            *++result = _NEFORCE move(*first);
        }
    }
    return ++result;
}

/**
 * @brief 移除连续重复元素
 * @tparam Iterator 迭代器类型
 * @param first 范围起始
 * @param last 范围结束
 * @return 新逻辑结束迭代器
 *
 * 移除范围 [first, last) 中连续的重复元素，保留每个重复组的第一个元素。
 */
template <typename Iterator>
constexpr Iterator unique(Iterator first, Iterator last) {
    return _NEFORCE unique(first, last, _NEFORCE equal_to<iter_value_t<Iterator>>{});
}

/** @} */ // ShiftAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_SHIFT_HPP__
