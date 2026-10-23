#ifndef NEFORCE_TUI_COMPONENT_MOTION_HPP__
#define NEFORCE_TUI_COMPONENT_MOTION_HPP__

/**
 * @file motion.hpp
 * @brief 动画时钟、扫光与平滑揭示
 *
 * 提供三类动效原语：
 * - `frame_clock`：全进程共享的帧时钟，保证同一帧内所有动画看到同一时间；
 * - `shimmer_sweep` / `sweep_text`：移动高亮（扫光）；
 * - `reveal`：把"已到达的文本"平滑揭示成"正在显示的文本"。
 *
 * @note 动效只改颜色与字形，不改变布局；否则每帧都会触发整屏重排。
 */

#include "NeForce/core/utility/color.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 共享帧时钟
 *
 * 由 `application` 每帧推进一次；
 * 组件通过 `now_ms()` 取时间、通过 `frame_index()` 取帧号。
 * 暂停后时间冻结，动画随之停摆。
 */
class NEFORCE_API frame_clock {
public:
    /**
     * @brief 获取全局时钟
     * @return 时钟实例
     */
    static frame_clock& instance();

    /**
     * @brief 当前时间
     * @return 毫秒数（从时钟创建或上次 reset 起算）
     */
    NEFORCE_NODISCARD int64_t now_ms() const noexcept { return now_ms_; }

    /**
     * @brief 当前帧号
     * @return 帧号（每 tick 递增）
     */
    NEFORCE_NODISCARD int64_t frame_index() const noexcept { return frame_index_; }

    /**
     * @brief 推进一帧
     * @param delta_ms 距上一帧的毫秒数
     */
    void tick(int64_t delta_ms) noexcept;

    /**
     * @brief 是否暂停
     * @return 暂停返回 true
     */
    NEFORCE_NODISCARD bool paused() const noexcept { return paused_; }

    /**
     * @brief 设置暂停状态
     * @param paused 是否暂停
     * @note 暂停后 `tick` 不再推进时间，窗口失焦时应当暂停
     */
    void set_paused(bool paused) noexcept { paused_ = paused; }

    /**
     * @brief 重置时间与帧号
     */
    void reset() noexcept;

private:
    frame_clock() = default;

    int64_t now_ms_ = 0;
    int64_t frame_index_ = 0;
    bool paused_ = false;
};

/**
 * @brief 计算一列在扫光中的颜色
 * @param column 列号（0 起始）
 * @param width 文本总宽
 * @param time_ms 当前时间
 * @param base 基础色
 * @param highlight 高亮色
 * @param step_ms 高亮窗口每前进一列的毫秒数
 * @param window 高亮窗口列数
 * @return 该列颜色
 *
 * 窗口按 `width + 2 * window` 循环，窗口内按正弦相位在基础色与高亮色之间插值。
 */
NEFORCE_API _NEFORCE color shimmer_color(int column, int width, int64_t time_ms, _NEFORCE color base,
                                         _NEFORCE color highlight, int step_ms = 60, int window = 10);

/**
 * @brief 计算整行扫光
 * @param width 文本总宽
 * @param time_ms 当前时间
 * @param base 基础色
 * @param highlight 高亮色
 * @param step_ms 每列毫秒数
 * @param window 窗口列数
 * @return 每列颜色（长度为 width）
 */
NEFORCE_API vector<_NEFORCE color> shimmer_sweep(int width, int64_t time_ms, _NEFORCE color base,
                                                 _NEFORCE color highlight, int step_ms = 60, int window = 10);

/**
 * @brief 平滑揭示
 *
 * 维护一个"已显示"游标：
 * 每帧按 `max(3, ceil(积压/8))` 前进，因此在 8 帧内追平积压。
 */
class NEFORCE_API reveal {
public:
    /**
     * @brief 构造函数
     * @param arrived 初始已到达的长度
     */
    explicit reveal(size_t arrived = 0) noexcept;

    /**
     * @brief 重置游标
     * @param arrived 已到达长度
     */
    void reset(size_t arrived = 0) noexcept;

    /**
     * @brief 推进一帧
     * @param arrived 当前已到达长度
     * @return 推进后应显示的长度
     */
    size_t advance(size_t arrived) noexcept;

    /**
     * @brief 当前应显示的长度
     * @return 长度
     */
    NEFORCE_NODISCARD size_t visible() const noexcept { return visible_; }

    /**
     * @brief 是否已追平
     * @param arrived 当前已到达长度
     * @return 追平返回 true
     */
    NEFORCE_NODISCARD bool done(size_t arrived) const noexcept { return visible_ >= arrived; }

private:
    size_t visible_ = 0; ///< 已显示长度
};

/**
 * @brief 旋转动画帧集
 */
struct spinner_frame_set {
    string_view name;          ///< 名称
    const string_view* frames; ///< 帧表
    size_t count;              ///< 帧数
    int interval_ms;           ///< 每帧毫秒数
};

/**
 * @brief 按名称查找旋转动画帧集
 * @param name 名称（`braille`、`dots`、`line`、`clock`）
 * @return 帧集，未知名称返回 nullptr
 */
NEFORCE_API const spinner_frame_set* find_spinner(string_view name) noexcept;

/**
 * @brief 取当前帧
 * @param set 帧集
 * @param time_ms 当前时间
 * @return 当前帧文本
 */
NEFORCE_API string_view spinner_frame(const spinner_frame_set& set, int64_t time_ms) noexcept;

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_MOTION_HPP__
