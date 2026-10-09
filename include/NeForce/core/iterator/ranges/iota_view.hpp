#ifndef NEFORCE_CORE_ITERATOR_RANGES_IOTA_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_IOTA_VIEW_HPP__

/**
 * @file iota_view.hpp
 * @brief 递增序列视图
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
 * @brief 递增序列迭代器
 * @tparam T 序列的值类型
 */
template <typename T>
class iota_iterator {
public:
    using iterator_category = forward_iterator_tag; ///< 迭代器类别
    using value_type = T;                           ///< 值类型
    using difference_type = conditional_t<is_integral_v<T>, conditional_t<sizeof(T) < sizeof(int), int, T>,
                                          ptrdiff_t>; ///< 差值类型
    using pointer = const T*;                         ///< 指针类型
    using reference = T;                              ///< 引用类型

    /**
     * @brief 默认构造函数
     * @note 默认构造的迭代器值为值初始化的 T
     */
    constexpr iota_iterator() = default;
    /**
     * @brief 构造函数
     * @param value 迭代器当前值
     */
    constexpr explicit iota_iterator(T value) :
    value_(value) {}

    /**
     * @brief 解引用操作符
     * @return 当前值的拷贝
     */
    constexpr reference operator*() const { return value_; }
    /**
     * @brief 成员访问操作符
     * @return 指向迭代器内部当前值的指针
     */
    constexpr pointer operator->() const { return &value_; }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    constexpr iota_iterator& operator++() {
        ++value_;
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr iota_iterator operator++(int) {
        auto tmp = *this;
        ++value_;
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一递增序列迭代器
     * @return 两者当前值是否相等
     */
    constexpr bool operator==(const iota_iterator& other) const { return value_ == other.value_; }
    /**
     * @brief 不等比较操作符
     * @param other 另一递增序列迭代器
     * @return 两者当前值是否不相等
     */
    constexpr bool operator!=(const iota_iterator& other) const { return value_ != other.value_; }

    /**
     * @brief 小于比较操作符
     * @param other 另一递增序列迭代器
     * @return 当前值是否小于另一迭代器的值
     */
    constexpr bool operator<(const iota_iterator& other) const
        requires totally_ordered<T>
    {
        return value_ < other.value_;
    }

private:
    T value_{};
};

/**
 * @brief 递增序列的哨兵类型
 * @tparam T 序列的值类型
 */
template <typename T>
class iota_sentinel {
private:
    bool has_bound_ = false;
    iota_iterator<T> bound_iter_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr iota_sentinel() noexcept = default;
    /**
     * @brief 构造函数
     * @param bound 序列的边界值
     */
    constexpr explicit iota_sentinel(T bound) noexcept :
    has_bound_(true),
    bound_iter_(bound) {}

    /**
     * @brief 迭代器与哨兵的相等比较
     * @param it 递增序列迭代器
     * @param s 递增序列哨兵
     * @return 哨兵有界且迭代器值等于边界值时返回 true；无界哨兵始终返回 false
     */
    friend constexpr bool operator==(const iota_iterator<T>& it, const iota_sentinel& s) noexcept {
        return s.has_bound_ && it == s.bound_iter_;
    }
    /**
     * @brief 迭代器与哨兵的不等比较
     * @param it 递增序列迭代器
     * @param s 递增序列哨兵
     * @return 是否尚未到达边界
     * @note 无界哨兵始终返回 true
     */
    friend constexpr bool operator!=(const iota_iterator<T>& it, const iota_sentinel& s) noexcept { return !(it == s); }
    /**
     * @brief 哨兵与迭代器的相等比较
     * @param s 递增序列哨兵
     * @param it 递增序列迭代器
     * @return 与 it == s 结果相同
     */
    friend constexpr bool operator==(const iota_sentinel& s, const iota_iterator<T>& it) noexcept { return it == s; }
    /**
     * @brief 哨兵与迭代器的不等比较
     * @param s 递增序列哨兵
     * @param it 递增序列迭代器
     * @return 与 it != s 结果相同
     */
    friend constexpr bool operator!=(const iota_sentinel& s, const iota_iterator<T>& it) noexcept { return !(it == s); }
};

/**
 * @brief 递增序列视图
 * @tparam T 序列的值类型
 */
template <typename T>
class iota_view : public view_base<iota_view<T>> {
public:
    using iterator = iota_iterator<T>; ///< 迭代器类型
    using sentinel = iota_sentinel<T>; ///< 哨兵类型

    /**
     * @brief 默认构造函数
     */
    constexpr iota_view() = default;
    /**
     * @brief 构造无界递增序列视图
     * @param start 起始值
     */
    constexpr explicit iota_view(T start) :
    start_(start) {}
    /**
     * @brief 构造有界递增序列视图
     * @param start 起始值
     * @param bound 终止值
     */
    constexpr iota_view(T start, T bound) :
    start_(start),
    bound_(bound),
    has_bound_(true) {}

    /**
     * @brief 获取起始迭代器
     * @return 指向起始值的迭代器
     */
    constexpr iterator begin() const { return iterator(start_); }
    /**
     * @brief 获取哨兵
     * @return 有界视图返回携带边界值的哨兵；无界视图返回与任何迭代器都不相等的哨兵
     */
    constexpr sentinel end() const { return has_bound_ ? sentinel(bound_) : sentinel(); }

private:
    T start_{};
    T bound_{};
    bool has_bound_ = false;
};

template <typename T>
iota_view(T) -> iota_view<T>;

template <typename T>
iota_view(T, T) -> iota_view<T>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_IOTA_VIEW_HPP__
