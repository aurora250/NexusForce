#ifndef NEFORCE_TUI_FOCUS_MANAGER_HPP__
#define NEFORCE_TUI_FOCUS_MANAGER_HPP__

/**
 * @file focus_manager.hpp
 * @brief 焦点管理器
 *
 * focus_manager 负责焦点链的构建与 Tab/Shift+Tab 导航。
 * 从组件树收集可聚焦组件，维护当前焦点，支持前后导航。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 焦点管理器
 *
 * 维护焦点链，管理当前焦点组件的设置与导航。
 */
class NEFORCE_API focus_manager {
public:
    focus_manager() = default;

    /**
     * @brief 从组件树重建焦点链
     * @param root 根组件
     */
    void rebuild_chain(component_base* root);

    /**
     * @brief 设置焦点组件
     * @param comp 目标组件
     */
    void set_focus(component_base* comp);

    /**
     * @brief 焦点移动到下一个可聚焦组件
     */
    void focus_next();

    /**
     * @brief 焦点移动到上一个可聚焦组件
     */
    void focus_prev();

    /**
     * @brief 获取当前焦点组件
     * @return 焦点组件指针，无可聚焦组件时返回 nullptr
     */
    NEFORCE_NODISCARD component_base* focused() const noexcept { return focused_; }

    /**
     * @brief 获取焦点链
     * @return 可聚焦组件列表
     */
    NEFORCE_NODISCARD const vector<component_base*>& chain() const noexcept { return focus_chain_; }

    /**
     * @brief 标记焦点链需要重建
     */
    void mark_chain_dirty() noexcept { chain_dirty_ = true; }

    /**
     * @brief 焦点链是否需要重建
     * @return true 表示需要重建
     */
    NEFORCE_NODISCARD bool is_chain_dirty() const noexcept { return chain_dirty_; }

private:
    component_base* focused_ = nullptr;
    vector<component_base*> focus_chain_;
    bool chain_dirty_ = true;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_FOCUS_MANAGER_HPP__
