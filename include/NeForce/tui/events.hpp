#ifndef NEFORCE_TUI_EVENTS_HPP__
#define NEFORCE_TUI_EVENTS_HPP__

/**
 * @file events.hpp
 * @brief TUI输入事件类型定义
 *
 * 定义了键盘、鼠标和终端尺寸变化等输入事件类型。
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 修饰键位掩码
 */
enum class Modifier : uint8_t {
    None = 0,  ///< 无修饰键
    Ctrl = 1,  ///< Ctrl 键
    Alt = 2,   ///< Alt 键
    Shift = 4, ///< Shift 键
};

constexpr Modifier operator|(Modifier a, Modifier b) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<Modifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr Modifier operator&(Modifier a, Modifier b) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<Modifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}


/**
 * @brief 键盘事件
 *
 * 表示一次键盘输入，可能是特殊键或可打印字符。
 * 特殊键由 Key 枚举标识，可打印字符通过 ch 字段传递 Unicode 码点。
 */
struct KeyEvent {
    /**
     * @brief 特殊按键枚举
     */
    enum class Key {
        Unknown,   ///< 未知按键
        Up,        ///< 上方向键
        Down,      ///< 下方向键
        Left,      ///< 左方向键
        Right,     ///< 右方向键
        Home,      ///< Home 键
        End,       ///< End 键
        PageUp,    ///< Page Up 键
        PageDown,  ///< Page Down 键
        Insert,    ///< Insert 键
        Delete,    ///< Delete 键
        Enter,     ///< 回车键
        Tab,       ///< Tab 键
        Escape,    ///< Escape 键
        Backspace, ///< 退格键
        F1,        ///< F1 功能键
        F2,        ///< F2 功能键
        F3,        ///< F3 功能键
        F4,        ///< F4 功能键
        F5,        ///< F5 功能键
        F6,        ///< F6 功能键
        F7,        ///< F7 功能键
        F8,        ///< F8 功能键
        F9,        ///< F9 功能键
        F10,       ///< F10 功能键
        F11,       ///< F11 功能键
        F12,       ///< F12 功能键
        Printable, ///< 可打印字符，查看 ch 字段
    };

    Key key = Key::Unknown;         ///< 按键标识
    char32_t ch = 0;                ///< 可打印字符的 Unicode 码点
    Modifier mods = Modifier::None; ///< 当前按下的修饰键

    /**
     * @brief 检查是否为可打印字符
     * @return 是否为可打印字符
     */
    NEFORCE_NODISCARD bool isPrintable() const noexcept { return key == Key::Printable; }
};

/**
 * @brief 鼠标按钮枚举
 */
enum class MouseButton : uint8_t {
    Left,      ///< 左键
    Middle,    ///< 中键
    Right,     ///< 右键
    WheelUp,   ///< 滚轮上滚
    WheelDown, ///< 滚轮下滚
    None,      ///< 无按钮
};

/**
 * @brief 鼠标动作枚举
 */
enum class MouseAction : uint8_t {
    Press,   ///< 按下
    Release, ///< 释放
    Move,    ///< 移动
    Wheel,   ///< 滚轮
};

/**
 * @brief 鼠标事件
 *
 * 表示一次鼠标操作，包含坐标、按钮和动作类型。
 * 坐标以字符行列计，左上角为 (0, 0)。
 */
struct MouseEvent {
    int x = 0;                              ///< 列坐标（0起始）
    int y = 0;                              ///< 行坐标（0起始）
    MouseButton button = MouseButton::None; ///< 触发按钮
    MouseAction action = MouseAction::Move; ///< 动作类型
    Modifier mods = Modifier::None;         ///< 当前按下的修饰键
};

/**
 * @brief 终端尺寸变化事件
 */
struct ResizeEvent {
    int width;  ///< 新的终端宽度
    int height; ///< 新的终端高度
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_EVENTS_HPP__
