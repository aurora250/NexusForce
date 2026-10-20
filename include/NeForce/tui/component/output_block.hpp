#ifndef NEFORCE_TUI_COMPONENT_OUTPUT_BLOCK_HPP__
#define NEFORCE_TUI_COMPONENT_OUTPUT_BLOCK_HPP__

/**
 * @file output_block.hpp
 * @brief 可折叠输出块
 *
 * 头部一行 + 可折叠正文，用于工具输出、长代码块与日志片段。
 */

#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/element.hpp"
#include "NeForce/tui/dom/layout_types.hpp"
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
 * @brief 折叠输出块配置
 */
struct output_block_option {
    element header = element::empty();                                                 ///< 头部元素
    element body = element::empty();                                                   ///< 正文元素
    bool collapsed = false;                                                            ///< 初始是否折叠
    size_t hidden_lines = 0;                                                           ///< 折叠时隐藏的行数
    string collapsed_format = "\xe2\x80\xa6 %d lines hidden \xc2\xb7 enter to expand"; ///< 折叠提示，`%d` 指代行数
    tui::style style;                                                                  ///< 背景带样式
    tui::style hint_style;                                                             ///< 折叠提示样式
};

/**
 * @brief 可折叠输出块
 *
 * 键位：`Enter` / `Space` / `Right` 展开，`Left` 折叠；点击头部切换。
 */
class NEFORCE_API output_block final : public component<> {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit output_block(output_block_option opt);

    /**
     * @brief 设置头部
     * @param header 头部元素
     */
    void set_header(element header);

    /**
     * @brief 设置正文
     * @param body 正文元素
     */
    void set_body(element body);

    /**
     * @brief 设置折叠状态
     * @param collapsed 是否折叠
     */
    void set_collapsed(bool collapsed);

    /**
     * @brief 是否处于折叠状态
     * @return 是否折叠
     */
    NEFORCE_NODISCARD bool collapsed() const noexcept { return opt_.collapsed; }

    /**
     * @brief 切换折叠状态
     */
    void toggle();

    /**
     * @brief 设置隐藏行数（用于折叠提示）
     * @param lines 行数
     */
    void set_hidden_lines(size_t lines);

    void setup() override;

    element render() override;

    bool on_key(const key_event& e) override;

    bool on_mouse(const mouse_event& e) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    output_block_option opt_;
    state<layout_rect>* rect_ = nullptr;
    string collapsed_text_;

    NEFORCE_NODISCARD element render_hint() const;
};

/**
 * @brief 创建可折叠输出块
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API output_block_component(output_block_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_OUTPUT_BLOCK_HPP__
