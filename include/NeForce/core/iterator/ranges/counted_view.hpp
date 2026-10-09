#ifndef NEFORCE_CORE_ITERATOR_RANGES_COUNTED_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_COUNTED_VIEW_HPP__

/**
 * @file counted_view.hpp
 * @brief 由迭代器与计数构成的视图
 */

#include "NeForce/core/algorithm/iterator.hpp"
#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @class counted_iterator
 * @brief 带剩余计数的迭代器适配器
 * @tparam Iter 底层迭代器类型
 */
template <typename Iter>
class counted_iterator {
public:
    /** @brief 迭代器类别 */
    using iterator_category = forward_iterator_tag;
    /** @brief 值类型 */
    using value_type = iter_value_t<Iter>;
    /** @brief 差值类型 */
    using difference_type = iter_difference_t<Iter>;
    /** @brief 指针类型 */
    using pointer = add_pointer_t<iter_reference_t<Iter>>;
    /** @brief 引用类型 */
    using reference = iter_reference_t<Iter>;

    /**
     * @brief 默认构造函数
     * @note 默认构造后剩余计数为 0，即等价于末尾位置。
     */
    constexpr counted_iterator() = default;
    /**
     * @brief 构造函数
     * @param current 当前底层迭代器
     * @param count 剩余元素个数
     */
    constexpr counted_iterator(Iter current, difference_type count) :
    current_(current),
    count_(count > 0 ? count : 0) {}

    /**
     * @brief 解引用
     * @return 底层迭代器所指元素的引用
     */
    constexpr reference operator*() const { return *current_; }
    /**
     * @brief 成员访问
     * @return 指向当前元素的指针
     */
    constexpr pointer operator->() const { return _NEFORCE addressof(*current_); }

    /**
     * @brief 前置递增
     * @return 递增后的迭代器引用
     */
    constexpr counted_iterator& operator++() {
        ++current_;
        --count_;
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr counted_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较
     * @param other 右侧迭代器
     * @return 两个迭代器的剩余计数是否相等
     */
    constexpr bool operator==(const counted_iterator& other) const {
        return current_ == other.current_ && count_ == other.count_;
    }

    /**
     * @brief 不等比较
     * @param other 右侧迭代器
     * @return 两个迭代器的剩余计数是否不相等
     */
    constexpr bool operator!=(const counted_iterator& other) const { return !(*this == other); }

    /**
     * @brief 获取底层迭代器
     * @return 当前底层迭代器
     */
    constexpr Iter base() const { return current_; }
    /**
     * @brief 获取剩余计数
     * @return 剩余元素个数
     */
    constexpr difference_type count() const { return count_; }

private:
    Iter current_{};
    difference_type count_ = 0;
};

/**
 * @class counted_view
 * @brief 由迭代器与计数构成的视图
 * @tparam Iter 底层迭代器类型
 */
template <input_iterator Iter>
class counted_view : public view_base<counted_view<Iter>> {
public:
    /** @brief 迭代器类型 */
    using iterator = counted_iterator<Iter>;
    /** @brief 差值类型 */
    using difference_type = iter_difference_t<Iter>;

    /**
     * @brief 默认构造函数
     */
    constexpr counted_view() = default;
    /**
     * @brief 构造函数
     * @param iter 起始迭代器
     * @param count 元素个数
     */
    constexpr counted_view(Iter iter, difference_type count) :
    iter_(iter),
    count_(count) {}

    /**
     * @brief 获取起始迭代器
     * @return 携带完整计数的起始迭代器
     */
    constexpr iterator begin() const { return iterator(iter_, count_); }

    /**
     * @brief 获取末尾迭代器
     * @return 计数为 0、且底层迭代器已推进 count 个元素的末尾迭代器
     */
    constexpr iterator end() const { return iterator(_NEFORCE next(iter_, static_cast<difference_type>(size())), 0); }

    /**
     * @brief 获取元素个数
     * @return 元素个数，负的计数按 0 处理
     */
    constexpr auto size() const { return static_cast<size_t>(count_ > 0 ? count_ : 0); }

private:
    Iter iter_{};
    difference_type count_ = 0;
};

/**
 * @brief 推导指引
 * @tparam Iter 迭代器类型
 * @tparam N 计数类型
 */
template <typename Iter, typename N>
counted_view(Iter, N) -> counted_view<Iter>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_COUNTED_VIEW_HPP__
