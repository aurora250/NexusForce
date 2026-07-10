#ifndef NEFORCE_CORE_CONFIG_EXCEPTION_HPP__
#define NEFORCE_CORE_CONFIG_EXCEPTION_HPP__

/**
 * @file exception.hpp
 * @brief 异常处理框架
 *
 * 此文件提供了异常处理框架，
 * 包括异常基类、各种特定异常类型以及相关的辅助宏和工具函数。
 */

#include "NeForce/core/memory/memory.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ExceptionHandling 异常处理
 * @brief 异常处理类与工具
 * @{
 */

#define __NEFORCE_ERROR_CONSTRUCTOR(THIS, BASE, INFO) \
    explicit THIS(const char* info = INFO) noexcept : \
    BASE(info) {}                                     \
                                                      \
    explicit THIS(const exception& e) :               \
    BASE(e) {}

#define __NEFORCE_ERROR_DERIVED_DESTRUCTOR(CLASS) ~CLASS() override = default;

#define __NEFORCE_ERROR_FINAL_DESTRUCTOR(CLASS) ~CLASS() override = default;

#define __NEFORCE_ERROR_TYPE(CLASS) \
    NEFORCE_NODISCARD const char* type() const noexcept override { return #CLASS; }

/**
 * @def NEFORCE_ERROR_BUILD_DERIVED_CLASS
 * @brief 构建可派生的异常类宏
 * @param THIS 当前类名
 * @param BASE 基类名
 * @param INFO 默认错误信息
 *
 * 快速定义可进一步派生的异常类。
 */
#define NEFORCE_ERROR_BUILD_DERIVED_CLASS(THIS, BASE, INFO) \
    struct THIS : BASE {                                    \
        __NEFORCE_ERROR_CONSTRUCTOR(THIS, BASE, INFO)       \
        __NEFORCE_ERROR_DERIVED_DESTRUCTOR(THIS)            \
        __NEFORCE_ERROR_TYPE(THIS)                          \
    };

/**
 * @def NEFORCE_ERROR_BUILD_FINAL_CLASS
 * @brief 构建最终异常类宏
 * @param THIS 当前类名
 * @param BASE 基类名
 * @param INFO 默认错误信息
 *
 * 快速定义不可派生的异常类。
 */
#define NEFORCE_ERROR_BUILD_FINAL_CLASS(THIS, BASE, INFO) \
    struct THIS final : BASE {                            \
        __NEFORCE_ERROR_CONSTRUCTOR(THIS, BASE, INFO)     \
        __NEFORCE_ERROR_FINAL_DESTRUCTOR(THIS)            \
        __NEFORCE_ERROR_TYPE(THIS)                        \
    };

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct exception
 * @brief 异常基类
 */
struct exception {
private:
    static constexpr size_t INFO_SIZE = 256; // 异常信息长度

    char info_[INFO_SIZE]; // 异常信息

public:
    /**
     * @brief 构造函数
     * @param info 异常信息
     */
    explicit exception(const char* info = "") {
        string_copy(info_, info, INFO_SIZE - 1);
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        info_[INFO_SIZE - 1] = '\0';
    }

    /**
     * @brief 复制构造函数
     */
    exception(const exception& other) noexcept { memory_copy(info_, other.info_, INFO_SIZE); }

    /**
     * @brief 复制赋值运算符
     */
    exception& operator=(const exception& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

        memory_copy(info_, other.info_, INFO_SIZE);

        return *this;
    }

    /**
     * @brief 移动构造函数
     */
    exception(exception&& other) noexcept {
        memory_copy(info_, other.info_, INFO_SIZE);
        other.info_[0] = '\0';
    }

    /**
     * @brief 移动赋值运算符
     */
    exception& operator=(exception&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

        memory_copy(info_, other.info_, INFO_SIZE);

        other.info_[0] = '\0';

        return *this;
    }

    /**
     * @brief 模板构造函数
     * @tparam Error 错误类型
     * @param error 错误对象
     *
     * 从其他异常类型构造。
     */
    template <typename Error, enable_if_t<is_base_of_v<exception, Error>, int> = 0>
    explicit exception(const Error& error) :
    exception(error.what(), error.type(), error.code()) {}

    /**
     * @brief 虚析构函数
     */
    virtual ~exception() = default;

    /**
     * @brief 获取错误信息
     * @return 错误信息字符串
     */
    NEFORCE_NODISCARD virtual const char* what() const noexcept { return info_; }

    /**
     * @brief 获取异常类型
     * @return 异常类型字符串
     */
    NEFORCE_NODISCARD virtual const char* type() const noexcept { return "exception"; }
};

/**
 * @struct memory_exception
 * @brief 内存操作异常
 */
NEFORCE_ERROR_BUILD_DERIVED_CLASS(memory_exception, exception, "Memory Operation Failed.")

/**
 * @struct allocate_exception
 * @brief 内存分配异常
 */
NEFORCE_ERROR_BUILD_DERIVED_CLASS(allocate_exception, memory_exception, "Memory Allocation Failed.")

/**
 * @struct iterator_exception
 * @brief 指针或迭代器行为异常
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(iterator_exception, memory_exception, "Iterator or Pointer Access Invalid.")

/**
 * @struct typecast_exception
 * @brief 类型转换异常
 */
NEFORCE_ERROR_BUILD_DERIVED_CLASS(typecast_exception, memory_exception, "Type Cast Mismatch.")

/**
 * @struct value_exception
 * @brief 变量处理异常
 */
NEFORCE_ERROR_BUILD_DERIVED_CLASS(value_exception, exception, "Variable Operation Invalid.")

/**
 * @struct math_exception
 * @brief 数学计算异常
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(math_exception, value_exception, "Math Calculation Invalid.")

/**
 * @struct thirdparty_exception
 * @brief 第三方操作异常
 */
NEFORCE_ERROR_BUILD_DERIVED_CLASS(thirdparty_exception, exception, "ThirdParty Operation Invalid.")

/**
 * @struct database_exception
 * @brief 数据库行为异常
 */
NEFORCE_ERROR_BUILD_DERIVED_CLASS(database_exception, thirdparty_exception, "Database Operation Failed.")

/** @} */ // Exceptions

/**
 * @brief 未捕获的异常数量
 * @return 当前线程中未捕获的异常数量
 *
 * 该函数返回在当前线程中已经抛出但尚未被捕获的异常数量。
 */
int NEFORCE_API uncaught_exceptions() noexcept;

/**
 * @brief 抛出异常并打印堆栈信息
 * @param err 要抛出的异常对象
 */
NEFORCE_NORETURN void NEFORCE_API throw_with_stack(const exception& err);

#if (defined(NEFORCE_STATE_DEBUG) || !defined(NDEBUG)) && 0 // 0 switch
#    define NEFORCE_THROW_EXCEPTION(err) throw_with_stack(err)
#else
#    define NEFORCE_THROW_EXCEPTION(err) throw err
#endif

#define NEFORCE_ASSERTION(COND) \
    if (!(COND)) {              \
        _NEFORCE terminate();   \
    }

/** @} */ // ExceptionHandling

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONFIG_EXCEPTION_HPP__
