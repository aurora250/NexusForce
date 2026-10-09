#ifndef NEFORCE_CORE_ITERATOR_RANGES_ORDER_BY_VIEW_HPP__
#define NEFORCE_CORE_ITERATOR_RANGES_ORDER_BY_VIEW_HPP__

/**
 * @file order_by_view.hpp
 * @brief 按键排序的视图
 */

#include "NeForce/core/algorithm/sort.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/functor.hpp"
#include "NeForce/core/iterator/ranges/core.hpp"
#ifdef NEFORCE_STANDARD_20
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_RANGES__

/**
 * @addtogroup RangeViews 范围视图
 * @{
 */

/**
 * @struct descending_order
 * @brief 反序比较器适配器
 * @tparam Cmp 被包装的比较器类型
 */
template <typename Cmp>
struct descending_order {
    Cmp cmp; ///< 被包装的比较器

    /**
     * @brief 构造反序比较器
     * @param comparator 被包装的比较器
     */
    constexpr explicit descending_order(Cmp comparator) :
    cmp(_NEFORCE move(comparator)) {}

    /**
     * @brief 反序比较
     * @tparam T 左操作数类型
     * @tparam U 右操作数类型
     * @param lhs 左操作数
     * @param rhs 右操作数
     * @return 交换参数后用被包装比较器比较的结果
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD constexpr bool operator()(const T& lhs, const U& rhs) const {
        return cmp(rhs, lhs);
    }
};


/**
 * @class order_by_view
 * @brief 按键排序的视图
 * @tparam V 底层视图类型
 * @tparam Key 键函数类型
 * @tparam Cmp 键比较器类型
 */
template <View V, typename Key, typename Cmp>
class order_by_view : public view_base<order_by_view<V, Key, Cmp>> {
public:
    using base_iterator = decltype(_NEFORCE declval<remove_const_t<V>&>().begin()); ///< 底层迭代器类型
    using value_type = iter_value_t<base_iterator>;                                 ///< 元素类型
    using key_function_type = Key;                          ///< 键函数类型，供 then_by 组合多级排序使用
    using compare_type = Cmp;                               ///< 键比较器类型，供 then_by 组合多级排序使用
    using iterator = typename vector<value_type>::iterator; ///< 迭代器类型
    using const_iterator = iterator;                        ///< 常量迭代器类型，与 iterator 相同
    using sentinel = iterator;                              ///< 哨兵类型
    using const_sentinel = iterator;                        ///< 常量哨兵类型

private:
    V base_;
    Key key_;
    Cmp cmp_;
    mutable vector<value_type> buffer_;
    mutable bool built_ = false;

    void ensure() const {
        if (built_) {
            return;
        }
        buffer_.clear();
        auto& self = const_cast<remove_const_t<V>&>(base_);
        for (auto&& element: self) {
            buffer_.push_back(element);
        }
        _NEFORCE merge_sort(buffer_.begin(), buffer_.end(), [this](const value_type& lhs, const value_type& rhs) {
            return cmp_(key_(lhs), key_(rhs));
        });
        built_ = true;
    }

public:
    /**
     * @brief 默认构造函数
     */
    order_by_view() = default;

    /**
     * @brief 构造排序视图
     * @param base 底层视图
     * @param key 键函数
     * @param cmp 键比较器
     */
    constexpr order_by_view(V base, Key key, Cmp cmp) :
    base_(_NEFORCE move(base)),
    key_(_NEFORCE move(key)),
    cmp_(_NEFORCE move(cmp)) {}

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() {
        ensure();
        return buffer_.begin();
    }

    /**
     * @brief 获取结束哨兵
     * @return 结束哨兵
     */
    sentinel end() {
        ensure();
        return buffer_.end();
    }

    /**
     * @brief 获取起始常量迭代器
     * @return 起始常量迭代器
     */
    const_iterator begin() const {
        ensure();
        return buffer_.begin();
    }

    /**
     * @brief 获取结束常量哨兵
     * @return 结束常量哨兵
     */
    const_sentinel end() const {
        ensure();
        return buffer_.end();
    }

    /**
     * @brief 获取元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t size() const {
        ensure();
        return buffer_.size();
    }

    /**
     * @brief 获取元素个数
     * @return 元素个数
     */
    NEFORCE_NODISCARD size_t size_hint() const { return size(); }

    /**
     * @brief 获取键函数
     * @return 键函数的引用
     */
    NEFORCE_NODISCARD constexpr const Key& key_function() const noexcept { return key_; }

    /**
     * @brief 获取键比较器
     * @return 键比较器的引用
     */
    NEFORCE_NODISCARD constexpr const Cmp& comparator() const noexcept { return cmp_; }

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

/**
 * @struct composite_key
 * @brief 复合键函数
 * @tparam First 主键函数类型
 * @tparam Second 次键函数类型
 *
 * 把两个键函数的结果组合成一个 pair，配合 pair_compare 可实现多级排序
 */
template <typename First, typename Second>
struct composite_key {
    First first;   ///< 主键函数
    Second second; ///< 次键函数

    /**
     * @brief 计算复合键
     * @tparam T 元素类型
     * @param value 元素
     * @return 主键与次键组成的 pair
     */
    template <typename T>
    NEFORCE_NODISCARD constexpr auto operator()(const T& value) const {
        return pair<remove_cvref_t<decltype(first(value))>, remove_cvref_t<decltype(second(value))>>{first(value),
                                                                                                     second(value)};
    }
};


/**
 * @struct pair_compare
 * @brief 按 pair 逐级比较的比较器
 * @tparam FirstCmp 比较 pair 第一个分量的比较器类型
 * @tparam SecondCmp 比较 pair 第二个分量的比较器类型
 *
 * 先比较第一个分量，两者等价时再比较第二个分量
 */
template <typename FirstCmp, typename SecondCmp>
struct pair_compare {
    FirstCmp first;   ///< 第一个分量的比较器
    SecondCmp second; ///< 第二个分量的比较器

    /**
     * @brief 逐级比较两个 pair
     * @tparam T 左操作数类型
     * @tparam U 右操作数类型
     * @param lhs 左操作数
     * @param rhs 右操作数
     * @return 左操作数是否应排在右操作数之前
     */
    template <typename T, typename U>
    NEFORCE_NODISCARD constexpr bool operator()(const T& lhs, const U& rhs) const {
        if (first(lhs.first, rhs.first)) {
            return true;
        }
        if (first(rhs.first, lhs.first)) {
            return false;
        }
        return second(lhs.second, rhs.second);
    }
};


/**
 * @struct is_order_by_view
 * @brief 判断类型是否为排序视图
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_order_by_view : false_type {};

/**
 * @brief 排序视图的特化
 * @tparam V 底层视图类型
 * @tparam Key 键函数类型
 * @tparam Cmp 键比较器类型
 */
template <View V, typename Key, typename Cmp>
struct is_order_by_view<order_by_view<V, Key, Cmp>> : true_type {};

/**
 * @var is_order_by_view_v
 * @brief is_order_by_view 的便捷变量模板
 */
template <typename T>
NEFORCE_INLINE17 constexpr bool is_order_by_view_v = is_order_by_view<remove_cvref_t<T>>::value;


/** @} */ // RangeViews

NEFORCE_END_RANGES__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ITERATOR_RANGES_ORDER_BY_VIEW_HPP__
