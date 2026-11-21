#ifndef NEFORCE_CORE_EXCEPTION_SYSTEM_EXCEPTION_HPP__
#define NEFORCE_CORE_EXCEPTION_SYSTEM_EXCEPTION_HPP__
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/exception/error_code.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct system_exception
 * @brief 系统访问异常
 */
struct system_exception : exception {
    explicit system_exception(const char* info = "System Access Failed.", const error_code code = error_code()) noexcept
    :
    exception(info),
    code_(code) {}

    explicit system_exception(const string& info, const error_code code = error_code()) noexcept :
    system_exception(info.data(), code) {}

    explicit system_exception(const error_code code) :
    system_exception(code.message(), code) {}

    explicit system_exception(const exception& e) :
    exception(e) {}

    ~system_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "system_exception"; }
    NEFORCE_NODISCARD exception* clone() const override { return new system_exception(*this); }
    NEFORCE_NORETURN void rethrow() const override { throw *this; }

    NEFORCE_NODISCARD virtual const error_code& code() const noexcept { return code_; }

    static error_code last_error() noexcept { return _NEFORCE last_error(); }

protected:
    error_code code_;
};


#define NEFORCE_ERROR_BUILD_SYSTEM_CLASS(THIS, INFO)                                            \
    struct THIS final : system_exception {                                                      \
        explicit THIS(const char* info = INFO, const error_code code = last_error()) noexcept : \
        system_exception(info, code) {}                                                         \
        explicit THIS(const string& info, const error_code code = last_error()) noexcept :      \
        system_exception(info, code) {}                                                         \
        explicit THIS(const error_code err) :                                                   \
        system_exception(err) {}                                                                \
        explicit THIS(const exception& e) :                                                     \
        system_exception(e) {}                                                                  \
        ~THIS() override = default;                                                             \
        NEFORCE_NODISCARD const char* type() const noexcept override { return #THIS; }          \
        NEFORCE_NODISCARD exception* clone() const override { return new THIS(*this); }         \
        NEFORCE_NORETURN void rethrow() const override { throw *this; }                         \
    };


/**
 * @struct device_exception
 * @brief 设备行为异常
 */
NEFORCE_ERROR_BUILD_SYSTEM_CLASS(device_exception, "Device Operation Failed.")

/**
 * @struct file_exception
 * @brief 文件处理异常
 */
NEFORCE_ERROR_BUILD_SYSTEM_CLASS(file_exception, "File Operation Failed.")

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_SYSTEM_EXCEPTION_HPP__
