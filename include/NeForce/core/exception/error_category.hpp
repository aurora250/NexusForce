#ifndef NEFORCE_CORE_EXCEPTION_ERROR_CATEGORY_HPP__
#define NEFORCE_CORE_EXCEPTION_ERROR_CATEGORY_HPP__

/**
 * @file error_category.hpp
 * @brief 错误类别体系
 *
 * 此文件提供了错误类别的抽象基类以及通用和系统两种标准错误类别实现。
 * 与 error_code / error_condition 配合，构成完整的错误码体系。
 */

#include "NeForce/core/exception/errc.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

class error_code;
class error_condition;

/**
 * @addtogroup ErrorCode 错误码
 * @{
 */

/**
 * @class error_category
 * @brief 错误类别抽象基类
 *
 * 定义错误类别的核心接口。每个错误类别以单例形式存在，
 * 通过地址比较区分，提供错误码到可读消息的映射能力。
 *
 * 内置两种标准类别：
 * - generic_category()：通用错误类别，映射 POSIX errc 枚举
 * - system_category()：系统错误类别，映射操作系统原生错误码
 *
 * @note 实例不可复制、不可移动，始终通过引用或指针使用
 */
class NEFORCE_API error_category : public icomparable<error_category> {
public:
    error_category() noexcept = default;
    virtual ~error_category() noexcept = default;

    error_category(const error_category&) = delete;
    error_category& operator=(const error_category&) = delete;

    /**
     * @brief 获取错误类别名称
     * @return 类别名称
     */
    NEFORCE_NODISCARD virtual const char* name() const noexcept = 0;

    /**
     * @brief 获取错误码对应的可读描述
     * @param ev 错误码值
     * @return 错误描述字符串
     */
    NEFORCE_NODISCARD virtual string message(int32_t ev) const = 0;

    /**
     * @brief 获取错误码对应的默认错误条件
     * @param ev 错误码值
     * @return 默认错误条件
     *
     * 将当前类别的错误码映射到跨类别的通用错误条件。
     */
    NEFORCE_NODISCARD virtual error_condition default_error_condition(int32_t ev) const noexcept;

    /**
     * @brief 检查错误码与错误条件是否等价
     * @param code 错误码值
     * @param condition 错误条件
     * @return 等价返回true
     */
    NEFORCE_NODISCARD virtual bool equivalent(int code, const error_condition& condition) const noexcept;

    /**
     * @brief 检查错误码与错误条件是否等价
     * @param code 错误码
     * @param condition 错误码值
     * @return 等价返回true
     */
    NEFORCE_NODISCARD virtual bool equivalent(const error_code& code, int condition) const noexcept;

    NEFORCE_NODISCARD bool equal_to(const error_category& rhs) const noexcept { return this == &rhs; }
    NEFORCE_NODISCARD bool less_than(const error_category& rhs) const noexcept {
        return less<const error_category*>()(this, &rhs);
    }

    /**
     * @brief 获取通用错误类别单例
     * @return 通用错误类别引用
     */
    static const error_category& generic() noexcept;

    /**
     * @brief 获取系统错误类别单例
     * @return 系统错误类别引用
     */
    static const error_category& system() noexcept;
};


/**
 * @class generic_error_category
 * @brief 通用错误类别
 *
 * 将 POSIX errc 枚举值映射为可读描述。
 */
class NEFORCE_API generic_error_category final : public error_category {
public:
    NEFORCE_NODISCARD const char* name() const noexcept override { return "generic"; }

    NEFORCE_NODISCARD string message(int32_t ev) const override;

    NEFORCE_NODISCARD error_condition default_error_condition(int32_t ev) const noexcept override;
};

/**
 * @brief 获取通用错误类别单例
 * @return 通用错误类别引用
 */
NEFORCE_API const error_category& generic_category() noexcept;

inline error_code make_error_code(errc e) noexcept;
inline error_condition make_error_condition(errc e) noexcept;

inline const error_category& error_category::generic() noexcept { return generic_category(); }


/**
 * @class system_error_category
 * @brief 系统错误类别
 *
 * 将操作系统原生错误码映射为可读描述。
 */
class NEFORCE_API system_error_category final : public error_category {
public:
    NEFORCE_NODISCARD const char* name() const noexcept override { return "system"; }

    NEFORCE_NODISCARD string message(int32_t ev) const override;

    NEFORCE_NODISCARD error_condition default_error_condition(int32_t ev) const noexcept override;
};

/**
 * @brief 获取系统错误类别单例
 * @return 系统错误类别引用
 */
NEFORCE_API const error_category& system_category() noexcept;

inline const error_category& error_category::system() noexcept { return system_category(); }

/** @} */ // ErrorCode

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERROR_CATEGORY_HPP__
