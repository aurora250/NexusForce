#ifndef NEFORCE_CORE_INTERFACE_INUMERIC_HPP__
#define NEFORCE_CORE_INTERFACE_INUMERIC_HPP__

/**
 * @file inumeric.hpp
 * @brief 数值操作接口
 *
 * 此文件提供了数值类型的通用操作接口，包括算术运算和位运算的CRTP基类，
 * 用于简化数值类型运算符的实现。
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct iarithmetic
 * @brief 算术运算接口基类
 * @tparam T 派生类类型
 *
 * 使用CRTP模式为派生类提供算术运算符的通用实现。
 * 派生类只需实现复合赋值运算符，即可自动获得相应的二元运算符。
 */
template <typename T> struct iarithmetic {
private:
    /**
     * @brief 获取派生类的常量引用
     * @return 派生类的常量引用
     */
    constexpr const T& derived() const noexcept { return static_cast<const T&>(*this); }

    /**
     * @brief 获取派生类的引用
     * @return 派生类的引用
     */
    constexpr T& derived() noexcept { return static_cast<T&>(*this); }

public:
    /**
     * @brief 加法运算符
     * @param other 右操作数
     * @return 两个对象相加的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator+(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator+=(other))) {
        T tmp(derived());
        tmp += other;
        return tmp;
    }

    /**
     * @brief 减法运算符
     * @param other 右操作数
     * @return 两个对象相减的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator-(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator-=(other))) {
        T tmp(derived());
        tmp -= other;
        return tmp;
    }

    /**
     * @brief 乘法运算符
     * @param other 右操作数
     * @return 两个对象相乘的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator*(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator*=(other))) {
        T tmp(derived());
        tmp *= other;
        return tmp;
    }

    /**
     * @brief 除法运算符
     * @param other 右操作数
     * @return 两个对象相除的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator/(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator/=(other))) {
        T tmp(derived());
        tmp /= other;
        return tmp;
    }

    /**
     * @brief 取模运算符
     * @param other 右操作数
     * @return 两个对象取模的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator%(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator%=(other))) {
        T tmp(derived());
        tmp %= other;
        return tmp;
    }

    /**
     * @brief 一元负号运算符
     * @return 对象的相反数
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator-() const noexcept(noexcept(derived().operator-())) {
        return derived().operator-();
    }

    /**
     * @brief 加法赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator+=(const T& other) noexcept(noexcept(derived().operator+=(other))) {
        return derived().operator+=(other);
    }

    /**
     * @brief 减法赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator-=(const T& other) noexcept(noexcept(derived().operator-=(other))) {
        return derived().operator-=(other);
    }

    /**
     * @brief 乘法赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator*=(const T& other) noexcept(noexcept(derived().operator*=(other))) {
        return derived().operator*=(other);
    }

    /**
     * @brief 除法赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator/=(const T& other) { return derived().operator/=(other); }

    /**
     * @brief 取模赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator%=(const T& other) { return derived().operator%=(other); }

    /**
     * @brief 前置自增运算符
     * @return 自增后的对象引用
     */
    NEFORCE_CONSTEXPR14 T& operator++() noexcept(noexcept(derived().operator++())) { return derived().operator++(); }

    /**
     * @brief 后置自增运算符
     * @return 自增前的对象副本
     */
    NEFORCE_CONSTEXPR14 T operator++(int) noexcept(noexcept(derived().operator++())) {
        T tmp(derived());
        ++derived();
        return tmp;
    }

    /**
     * @brief 前置自减运算符
     * @return 自减后的对象引用
     */
    NEFORCE_CONSTEXPR14 T& operator--() noexcept(noexcept(derived().operator--())) { return derived().operator--(); }

    /**
     * @brief 后置自减运算符
     * @return 自减前的对象副本
     */
    NEFORCE_CONSTEXPR14 T operator--(int) noexcept(noexcept(derived().operator--())) {
        T tmp(derived());
        --derived();
        return tmp;
    }
};


/**
 * @struct ibinary
 * @brief 位运算接口基类
 * @tparam T 派生类类型
 *
 * 使用CRTP模式为派生类提供位运算运算符的通用实现。
 * 派生类只需实现复合赋值运算符，即可自动获得相应的二元运算符。
 */
template <typename T> struct ibinary {
private:
    /**
     * @brief 获取派生类的常量引用
     * @return 派生类的常量引用
     */
    constexpr const T& derived() const noexcept { return static_cast<const T&>(*this); }

    /**
     * @brief 获取派生类的引用
     * @return 派生类的引用
     */
    constexpr T& derived() noexcept { return static_cast<T&>(*this); }

public:
    /**
     * @brief 按位与运算符
     * @param other 右操作数
     * @return 两个对象按位与的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator&(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator&=(other))) {
        T tmp(derived());
        tmp &= other;
        return tmp;
    }

    /**
     * @brief 按位或运算符
     * @param other 右操作数
     * @return 两个对象按位或的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator|(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator|=(other))) {
        T tmp(derived());
        tmp |= other;
        return tmp;
    }

    /**
     * @brief 按位异或运算符
     * @param other 右操作数
     * @return 两个对象按位异或的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator^(const T& other) const
            noexcept(noexcept(const_cast<T&>(derived()).operator^=(other))) {
        T tmp(derived());
        tmp ^= other;
        return tmp;
    }

    /**
     * @brief 按位取反运算符
     * @return 对象按位取反的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator~() const noexcept(noexcept(derived().operator~())) {
        return derived().operator~();
    }

    /**
     * @brief 左移运算符
     * @param shift 移位位数
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator<<(const uint32_t shift) const {
        T tmp(derived());
        tmp <<= shift;
        return tmp;
    }

    /**
     * @brief 右移运算符
     * @param shift 移位位数
     * @return 对象右移shift位的结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR14 T operator>>(const uint32_t shift) const {
        T tmp(derived());
        tmp >>= shift;
        return tmp;
    }

    /**
     * @brief 按位与赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator&=(const T& other) noexcept(noexcept(derived().operator&=(other))) {
        return derived().operator&=(other);
    }

    /**
     * @brief 按位或赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator|=(const T& other) noexcept(noexcept(derived().operator|=(other))) {
        return derived().operator|=(other);
    }

    /**
     * @brief 按位异或赋值运算符
     * @param other 右操作数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator^=(const T& other) noexcept(noexcept(derived().operator^=(other))) {
        return derived().operator^=(other);
    }

    /**
     * @brief 左移赋值运算符
     * @param shift 移位位数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator<<=(const uint32_t shift) { return derived().operator<<=(shift); }

    /**
     * @brief 右移赋值运算符
     * @param shift 移位位数
     * @return 当前对象的引用
     */
    NEFORCE_CONSTEXPR14 T& operator>>=(const uint32_t shift) { return derived().operator>>=(shift); }
};

/** @} */ // CRTPInterfaces

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_INTERFACE_INUMERIC_HPP__
