#ifndef NEFORCE_CORE_ITERATOR_RANGES_SCAN_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_SCAN_VIEW_HPP__

/**
 * @file scan_view.hpp
 * @brief 前缀累计视图
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
 * @class scan_view
 * @brief 前缀累计视图
 * @tparam V 底层视图类型
 * @tparam T 累加值类型
 * @tparam Op 累加函数类型
 */
template <View V, typename T, typename Op>
class scan_view : public view_base<scan_view<V, T, Op>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin()); ///< 底层迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>&>().end());   ///< 底层哨兵类型
    using const_base_iterator = typename const_range_traits<V>::iterator;           ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;           ///< 底层常量哨兵类型

    /**
     * @class basic_iterator
     * @brief 前缀累计视图的迭代器
     * @tparam Iter 底层迭代器类型
     * @tparam Sentinel 底层哨兵类型
     */
    template <typename Iter, typename Sentinel>
    class basic_iterator {
    public:
        using iterator_category = conditional_t<is_base_of_v<forward_iterator_tag, iter_category_t<Iter>>,
                                                forward_iterator_tag, input_iterator_tag>; ///< 迭代器类别
        using value_type = T;                                                              ///< 元素类型
        using reference = const T&;                                                        ///< 元素引用类型
        using difference_type = iter_difference_t<Iter>;                                   ///< 迭代器距离类型
        using pointer = const T*;                                                          ///< 元素指针类型

        basic_iterator() = default;

        /**
         * @brief 构造前缀累计迭代器
         * @param current 当前底层迭代器
         * @param end 底层哨兵
         * @param op 累加函数的指针，指向视图内部持有的累加函数
         * @param init 初值
         */
        basic_iterator(Iter current, Sentinel end, const Op* op, T init) :
        current_(current),
        end_(end),
        op_(op),
        value_(_NEFORCE move(init)),
        done_(false) {}

        /**
         * @brief 解引用迭代器，得到当前累计值
         * @return 当前累计值的引用
         */
        NEFORCE_NODISCARD constexpr reference operator*() const { return value_; }

        /**
         * @brief 获取当前累计值的指针
         * @return 指向当前累计值的指针
         */
        NEFORCE_NODISCARD constexpr pointer operator->() const { return _NEFORCE addressof(value_); }

        /**
         * @brief 前置递增，把下一个底层元素累计进当前值
         * @return 递增后的迭代器引用
         */
        basic_iterator& operator++() {
            if (current_ == end_) {
                done_ = true;
                return *this;
            }
            value_ = (*op_)(_NEFORCE move(value_), *current_);
            ++current_;
            return *this;
        }

        /**
         * @brief 后置递增
         * @return 递增前的迭代器副本
         */
        basic_iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        /**
         * @brief 判断是否已产出全部累计值
         * @return 是否已结束
         */
        NEFORCE_NODISCARD constexpr bool done() const noexcept { return done_; }

        /**
         * @brief 获取当前底层迭代器
         * @return 当前底层迭代器
         */
        NEFORCE_NODISCARD constexpr Iter current() const { return current_; }

    private:
        Iter current_{};
        Sentinel end_{};
        const Op* op_ = nullptr;
        T value_{};
        bool done_ = true;
    };

    /**
     * @class sentinel
     * @brief 前缀累计视图的哨兵
     */
    class sentinel {
    public:
        constexpr sentinel() = default;

        /**
         * @brief 迭代器与哨兵相等比较
         * @tparam Iter 底层迭代器类型
         * @tparam BaseSentinel 底层哨兵类型
         * @param iter 迭代器
         * @return 迭代器是否已结束
         */
        template <typename Iter, typename BaseSentinel>
        NEFORCE_NODISCARD friend constexpr bool operator==(const basic_iterator<Iter, BaseSentinel>& iter,
                                                           const sentinel& /*unused*/) noexcept {
            return iter.done();
        }

        /**
         * @brief 迭代器与哨兵不等比较
         * @tparam Iter 底层迭代器类型
         * @tparam BaseSentinel 底层哨兵类型
         * @param iter 迭代器
         * @param s 哨兵
         * @return 迭代器是否未结束
         */
        template <typename Iter, typename BaseSentinel>
        NEFORCE_NODISCARD friend constexpr bool operator!=(const basic_iterator<Iter, BaseSentinel>& iter,
                                                           const sentinel& s) noexcept {
            return !(iter == s);
        }

        /**
         * @brief 哨兵与迭代器相等比较
         * @tparam Iter 底层迭代器类型
         * @tparam BaseSentinel 底层哨兵类型
         * @param s 哨兵
         * @param iter 迭代器
         * @return 迭代器是否已结束
         */
        template <typename Iter, typename BaseSentinel>
        NEFORCE_NODISCARD friend constexpr bool operator==(const sentinel& s,
                                                           const basic_iterator<Iter, BaseSentinel>& iter) noexcept {
            return iter == s;
        }

        /**
         * @brief 哨兵与迭代器不等比较
         * @tparam Iter 底层迭代器类型
         * @tparam BaseSentinel 底层哨兵类型
         * @param s 哨兵
         * @param iter 迭代器
         * @return 迭代器是否未结束
         */
        template <typename Iter, typename BaseSentinel>
        NEFORCE_NODISCARD friend constexpr bool operator!=(const sentinel& s,
                                                           const basic_iterator<Iter, BaseSentinel>& iter) noexcept {
            return !(iter == s);
        }
    };

    using iterator = basic_iterator<base_iterator, base_sentinel>;                   ///< 迭代器类型
    using const_iterator = basic_iterator<const_base_iterator, const_base_sentinel>; ///< 常量迭代器类型
    using const_sentinel = sentinel;                                                 ///< 常量哨兵类型

private:
    V base_;
    T init_;
    Op op_;

public:
    scan_view() = default;

    /**
     * @brief 构造前缀累计视图
     * @param base 底层视图
     * @param init 初值
     * @param op 累加函数
     */
    scan_view(V base, T init, Op op) :
    base_(_NEFORCE move(base)),
    init_(_NEFORCE move(init)),
    op_(_NEFORCE move(op)) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() { return iterator(base_.begin(), base_.end(), &op_, init_); }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() { return sentinel{}; }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), base_.end(), &op_, init_);
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const
        requires Range<const V>
    {
        return const_sentinel{};
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
#endif // NEFORCE_CORE_ITERATOR_RANGES_SCAN_VIEW_HPP__
