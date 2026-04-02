#ifndef NEFORCE_CORE_ALGORITHM_HEAP_HPP__
#define NEFORCE_CORE_ALGORITHM_HEAP_HPP__

/**
 * @file heap.hpp
 * @brief 堆算法
 *
 * 此文件提供了堆算法实现，用于在随机访问容器上创建和操作二叉堆数据结构。
 */

#include "NeForce/core/algorithm/iterator.hpp"
#include "NeForce/core/functional/functor.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StandardAlgorithms 标准算法
 * @brief 基于迭代器的标准算法的实现
 * @{
 */

/**
 * @defgroup HeapAlgorithms 堆算法
 * @brief 堆算法的实现
 * @{
 */

/**
 * @brief 查找堆中破坏堆性质的首个元素
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param last 堆结束迭代器
 * @param comp 比较函数对象
 * @return 破坏堆性质的第一个元素的迭代器，如果范围是有效堆则返回 last
 *
 * 检查范围 [first, last) 是否满足堆性质，返回第一个破坏堆性质的元素。
 * 堆性质：
 *  - 对于最大堆，父节点不小于子节点；
 *  - 对于最小堆，父节点不大于子节点。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 Iterator is_heap_until(Iterator first, Iterator last, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    auto n = last - first;
    for (iter_difference_t<Iterator> child = 1; child < n; ++child) {
        auto parent = (child - 1) / 2;
        if (comp(*(first + parent), *(first + child))) {
            return first + child;
        }
    }
    return last;
}

/**
 * @brief 查找堆中破坏堆性质的首个元素
 * @tparam Iterator 随机访问迭代器类型
 * @param first 堆起始迭代器
 * @param last 堆结束迭代器
 * @return 破坏堆性质的第一个元素的迭代器，如果范围是有效堆则返回 last
 *
 * 默认使用小于比较，创建最大堆。
 */
template <typename Iterator> NEFORCE_CONSTEXPR20 Iterator is_heap_until(Iterator first, Iterator last) {
    return _NEFORCE is_heap_until(first, last, less<iter_value_t<Iterator>>());
}

/**
 * @brief 检查范围是否为有效堆
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @param comp 比较函数对象
 * @return 如果范围是有效堆则返回 true，否则返回 false
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 bool is_heap(Iterator first, Iterator last, Compare comp) {
    return _NEFORCE is_heap_until(first, last, comp) == last;
}

/**
 * @brief 检查范围是否为有效堆
 * @tparam Iterator 随机访问迭代器类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @return 如果范围是有效堆则返回 true，否则返回 false
 */
template <typename Iterator> NEFORCE_CONSTEXPR20 bool is_heap(Iterator first, Iterator last) {
    return _NEFORCE is_heap_until(first, last) == last;
}


/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 堆插入辅助函数
 * @tparam Iterator 随机访问迭代器类型
 * @tparam T 元素值类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param hole_index 空洞位置索引
 * @param top_index 堆顶索引
 * @param value 要插入的值
 * @param comp 比较函数对象
 *
 * 将新元素插入堆中的合适位置，通过向上调整维护堆性质。
 */
template <typename Iterator, typename T, typename Compare>
NEFORCE_CONSTEXPR20 void push_heap_aux(Iterator first, iter_difference_t<Iterator> hole_index,
                                       iter_difference_t<Iterator> top_index, T value, Compare comp) {
    static_assert(is_ranges_rnd_iter_v<Iterator>, "Iterator must be random_access_iterator");
    static_assert(is_invocable_v<Compare, decltype(*first), decltype(*first)>, "Compare must be invocable");

    auto parent = (hole_index - 1) / 2;
    while (hole_index > top_index && comp(*(first + parent), value)) {
        *(first + hole_index) = *(first + parent);
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    *(first + hole_index) = value;
}

/**
 * @brief 堆插入辅助函数
 */
template <typename Iterator, typename T>
NEFORCE_CONSTEXPR20 void push_heap_aux(Iterator first, iter_difference_t<Iterator> hole_index,
                                       iter_difference_t<Iterator> top_index, T value) {
    inner::push_heap_aux(first, hole_index, top_index, value, less<iter_value_t<Iterator>>());
}

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 向堆中插入元素
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param last 堆结束迭代器
 * @param comp 比较函数对象
 *
 * 假设范围 [first, last-1) 是有效堆，将 *(last-1) 插入堆中。
 * 调用后范围 [first, last) 是有效堆。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void push_heap(Iterator first, Iterator last, Compare comp) {
    if (last - first < 2) {
        return;
    }
    inner::push_heap_aux(first, last - first - 1, 0, *(last - 1), comp);
}

/**
 * @brief 向堆中插入元素
 */
template <typename Iterator> NEFORCE_CONSTEXPR20 void push_heap(Iterator first, Iterator last) {
    _NEFORCE push_heap(first, last, less<iter_value_t<Iterator>>());
}

/**
 * @brief 堆调整辅助函数
 * @tparam Iterator 随机访问迭代器类型
 * @tparam T 元素值类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param hole_index 空洞位置索引
 * @param len 堆长度
 * @param value 要调整的值
 * @param comp 比较函数对象
 *
 * 在指定位置向下调整堆，然后在调整后的位置向上调整。
 * 用于删除堆顶元素后的堆调整。
 */
template <typename Iterator, typename T, typename Compare>
NEFORCE_CONSTEXPR20 void adjust_heap(Iterator first, iter_difference_t<Iterator> hole_index,
                                     iter_difference_t<Iterator> len, T value, Compare comp) {
    auto top_index = hole_index;
    auto child = 2 * hole_index + 1;
    while (child < len) {
        if (child + 1 < len && comp(*(first + child), *(first + child + 1))) {
            ++child;
        }
        if (!comp(value, *(first + child))) {
            break;
        }
        *(first + hole_index) = *(first + child);
        hole_index = child;
        child = 2 * hole_index + 1;
    }
    inner::push_heap_aux(first, hole_index, top_index, value, comp);
}

/**
 * @brief 堆调整辅助函数
 */
template <typename Iterator, typename T>
NEFORCE_CONSTEXPR20 void adjust_heap(Iterator first, iter_difference_t<Iterator> hole_index,
                                     iter_difference_t<Iterator> len, T value) {
    _NEFORCE adjust_heap(first, hole_index, len, value, less<iter_value_t<Iterator>>());
}


/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 删除堆顶元素辅助函数
 * @tparam Iterator 随机访问迭代器类型
 * @tparam T 元素值类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param last 堆结束迭代器
 * @param result 存放堆顶元素的位置
 * @param value 最后一个元素的值
 * @param comp 比较函数对象
 *
 * 将堆顶元素移动到 result，用最后一个元素 value 调整堆。
 */
template <typename Iterator, typename T, typename Compare>
NEFORCE_CONSTEXPR20 void pop_heap_aux(Iterator first, Iterator last, Iterator result, T value, Compare comp) {
    *result = *first;
    _NEFORCE adjust_heap(first, 0, last - first, value, comp);
}

/**
 * @brief 删除堆顶元素辅助函数
 */
template <typename Iterator, typename T>
NEFORCE_CONSTEXPR20 void pop_heap_aux(Iterator first, Iterator last, Iterator result, T value) {
    inner::pop_heap_aux(first, last, result, value, less<iter_value_t<Iterator>>());
}

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 删除堆顶元素
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param last 堆结束迭代器
 * @param comp 比较函数对象
 *
 * 将堆的最大（或最小）元素移动到 last-1 位置，
 * 并使范围 [first, last-1) 成为有效堆。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void pop_heap(Iterator first, Iterator last, Compare comp) {
    if (last - first < 2) {
        return;
    }
    --last;
    inner::pop_heap_aux(first, last, last, *last, comp);
}

/**
 * @brief 删除堆顶元素
 */
template <typename Iterator> NEFORCE_CONSTEXPR20 void pop_heap(Iterator first, Iterator last) {
    _NEFORCE pop_heap(first, last, less<iter_value_t<Iterator>>());
}

/**
 * @brief 将堆转换为有序序列
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 堆起始迭代器
 * @param last 堆结束迭代器
 * @param comp 比较函数对象
 *
 * 通过反复弹出堆顶元素，将堆转换为升序（或降序）序列。
 * 操作后，范围 [first, last) 不再满足堆性质，而是已排序。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void sort_heap(Iterator first, Iterator last, Compare comp) {
    while (last - first > 1) {
        _NEFORCE pop_heap(first, last--, comp);
    }
}

/**
 * @brief 将堆转换为有序序列
 */
template <typename Iterator> NEFORCE_CONSTEXPR20 void sort_heap(Iterator first, Iterator last) {
    _NEFORCE sort_heap(first, last, less<iter_value_t<Iterator>>());
}

/**
 * @brief 创建堆
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 范围起始迭代器
 * @param last 范围结束迭代器
 * @param comp 比较函数对象
 *
 * 将范围 [first, last) 重新排列，使其满足堆性质。
 * 从最后一个非叶子节点开始，向前调整每个节点。
 */
template <typename Iterator, typename Compare>
NEFORCE_CONSTEXPR20 void make_heap(Iterator first, Iterator last, Compare comp) {
    if (last - first < 2) {
        return;
    }
    const auto len = last - first;
    if (len < 2) {
        return;
    }

    auto parent = (len - 2) / 2;
    while (true) {
        _NEFORCE adjust_heap(first, parent, len, *(first + parent), comp);
        if (parent == 0) {
            return;
        }
        --parent;
    }
}

/**
 * @brief 创建堆
 */
template <typename Iterator> NEFORCE_CONSTEXPR20 void make_heap(Iterator first, Iterator last) {
    _NEFORCE make_heap(first, last, less<iter_value_t<Iterator>>());
}

/** @} */ // HeapAlgorithms

/** @} */ // StandardAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ALGORITHM_HEAP_HPP__
