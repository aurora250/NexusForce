#ifndef NEFORCE_TUI_COMPONENT_TOOL_CARD_HPP__
#define NEFORCE_TUI_COMPONENT_TOOL_CARD_HPP__

/**
 * @file tool_card.hpp
 * @brief 工具调用卡片
 *
 * 展示一次工具调用的名称、参数摘要、状态、耗时与可折叠输出。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/component/output_block.hpp"
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
 * @brief 工具调用状态
 */
enum class tool_status : uint8_t {
    running,    ///< 执行中
    success,    ///< 成功
    failed,     ///< 失败
    interrupted ///< 被中断
};

/**
 * @brief 工具卡片配置
 */
struct tool_card_option {
    string title;                              ///< 工具名，如 `bash`
    string summary;                            ///< 参数摘要
    tool_status status = tool_status::running; ///< 初始状态
    int64_t elapsed_ms = 0;                    ///< 已耗时
    bool collapsed = true;                     ///< 输出默认是否折叠
    size_t hidden_lines = 0;                   ///< 折叠时隐藏的行数
    tui::style style;                          ///< 背景带样式
    tui::style title_style;                    ///< 工具名样式
    tui::style summary_style;                  ///< 摘要样式
};

/**
 * @brief 工具调用卡片
 *
 * 状态图标：执行中为旋转动画，成功 `✔`、失败 `✖`、中断 `⏹`。
 */
class NEFORCE_API tool_card final : public component_base {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit tool_card(tool_card_option opt);

    /**
     * @brief 设置状态
     * @param status 新状态
     * @param elapsed_ms 已耗时；负值表示保持当前值
     */
    void set_status(tool_status status, int64_t elapsed_ms = -1);

    /**
     * @brief 当前状态
     * @return 状态
     */
    NEFORCE_NODISCARD tool_status status() const noexcept { return opt_.status; }

    /**
     * @brief 设置参数摘要
     * @param summary 摘要
     */
    void set_summary(string summary);

    /**
     * @brief 设置输出（纯文本，按行渲染）
     * @param text 输出文本
     */
    void set_output(string text);

    /**
     * @brief 设置输出为任意元素
     * @param content 输出元素
     * @param hidden_lines 折叠时隐藏的行数
     */
    void set_output_element(element content, size_t hidden_lines);

    /**
     * @brief 设置输出折叠状态
     * @param collapsed 是否折叠
     */
    void set_collapsed(bool collapsed);

    /**
     * @brief 是否处于运行中
     * @return 是否运行中
     */
    NEFORCE_NODISCARD bool running() const noexcept { return opt_.status == tool_status::running; }

    /**
     * @brief 获取输出区组件
     * @return 输出区引用
     */
    NEFORCE_NODISCARD output_block& output() noexcept { return *output_; }

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    void on_animation(int64_t delta) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    tool_card_option opt_;
    output_block* output_ = nullptr;
    size_t frame_ = 0;
    int64_t frame_accum_ = 0;
    string header_key_;

    NEFORCE_NODISCARD string status_icon();
    NEFORCE_NODISCARD tui::style status_style() const;
};

/**
 * @brief 创建工具调用卡片
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API tool_card_component(tool_card_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_TOOL_CARD_HPP__
