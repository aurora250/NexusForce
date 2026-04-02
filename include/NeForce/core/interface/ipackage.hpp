#ifndef NEFORCE_CORE_INTERFACE_IPACKAGE_HPP__
#define NEFORCE_CORE_INTERFACE_IPACKAGE_HPP__

/**
 * @file ipackage.hpp
 * @brief 基础数值包装接口
 *
 * 此文件提供了数值类型包装的基础接口。
 * 支持算术运算、位运算和比较操作。
 */

#include "NeForce/core/interface/icommon.hpp"
#include "NeForce/core/interface/inumeric.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CRTPInterfaces CRTP接口
 * @brief 提供基本功能的CRTP基类
 * @{
 */

/**
 * @struct ipackage
 * @brief 基础数值包装接口
 * @tparam T 派生类类型
 * @tparam PackageT 包装的数值类型
 *
 * 提供数值类型的通用接口，包括算术运算、位运算和比较操作。
 * @note PackageT必须是算术类型。
 */
template <typename T, typename PackageT> struct ipackage : icommon<T>, iarithmetic<T>, ibinary<T> {
    static_assert(is_arithmetic_v<PackageT>, "PackageT must be arithmetic.");

private:
    /**
     * @brief 获取常量派生类引用
     * @return 常量派生类引用
     */
    constexpr const T& derived() const noexcept { return static_cast<const T&>(*this); }

    /**
     * @brief 获取派生类引用
     * @return 派生类引用
     */
    constexpr T& derived() noexcept { return static_cast<T&>(*this); }

public:
    using package_type = PackageT; ///< 包装类型

protected:
    package_type value_{static_cast<package_type>(0)}; ///< 存储的数值

public:
    constexpr ipackage() noexcept = default;

    constexpr ipackage(const ipackage& other) noexcept = default;
    constexpr ipackage(ipackage&& other) noexcept = default;

    constexpr ipackage& operator=(const ipackage& other) noexcept = default;
    constexpr ipackage& operator=(ipackage&& other) noexcept = default;

    /**
     * @brief 构造函数
     * @param value 初始数值
     *
     * 使用指定数值构造包装对象。
     */
    explicit constexpr ipackage(package_type value) noexcept :
    value_(value) {}

protected:
    /**
     * @brief 受保护的析构函数
     *
     * 防止通过基类指针删除派生类对象。
     */
    NEFORCE_CONSTEXPR20 ~ipackage() = default;

public:
    /**
     * @brief 类型转换操作符
     * @return 转换为包装类型的数值
     */
    NEFORCE_NODISCARD explicit constexpr operator package_type() const noexcept { return value_; }

    /**
     * @brief 获取数值
     * @return 存储的数值
     */
    NEFORCE_NODISCARD constexpr package_type value() const noexcept { return value_; }

    /**
     * @brief 转换为64位整数
     * @return 转换为int64_t的数值
     */
    NEFORCE_NODISCARD constexpr int64_t to_int64() const noexcept { return static_cast<int64_t>(value_); }

    /**
     * @brief 获取类型字节大小
     * @return 包装类型占用的字节数
     */
    NEFORCE_NODISCARD static constexpr size_t bytes() noexcept { return sizeof(package_type); }

    /**
     * @brief 获取类型位大小
     * @return 包装类型占用的位数
     */
    NEFORCE_NODISCARD static constexpr size_t bits() noexcept { return sizeof(package_type) * 8; }

    /**
     * @brief 计算哈希值
     * @return 数值的哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept { return _NEFORCE hash<package_type>()(value_); }

    /**
     * @brief 交换内容
     * @param other 要交换的另一个对象
     */
    constexpr void swap(T& other) noexcept { _NEFORCE swap(value_, other.value_); }

    /**
     * @brief 相等比较操作符
     * @param other 右侧对象
     * @return 两个对象的数值是否相等
     */
    NEFORCE_NODISCARD constexpr bool operator==(const T& other) const noexcept { return value_ == other.value_; }

    /**
     * @brief 小于比较操作符
     * @param other 右侧对象
     * @return 当前对象的数值是否小于右侧对象的数值
     */
    NEFORCE_NODISCARD constexpr bool operator<(const T& other) const noexcept { return value_ < other.value_; }

    /**
     * @brief 加法赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     */
    constexpr T& operator+=(const T& other) noexcept {
        value_ += other.value_;
        return derived();
    }

    /**
     * @brief 减法赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     */
    constexpr T& operator-=(const T& other) noexcept {
        value_ -= other.value_;
        return derived();
    }

    /**
     * @brief 乘法赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     */
    constexpr T& operator*=(const T& other) noexcept {
        value_ *= other.value_;
        return derived();
    }

    /**
     * @brief 除法赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     * @exception math_exception 除数为0时
     */
    constexpr T& operator/=(const T& other) {
        if (other.value_ == 0) {
            NEFORCE_THROW_EXCEPTION(math_exception("Division by zero"));
        }
        value_ /= other.value_;
        return derived();
    }

    /**
     * @brief 取模赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     * @exception math_exception 除数为0时
     */
    constexpr T& operator%=(const T& other) {
        value_ = _NEFORCE float_mod(value_, other.value_);
        return derived();
    }

    /**
     * @brief 一元负号操作符
     * @return 取负后的新对象
     */
    NEFORCE_NODISCARD constexpr T operator-() const noexcept { return T(-value_); }

    /**
     * @brief 前置递增操作符
     * @return 递增后的自身引用
     */
    constexpr T& operator++() noexcept {
        ++value_;
        return derived();
    }

    /**
     * @brief 前置递减操作符
     * @return 递减后的自身引用
     */
    constexpr T& operator--() noexcept {
        --value_;
        return derived();
    }

    /**
     * @brief 按位取反操作符
     * @return 按位取反后的新对象
     */
    constexpr T operator~() const noexcept { return T{~value_}; }

    /**
     * @brief 按位与赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     */
    constexpr T& operator&=(const T& other) noexcept {
        value_ &= other.value_;
        return derived();
    }

    /**
     * @brief 按位或赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     */
    constexpr T& operator|=(const T& other) noexcept {
        value_ |= other.value_;
        return derived();
    }

    /**
     * @brief 按位异或赋值操作符
     * @param other 右侧对象
     * @return 自身引用
     */
    constexpr T& operator^=(const T& other) noexcept {
        value_ ^= other.value_;
        return derived();
    }

    /**
     * @brief 左移赋值操作符
     * @param shift 移位位数
     * @return 自身引用
     * @throws value_exception 移位位数超出范围时抛出
     */
    constexpr T& operator<<=(const uint32_t shift) {
        if (shift >= 64) {
            NEFORCE_THROW_EXCEPTION(value_exception("Shift count out of range"));
        }
        value_ <<= shift;
        return derived();
    }

    /**
     * @brief 右移赋值操作符
     * @param shift 移位位数
     * @return 自身引用
     * @throws value_exception 移位位数超出范围时抛出
     */
    constexpr T& operator>>=(const uint32_t shift) {
        if (shift >= 64) {
            NEFORCE_THROW_EXCEPTION(value_exception("Shift count out of range"));
        }
        value_ >>= shift;
        return derived();
    }
};

/** @} */ // CRTPInterfaces

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_INTERFACE_IPACKAGE_HPP__
