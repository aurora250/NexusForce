#ifndef NEFORCE_TUI_RECONCILER_HPP__
#define NEFORCE_TUI_RECONCILER_HPP__

/**
 * @file reconciler.hpp
 * @brief 差分渲染引擎
 *
 * reconciler 是 TUI 框架的渲染调度核心——
 * 调用组件 render() 获取元素树，与旧树 diff，生成最小 ANSI 更新。
 */

#include "NeForce/core/system/console.hpp"
#include "NeForce/tui/component/component.hpp"
#include "NeForce/tui/dom/layout.hpp"
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
     * @brief 有脏标记则执行 diff + 渲染
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
     * @param theme 主题
     */
    void set_theme(const theme& theme) { theme_ = theme; }

private:
    sys_console& console_;
    strand& strand_;
    io_context& ctx_;
    component_base* root_ = nullptr;
    component_base* focused_ = nullptr;
    component_base* last_mouse_target_ = nullptr;
    vector<component_base*> focus_chain_;

    element prev_tree_;
    vector<layout_rect> prev_layout_;
    int term_w_ = 80;
    int term_h_ = 24;
    bool dirty_ = true;
    bool mounted_ = false;
    bool update_pending_ = false;
    bool rendering_ = false;
    theme theme_{dark_theme};

    struct clip_rect {
        int x, y, w, h;
    };
    vector<clip_rect> clip_stack_;

    struct scrollbar_hit {
        void* scroll_state;
        int track_x;
        int track_y;
        int track_h;
        int thumb_pos;
        int thumb_size;
        int content_h;
        int sv_x;
        int sv_w;
        bool vertical;
    };
    vector<scrollbar_hit> scrollbar_hits_;
    void* dragging_scroll_state_ = nullptr;
    int drag_anchor_y_ = 0;
    int drag_anchor_scroll_ = 0;

    screen current_screen_{80, 24};
    screen prev_screen_{80, 24};

    void refresh_term_size();
    void setup_tree(component_base* comp);
    void build_focus_chain();

    void focus_next();
    void focus_prev();
    void set_focus(component_base* comp);

    void render_subtree(const element& el, const vector<layout_rect>& layout, int& idx);

    void render_text_block(int x, int y, int w, int h, const string& text, const tui::style& style, style::wrap_mode wm,
                           int* out_end_x = nullptr, int* out_end_y = nullptr);

    void apply_clear(int x, int y, int w, int h);
    void apply_text(int x, int y, const string& text, const style& style);
    void apply_text(int x, int y, const string& text, const style& style, int max_x);
    void apply_border(int x, int y, int w, int h, enum class style::border border, const _NEFORCE color& c);
    void apply_style_to_cell(cell& cell, const style& style);

    void render_button(int x, int y, int w, int h, const string& label, const style& style, const tui::theme& theme,
                       style::variant variant);
    void render_checkbox(int x, int y, int w, int h, const string& label, bool checked, const style& style);
    void render_separator(int x, int y, int w);

    void clear_element_area(const element& el, const vector<layout_rect>& layout, int& idx);
    const element* find_element_at(const vector<layout_rect>& layout, const element& tree, int mx, int my, int& idx);
    component_base* hit_test_at(const vector<layout_rect>& layout, const element& tree, int mx, int my, int& idx);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RECONCILER_HPP__
