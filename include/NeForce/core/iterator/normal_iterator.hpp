#ifndef NEFORCE_CORE_ITERATOR_NORMAL_ITERATOR_HPP__
#define NEFORCE_CORE_ITERATOR_NORMAL_ITERATOR_HPP__

/**
 * @file normal_iterator.hpp
 * @brief 标准迭代器适配器
 *
 * 此文件提供了标准迭代器适配器的实现，用于包装底层迭代器并为其提供标准迭代器接口。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup NormalIterators 标准迭代器
 * @brief 包装底层迭代器的标准适配器
 * @{
 */

/**
 * @class normal_iterator
 * @brief 标准迭代器适配器
 * @tparam Iterator 底层迭代器类型
 *
 * 将底层迭代器包装为标准迭代器接口，提供完整的迭代器操作和关系运算符，确保所有迭代器具有一致的接口。
 */
template <typename Iterator>
class normal_iterator {
public:
    using iterator_type     = Iterator;                       ///< 底层迭代器类型
    using iterator_category = iter_category_t<Iterator>;      ///< 迭代器类别
    using value_type        = iter_value_t<Iterator>;         ///< 元素值类型
    using difference_type   = iter_difference_t<Iterator>;    ///< 差值类型
    using reference         = iter_reference_t<Iterator>;     ///< 引用类型
    using pointer           = iter_pointer_t<Iterator>;       ///< 指针类型
    
private:
    Iterator current_{};  ///< 底层迭代器实例
    
public:
    /**
     * @brief 默认构造函数
     */
    constexpr normal_iterator()
    noexcept(is_nothrow_default_constructible_v<Iterator>) {}

    /**
     * @brief 从底层迭代器构造
     * @param iter 底层迭代器
     */
    explicit constexpr normal_iterator(const Iterator& iter)
    noexcept(is_nothrow_copy_constructible_v<Iterator>)
    : current_(iter) {}

    /**
     * @brief 从其他normal_iterator转换构造
     * @tparam Iter 可转换为Iterator的类型
     * @param other 其他normal_iterator实例
     */
    template <typename Iter, enable_if_t<is_convertible_v<Iter, Iterator>, int> = 0>
    constexpr normal_iterator(const normal_iterator<Iter>& other)
    noexcept(is_nothrow_copy_constructible_v<Iterator>)
    : current_(other.base()) {}

    /**
     * @brief 解引用操作符
     * @return 当前元素的引用
     */
    constexpr reference operator *() const noexcept { return *current_; }

    /**
     * @brief 成员访问操作符
     * @return 当前元素的指针
     */
    constexpr pointer operator ->() const noexcept { return current_; }

    /**
     * @brief 前置自增操作符
     * @return 自增后的迭代器引用
     */
    constexpr normal_iterator& operator ++()
    noexcept(noexcept(++current_)) {
        ++current_;
        return *this;
    }

    /**
     * @brief 后置自增操作符
     * @return 自增前的迭代器副本
     */
    constexpr normal_iterator operator ++(int)
    noexcept(noexcept(current_++)) {
        return normal_iterator(current_++);
    }

    /**
     * @brief 前置自减操作符
     * @return 自减后的迭代器引用
     */
    constexpr normal_iterator& operator --()
    noexcept(noexcept(--current_)) {
        --current_;
        return *this;
    }

    /**
     * @brief 后置自减操作符
     * @return 自减前的迭代器副本
     */
    constexpr normal_iterator operator --(int)
    noexcept(noexcept(current_--)) {
        return normal_iterator(current_--);
    }

    /**
     * @brief 下标访问操作符
     * @param n 偏移量
     * @return 偏移位置元素的引用
     */
    constexpr reference operator [](difference_type n) const noexcept {
        return current_[n];
    }

    /**
     * @brief 加法赋值操作符
     * @param n 要增加的偏移量
     * @return 修改后的迭代器引用
     */
    constexpr normal_iterator& operator +=(difference_type n)
    noexcept(noexcept(current_ += n)) {
        current_ += n;
        return *this;
    }

    /**
     * @brief 加法操作符
     * @param n 要增加的偏移量
     * @return 增加偏移后的新迭代器
     */
    constexpr normal_iterator operator +(difference_type n) const
    noexcept(noexcept(current_ + n)) {
        return normal_iterator(current_ + n);
    }

