#ifndef NEFORCE_CORE_ASYNC_NAMED_MUTEX_HPP__
#define NEFORCE_CORE_ASYNC_NAMED_MUTEX_HPP__

/**
 * @file named_mutex.hpp
 * @brief 命名互斥锁
 *
 * 此文件提供了跨进程命名互斥锁
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Mutex 互斥锁
 * @brief 互斥锁类型和工具
 * @{
 */

/**
 * @class named_mutex
 * @brief 命名互斥锁
 *
 * 提供独立的跨进程命名互斥锁，可与共享内存解耦使用。
 */
class NEFORCE_API named_mutex : public mutex_base {
public:
    using native_handle_type = _NEFORCE native_handle_type;

private:
    struct impl;
    unique_ptr<impl> impl_;

public:
    /**
     * @brief 默认构造函数
     */
    named_mutex() noexcept;

    /**
     * @brief 构造函数，创建或打开命名互斥锁
     * @param name 互斥锁名称
     * @param create 是否创建（true: 创建, false: 打开已有）
     * @throws share_memory_exception 创建或打开失败时抛出
     */
    explicit named_mutex(const string& name, bool create = false);

    ~named_mutex();

    named_mutex(const named_mutex&) = delete;
    named_mutex& operator=(const named_mutex&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    named_mutex(named_mutex&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    named_mutex& operator=(named_mutex&& other) noexcept;

    /**
     * @brief 获取互斥锁
     *
     * 阻塞直到锁可用，支持跨进程同步。
     * @throws share_memory_exception 未初始化时抛出
     */
    void lock();

    /**
     * @brief 尝试获取互斥锁
     * @return 是否成功获取锁
     */
    bool try_lock();

    /**
     * @brief 释放互斥锁
     */
    void unlock();

    /**
     * @brief 获取互斥锁名称
     * @return 名称，对象未初始化时返回空串
     */
    NEFORCE_NODISCARD string_view name() const noexcept;

    /**
     * @brief 检查是否有效
     * @return 是否已成功创建或打开
     */
    NEFORCE_NODISCARD bool is_valid() const noexcept;

    /**
     * @brief 删除命名互斥锁
     * @param name 互斥锁名称
     * @return 是否成功删除
     */
    static bool remove(const string& name);
};

/** @} */ // Mutex

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_NAMED_MUTEX_HPP__
