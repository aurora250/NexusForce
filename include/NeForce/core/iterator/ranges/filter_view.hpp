#ifndef NEFORCE_CORE_ITERATOR_RANGES_FILTER_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_FILTER_VIEW_HPP__

/**
 * @file filter_view.hpp
 * @brief 过滤视图
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
 * @class filter_iterator
 * @brief 过滤迭代器
 * @tparam BaseIter 底层迭代器类型
 * @tparam Sentinel 底层哨兵类型
 * @tparam Pred 一元谓词类型
 */
template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel, typename Pred>
class filter_iterator {
private:
    using base_category = iter_category_t<BaseIter>; ///< 底层迭代器类别

public:
    using iterator_category =
            conditional_t<is_base_of_v<bidirectional_iterator_tag, base_category>, bidirectional_iterator_tag,
                          conditional_t<is_base_of_v<forward_iterator_tag, base_category>, forward_iterator_tag,
                                        input_iterator_tag>>; ///< 迭代器类别

    using value_type = iter_value_t<BaseIter>;                 ///< 值类型
    using difference_type = iter_difference_t<BaseIter>;       ///< 差值类型
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>; ///< 指针类型
    using reference = iter_reference_t<BaseIter>;              ///< 引用类型

private:
    BaseIter base_begin_{};
    BaseIter current_{};
    Sentinel end_{};
    const Pred* pred_ = nullptr;

    constexpr void satisfy_predicate_forward() {
        while (current_ != end_ && !(*pred_)(*current_)) {
            ++current_;
        }
    }

public:
    /**
     * @brief 默认构造函数
     */
    constexpr filter_iterator() = default;

    /**
     * @brief 构造函数
     * @param base_begin 底层范围的起始迭代器
     * @param current 当前底层迭代器位置
     * @param end 底层哨兵
     * @param pred 指向谓词的指针
     */
    constexpr filter_iterator(BaseIter base_begin, BaseIter current, Sentinel end, const Pred* pred) :
    base_begin_(base_begin),
    current_(current),
    end_(end),
    pred_(pred) {
        satisfy_predicate_forward();
    }

    /**
     * @brief 解引用操作符
     * @return 当前元素
     */
    constexpr reference operator*() const { return *current_; }

