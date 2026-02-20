#ifndef MSTL_CORE_ALGORITHM_EXT_SORT_HPP__
#define MSTL_CORE_ALGORITHM_EXT_SORT_HPP__

/**
 * @file ext_sort.hpp
 * @brief 扩展排序算法集合
 *
 * 此文件提供了各种排序算法的实现，
 * 包括基础排序、线性时间排序、混合排序和娱乐性排序算法。
 */

#include "MSTL/core/algorithm/leonardo_heap.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup SortAlgorithms 排序算法
 * @brief MSTL排序算法的实现
 * @{
 */

/**
 * @brief 冒泡排序
 * @tparam Iterator 双向迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O(N²)，最优O(N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：稳定
 *
 * 通过重复交换相邻的逆序元素将最大元素冒泡到末尾。
 */
template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void bubble_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    auto revend = _MSTL make_reverse_iterator(first);
    auto revstart = _MSTL make_reverse_iterator(--last);
    for (auto iter = revstart; iter != revend; ++iter) {
        bool not_finished = false;
        Iterator curend = iter.base();
        for (Iterator it = first; it != curend; ++it) {
            Iterator next = it;
            ++next;
            if (comp(*next, *it)) {
                _MSTL iter_swap(it, next);
                not_finished = true;
            }
        }
        if (!not_finished) break;
    }
}

/**
 * @brief 冒泡排序（默认升序）
 * @tparam Iterator 双向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void bubble_sort(Iterator first, Iterator last) {
    return _MSTL bubble_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/**
 * @brief 鸡尾酒排序（双向冒泡排序）
 * @tparam Iterator 双向迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O(N²)，最优O(N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：稳定
 *
 * 冒泡排序的改进版本，同时从两端进行冒泡，减少循环次数。
 */
template <typename Iterator, typename Compare, enable_if_t<is_ranges_bid_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void cocktail_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    bool swapped = true;
    Iterator left = first;
    Iterator right = last;
    --right;
    while (swapped) {
        swapped = false;
        for (Iterator i = left; i != right; ++i) {
            Iterator next = i;
            ++next;
            if (comp(*next, *i)) {
                _MSTL iter_swap(i, next);
                swapped = true;
            }
        }
        if (!swapped) break;
        --right;
        swapped = false;
        for (Iterator i = right; i != left; --i) {
            Iterator prev = i;
            --prev;
            if (comp(*i, *prev)) {
                _MSTL iter_swap(i, prev);
                swapped = true;
            }
        }
        ++left;
    }
}

/**
 * @brief 鸡尾酒排序（默认升序）
 * @tparam Iterator 双向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void cocktail_sort(Iterator first, Iterator last) {
    return _MSTL cocktail_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/**
 * @brief 选择排序
 * @tparam Iterator 前向迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：O(N²)
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 每次从未排序部分选择元素放到已排序部分的末尾。
 */
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void select_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    Iterator min;
    for (Iterator i = first; i != last; ++i) {
        min = i;
        for (Iterator j = i + 1; j != last; ++j) {
            if (comp(*j, *min)) {
                min = j;
            }
        }
        _MSTL iter_swap(i, min);
    }
}

/**
 * @brief 选择排序（默认升序）
 * @tparam Iterator 前向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void select_sort(Iterator first, Iterator last) {
    return _MSTL select_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/**
 * @brief 希尔排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O(N log N)，最差O(N²)
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 插入排序的改进版本，通过比较相距一定间隔的元素来工作。
 */
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void shell_sort(Iterator first, Iterator last, Compare comp) {
    if (first == last) return;
    using Distance = iter_difference_t<Iterator>;
    using T = iter_value_t<Iterator>;
    Distance dist = _MSTL distance(first, last);
    for (Distance gap = dist / 2; gap > 0; gap /= 2) {
        for (Iterator i = first + gap; i < last; ++i) {
            T temp = *i;
            Iterator j;
            for (j = i; j >= first + gap && comp(temp, *(j - gap)); j -= gap)
                *j = *(j - gap);
            *j = temp;
        }
    }
}

/**
 * @brief 希尔排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void shell_sort(Iterator first, Iterator last) {
    return _MSTL shell_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/**
 * @brief 计数排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @tparam IndexMapper 索引映射函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 * @param mapper 将元素映射为整数索引的函数
 *
 * 时间复杂度：O(N + k)，其中k是元素范围
 * 空间复杂度：O(k)
 * 稳定性：稳定
 *
 * 适用于整数或可映射为整数的类型，范围不宜过大。
 */
