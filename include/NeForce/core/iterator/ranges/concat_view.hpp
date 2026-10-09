#ifndef NEFORCE_CORE_ITERATOR_RANGES_CONCAT_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_CONCAT_VIEW_HPP__

/**
 * @file concat_view.hpp
 * @brief 顺序拼接两个范围的视图
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
 * @brief 顺序拼接两个范围后的迭代器
 * @tparam IsConst 是否使用常量迭代器，true 时取两侧视图的 const 迭代器
 * @tparam V1 第一段视图类型，需满足 View
 * @tparam V2 第二段视图类型，需满足 View
 */
template <bool IsConst, View V1, View V2>
struct concat_iterator {
    using iter1_t = decltype(_NEFORCE declval<remove_const_t<V1>>().begin()); ///< 第一段视图的可修改迭代器类型
    using iter2_t = decltype(_NEFORCE declval<remove_const_t<V2>>().begin()); ///< 第二段视图的可修改迭代器类型
    using const_iter1_t = typename const_range_traits<V1>::iterator;          ///< 第一段视图的常量迭代器类型
    using const_iter2_t = typename const_range_traits<V2>::iterator;          ///< 第二段视图的常量迭代器类型
    using iterator1 = conditional_t<IsConst, const_iter1_t, iter1_t>;         ///< 按 IsConst 选定的第一段迭代器类型
    using iterator2 = conditional_t<IsConst, const_iter2_t, iter2_t>;         ///< 按 IsConst 选定的第二段迭代器类型

    using iterator_category = forward_iterator_tag;                                                    ///< 迭代器类别
    using value_type = common_type_t<iter_value_t<iterator1>, iter_value_t<iterator2>>;                ///< 值类型
    using difference_type = common_type_t<iter_difference_t<iterator1>, iter_difference_t<iterator2>>; ///< 差值类型
    using pointer = void;                                                                              ///< 指针类型
    using reference = common_reference_t<iter_reference_t<iterator1>, iter_reference_t<iterator2>>;    ///< 公共引用类型

    iterator1 current1_;   ///< 第一段当前迭代器
    iterator1 end1_;       ///< 第一段末尾迭代器
    iterator2 current2_;   ///< 第二段当前迭代器
    iterator2 end2_;       ///< 第二段末尾迭代器
    bool in_first_ = true; ///< 是否仍在第一段

    /**
     * @brief 默认构造函数
     */
    concat_iterator() = default;

    /**
     * @brief 由两段的起止迭代器构造
     * @param first1 第一段起始迭代器
     * @param last1 第一段末尾迭代器
     * @param first2 第二段起始迭代器
     * @param last2 第二段末尾迭代器
     */
    concat_iterator(iterator1 first1, iterator1 last1, iterator2 first2, iterator2 last2) :
    current1_(first1),
    end1_(last1),
    current2_(first2),
    end2_(last2) {
        if (current1_ == end1_) {
            in_first_ = false;
        }
    }

    /**
     * @brief 解引用，访问当前元素
     * @return 当前段当前元素的公共引用类型引用
     */
    reference operator*() const { return in_first_ ? *current1_ : *current2_; }

    /**
     * @brief 前置递增，前进到下一个元素
     * @return 递增后的自身引用
     */
    concat_iterator& operator++() {
        if (in_first_) {
            ++current1_;
            if (current1_ == end1_) {
                in_first_ = false;
            }
        } else {
            ++current2_;
        }
        return *this;
    }

    /**
     * @brief 后置递增，前进到下一个元素
     * @return 递增前的迭代器副本
     */
    concat_iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    /**
     * @brief 判断两个迭代器是否相等
     * @param other 待比较的迭代器
     */
    bool operator==(const concat_iterator& other) const {
        if (in_first_ != other.in_first_) {
            return false;
        }
        if (in_first_) {
            return current1_ == other.current1_;
        }
        return current2_ == other.current2_;
    }

    /**
     * @brief 判断两个迭代器是否不相等
     * @param other 待比较的迭代器
     * @return 不相等时返回 true
     */
    bool operator!=(const concat_iterator& other) const { return !(*this == other); }
};

/**
 * @brief 顺序拼接两个范围的 view
 * @tparam V1 第一段视图类型，需满足 View
 * @tparam V2 第二段视图类型，需满足 View
 */
template <View V1, View V2>
class concat_view : public view_base<concat_view<V1, V2>> {
    remove_const_t<V1> v1_;
    remove_const_t<V2> v2_;

    using iterator = concat_iterator<false, V1, V2>;
    using const_iterator = concat_iterator<true, V1, V2>;

public:
    /**
     * @brief 默认构造函数
     */
    concat_view() = default;

    /**
     * @brief 由两侧视图构造
     * @param v1 第一段视图，按值移动持有
     * @param v2 第二段视图，按值移动持有
     */
    concat_view(V1 v1, V2 v2) :
    v1_(_NEFORCE move(v1)),
    v2_(_NEFORCE move(v2)) {}

    /**
     * @brief 获取指向首个元素的迭代器
     * @return 由第一段与第二段起始迭代器构造的迭代器
     */
    auto begin() { return iterator(v1_.begin(), v1_.end(), v2_.begin(), v2_.end()); }

    /**
     * @brief 获取末尾迭代器
     * @return 两段均指向各自末尾、且 in_first_ 为 false 的迭代器
     */
    auto end() { return iterator(v1_.end(), v1_.end(), v2_.end(), v2_.end()); }

    /**
     * @brief 获取指向首个元素的常量迭代器
     * @return 使用两侧常量迭代器构造的迭代器
     */
    auto begin() const { return const_iterator(v1_.begin(), v1_.end(), v2_.begin(), v2_.end()); }

    /**
     * @brief 获取末尾常量迭代器
     * @return 使用两侧常量迭代器构造的末尾迭代器
     */
    auto end() const { return const_iterator(v1_.end(), v1_.end(), v2_.end(), v2_.end()); }

    /**
     * @brief 获取拼接后的元素总数
     * @return 两段大小之和，类型为 size_t
     */
    auto size() const
        requires requires(const remove_const_t<V1>& a, const remove_const_t<V2>& b) {
            { a.size() } -> convertible_to<size_t>;
            { b.size() } -> convertible_to<size_t>;
        }
    {
        return static_cast<size_t>(v1_.size() + v2_.size());
    }
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_CONCAT_VIEW_HPP__
