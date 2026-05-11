#ifndef NEFORCE_CORE_SYSTEM_SYSTEM_EVENT_HPP__
#define NEFORCE_CORE_SYSTEM_SYSTEM_EVENT_HPP__

/**
 * @file system_event.hpp
 * @brief 系统事件同步原语
 *
 * 此文件提供了系统事件，支持手动重置和自动重置两种模式，可用于线程间同步。
 */

#include "NeForce/core/numeric/numeric_traits.hpp"
#ifdef NEFORCE_PLATFORM_LINUX
#    include "NeForce/core/memory/unique_ptr.hpp"
#    include <pthread.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SystemEvent 系统事件
 * @brief 系统事件实现
 * @{
 */

/**
 * @class system_event
 * @brief 系统事件类
 *
 * 提供线程同步的事件对象，使用场景：
 * - 线程间通知
 * - 生产者-消费者模式
 * - 工作队列通知
 */
class NEFORCE_API system_event {
public:
    /**
     * @enum type
     * @brief 事件类型枚举
     */
    enum class type {
        manual_reset, ///< 手动重置，多个等待线程都会被唤醒，需要手动reset
        auto_reset    ///< 自动重置，只唤醒一个等待线程，之后自动重置为无信号
    };

private:
#ifdef NEFORCE_PLATFORM_LINUX
    struct mutex_deleter {
        void operator()(::pthread_mutex_t* m) const { ::pthread_mutex_destroy(m); }
    };

    struct cond_deleter {
        void operator()(::pthread_cond_t* d) const { ::pthread_cond_destroy(d); }
    };
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    void* handle_; ///< 事件句柄
#else
    unique_ptr<::pthread_mutex_t, mutex_deleter> mutex_; ///< 互斥锁
    unique_ptr<::pthread_cond_t, cond_deleter> cond_;    ///< 条件变量
    bool signaled_;                                      ///< 信号状态
#endif
    type type_; ///< 事件类型

public:
    /**
     * @brief 构造函数
     * @param initial_state 初始信号状态
     * @param type 事件类型
     * @throws system_exception 创建失败时抛出
     */
    explicit system_event(bool initial_state = false, type type = type::auto_reset);

    /**
     * @brief 析构函数
     */
    ~system_event();

    system_event(const system_event&) = delete;
    system_event& operator=(const system_event&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    system_event(system_event&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    system_event& operator=(system_event&& other) noexcept;

    /**
     * @brief 设置事件为有信号状态
     *
     * 唤醒等待该事件的线程。
     */
    void set() noexcept;

    /**
     * @brief 重置事件为无信号状态
     */
    void reset() noexcept;

    /**
     * @brief 等待事件变为有信号状态
     * @param timeout_ms 超时时间（毫秒），默认无限等待
     * @return 等待是否成功
     *
     * 阻塞当前线程直到事件被设置或超时。
     * 对于自动重置事件，唤醒后事件自动重置。
     * 对于手动重置事件，唤醒后事件保持有信号状态。
     */
    bool wait(uint32_t timeout_ms = numeric_traits<uint32_t>::max()) noexcept;

    /**
     * @brief 获取事件类型
     * @return 事件类型
     */
    type event_type() const noexcept { return type_; }
};

/** @} */ // SystemEvent

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_SYSTEM_EVENT_HPP__
