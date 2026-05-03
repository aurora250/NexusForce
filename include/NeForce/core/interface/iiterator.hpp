#ifndef NEFORCE_CORE_INTERFACE_IITERATOR_HPP__
#define NEFORCE_CORE_INTERFACE_IITERATOR_HPP__

/**
 * @file iiterator.hpp
 * @brief 迭代器接口
 *
 * 此文件定义了迭代器接口，为迭代器类型提供统一的操作接口。
 */

#include "NeForce/core/interface/icommon.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct iiterator
 * @brief 迭代器接口模板
 * @tparam Iterator 派生迭代器类型
 *
 * 为迭代器类型提供完整的操作接口，包括解引用、递增、递减、算术运算和比较运算。
 * 基于CRTP模式，要求派生类实现核心操作方法。
 */
template <typename Iterator>
struct iiterator : icomparable<Iterator> {
private:
    constexpr Iterator& derived() noexcept { return static_cast<Iterator&>(*this); }

    constexpr const Iterator& derived() const noexcept { return static_cast<const Iterator&>(*this); }

public:
    /**
     * @brief 解引用操作符
     * @return 迭代器指向的元素引用或值
     */
    NEFORCE_NODISCARD constexpr decltype(auto) operator*() const noexcept { return derived().dereference(); }

    /**
     * @brief 成员访问操作符
     * @return 指向元素的指针
     */
    NEFORCE_NODISCARD constexpr decltype(auto) operator->() const noexcept { return &(derived().dereference()); }

    /**
     * @brief 前置递增操作符
     * @return 递增后的迭代器引用
     */
    constexpr Iterator& operator++() noexcept {
        derived().increment();
        return derived();
    }

    /**
     * @brief 后置递增操作符
     * @return 递增前的迭代器副本
     */
    constexpr Iterator operator++(int) noexcept {
        Iterator temp = derived();
        derived().increment();
        return temp;
    }

    /**
     * @brief 前置递减操作符
     * @return 递减后的迭代器引用
     */
    constexpr Iterator& operator--() noexcept {
        derived().decrement();
        return derived();
    }

    /**
     * @brief 后置递减操作符
     * @return 递减前的迭代器副本
     */
    constexpr Iterator operator--(int) noexcept {
        Iterator temp = derived();
        derived().decrement();
        return temp;
    }

    /**
     * @brief 复合加法赋值操作符
     * @param n 要前进的距离
     * @return 前进后的迭代器引用
     */
    template <typename Distance>
    constexpr Iterator& operator+=(Distance n) noexcept {
        derived().advance(n);
        return derived();
    }

    /**
     * @brief 加法操作符
     * @param n 要前进的距离
     * @return 前进后的新迭代器
     */
    template <typename Distance>
    NEFORCE_NODISCARD constexpr Iterator operator+(Distance n) const noexcept {
        Iterator temp = derived();
        temp.advance(n);
        return temp;
    }

    /**
     * @brief 友元加法操作符
     * @param n 要前进的距离
     * @param it 迭代器
     * @return 前进后的新迭代器
     */
    template <typename Distance>
    NEFORCE_NODISCARD friend constexpr Iterator operator+(Distance n, const iiterator& it) noexcept {
        return it.derived() + n;
    }

    /**
     * @brief 复合减法赋值操作符
     * @param n 要后退的距离
     * @return 后退后的迭代器引用
     */
    template <typename Distance>
    constexpr Iterator& operator-=(Distance n) noexcept {
        derived().advance(-n);
        return derived();
    }

    /**
     * @brief 减法操作符
     * @tparam T 距离类型
     * @param n 要后退的距离
     * @return 后退后的新迭代器
     */
    template <typename T>
    NEFORCE_NODISCARD constexpr enable_if_t<!is_same_v<T, Iterator>, Iterator> operator-(const T n) const noexcept {
        Iterator temp = derived();
        temp.advance(-n);
        return temp;
    }

    /**
     * @brief 减法操作符
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     */
    NEFORCE_NODISCARD constexpr decltype(auto) operator-(const Iterator& other) const noexcept {
        return derived().distance_to(other);
    }
};

/** @} */ // CRTPInterfaces

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_INTERFACE_IITERATOR_HPP__
