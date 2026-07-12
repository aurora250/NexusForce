#ifndef NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#define NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct ssl_exception
 * @brief SSL操作异常
 */
struct NEFORCE_API ssl_exception final : thirdparty_exception {
    static int last_error() noexcept;
    static string last_error_message();

    explicit ssl_exception(const char* info = "SSL Operation Failed.", const int code = last_error()) noexcept :
    thirdparty_exception(info),
    code_(code) {}

    explicit ssl_exception(const int code) :
    thirdparty_exception(last_error_message().data()),
    code_(code) {}

    explicit ssl_exception(const exception& e) :
    thirdparty_exception(e) {}

    ~ssl_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "ssl_exception"; }

    NEFORCE_NODISCARD int code() const noexcept { return code_; }

private:
    int code_{0};
};

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
