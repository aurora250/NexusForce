#ifndef NEFORCE_CORE_ITERATOR_RANGES_REVERSE_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_REVERSE_VIEW_HPP__

/**
 * @file reverse_view.hpp
 * @brief 反向遍历视图
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
 * @brief 反向迭代器
 * @tparam Iter 底层双向迭代器类型
 */
template <bidirectional_iterator Iter>
class reverse_iterator {
public:
    using iterator_category = bidirectional_iterator_tag;  ///< 迭代器类别
    using value_type = iter_value_t<Iter>;                 ///< 值类型
    using difference_type = iter_difference_t<Iter>;       ///< 差值类型
    using pointer = add_pointer_t<iter_reference_t<Iter>>; ///< 指针类型
    using reference = iter_reference_t<Iter>;              ///< 引用类型

    /**
     * @brief 默认构造函数
     */
    constexpr reverse_iterator() = default;
    /**
     * @brief 构造函数
     * @param current 底层迭代器位置
     */
    constexpr explicit reverse_iterator(Iter current) :
    current_(current) {}

    /**
     * @brief 解引用操作符
     * @return 当前反向位置对应的元素引用
     */
    constexpr reference operator*() const {
        auto tmp = current_;
        return *--tmp;
    }

    /**
     * @brief 成员访问操作符
     * @return 指向当前元素的指针
     */
    constexpr pointer operator->() const
        requires is_lvalue_reference_v<reference>
    {
        auto tmp = current_;
        return _NEFORCE addressof(*--tmp);
    }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    constexpr reverse_iterator& operator++() {
        --current_;
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr reverse_iterator operator++(int) {
        auto tmp = *this;
        --current_;
        return tmp;
    }

    /**
     * @brief 前置递减操作符
     * @return 递减后的迭代器引用
     */
    constexpr reverse_iterator& operator--() {
        ++current_;
        return *this;
    }

    /**
     * @brief 后置递减操作符
     * @return 递减前的迭代器副本
     */
    constexpr reverse_iterator operator--(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一反向迭代器
     * @return 两者包装的底层迭代器是否相等
     */
    constexpr bool operator==(const reverse_iterator& other) const { return current_ == other.current_; }

    /**
     * @brief 不等比较操作符
     * @param other 另一反向迭代器
     * @return 两者包装的底层迭代器是否不相等
     */
    constexpr bool operator!=(const reverse_iterator& other) const { return !(*this == other); }

private:
    Iter current_{};
};

/**
 * @brief 反向遍历视图
 * @tparam V 底层视图类型
 */
template <View V>
    requires bidirectional_iterator<decltype(_NEFORCE declval<V>().begin())> && common_range<V>
class reverse_view : public view_base<reverse_view<V>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin()); ///< 底层迭代器类型
    using iterator = reverse_iterator<base_iterator>;                              ///< 迭代器类型
    using const_base_iterator = typename const_range_traits<V>::iterator;          ///< 底层常量迭代器类型
    using const_iterator = reverse_iterator<const_base_iterator>;                  ///< 常量迭代器类型

private:
    V base_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr reverse_view() = default;
    /**
     * @brief 构造函数
     * @param base 底层视图，按值接收，可移动传入
     */
    constexpr explicit reverse_view(V base) :
    base_(_NEFORCE move(base)) {}

    /**
     * @brief 移动构造函数
     * @note 仅在底层视图可移动时提供
     */
    constexpr reverse_view(reverse_view&&)
        requires movable<V>
    = default;

    /**
     * @brief 移动赋值操作符
     * @return 自身引用
     */
    constexpr reverse_view& operator=(reverse_view&&)
        requires movable<V>
    = default;

    /**
     * @brief 拷贝构造函数
     */
    constexpr reverse_view(const reverse_view&)
        requires copy_constructible<V>
    = default;

    /**
     * @brief 拷贝赋值操作符
     * @return 自身引用
     */
    constexpr reverse_view& operator=(const reverse_view&)
        requires copyable<V>
    = default;

    /**
     * @brief 获取起始迭代器
     * @return 指向底层范围最后一个元素的反向迭代器
     */
    constexpr iterator begin() { return iterator(base_.end()); }
    /**
     * @brief 获取末尾迭代器
     * @return 包装底层 begin() 的反向迭代器，不可解引用
     */
    constexpr iterator end() { return iterator(base_.begin()); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向底层范围最后一个元素的常量反向迭代器
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.end());
    }

    /**
     * @brief 获取元素个数
     * @return 底层范围的元素个数
     */
    constexpr auto size() const
        requires requires(const V& v) {
            { v.size() } -> convertible_to<size_t>;
        }
    {
        return static_cast<size_t>(base_.size());
    }

    /**
     * @brief 获取常量末尾迭代器
     * @return 包装底层 begin() 的常量反向迭代器
     */
    constexpr const_iterator end() const
        requires Range<const V>
    {
        return const_iterator(base_.begin());
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
};

template <typename V>
reverse_view(V&&) -> reverse_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_REVERSE_VIEW_HPP__
