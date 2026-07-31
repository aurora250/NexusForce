#ifndef NEFORCE_TUI_SCREEN_HPP__
#define NEFORCE_TUI_SCREEN_HPP__

/**
 * @file screen.hpp
 * @brief 终端帧缓冲抽象
 */

#include "NeForce/core/utility/color.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class screen;
/// @endcond

/**
 * @brief 终端尺寸
 */
struct dimensions {
    int dimx; ///< 宽度
    int dimy; ///< 高度
};

/**
 * @brief 像素/字符单元
 */
struct cell {
    string character;           ///< 显示的字符
    _NEFORCE color foreground;  ///< 前景色
    _NEFORCE color background;  ///< 背景色
    bool bold : 1;              ///< 粗体
    bool dim : 1;               ///< 暗色
    bool italic : 1;            ///< 斜体
    bool inverted : 1;          ///< 反色
    bool underlined : 1;        ///< 单下划线
    bool underlined_double : 1; ///< 双下划线
    bool blink : 1;             ///< 闪烁
    bool strikethrough : 1;     ///< 删除线
    bool automerge : 1;         ///< 相邻同内容自动融合
    uint8_t hyperlink = 0;      ///< 超链接 ID（0 表示无链接）

    cell() noexcept :
    bold(false),
    dim(false),
    italic(false),
    inverted(false),
    underlined(false),
    underlined_double(false),
    blink(false),
    strikethrough(false),
    automerge(false) {}

    /**
     * @brief 重置所有样式为默认值
     */
    void reset() noexcept {
        bold = dim = italic = inverted = underlined = underlined_double = blink = strikethrough = automerge = false;
        hyperlink = 0;
        foreground = _NEFORCE color{};
        background = _NEFORCE color{};
        character.clear();
    }

    /**
     * @brief 比较两个 cell 的视觉内容
     * @param other 另一个 cell
     * @return 是否视觉相同
     */
    NEFORCE_NODISCARD bool visually_equal(const cell& other) const noexcept {
        return character == other.character && bold == other.bold && dim == other.dim && italic == other.italic &&
               inverted == other.inverted && underlined == other.underlined &&
               underlined_double == other.underlined_double && blink == other.blink &&
               strikethrough == other.strikethrough && hyperlink == other.hyperlink && foreground == other.foreground &&
               background == other.background;
    }
};

/**
 * @brief 光标状态
 */
struct cursor {
    /**
 * @brief 光标形状枚举
 */
    enum class shape : uint8_t {
        Hidden,            ///< 隐藏光标
        BlockBlinking,     ///< 闪烁块光标
        Block,             ///< 块光标
        UnderlineBlinking, ///< 闪烁下划线光标
        Underline,         ///< 下划线光标
        BarBlinking,       ///< 闪烁竖线光标
        Bar,               ///< 竖线光标
    };

    int x = 0;                   ///< 列坐标
    int y = 0;                   ///< 行坐标
    shape shape = shape::Hidden; ///< 光标形状
};

/**
 * @brief 像素网格基类
 *
 * 管理一维 cell 数组和二维坐标映射。
 */
class NEFORCE_API surface {
public:
    /**
     * @brief 构造函数
     * @param dimx 宽度
     * @param dimy 高度
     */
    surface(int dimx, int dimy);

    virtual ~surface() = default;

    /**
     * @brief 获取宽度
     * @return 列数
     */
    NEFORCE_NODISCARD int dimx() const noexcept { return dimx_; }

    /**
     * @brief 获取高度
     * @return 行数
     */
    NEFORCE_NODISCARD int dimy() const noexcept { return dimy_; }

    /**
     * @brief 有边界检查的字符访问
     * @param x 列坐标
     * @param y 行坐标
     * @return 字符引用
     */
    NEFORCE_NODISCARD string& at(int x, int y);

    /**
     * @brief 有边界检查的 cell 访问
     * @param x 列坐标
     * @param y 行坐标
     * @return cell 引用
     */
    NEFORCE_NODISCARD cell& cell_at(int x, int y);

    /**
     * @brief 无边界检查的 cell 访问
     * @param x 列坐标
     * @param y 行坐标
     * @return cell 引用
     */
    NEFORCE_NODISCARD cell& fast_cell_at(int x, int y) noexcept { return cells_[static_cast<size_t>(y) * dimx_ + x]; }

    /**
     * @brief 清空所有 cell
     */
    virtual void clear();

protected:
    int dimx_;
    int dimy_;
    vector<cell> cells_;
};

/**
 * @brief 终端屏幕帧缓冲
 *
 * 继承 surface，附加光标管理、超链接注册和 ANSI 序列化。
 */
// NOLINTNEXTLINE(bugprone-exception-escape)
class NEFORCE_API screen final : public surface {
public:
    /**
     * @brief 构造函数
     * @param dimx 宽度
     * @param dimy 高度
     */
    screen(int dimx, int dimy);

    /**
     * @brief 调整尺寸
     * @param dimx 新宽度
     * @param dimy 新高度
     */
    void resize(int dimx, int dimy);

    /**
     * @brief 获取光标
     * @return 当前光标状态
     */
    NEFORCE_NODISCARD const tui::cursor& cursor() const noexcept { return cursor_; }

    /**
     * @brief 设置光标
     * @param c 新光标状态
     */
    void set_cursor(const tui::cursor& c) noexcept { cursor_ = c; }

    /**
     * @brief 注册超链接，返回句柄
     * @param link URL 字符串
     * @return 超链接句柄（1-255），0 表示无链接
     */
    uint8_t register_hyperlink(const string& link);

    /**
     * @brief 根据句柄查找超链接
     * @param id 超链接句柄
     * @return URL 字符串
     */
    NEFORCE_NODISCARD const string& hyperlink(uint8_t id) const;

    /**
     * @brief 生成 ANSI 转义序列
     * @param prev 上一帧的 screen
     * @return ANSI 转义序列字符串
     *
     * 比较当前帧与上一帧的每个 cell，仅输出变化的
     * 样式和字符，最小化终端写入量。
     */
    NEFORCE_NODISCARD string to_string(const screen& prev) const;

    /**
     * @brief 导出为纯文本（无 ANSI 转义）
     * @return 纯文本字符串，行尾去除空白，行间换行符分隔
     *
     * 用于快照测试。样式信息不保留，仅输出字符内容。
     */
    NEFORCE_NODISCARD string to_plaintext() const;

    /**
     * @brief 重置光标位置跟踪
     * @param clear 同时清空 cell 内容
     */
    void reset_position(bool clear = false);

    /**
     * @brief 清空所有状态
     */
    void clear() override;

private:
    tui::cursor cursor_;
    vector<string> hyperlinks_;

    void update_cell_style(string& ss, const cell& prev, const cell& next) const;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_SCREEN_HPP__
