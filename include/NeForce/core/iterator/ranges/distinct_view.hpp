#ifndef NEFORCE_CORE_ITERATOR_RANGES_DISTINCT_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_DISTINCT_VIEW_HPP__

/**
 * @file distinct_view.hpp
 * @brief 按元素或键去重的视图
 */

#include "NeForce/core/functional/functor.hpp"
#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @class distinct_iterator
 * @brief 去重视图的迭代器
 * @tparam BaseIter 底层迭代器类型
 * @tparam Sentinel 底层哨兵类型
 * @tparam Key 键函数类型
 * @tparam Set 记录已出现键的集合类型
 *
 * 构造与递增时反复推进底层迭代器，跳过已经出现过的键，直到命中新的键或到达末尾。
 */
template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel, typename Key, typename Set>
class distinct_iterator {
public:
    using iterator_category = forward_iterator_tag;            ///< 迭代器类别
    using value_type = iter_value_t<BaseIter>;                 ///< 元素类型
    using difference_type = iter_difference_t<BaseIter>;       ///< 迭代器距离类型
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>; ///< 元素指针类型
    using reference = iter_reference_t<BaseIter>;              ///< 元素引用类型

    /**
     * @brief 默认构造函数
     */
    constexpr distinct_iterator() = default;

    /**
     * @brief 构造去重迭代器
     * @param current 当前底层迭代器
     * @param end 底层哨兵
     * @param key 键函数的指针，指向视图内部持有的键函数
     * @param seen 已出现键集合的指针，指向视图内部持有的集合
     */
    constexpr distinct_iterator(BaseIter current, Sentinel end, const Key* key, Set* seen) :
    current_(current),
    end_(end),
    key_(key),
    seen_(seen) {
        satisfy();
    }

    /**
     * @brief 解引用迭代器
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD constexpr reference operator*() const { return *current_; }

    /**
     * @brief 获取元素指针
     * @return 指向当前元素的指针
     */
    NEFORCE_NODISCARD constexpr pointer operator->() const { return _NEFORCE addressof(*current_); }

    /**
     * @brief 前置递增，跳到下一个键未出现过的元素
     * @return 递增后的迭代器引用
     */
    constexpr distinct_iterator& operator++() {
        ++current_;
        satisfy();
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr distinct_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较，只比较当前底层迭代器
     * @param other 另一个迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool operator==(const distinct_iterator& other) const {
        return current_ == other.current_;
    }

    /**
     * @brief 不等比较
     * @param other 另一个迭代器
     * @return 是否不相等
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const distinct_iterator& other) const { return !(*this == other); }

private:
    constexpr void satisfy() {
        while (current_ != end_) {
            if (seen_->insert((*key_)(*current_)).second) {
                return;
            }
            ++current_;
        }
    }

    BaseIter current_{};
    Sentinel end_{};
    const Key* key_ = nullptr;
    Set* seen_ = nullptr;
};


/**
 * @class distinct_view
 * @brief 按元素或键去重的视图
 * @tparam V 底层视图类型
 * @tparam Key 键函数类型
 * @tparam Set 记录已出现键的集合类型
 */
template <View V, typename Key, typename Set>
class distinct_view : public view_base<distinct_view<V, Key, Set>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin()); ///< 底层迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>&>().end());   ///< 底层哨兵类型
    using const_base_iterator = typename const_range_traits<V>::iterator;           ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;           ///< 底层常量哨兵类型

    using iterator = distinct_iterator<base_iterator, base_sentinel, Key, Set>;                   ///< 迭代器类型
    using const_iterator = distinct_iterator<const_base_iterator, const_base_sentinel, Key, Set>; ///< 常量迭代器类型

    using sentinel = iterator;             ///< 哨兵类型
    using const_sentinel = const_iterator; ///< 常量哨兵类型

private:
    V base_;
    Key key_;
    mutable Set seen_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr distinct_view() = default;

    /**
     * @brief 构造去重视图
     * @param base 底层视图
     * @param key 键函数
     */
    constexpr distinct_view(V base, Key key) :
    base_(_NEFORCE move(base)),
    key_(_NEFORCE move(key)) {}

    /**
     * @brief 获取起始迭代器，同时清空判重状态
     * @return 起始迭代器
     */
    iterator begin() {
        seen_.clear();
        return iterator(base_.begin(), base_.end(), &key_, &seen_);
    }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() { return sentinel(base_.end(), base_.end(), &key_, &seen_); }

    /**
     * @brief 获取起始常量迭代器，同时清空判重状态
     * @return 起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V>
    {
        seen_.clear();
        return const_iterator(base_.begin(), base_.end(), &key_, &seen_);
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const
        requires Range<const V>
    {
        return const_sentinel(base_.end(), base_.end(), &key_, &seen_);
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
#endif // NEFORCE_CORE_ITERATOR_RANGES_DISTINCT_VIEW_HPP__
