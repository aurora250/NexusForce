#ifndef NEFORCE_CORE_ITERATOR_RANGES_SPLIT_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_SPLIT_VIEW_HPP__

/**
 * @file split_view.hpp
 * @brief 按分隔元素切分的视图
 */

#include "NeForce/core/iterator/ranges/core.hpp"
#include "NeForce/core/iterator/ranges/subrange_view.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @brief 按分隔元素切分后的迭代器
 * @tparam IsConst 是否使用常量迭代器，true 时取底层视图的 const 迭代器
 * @tparam V 底层视图类型，需满足 View
 * @tparam T 分隔元素类型
 */
template <bool IsConst, View V, typename T>
struct split_iterator {
private:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin());
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>>().end());
    using const_base_iterator = typename const_range_traits<V>::iterator;
    using const_base_sentinel = typename const_range_traits<V>::sentinel;
    using iterator = conditional_t<IsConst, const_base_iterator, base_iterator>;
    using sentinel = conditional_t<IsConst, const_base_sentinel, base_sentinel>;

public:
    iterator current_;            ///< 当前游标所在的底层迭代器，指向下一个子范围的起点
    sentinel end_;                ///< 底层范围的末尾 sentinel
    T delimiter_;                 ///< 分隔元素
    bool trailing_empty_ = false; ///< 是否还需产出一个尾随空子范

    using iterator_category = forward_iterator_tag;      ///< 迭代器类别
    using value_type = subrange_view<iterator>;          ///< 值类型
    using difference_type = iter_difference_t<iterator>; ///< 差值类型
    using pointer = void;                                ///< 指针类型
    using reference = value_type;                        ///< 引用类型

    split_iterator() = default;

    /**
     * @brief 由底层游标、末尾 sentinel 与分隔元素构造
     * @param cur 底层当前游标，作为子范围的起点
     * @param end 底层范围的末尾 sentinel
     * @param delim 分隔元素
     */
    split_iterator(iterator cur, sentinel end, T delim) :
    current_(cur),
    end_(end),
    delimiter_(delim) {}

    /**
     * @brief 解引用，得到当前子范围
     * @return 从当前游标扫描到下一个
     */
    value_type operator*() const {
        if (trailing_empty_) {
            return value_type{end_, end_};
        }
        iterator start = current_;
        iterator iter = current_;
        while (iter != end_ && !(*iter == delimiter_)) {
            ++iter;
        }
        return value_type{start, iter};
    }

    /**
     * @brief 前置递增，前进到下一个子范围
     * @return 递增后的自身引用
     */
    split_iterator& operator++() {
        if (current_ == end_) {
            trailing_empty_ = false;
            return *this;
        }

        iterator next = current_;
        while (next != end_ && !(*next == delimiter_)) {
            ++next;
        }

        if (next != end_) {
            ++next;
            trailing_empty_ = (next == end_);
        } else {
            trailing_empty_ = false;
        }

        current_ = next;
        return *this;
    }

    /**
     * @brief 后置递增，前进到下一个子范围
     * @return 递增前的迭代器副本
     */
    split_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 判断两个迭代器是否相等
     * @param other 待比较的迭代器
     * @return 游标与 trailing_empty_ 都相同时返回 true
     */
    bool operator==(const split_iterator& other) const {
        return current_ == other.current_ && trailing_empty_ == other.trailing_empty_;
    }

    /**
     * @brief 判断两个迭代器是否不相等
     * @param other 待比较的迭代器
     * @return 不相等时返回 true
     */
    bool operator!=(const split_iterator& other) const { return !(*this == other); }
};

/**
 * @brief 按分隔元素切分的 view
 * @tparam V 底层视图类型，需满足 View
 * @tparam T 分隔元素类型
 */
template <View V, typename T>
class split_view : public view_base<split_view<V, T>> {
    V base_;
    T delimiter_;

public:
    using iterator = split_iterator<false, V, T>;      ///< 迭代器类型
    using const_iterator = split_iterator<true, V, T>; ///< 常量迭代器类型

    split_view() = default;

    /**
     * @brief 由底层视图与分隔元素构造
     * @param base 底层视图，按值移动持有
     * @param delimiter 分隔元素，按值拷贝持有
     */
    split_view(V base, T delimiter) :
    base_(_NEFORCE move(base)),
    delimiter_(delimiter) {}

    /**
     * @brief 获取指向首个切分段的迭代器
     * @return 游标位于底层开头、trailing_empty_ 为 false 的迭代器
     */
    iterator begin() { return iterator{base_.begin(), base_.end(), delimiter_}; }

    /**
     * @brief 获取末尾迭代器
     * @return 游标位于底层末尾、trailing_empty_ 为 false 的迭代器
     */
    iterator end() { return iterator{base_.end(), base_.end(), delimiter_}; }

    /**
     * @brief 获取指向首个切分段的常量迭代器
     * @return 使用底层 const 迭代器构造的迭代器
     */
    const_iterator begin() const { return const_iterator{base_.begin(), base_.end(), delimiter_}; }

    /**
     * @brief 获取末尾常量迭代器
     * @return 游标位于底层末尾的常量迭代器
     */
    const_iterator end() const { return const_iterator{base_.end(), base_.end(), delimiter_}; }
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_SPLIT_VIEW_HPP__
