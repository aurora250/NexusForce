#ifndef NEFORCE_CORE_ITERATOR_RANGES_TAKE_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_TAKE_VIEW_HPP__

/**
 * @file take_view.hpp
 * @brief 取前若干个元素的视图
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
 * @brief 取前 N 个元素的 iterator
 * @tparam BaseIter 底层迭代器类型
 */
template <input_iterator BaseIter>
class take_iterator {
public:
    using iterator_category = conditional_t<is_base_of_v<forward_iterator_tag, iter_category_t<BaseIter>>,
                                            forward_iterator_tag, input_iterator_tag>; ///< 迭代器类别
    using value_type = iter_value_t<BaseIter>;                                         ///< 值类型
    using difference_type = iter_difference_t<BaseIter>;                               ///< 差值类型
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>;                         ///< 指针类型
    using reference = iter_reference_t<BaseIter>;                                      ///< 引用类型

    constexpr take_iterator() = default;

    /**
     * @brief 由底层迭代器与剩余计数构造
     * @param current 底层迭代器，指向当前元素
     * @param count 还可取的剩余元素个数，为 0 时表示立即结束
     */
    constexpr take_iterator(BaseIter current, difference_type count) :
    current_(current),
    count_(count) {}

    /**
     * @brief 解引用，访问当前元素
     * @return 当前元素的引用
     */
    constexpr reference operator*() const { return *current_; }

    /**
     * @brief 成员访问
     * @return 指向当前元素的指针
     */
    constexpr pointer operator->() const
        requires is_lvalue_reference_v<reference>
    {
        return _NEFORCE addressof(*current_);
    }

    /**
     * @brief 前置递增，前进到底层下一个元素并递减剩余计数
     * @return 递增后的迭代器引用
     */
    constexpr take_iterator& operator++() {
        ++current_;
        if (count_ > 0) {
            --count_;
        }
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr take_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 判断两个迭代器是否相等
     * @param other 待比较的另一个 take_iterator
     * @return 底层迭代器与剩余计数都相同时返回 true
     */
    constexpr bool operator==(const take_iterator& other) const {
        return current_ == other.current_ && count_ == other.count_;
    }

    /**
     * @brief 判断两个迭代器是否不相等
     * @param other 待比较的另一个 take_iterator
     * @return 不相等时返回 true
     */
    constexpr bool operator!=(const take_iterator& other) const { return !(*this == other); }

    /**
     * @brief 与 sentinel 比较，判断是否已到达末尾
     * @tparam S sentinel 类型
     * @param s 待比较的 sentinel
     * @return 剩余计数为 0 或底层迭代器等于 sentinel 时返回 true
     */
    template <typename S>
        requires(!same_as<S, take_iterator>)
    constexpr bool operator==(const S& s) const {
        return current_ == s || count_ == 0;
    }

    /**
     * @brief 与 sentinel 比较，判断是否未到达末尾
     * @tparam S sentinel 类型
     * @param s 待比较的 sentinel
     * @return 未到达末尾时返回 true
     */
    template <typename S>
        requires(!same_as<S, take_iterator>)
    constexpr bool operator!=(const S& s) const {
        return !(*this == s);
    }

    /**
     * @brief 获取底层迭代器
     * @return 当前底层迭代器的常量引用
     */
    constexpr const BaseIter& current() const { return current_; }

    /**
     * @brief 获取剩余可取元素个数
     * @return 当前剩余计数，可能为负值
     */
    constexpr difference_type count() const { return count_; }

private:
    BaseIter current_{};
    difference_type count_ = 0;
};

/**
 * @brief take_view 的 sentinel
 * @tparam BaseIter 底层迭代器类型，需满足 input_iterator
 * @tparam BaseSentinel 底层 sentinel 类型
 */
template <input_iterator BaseIter, typename BaseSentinel>
class take_sentinel {
public:
    constexpr take_sentinel() = default;

    /**
     * @brief 由底层 sentinel 与计数构造
     * @param base_sen 底层 range 的 sentinel
     * @param count 该 sentinel 参与比较时的剩余计数阈值，通常传 0
     */
    constexpr take_sentinel(BaseSentinel base_sen, iter_difference_t<BaseIter> count) :
    base_sen_(base_sen),
    count_(count) {}

    /**
     * @brief 判断迭代器是否已到达末尾
     * @param it 待比较的 take_iterator
     * @param s 待比较的 sentinel
     * @return 剩余计数为 0 或底层迭代器等于底层 sentinel 时返回 true
     */
    friend constexpr bool operator==(const take_iterator<BaseIter>& it, const take_sentinel& s) {
        return it.count() <= s.count_ || it.current() == s.base_sen_;
    }

