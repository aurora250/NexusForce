#ifndef NEFORCE_CORE_ITERATOR_RANGES_JOIN_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_JOIN_VIEW_HPP__

/**
 * @file join_view.hpp
 * @brief 展平嵌套范围的视图
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
 * @brief 展平嵌套范围的迭代器
 * @tparam OuterIter 外层范围的迭代器类型
 * @tparam OuterSentinel 外层范围的哨兵类型
 */
template <typename OuterIter, typename OuterSentinel>
class join_iterator {
private:
    using outer_reference = iter_reference_t<OuterIter>;
    using inner_range = conditional_t<is_lvalue_reference_v<outer_reference>, remove_reference_t<outer_reference>,
                                      remove_cvref_t<outer_reference>>;
    using inner_iterator = decltype(_NEFORCE declval<inner_range>().begin());
    using inner_sentinel = decltype(_NEFORCE declval<inner_range>().end());

public:
    using iterator_category = forward_iterator_tag;                  ///< 迭代器类别
    using value_type = iter_value_t<inner_iterator>;                 ///< 值类型
    using difference_type = iter_difference_t<inner_iterator>;       ///< 差值类型
    using pointer = add_pointer_t<iter_reference_t<inner_iterator>>; ///< 指针类型
    using reference = iter_reference_t<inner_iterator>;              ///< 引用类型

    /**
     * @brief 默认构造函数
     */
    constexpr join_iterator() = default;

    /**
     * @brief 构造函数
     * @param outer_current 外层范围起始迭代器
     * @param outer_end 外层范围哨兵
     */
    constexpr join_iterator(OuterIter outer_current, OuterSentinel outer_end) :
    outer_current_(outer_current),
    outer_end_(outer_end) {
        if (outer_current_ != outer_end_) {
            inner_current_ = (*outer_current_).begin();
            inner_end_ = (*outer_current_).end();
            satisfy();
        }
    }

    /**
     * @brief 解引用操作符
     * @return 当前内层范围当前元素的引用
     */
    constexpr reference operator*() const { return *inner_current_; }
    /**
     * @brief 成员访问操作符
     * @return 指向当前元素的指针
     */
    constexpr pointer operator->() const { return _NEFORCE addressof(*inner_current_); }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    constexpr join_iterator& operator++() {
        ++inner_current_;
        satisfy();
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr join_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一展平迭代器
     * @return 外层位置相同返回 true
     */
    constexpr bool operator==(const join_iterator& other) const {
        return outer_current_ == other.outer_current_ &&
               (outer_current_ == outer_end_ || inner_current_ == other.inner_current_);
    }
    /**
     * @brief 不等比较操作符
     * @param other 另一展平迭代器
     * @return 是否不相等
     */
    constexpr bool operator!=(const join_iterator& other) const { return !(*this == other); }

private:
    constexpr void satisfy() {
        while (outer_current_ != outer_end_ && inner_current_ == inner_end_) {
            ++outer_current_;
            if (outer_current_ != outer_end_) {
                inner_current_ = (*outer_current_).begin();
                inner_end_ = (*outer_current_).end();
            }
        }
    }

    OuterIter outer_current_{};
    OuterSentinel outer_end_{};
    inner_iterator inner_current_{};
    inner_sentinel inner_end_{};
};

/**
 * @brief 展平嵌套范围的视图
 * @tparam V 外层视图类型，其元素本身必须是范围
 */
template <View V>
    requires Range<iter_reference_t<decltype(_NEFORCE declval<V>().begin())>> && common_range<V>
class join_view : public view_base<join_view<V>> {
public:
    using outer_iterator = decltype(_NEFORCE declval<V>().begin()); ///< 外层迭代器类型
    using outer_sentinel = decltype(_NEFORCE declval<V>().end());   ///< 外层哨兵类型
    using iterator = join_iterator<outer_iterator, outer_sentinel>; ///< 迭代器类型

    /**
     * @brief 默认构造函数
     */
    constexpr join_view() = default;
    /**
     * @brief 构造函数
     * @param base 底层嵌套范围视图
     */
    constexpr explicit join_view(V base) :
    base_(_NEFORCE move(base)) {}

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个非空内层范围首元素的迭代器
     */
    constexpr iterator begin() { return iterator(base_.begin(), base_.end()); }
    /**
     * @brief 获取末尾迭代器
     * @return 外层已到达末尾的迭代器
     */
    constexpr iterator end() { return iterator(base_.end(), base_.end()); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个非空内层范围首元素的迭代器
     */
    constexpr iterator begin() const
        requires Range<const V>
    {
        return iterator(base_.begin(), base_.end());
    }
    /**
     * @brief 获取常量末尾迭代器
     * @return 外层已到达末尾的迭代器
     */
    constexpr iterator end() const
        requires Range<const V>
    {
        return iterator(base_.end(), base_.end());
    }

    /**
     * @brief 获取底层视图的副本
     * @return 底层视图的拷贝
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

private:
    V base_;
};

template <typename V>
join_view(V&&) -> join_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_JOIN_VIEW_HPP__
