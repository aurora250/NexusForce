#ifndef NEFORCE_CORE_EXCEPTION_SYSTEM_EXCEPTION_HPP__
#define NEFORCE_CORE_EXCEPTION_SYSTEM_EXCEPTION_HPP__
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/exception/error_code.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @struct system_exception
 * @brief 系统访问异常
 */
struct system_exception : exception {
    explicit system_exception(const char* info = "System Access Failed.", const error_code code = last_error()) noexcept
    :
    exception(info),
    code_(code) {}

    explicit system_exception(const error_code code) :
    code_(code) {}

    explicit system_exception(const exception& e) :
    exception(e) {}

    ~system_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "system_exception"; }

    NEFORCE_NODISCARD virtual const error_code& code() const noexcept { return code_; }

protected:
    error_code code_;
};

/**
 * @struct device_exception
 * @brief 设备行为异常
 */
struct device_exception : system_exception {
    explicit device_exception(const char* info = "Device Operation Failed.",
                              const error_code code = last_error()) noexcept :
    system_exception(info, code) {}

    explicit device_exception(const error_code err) :
    system_exception(err) {}

    explicit device_exception(const exception& e) :
    system_exception(e) {}

    ~device_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "device_exception"; }
};

/**
 * @struct file_exception
 * @brief 文件处理异常
 */
struct file_exception final : system_exception {
    explicit file_exception(const char* info = "File Operation Failed.", const error_code code = last_error()) noexcept
    :
    system_exception(info, code) {}

    explicit file_exception(const error_code err) :
    system_exception(err) {}

    explicit file_exception(const exception& e) :
    system_exception(e) {}

    ~file_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "file_exception"; }
};

/**
 * @struct network_exception
 * @brief 网络操作或行为异常
 */
struct network_exception : system_exception {
    explicit network_exception(const char* info = "Network Operation or Action Failed.",
                               const error_code code = last_error()) noexcept :
    system_exception(info, code) {}

    explicit network_exception(const error_code err) :
    system_exception(err) {}

    explicit network_exception(const exception& e) :
    system_exception(e) {}

    ~network_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "network_exception"; }
};


NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_SYSTEM_EXCEPTION_HPP__
