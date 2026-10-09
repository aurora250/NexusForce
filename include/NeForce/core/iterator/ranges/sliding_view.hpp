#ifndef NEFORCE_CORE_ITERATOR_RANGES_SLIDING_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_SLIDING_VIEW_HPP__

/**
 * @file sliding_view.hpp
 * @brief 滑动窗口视图
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
 * @class sliding_iterator
 * @brief 滑动窗口视图的迭代器
 * @tparam BaseIter 底层迭代器类型
 * @tparam Sentinel 底层哨兵类型
 *
 * 每次解引用返回长度为窗口宽度的子范围，递增时整体前进一个元素，相邻窗口相互重叠
 */
template <forward_iterator BaseIter, sentinel_for<BaseIter> Sentinel>
class sliding_iterator {
public:
    using iterator_category = forward_iterator_tag;      ///< 迭代器类别
    using value_type = subrange_view<BaseIter>;          ///< 元素类型
    using difference_type = iter_difference_t<BaseIter>; ///< 迭代器距离类型
    using pointer = void;                                ///< 元素指针类型
    using reference = subrange_view<BaseIter>;           ///< 元素引用类型

    /**
     * @brief 默认构造函数
     */
    constexpr sliding_iterator() = default;

    /**
     * @brief 构造滑动窗口迭代器
     * @param current 当前窗口起始迭代器
     * @param end 底层哨兵
     * @param window 窗口宽度
     */
    constexpr sliding_iterator(BaseIter current, Sentinel end, difference_type window) :
    current_(current),
    end_(end),
    window_(window > 1 ? window : 1),
    window_end_(current),
    done_(false) {
        satisfy();
    }

    /**
     * @brief 解引用迭代器，得到当前窗口
     * @return 当前窗口
     */
    NEFORCE_NODISCARD constexpr reference operator*() const { return subrange_view<BaseIter>{current_, window_end_}; }

    /**
     * @brief 前置递增，窗口整体前移一个元素
     * @return 递增后的迭代器引用
     */
    constexpr sliding_iterator& operator++() {
        ++current_;
        satisfy();
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr sliding_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较，同时比较窗口起始位置与结束标志
     * @param other 另一个迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool operator==(const sliding_iterator& other) const {
        return done_ == other.done_ && (done_ || current_ == other.current_);
    }

    /**
     * @brief 不等比较
     * @param other 另一个迭代器
     * @return 是否不相等
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const sliding_iterator& other) const { return !(*this == other); }

private:
    constexpr void satisfy() {
        if (current_ == end_) {
            done_ = true;
            return;
        }
        window_end_ = current_;
        difference_type remaining = window_;
        while (window_end_ != end_ && remaining > 0) {
            ++window_end_;
            --remaining;
        }
        done_ = remaining > 0;
    }

    BaseIter current_{};
    Sentinel end_{};
    difference_type window_ = 1;
    BaseIter window_end_{};
    bool done_ = false;
};


/**
 * @class sliding_view
 * @brief 滑动窗口视图
 * @tparam V 底层视图类型
 *
 * 产出所有长度为窗口宽度的连续子范围，步长为 1，相邻窗口重叠
 */
template <View V>
    requires forward_iterator<decltype(_NEFORCE declval<remove_const_t<V>&>().begin())>
class sliding_view : public view_base<sliding_view<V>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin()); ///< 底层迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>&>().end());   ///< 底层哨兵类型
    using const_base_iterator = typename const_range_traits<V>::iterator;           ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;           ///< 底层常量哨兵类型

    using iterator = sliding_iterator<base_iterator, base_sentinel>;                   ///< 迭代器类型
    using const_iterator = sliding_iterator<const_base_iterator, const_base_sentinel>; ///< 常量迭代器类型

    using sentinel = iterator;                                ///< 哨兵类型
    using const_sentinel = const_iterator;                    ///< 常量哨兵类型
    using difference_type = iter_difference_t<base_iterator>; ///< 迭代器距离类型

private:
    V base_;
    difference_type window_ = 1;

public:
    constexpr sliding_view() = default;

    /**
     * @brief 构造滑动窗口视图
     * @param base 底层视图
     * @param window 窗口宽度，小于 1 时按 1 处理
     */
    constexpr sliding_view(V base, difference_type window) :
    base_(_NEFORCE move(base)),
    window_(window > 1 ? window : 1) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() { return iterator(base_.begin(), base_.end(), window_); }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() { return sentinel(base_.end(), base_.end(), window_); }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), base_.end(), window_);
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const
        requires Range<const V>
    {
        return const_sentinel(base_.end(), base_.end(), window_);
    }

    /**
     * @brief 获取窗口个数
     * @return 窗口个数
     */
    NEFORCE_NODISCARD constexpr size_t size() const
        requires sized_view<remove_cvref_t<V>>
    {
        const auto total = static_cast<size_t>(base_.size());
        const auto window = static_cast<size_t>(window_);
        return total < window ? 0 : total - window + 1;
    }

    /**
     * @brief 获取底层视图
     * @return 底层视图的副本
     */
    constexpr V base() const&
        requires copy_constructible<V>
    {
        return base_;
    }

    /**
     * @brief 获取底层视图
     * @return 底层视图
     */
    constexpr V base() && { return _NEFORCE move(base_); }
};

template <typename V>
sliding_view(V&&, iter_difference_t<decltype(_NEFORCE declval<V>().begin())>)
        -> sliding_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_SLIDING_VIEW_HPP__