template <typename Iterator, typename Compare, typename IndexMapper, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void counting_sort(Iterator first, Iterator last, Compare comp, IndexMapper mapper) {
    if (first == last) return;
    auto min_max = _MSTL minmax_element(first, last, comp);
    auto min_val = mapper(*min_max.first);
    auto max_val = mapper(*min_max.second);
    const auto range = static_cast<size_t>(max_val - min_val + 1);
    vector<int> count(range, 0);

    for (Iterator it = first; it != last; ++it) {
        auto value = mapper(*it);
        if (value < min_val || value > max_val) {
            throw_exception(iterator_exception("element out of range for counting sort."));
        }
        ++count[static_cast<size_t>(value - min_val)];
    }

    for (size_t i = 1; i < count.size(); ++i)
        count[i] += count[i - 1];

    vector<iter_value_t<Iterator>> sorted(_MSTL distance(first, last));
    auto bound = _MSTL make_reverse_iterator(first);

    for (auto rit = _MSTL make_reverse_iterator(last); rit != bound; ++rit) {
        auto value = mapper(*rit);
        const auto index = static_cast<size_t>(value - min_val);
        size_t position = --count[index];
        sorted[position] = *rit;
    }
    _MSTL copy(sorted.begin(), sorted.end(), first);
}

/**
 * @brief 计数排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void counting_sort(Iterator first, Iterator last) {
    _MSTL counting_sort(first, last, 
        _MSTL less<iter_value_t<Iterator>>(), _MSTL identity<iter_value_t<Iterator>>());
}

/**
 * @brief 桶排序（升序）
 * @tparam Iterator 前向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 *
 * 时间复杂度：平均O(N + k)，最差O(N²)
 * 空间复杂度：O(N + k)
 * 稳定性：稳定
 *
 * 适用于均匀分布的整数或浮点数。
 */
template <typename Iterator, enable_if_t<
    is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void bucket_sort_less(Iterator first, Iterator last) {
    using T = iter_value_t<Iterator>;
    pair<Iterator, Iterator> min_max = _MSTL minmax_element(first, last);
    T min_val = *min_max.first;
    T max_val = *min_max.second;
    T range = max_val - min_val + 1;
    vector<size_t> bucket(range, 0);

    for (Iterator it = first; it != last; ++it) {
        ++bucket[*it - min_val];
    }

    Iterator index = first;

    for (size_t i = 0; i < bucket.size(); ++i) {
        while (bucket[i] > 0) {
            *index++ = static_cast<T>(i + min_val);
            --bucket[i];
        }
    }
}

/**
 * @brief 桶排序（降序）
 * @tparam Iterator 前向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator, enable_if_t<
    is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void bucket_sort_greater(Iterator first, Iterator last) {
    using T = iter_value_t<Iterator>;
    pair<Iterator, Iterator> min_max = _MSTL minmax_element(first, last);
    T min_val = *min_max.first;
    T max_val = *min_max.second;
    T range = max_val - min_val + 1;
    vector<size_t> bucket(range, 0);

    for (Iterator it = first; it != last; ++it) {
        ++bucket[*it - min_val];
    }

    Iterator index = first;

    for (size_t i = bucket.size(); i-- > 0; ) {
        while (bucket[i] > 0) {
            *index++ = static_cast<T>(i + min_val);
            --bucket[i];
        }
    }
}

/**
 * @brief 桶排序（默认升序）
 * @tparam Iterator 前向迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void bucket_sort(Iterator first, Iterator last) {
    _MSTL bucket_sort_less(first, last);
}

/// @cond
MSTL_BEGIN_INNER__

template <typename Iterator>
int __max_bit_aux(Iterator first, Iterator last) {
    auto max_num = *_MSTL max_element(first, last,
        [](const auto& a, const auto& b) -> bool { return a < b; });
    int p = 0;
    while (max_num > 0) {
        p++;
        max_num = max_num / 10;
    }
    return p;
}

template <typename T>
int __get_number_aux(T num, T d) {
    int p = 1;
    for (T i = 1; i < d; ++i)
        p *= 10;
    return num / p % 10;
}

MSTL_END_INNER__
/// @endcond

/**
 * @brief 基数排序（升序）
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Mapper 映射函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param mapper 将元素映射为整数的函数
 *
 * 时间复杂度：O(d * (N + k))，d是位数
 * 空间复杂度：O(N + k)
 * 稳定性：稳定
 *
 * 适用于整数或可分解为固定数位的类型。
 */
template <typename Iterator, typename Mapper, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void radix_sort_less(Iterator first, Iterator last, Mapper mapper) {
    if (first == last) return;
    using Mapped = remove_reference_t<decltype(mapper(*first))>;

    iter_difference_t<Iterator> length = _MSTL distance(first, last);
    vector<Mapped> mapped_values(length);
    vector<iter_value_t<Iterator>> bucket(length);
    vector<int> count(10);
    Iterator it = first;

    for (auto& value : mapped_values) {
        value = mapper(*it++);
    }

    for (int d = 1; d <= _INNER __max_bit_aux(mapped_values.begin(), mapped_values.end()); ++d) {
        _MSTL fill(count.begin(), count.end(), 0);
        for(const auto& num : mapped_values) {
            ++count[_INNER __get_number_aux(num, d)];
        }

        for (size_t i = 1; i < count.size(); ++i) {
            count[i] += count[i - 1];
        }
        for (auto iter = mapped_values.rbegin(); iter != mapped_values.rend(); ++iter) {
            const int k = _INNER __get_number_aux(*iter, d);
            bucket[--count[k]] = *(first + _MSTL distance(mapped_values.begin(), iter.base() - 1));
        }

        it = first;
        for(const auto& value : bucket) {
            *it++ = value;
        }
        _MSTL transform(bucket.begin(), bucket.end(), mapped_values.begin(), mapper);
    }
}

