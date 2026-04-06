#ifndef NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#define NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct ssl_exception
 * @brief SSL操作异常
 */
struct NEFORCE_API ssl_exception final : thirdparty_exception {
    static int last_error() noexcept;
    static string last_error_message();

    explicit ssl_exception(const char* info = "SSL Operation Failed.", const char* type = static_type,
                           const int code = last_error()) noexcept :
    thirdparty_exception(info, type, code) {}

    explicit ssl_exception(const int code) :
    thirdparty_exception(last_error_message().data(), static_type, code) {}

    explicit ssl_exception(const exception& e) :
    thirdparty_exception(e) {}

    ~ssl_exception() override = default;

    static constexpr auto static_type = "ssl_exception";
};

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
