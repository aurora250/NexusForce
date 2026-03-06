#ifndef NEFORCE_CORE_ITERATOR_REVERSE_ITERATOR_HPP__
#define NEFORCE_CORE_ITERATOR_REVERSE_ITERATOR_HPP__

/**
 * @file reverse_iterator.hpp
 * @brief 反向迭代器
 *
 * 此文件提供了反向迭代器的实现，用于以相反顺序遍历容器。
 */

#include "NeForce/core/algorithm/iterator.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ReverseIterator 反向迭代器
 * @brief 反向遍历容器的迭代器适配器
 * @{
 */

/**
 * @class reverse_iterator
 * @brief 反向迭代器模板类
 * @tparam Iterator 底层迭代器类型
 *
 * 将双向迭代器包装为反向迭代器，使得递增操作变为递减，递减操作变为递增。
 * 用于从容器末尾向开头反向遍历。
 */
template <typename Iterator>
class reverse_iterator {
    static_assert(is_ranges_bid_iter_v<Iterator>, "reverse iterator requires bidirectional iterator.");

public:
    using iterator_category = iter_category_t<Iterator>;    ///< 迭代器类别
    using value_type        = iter_value_t<Iterator>;       ///< 值类型
    using difference_type   = iter_difference_t<Iterator>;  ///< 差值类型
    using pointer           = iter_pointer_t<Iterator>;     ///< 指针类型
    using reference         = iter_reference_t<Iterator>;   ///< 引用类型

private:
    Iterator current;  ///< 底层迭代器

public:
    /**
     * @brief 默认构造函数
     */
    constexpr reverse_iterator() = default;

    /**
     * @brief 从底层迭代器构造
     * @param x 底层迭代器
     */
    constexpr explicit reverse_iterator(Iterator x)
    noexcept(is_nothrow_move_constructible_v<Iterator>)
    : current(_NEFORCE move(x)) {}

    /**
     * @brief 从其他类型的反向迭代器构造
     * @tparam U 其他迭代器类型
     * @param x 其他反向迭代器
     *
     * 允许从兼容迭代器类型的反向迭代器构造。
     */
    template <typename U>
#ifdef NEFORCE_STANDARD_20
        requires(!same_as<U, Iterator> && convertible_to<const U&, Iterator>)
#endif // NEFORCE_STANDARD_20
    constexpr explicit reverse_iterator(const reverse_iterator<U>& x)
    noexcept(is_nothrow_constructible_v<Iterator, const U&>)
    : current(x.current) {}

    /**
     * @brief 从其他类型的反向迭代器赋值
     * @tparam U 其他迭代器类型
     * @param x 其他反向迭代器
     * @return 当前反向迭代器的引用
     */
    template <typename U>
#ifdef NEFORCE_STANDARD_20
        requires(!same_as<U, Iterator> && convertible_to<const U&, Iterator>
    && assignable_from<Iterator&, const U&>)
#endif // NEFORCE_STANDARD_20
    constexpr reverse_iterator& operator =(const reverse_iterator<U>& x)
    noexcept(is_nothrow_assignable<reverse_iterator&, const U&>::value) {
        current = x.current;
        return *this;
    }

    NEFORCE_CONSTEXPR20 ~reverse_iterator() noexcept = default;  ///< 析构函数

    /**
     * @brief 解引用运算符
     * @return 当前元素的引用
     *
     * 返回底层迭代器前一个位置的元素引用。
     */
    NEFORCE_NODISCARD constexpr reference operator *() const
        noexcept(is_nothrow_copy_assignable<Iterator>::value && noexcept(*--(_NEFORCE declval<Iterator&>()))) {
        Iterator iter = current;
        return *--iter;
    }

    /**
     * @brief 成员访问运算符
     * @return 当前元素的指针
     *
     * 返回底层迭代器前一个位置的元素指针。
     */
    NEFORCE_NODISCARD constexpr pointer operator ->() const
        noexcept(is_nothrow_copy_constructible<Iterator>::value && noexcept(--(_NEFORCE declval<Iterator&>()))
            && is_nothrow_arrow<Iterator&, pointer>::value)
#ifdef NEFORCE_STANDARD_20
        requires (is_pointer_v<Iterator> || requires(const Iterator it) { it.operator->(); })
#endif // NEFORCE_STANDARD_20
    {
        Iterator tmp = current;
        --tmp;
        return _NEFORCE to_pointer(tmp);
    }

    /**
     * @brief 前缀递增运算符
     * @return 当前反向迭代器的引用
     *
     * 反向迭代器的递增对应底层迭代器的递减。
     */
    constexpr reverse_iterator& operator ++()
         noexcept(noexcept(--current)) {
        --current;
        return *this;
    }

    /**
     * @brief 后缀递增运算符
     * @return 递增前的反向迭代器副本
     */
    constexpr reverse_iterator operator ++(int)
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(--current)) {
        reverse_iterator tmp = *this;
        --current;
        return tmp;
    }

    /**
     * @brief 前缀递减运算符
     * @return 当前反向迭代器的引用
     *
     * 反向迭代器的递减对应底层迭代器的递增。
     */
    constexpr reverse_iterator& operator --()
        noexcept(noexcept(++current)) {
        ++current;
        return *this;
    }

    /**
     * @brief 后缀递减运算符
     * @return 递减前的反向迭代器副本
     */
    constexpr reverse_iterator operator --(int)
        noexcept(is_nothrow_copy_constructible_v<Iterator> && noexcept(++current)) {
        reverse_iterator tmp = *this;
        ++current;
        return tmp;
    }

    /**
     * @brief 加法运算符
     * @param n 前进距离
     * @return 前进n个位置后的反向迭代器
     *
     * 反向迭代器的前进对应底层迭代器的后退。
     */
    constexpr reverse_iterator operator +(const difference_type n) const
        noexcept(noexcept(reverse_iterator(current - n))) {
        return reverse_iterator(current - n);
    }

    /**
     * @brief 复合加法赋值运算符
     * @param n 前进距离
     * @return 当前反向迭代器的引用
     */
    constexpr reverse_iterator& operator +=(const difference_type n)
        noexcept(noexcept(current -= n)) {
        current -= n;
        return *this;
    }

    /**
     * @brief 减法运算符
     * @param n 后退距离
     * @return 后退n个位置后的反向迭代器
     *
     * 反向迭代器的后退对应底层迭代器的前进。
     */
    constexpr reverse_iterator operator -(const difference_type n) const
        noexcept(noexcept(reverse_iterator(current + n))) {
        return reverse_iterator(current + n);
    }

    /**
     * @brief 复合减法赋值运算符
     * @param n 后退距离
     * @return 当前反向迭代器的引用
     */
    constexpr reverse_iterator& operator -=(const difference_type n)
        noexcept(noexcept(current += n)) {
        current += n;
        return *this;
    }

    /**
     * @brief 下标运算符
     * @param n 偏移量
     * @return 偏移n个位置后的元素引用
     */
    constexpr reference operator [](const difference_type n) const
        noexcept(noexcept(_NEFORCE declcopy<reference>(reverse_iterator(current - n)))) {
        return *(*this + n);
    }

    /**
     * @brief 获取底层迭代器
     * @return 底层迭代器的常量引用
     */
    NEFORCE_NODISCARD constexpr const Iterator& base() const noexcept {
        return current;
    }
};

