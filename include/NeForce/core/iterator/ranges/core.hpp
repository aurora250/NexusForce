#ifndef NEFORCE_CORE_ITERATOR_RANGES_CORE_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_CORE_HPP__

/**
 * @file core.hpp
 * @brief 范围与视图的核心概念、管道操作符与适配器闭包
 */

#include "NeForce/core/typeinfo/concepts.hpp"
#ifdef NEFORCE_STANDARD_20
#    include "NeForce/core/functional/invoke.hpp"
#    include "NeForce/core/utility/tuple.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @defgroup RangeViews 范围视图
 * @brief 惰性视图、管道适配器与范围组合工具
 * @{
 */

/**
 * @brief 范围概念
 * @tparam R 待检查的类型
 * @note 要求 R& 上可调用 begin() 与 end()，且 begin() 返回 input_iterator，end() 返回该迭代器的 sentinel。
 */
template <typename R>
concept Range = requires(R& r) {
    { r.begin() } -> input_iterator;
    { r.end() } -> sentinel_for<decltype(r.begin())>;
};

/**
 * @brief 公共范围概念
 * @tparam R 待检查的类型
 */
template <typename R>
concept common_range =
        Range<R> && same_as<decltype(_NEFORCE declval<R>().begin()), decltype(_NEFORCE declval<R>().end())>;

/**
 * @brief 视图概念
 * @tparam V 待检查的类型
 */
template <typename V>
concept View = Range<V> && move_constructible<V> && is_view_v<remove_cvref_t<V>>;

/**
 * @brief 有大小视图概念
 * @tparam V 待检查的类型
 */
template <typename V>
concept sized_view = View<V> && requires(V& v) {
    { v.size() } -> convertible_to<size_t>;
};


/**
 * @brief 管道操作符
 * @tparam R 范围类型
 * @tparam Adaptor 适配器类型
 * @param range 输入的 range
 * @param adaptor 适配器
 */
template <Range R, typename Adaptor>
constexpr auto operator|(R&& range, Adaptor&& adaptor) {
    return _NEFORCE forward<Adaptor>(adaptor)(_NEFORCE forward<R>(range));
}


/**
 * @brief 适配器闭包的 CRTP 基类
 * @tparam Derived 派生适配器闭包类型
 */
template <typename Derived>
struct range_adaptor_closure {
protected:
    constexpr range_adaptor_closure() noexcept = default;

public:
    /**
     * @brief 组合两个适配器闭包
     * @tparam OtherClosure 右侧闭包类型
     * @param lhs 左侧闭包
     * @param rhs 右侧闭包
     * @return 依次施加 lhs、rhs 的新闭包
     */
    template <typename OtherClosure>
    friend constexpr auto operator|(Derived lhs, OtherClosure rhs) {
        return [lhs = _NEFORCE move(lhs), rhs = _NEFORCE move(rhs)](auto&& range) {
            return _NEFORCE forward<decltype(range)>(range) | lhs | rhs;
        };
    }
};

/**
 * @struct unavailable_iterator
 * @brief 不可用迭代器占位类型
 *
 * 当底层范围不支持常量遍历时，作为视图常量迭代器别名的占位类型，使视图类型本身仍然可实例化，
 * 只是其 begin() const / end() const 因约束不满足而不可用。该类型的对象不可解引用。
 */
struct unavailable_iterator {
    using iterator_category = input_iterator_tag; ///< 迭代器类别
    using value_type = char;                      ///< 元素类型
    using difference_type = ptrdiff_t;            ///< 迭代器距离类型
    using pointer = const char*;                  ///< 元素指针类型
    using reference = const char&;                ///< 元素引用类型

    /**
     * @brief 解引用
     * @return 常量字符引用
     */
    NEFORCE_NODISCARD constexpr reference operator*() const;

    /**
     * @brief 前置递增（空操作）
     * @return 自身引用
     */
    constexpr unavailable_iterator& operator++() { return *this; }

    /**
     * @brief 后置递增（空操作）
     * @return 自身副本
     */
    constexpr unavailable_iterator operator++(int) { return *this; }

    /**
     * @brief 相等比较
     * @return 恒为 true
     */
    NEFORCE_NODISCARD friend constexpr bool operator==(const unavailable_iterator& /*unused*/,
                                                       const unavailable_iterator& /*unused*/) {
        return true;
    }
};

/**
 * @struct const_range_traits
 * @brief 底层范围的常量迭代器类型萃取
 * @tparam V 视图类型
 * @tparam Available 底层是否支持常量遍历
 */
template <typename V, bool Available = Range<add_const_t<V>>>
struct const_range_traits {
    using iterator = unavailable_iterator; ///< 底层常量迭代器类型
    using sentinel = unavailable_iterator; ///< 底层常量哨兵类型
};

/**
 * @brief 底层支持常量遍历时的特化
 * @tparam V 视图类型
 */
template <typename V>
struct const_range_traits<V, true> {
    using iterator = decltype(_NEFORCE declval<const V&>().begin()); ///< 底层常量迭代器类型
    using sentinel = decltype(_NEFORCE declval<const V&>().end());   ///< 底层常量哨兵类型
};


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_CORE_HPP__
