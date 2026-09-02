#ifndef NEFORCE_NETWORK_UTIL_NETWORK_EXCEPTION_HPP__
#define NEFORCE_NETWORK_UTIL_NETWORK_EXCEPTION_HPP__
#include "NeForce/core/exception/system_exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ErrorCode 错误码
 * @{
 */

/**
 * @enum network_errc
 * @brief 网络通用错误码
 */
enum class network_errc {
    timeout = 1,        ///< 查询超时
    network_error = 2,  ///< 网络错误
    parse_error = 3,    ///< 解析错误
    server_failure = 4, ///< 服务器失败
    truncated = 5,      ///< 响应被截断
    no_record = 6       ///< 无记录
};

/**
 * @class network_error_category
 * @brief 网络错误类别
 *
 * 将网络错误码映射为可读的错误描述信息。
 */
class NEFORCE_API network_error_category final : public error_category {
public:
    /**
     * @brief 获取错误类别名称
     */
    NEFORCE_NODISCARD const char* name() const noexcept override { return "network"; }

    /**
     * @brief 获取错误码对应的描述信息
     * @param ev 网络错误码
     * @return 错误描述字符串
     *
     * 错误码为0时返回空字符串。
     */
    NEFORCE_NODISCARD string message(int32_t ev) const override;
};

/**
 * @brief 获取网络错误类别单例
 * @return 网络错误类别引用
 */
NEFORCE_API const error_category& network_category() noexcept;

/** @} */ // ErrorCode

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct network_exception
 * @brief 网络操作或行为异常
 */
struct network_exception : system_exception {
    /**
     * @brief 获取当前线程最后一次网络调用的系统错误码
     * @return 系统错误码
     */
    static error_code NEFORCE_API last_error() noexcept;

    explicit network_exception(const char* info = "Network Operation or Action Failed.",
                               const error_code code = network_exception::last_error()) noexcept :
    system_exception(info, code) {}

    explicit network_exception(const string& info, const error_code code = network_exception::last_error()) noexcept :
    system_exception(info, code) {}

    explicit network_exception(const error_code err) :
    system_exception(err) {}

    explicit network_exception(const exception& e) :
    system_exception(e) {}

    ~network_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "network_exception"; }
};

/**
 * @def NEFORCE_ERROR_BUILD_NETWORK_CLASS
 * @brief 构建网络异常类宏
 * @param THIS 当前类名
 * @param INFO 默认错误信息
 */
#define NEFORCE_ERROR_BUILD_NETWORK_CLASS(THIS, INFO)                                                              \
    struct THIS final : network_exception {                                                                        \
        explicit THIS(const char* info = INFO, const error_code code = network_exception::last_error()) noexcept : \
        network_exception(info, code) {}                                                                           \
        explicit THIS(const string& info, const error_code code = network_exception::last_error()) noexcept :      \
        network_exception(info, code) {}                                                                           \
        explicit THIS(const error_code err) :                                                                      \
        network_exception(err) {}                                                                                  \
        explicit THIS(const exception& e) :                                                                        \
        network_exception(e) {}                                                                                    \
        ~THIS() override = default;                                                                                \
        NEFORCE_NODISCARD const char* type() const noexcept override { return #THIS; }                             \
    };

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_UTIL_NETWORK_EXCEPTION_HPP__
