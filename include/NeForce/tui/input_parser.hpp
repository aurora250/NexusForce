#ifndef NEFORCE_TUI_INPUT_PARSER_HPP__
#define NEFORCE_TUI_INPUT_PARSER_HPP__

/**
 * @file input_parser.hpp
 * @brief 终端输入序列解析器
 *
 * 把 stdin 的原始字节流切分为键盘、鼠标与粘贴事件。
 * 解析器是无 I/O 的纯状态机，可脱离终端单独测试。
 */

#include "NeForce/core/system/console.hpp"
#include "NeForce/tui/events.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 解析结果
 *
 * 一次 feed() 最多产出一个事件，具体类型由 type 指示，
 * 对应的字段仅在匹配的 type 下有效。
 */
struct parsed_input {
    /**
     * @brief 事件类型
     */
    enum class kind : uint8_t {
        none,      ///< 未产出事件
        key,       ///< 键盘事件，见 key
        mouse,     ///< 鼠标事件，见 mouse
        paste,     ///< 粘贴事件，见 paste_text
        clipboard, ///< 剪贴板应答
        focus      ///< 焦点事件，见 focused
    };

    kind type = kind::none; ///< 事件类型
    tui::key_event key;     ///< 键盘事件
    tui::mouse_event mouse; ///< 鼠标事件
    string paste_text;      ///< 剪贴板文本
    bool focused = true;    ///< 焦点事件
};

/**
 * @brief 终端输入序列解析器
 *
 * 增量接收字节，识别以下输入形式：
 * - 可打印字符与 UTF-8 多字节字符
 * - 控制字符（含 Ctrl 组合键）
 * - CSI / SS3 转义序列（方向键、Home/End、PgUp/PgDn、Insert/Delete、F1-F12、Shift+Tab）
 * - 带修饰键的 CSI 序列（`ESC[1;5C` 等，解码为 Ctrl/Alt/Shift 修饰）
 * - Alt+字符（`ESC` 前缀）
 * - SGR 鼠标序列（`ESC[<b;x;yM/m`）
 * - 括号粘贴（`ESC[200~` … `ESC[201~`）
 * - OSC 52 剪贴板应答（`ESC]52;c;<base64>BEL` 或 `ESC \\` 结尾）
 * - 焦点事件（`ESC[I` / `ESC[O`）
 * - Kitty 键盘协议（`ESC[<code>;<mods>[:<event>]u`）与 modifyOtherKeys（`ESC[27;<mods>;<code>~`）
 *
 * @note 解析器不持有任何终端资源，应用需自行开启 raw 模式、`?1006` 鼠标模式与 `?2004` 括号粘贴模式
 */
class NEFORCE_API input_parser {
public:
    /**
     * @brief 输入一个字节
     * @param byte 输入字节
     * @param out 产出的解析结果
     * @return true 表示 out 中有一个事件
     */
    bool feed(byte_t byte, parsed_input& out);

    /**
     * @brief 结束当前批次
     * @param out 产出的解析结果
     * @return true 表示 out 中有一个事件
     *
     * 用于在读取批次结束后冲刷悬挂状态：
     * 单独的 `ESC` 会作为 Escape 键产出，未完成的转义序列被丢弃；
     * 未结束的粘贴内容保留在缓冲区中等待后续字节。
     */
    bool flush(parsed_input& out);

    /**
     * @brief 重置解析器
     */
    void reset();

    /**
     * @brief 是否正处于粘贴收集中
     * @return 是否处于粘贴状态
     */
    NEFORCE_NODISCARD bool in_paste() const noexcept { return state_ == state::paste; }

private:
    enum class state : uint8_t {
        ground,
        escape,
        csi,
        ss3,
        utf8,
        paste,
        osc,
    };

    state state_ = state::ground;
    string seq_;
    string utf8_;
    string paste_;
    string osc_;
    key_modifier alt_ = key_modifier::none;

    void emit_key(parsed_input& out, const key_event& ke);
    bool feed_ground(byte_t byte, parsed_input& out);
    bool feed_escape(byte_t byte, parsed_input& out);
    bool feed_csi(byte_t byte, parsed_input& out);
    bool feed_ss3(byte_t byte, parsed_input& out);
    bool feed_utf8(byte_t byte, parsed_input& out);
    bool feed_paste(byte_t byte, parsed_input& out);
    bool feed_osc(byte_t byte, parsed_input& out);
    bool finish_osc(parsed_input& out);
    bool complete_utf8(parsed_input& out);
    bool finish_csi(parsed_input& out);
    bool parse_sgr_mouse(string_view body, parsed_input& out, bool release);
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_INPUT_PARSER_HPP__
