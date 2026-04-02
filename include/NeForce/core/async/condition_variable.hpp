#ifndef NEFORCE_CORE_ASYNC_CONDITION_VARIABLE_HPP__
#define NEFORCE_CORE_ASYNC_CONDITION_VARIABLE_HPP__

/**
 * @file condition_variable.hpp
 * @brief 条件变量行为
 *
 * 此文件提供了条件变量的实现，用于线程间的同步和通信。
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup ConditionVariables 条件变量
 * @brief 条件变量和同步原语
 * @{
 */

/**
 * @enum cv_status
 * @brief 条件变量等待状态
 *
 * 表示条件变量等待操作的结果状态。
 */
enum class cv_status {
    success, ///< 等待成功
    timeout  ///< 等待超时
};


/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @class condition_variable_base
 * @brief 条件变量基类
 *
 * 提供平台特定的条件变量实现。
 */
class NEFORCE_API condition_variable_base {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using native_handle_type = ::CONDITION_VARIABLE;
#else
    using native_handle_type = ::pthread_cond_t;
#endif

private:
    native_handle_type cond_; ///< 条件变量句柄

public:
    condition_variable_base();

    condition_variable_base(const condition_variable_base&) = delete;
    condition_variable_base& operator=(const condition_variable_base&) = delete;

    condition_variable_base(condition_variable_base&&) = default;
    condition_variable_base& operator=(condition_variable_base&&) = default;

    ~condition_variable_base();

    native_handle_type* native_handle() noexcept { return &cond_; }

    /**
     * @brief 无限期等待条件变量
     * @param mtx 已锁定的互斥锁
     *
     * 原子地解锁互斥锁并等待条件变量，被唤醒后重新锁定互斥锁。
     *
     * @note 如果等待失败则终止进程
     */
    void wait(mutex& mtx);

    /**
     * @brief 等待条件变量直到指定时间（默认时钟）
     * @param mtx 已锁定的互斥锁
     * @param sec 超时的秒数
     * @param ns 超时的纳秒数
     * @return 等待结果状态
     *
     * 原子地解锁互斥锁并等待条件变量，直到超时或被唤醒。
     */
    cv_status wait_until(mutex& mtx, int64_t sec, int64_t ns);

    /**
     * @brief 等待条件变量直到指定时间
     * @param mtx 已锁定的互斥锁
     * @param is_monotonic 是否使用静态时钟
     * @param sec 超时的秒数
     * @param ns 超时的纳秒数
     * @return 等待结果状态
     *
     * 使用指定的时钟进行超时等待。
     */
    cv_status wait_until(mutex& mtx, bool is_monotonic, int64_t sec, int64_t ns);


    /**
     * @brief 通知一个等待线程
     *
     * 唤醒一个正在等待此条件变量的线程。
     */
    void notify_one() noexcept;

    /**
     * @brief 通知所有等待线程
     *
     * 唤醒所有正在等待此条件变量的线程。
     */
    void notify_all() noexcept;
};

NEFORCE_END_INNER__
/// @endcond

/**
 * @class condition_variable
 * @brief 条件变量类
 *
 * 条件变量允许线程等待某个条件成立，或通知其他线程条件已满足。
 */
class condition_variable {
public:
    using base_type = inner::condition_variable_base;         ///< 基类类型
    using native_handle_type = base_type::native_handle_type; ///< 原生句柄类型
    using clock_type = _NEFORCE steady_clock;                 ///< 默认时钟类型

private:
    base_type cond_; ///< 底层条件变量

    /**
     * @brief 等待直到稳定时钟时间点的实现
     * @tparam Dur 持续时间类型
     * @param lock 智能锁
     * @param util 目标时间点
     * @return 等待状态
     */
    template <typename Dur>
    cv_status __wait_until_impl(unique_lock<mutex>& lock, const time_point<steady_clock, Dur>& util) {
        auto s = util.to_sec();
        const nanoseconds ns(util - s);
        cond_.wait_until(*lock.mutex(), true, s.since_epoch().count(), ns.count());
        return steady_clock::now() < util ? cv_status::success : cv_status::timeout;
    }

    /**
     * @brief 等待直到系统时钟时间点的实现
     * @tparam Dur 持续时间类型
     * @param lock 智能锁
     * @param util 目标时间点
     * @return 等待状态
     */
    template <typename Dur>
    cv_status __wait_until_impl(unique_lock<mutex>& lock, const time_point<system_clock, Dur>& util) {
        auto sec = util.to_sec();
        const nanoseconds nanosec(util - sec);
        cond_.wait_until(*lock.mutex(), sec.since_epoch().count(), nanosec.count());
        return system_clock::now() < util ? cv_status::success : cv_status::timeout;
    }

public:
    /**
     * @brief 构造函数
     */
    condition_variable() = default;

    /**
     * @brief 析构函数
     */
    ~condition_variable() = default;

