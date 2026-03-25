#ifndef NEFORCE_CORE_MEMORY_UNINITIALIZED_HPP__
#define NEFORCE_CORE_MEMORY_UNINITIALIZED_HPP__

/**
 * @file uninitialized.hpp
 * @brief 未初始化内存操作
 *
 * 此文件提供了未初始化内存操作实现，
 * 用于在未初始化的内存区域上进行构造、复制、移动和填充操作。
 */

#include "NeForce/core/algorithm/shift.hpp"
#include "NeForce/core/memory/construct.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup UninitializedMemoryOperations 未初始化内存操作
 * @brief 在未初始化内存上进行的安全操作
 * @{
 */

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 未初始化复制辅助函数（平凡可复制类型版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 *
 * 对于平凡可复制类型，可以直接使用copy算法。
 */
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator2 __uninitialized_copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _NEFORCE copy(first, last, result);
}

/**
 * @brief 未初始化复制辅助函数（非平凡类型版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 * @throws memory_exception 如果构造过程中发生异常
 *
 * 对于非平凡类型，需要构造每个元素并在异常时清理已构造的元素。
 * 如果发生异常，已构造的对象会被销毁。
 */
template <typename Iterator1, typename Iterator2, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator2 __uninitialized_copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    Iterator2 cur = result;
    try {
        for (; first != last; ++first, ++cur) {
            _NEFORCE construct(&*cur, *first);
        }
    } catch (...) {
        for (; result != cur; --cur) {
            _NEFORCE destroy(&*cur);
        }
        NEFORCE_THROW_EXCEPTION(memory_exception("uninitialized copy failed."));
    }
    return cur;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 复制元素到未初始化内存
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
 *
 * 将范围 [first, last) 的元素复制到未初始化的内存区域 [result, ...)。
 * 如果复制过程中抛出异常，已构造的元素会被析构。
 */
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_fwd_iter_v<Iterator2>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator2 uninitialized_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
    if (first == last) return result;
    return inner::__uninitialized_copy_aux(first, last, result);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 未初始化复制n个元素辅助函数（非随机访问迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要复制的元素数量
 * @param result 输出起始
 * @return pair<输入结束迭代器, 输出结束迭代器>
 * @throws memory_exception 如果构造过程中发生异常
 */
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
NEFORCE_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_copy_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator2 cur = result;
    try {
        for (; count > 0; --count, ++first, ++cur) {
            _NEFORCE construct(&*cur, *first);
        }
    } catch (...) {
        for (; result != cur; --cur) {
            _NEFORCE destroy(&*cur);
        }
        NEFORCE_THROW_EXCEPTION(memory_exception("uninitialized copy n failed."));
    }
    return pair<Iterator1, Iterator2>(first, cur);
}

