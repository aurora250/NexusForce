#ifndef NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#define NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__

/**
 * @file ssl_exception.hpp
 * @brief SSL异常类和错误类别
 *
 * 此文件提供了SSL操作的异常类型和OpenSSL错误码到错误类别的映射。
 */

#include "NeForce/core/exception/error_code.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ErrorCode 错误码
 * @{
 */

/**
 * @class ssl_error_category
 * @brief OpenSSL错误类别
 *
 * 将OpenSSL错误码映射为可读的错误描述信息。
 */
class NEFORCE_API ssl_error_category final : public error_category {
public:
    /**
     * @brief 获取错误类别名称
     */
    NEFORCE_NODISCARD const char* name() const noexcept override { return "ssl"; }

    /**
     * @brief 获取错误码对应的描述信息
     * @param ev OpenSSL错误码
     * @return 错误描述字符串
     *
     * 错误码为0时返回空字符串。
     */
    NEFORCE_NODISCARD string message(int32_t ev) const override;
};

/**
 * @brief 获取SSL错误类别单例
 * @return SSL错误类别引用
 */
NEFORCE_API const error_category& ssl_category() noexcept;

/** @} */ // ErrorCode

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct ssl_exception
 * @brief SSL操作异常
 */
struct NEFORCE_API ssl_exception final : thirdparty_exception {
    /**
     * @brief 获取OpenSSL错误队列中的最后一个错误码（不弹出）
     * @return error_code
     */
    static error_code last_error() noexcept;

    /**
     * @brief 错误信息构造函数
     * @param info 错误描述信息
     *
     * 错误码自动从 OpenSSL 错误队列获取。
     */
    explicit ssl_exception(const string& info = "SSL Operation Failed.") noexcept :
    thirdparty_exception(info.data()),
    code_(last_error()) {}

    /**
     * @brief 错误码构造函数
     * @param code OpenSSL错误码
     */
    explicit ssl_exception(const int code) :
    thirdparty_exception(ssl_category().message(code).data()),
    code_(code, ssl_category()) {}

    /**
     * @brief 构造函数
     * @param info 错误描述信息
     * @param code OpenSSL错误码
     */
    explicit ssl_exception(const string& info, const int code) noexcept :
    thirdparty_exception(info.data()),
    code_(code, ssl_category()) {}

    explicit ssl_exception(const exception& e) :
    thirdparty_exception(e) {}

    ~ssl_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "ssl_exception"; }

    /**
     * @brief 获取 error_code 对象
     * @return error_code 常量引用
     */
    NEFORCE_NODISCARD const error_code& code() const noexcept { return code_; }

private:
    error_code code_; ///< 错误码
};

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
