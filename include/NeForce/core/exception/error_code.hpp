#ifndef NEFORCE_CORE_EXCEPTION_ERROR_CODE_HPP__
#define NEFORCE_CORE_EXCEPTION_ERROR_CODE_HPP__

/**
 * @file error_code.hpp
 * @brief 错误码
 *
 * 此文件提供了错误码类，由错误码值和错误类别组合而成。
 * 错误码用于表示特定类别的具体错误，可通过 message() 获取可读描述。
 */

#include "NeForce/core/exception/error_condition.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ErrorCode 错误码
 * @{
 */

/**
 * @class error_code
 * @brief 错误码
 *
 * 错误码由整数值和错误类别指针组成，表示特定类别域内的一个具体错误。
 */
class error_code : public icommon<error_code> {
private:
    int value_{0};                                       ///< 错误码值
    const error_category* category_{&system_category()}; ///< 错误类别指针

public:
    /**
     * @brief 默认构造函数
     *
     * 构造值为0、类别为 system_category() 的空错误码。
     */
    error_code() noexcept = default;

    /**
     * @brief 构造函数
     * @param val 错误码值
     * @param cat 错误类别
     */
    error_code(int val, const error_category& cat) noexcept :
    value_(val),
    category_(&cat) {}

    /**
     * @brief 从 errc 枚举构造
     * @param e 错误码枚举值
     *
     * 等价于 make_error_code(e)，类别为 generic_category()。
     */
    error_code(errc e) noexcept { *this = make_error_code(e); }

    /**
     * @brief 赋值错误码值和类别
     * @param val 错误码值
     * @param cat 错误类别
     */
    void assign(int val, const error_category& cat) noexcept {
        value_ = val;
        category_ = &cat;
    }

    /**
     * @brief 清空错误码
     *
     * 重置值为0，类别重置为 system_category()。
     */
    void clear() noexcept {
        value_ = 0;
        category_ = &system_category();
    }

    /**
     * @brief 获取错误码值
     * @return 整数值
     */
    NEFORCE_NODISCARD int value() const noexcept { return value_; }

    /**
     * @brief 获取对应 errc 枚举值
     * @return errc 枚举值
     */
    NEFORCE_NODISCARD errc error() const noexcept { return static_cast<errc>(value_); }

    /**
     * @brief 获取错误类别
     * @return 错误类别引用
     */
    NEFORCE_NODISCARD const error_category& category() const noexcept { return *category_; }

    /**
     * @brief 获取对应的默认错误条件
     * @return 错误条件
     */
    NEFORCE_NODISCARD error_condition default_error_condition() const noexcept {
        return category_->default_error_condition(value_);
    }

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

    NEFORCE_NODISCARD bool equal_to(const error_code& rhs) const noexcept {
        return category_ == rhs.category_ && value_ == rhs.value_;
    }
    NEFORCE_NODISCARD bool less_than(const error_code& rhs) const noexcept {
        if (*category_ < *rhs.category_) {
            return true;
        }
        if (*rhs.category_ < *category_) {
            return false;
        }
        return value_ < rhs.value_;
    }

    /**
     * @brief 与错误条件进行等价比较
     * @param cond 错误条件
     * @return 等价返回true
     */
    NEFORCE_NODISCARD bool operator==(const error_condition& cond) const noexcept {
        return category_->equivalent(value_, cond) || cond.category().equivalent(*this, cond.value());
    }

    /**
     * @brief 与错误条件进行不等价比较
     * @param cond 错误条件
     * @return 不等价返回true
     */
    NEFORCE_NODISCARD bool operator!=(const error_condition& cond) const noexcept { return !(*this == cond); }

    NEFORCE_NODISCARD size_t to_hash() const noexcept { return hash_combine_all(category_, value_); }
};

inline bool operator==(const error_code& lhs, const errc rhs) noexcept { return lhs == make_error_condition(rhs); }

inline bool operator!=(const error_code& lhs, const errc rhs) noexcept { return !(lhs == rhs); }

inline bool operator==(const errc lhs, const error_code& rhs) noexcept { return rhs == lhs; }

inline bool operator!=(const errc lhs, const error_code& rhs) noexcept { return !(rhs == lhs); }


/**
 * @brief 从 errc 枚举创建错误码
 * @param e 错误码枚举值
 * @return 关联 generic_category() 的错误码
 */
inline error_code make_error_code(errc e) noexcept { return {static_cast<int>(e), generic_category()}; }

/**
 * @brief 获取当前系统错误码
 * @return 关联 system_category() 的当前 errno 值
 */
error_code NEFORCE_API last_error() noexcept;

/** @} */ // ErrorCode

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERROR_CODE_HPP__
