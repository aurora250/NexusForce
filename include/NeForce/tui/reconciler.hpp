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
#include "NeForce/tui/keymap.hpp"
#include "NeForce/tui/renderer.hpp"
#include "NeForce/tui/screen.hpp"
#include "NeForce/tui/selection.hpp"
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
     * @brief 丢弃帧缓冲并强制整屏重绘
     *
     * 终端内容被外部改写过后调用，下一帧会先清屏再完整重绘
     */
    void invalidate();

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
     * @brief 分发粘贴事件到焦点组件
     * @param e 粘贴事件
     * @return true 表示事件已被处理
     */
    bool dispatch_paste(const paste_event& e);

    /**
     * @brief 分发终端焦点事件
     * @param e 焦点事件
     * @return 事件总会分发给整棵组件树，返回 true
     */
    bool dispatch_focus(const focus_event& e);

    /**
     * @brief 终端是否处于焦点
     * @return 获得焦点返回 true
     */
    NEFORCE_NODISCARD bool focused() const noexcept { return focused_; }

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
     * @brief 获取最近一帧的帧缓冲
     * @return 帧缓冲引用
     *
     * 供 headless 驱动、快照测试与调试读取渲染结果。
     */
    NEFORCE_NODISCARD const screen& frame() const noexcept { return current_screen_; }

    /**
     * @brief 获取最近一帧的布局结果
     * @return 布局矩形列表
     */
    NEFORCE_NODISCARD const vector<layout_rect>& frame_layout() const noexcept { return prev_layout_; }

    /**
     * @brief 获取最近一帧的元素树
     * @return 元素树引用
     */
    NEFORCE_NODISCARD const element& frame_tree() const noexcept { return prev_tree_; }

    /**
     * @brief 获取组件调度回调
     */
    NEFORCE_NODISCARD function<void(component_base*)> schedule_render_callback();

    /**
     * @brief 应用主题
     * @param t 主题
     */
    void set_theme(const theme& t);

    /**
     * @brief 获取快捷键表
     * @return 快捷键表引用
     */
    NEFORCE_NODISCARD keymap& shortcuts() noexcept { return shortcuts_; }

    /**
     * @brief 获取快捷键表
     * @return 快捷键表常量引用
     */
    NEFORCE_NODISCARD const keymap& shortcuts() const noexcept { return shortcuts_; }

    /**
     * @brief 设置帧输出颜色深度
     * @param depth 颜色深度
     *
     * 默认按终端能力自动选择，可显式覆盖以便颜色降级测试。
     */
    void set_color_depth(color_depth depth);

    /**
     * @brief 获取当前颜色深度
     * @return 颜色深度
     */
    NEFORCE_NODISCARD color_depth color_depth_mode() const noexcept;

    /**
     * @brief 开关同步输出
     * @param enable 是否启用
     *
     * 启用后每帧以 `ESC[?2026h` / `ESC[?2026l` 包裹，
     * 支持该扩展的终端会把这一帧提交；不支持的终端会忽略该序列。
     */
    void set_synchronized_output(bool enable) noexcept { synchronized_output_ = enable; }

    /**
     * @brief 是否启用同步输出
     * @return 是否启用
     */
    NEFORCE_NODISCARD bool synchronized_output() const noexcept { return synchronized_output_; }

    /**
     * @brief 设置帧输出接收方
     * @param sink 接收 ANSI 字符串的回调，为空表示写往控制台
     *
     * 用于无终端环境（headless 测试、录制回放）接管帧输出。
     */
    void set_output_sink(function<void(string_view)> sink) { output_sink_ = _NEFORCE move(sink); }

    /**
     * @brief 获取当前屏幕选区
     * @return 选区引用
     */
    NEFORCE_NODISCARD const screen_selection& selection() const noexcept { return selection_; }

    /**
     * @brief 当前进程内最近构造的 reconciler
     */
    NEFORCE_NODISCARD static reconciler* active_instance() noexcept;

    /**
     * @brief 复制当前选区并清空
     * @return 选区内有文本并已写入剪贴板返回true
     */
    bool copy_selection();

    /**
     * @brief 清空选区
     */
    void clear_selection();

    /**
     * @brief 获取选区文本
     * @return 文本，无选区时为空串
     */
    NEFORCE_NODISCARD string selection_text() const;

    /**
     * @brief 平移选区
     * @param delta_rows 行偏移
     */
    void scroll_selection(int delta_rows);

    /**
     * @brief 开关"松开鼠标即复制"
     * @param enable 是否启用
     */
    void set_copy_on_select(bool enable) noexcept { copy_on_select_ = enable; }

    /**
     * @brief 是否启用"松开鼠标即复制"
     * @return 启用返回 true
     */
    NEFORCE_NODISCARD bool copy_on_select() const noexcept { return copy_on_select_; }

    /**
     * @brief 设置剪贴板写入回调
     * @param writer 回调，为空表示写往终端剪贴板
     */
    void set_clipboard_writer(function<void(string_view)> writer) { clipboard_writer_ = _NEFORCE move(writer); }

    /**
     * @brief 开关鼠标拖选
     * @param enable 是否启用
     */
    void set_selection_enabled(bool enable) noexcept { selection_enabled_ = enable; }

    /**
     * @brief 是否启用鼠标拖选
     * @return 启用返回true
     */
    NEFORCE_NODISCARD bool selection_enabled() const noexcept { return selection_enabled_; }

    /**
     * @brief 强制终端尺寸
     * @param width 列数
     * @param height 行数
     *
     * 设置为非正数表示恢复为自动探测。headless 场景下必须固定尺寸，
     * 否则尺寸会取自控制台。
     */
    void set_term_size(int width, int height) noexcept;

private:
    sys_console& console_;
    strand& strand_;
    io_context& ctx_;
    component_base* root_ = nullptr;

    int term_w_ = 80;
    int term_h_ = 24;
    int painted_w_ = 0;
    int painted_h_ = 0;
    theme theme_{dark_theme};

    screen current_screen_{80, 24};
    screen prev_screen_{80, 24};

    focus_manager focus_mgr_;
    keymap shortcuts_;
    event_dispatcher event_dispatcher_;
    renderer renderer_;

    element prev_tree_;
    vector<layout_rect> prev_layout_;
    function<void(string_view)> output_sink_;
    bool dirty_ = true;
    bool mounted_ = false;
    bool synchronized_output_ = true;
    int forced_w_ = 0;
    int forced_h_ = 0;
    bool update_pending_ = false;
    bool rendering_ = false;

    screen_selection selection_;
    static reconciler* active_instance_;
    bool focused_ = true;
    bool selecting_ = false;
    bool copy_on_select_ = true;
    int select_origin_x_ = 0;
    int select_origin_y_ = 0;
    bool selection_enabled_ = true;
    function<void(string_view)> clipboard_writer_;

    void refresh_term_size();
    void setup_tree(component_base* comp);
    bool handle_selection_mouse(const mouse_event& e);
    bool write_selection(bool keep);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RECONCILER_HPP__
