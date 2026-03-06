#ifndef NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#define NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
#ifdef NEFORCE_SUPPORT_OPENSSL
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct ssl_exception
 * @extends exception
 * @brief SSL操作异常
 */
struct NEFORCE_API ssl_exception final : network_exception {
    static int last_error() noexcept;
    static string last_error_message();

    explicit ssl_exception(
        const char* info = "SSL Operation Failed.",
        const char* type = static_type,
        const int code = last_error()) noexcept
    : network_exception(info, type, code) {}

    explicit ssl_exception(const int code) noexcept
    : network_exception(last_error_message().data(), static_type, code) {}

    explicit ssl_exception(const exception& e)
    : network_exception(e) {}

    ~ssl_exception() override = default;

    static constexpr auto static_type = "ssl_exception";
};

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_NETWORK_SSL_SSL_EXCEPTION_HPP__
