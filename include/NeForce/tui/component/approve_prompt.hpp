#ifndef NEFORCE_TUI_COMPONENT_APPROVE_PROMPT_HPP__
#define NEFORCE_TUI_COMPONENT_APPROVE_PROMPT_HPP__

/**
 * @file approve_prompt.hpp
 * @brief 确认对话框组件
 *
 * 用于权限确认、危险操作二次确认等场景：标题 + 说明 + 选项行。
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
 * @brief 确认结果
 */
enum class approve_result : int8_t {
    pending = -1,        ///< 尚未决定
    rejected = 0,        ///< 取消 / 拒绝
    approved = 1,        ///< 允许
    approved_always = 2, ///< 总是允许
};

/**
 * @brief 确认对话框配置
 */
struct approve_prompt_option {
    string title = "Confirm";                 ///< 标题
    string message;                           ///< 说明文本（自动换行）
    string approve_label = "Allow";           ///< 允许标签
    string reject_label = "Deny";             ///< 拒绝标签
    string always_label;                      ///< "总是允许"标签，为空表示不显示该选项
    int width = 48;                           ///< 对话框宽度
    bool default_approve = true;              ///< 默认选中项是否为允许
    state<approve_result>* result = nullptr;  ///< 结果状态
    function<void(approve_result)> on_decide; ///< 决定回调
    tui::style style;                         ///< 面板样式
};

/**
 * @brief 确认对话框组件
 *
 * 键位：`←`/`→`/`Tab` 切换选项，`1`-`3` 直接选择并确认，`Enter` 确认当前项，`Esc` 取消（拒绝）。
 */
class NEFORCE_API approve_prompt_view final : public component<> {
public:
    /**
     * @brief 构造函数
     * @param opt 配置
     */
    explicit approve_prompt_view(approve_prompt_option opt);

    /**
     * @brief 设置说明文本
     * @param message 文本
     */
    void set_message(string message);

    /**
     * @brief 设置结果并回调
     * @param value 结果值
     */
    void decide(approve_result value);

    /**
     * @brief 当前高亮的选项序号
     * @return 序号（0 允许 / 1 拒绝 / 2 总是允许）
     */
    NEFORCE_NODISCARD int highlighted() const noexcept { return highlight_; }

    /**
     * @brief 当前结果
     * @return 结果值
     */
    NEFORCE_NODISCARD approve_result result() const noexcept;

    element render() override;

    bool on_key(const key_event& e) override;

    bool on_mouse(const mouse_event& e) override;

    NEFORCE_NODISCARD bool focusable() const override { return true; }

private:
    approve_prompt_option opt_;
    int highlight_ = 0;
    approve_result own_result_ = approve_result::pending;

    NEFORCE_NODISCARD bool has_always() const noexcept { return !opt_.always_label.empty(); }
    NEFORCE_NODISCARD int choice_count() const noexcept { return has_always() ? 3 : 2; }
    NEFORCE_NODISCARD element render_choice(int index) const;
};

/**
 * @brief 创建确认对话框组件
 * @param opt 配置
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API approve_prompt(approve_prompt_option opt);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_APPROVE_PROMPT_HPP__
