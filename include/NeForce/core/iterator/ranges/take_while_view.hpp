#ifndef NEFORCE_CORE_ITERATOR_RANGES_TAKE_WHILE_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_TAKE_WHILE_VIEW_HPP__

/**
 * @file take_while_view.hpp
 * @brief 取满足谓词的最长前缀的视图
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
 * @brief 取满足谓词的最长前缀的 iterator
 * @tparam BaseIter 底层迭代器类型，需满足 input_iterator
 * @tparam Sentinel 底层 sentinel 类型，需满足 sentinel_for<BaseIter>
 * @tparam Pred 一元谓词类型，接受元素引用并返回可转换为 bool 的结果
 */
template <input_iterator BaseIter, sentinel_for<BaseIter> Sentinel, typename Pred>
class take_while_iterator {
public:
    using iterator_category = conditional_t<is_base_of_v<forward_iterator_tag, iter_category_t<BaseIter>>,
                                            forward_iterator_tag, input_iterator_tag>; ///< 迭代器类别
    using value_type = iter_value_t<BaseIter>;                                         ///< 值类型
    using difference_type = iter_difference_t<BaseIter>;                               ///< 差值类型
    using pointer = add_pointer_t<iter_reference_t<BaseIter>>;                         ///< 指针类型
    using reference = iter_reference_t<BaseIter>;                                      ///< 引用类型

    constexpr take_while_iterator() = default;

    /**
     * @brief 由底层迭代器、sentinel 与谓词构造
     * @param current 底层迭代器，指向当前元素
     * @param end 底层 sentinel
     * @param pred 指向谓词的指针，通常指向所属视图内部的谓词
     */
    constexpr take_while_iterator(BaseIter current, Sentinel end, const Pred* pred) :
    current_(current),
    end_(end),
    pred_(pred),
    done_(current == end || !(*pred)(*current)) {}

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
     * @brief 前置递增，前进到底层下一个元素并在需要时重新检测谓词
     * @return 递增后的迭代器引用
     */
    constexpr take_while_iterator& operator++() {
        ++current_;
        if (current_ == end_ || !(*pred_)(*current_)) {
            done_ = true;
        }
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr take_while_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 判断两个迭代器是否相等
     * @param other 待比较的另一个 take_while_iterator
     */
    constexpr bool operator==(const take_while_iterator& other) const {
        return current_ == other.current_ && done_ == other.done_;
    }

    /**
     * @brief 判断两个迭代器是否不相等
     * @param other 待比较的另一个 take_while_iterator
     * @return 不相等时返回 true
     */
    constexpr bool operator!=(const take_while_iterator& other) const { return !(*this == other); }

    /**
     * @brief 与 sentinel 比较，判断是否已到达末尾
     * @tparam S 底层 sentinel 类型
     * @param s 待比较的 sentinel
     * @return 已置位 done_ 或底层迭代器等于 sentinel 时返回 true
     */
    template <typename S>
        requires(!same_as<S, take_while_iterator>)
    constexpr bool operator==(const S& s) const {
        return done_ || current_ == s;
    }

    /**
     * @brief 与 sentinel 比较，判断是否未到达末尾
     * @tparam S 底层 sentinel 类型
     * @param s 待比较的 sentinel
     * @return 未到达末尾时返回 true
     */
    template <typename S>
        requires(!same_as<S, take_while_iterator>)
    constexpr bool operator!=(const S& s) const {
        return !(*this == s);
    }

    /**
     * @brief 判断 sentinel 与迭代器是否相等
     * @tparam S 底层 sentinel 类型
     * @param s 待比较的 sentinel
     * @param t 待比较的迭代器
     * @return 已到达末尾时返回 true
     */
    template <typename S>
    friend constexpr bool operator==(const S& s, const take_while_iterator& t) {
        return t.done_ || t.current_ == s;
    }

    /**
     * @brief 判断 sentinel 与迭代器是否不相等
     * @tparam S 底层 sentinel 类型
     * @param s 待比较的 sentinel
     * @param t 待比较的迭代器
     * @return 未到达末尾时返回 true
     */
    template <typename S>
    friend constexpr bool operator!=(const S& s, const take_while_iterator& t) {
        return !(s == t);
    }

private:
    BaseIter current_{};
    Sentinel end_{};
    const Pred* pred_ = nullptr;
    bool done_ = false;
};

/**
 * @brief 取满足谓词的最长前缀的 view
 * @tparam V 底层视图类型，需满足 View
 * @tparam Pred 一元谓词类型，接受元素引用并返回可转换为 bool 的结果
 */
template <View V, typename Pred>
class take_while_view : public view_base<take_while_view<V, Pred>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>>().begin()); ///< 底层可修改迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<remove_const_t<V>>().end());   ///< 底层可修改 sentinel 类型
    using const_base_iterator = typename const_range_traits<V>::iterator;          ///< 底层常量迭代器类型
    using const_base_sentinel = typename const_range_traits<V>::sentinel;          ///< 底层常量 sentinel 类型

    using iterator = take_while_iterator<base_iterator, base_sentinel, Pred>;                   ///< 迭代器类型
    using const_iterator = take_while_iterator<const_base_iterator, const_base_sentinel, Pred>; ///< 常量迭代器类型

    using sentinel = base_sentinel;             ///< sentinel 类型
    using const_sentinel = const_base_sentinel; ///< 常量 sentinel 类型

private:
    V base_;
    Pred pred_;

public:
    constexpr take_while_view() = default;

    /**
     * @brief 由底层视图与谓词构造
     * @param base 底层视图，按值移动持有
     * @param pred 谓词，按值移动持有
     */
    constexpr take_while_view(V base, Pred pred) :
    base_(_NEFORCE move(base)),
    pred_(_NEFORCE move(pred)) {}

    /**
     * @brief 获取指向最长前缀首个元素的迭代器
     * @return 携带底层起始迭代器、底层 sentinel 与内部谓词指针的 take_while_iterator
     */
    constexpr iterator begin() { return iterator(base_.begin(), base_.end(), &pred_); }

    /**
     * @brief 获取末尾 sentinel
     * @return 底层视图的 sentinel
     */
    constexpr sentinel end() { return base_.end(); }

    /**
     * @brief 获取指向最长前缀首个元素的常量迭代器
     * @return 携带底层起始迭代器、底层 sentinel 与内部谓词指针的常量 take_while_iterator
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        return const_iterator(base_.begin(), base_.end(), &pred_);
    }

    /**
     * @brief 获取末尾常量 sentinel
     * @return 底层视图的常量 sentinel
     */
    constexpr const_sentinel end() const
        requires Range<const V>
    {
        return base_.end();
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

template <typename V, typename Pred>
take_while_view(V&&, Pred) -> take_while_view<V, Pred>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_TAKE_WHILE_VIEW_HPP__
