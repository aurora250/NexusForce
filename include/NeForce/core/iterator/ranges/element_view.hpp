#ifndef NEFORCE_CORE_ITERATOR_RANGES_ELEMENT_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_ELEMENT_VIEW_HPP__

/**
 * @file element_view.hpp
 * @brief 提取元组元素或键值对的视图
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
 * @class element_iterator
 * @brief 提取元素迭代器
 * @tparam N 要提取的元素下标
 * @tparam BaseIter 底层迭代器类型
 */
template <size_t N, typename BaseIter>
class element_iterator {
public:
    /** @brief 底层迭代器的引用类型 */
    using base_reference = iter_reference_t<BaseIter>;
    /** @brief 第 N 个元素的类型 */
    using element_type = decltype(get<N>(_NEFORCE declval<base_reference>()));
    /** @brief 迭代器类别 */
    using iterator_category = forward_iterator_tag;
    /** @brief 值类型 */
    using value_type = remove_cvref_t<element_type>;
    /** @brief 差值类型 */
    using difference_type = iter_difference_t<BaseIter>;
    /** @brief 指针类型 */
    using pointer = add_pointer_t<element_type>;
    /** @brief 引用类型 */
    using reference = element_type;

    /**
     * @brief 默认构造函数
     */
    constexpr element_iterator() = default;
    /**
     * @brief 构造函数
     * @param current 底层迭代器
     */
    constexpr explicit element_iterator(BaseIter current) :
    current_(current) {}

    /**
     * @brief 解引用
     * @return 当前元素的第 N 个元素
     */
    constexpr reference operator*() const { return _NEFORCE get<N>(*current_); }

    /**
     * @brief 成员访问
     * @return 指向当前元素第 N 个元素的指针
     * @warning 若 get<N> 返回值而非引用，返回的指针将指向临时对象，立即悬垂。
     */
    constexpr pointer operator->() const
        requires is_lvalue_reference_v<reference>
    {
        return _NEFORCE addressof(_NEFORCE get<N>(*current_));
    }

    /**
     * @brief 前置递增
     * @return 递增后的迭代器引用
     */
    constexpr element_iterator& operator++() {
        ++current_;
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr element_iterator operator++(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    /**
     * @brief 相等比较
     * @param other 右侧迭代器
     * @return 底层迭代器是否相等
     */
    constexpr bool operator==(const element_iterator& other) const { return current_ == other.current_; }
    /**
     * @brief 不等比较
     * @param other 右侧迭代器
     * @return 底层迭代器是否不相等
     */
    constexpr bool operator!=(const element_iterator& other) const { return current_ != other.current_; }

    /**
     * @brief 与 sentinel 相等比较
     * @tparam S sentinel 类型
     * @param s 右侧 sentinel
     * @return 底层迭代器是否与 sentinel 相等
     */
    template <typename S>
        requires(!same_as<S, element_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator==(const S& s) const {
        return current_ == s;
    }
    /**
     * @brief 与 sentinel 不等比较
     * @tparam S sentinel 类型
     * @param s 右侧 sentinel
     * @return 底层迭代器是否与 sentinel 不相等
     */
    template <typename S>
        requires(!same_as<S, element_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator!=(const S& s) const {
        return current_ != s;
    }

private:
    BaseIter current_{};
};

/**
 * @class element_view
 * @brief 提取元素视图
 * @tparam N 要提取的元素下标
 * @tparam V 底层视图类型
 */
template <size_t N, View V>
class element_view : public view_base<element_view<N, V>> {
public:
    /** @brief 底层视图的迭代器类型 */
    using base_iterator = decltype(_NEFORCE declval<V>().begin());
    /** @brief 底层视图的 sentinel 类型 */
    using base_sentinel = decltype(_NEFORCE declval<V>().end());
    /** @brief 迭代器类型 */
    using iterator = element_iterator<N, base_iterator>;
    /** @brief sentinel 类型 */
    using sentinel = base_sentinel;

    /**
     * @brief 默认构造函数
     */
    constexpr element_view() = default;
    /**
     * @brief 构造函数
     * @param base 被包装的底层视图
     */
    constexpr explicit element_view(V base) :
    base_(_NEFORCE move(base)) {}

    /**
     * @brief 获取起始迭代器
     * @return 提取元素的起始迭代器
     */
    constexpr iterator begin() { return iterator(base_.begin()); }
    /**
     * @brief 获取末尾 sentinel
     * @return 底层视图的末尾 sentinel
     */
    constexpr sentinel end() { return base_.end(); }

    /**
     * @brief 获取起始迭代器
     * @return 提取元素的起始迭代器
     */
    constexpr iterator begin() const
        requires Range<const V>
    {
        return iterator(base_.begin());
    }

    /**
     * @brief 获取元素个数
     * @return 底层视图的元素个数
     */
    constexpr auto size() const
        requires requires(const V& v) {
            { v.size() } -> convertible_to<size_t>;
        }
    {
        return static_cast<size_t>(base_.size());
    }
    /**
     * @brief 获取末尾 sentinel
     * @return 底层视图的末尾 sentinel
     */
    constexpr sentinel end() const
        requires Range<const V>
    {
        return base_.end();
    }

    /**
     * @brief 获取底层视图
     * @return 底层视图的拷贝
     */
    constexpr V base() const&
        requires copy_constructible<V>
    {
        return base_;
    }
    /**
     * @brief 获取底层视图
     * @return 被移动的底层视图
     */
    constexpr V base() && { return _NEFORCE move(base_); }

private:
    V base_;
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_ELEMENT_VIEW_HPP__
