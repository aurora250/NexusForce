#ifndef NEFORCE_CORE_ASYNC_THREAD_TRACKER_HPP__
#define NEFORCE_CORE_ASYNC_THREAD_TRACKER_HPP__

/**
 * @file thread_tracker.hpp
 * @brief 线程跟踪器
 *
 * 此文件提供了线程跟踪功能，主要用于检测当前是否处于单线程环境。
 */

#include "NeForce/core/async/atomic.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Thread 线程
 * @brief 线程管理和相关操作
 * @{
 */

/**
 * @class thread_tracker
 * @brief 线程跟踪器类
 *
 * 单例类，自动记录主线程的创建和销毁，并提供线程计数查询功能。
 *
 * @note 仅跟踪通过NexusForce线程库创建的线程，不跟踪OS原生线程
 */
class thread_tracker {
private:
    static atomic<int> count_; ///< 当前活动的线程计数

    friend class thread;

    static NEFORCE_ALWAYS_INLINE void on_thread_create() noexcept { ++count_; }

    static NEFORCE_ALWAYS_INLINE void on_thread_destroy() noexcept { --count_; }

    thread_tracker() {
        on_thread_create(); // main thread
    }

    ~thread_tracker() {
        on_thread_destroy(); // main thread
    }

public:
    /**
     * @brief 获取线程跟踪器单例实例
     * @return 线程跟踪器实例的引用
     *
     * 首次调用时创建跟踪器实例并记录主线程。
     */
    static thread_tracker& instance() noexcept {
        static thread_tracker tracker;
        return tracker;
    }

    /**
     * @brief 检查是否处于单线程模式
     * @return 如果只有主线程运行则返回true，否则返回false
     *
     * 当线程计数为1时表示当前只有主线程在运行。
     */
    static NEFORCE_ALWAYS_INLINE bool is_single_threaded() noexcept { return instance().count_.load() == 1; }

    /**
     * @brief 获取当前活动线程数量
     * @return 活动的NexusForce线程数量
     *
     * 返回由NexusForce线程库创建且尚未销毁的线程数量。
     */
    static NEFORCE_ALWAYS_INLINE int thread_count() noexcept { return instance().count_.load(); }
};


/**
 * @brief 检查当前是否处于单线程模式
 * @return 如果只有主线程运行则返回true，否则返回false
 */
NEFORCE_ALWAYS_INLINE_INLINE bool is_single_threaded() noexcept { return thread_tracker::is_single_threaded(); }

/**
 * @brief 获取当前活动线程数量
 * @return 活动的NexusForce线程数量
 */
NEFORCE_ALWAYS_INLINE_INLINE int thread_count() noexcept { return thread_tracker::thread_count(); }

/** @} */ // Thread

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_THREAD_TRACKER_HPP__
