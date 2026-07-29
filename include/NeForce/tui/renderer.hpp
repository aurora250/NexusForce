#ifndef NEFORCE_TUI_RENDERER_HPP__
#define NEFORCE_TUI_RENDERER_HPP__

/**
 * @file renderer.hpp
 * @brief 元素树渲染器
 *
 * renderer 负责将虚拟元素树渲染到 screen 帧缓冲区——
 * 遍历元素树与布局矩形，绘制文本、边框、样式、滚动条等。
 */

#include "NeForce/tui/dom/element.hpp"
#include "NeForce/tui/dom/style.hpp"
#include "NeForce/tui/screen.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class component_base;
/// @endcond

/**
 * @brief 滚动条命中信息
 *
 * 由渲染器在 scroll_view 渲染时收集，供事件分发器处理鼠标拖拽。
 */
struct scrollbar_hit {
    void* scroll_state; /**< state<int>* 指针 */
    int track_x;        /**< 滚动条轨道列坐标 */
    int track_y;        /**< 滚动条轨道行坐标 */
    int track_h;        /**< 滚动条轨道长度 */
    int thumb_pos;      /**< 滑块起始位置 */
    int thumb_size;     /**< 滑块大小 */
    int content_h;      /**< 内容总高度（或宽度） */
    int sv_x;           /**< scroll_view 区域列坐标 */
    int sv_w;           /**< scroll_view 区域宽度 */
    bool vertical;      /**< 垂直滚动条则为 true */
};

/**
 * @brief 元素树渲染器
 *
 * 将元素树与布局结果渲染到 screen 帧缓冲区。
 * 负责文本换行、边框绘制、样式应用、滚动条绘制等。
 */
class NEFORCE_API renderer {
public:
    /**
     * @brief 构造函数
     * @param target 渲染目标 screen
     * @param t 主题
     * @param term_w 终端宽度
     * @param term_h 终端高度
     */
    renderer(screen& target, const theme& t, const int& term_w, const int& term_h);

    /**
     * @brief 渲染元素树
     * @param tree 元素树根节点
     * @param layout 布局结果
     * @param has_focus 当前是否有焦点组件
     */
    void render(const element& tree, const vector<layout_rect>& layout, bool has_focus);

    /**
     * @brief 获取本次渲染收集的滚动条命中信息
     * @return 滚动条命中列表
     */
    NEFORCE_NODISCARD const vector<scrollbar_hit>& scrollbar_hits() const noexcept { return scrollbar_hits_; }

    /**
     * @brief 渲染文本块
     * @param x 起始列
     * @param y 起始行
     * @param w 宽度约束
     * @param h 高度约束
     * @param text 文本内容
     * @param style 样式
     * @param wm 换行模式
     * @param out_end_x 输出结束列
     * @param out_end_y 输出结束行
     */
    void render_text_block(int x, int y, int w, int h, const string& text, const tui::style& style, style::wrap_mode wm,
                           int* out_end_x = nullptr, int* out_end_y = nullptr);

    /**
     * @brief 绘制边框
     * @param x 起始列
     * @param y 起始行
     * @param w 宽度
     * @param h 高度
     * @param border 边框样式
     * @param c 边框颜色
     */
    void apply_border(int x, int y, int w, int h, enum style::border border, const _NEFORCE color& c);

    /**
     * @brief 将样式字段应用到 cell
     * @param cell 目标 cell
     * @param style 样式
     */
    void apply_style_to_cell(cell& cell, const style& style);

    /**
     * @brief 渲染按钮
     * @param x 起始列
     * @param y 起始行
     * @param w 宽度
     * @param h 高度
     * @param label 按钮文本
     * @param style 样式
     * @param theme 主题
     * @param variant 按钮变体
     */
    void render_button(int x, int y, int w, int h, const string& label, const style& style, const tui::theme& theme,
                       style::variant variant);

    /**
     * @brief 渲染复选框
     * @param x 起始列
     * @param y 起始行
     * @param w 宽度
     * @param h 高度
     * @param label 标签文本
     * @param checked 是否选中
     * @param style 样式
     */
    void render_checkbox(int x, int y, int w, int h, const string& label, bool checked, const style& style);

    /**
     * @brief 渲染分隔线
     * @param x 起始列
     * @param y 行
     * @param w 宽度
     */
    void render_separator(int x, int y, int w);

    /**
     * @brief 清除指定区域
     * @param x 起始列
     * @param y 起始行
     * @param w 宽度
     * @param h 高度
     */
    void apply_clear(int x, int y, int w, int h);

    /**
     * @brief 写入文本
     * @param x 起始列
     * @param y 行
     * @param text 文本
     * @param style 样式
     */
    void apply_text(int x, int y, const string& text, const style& style);

    /**
     * @brief 写入文本
     * @param x 起始列
     * @param y 行
     * @param text 文本
     * @param style 样式
     * @param max_x 最大列（超出则截断）
     */
    void apply_text(int x, int y, const string& text, const style& style, int max_x);

    /**
     * @brief 清除元素树中所有叶子元素区域
     * @param el 元素树根
     * @param layout 布局结果
     * @param idx 当前布局索引
     */
    void clear_element_area(const element& el, const vector<layout_rect>& layout, int& idx);

private:
    struct clip_rect {
        int x, y, w, h;
    };

    screen& screen_;
    const theme& theme_;
    const int& term_w_;
    const int& term_h_;
    vector<clip_rect> clip_stack_;
    vector<scrollbar_hit> scrollbar_hits_;
    bool has_focus_ = false;

    void render_subtree(const element& el, const vector<layout_rect>& layout, int& idx);
};

/**
 * @brief 在元素树中查找可点击元素
 * @param layout 布局结果
 * @param tree 元素树
 * @param mx 鼠标列坐标
 * @param my 鼠标行坐标
 * @param idx 当前布局索引
 * @return 命中的元素指针，未命中返回 nullptr
 */
NEFORCE_NODISCARD NEFORCE_API const element* find_element_at(const vector<layout_rect>& layout, const element& tree,
                                                             int mx, int my, int& idx);

/**
 * @brief 命中测试
 * @param layout 布局结果
 * @param tree 元素树
 * @param mx 鼠标列坐标
 * @param my 鼠标行坐标
 * @param idx 当前布局索引
 * @param fallback 无 owner 时的回退组件
 * @return 命中的组件指针，未命中返回 nullptr
 *
 * 找到鼠标坐标下的组件
 */
NEFORCE_NODISCARD NEFORCE_API component_base* hit_test_at(const vector<layout_rect>& layout, const element& tree,
                                                          int mx, int my, int& idx, component_base* fallback);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RENDERER_HPP__
