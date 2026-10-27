#ifndef NEFORCE_TUI_SCREEN_HPP__
#define NEFORCE_TUI_SCREEN_HPP__

/**
 * @file screen.hpp
 * @brief 终端帧缓冲抽象
 */

#include "NeForce/core/string/codepoint.hpp"
#include "NeForce/core/string/width.hpp"
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
 * @brief 单元格字形
 *
 * 单个 UTF-8 码点的定长存储（最多 4 字节），并缓存显示宽度。
 * 固定 8 字节，使整屏清空与差量比较保持连续访存。
 */
class cell_text {
public:
    /**
     * @brief 构造空字形
     */
    cell_text() noexcept = default;

    /**
     * @brief 从 UTF-8 文本构造，只保留首个码点
     * @param text 文本
     */
    cell_text(const char* text) noexcept { assign(string_view{text}); }

    /**
     * @brief 从 UTF-8 文本构造，只保留首个码点
     * @param text 文本
     */
    explicit cell_text(const string_view text) noexcept { assign(text); }

    /**
     * @brief 赋值 UTF-8 文本，只保留首个码点
     * @param text 文本
     * @return 自身引用
     */
    cell_text& operator=(const char* text) noexcept {
        assign(string_view{text});
        return *this;
    }

    /**
     * @brief 赋值 UTF-8 文本，只保留首个码点
     * @param text 文本
     * @return 自身引用
     */
    cell_text& operator=(const string_view text) noexcept {
        assign(text);
        return *this;
    }

    /**
     * @brief 赋值 UTF-8 文本，只保留首个码点
     * @param text 文本
     * @return 自身引用
     */
    cell_text& operator=(const string& text) noexcept {
        assign(text.view());
        return *this;
    }

    /**
     * @brief 写入一个码点
     * @param cp 码点
     * @param width 该码点的显示宽度
     */
    void set(const codepoint& cp, const int width) noexcept {
        length_ = static_cast<uint8_t>(cp.write_utf8(bytes_));
        width_ = static_cast<int8_t>(width);
    }

    /**
     * @brief 赋值 UTF-8 文本，只保留首个码点并重新计算宽度
     * @param text 文本
     */
    void assign(const string_view text) noexcept {
        const size_t size = text.size() < sizeof(bytes_) ? text.size() : sizeof(bytes_);
        for (size_t i = 0; i < size; ++i) {
            bytes_[i] = text[i];
        }
        length_ = static_cast<uint8_t>(size);
        width_ = static_cast<int8_t>(size == 0 ? 0 : text_display_width(view()));
    }

    /**
     * @brief 清空字形
     */
    void clear() noexcept {
        length_ = 0;
        width_ = 0;
    }

    /**
     * @brief 字节数
     * @return UTF-8 字节数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return length_; }

    /**
     * @brief 是否为空
     * @return 空返回 true
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return length_ == 0; }

    /**
     * @brief UTF-8 字节指针
     * @return 数据指针
     */
    NEFORCE_NODISCARD const char* data() const noexcept { return bytes_; }

    /**
     * @brief UTF-8 文本视图
     * @return 文本视图
     */
    NEFORCE_NODISCARD string_view view() const noexcept { return {bytes_, length_}; }

    /**
     * @brief 显示宽度
     * @return 列数
     */
    NEFORCE_NODISCARD int width() const noexcept { return width_; }

    /**
     * @brief 字符串视图转换
     * @return 文本视图
     */
    operator string_view() const noexcept { return view(); }

    NEFORCE_NODISCARD bool operator==(const cell_text& other) const noexcept {
        return length_ == other.length_ && (length_ == 0 || memory_compare(bytes_, other.bytes_, length_) == 0);
    }

    NEFORCE_NODISCARD bool operator!=(const cell_text& other) const noexcept { return !(*this == other); }

    NEFORCE_NODISCARD bool operator==(const string_view other) const noexcept { return view() == other; }

    NEFORCE_NODISCARD bool operator!=(const string_view other) const noexcept { return view() != other; }

    NEFORCE_NODISCARD bool operator==(const char* other) const noexcept { return view() == string_view{other}; }

    NEFORCE_NODISCARD bool operator!=(const char* other) const noexcept { return view() != string_view{other}; }

    NEFORCE_NODISCARD bool operator==(const string& other) const noexcept { return view() == other.view(); }

    NEFORCE_NODISCARD bool operator!=(const string& other) const noexcept { return view() != other.view(); }

private:
    char bytes_[4]{};    ///< UTF-8 字节
    uint8_t length_ = 0; ///< 字节数
    int8_t width_ = 0;   ///< 显示宽度
    uint16_t reserved_ = 0;
};

/**
 * @brief 像素/字符单元
 */
struct cell {
    cell_text character;        ///< 显示的字形
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
    uint8_t reserved_ = 0;      ///< 填充字节

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
        background = _NEFORCE color{0, 0, 0, 0};
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
 * @brief 终端颜色深度
 */
enum class color_depth : uint8_t {
    ansi16,    ///< 基础 16 色（SGR 30-37 / 90-97）
    ansi256,   ///< 256 色（SGR 38;5;n）
    truecolor, ///< 24 位真彩色（SGR 38;2;r;g;b）
};

/**
 * @brief 光标状态
 */
struct cursor {
    /**
 * @brief 光标形状枚举
 */
    enum class shape : uint8_t {
        hidden,             ///< 隐藏光标
        block_blinking,     ///< 闪烁块光标
        block,              ///< 块光标
        underline_blinking, ///< 闪烁下划线光标
        underline,          ///< 下划线光标
        bar_blinking,       ///< 闪烁竖线光标
        bar,                ///< 竖线光标
    };

    int x = 0;                   ///< 列坐标
    int y = 0;                   ///< 行坐标
    shape shape = shape::hidden; ///< 光标形状
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
    NEFORCE_NODISCARD string at(int x, int y) const;

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
     * @brief 无边界检查的只读 cell 访问
     * @param x 列坐标
     * @param y 行坐标
     * @return cell 常量引用
     */
    NEFORCE_NODISCARD const cell& fast_cell_at(int x, int y) const noexcept {
        return cells_[static_cast<size_t>(y) * dimx_ + x];
    }

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
    NEFORCE_NODISCARD string_view hyperlink(uint8_t id) const noexcept;

    /**
     * @brief 生成 ANSI 转义序列
     * @param prev 上一帧的 screen
     * @return ANSI 转义序列字符串
     *
     * 比较当前帧与上一帧的每个 cell，仅输出变化的样式和字符，最小化终端写入量。
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
     * @brief 设置输出颜色深度
     * @param depth 颜色深度
     *
     * 决定 to_string() 生成的 SGR 序列形式：
     * 默认 256 色，真彩色终端可输出 24 位色，能力不足时降级到 16 色。
     */
    void set_color_depth(color_depth depth) noexcept { depth_ = depth; }

    /**
     * @brief 获取输出颜色深度
     * @return 颜色深度
     */
    NEFORCE_NODISCARD color_depth color_depth_mode() const noexcept { return depth_; }

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
    color_depth depth_ = color_depth::ansi256;
    vector<string> hyperlinks_;

    void update_cell_style(string& ss, const cell& prev, const cell& next) const;
    void append_cursor(string& ss) const;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_SCREEN_HPP__
