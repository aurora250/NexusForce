#ifndef NEFORCE_TUI_COMPONENT_STATUS_INDICATOR_HPP__
#define NEFORCE_TUI_COMPONENT_STATUS_INDICATOR_HPP__

/**
 * @file status_indicator.hpp
 * @brief 运行状态指示组件
 *
 * 旋转动画 + 文案 + 已耗时，用于状态行。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/element.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @addtogroup Components 用户组件
 * @{
 */

/**
 * @brief 状态指示配置
 */
struct status_indicator_option {
    string label;               ///< 文案
    int charset = 3;            ///< 旋转字符集
    int frame_interval_ms = 90; ///< 每帧间隔
    bool show_elapsed = true;   ///< 是否显示已耗时
    bool active = true;         ///< 是否处于运行状态
    tui::style spinner_style;   ///< 旋转符样式
    tui::style label_style;     ///< 文案样式
    tui::style elapsed_style;   ///< 计时样式
};

/**
 * @brief 状态指示组件
 *
 * 由 `on_animation()` 驱动帧切换与计时。
 */
class NEFORCE_API status_indicator_view final : public component_base {
public:
    status_indicator_view() = default;

    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit status_indicator_view(status_indicator_option opt);

    /**
     * @brief 设置文案
     * @param label 文案
     */
    void set_label(string label);

    /**
     * @brief 获取文案
     * @return 文案
     */
    NEFORCE_NODISCARD const string& label() const noexcept { return opt_.label; }

    /**
     * @brief 设置运行状态
     * @param active 是否运行
     *
     * 停止后不再推进帧与计时，且不再占用一行；调用 `keep_visible()` 可保留文案。
     */
    void set_active(bool active);

    /**
     * @brief 是否运行中
     * @return 是否运行
     */
    NEFORCE_NODISCARD bool active() const noexcept override { return opt_.active; }

    /**
     * @brief 停止运行但保留最后一行输出
     * @param keep 是否保留
     */
    void set_keep_visible(bool keep) noexcept { keep_visible_ = keep; }

    /**
     * @brief 获取已耗时毫秒
     * @return 毫秒
     */
    NEFORCE_NODISCARD int64_t elapsed_ms() const noexcept { return elapsed_ms_; }

    /**
     * @brief 重置计时与帧
     */
    void restart();

    element render() override;

    void on_animation(int64_t delta) override;

private:
    status_indicator_option opt_;
    size_t frame_ = 0;
    int64_t frame_accum_ = 0;
    int64_t elapsed_ms_ = 0;
    bool keep_visible_ = false;
};

/**
 * @brief 创建状态指示组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API status_indicator(status_indicator_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_STATUS_INDICATOR_HPP__
