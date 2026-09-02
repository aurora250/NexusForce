#ifndef NEFORCE_CORE_ASYNC_NOTIFICATION_HPP__
#define NEFORCE_CORE_ASYNC_NOTIFICATION_HPP__

/**
 * @file notification.hpp
 * @brief 一次性通知机制实现
 *
 * 此文件提供了线程间的一次性事件通知实现。
 */

#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/atomic.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup AsyncComponents 异步组件
 * @{
 */

/**
 * @defgroup Notification 线程通知
 * @brief 线程轻量级的一次性事件通知机制
 * @{
 */

/**
 * @class notification
 * @brief 一次性通知类
 *
 * 提供轻量级的一次性事件通知机制。
 * 通知状态不可复位，因此适用于一次性同步场景。
 */
class NEFORCE_API notification {
private:
    mutable mutex mutex_;              ///< 保护条件变量
    mutable condition_variable cv_;    ///< 等待通知
    atomic<bool> notified_yet_{false}; ///< 是否已发出通知

public:
    /**
     * @brief 默认构造函数
     */
    notification() = default;

    /**
     * @brief 带初始通知状态的构造函数
     * @param prenotify 已通知状态
     */
    explicit notification(bool prenotify) :
    notified_yet_(prenotify) {}

    notification(const notification&) = delete;
    notification& operator=(const notification&) = delete;

    ~notification() = default;

    /**
     * @brief 查询是否已通知
     * @return 是否已通知
     */
    NEFORCE_NODISCARD bool notified() const noexcept { return notified_yet_.load(memory_order_acquire); }

    /**
     * @brief 阻塞等待通知
     *
     * 若尚未通知，则阻塞当前线程直到 notify() 被调用。
     * 若在调用前已通知，则立即返回。
     */
    void wait() const;

    /**
     * @brief 限时等待通知
     * @param rest 最大等待时间
     * @return 若在超时前收到通知返回 true，否则返回 false。
     */
    bool wait_for(milliseconds rest) const;

    /**
     * @brief 等待直到指定时间点
     * @param util 绝对时间点
     * @return 若在时间点前收到通知返回 true，否则返回 false。
     */
    bool wait_until(system_clock::time_point util) const;

    /**
     * @brief 发出通知
     * @throws exception 当重复调用通知时抛出
     *
     * 设置通知标志为 true，并唤醒所有正在等待的线程。
     */
    void notify();

    /**
     * @brief 模板化的限时等待
     * @tparam Rep 时间单位的数值类型
     * @tparam Period 时间单位的周期类型
     * @param timeout 最大等待时长
     * @return 若在超时前收到通知返回 true，否则返回 false。
     */
    template <typename Rep, typename Period>
    bool wait(const duration<Rep, Period>& timeout) const {
        return notification::wait_for(_NEFORCE time_cast<milliseconds>(timeout));
    }
};

/** @} */ // Notification

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_NOTIFICATION_HPP__
