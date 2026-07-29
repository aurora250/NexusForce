#ifndef NEFORCE_TUI_EVENT_DISPATCHER_HPP__
#define NEFORCE_TUI_EVENT_DISPATCHER_HPP__

/**
 * @file event_dispatcher.hpp
 * @brief 事件分发器
 *
 * event_dispatcher 负责将键盘和鼠标事件路由到正确的组件。
 * 键盘事件分发到焦点组件（含 Tab 导航和 pass_through），
 * 鼠标事件处理滚动条拖拽、滚轮、点击、悬停追踪。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/tui/events.hpp"
#include "NeForce/tui/focus_manager.hpp"
#include "NeForce/tui/renderer.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 事件分发器
 *
 * 管理键盘/鼠标事件的完整分发流程，包括焦点导航、
 * 滚动条交互状态机、元素点击检测和悬停追踪。
 */
class NEFORCE_API event_dispatcher {
public:
    /**
     * @brief 构造函数
     * @param fm 焦点管理器引用
     */
    explicit event_dispatcher(focus_manager& fm);

    /**
     * @brief 分发键盘事件
     * @param e 键盘事件
     * @param root 根组件
     * @return true 表示事件已被处理
     */
    bool dispatch_key(const key_event& e, component_base* root);

    /**
     * @brief 分发鼠标事件
     * @param e 鼠标事件
     * @param root 根组件
     * @param layout 布局结果
     * @param tree 当前元素树
     * @param hits 滚动条命中信息列表
     * @return true 表示事件已被处理
     */
    bool dispatch_mouse(const mouse_event& e, component_base* root, const vector<layout_rect>& layout,
                        const element& tree, const vector<scrollbar_hit>& hits);

    /**
     * @brief 设置脏标记回调
     * @param cb 回调函数
     * @note 由 reconciler 注入
     */
    void set_dirty_callback(function<void()> cb) { mark_dirty_ = move(cb); }

private:
    struct scroll_drag_state {
        state<int>* scroll_state = nullptr;
        int anchor_position = 0;
        int anchor_value = 0;
        bool vertical = false;
    };

    focus_manager& focus_mgr_;
    component_base* last_mouse_target_ = nullptr;
    scroll_drag_state drag_state_storage_;
    scroll_drag_state* drag_state_ = nullptr;
    function<void()> mark_dirty_;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_EVENT_DISPATCHER_HPP__
