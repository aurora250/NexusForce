#ifndef NEFORCE_TUI_SELECTION_HPP__
#define NEFORCE_TUI_SELECTION_HPP__

/**
 * @file selection.hpp
 * @brief 屏幕选区
 *
 * 以帧缓冲单元格坐标描述"用户拖选的一段屏幕内容"。选区落在帧缓冲上而不是某个组件的文本上，
 * 因此可以跨越日志、表格、工具卡片等任意组件连续选取。
 */

#include "NeForce/tui/screen.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @struct screen_selection
 * @brief 屏幕选区
 *
 * 起点是按下鼠标的单元格，终点是当前拖动位置，
 * 两者之间的所有单元格都属于选区，中间行整行参与选取。
 */
struct screen_selection {
    bool active = false; ///< 是否已有选区
    int anchor_x = 0;    ///< 起点列
    int anchor_y = 0;    ///< 起点行
    int caret_x = 0;     ///< 终点列
    int caret_y = 0;     ///< 终点行

    /**
     * @brief 是否没有可复制的单元格
     * @return 未激活返回true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return !active; }

    /**
     * @brief 以指定单元格为起点开始新选区
     * @param x 列坐标
     * @param y 行坐标
     */
    void start(int x, int y) noexcept;

    /**
     * @brief 移动选区终点
     * @param x 列坐标
     * @param y 行坐标
     */
    void extend_to(int x, int y) noexcept;

    /**
     * @brief 清空选区
     */
    void clear() noexcept;

    /**
     * @brief 计算归一化后的对角坐标
     * @param x1 起始列（含）
     * @param y1 起始行（含）
     * @param x2 结束列（含）
     * @param y2 结束行（含）
     */
    void bounds(int& x1, int& y1, int& x2, int& y2) const noexcept;

    /**
     * @brief 判断单元格是否落在选区内
     * @param x 列坐标
     * @param y 行坐标
     * @return 在选区内返回true
     */
    NEFORCE_NODISCARD bool contains(int x, int y) const noexcept;

    /**
     * @brief 平移选区
     * @param delta_y 行偏移
     */
    void translate(int delta_y) noexcept;
};

/**
 * @brief 反色高亮选区内的单元格
 * @param target 目标帧缓冲
 * @param selection 选区
 * @note 直接改写帧缓冲里的样式位，因此与绘制顺序无关，可以在整帧渲染完成后统一调用
 */
NEFORCE_API void paint_selection(surface& target, const screen_selection& selection);

/**
 * @brief 提取选区文本
 * @param source 来源帧缓冲
 * @param selection 选区
 * @return 纯文本，行内尾随空白被去除，行间以换行符分隔；无选区时返回空串
 */
NEFORCE_NODISCARD NEFORCE_API string selection_text(const surface& source, const screen_selection& selection);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_SELECTION_HPP__
