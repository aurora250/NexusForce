#ifndef NEFORCE_CORE_TIME_CLICK_HPP__
#define NEFORCE_CORE_TIME_CLICK_HPP__

/**
 * @file click.hpp
 * @brief 计时器工具
 *
 * 此文件提供了计时器和作用域计时器工具，用于测量代码执行时间。
 */

#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Click 计时器
 * @brief 计时器和时间测量工具
 * @{
 */

/**
 * @struct click
 * @brief 计时器结构
 *
 * 简单的计时器，用于测量代码段的执行时间。
 * 支持开始、更新、停止和重置操作。
 */
struct click {
    using time_point = system_clock::time_point; ///< 时间点类型

    time_point start_time{}; ///< 开始时间点
    time_point last_time{};  ///< 最后更新时间点或停止时间点
    bool started = false;    ///< 是否已开始计时
    bool stopped = false;    ///< 是否已停止计时

    /**
     * @brief 开始计时
     *
     * 记录当前时间为开始时间，将计时器标记为已开始。
     */
    void start() noexcept {
        start_time = system_clock::now();
        started = true;
        stopped = false;
    }

    /**
     * @brief 更新时间
     *
     * 更新最后时间点为当前时间，用于记录中间时间点。
     */
    void update() noexcept {
        last_time = system_clock::now();
    }

    /**
     * @brief 停止计时
     *
     * 记录当前时间为停止时间，将计时器标记为已停止。
     */
    void stop() noexcept {
        last_time = system_clock::now();
        stopped = true;
    }

    /**
     * @brief 获取经过的时间
     * @return 经过的纳秒数
     * @throw value_exception 如果计时器未正确开始/停止
     *
     * 返回从开始到停止经过的时间，会检查计时器状态。
     */
    nanoseconds during() const {
        if (!started || !stopped) {
            NEFORCE_THROW_EXCEPTION(value_exception("click not properly started/stopped"));
        }
        return last_time - start_time;
    }

    /**
     * @brief 获取经过的时间
     * @return 经过的纳秒数，如果计时器未正确开始/停止则返回0
     *
     * 返回从开始到停止经过的时间，不会抛出异常。
     */
    nanoseconds during_s() const noexcept {
        if (!started || !stopped) {
            return nanoseconds{0};
        }
        const auto diff = last_time - start_time;
        return diff.count() >= 0 ? diff.to_nano() : 0_ns;
    }

    /**
     * @brief 重置计时器
     * @throw 无
     *
     * 重置计时器的所有状态和记录的时间点。
     */
    void reset() noexcept {
        started = false;
        stopped = false;
        start_time = time_point{};
        last_time = time_point{};
    }
};


/**
 * @class scoped_click
 * @brief 作用域计时器
 *
 * 在构造时开始计时，在析构时停止计时，
 * 用于在指定作用域内自动测量代码块执行时间。
 */
class scoped_click {
    click& clk_;  ///< 引用的计时器

public:
    /**
     * @brief 构造函数
     * @param clk 要管理的计时器引用
     * @throw 无
     *
     * 构造时自动开始计时。
     */
    explicit scoped_click(click& clk) noexcept : clk_(clk) {
        clk_.start();
    }

    /**
     * @brief 禁止复制构造
     */
    scoped_click(const scoped_click&) = delete;

    /**
     * @brief 禁止复制赋值
     */
    scoped_click& operator =(const scoped_click&) = delete;

    /**
     * @brief 析构函数
     * @throw 无
     *
     * 析构时自动停止计时。
     */
    ~scoped_click() noexcept {
        clk_.stop();
    }
};

/** @} */ // Click

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_TIME_CLICK_HPP__