/**
 * @brief 基数排序（降序）
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Mapper 映射函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param mapper 将元素映射为整数的函数
 */
template <typename Iterator, typename Mapper, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void radix_sort_greater(Iterator first, Iterator last, Mapper mapper) {
    if (first == last) return;
    using Mapped = remove_cvref_t<decltype(mapper(*first))>;

    iter_difference_t<Iterator> length = _MSTL distance(first, last);
    vector<Mapped> mapped_values(length);
    vector<iter_value_t<Iterator>> bucket(length);
    vector<int> count(10);
    Iterator it = first;

    for (auto& value : mapped_values) {
        value = mapper(*it++);
    }
    for (int d = 1; d <= _INNER __max_bit_aux(mapped_values.begin(), mapped_values.end()); ++d) {
        _MSTL fill(count.begin(), count.end(), 0);
        for(const auto& num : mapped_values) {
            ++count[_INNER __get_number_aux(*num, d)];
        }

        for (size_t i = count.size() - 1; i > 0; --i) {
            count[i - 1] += count[i];
        }

        for (auto iter = mapped_values.rbegin(); iter != mapped_values.rend(); ++iter) {
            const int k = _INNER __get_number_aux(*iter, d);
            bucket[--count[k]] = *(first + _MSTL distance(mapped_values.begin(), iter.base() - 1));
        }

        it = first;
        for(const auto& value : bucket) {
            *it++ = value;
        }
        _MSTL transform(bucket.begin(), bucket.end(), mapped_values.begin(), mapper);
    }
}

/**
 * @brief 基数排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Mapper 映射函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param mapper 将元素映射为整数的函数
 */
template <typename Iterator, typename Mapper = _MSTL identity<iter_value_t<Iterator>>>
MSTL_CONSTEXPR20 void radix_sort(Iterator first, Iterator last, Mapper mapper = Mapper()) {
    _MSTL radix_sort_less(first, last, mapper);
}

/**
 * @brief 平滑排序
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 基于莱昂纳多堆的排序算法，是堆排序的改进版本，
 * 在部分有序的序列上表现优异。
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void smooth_sort(Iterator first, Iterator last) {
    _MSTL make_leonardo_heap(first, last);
    _MSTL sort_leonardo_heap(first, last);
}

/**
 * @brief Tim排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：O(N log N)
 * 空间复杂度：O(N)
 * 稳定性：稳定
 *
 * 混合排序算法，结合了归并排序和插入排序。
 */
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void tim_sort(Iterator first, Iterator last, Compare comp) {
    constexpr int MIN_MERGE = 32;
    iter_difference_t<Iterator> n = _MSTL distance(first, last);
    for (Iterator i = first; i < last; i += MIN_MERGE) {
        Iterator end = _MSTL min(i + MIN_MERGE, last);
        _MSTL insertion_sort(i, end, comp);
    }
    for (int size = MIN_MERGE; size < n; size *= 2) {
        for (Iterator left = first; left < last; left += 2 * size) {
            Iterator mid = left + size;
            Iterator right = _MSTL min(left + 2 * size, last);
            if (mid < right) {
                _MSTL inplace_merge(left, mid, right, comp);
            }
        }
    }
}

/**
 * @brief Tim排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
MSTL_CONSTEXPR20 void tim_sort(Iterator first, Iterator last) {
    return _MSTL tim_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/**
 * @brief 猴子排序
 * @tparam Iterator 随机访问迭代器类型
 * @tparam Compare 比较函数类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 * @param comp 比较函数对象
 *
 * 时间复杂度：平均O((N+1)!)，理论上无限
 * 空间复杂度：O(1)
 * 稳定性：不稳定
 *
 * 通过随机打乱并检查是否有序来进行排序。
 * 仅用于教学和娱乐目的，切勿用于实际生产环境。
 */
template <typename Iterator, typename Compare, enable_if_t<
    is_ranges_rnd_iter_v<Iterator>, int> = 0>
void monkey_sort(Iterator first, Iterator last, Compare comp) {
    while (!_MSTL is_sorted(first, last, comp)) {
        _MSTL shuffle(first, last);
    }
}

/**
 * @brief 猴子排序（默认升序）
 * @tparam Iterator 随机访问迭代器类型
 * @param first 序列起始迭代器
 * @param last 序列结束迭代器
 */
template <typename Iterator>
void monkey_sort(Iterator first, Iterator last) {
    return _MSTL monkey_sort(first, last, _MSTL less<iter_value_t<Iterator>>());
}

/** @} */ // SortAlgorithms

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_EXT_SORT_HPP__