/**
 * @brief 未初始化复制n个元素辅助函数（随机访问迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要复制的元素数量
 * @param result 输出起始
 * @return pair<输入结束迭代器, 输出结束迭代器>
 */
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
NEFORCE_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_copy_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator1 last = first + count;
    return _NEFORCE make_pair(last, _NEFORCE uninitialized_copy(first, last, result));
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 复制指定数量的元素到未初始化内存
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要复制的元素数量
 * @param result 输出起始
 * @return pair<输入结束迭代器, 输出结束迭代器>
 * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
 *
 * 从 first 开始复制 count 个元素到未初始化的内存区域。
 */
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_fwd_iter_v<Iterator2>, int> = 0>
NEFORCE_CONSTEXPR20 pair<Iterator1, Iterator2> uninitialized_copy_n(
    Iterator1 first, size_t count, Iterator2 result) {
    return inner::__uninitialized_copy_n_aux(first, count, result);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 未初始化填充辅助函数（平凡可复制类型版本）
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param x 要填充的值
 */
template <typename Iterator, typename T, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
NEFORCE_CONSTEXPR20 void __uninitialized_fill_aux(Iterator first, Iterator last, const T& x) {
    _NEFORCE fill(first, last, x);
}

/**
 * @brief 未初始化填充辅助函数（非平凡类型版本）
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param x 要填充的值
 * @throws memory_exception 如果构造过程中发生异常
 */
template <typename Iterator, typename T, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
NEFORCE_CONSTEXPR20 void __uninitialized_fill_aux(Iterator first, Iterator last, const T& x) {
    Iterator cur = first;
    try {
        for (; cur != last; ++cur) {
            _NEFORCE construct(&*cur, x);
        }
    } catch (...) {
        for (; cur != first; --cur) {
            _NEFORCE destroy(&*cur);
        }
        NEFORCE_THROW_EXCEPTION(memory_exception("uninitialized fill failed."));
    }
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 在未初始化内存上填充值
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 范围起始
 * @param last 范围结束
 * @param x 要填充的值
 * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
 *
 * 在未初始化的内存区域 [first, last) 上构造值为 x 的对象。
 * 如果构造过程中抛出异常，已构造的元素会被析构。
 */
template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
NEFORCE_CONSTEXPR20 void uninitialized_fill(Iterator first, Iterator last, const T& x) {
    if (first == last) return;
    inner::__uninitialized_fill_aux(first, last, x);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 未初始化填充n个元素辅助函数（平凡可复制类型版本）
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 起始位置
 * @param n 要填充的元素数量
 * @param x 要填充的值
 * @return 填充后的结束迭代器
 */
template <typename Iterator, typename T, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator __uninitialized_fill_n_aux(Iterator first, size_t n, const T& x) {
    return _NEFORCE fill_n(first, n, x);
}

/**
 * @brief 未初始化填充n个元素辅助函数（非平凡类型版本）
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 起始位置
 * @param n 要填充的元素数量
 * @param x 要填充的值
 * @return 填充后的结束迭代器
 * @throws memory_exception 如果构造过程中发生异常
 */
template <typename Iterator, typename T, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator>>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator __uninitialized_fill_n_aux(Iterator first, size_t n, const T& x) {
    Iterator cur = first;
    try{
        for (; n > 0; --n, ++cur) {
            _NEFORCE construct(&*cur, x);
        }
    } catch (...) {
        for (; cur != first; --cur) {
            _NEFORCE destroy(&*cur);
        }
        NEFORCE_THROW_EXCEPTION(memory_exception("uninitialized fill n failed."));
    }
    return cur;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 在未初始化内存中用指定值填充指定数量的元素
 * @tparam Iterator 迭代器类型
 * @tparam T 值类型
 * @param first 起始位置
 * @param n 要填充的元素数量
 * @param x 要填充的值
 * @return 填充后的结束迭代器
 * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
 *
 * 类似于 fill_n()，但在未初始化内存上操作。
 */
template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator uninitialized_fill_n(Iterator first, size_t n, const T& x) {
    return inner::__uninitialized_fill_n_aux(first, n, x);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 未初始化移动辅助函数（平凡可复制类型版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 */
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator2 __uninitialized_move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _NEFORCE move(first, last, result);
}

/**
 * @brief 未初始化移动辅助函数（非平凡可复制类型版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束
 * @throws memory_exception 如果构造过程中发生异常
 *
 * 使用移动构造函数构造对象，并可能使源对象处于有效但未指定的状态。
 */
template <typename Iterator1, typename Iterator2, enable_if_t<
    !is_trivially_copy_assignable_v<iter_value_t<Iterator1>>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator2 __uninitialized_move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    Iterator2 cur = result;
    try{
        for (; first != last; ++first, ++cur) {
            _NEFORCE construct(&*cur, _NEFORCE move(*first));
        }
    } catch (...) {
        for (; result != cur; --cur) {
            _NEFORCE destroy(&*cur);
        }
        NEFORCE_THROW_EXCEPTION(memory_exception("uninitialized move failed."));
    }
    return cur;
}
NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 在未初始化内存中移动元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入范围起始
 * @param last 输入范围结束
 * @param result 输出范围起始
 * @return 输出范围结束迭代器
 * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
 *
 * 将范围 [first, last) 的元素移动到以 result 开始的未初始化内存区域。
 * 使用移动构造函数构造对象，可能使源对象处于有效但未指定的状态。
 */
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
NEFORCE_CONSTEXPR20 Iterator2 uninitialized_move(Iterator1 first, Iterator1 last, Iterator2 result) {
    if (first == last) return result;
    return inner::__uninitialized_move_aux(first, last, result);
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 未初始化移动n个元素辅助函数（非随机访问迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要移动的元素数量
 * @param result 输出范围起始
 * @return pair<输入结束迭代器, 输出结束迭代器>
 * @throws memory_exception 如果构造过程中发生异常
 */
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
NEFORCE_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_move_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator2 cur = result;
    try{
        for (; count > 0; --count, ++first, ++cur) {
            _NEFORCE construct(&*cur, _NEFORCE move(*first));
        }
    } catch (...) {
        for (; result != cur; --cur) {
            _NEFORCE destroy(&*cur);
        }
        NEFORCE_THROW_EXCEPTION(memory_exception("uninitialized move n failed."));
    }
    return pair<Iterator1, Iterator2>(first, cur);
}

/**
 * @brief 未初始化移动n个元素辅助函数（随机访问迭代器版本）
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始
 * @param count 要移动的元素数量
 * @param result 输出范围起始
 * @return pair<输入结束迭代器, 输出结束迭代器>
 */
template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
NEFORCE_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_move_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator1 last = first + count;
    return _NEFORCE make_pair(last, _NEFORCE uninitialized_move(first, last, result));
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 在未初始化内存中移动指定数量的元素
 * @tparam Iterator1 输入迭代器类型
 * @tparam Iterator2 输出迭代器类型
 * @param first 输入起始迭代器
 * @param count 要移动的元素数量
 * @param result 输出范围起始
 * @return pair<输入结束迭代器, 输出结束迭代器>
 * @throws memory_exception 当值类型为非平凡类型时，如果构造过程中发生异常
 */
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
NEFORCE_CONSTEXPR20 pair<Iterator1, Iterator2> uninitialized_move_n(
    Iterator1 first, size_t count, Iterator2 result) {
    return inner::__uninitialized_move_n_aux(first, count, result);
}

/** @} */ // UninitializedAlgorithms

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_UNINITIALIZED_HPP__
