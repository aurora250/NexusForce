#ifndef NEFORCE_CORE_ASYNC_THREAD_EXIT_REGISTER_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_EXIT_REGISTER_HPP__

/**
 * @file thread_exit_register.hpp
 * @brief 线程退出回调支持
 *
 * 此文件提供了线程退出时的回调注册和执行机制，
 * 允许在线程退出时执行操作。
 */

#include "NeForce/core/config/c++config.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup ThreadExit 线程退出回调
 * @brief 线程退出时的回调管理
 * @{
 */

/**
 * @typedef thread_exit_callback_t
 * @brief 线程退出回调类型
 */
using thread_exit_callback_t = void (*)(void*);

/**
 * @struct thread_exit_elt
 * @brief 线程退出回调元素
 *
 * 表示一个线程退出回调，包含链表指针和回调函数。
 * 使用链表结构存储所有注册的回调，按照注册的逆序执行。
 */
struct thread_exit_elt {
    thread_exit_elt* next;           ///< 指向下一个回调元素的指针
    thread_exit_callback_t callback; ///< 回调函数指针
};

/**
 * @brief 注册线程退出回调
 * @param elt 回调元素指针
 * @param callback 回调函数
 *
 * 将一个回调函数注册到当前线程的退出回调列表中。
 * 回调函数将在当前线程退出时被调用，参数为elt指针本身。
 */
void NEFORCE_API thread_exit_register(thread_exit_elt* elt, thread_exit_callback_t callback) noexcept;

/** @} */ // ThreadExit

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_EXIT_REGISTER_HPP__
