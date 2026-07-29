#ifndef NEFORCE_TUI_RECONCILER_HPP__
#define NEFORCE_TUI_RECONCILER_HPP__

/**
 * @file reconciler.hpp
 * @brief 差分渲染引擎
 *
 * reconciler 是 TUI 框架的渲染调度核心——
 * 协调渲染器、焦点管理器、事件分发器，
 * 调用组件 render() 获取元素树，计算布局，生成最小 ANSI 更新。
 */

#include "NeForce/core/system/console.hpp"
#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/layout.hpp"
#include "NeForce/tui/event_dispatcher.hpp"
#include "NeForce/tui/focus_manager.hpp"
#include "NeForce/tui/renderer.hpp"
#include "NeForce/tui/screen.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 差分渲染引擎
 *
 * 编排渲染管线：组件树 → 元素树 → 布局 → 屏幕渲染 → ANSI 输出。
 * 最小化 ANSI 写入操作，仅更新变化区域。
 */
class NEFORCE_API reconciler {
public:
    /**
     * @brief 构造函数
     * @param console 控制台渲染后端
     * @param s 串行执行器
     * @param ctx 事件循环上下文
     */
    reconciler(sys_console& console, strand& s, io_context& ctx);

    ~reconciler();

    reconciler(const reconciler&) = delete;
    reconciler& operator=(const reconciler&) = delete;
    reconciler(reconciler&&) = delete;
    reconciler& operator=(reconciler&&) = delete;

    /**
     * @brief 挂载根组件
     * @param root 根组件指针
     *
     * 注入 schedule_render 回调，调用 setup()，标记全量渲染。
     */
    void mount(component_base* root);

    /**
     * @brief 调度组件更新
     * @param comp 需要重渲染的组件
     */
    void schedule_update(component_base* comp);

    /**
     * @brief 强制全量重绘
     */
    void mark_dirty();

    /**
     * @brief 分发键盘事件到焦点组件
     * @param e 键盘事件
     * @return true 表示事件已被处理
     */
    bool dispatch_key(const key_event& e);

    /**
     * @brief 分发鼠标事件
     * @param e 鼠标事件
     * @return true 表示事件已被处理
     */
    bool dispatch_mouse(const mouse_event& e);

    /**
     * @brief 有脏标记则执行渲染管线
     */
    void flush();

    /**
     * @brief 是否有待渲染的脏标记
     * @return 是否有脏标记
     */
    NEFORCE_NODISCARD bool is_dirty() const noexcept { return dirty_; }

    /**
     * @brief 获取组件调度回调
     */
    NEFORCE_NODISCARD function<void(component_base*)> schedule_render_callback();

    /**
     * @brief 应用主题
     * @param t 主题
     */
    void set_theme(const theme& t);

private:
    sys_console& console_;
    strand& strand_;
    io_context& ctx_;
    component_base* root_ = nullptr;

    int term_w_ = 80;
    int term_h_ = 24;
    theme theme_{dark_theme};

    screen current_screen_{80, 24};
    screen prev_screen_{80, 24};

    focus_manager focus_mgr_;
    event_dispatcher event_dispatcher_;
    renderer renderer_;

    element prev_tree_;
    vector<layout_rect> prev_layout_;
    bool dirty_ = true;
    bool mounted_ = false;
    bool update_pending_ = false;
    bool rendering_ = false;

    void refresh_term_size();
    void setup_tree(component_base* comp);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RECONCILER_HPP__
