#ifndef NEFORCE_CORE_ITERATOR_RANGES_CHUNK_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_CHUNK_VIEW_HPP__

/**
 * @file chunk_view.hpp
 * @brief 按固定长度切分的视图
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
 * @class chunk_iterator
 * @brief 分块视图的迭代器
 * @tparam BaseIter 底层迭代器类型
 * @tparam Sentinel 底层哨兵类型
 *
 * 每次解引用返回一个长度不超过 chunk 的子范围，递增时整体前进 chunk
 */
template <forward_iterator BaseIter, sentinel_for<BaseIter> Sentinel>
class chunk_iterator {
public:
    using iterator_category = forward_iterator_tag;      ///< 迭代器类别
    using value_type = subrange_view<BaseIter>;          ///< 元素类型
    using difference_type = iter_difference_t<BaseIter>; ///< 迭代器距离类型
    using pointer = void;                                ///< 元素指针类型
    using reference = subrange_view<BaseIter>;           ///< 元素引用类型

    /**
     * @brief 默认构造函数
     */
    constexpr chunk_iterator() = default;

    /**
     * @brief 构造分块迭代器
     * @param current 当前块起始迭代器
     * @param end 底层哨兵
     * @param chunk 每块的目标长度
     */
    constexpr chunk_iterator(BaseIter current, Sentinel end, difference_type chunk) :
    current_(current),
    end_(end),
    chunk_(chunk > 0 ? chunk : 1),
    window_end_(current) {
        satisfy();
    }

    /**
     * @brief 解引用迭代器，得到当前块
     * @return 当前块
     */
    NEFORCE_NODISCARD constexpr reference operator*() const { return subrange_view<BaseIter>{current_, window_end_}; }

    /**
     * @brief 前置递增，整体前进一块
     * @return 递增后的迭代器引用
     */
    constexpr chunk_iterator& operator++() {
        current_ = window_end_;
        satisfy();
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr chunk_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较，只比较当前块起始迭代器
     * @param other 另一个迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool operator==(const chunk_iterator& other) const {
        return current_ == other.current_;
    }

    /**
     * @brief 不等比较
     * @param other 另一个迭代器
     * @return 是否不相等
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const chunk_iterator& other) const { return !(*this == other); }

private:
    constexpr void satisfy() {
        window_end_ = current_;
        difference_type remaining = chunk_;
        while (window_end_ != end_ && remaining > 0) {
            ++window_end_;
            --remaining;
        }
    }

    BaseIter current_{};
    Sentinel end_{};
    difference_type chunk_ = 1;
    BaseIter window_end_{};
};


/**
 * @class chunk_view
 * @brief 按固定长度切分的视图
 * @tparam V 底层视图类型
 *
 * 把底层范围切分为若干不重叠的定长子范围
 */
template <View V>
    requires forward_iterator<decltype(_NEFORCE declval<remove_const_t<V>&>().begin())>
class chunk_view : public view_base<chunk_view<V>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin());  ///< 底层迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>&>().end());    ///< 底层哨兵类型
    using const_base_iterator = typename const_range_traits<V>::iterator;            ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;            ///< 底层常量哨兵类型
    using iterator = chunk_iterator<base_iterator, base_sentinel>;                   ///< 迭代器类型
    using const_iterator = chunk_iterator<const_base_iterator, const_base_sentinel>; ///< 常量迭代器类型
    using sentinel = iterator;                                                       ///< 哨兵类型
    using const_sentinel = const_iterator;                                           ///< 常量哨兵类型
    using difference_type = iter_difference_t<base_iterator>;                        ///< 迭代器距离类型

private:
    V base_;
    difference_type chunk_ = 1;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr chunk_view() = default;

    /**
     * @brief 构造分块视图
     * @param base 底层视图
     * @param chunk 每块长度，小于 1 时按 1 处理
     */
    constexpr chunk_view(V base, difference_type chunk) :
    base_(_NEFORCE move(base)),
    chunk_(chunk > 0 ? chunk : 1) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() { return iterator(base_.begin(), base_.end(), chunk_); }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() { return sentinel(base_.end(), base_.end(), chunk_); }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), base_.end(), chunk_);
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const
        requires Range<const V>
    {
        return const_sentinel(base_.end(), base_.end(), chunk_);
    }

    /**
     * @brief 获取块数
     * @return 块数
     */
    NEFORCE_NODISCARD constexpr size_t size() const
        requires sized_view<remove_cvref_t<V>>
    {
        const auto total = static_cast<size_t>(base_.size());
        const auto chunk = static_cast<size_t>(chunk_);
        return total == 0 ? 0 : (total + chunk - 1) / chunk;
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
chunk_view(V&&, iter_difference_t<decltype(_NEFORCE declval<V>().begin())>)
        -> chunk_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_CHUNK_VIEW_HPP__
