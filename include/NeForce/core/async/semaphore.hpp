#ifndef NEFORCE_CORE_ASYNC_SEMAPHORE_HPP__
#define NEFORCE_CORE_ASYNC_SEMAPHORE_HPP__

/**
 * @file semaphore.hpp
 * @brief 信号量支持
 *
 * 此文件提供了信号量的实现，用于控制对共享资源的并发访问。
 * 包含两种信号量类型：
 * - counting_semaphore: 轻量级计数信号量，基于原子操作和等待机制
 * - semaphore: 系统级信号量，基于操作系统API
 */

#include "atomic_base.hpp"
#include "atomic_timed_wait.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Semaphores 信号量
 * @brief 信号量同步原语
 * @{
 */

/**
 * @class counting_semaphore
 * @brief 计数信号量类模板
 * @tparam LeastMaxValue 信号量的最小最大值
 *
 * 计数信号量是一个轻量级的同步原语，用于控制对共享资源的访问。
 * 基于原子操作和平台无关的等待机制实现，适用于用户态同步场景。
 *
 * LeastMaxValue指定了信号量计数器的最小最大值，实际值可以小于此值。
 * 这个模板参数主要用于静态检查，确保信号量的合理使用。
 *
 * @note 此信号量不支持递归获取
 */
template <platform_wait_t LeastMaxValue = numeric_traits<platform_wait_t>::max()>
class counting_semaphore {
    static_assert(LeastMaxValue >= 0, "LeastMaxValue should be upper than zero.");

    alignas(alignof(platform_wait_t)) platform_wait_t counter_;  ///< 信号量计数器

    /**
     * @brief 尝试获取信号量的内部实现
     * @return 是否成功获取
     *
     * 使用原子操作尝试减少计数器值。如果计数器为0，则返回false。
     * 使用CAS操作确保线程安全，避免ABA问题。
     */
    NEFORCE_ALWAYS_INLINE bool do_try_acquire() noexcept {
        auto old_value = _NEFORCE atomic_load(&counter_, memory_order_acquire);
        if (old_value == 0) {
            return false;
        }
        return _NEFORCE atomic_cmpexch_strong(
            &counter_, &old_value, old_value - 1,
            memory_order_acquire, memory_order_relaxed);
    }

public:
    /**
     * @brief 构造函数
     * @param desired 信号量的初始计数值
     *
     * 创建计数信号量并设置初始计数值。
     * @note desired不能为负数
     */
    explicit counting_semaphore(const platform_wait_t desired) noexcept
    : counter_(desired) {
        NEFORCE_CONSTEXPR_ASSERT(desired >= 0);
    }

    ~counting_semaphore() = default;  ///< 析构函数

    counting_semaphore(const counting_semaphore&) = delete;
    counting_semaphore& operator =(const counting_semaphore&) = delete;

    /**
     * @brief 获取信号量的最大可能值
     * @return 信号量的最大计数值
     */
    static constexpr platform_wait_t max() noexcept {
        return LeastMaxValue; 
    }

    /**
     * @brief 释放信号量
     * @param update 释放的数量，默认为1
     *
     * 增加信号量的计数值，并通知等待的线程。
     */
    void release(const platform_wait_t update = 1) noexcept {
        if (0 < _NEFORCE atomic_fetch_add(&counter_, update, memory_order_release)) {
            return;
        }
        _NEFORCE atomic_notify_address(&counter_, true);
    }

    /**
     * @brief 获取信号量
     *
     * 阻塞当前线程，直到成功获取信号量。
     */
    void acquire() noexcept {
        auto const pred = [this] {
            return this->do_try_acquire();
        };
        _NEFORCE atomic_wait_address(&counter_, pred);
    }

    /**
     * @brief 尝试获取信号量
     * @return 是否成功获取
     *
     * 非阻塞地尝试获取信号量。
     */
    bool try_acquire() noexcept {
        auto const pred = [this] {
            return this->do_try_acquire();
        };
        return _NEFORCE atomic_spin(pred, [] { return false; });
    }

    /**
     * @brief 在指定时间内尝试获取信号量
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @param relative 相对超时时间
     * @return 是否在超时前成功获取
     *
     * 阻塞当前线程，直到成功获取信号量或超时。
     * 超时时间从调用开始计算。
     */
    template <typename Rep, typename Period>
    bool try_acquire_for(const duration<Rep, Period>& relative) noexcept {
        auto const pred = [this] {
            return this->do_try_acquire();
        };
        return _NEFORCE atomic_wait_address_for(&counter_, pred, relative);
    }

    /**
     * @brief 在指定时间点前尝试获取信号量
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param timeout 绝对超时时间点
     * @return 是否在超时前成功获取
     *
     * 阻塞当前线程，直到成功获取信号量或到达指定时间点。
     */
    template <typename Clock, typename Dur>
    bool try_acquire_until(const time_point<Clock, Dur>& timeout) noexcept {
        auto const pred = [this] {
            return this->do_try_acquire();
        };
        return _NEFORCE atomic_wait_address_until(&counter_, pred, timeout);
    }
};

/**
 * @brief 二元信号量
 *
 * 二元信号量是计数信号量的特化版本，其计数值只能是0或1。
 * 用于实现互斥锁或类似的功能，但比互斥锁更轻量级。
 */
using binary_semaphore = counting_semaphore<1>;


/**
 * @class semaphore
 * @brief 系统信号量类
 *
 * 操作系统原生信号量，适用于跨进程同步或需要系统级支持的场景。
 * 提供比原子信号量更丰富的功能，但性能相对较低。
 */
class NEFORCE_API semaphore {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    static constexpr long max_count = numeric_traits<long>::max(); ///< 最大计数值
    using native_handle_type = ::HANDLE; ///< 原生句柄类型
#else
    static constexpr int max_count = SEM_VALUE_MAX;
    using native_handle_type = ::sem_t;
#endif

private:
#ifdef NEFORCE_PLATFORM_WINDOWS
    ::HANDLE handle_ = nullptr;
#else
    ::sem_t sem_{};
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    bool try_acquire_for_impl(milliseconds timeout) noexcept;
#else
    bool try_acquire_for_impl(nanoseconds timeout) noexcept;
#endif

public:
    /**
     * @brief 构造系统级信号量
     * @param initial 初始计数值，必须 >= 0
     * @param maximum 最大计数值（Windows），必须 >= initial
     * @throws system_exception 当系统API调用失败时抛出
     */
    explicit semaphore(long initial = 0, long maximum = max_count);

    /**
     * @brief 析构函数，释放系统资源
     */
    ~semaphore() noexcept;

    semaphore(const semaphore&) = delete;
    semaphore& operator =(const semaphore&) = delete;
    semaphore(semaphore&&) = delete;
    semaphore& operator =(semaphore&&) = delete;

    /**
     * @brief 阻塞获取信号量
     *
     * 将计数器减1；若计数器为0则阻塞，直到其他线程调用release()。
     * 被信号中断时会自动重试。
     */
    void acquire() noexcept;

    /**
     * @brief 非阻塞尝试获取信号量
     * @return 成功返回true，信号量已为0时返回false
     *
     * 尝试获取信号量，如果计数器大于0则立即减少计数器并返回true；
     * 否则立即返回false。
     */
    bool try_acquire() noexcept;

    /**
     * @brief 在相对超时时间内尝试获取信号量
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @param  relative 相对超时时长
     * @return 成功获取返回true，超时返回false
     *
     * 阻塞当前线程，直到成功获取信号量或超时。
     * 超时时间从调用开始计算。
     */
    template <typename Rep, typename Period>
    bool try_acquire_for(const duration<Rep, Period>& relative) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return semaphore::try_acquire_for_impl(_NEFORCE time_cast<milliseconds>(relative));
#else
        return semaphore::try_acquire_for_impl(_NEFORCE time_cast<nanoseconds>(relative));
#endif
    }

    /**
     * @brief 在绝对时间点前尝试获取信号量
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param  timeout 绝对超时时间点
     * @return 成功获取返回true，超时返回false
     *
     * 阻塞当前线程，直到成功获取信号量或到达指定时间点。
     * 如果timeout <= now，则立即调用try_acquire()。
     */
    template <typename Clock, typename Dur>
    bool try_acquire_until(const time_point<Clock, Dur>& timeout) noexcept {
        auto now = Clock::now();
        if (timeout <= now) {
            return try_acquire();
        }
        return try_acquire_for(timeout - now);
    }

    /**
     * @brief 释放信号量
     * @param update 释放数量，默认为1，必须 > 0
     * @throws system_exception 当update <= 0时抛出
     *
     * 将计数器增加update，并唤醒对应数量的等待线程。
     */
    void release(long update = 1);

    /**
     * @brief 查询当前信号量计数值
     * @return 当前计数值，失败返回-1
     * @warning 返回值仅供参考，不保证时序一致性。
     *
     * 查询后立即可能有其他线程改变信号量值。
     */
    int value() const noexcept;
};

/** @} */ // Semaphores

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_SEMAPHORE_HPP__