    /**
     * @brief 成员访问操作符
     * @return 指向当前元素的指针
     */
    constexpr pointer operator->() const
        requires is_lvalue_reference_v<reference>
    {
        return _NEFORCE addressof(*current_);
    }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    constexpr filter_iterator& operator++() {
        ++current_;
        satisfy_predicate_forward();
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr filter_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 前置递减操作符
     * @return 递减后的迭代器引用
     */
    constexpr filter_iterator& operator--()
        requires bidirectional_iterator<BaseIter>
    {
        do {
            --current_;
        } while (current_ != base_begin_ && !(*pred_)(*current_));
        return *this;
    }

    /**
     * @brief 后置递减操作符
     * @return 递减前的迭代器副本
     */
    constexpr filter_iterator operator--(int)
        requires bidirectional_iterator<BaseIter>
    {
        auto tmp = *this;
        --(*this);
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一个 filter_iterator
     * @return 两者的当前底层迭代器相等则返回 true
     */
    constexpr bool operator==(const filter_iterator& other) const { return current_ == other.current_; }

    /**
     * @brief 不等比较操作符
     * @param other 另一个 filter_iterator
     * @return 两者的当前底层迭代器不相等则返回 true
     */
    constexpr bool operator!=(const filter_iterator& other) const { return !(*this == other); }

    /**
     * @brief 与哨兵比较的相等操作符
     * @tparam S 哨兵类型
     * @param s 哨兵对象
     * @return 当前底层迭代器与哨兵相等则返回 true
     */
    template <typename S>
        requires(!same_as<S, filter_iterator>)
    constexpr bool operator==(const S& s) const {
        return current_ == s;
    }

    /**
     * @brief 与哨兵比较的不等操作符
     * @tparam S 哨兵类型
     * @param s 哨兵对象
     * @return 当前底层迭代器与哨兵不相等则返回 true
     */
    template <typename S>
        requires(!same_as<S, filter_iterator>)
    constexpr bool operator!=(const S& s) const {
        return current_ != s;
    }

    /**
     * @brief 友元相等比较操作符
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵与迭代器当前底层位置相等则返回 true
     */
    friend constexpr bool operator==(Sentinel s, const filter_iterator& it) { return it.current_ == s; }

    /**
     * @brief 友元不等比较操作符
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵与迭代器当前底层位置不相等则返回 true
     */
    friend constexpr bool operator!=(Sentinel s, const filter_iterator& it) { return it.current_ != s; }

    /**
     * @brief 小于比较操作符
     * @param other 另一个 filter_iterator
     * @return 当前底层迭代器小于对方则返回 true
     */
    constexpr bool operator<(const filter_iterator& other) const
        requires totally_ordered<BaseIter>
    {
        return current_ < other.current_;
    }

    /**
     * @brief 大于比较操作符
     * @param other 另一个 filter_iterator
     * @return 当前底层迭代器大于对方则返回 true
     */
    constexpr bool operator>(const filter_iterator& other) const
        requires totally_ordered<BaseIter>
    {
        return current_ > other.current_;
    }

    /**
     * @brief 小于等于比较操作符
     * @param other 另一个 filter_iterator
     * @return 当前底层迭代器小于等于对方则返回 true
     */
    constexpr bool operator<=(const filter_iterator& other) const
        requires totally_ordered<BaseIter>
    {
        return current_ <= other.current_;
    }

    /**
     * @brief 大于等于比较操作符
     * @param other 另一个 filter_iterator
     * @return 当前底层迭代器大于等于对方则返回 true
     */
    constexpr bool operator>=(const filter_iterator& other) const
        requires totally_ordered<BaseIter>
    {
        return current_ >= other.current_;
    }

    /**
     * @brief 与哨兵比较的小于操作符
     * @param s 哨兵对象
     * @return 当前底层迭代器小于哨兵则返回 true
     */
    constexpr bool operator<(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return current_ < s;
    }

    /**
     * @brief 与哨兵比较的大于操作符
     * @param s 哨兵对象
     * @return 当前底层迭代器大于哨兵则返回 true
     */
    constexpr bool operator>(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return current_ > s;
    }

    /**
     * @brief 与哨兵比较的小于等于操作符
     * @param s 哨兵对象
     * @return 当前底层迭代器小于等于哨兵则返回 true
     */
    constexpr bool operator<=(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return current_ <= s;
    }

    /**
     * @brief 与哨兵比较的大于等于操作符
     * @param s 哨兵对象
     * @return 当前底层迭代器大于等于哨兵则返回 true
     */
    constexpr bool operator>=(Sentinel s) const
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return current_ >= s;
    }

    /**
     * @brief 友元小于比较操作符
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵小于迭代器当前底层位置则返回 true
     */
    friend constexpr bool operator<(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return s < it.current_;
    }

    /**
     * @brief 友元大于比较操作符
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵大于迭代器当前底层位置则返回 true
     */
    friend constexpr bool operator>(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return s > it.current_;
    }

    /**
     * @brief 友元小于等于比较操作符
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵小于等于迭代器当前底层位置则返回 true
     */
    friend constexpr bool operator<=(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return s <= it.current_;
    }

    /**
     * @brief 友元大于等于比较操作符
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵大于等于迭代器当前底层位置则返回 true
     */
    friend constexpr bool operator>=(Sentinel s, const filter_iterator& it)
        requires totally_ordered_with<BaseIter, Sentinel>
    {
        return s >= it.current_;
    }

    /**
     * @brief 计算与另一个迭代器的距离
     * @param other 被减去的另一个 filter_iterator
     * @return 两个位置之间满足谓词的元素个数
     */
    constexpr difference_type operator-(const filter_iterator& other) const
        requires sized_sentinel_for<BaseIter, BaseIter> && totally_ordered<BaseIter>
    {
        if (current_ == other.current_) {
            return 0;
        }
        difference_type total = 0;
        if (other.current_ < current_) {
            auto iter = other;
            while (iter.current_ != current_) {
                ++iter;
                ++total;
            }
            return total;
        }
        auto iter = *this;
        while (iter.current_ != other.current_) {
            ++iter;
            ++total;
        }
        return -total;
    }

    /**
     * @brief 获取当前底层迭代器
     * @return 当前底层迭代器
     */
    constexpr BaseIter base() const { return current_; }
};


/**
 * @class filter_view
 * @brief 过滤视图
 * @tparam V 底层视图类型
 * @tparam Pred 一元谓词类型
 */
template <View V, typename Pred>
class filter_view : public view_base<filter_view<V, Pred>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin()); ///< 底层可写迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>>().end());   ///< 底层可写哨兵类型
    using const_base_iterator = typename const_range_traits<V>::iterator;          ///< 底层 const 迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;          ///< 底层 const 哨兵类型

    using iterator = filter_iterator<base_iterator, base_sentinel, Pred>;                   ///< 迭代器类型
    using const_iterator = filter_iterator<const_base_iterator, const_base_sentinel, Pred>; ///< const 迭代器类型

    using sentinel = base_sentinel;             ///< 哨兵类型
    using const_sentinel = const_base_sentinel; ///< const 哨兵类型

