#ifndef NEFORCE_CORE_CONFIG_TERMINATE_HPP__
#define NEFORCE_CORE_CONFIG_TERMINATE_HPP__

/**
 * @file terminate.hpp
 * @brief 程序终止处理
 *
 * 此文件提供了程序终止相关的函数声明，
 * 用于处理未捕获的异常和程序异常终止。
 */

#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
void NEFORCE_API set_terminate(terminate_handler handler) noexcept;

/**
 * @brief 终止处理
 *
 * 调用当前设置的终止处理函数。
 *
 * @note 此函数不会返回，进程将在执行终止处理函数后终止。
 */
NEFORCE_NORETURN void NEFORCE_API terminate();


/**
 * @brief 终止进程
 *
 * 强制终止调用进程，不会进行任何析构行为
 *
 * @note 此函数不会返回，进程将在执行函数后终止。
 */
NEFORCE_NORETURN void NEFORCE_API abort();


/**
 * @typedef exit_handler
 * @brief 退出处理函数指针类型
 *
 * 定义退出处理函数的签名，用于注册在程序正常退出时调用的函数。
 */
using exit_handler = void(*)();

/**
 * @brief 注册退出处理函数
 * @param handler 要注册的函数指针
 * @return 成功返回0，失败返回非0
 *
 * 注册在程序正常退出（通过exit）时调用的函数。
 * 注册的函数按照注册顺序的逆序调用。
 */
int NEFORCE_API set_exit(exit_handler handler) noexcept;

/**
 * @brief 正常终止程序
 * @param status 退出状态码
 *
 * 执行以下操作：
 * 1. 逆序调用所有通过set_exit注册的函数
 * 2. 刷新所有输出流
 * 3. 关闭所有打开的文件
 * 4. 将控制权返回给宿主环境，返回status作为退出码
 *
 * @note 此函数不会返回，进程将在执行函数后终止。
 */
NEFORCE_NORETURN void NEFORCE_API exit(int status);


/**
 * @brief 快速终止程序
 * @param status 退出状态码
 *
 * 不调用set_exit注册的函数，直接终止程序。
 *
 * @note 此函数不会返回，进程将在执行函数后终止。
 */
NEFORCE_NORETURN void NEFORCE_API immediate_exit(int status) noexcept;

/**
 * @brief 注册快速退出处理函数
 * @param handler 要注册的函数指针
 * @return 成功返回0，失败返回非0
 *
 * 注册在程序通过quick_exit退出时调用的函数。
 * 注册的函数按照注册顺序的逆序调用。
 */
int NEFORCE_API set_quick_exit(exit_handler handler) noexcept;

/**
 * @brief 快速退出程序
 * @param status 退出状态码
 *
 * 执行以下操作：
 * 1. 调用所有通过at_quick_exit注册的函数
 * 2. 不调用set_exit注册的函数，直接终止程序
 *
 * @note 此函数不会返回，进程将在执行函数后终止。
 */
NEFORCE_NORETURN void NEFORCE_API quick_exit(int status) noexcept;

/** @} */ // TerminationFunctions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONFIG_TERMINATE_HPP__
