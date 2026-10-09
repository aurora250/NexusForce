#ifndef NEFORCE_CORE_ITERATOR_RANGES_TRANSFORM_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_TRANSFORM_VIEW_HPP__

/**
 * @file transform_view.hpp
 * @brief 变换视图
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
 * @class transform_iterator
 * @brief 变换迭代器
 * @tparam BaseIter 底层迭代器类型
 * @tparam Func 一元变换函数类型
 *
 * 在底层迭代器之上惰性地施加变换
 */
template <input_iterator BaseIter, typename Func>
class transform_iterator {
public:
    using iterator_category = forward_iterator_tag;                           ///< 迭代器类别
    using base_reference = iter_reference_t<BaseIter>;                        ///< 引用类型
    using value_type = remove_cvref_t<invoke_result_t<Func, base_reference>>; ///< 值类型
    using difference_type = iter_difference_t<BaseIter>;                      ///< 差值类型
    using reference = invoke_result_t<Func, base_reference>;                  ///< 引用类型
    using pointer = void;                                                     ///< 指针类型

    constexpr transform_iterator() = default;

    /**
     * @brief 构造函数
     * @param current 当前底层迭代器位置
     * @param func 指向变换函数的指针
     */
    constexpr transform_iterator(BaseIter current, const Func* func) :
    current_(current),
    func_(func) {}

    /**
     * @brief 解引用操作符
     * @return 变换函数作用于底层当前元素后的结果
     */
    constexpr reference operator*() const { return (*func_)(*current_); }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    constexpr transform_iterator& operator++() {
        ++current_;
        return *this;
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr transform_iterator operator++(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一个 transform_iterator
     * @return 两者的当前底层迭代器相等则返回 true
     */
    constexpr bool operator==(const transform_iterator& other) const { return current_ == other.current_; }

    /**
     * @brief 不等比较操作符
     * @param other 另一个 transform_iterator
     * @return 两者的当前底层迭代器不相等则返回 true
     */
    constexpr bool operator!=(const transform_iterator& other) const { return !(*this == other); }

    /**
     * @brief 与哨兵比较的相等操作符
     * @tparam S 哨兵类型
     * @param s 哨兵对象
     * @return 当前底层迭代器与哨兵相等则返回 true
     */
    template <typename S>
        requires(!same_as<S, transform_iterator>) && sentinel_for<S, BaseIter>
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
        requires(!same_as<S, transform_iterator>) && sentinel_for<S, BaseIter>
    constexpr bool operator!=(const S& s) const {
        return !(current_ == s);
    }

    /**
     * @brief 友元相等比较操作符
     * @tparam Sentinel 哨兵类型
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 迭代器当前底层位置与哨兵相等则返回 true
     */
    template <typename Sentinel>
    friend constexpr bool operator==(const Sentinel& s, const transform_iterator& it) {
        return it.current_ == s;
    }

    /**
     * @brief 友元不等比较操作符
     * @tparam Sentinel 哨兵类型
     * @param s 哨兵对象
     * @param it 迭代器
     * @return 哨兵与迭代器当前底层位置不相等则返回 true
     */
    template <typename Sentinel>
    friend constexpr bool operator!=(const Sentinel& s, const transform_iterator& it) {
        return !(s == it.current_);
    }

private:
    BaseIter current_{};
    const Func* func_ = nullptr;
};


/**
 * @class transform_view
 * @brief 变换视图
 * @tparam V 底层视图类型
 * @tparam Func 一元变换函数类型
 *
 * 惰性、非拥有地对底层视图逐元素施加变换
 */
template <View V, typename Func>
class transform_view : public view_base<transform_view<V, Func>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin()); ///< 底层可写迭代器类型
    using const_base_iterator = typename const_range_traits<V>::iterator;          ///< 底层 const 迭代器类型
    using iterator = transform_iterator<base_iterator, Func>;                      ///< 迭代器类型
    using const_iterator = transform_iterator<const_base_iterator, Func>;          ///< const 迭代器类型
    using sentinel = decltype(_NEFORCE declval<remove_const_t<V>>().end());        ///< 哨兵类型
    using const_sentinel = typename const_range_traits<V>::sentinel;               ///< const 哨兵类型

private:
    V base_;
    Func func_;

public:
    constexpr transform_view() = default;

    /**
     * @brief 构造函数
     * @param base 底层视图
     * @param func 变换函数
     *
     * 视图接管底层视图与变换函数的副本。
     */
    constexpr transform_view(V base, Func func) :
    base_(_NEFORCE move(base)),
    func_(_NEFORCE move(func)) {}

    constexpr transform_view(transform_view&&)
        requires movable<V> && movable<Func>
    = default;

    /**
     * @brief 移动赋值操作符
     * @param other 源视图
     * @return 自身引用
     */
    constexpr transform_view& operator=(transform_view&& other) noexcept(is_nothrow_move_assignable_v<V> &&
                                                                         is_nothrow_move_assignable_v<Func>)
        requires movable<V> && movable<Func>
    {
        base_ = _NEFORCE move(other.base_);
        func_ = _NEFORCE move(other.func_);
        return *this;
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源视图
     */
    constexpr transform_view(const transform_view& other)
        requires copy_constructible<V> && copy_constructible<Func>
    :
    base_(other.base_),
    func_(other.func_) {}

    /**
     * @brief 拷贝赋值操作符
     * @param other 源视图
     * @return 自身引用
     */
    constexpr transform_view& operator=(const transform_view& other)
        requires copyable<V> && copyable<Func>
    {
        if (addressof(other) == this) {
            return *this;
        }
        base_ = other.base_;
        func_ = other.func_;
        return *this;
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向底层起始位置的变换迭代器
     */
    constexpr iterator begin() { return iterator(base_.begin(), &func_); }

    /**
     * @brief 获取结束哨兵
     * @return 底层视图的哨兵
     */
    constexpr sentinel end() { return base_.end(); }

    /**
     * @brief 获取 const 起始迭代器
     * @return 指向底层起始位置的 const 变换迭代器
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), &func_);
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
     * @brief 获取 const 结束哨兵
     * @return 底层视图的 const 哨兵
     */
    constexpr const_sentinel end() const
        requires Range<const V>
    {
        return base_.end();
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
#endif // NEFORCE_CORE_ITERATOR_RANGES_TRANSFORM_VIEW_HPP__
