#ifndef NEFORCE_CORE_ITERATOR_RANGES_COMMON_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_COMMON_VIEW_HPP__

/**
 * @file common_view.hpp
 * @brief 统一 begin 与 end 类型的视图
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
 * @brief 统一 begin 与 end 类型的 view
 * @tparam V 底层视图类型
 *
 * 把 begin 与 end 类型不同的 range 统一成 common_range，使 end() 返回与 begin() 同类型的迭代器
 */
template <View V>
    requires(!common_range<V>)
class common_view : public view_base<common_view<V>> {
public:
    using base_iterator = decltype(_NEFORCE declval<V>().begin()); ///< 底层视图的 begin 迭代器类型
    using base_sentinel = decltype(_NEFORCE declval<V>().end());   ///< 底层视图的 end sentinel 类型
    using iterator = base_iterator;                                ///< 迭代器类型，与底层 begin 迭代器相同
    using sentinel = base_iterator;                                ///< sentinel 类型

    /**
     * @brief 默认构造函数
     */
    constexpr common_view() = default;

    /**
     * @brief 由底层视图构造
     * @param base 底层视图，按值移动持有
     */
    constexpr explicit common_view(V base) :
    base_(_NEFORCE move(base)) {}

    /**
     * @brief 获取指向首个元素的迭代器
     * @return 底层视图 begin() 的结果
     */
    constexpr iterator begin() { return base_.begin(); }

    /**
     * @brief 获取末尾迭代器
     * @return 与 begin() 同类型的末尾迭代器；底层 range 为空时等于 begin()
     */
    constexpr iterator end() {
        auto it = base_.begin();
        auto sen = base_.end();
        while (it != sen) {
            ++it;
        }
        return it;
    }

    /**
     * @brief 获取指向首个元素的常量迭代器
     * @return 底层视图 begin() 的结果
     */
    constexpr iterator begin() const
        requires Range<const V>
    {
        return base_.begin();
    }

    /**
     * @brief 获取末尾常量迭代器
     * @return 与 begin() const 同类型的末尾迭代器
     */
    constexpr iterator end() const
        requires Range<const V>
    {
        auto it = base_.begin();
        auto sen = base_.end();
        while (it != sen) {
            ++it;
        }
        return it;
    }

    /**
     * @brief 获取元素个数
     * @return 底层视图 size() 转换得到的元素个数
     */
    constexpr auto size() const
        requires requires(const V& v) {
            { v.size() } -> convertible_to<size_t>;
        }
    {
        return static_cast<size_t>(base_.size());
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

private:
    V base_;
};

template <typename V>
common_view(V&&) -> common_view<decltype(all(_NEFORCE declval<V>()))>;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_COMMON_VIEW_HPP__