/**
 * @brief 相等比较运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 如果两个反向迭代器指向相同位置则返回true
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool operator ==(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() == y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires { { x.base() == y.base() } -> convertible_to<bool>; }
#endif // NEFORCE_STANDARD_20
{
    return x.base() == y.base();
}

/**
 * @brief 不等比较运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 如果两个反向迭代器指向不同位置则返回true
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool operator !=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() != y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires { { x.base() != y.base() } -> convertible_to<bool>; }
#endif // NEFORCE_STANDARD_20
{
    return x.base() != y.base();
}

/**
 * @brief 小于比较运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 如果第一个反向迭代器在第二个之前则返回true
 * @note 对于反向迭代器，x < y 当且仅当 x.base() > y.base()
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool operator <(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() > y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires { { x.base() > y.base() } -> convertible_to<bool>; }
#endif
{
    return x.base() > y.base();
}

/**
 * @brief 大于比较运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 如果第一个反向迭代器在第二个之后则返回true
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool operator >(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() < y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires { { x.base() < y.base() } -> convertible_to<bool>; }
#endif // NEFORCE_STANDARD_20
{
    return x.base() < y.base();
}

/**
 * @brief 小于等于比较运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 如果第一个反向迭代器不在第二个之后则返回true
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool operator <=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() >= y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires { { x.base() >= y.base() } -> convertible_to<bool>; }
#endif // NEFORCE_STANDARD_20
{
    return x.base() >= y.base();
}

/**
 * @brief 大于等于比较运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 如果第一个反向迭代器不在第二个之前则返回true
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr bool operator >=(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(_NEFORCE declcopy<bool>(x.base() <= y.base())))
#ifdef NEFORCE_STANDARD_20
    requires requires { { x.base() <= y.base() } -> convertible_to<bool>; }
#endif // NEFORCE_STANDARD_20
{
    return x.base() <= y.base();
}

/**
 * @brief 距离运算符
 * @tparam Iterator1 第一个反向迭代器类型
 * @tparam Iterator2 第二个反向迭代器类型
 * @param x 第一个反向迭代器
 * @param y 第二个反向迭代器
 * @return 两个反向迭代器之间的距离
 * @note 对于反向迭代器，x - y = y.base() - x.base()
 */
template <typename Iterator1, typename Iterator2>
NEFORCE_NODISCARD constexpr decltype(auto) operator -(
    const reverse_iterator<Iterator1>& x, const reverse_iterator<Iterator2>& y)
    noexcept(noexcept(y.base() - x.base())) {
    return y.base() - x.base();
}

/**
 * @brief 与整数相加的运算符
 * @tparam Iterator 反向迭代器类型
 * @param n 距离
 * @param x 反向迭代器
 * @return 前进n个位置后的反向迭代器
 */
template <typename Iterator>
constexpr reverse_iterator<Iterator> operator +(
    iter_difference_t<Iterator> n, const reverse_iterator<Iterator>& x)
    noexcept(noexcept(x + n)) {
    return x + n;
}

/**
 * @brief 创建反向迭代器
 * @tparam Iterator 迭代器类型
 * @param it 底层迭代器
 * @return 包装该迭代器的反向迭代器
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr reverse_iterator<Iterator>
make_reverse_iterator(Iterator it) noexcept(is_nothrow_move_constructible_v<Iterator>) {
    return reverse_iterator<Iterator>(_NEFORCE move(it));
}

/** @} */ // ReverseIterator

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ITERATOR_REVERSE_ITERATOR_HPP__