    /**
     * @brief 减法赋值操作符
     * @param n 要减少的偏移量
     * @return 修改后的迭代器引用
     */
    constexpr normal_iterator& operator -=(difference_type n)
    noexcept(noexcept(current_ -= n)) {
        current_ -= n;
        return *this;
    }

    /**
     * @brief 减法操作符
     * @param n 要减少的偏移量
     * @return 减少偏移后的新迭代器
     */
    constexpr normal_iterator operator -(difference_type n) const
    noexcept(noexcept(current_ - n)) {
        return normal_iterator(current_ - n);
    }

    /**
     * @brief 获取底层迭代器
     * @return 底层迭代器的常量引用
     */
    constexpr const Iterator& base() const noexcept { 
        return current_;
    }
};

/**
 * @brief 相等比较运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr bool operator ==(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept { 
    return lhs.base() == rhs.base(); 
}

/**
 * @brief 相等比较运算符
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr bool operator ==(
    const normal_iterator<Iterator>& lhs,
    const normal_iterator<Iterator>& rhs) noexcept { 
    return lhs.base() == rhs.base(); 
}

/**
 * @brief 不等比较运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr bool operator !=(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept { 
    return lhs.base() != rhs.base(); 
}

/**
 * @brief 不等比较运算符
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr bool operator !=(
    const normal_iterator<Iterator>& lhs,
    const normal_iterator<Iterator>& rhs) noexcept { 
    return lhs.base() != rhs.base(); 
}

/**
 * @brief 小于比较运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr bool operator <(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept { 
    return lhs.base() < rhs.base(); 
}

/**
 * @brief 小于比较运算符
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr bool operator <(
    const normal_iterator<Iterator>& lhs,
    const normal_iterator<Iterator>& rhs) noexcept { 
    return lhs.base() < rhs.base(); 
}

/**
 * @brief 大于比较运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr bool operator >(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept { 
    return lhs.base() > rhs.base(); 
}

/**
 * @brief 大于比较运算符
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr bool operator >(
    const normal_iterator<Iterator>& lhs,
    const normal_iterator<Iterator>& rhs) noexcept { 
    return lhs.base() > rhs.base(); 
}

/**
 * @brief 小于等于比较运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr bool operator <=(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept { 
    return lhs.base() <= rhs.base(); 
}

/**
 * @brief 小于等于比较运算符
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr bool operator <=(
    const normal_iterator<Iterator>& lhs,
    const normal_iterator<Iterator>& rhs) noexcept { 
    return lhs.base() <= rhs.base(); 
}

/**
 * @brief 大于等于比较运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr bool operator >=(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept { 
    return lhs.base() >= rhs.base(); 
}

/**
 * @brief 大于等于比较运算符
 * @tparam Iterator 迭代器类型
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr bool operator >=(
    const normal_iterator<Iterator>& lhs,
    const normal_iterator<Iterator>& rhs) noexcept { 
    return lhs.base() >= rhs.base(); 
}

/**
 * @brief 减法运算符
 * @tparam LeftIter 左操作数迭代器类型
 * @tparam RightIter 右操作数迭代器类型
 * @return 两个迭代器之间的距离
 */
template <typename LeftIter, typename RightIter>
NEFORCE_NODISCARD constexpr auto operator -(
    const normal_iterator<LeftIter>& lhs,
    const normal_iterator<RightIter>& rhs) noexcept
    -> decltype(lhs.base() - rhs.base()) { 
    return lhs.base() - rhs.base(); 
}

/**
 * @brief 减法运算符
 * @tparam Iterator 迭代器类型
 * @return 两个迭代器之间的距离
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr typename normal_iterator<Iterator>::difference_type
operator -(const normal_iterator<Iterator>& lhs,
           const normal_iterator<Iterator>& rhs) noexcept {
    return lhs.base() - rhs.base(); 
}

/**
 * @brief 加法运算符
 * @tparam Iterator 迭代器类型
 * @param n 偏移量
 * @param iter 迭代器
 * @return 偏移后的新迭代器
 */
template <typename Iterator>
NEFORCE_NODISCARD constexpr normal_iterator<Iterator>
operator +(iter_difference_t<normal_iterator<Iterator>> n,
           const normal_iterator<Iterator>& iter) noexcept {
    return normal_iterator<Iterator>(iter.base() + n); 
}

/** @} */ // NormalIterators

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ITERATOR_NORMAL_ITERATOR_HPP__
