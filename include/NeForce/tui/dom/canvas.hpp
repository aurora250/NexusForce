#ifndef NEFORCE_TUI_DOM_CANVAS_HPP__
#define NEFORCE_TUI_DOM_CANVAS_HPP__

/**
 * @file canvas.hpp
 * @brief 自定义画布绘制上下文
 *
 * 画布上下文把屏幕帧缓冲包装成以元素左上角为原点的局部坐标系，
 * 供 `element::canvas` 的自定义绘制回调逐格写入字符与样式。
 */

#include "NeForce/core/string/codepoint.hpp"
#include "NeForce/tui/dom/style.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class screen;
class renderer;
/// @endcond

/**
 * @brief 画布绘制上下文
 *
 * 回调内使用局部坐标：`(0, 0)` 是画布元素左上角，`(width()-1, height()-1)` 是右下角。
 * 写入越界、被裁剪或落在终端之外的字符会被静默丢弃。
 *
 * 局部坐标以终端字符列为单位，宽字符按 `codepoint::display_width()` 占据后续列。
 * 所有写入会自动与画布元素的基础样式合并，因此回调只需给出差异化样式。
 *
 * @note 上下文只在下一次绘制调用前有效，回调返回后不得保存其引用。
 * @warning 绘制回调在渲染阶段执行，禁止在其中修改状态或触发重渲染。
 */
class NEFORCE_API canvas_painter {
public:
    canvas_painter(const canvas_painter&) = delete;
    canvas_painter& operator=(const canvas_painter&) = delete;
    canvas_painter(canvas_painter&&) = delete;
    canvas_painter& operator=(canvas_painter&&) = delete;

    /**
     * @brief 获取画布宽度
     * @return 列数
     */
    NEFORCE_NODISCARD int width() const noexcept { return w_; }

    /**
     * @brief 获取画布高度
     * @return 行数
     */
    NEFORCE_NODISCARD int height() const noexcept { return h_; }

    /**
     * @brief 获取当前主题
     * @return 主题引用
     *
     * 画布不自动使用主题颜色，回调可据此解析角色色。
     */
    NEFORCE_NODISCARD const theme& theme_ref() const noexcept { return theme_; }

    /**
     * @brief 获取画布基础样式
     * @return 元素样式引用
     */
    NEFORCE_NODISCARD const style& base_style() const noexcept { return base_; }

    /**
     * @brief 判断局部坐标是否位于可绘制区域内
     * @param x 列坐标
     * @param y 行坐标
     * @return 可绘制返回 true
     */
    NEFORCE_NODISCARD bool contains(int x, int y) const noexcept;

    /**
     * @brief 写入一个字符
     * @param x 列坐标
     * @param y 行坐标
     * @param glyph UTF-8 字符串，只写入第一个码点
     * @param over 覆盖样式
     */
    void put(int x, int y, string_view glyph, const style& over = {});

    /**
     * @brief 写入一个码点
     * @param x 列坐标
     * @param y 行坐标
     * @param cp 码点
     * @param over 覆盖样式
     */
    void put(int x, int y, const codepoint& cp, const style& over = {});

    /**
     * @brief 用同一个字符填充矩形
     * @param x 起始列
     * @param y 起始行
     * @param w 宽度
     * @param h 高度
     * @param glyph UTF-8 字符
     * @param over 覆盖样式
     */
    void fill(int x, int y, int w, int h, string_view glyph, const style& over = {});

    /**
     * @brief 绘制水平线
     * @param x 起始列
     * @param y 行坐标
     * @param w 长度
     * @param glyph UTF-8 字符
     * @param over 覆盖样式
     */
    void hline(int x, int y, int w, string_view glyph, const style& over = {});

    /**
     * @brief 绘制垂直线
     * @param x 列坐标
     * @param y 起始行
     * @param h 长度
     * @param glyph UTF-8 字符
     * @param over 覆盖样式
     */
    void vline(int x, int y, int h, string_view glyph, const style& over = {});

    /**
     * @brief 绘制 UTF-8 文本
     * @param x 起始列
     * @param y 行坐标
     * @param content 文本
     * @param over 覆盖样式
     * @return 实际推进的列数
     *
     * 文本不会换行，超出画布右边界时截断。
     */
    int text(int x, int y, string_view content, const style& over = {});

    /**
     * @brief 在给定宽度内右对齐绘制文本
     * @param x 区域起始列
     * @param y 行坐标
     * @param w 区域宽度
     * @param content 文本
     * @param over 覆盖样式
     * @return 文本起始列，未绘制时返回 x
     */
    int text_right(int x, int y, int w, string_view content, const style& over = {});

    /**
     * @brief 计算自给定列起可写入的列数
     * @param x 起始列
     * @return 可写入列数，越界时为 0
     */
    NEFORCE_NODISCARD int available_width(int x) const noexcept;

private:
    friend class renderer;

    screen& screen_;
    const theme& theme_;
    style base_;
    int origin_x_;
    int origin_y_;
    int w_;
    int h_;
    int clip_x_;
    int clip_y_;
    int clip_w_;
    int clip_h_;

    canvas_painter(screen& target, const theme& t, int x, int y, int w, int h, style base, int clip_x, int clip_y,
                   int clip_w, int clip_h);

    NEFORCE_NODISCARD bool visible(int x, int y) const noexcept;
    void write_cell(int x, int y, const codepoint& cp, const style& over);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_CANVAS_HPP__