private:
    V base_;
    Pred pred_;

public:
    /**
     * @brief 默认构造函数
     *
     * 要求底层视图与谓词均可默认构造。
     */
    constexpr filter_view() = default;

    /**
     * @brief 拷贝构造函数
     * @param other 源视图
     */
    constexpr filter_view(const filter_view& other)
        requires copy_constructible<V> && copy_constructible<Pred>
    :
    base_(other.base_),
    pred_(other.pred_) {}

    /**
     * @brief 拷贝赋值操作符
     * @param other 源视图
     * @return 自身引用
     */
    constexpr filter_view& operator=(const filter_view& other)
        requires copyable<V> && copyable<Pred>
    {
        if (addressof(other) == this) {
            return *this;
        }
        base_ = other.base_;
        pred_ = other.pred_;
        return *this;
    }

    /**
     * @brief 移动构造函数
     */
    constexpr filter_view(filter_view&&) = default;

    /**
     * @brief 移动赋值操作符
     * @param other 源视图
     * @return 自身引用
     */
    constexpr filter_view& operator=(filter_view&& other) noexcept(is_nothrow_move_assignable_v<V> &&
                                                                   is_nothrow_move_assignable_v<Pred>)
        requires is_move_assignable_v<V> && is_move_assignable_v<Pred>
    {
        base_ = _NEFORCE move(other.base_);
        pred_ = _NEFORCE move(other.pred_);
        return *this;
    }

    /**
     * @brief 构造函数
     * @param base 底层视图
     * @param pred 谓词
     */
    constexpr filter_view(V base, Pred pred) :
    base_(_NEFORCE move(base)),
    pred_(_NEFORCE move(pred)) {}

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个满足谓词元素的迭代器
     */
    constexpr iterator begin() { return iterator(base_.begin(), base_.begin(), base_.end(), &pred_); }

    /**
     * @brief 获取结束哨兵
     * @return 底层视图的哨兵
     */
    constexpr sentinel end() { return base_.end(); }

    /**
     * @brief 获取 const 起始迭代器
     * @return 指向第一个满足谓词元素的 const 迭代器
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), base_.begin(), base_.end(), &pred_);
    }

    /**
     * @brief 获取 const 结束哨兵
     * @return 底层视图的 const 哨兵
     */
    constexpr const_sentinel end() const
        requires Range<const V>
    {
        return base_.end();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     */
    constexpr iterator end()
        requires common_range<V>
    {
        return iterator(base_.begin(), base_.end(), base_.end(), &pred_);
    }

    /**
     * @brief 获取 const 结束迭代器
     * @return 指向末尾的 const 迭代器
     */
    constexpr const_iterator end() const
        requires common_range<const V>
    {
        return const_iterator(base_.begin(), base_.end(), base_.end(), &pred_);
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
     * @brief 移动取出底层视图
     * @return 被移动的底层视图
     */
    constexpr V base() && { return _NEFORCE move(base_); }
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_FILTER_VIEW_HPP__
