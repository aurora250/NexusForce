#ifndef NEFORCE_TUI_TERMINAL_PROTOCOL_HPP__
#define NEFORCE_TUI_TERMINAL_PROTOCOL_HPP__

/**
 * @file terminal_protocol.hpp
 * @brief 终端能力探测与 OSC 控制序列
 *
 * 把终端环境变量映射为能力画像，并提供背景色查询、通知、光标形状、焦点事件
 * 与 Kitty 键盘协议等转义序列的构造与解析。
 *
 * 所有函数都是无 I/O 的纯函数，便于在测试或非终端环境中复用。
 */

#include "NeForce/core/utility/color.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @struct terminal_profile
 * @brief 终端画像
 *
 * 由 TERM、COLORTERM 等环境变量推断出的能力集合。各字段为保守估计，
 * 未识别的终端不会声称支持高级能力。
 */
struct terminal_profile {
    bool truecolor = false;           ///< 支持 24 位色
    bool ansi256 = false;             ///< 支持 256 色
    bool unicode = false;             ///< 支持 Unicode
    bool kitty_keyboard = false;      ///< 支持 Kitty 键盘协议
    bool focus_events = false;        ///< 支持焦点事件
    bool synchronized_output = false; ///< 支持 DEC 2026 同步输出
    bool kitty_graphics = false;      ///< 支持 Kitty 图形
    bool sixel = false;               ///< 支持 Sixel
    bool tmux = false;                ///< 运行在 tmux 下
};

/**
 * @brief 通知方式
 */
enum class notify_method : uint8_t {
    auto_ = 0, ///< 按终端画像选择
    osc9,      ///< OSC 9
    osc777,    ///< OSC 777
    osc99,     ///< OSC 99
    bel,       ///< 响铃
    windows,   ///< Windows 原生通知（非 Windows 平台退化为响铃）
};

/**
 * @brief 光标形状（DECSCUSR）
 */
enum class cursor_shape : uint8_t {
    default_ = 0,    ///< 终端默认
    block_blink,     ///< 闪烁方块
    block,           ///< 稳定方块
    underline_blink, ///< 闪烁下划线
    underline,       ///< 稳定下划线
    bar_blink,       ///< 闪烁竖线
    bar,             ///< 稳定竖线
};

/**
 * @brief 从环境变量字符串推断终端画像
 * @param term TERM
 * @param colorterm COLORTERM
 * @param term_program TERM_PROGRAM
 * @param tmux TMUX
 * @param kitty_window_id KITTY_WINDOW_ID
 * @param wt_session WT_SESSION
 * @return 画像
 *
 * `tmux` 非空时 `synchronized_output` 强制为 false。
 */
NEFORCE_API terminal_profile detect_terminal_profile(string_view term, string_view colorterm, string_view term_program,
                                                     string_view tmux, string_view kitty_window_id,
                                                     string_view wt_session);

/**
 * @brief 构造 OSC 11 背景色查询序列
 * @return `ESC ] 11 ; ? BEL`
 */
NEFORCE_API string build_osc11_query();

/**
 * @brief 解析 OSC 11 应答
 * @param reply 应答文本
 * @return 背景色；格式不符返回 none
 *
 * 支持 `ESC]11;rgb:RRRR/GGGG/BBBB` 与 `ESC]11;#RRGGBB` 两种形态，大小写不敏感。
 * 允许首尾空白以及 BEL、ST（`ESC \`）两种终止符；16 位分量取高字节缩放为 8 位。
 */
NEFORCE_API optional<_NEFORCE color> parse_osc11_reply(string_view reply);

/**
 * @brief 根据背景色亮度判断是否为浅色终端
 * @param background 背景色
 * @return 亮度 > 140 返回 true
 *
 * 亮度采用 BT.709 加权：`0.299 R + 0.587 G + 0.114 B`。
 */
NEFORCE_API bool is_light_background(_NEFORCE color background) noexcept;

/**
 * @brief 构造通知序列
 * @param title 标题
 * @param body 正文
 * @param method 方式
 * @return 转义序列；`windows` 在非 Windows 上退化为 BEL
 *
 * `auto_` 在当前实现中固定解析为 `osc9`。
 * 调用方可先用 `detect_terminal_profile()` 检测终端，再传入与画像匹配的具体方式。
 */
NEFORCE_API string build_notification(string_view title, string_view body, notify_method method = notify_method::auto_);

/**
 * @brief 构造光标形状序列
 * @param shape 形状
 * @return `ESC [ n q`
 *
 * 超出 0-6 的枚举值退化为终端默认形状。
 */
NEFORCE_API string build_cursor_shape(cursor_shape shape);

/**
 * @brief 构造启用/禁用焦点事件的序列
 * @param enable 是否启用
 * @return `ESC [ ? 1004 h/l`
 */
NEFORCE_API string build_focus_events(bool enable);

/**
 * @brief 构造启用/禁用 Kitty 键盘协议的序列
 * @param enable 是否启用
 * @param flags 协议标志位（1 = disambiguate，2 = report event types，4 = report alternate keys）
 * @return `ESC [ > flags u` / `ESC [ < u`
 *
 * 禁用序列不携带标志位；负数按 0 处理。
 */
NEFORCE_API string build_kitty_keyboard(bool enable, int flags = 1);

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_TERMINAL_PROTOCOL_HPP__
