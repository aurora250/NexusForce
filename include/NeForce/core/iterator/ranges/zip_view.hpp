#ifndef NEFORCE_CORE_ITERATOR_RANGES_ZIP_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_ZIP_VIEW_HPP__

/**
 * @file zip_view.hpp
 * @brief 逐元素配对两个范围的视图
 */

#include "NeForce/core/iterator/ranges/core.hpp"
#include "NeForce/core/utility/pair.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @class zip_iterator
 * @brief 配对视图的迭代器
 * @tparam Iter1 第一个底层迭代器类型
 * @tparam Sentinel1 第一个底层哨兵类型
 * @tparam Iter2 第二个底层迭代器类型
 * @tparam Sentinel2 第二个底层哨兵类型
 *
 * 同时推进两个底层迭代器，任一底层范围耗尽即结束，产出的元素为两个元素的 pair。
 */
template <input_iterator Iter1, sentinel_for<Iter1> Sentinel1, input_iterator Iter2, sentinel_for<Iter2> Sentinel2>
class zip_iterator {
public:
    using iterator_category = conditional_t<is_base_of_v<forward_iterator_tag, iter_category_t<Iter1>> &&
                                                    is_base_of_v<forward_iterator_tag, iter_category_t<Iter2>>,
                                            forward_iterator_tag, input_iterator_tag>;         ///< 迭代器类别
    using value_type = pair<iter_value_t<Iter1>, iter_value_t<Iter2>>;                         ///< 元素类型
    using reference = pair<iter_reference_t<Iter1>, iter_reference_t<Iter2>>;                  ///< 元素引用类型
    using difference_type = common_type_t<iter_difference_t<Iter1>, iter_difference_t<Iter2>>; ///< 迭代器距离类型
    using pointer = void;                                                                      ///< 元素指针类型

    constexpr zip_iterator() = default;

    /**
     * @brief 构造配对迭代器
     * @param current1 第一个范围的当前迭代器
     * @param end1 第一个范围的哨兵
     * @param current2 第二个范围的当前迭代器
     * @param end2 第二个范围的哨兵
     */
    constexpr zip_iterator(Iter1 current1, Sentinel1 end1, Iter2 current2, Sentinel2 end2) :
    current1_(current1),
    end1_(end1),
    current2_(current2),
    end2_(end2) {}

    /**
     * @brief 解引用迭代器，得到当前元素的 pair
     * @return 两个当前元素组成的 pair
     */
    NEFORCE_NODISCARD constexpr reference operator*() const { return reference{*current1_, *current2_}; }

    /**
     * @brief 前置递增，同时推进两个底层迭代器
     * @return 递增后的迭代器引用
     */
    constexpr zip_iterator& operator++() {
        ++current1_;
        ++current2_;
        return *this;
    }

    /**
     * @brief 后置递增
     * @return 递增前的迭代器副本
     */
    constexpr zip_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 相等比较，任一底层范围耗尽即视为相等
     * @param other 另一个迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool operator==(const zip_iterator& other) const {
        return current1_ == other.current1_ || current2_ == other.current2_;
    }

    /**
     * @brief 不等比较
     * @param other 另一个迭代器
     * @return 是否不相等
     */
    NEFORCE_NODISCARD constexpr bool operator!=(const zip_iterator& other) const { return !(*this == other); }

    /**
     * @brief 判断两个底层范围是否都已耗尽
     * @return 是否已到末尾
     */
    NEFORCE_NODISCARD constexpr bool exhausted() const { return current1_ == end1_ || current2_ == end2_; }

private:
    Iter1 current1_{};
    Sentinel1 end1_{};
    Iter2 current2_{};
    Sentinel2 end2_{};
};


/**
 * @class zip_view
 * @brief 逐元素配对两个范围的视图
 * @tparam V1 第一个底层视图类型
 * @tparam V2 第二个底层视图类型
 *
 * 把两个范围按位置配成 pair 依次产出，长度取两者较短者。
 */
template <View V1, View V2>
class zip_view : public view_base<zip_view<V1, V2>> {
public:
    using base_iterator1 = decltype(_NEFORCE declval<remove_const_t<V1>&>().begin()); ///< 第一个底层迭代器类型
    using base_sentinel1 = decltype(_NEFORCE declval<remove_const_t<V1>&>().end());   ///< 第一个底层哨兵类型
    using base_iterator2 = decltype(_NEFORCE declval<remove_const_t<V2>&>().begin()); ///< 第二个底层迭代器类型
    using base_sentinel2 = decltype(_NEFORCE declval<remove_const_t<V2>&>().end());   ///< 第二个底层哨兵类型

    using const_base_iterator1 = const_range_traits<V1>::iterator; ///< 第一个底层常量迭代器类型
    using const_base_sentinel1 = const_range_traits<V1>::sentinel; ///< 第一个底层常量哨兵类型
    using const_base_iterator2 = const_range_traits<V2>::iterator; ///< 第二个底层常量迭代器类型
    using const_base_sentinel2 = const_range_traits<V2>::sentinel; ///< 第二个底层常量哨兵类型

    using iterator = zip_iterator<base_iterator1, base_sentinel1, base_iterator2, base_sentinel2>; ///< 迭代器类型
    using const_iterator = zip_iterator<const_base_iterator1, const_base_sentinel1, const_base_iterator2,
                                        const_base_sentinel2>; ///< 常量迭代器类型

    using sentinel = iterator;             ///< 哨兵类型
    using const_sentinel = const_iterator; ///< 常量哨兵类型

private:
    V1 base1_;
    V2 base2_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr zip_view() = default;

    /**
     * @brief 构造配对视图
     * @param base1 第一个底层视图
     * @param base2 第二个底层视图
     */
    constexpr zip_view(V1 base1, V2 base2) :
    base1_(_NEFORCE move(base1)),
    base2_(_NEFORCE move(base2)) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() { return iterator(base1_.begin(), base1_.end(), base2_.begin(), base2_.end()); }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() { return sentinel(base1_.end(), base1_.end(), base2_.end(), base2_.end()); }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const
        requires Range<const V1> && Range<const V2>
    {
        return const_iterator(base1_.begin(), base1_.end(), base2_.begin(), base2_.end());
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const
        requires Range<const V1> && Range<const V2>
    {
        return const_sentinel(base1_.end(), base1_.end(), base2_.end(), base2_.end());
    }

    /**
     * @brief 获取元素个数
     * @return 两个底层范围长度的较小者
     * @note 仅当两个底层都可精确计数时可用。
     */
    NEFORCE_NODISCARD constexpr size_t size() const
        requires sized_view<remove_cvref_t<V1>> && sized_view<remove_cvref_t<V2>>
    {
        const auto first = static_cast<size_t>(base1_.size());
        const auto second = static_cast<size_t>(base2_.size());
        return first < second ? first : second;
    }

    /**
     * @brief 获取第一个底层视图
     * @return 第一个底层视图的副本
     */
    constexpr V1 base1() const&
        requires copy_constructible<V1>
    {
        return base1_;
    }

    /**
     * @brief 获取第二个底层视图
     * @return 第二个底层视图的副本
     */
    constexpr V2 base2() const&
        requires copy_constructible<V2>
    {
        return base2_;
    }
};

template <typename V1, typename V2>
zip_view(V1&&, V2&&) -> zip_view<V1, V2>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_ZIP_VIEW_HPP__
