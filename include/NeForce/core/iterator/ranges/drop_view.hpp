#ifndef NEFORCE_CORE_ITERATOR_RANGES_DROP_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_DROP_VIEW_HPP__

/**
 * @file drop_view.hpp
 * @brief 丢弃前若干个元素的视图
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
 * @brief 丢弃前若干个元素的 view
 * @tparam V 底层视图类型，需满足 View
 */
template <View V>
class drop_view : public view_base<drop_view<V>> {
public:
    using iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin()); ///< 迭代器类型
    using sentinel = decltype(_NEFORCE declval<remove_const_t<V>>().end());   ///< sentinel 类型
    using const_iterator = typename const_range_traits<V>::iterator;          ///< 常量迭代器类型
    using const_sentinel = typename const_range_traits<V>::sentinel;          ///< 常量 sentinel 类型
    using difference_type = iter_difference_t<iterator>;                      ///< 差值类型

private:
    V base_;
    difference_type count_ = 0;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr drop_view() = default;

    /**
     * @brief 由底层视图与计数构造
     * @param base 底层视图，按值移动持有
     * @param count 要丢弃的前缀长度，非正值表示不丢弃任何元素
     */
    constexpr drop_view(V base, difference_type count) :
    base_(_NEFORCE move(base)),
    count_(count > 0 ? count : 0) {}

    /**
     * @brief 移动构造函数
     */
    constexpr drop_view(drop_view&&)
        requires movable<V>
    = default;

    /**
     * @brief 移动赋值运算符
     * @return 移动赋值后的自身引用
     */
    constexpr drop_view& operator=(drop_view&&)
        requires movable<V>
    = default;

    /**
     * @brief 拷贝构造函数
     */
    constexpr drop_view(const drop_view&)
        requires copy_constructible<V>
    = default;

    /**
     * @brief 拷贝赋值运算符
     * @return 拷贝赋值后的自身引用
     */
    constexpr drop_view& operator=(const drop_view&)
        requires copyable<V>
    = default;

    /**
     * @brief 获取指向首个保留元素的迭代器
     * @return 从底层起始迭代器开始跳过 count 个元素后的迭代器
     */
    constexpr iterator begin() {
        auto it = base_.begin();
        auto end = base_.end();
        auto n = count_;
        while (n > 0 && it != end) {
            ++it;
            --n;
        }
        return it;
    }

    /**
     * @brief 获取末尾 sentinel
     * @return 底层视图的 sentinel
     */
    constexpr sentinel end() { return base_.end(); }

    /**
     * @brief 获取指向首个保留元素的常量迭代器
     * @return 从底层常量起始迭代器开始跳过 count 个元素后的迭代器
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        auto it = base_.begin();
        auto end = base_.end();
        auto n = count_;
        while (n > 0 && it != end) {
            ++it;
            --n;
        }
        return it;
    }

    /**
     * @brief 获取视图大小
     * @return max(底层大小 - 计数, 0)，计数大于底层大小时返回 0
     */
    constexpr auto size() const
        requires requires(const V& v) {
            { v.size() } -> convertible_to<size_t>;
        }
    {
        const auto total = static_cast<difference_type>(base_.size());
        const difference_type clamped_total = total > 0 ? total : 0;
        const difference_type dropped = count_ > 0 ? count_ : 0;
        return static_cast<size_t>(dropped < clamped_total ? clamped_total - dropped : 0);
    }

    /**
     * @brief 获取末尾常量 sentinel
     * @return 底层视图的常量 sentinel
     */
    constexpr const_sentinel end() const
        requires Range<const V>
    {
        return base_.end();
    }

    /**
     * @brief 获取底层视图的拷贝
     * @return 底层视图的副本
     */
    constexpr V base() const&
        requires copy_constructible<V>
    {
        return base_;
    }

    /**
     * @brief 移出底层视图
     * @return 被移动的底层视图
     */
    constexpr V base() && { return _NEFORCE move(base_); }
};

template <typename V>
drop_view(V&&, iter_difference_t<decltype(_NEFORCE declval<V>().begin())>)
        -> drop_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_DROP_VIEW_HPP__