    /**
     * @brief 判断迭代器是否未到达末尾
     * @param it 待比较的 take_iterator
     * @param s 待比较的 sentinel
     * @return 未到达末尾时返回 true
     */
    friend constexpr bool operator!=(const take_iterator<BaseIter>& it, const take_sentinel& s) { return !(it == s); }

    /**
     * @brief 判断 sentinel 与迭代器是否相等（反向参数顺序）
     * @param s 待比较的 sentinel
     * @param it 待比较的 take_iterator
     * @return 已到达末尾时返回 true
     */
    friend constexpr bool operator==(const take_sentinel& s, const take_iterator<BaseIter>& it) { return it == s; }

    /**
     * @brief 判断 sentinel 与迭代器是否不相等（反向参数顺序）
     * @param s 待比较的 sentinel
     * @param it 待比较的 take_iterator
     * @return 未到达末尾时返回 true
     */
    friend constexpr bool operator!=(const take_sentinel& s, const take_iterator<BaseIter>& it) { return it != s; }

private:
    BaseSentinel base_sen_{};
    iter_difference_t<BaseIter> count_{0};
};

/**
 * @brief 取前若干个元素的 view
 * @tparam V 底层视图类型，需满足 View
 */
template <View V>
class take_view : public view_base<take_view<V>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin()); ///< 底层可修改迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>>().end());   ///< 底层可修改 sentinel 类型
    using iterator = take_iterator<base_iterator>;                                 ///< 迭代器类型
    using sentinel = take_sentinel<base_iterator, base_sentinel>;                  ///< sentinel 类型

    using const_base_iterator = typename const_range_traits<V>::iterator;           ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;           ///< 底层常量 sentinel 类型
    using const_iterator = take_iterator<const_base_iterator>;                      ///< 常量迭代器类型
    using const_sentinel = take_sentinel<const_base_iterator, const_base_sentinel>; ///< 常量 sentinel 类型

    using difference_type = iter_difference_t<base_iterator>; ///< 差值类型

private:
    V base_;
    difference_type count_ = 0;

public:
    constexpr take_view() = default;

    /**
     * @brief 由底层视图与计数构造
     * @param base 底层视图，按值移动持有
     * @param count 要保留的前缀长度，非正值表示只取 0 个元素
     */
    constexpr take_view(V base, difference_type count) :
    base_(_NEFORCE move(base)),
    count_(count > 0 ? count : 0) {}

    constexpr take_view(take_view&&)
        requires movable<V>
    = default;

    /**
     * @brief 移动赋值运算符
     * @return 移动赋值后的自身引用
     */
    constexpr take_view& operator=(take_view&&)
        requires movable<V>
    = default;

    constexpr take_view(const take_view&)
        requires copy_constructible<V>
    = default;

    /**
     * @brief 拷贝赋值运算符
     * @return 拷贝赋值后的自身引用
     */
    constexpr take_view& operator=(const take_view&)
        requires copyable<V>
    = default;

    /**
     * @brief 获取指向首个元素的迭代器
     * @return 携带底层起始迭代器与计数的 take_iterator
     */
    constexpr iterator begin() { return iterator(base_.begin(), count_); }

    /**
     * @brief 获取末尾 sentinel
     * @return 携带底层 sentinel 与计数 0 的 take_sentinel
     */
    constexpr sentinel end() { return sentinel(base_.end(), 0); }

    /**
     * @brief 获取视图大小
     * @return min(计数, 底层大小)，计数或底层大小非正时按 0 处理
     */
    constexpr auto size() const
        requires requires(const V& v) {
            { v.size() } -> convertible_to<size_t>;
        }
    {
        const auto total = static_cast<difference_type>(base_.size());
        const difference_type clamped_total = total > 0 ? total : 0;
        const difference_type wanted = count_ > 0 ? count_ : 0;
        return static_cast<size_t>(wanted < clamped_total ? wanted : clamped_total);
    }

    /**
     * @brief 获取指向首个元素的常量迭代器
     * @return 携带底层起始迭代器与计数的常量 take_iterator
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), count_);
    }

    /**
     * @brief 获取末尾常量 sentinel
     * @return 携带底层 sentinel 与计数 0 的常量 take_sentinel
     */
    constexpr const_sentinel end() const
        requires Range<const V>
    {
        return const_sentinel(base_.end(), 0);
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
take_view(V&&, iter_difference_t<decltype(_NEFORCE declval<V>().begin())>)
        -> take_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_TAKE_VIEW_HPP__
