#ifndef NEFORCE_TUI_RECONCILER_HPP__
#define NEFORCE_TUI_RECONCILER_HPP__

/**
 * @file reconciler.hpp
 * @brief 差分渲染引擎
 *
 * Reconciler 是 TUI 框架的渲染调度核心——
 * 调用组件 render() 获取元素树，与旧树 diff，生成最小 ANSI 更新。
 */

#include "NeForce/core/system/console.hpp"
#include "NeForce/tui/component.hpp"
#include "NeForce/tui/layout.hpp"
#include "NeForce/tui/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 差分渲染引擎
 *
 * 调用 root_->render() 获取新元素树，与旧树 diff，
 * 最小化 ANSI 写入操作，仅更新变化区域。
 */
class Reconciler {
public:
    /**
     * @brief 构造函数
     * @param console 控制台渲染后端
     * @param s 串行执行器
     * @param ctx 事件循环上下文
     */
    Reconciler(sys_console& console, strand& s, io_context& ctx);

    ~Reconciler();

    Reconciler(const Reconciler&) = delete;
    Reconciler& operator=(const Reconciler&) = delete;
    Reconciler(Reconciler&&) = delete;
    Reconciler& operator=(Reconciler&&) = delete;

    /**
     * @brief 挂载根组件
     * @param root 根组件指针
     *
     * 注入 scheduleRender 回调，调用 setup()，标记全量渲染。
     */
    void mount(ComponentBase* root);

    /**
     * @brief 调度组件更新
     * @param comp 需要重渲染的组件
     */
    void scheduleUpdate(ComponentBase* comp);

    /**
     * @brief 强制全量重绘
     */
    void markDirty();

    /**
     * @brief 分发键盘事件到焦点组件
     * @param e 键盘事件
     * @return true 表示事件已被处理
     */
    bool dispatchKey(const KeyEvent& e);

    /**
     * @brief 分发鼠标事件
     * @param e 鼠标事件
     * @return true 表示事件已被处理
     */
    bool dispatchMouse(const MouseEvent& e);

    /**
     * @brief 有脏标记则执行 diff + 渲染
     */
    void flush();

    /**
     * @brief 是否有待渲染的脏标记
     * @return 是否有脏标记
     */
    NEFORCE_NODISCARD bool isDirty() const noexcept { return dirty_; }

    /**
     * @brief 获取组件调度回调
     */
    NEFORCE_NODISCARD function<void(ComponentBase*)> scheduleRenderCallback();

    /**
     * @brief 应用主题
     * @param theme 主题
     */
    void setTheme(const Theme& theme) { theme_ = theme; }

private:
    sys_console& console_;
    strand& strand_;
    io_context& ctx_;
    ComponentBase* root_ = nullptr;
    ComponentBase* focused_ = nullptr;
    vector<ComponentBase*> focusChain_;

    Element prevTree_;
    vector<LayoutRect> prevLayout_;
    int termW_ = 80;
    int termH_ = 24;
    bool dirty_ = true;
    bool mounted_ = false;
    bool updatePending_ = false;
    bool rendering_ = false;
    Theme theme_{dark_theme};

    struct ClipRect {
        int x, y, w, h;
    };
    vector<ClipRect> clipStack_;
    string outputBuffer_;

    void refreshTermSize();
    void setupTree(ComponentBase* comp);
    void buildFocusChain();

    void focusNext();
    void focusPrev();

    void setFocus(ComponentBase* comp);

    void renderDiff(const Element& oldTree, const Element& newTree, const vector<LayoutRect>& oldLayout,
                    const vector<LayoutRect>& newLayout, int& oldIdx, int& newIdx);
    void renderSubtree(const Element& el, const vector<LayoutRect>& layout, int& idx);

    void applyClear(int x, int y, int w, int h);
    void applyText(int x, int y, const string& text, const Style& style);
    void applyBorder(int x, int y, int w, int h, Border border, const color& c);

    void renderButton(int x, int y, int w, int h, const string& label, const Style& style, const Theme& theme,
                      Variant variant);
    void renderCheckbox(int x, int y, const string& label, bool checked, const Style& style);
    void renderSeparator(int x, int y, int w);

    void clearElementArea(const Element& el, const vector<LayoutRect>& layout, int& idx);

    const Element* findElementAt(const vector<LayoutRect>& layout, const Element& tree, int mx, int my, int& idx);

    ComponentBase* hitTestAt(const vector<LayoutRect>& layout, const Element& tree, int mx, int my, int& idx);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RECONCILER_HPP__
