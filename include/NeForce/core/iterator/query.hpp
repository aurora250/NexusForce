#ifndef NEFORCE_CORE_ITERATOR_QUERY_HPP__
#define NEFORCE_CORE_ITERATOR_QUERY_HPP__

/**
 * @file query.hpp
 * @brief 链式查询
 *
 * 此文件提供链式查询对象的定义以及由范围构造查询链的入口。
 */

#include "NeForce/core/interface/iqueryable.hpp"
#include "NeForce/core/iterator/ranges/ref_view.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @class basic_query
 * @brief 惰性查询链
 * @tparam V 底层视图类型
 */
template <typename V>
class basic_query : public view_base<basic_query<V>>, public iqueryable<basic_query<V>> {
public:
    using base_view_t = V;                                                 ///< 底层视图类型
    using iterator = decltype(_NEFORCE declval<V&>().begin());             ///< 迭代器类型
    using sentinel = decltype(_NEFORCE declval<V&>().end());               ///< 哨兵类型
    using const_iterator = decltype(_NEFORCE declval<const V&>().begin()); ///< 常量迭代器类型
    using const_sentinel = decltype(_NEFORCE declval<const V&>().end());   ///< 常量哨兵类型
    using value_type = iter_value_t<iterator>;                             ///< 元素类型
    using difference_type = iter_difference_t<iterator>;                   ///< 迭代器距离类型

private:
    V base_;

public:
    /**
     * @brief 默认构造函数
     */
    constexpr basic_query() = default;

    /**
     * @brief 由底层视图构造查询
     * @param base 底层视图
     */
    constexpr explicit basic_query(V base) :
    base_(_NEFORCE move(base)) {}

    /**
     * @brief 获取底层视图
     * @return 底层视图的引用
     */
    NEFORCE_NODISCARD constexpr const V& view() const noexcept { return base_; }

    /**
     * @brief 获取底层视图
     * @return 底层视图的引用
     */
    NEFORCE_NODISCARD constexpr V& view() noexcept { return base_; }

    /**
     * @brief 由给定视图构造新的查询对象
     * @tparam NV 新视图类型
     * @param view 新视图
     * @return 持有新视图的查询对象
     */
    template <typename NV>
    NEFORCE_NODISCARD constexpr auto make(NV view) const {
        return basic_query<NV>{_NEFORCE move(view)};
    }

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    constexpr iterator begin() { return base_.begin(); }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    constexpr sentinel end() { return base_.end(); }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    constexpr const_iterator begin() const
        requires Range<const V>
    {
        return base_.begin();
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    constexpr const_sentinel end() const
        requires Range<const V>
    {
        return base_.end();
    }

    /**
     * @brief 获取元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD constexpr size_t size() const
        requires sized_view<remove_cvref_t<V>>
    {
        return static_cast<size_t>(base_.size());
    }

    /**
     * @brief 获取元素个数的估计值
     * @return 精确元素个数，无法获知时返回 0
     */
    NEFORCE_NODISCARD constexpr size_t size_hint() const {
        if constexpr (sized_view<remove_cvref_t<V>>) {
            return static_cast<size_t>(base_.size());
        } else {
            return 0;
        }
    }
};

/**
 * @brief 构造惰性查询链
 * @tparam R 范围类型
 * @param range 源范围
 * @return 查询对象
 * @note 传入左值容器时查询只持有引用，传入右值容器时拥有该容器，传入视图时按视图语义处理。
 */
template <Range R>
NEFORCE_NODISCARD constexpr auto from(R&& range) {
    return basic_query<decltype(all(_NEFORCE forward<R>(range)))>{all(_NEFORCE forward<R>(range))};
}

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_QUERY_HPP__
