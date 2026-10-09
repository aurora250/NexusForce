#ifndef NEFORCE_CORE_ITERATOR_RANGES_ENUMERATE_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_ENUMERATE_VIEW_HPP__

/**
 * @file enumerate_view.hpp
 * @brief 为元素附带下标的视图
 */

#include "NeForce/core/iterator/ranges/core.hpp"
#include "NeForce/core/utility/pair.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @class enumerate_iterator
 * @brief 附带下标视图的迭代器
 * @tparam BaseIter 底层迭代器类型
 * @tparam Sentinel 底层哨兵类型
 */
template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel>
class enumerate_iterator {
public:
    using iterator_category = conditional_t<is_base_of_v<forward_iterator_tag, iter_category_t<BaseIter>>,
                                            forward_iterator_tag, input_iterator_tag>; ///< 迭代器类别
    using value_type = pair<size_t, iter_value_t<BaseIter>>;                           ///< 元素类型
    using reference = pair<size_t, iter_reference_t<BaseIter>>;                        ///< 元素引用类型
    using difference_type = iter_difference_t<BaseIter>;                               ///< 迭代器距离类型
    using pointer = void;                                                              ///< 元素指针类型

    /**
     * @brief 默认构造函数
     */
    constexpr enumerate_iterator() = default;

    /**
     * @brief 构造附带下标的迭代器
     * @param current 当前底层迭代器
     * @param end 底层哨兵
     * @param index 当前下标
     */
    constexpr enumerate_iterator(BaseIter current, Sentinel end, size_t index) :
    current_(current),
    end_(end),
    index_(index) {}

    /**
     * @brief 解引用迭代器，得到下标与元素组成的 pair
     * @return 下标与元素组成的 pair
     */
    NEFORCE_NODISCARD constexpr reference operator*() const { return reference{index_, *current_}; }

    /**
     * @brief 前置递增，同时推进底层迭代器与下标
     * @return 递增后的迭代器引用
     */
    constexpr enumerate_iterator& operator++() {
        ++current_;
        ++index_;
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr enumerate_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较，只比较当前底层迭代器
     * @param other 另一个迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool operator==(const enumerate_iterator& other) const {
        return current_ == other.current_;
    }

    /**
     * @brief 不等比较
     * @param other 另一个迭代器
     * @return 是否不相等
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const enumerate_iterator& other) const { return !(*this == other); }

    /**
     * @brief 获取当前下标
     * @return 当前下标
     */
    NEFORCE_NODISCARD constexpr size_t index() const { return index_; }

private:
    BaseIter current_{};
    Sentinel end_{};
    size_t index_ = 0;
};


/**
 * @class enumerate_view
 * @brief 为元素附带下标的视图
 * @tparam V 底层视图类型
 */
template <View V>
class enumerate_view : public view_base<enumerate_view<V>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin()); ///< 底层迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>&>().end());   ///< 底层哨兵类型
    using const_base_iterator = typename const_range_traits<V>::iterator;           ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;           ///< 底层常量哨兵类型

    using iterator = enumerate_iterator<base_iterator, base_sentinel>;                   ///< 迭代器类型
    using const_iterator = enumerate_iterator<const_base_iterator, const_base_sentinel>; ///< 常量迭代器类型

    using sentinel = iterator;             ///< 哨兵类型
    using const_sentinel = const_iterator; ///< 常量哨兵类型

private:
    V base_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr enumerate_view() = default;

    /**
     * @brief 构造附带下标的视图
     * @param base 底层视图
     */
    constexpr explicit enumerate_view(V base) :
    base_(_NEFORCE move(base)) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() { return iterator(base_.begin(), base_.end(), 0); }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() { return sentinel(base_.end(), base_.end(), 0); }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), base_.end(), 0);
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const
        requires Range<const V>
    {
        return const_sentinel(base_.end(), base_.end(), 0);
    }

    /**
     * @brief 获取元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD constexpr size_t size() const
        requires sized_view<remove_cvref_t<V>>
    {
        return static_cast<size_t>(base_.size());
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
enumerate_view(V&&) -> enumerate_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_ENUMERATE_VIEW_HPP__
