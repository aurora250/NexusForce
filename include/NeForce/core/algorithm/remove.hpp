#ifndef NEFORCE_CORE_ALGORITHM_REMOVE_HPP__
#define NEFORCE_CORE_ALGORITHM_REMOVE_HPP__

/**
 * @file remove.hpp
 * @brief 删除算法
 *
 * 此文件提供了删除算法实现，
 * 用于从序列中移除满足特定条件的元素。
 */

#include "NeForce/core/algorithm/search.hpp"
#include "NeForce/core/utility/reference_wrapper.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup RemoveAlgorithms 删除算法
 * @brief 删除算法的实现
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
template <typename Iterator1, typename Iterator2, typename T>
constexpr Iterator2 remove_copy(Iterator1 first, Iterator1 last, Iterator2 result, const T& value) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>,
                  "Iterator must be forward_iterator");

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
template <typename Iterator1, typename Iterator2, typename Predicate>
constexpr Iterator2 remove_copy_if(Iterator1 first, Iterator1 last, Iterator2 result, Predicate pred) {
    static_assert(is_ranges_fwd_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>,
                  "Iterator must be forward_iterator");

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
template <typename Iterator, typename T>
constexpr Iterator remove(Iterator first, Iterator last, const T& value) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

    first = _NEFORCE find(first, last, value);
    if (first != last) {
        for (Iterator i = first; ++i != last;) {
            if (!(*i == value)) {
                *first = _NEFORCE move(*i);
                ++first;
            }
        }
    }
    return first;
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
template <typename Iterator, typename Predicate>
constexpr Iterator remove_if(Iterator first, Iterator last, Predicate pred) {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "Iterator must be forward_iterator");

    first = _NEFORCE find_if(first, last, pred);
    if (first != last) {
        for (Iterator i = first; ++i != last;) {
            if (!pred(*i)) {
                *first = _NEFORCE move(*i);
                ++first;
            }
        }
    }
    return first;
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
 */
template <typename Container, typename U>
constexpr size_t erase(Container& cont, const U& value) {
    using value_type = typename Container::value_type;

    const auto old_size = cont.size();
    const auto end = cont.end();
    auto removed = _NEFORCE remove_if(cont.begin(), end, [&value](const value_type& iter) { return iter == value; });
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
    using value_type = typename Container::value_type;

    const size_t old_size = cont.size();
    const auto end = cont.end();
    auto removed = _NEFORCE remove_if(
            cont.begin(), end, [ref_pred = _NEFORCE ref(pred)](const value_type& iter) { return ref_pred(iter); });
    cont.erase(removed, end);
    return old_size - cont.size();
}

/** @} */ // RemoveAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_REMOVE_HPP__
