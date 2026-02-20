#ifndef MSTL_CORE_CONFIG_TERMINATE_HPP__
#define MSTL_CORE_CONFIG_TERMINATE_HPP__

/**
 * @file terminate.hpp
 * @brief MSTL程序终止处理
 *
 * 此文件提供了程序终止相关的函数声明，
 * 用于处理未捕获的异常和程序异常终止。
 */

#include "MSTL/core/config/c++config.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup TerminationFunctions 终止处理
 * @brief 异常终止处理的接口
 * @{
 */

/**
 * @typedef terminate_handler
 * @brief 终止处理函数指针类型
 *
 * 定义终止处理函数的签名，用于定义程序终止触发的事件。
 */
using terminate_handler = void(*)();

/**
 * @brief 设置终止处理函数
 * @param handler 新的终止处理函数指针
 *
 * 设置当前进程的终止处理函数，当程序因未捕获的异常而终止时，将调用设置的处理函数。
 */
void MSTL_API set_terminate(terminate_handler handler) noexcept;

/**
 * @brief 终止处理
 *
 * 调用当前设置的终止处理函数。
 *
 * @note 此函数不会返回，进程将在执行终止处理函数后终止。
 */
MSTL_NORETURN void MSTL_API terminate();

/**
 * @brief 终止进程
 *
 * 强制终止调用进程，不会进行任何析构行为
 *
 * @note 此函数不会返回，进程将在执行函数后终止。
 */
MSTL_NORETURN void MSTL_API abort();

/** @} */ // TerminationFunctions

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONFIG_TERMINATE_HPP__
