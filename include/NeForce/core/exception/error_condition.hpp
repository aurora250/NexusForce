#ifndef NEFORCE_CORE_EXCEPTION_ERROR_CONDITION_HPP__
#define NEFORCE_CORE_EXCEPTION_ERROR_CONDITION_HPP__

/**
 * @file error_condition.hpp
 * @brief 错误条件
 *
 * 此文件提供了错误条件类，用于表示跨类别的可移植错误条件。
 * 与 error_code 不同，error_condition 独立于具体错误类别，
 * 用于判断错误是否属于某一语义类别。
 */

#include "NeForce/core/exception/error_category.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ErrorCode 错误码
 * @{
 */

/**
 * @class error_condition
 * @brief 错误条件
 *
 * 错误条件表示跨类别的可移植错误语义，用于在不依赖具体错误类别的
 * 情况下判断错误类型。默认关联 generic_category()。
 */
class error_condition : public icomparable<error_condition> {
private:
    int value_{0};                                        ///< 错误条件值
    const error_category* category_{&generic_category()}; ///< 错误类别指针

public:
    /**
     * @brief 默认构造函数
     *
     * 构造值为0、类别为 generic_category() 的空错误条件。
     */
    error_condition() noexcept = default;

    /**
     * @brief 构造函数
     * @param val 错误条件值
     * @param cat 错误类别
     */
    error_condition(int val, const error_category& cat) noexcept :
    value_(val),
    category_(&cat) {}

    /**
     * @brief 从 errc 枚举构造
     * @param e 错误码枚举值
     *
     * 等价于 make_error_condition(e)，类别为 generic_category()。
     */
    error_condition(errc e) noexcept { *this = make_error_condition(e); }

    /**
     * @brief 赋值错误条件值和类别
     * @param val 错误条件值
     * @param cat 错误类别
     */
    void assign(int val, const error_category& cat) noexcept {
        value_ = val;
        category_ = &cat;
    }

    /**
     * @brief 清空错误条件
     *
     * 重置值为0，类别重置为 generic_category()。
     */
    void clear() noexcept {
        value_ = 0;
        category_ = &generic_category();
    }

    /**
     * @brief 获取错误条件值
     * @return 整数值
     */
    NEFORCE_NODISCARD int value() const noexcept { return value_; }

    /**
     * @brief 获取错误类别
     * @return 错误类别引用
     */
    NEFORCE_NODISCARD const error_category& category() const noexcept { return *category_; }

    /**
     * @brief 获取错误描述信息
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD string message() const { return category_->message(value_); }

    /**
     * @brief 布尔转换
     * @return 值非零返回true
     */
    explicit operator bool() const noexcept { return value_ != 0; }

    NEFORCE_NODISCARD bool equal_to(const error_condition& rhs) const noexcept {
        return category_ == rhs.category_ && value_ == rhs.value_;
    }
    NEFORCE_NODISCARD bool less_than(const error_condition& rhs) const noexcept {
        if (*category_ < *rhs.category_) {
            return true;
        }
        if (*rhs.category_ < *category_) {
            return false;
        }
        return value_ < rhs.value_;
    }
};

inline bool operator==(const error_condition& lhs, const errc rhs) noexcept { return lhs == make_error_condition(rhs); }

inline bool operator!=(const error_condition& lhs, const errc rhs) noexcept { return !(lhs == rhs); }

inline bool operator==(const errc lhs, const error_condition& rhs) noexcept { return rhs == lhs; }

inline bool operator!=(const errc lhs, const error_condition& rhs) noexcept { return !(rhs == lhs); }


/**
 * @brief 从 errc 枚举创建错误条件
 * @param e 错误码枚举值
 * @return 关联 generic_category() 的错误条件
 */
inline error_condition make_error_condition(errc e) noexcept { return {static_cast<int>(e), generic_category()}; }

/** @} */ // ErrorCode

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERROR_CONDITION_HPP__
