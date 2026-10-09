#ifndef NEFORCE_CORE_ITERATOR_RANGES_SLICE_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_SLICE_VIEW_HPP__

/**
 * @file slice_view.hpp
 * @brief 按下标区间截取的视图
 */

#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @brief 按下标区间截取的 view
 * @tparam V 底层视图类型，需满足 View
 */
template <View V>
class slice_view : public view_base<slice_view<V>> {
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin());
    using const_base_iterator = typename const_range_traits<V>::iterator;
    using iterator = base_iterator;
    using const_iterator = const_base_iterator;
    using difference_type = iter_difference_t<base_iterator>;

    V base_;
    difference_type start_ = 0;
    difference_type end_ = 0;
    mutable difference_type offset_ = 0;
    mutable difference_type length_ = 0;
    mutable bool has_value_ = false;

    void ensure_cache() const {
        if (has_value_) {
            return;
        }

        auto& self = const_cast<remove_const_t<V>&>(base_);
        difference_type total = 0;
        for (auto iter = self.begin(); iter != self.end(); ++iter) {
            ++total;
        }

        difference_type first = start_ > 0 ? start_ : 0;
        if (first > total) {
            first = total;
        }
        difference_type last = end_ > first ? end_ : first;
        if (last > total) {
            last = total;
        }

        offset_ = first;
        length_ = last - first;
        has_value_ = true;
    }

    template <typename CV>
    static auto advance_to(CV& range, difference_type count) {
        auto iter = range.begin();
        for (difference_type index = 0; index < count; ++index) {
            ++iter;
        }
        return iter;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 底层视图为值初始化，区间下标为 [0, 0)，即默认构造得到空视图。
     */
    slice_view() = default;

    /**
     * @brief 由底层视图与下标区间构造
     * @param base 底层视图，按值移动持有
     * @param start 起始下标（含），非正值按 0 处理
     * @param end 结束下标（不含），大于 start 的部分才被计入
     */
    constexpr slice_view(V base, difference_type start, difference_type end) :
    base_(_NEFORCE move(base)),
    start_(start),
    end_(end) {}

    /**
     * @brief 获取截取区间的起始迭代器
     * @return 底层范围中位置为 start_ 的迭代器
     */
    iterator begin() {
        ensure_cache();
        return advance_to(base_, offset_);
    }

    /**
     * @brief 获取结束迭代器
     * @return 区间结束迭代器
     */
    iterator end() {
        ensure_cache();
        return advance_to(base_, offset_ + length_);
    }

    /**
     * @brief 获取起始常量迭代器
     * @return 区间起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V>
    {
        ensure_cache();
        return advance_to(base_, offset_);
    }

    /**
     * @brief 获取结束常量迭代器
     * @return 区间结束常量迭代器
     */
    const_iterator end() const
        requires Range<const V>
    {
        ensure_cache();
        return advance_to(base_, offset_ + length_);
    }

    /**
     * @brief 获取元素个数
     * @return 区间长度
     */
    NEFORCE_NODISCARD size_t size() const
        requires sized_view<remove_cvref_t<V>>
    {
        ensure_cache();
        return static_cast<size_t>(length_);
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


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_SLICE_VIEW_HPP__