    condition_variable(const condition_variable&) = delete;
    condition_variable& operator=(const condition_variable&) = delete;

    condition_variable(condition_variable&& other) = default;
    condition_variable& operator=(condition_variable&& other) = default;

    /**
     * @brief 获取原生句柄
     * @return 指向条件变量原生句柄的指针
     */
    native_handle_type* native_handle() noexcept { return cond_.native_handle(); }

    /**
     * @brief 通知一个等待线程
     */
    void notify_one() noexcept { cond_.notify_one(); }

    /**
     * @brief 通知所有等待线程
     */
    void notify_all() noexcept { cond_.notify_all(); }

    /**
     * @brief 无限期等待
     * @param lock 已锁定的互斥锁
     *
     * 原子地解锁互斥锁并等待条件变量，被唤醒后重新锁定互斥锁。
     */
    void wait(unique_lock<mutex>& lock) { cond_.wait(*lock.mutex()); }

    /**
     * @brief 带谓词的无限期等待
     * @tparam Pred 谓词类型
     * @param lock 已锁定的互斥锁
     * @param pred 等待条件谓词，当返回true时停止等待
     *
     * 等待直到谓词返回true。防止虚假唤醒。
     */
    template <typename Pred> void wait(unique_lock<mutex>& lock, Pred pred) {
        while (!pred()) {
            wait(lock);
        }
    }

    /**
     * @brief 等待直到稳定时钟时间点
     * @tparam Dur 持续时间类型
     * @param lock 已锁定的智能锁
     * @param util 目标时间点
     * @return 等待结果状态
     */
    template <typename Dur> cv_status wait_until(unique_lock<mutex>& lock, const time_point<steady_clock, Dur>& util) {
        return this->__wait_until_impl(lock, util);
    }

    /**
     * @brief 等待直到系统时钟时间点
     * @tparam Dur 持续时间类型
     * @param lock 已锁定的智能锁
     * @param util 目标时间点
     * @return 等待结果状态
     */
    template <typename Dur> cv_status wait_until(unique_lock<mutex>& lock, const time_point<system_clock, Dur>& util) {
        return this->__wait_until_impl(lock, util);
    }

    /**
     * @brief 等待直到任意时钟时间点
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @param lock 已锁定的智能锁
     * @param util 目标时间点
     * @return 等待结果状态
     *
     * 支持任意时钟类型的等待，通过转换到稳定时钟实现。
     */
    template <typename Clock, typename Dur>
    cv_status wait_until(unique_lock<mutex>& lock, const time_point<Clock, Dur>& util) {
        const typename Clock::time_point entry = Clock::now();
        const auto atime = clock_type::now() + ceil<clock_type::duration>(util - entry);

        if (this->__wait_until_impl(lock, atime) == cv_status::success) {
            return cv_status::success;
        }
        if (Clock::now() < util) {
            return cv_status::success;
        }
        return cv_status::timeout;
    }

    /**
     * @brief 带谓词的等待直到时间点
     * @tparam Clock 时钟类型
     * @tparam Dur 持续时间类型
     * @tparam Pred 谓词类型
     * @param lock 已锁定的智能锁
     * @param util 目标时间点
     * @param pred 等待条件谓词
     * @return 谓词是否成立
     *
     * 等待直到谓词成立或超时，防止虚假唤醒。
     */
    template <typename Clock, typename Dur, typename Pred>
    bool wait_until(unique_lock<mutex>& lock, const time_point<Clock, Dur>& util, Pred pred) {
        while (!pred()) {
            if (this->wait_until(lock, util) == cv_status::timeout) {
                return pred();
            }
        }
        return true;
    }

    /**
     * @brief 等待指定的持续时间
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @param lock 已锁定的智能锁
     * @param rest 要等待的持续时间
     * @return 等待结果状态
     */
    template <typename Rep, typename Period>
    cv_status wait_for(unique_lock<mutex>& lock, const duration<Rep, Period>& rest) {
        const auto atime = steady_clock::now() + ceil<steady_clock::duration>(rest);
        return this->wait_until(lock, atime);
    }

    /**
     * @brief 带谓词的等待指定持续时间
     * @tparam Rep 时间表示类型
     * @tparam Period 时间单位比例
     * @tparam Pred 谓词类型
     * @param lock 已锁定的智能锁
     * @param rest 要等待的持续时间
     * @param pred 等待条件谓词
     * @return 谓词是否成立
     */
    template <typename Rep, typename Period, typename Pred>
    bool wait_for(unique_lock<mutex>& lock, const duration<Rep, Period>& rest, Pred pred) {
        const auto atime = steady_clock::now() + ceil<steady_clock::duration>(rest);
        return this->wait_until(lock, atime, _NEFORCE move(pred));
    }
};

/** @} */ // ConditionVariables

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_CONDITION_VARIABLE_HPP__
