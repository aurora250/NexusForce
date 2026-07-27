#ifndef NEFORCE_TUI_EVENTS_HPP__
#define NEFORCE_TUI_EVENTS_HPP__

/**
 * @file events.hpp
 * @brief TUI输入事件类型定义
 *
 * 定义了键盘、鼠标和终端尺寸变化等输入事件类型。
 */

#include "NeForce/core/string/codepoint.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 修饰键位掩码
 */
enum class key_modifier : uint8_t {
    none = 0,  ///< 无修饰键
    ctrl = 1,  ///< Ctrl 键
    alt = 2,   ///< Alt 键
    shift = 4, ///< Shift 键
};

constexpr key_modifier operator|(key_modifier a, key_modifier b) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<key_modifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr key_modifier operator&(key_modifier a, key_modifier b) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<key_modifier>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}


/**
 * @brief 键盘事件
 *
 * 表示一次键盘输入，可能是特殊键或可打印字符。
 * 特殊键由 Key 枚举标识，可打印字符通过 ch 字段传递 Unicode 码点。
 */
struct key_event {
    /**
     * @brief 特殊按键枚举
     */
    enum class type {
        unknown,     ///< 未知按键
        up,          ///< 上方向键
        down,        ///< 下方向键
        left,        ///< 左方向键
        right,       ///< 右方向键
        home,        ///< Home 键
        end,         ///< End 键
        page_up,     ///< Page Up 键
        page_down,   ///< Page Down 键
        insert,      ///< Insert 键
        delete_,     ///< Delete 键
        enter,       ///< 回车键
        tab,         ///< Tab 键
        tab_reverse, ///< Shift+Tab 键
        escape,      ///< Escape 键
        backspace,   ///< 退格键
        F1,          ///< F1 功能键
        F2,          ///< F2 功能键
        F3,          ///< F3 功能键
        F4,          ///< F4 功能键
        F5,          ///< F5 功能键
        F6,          ///< F6 功能键
        F7,          ///< F7 功能键
        F8,          ///< F8 功能键
        F9,          ///< F9 功能键
        F10,         ///< F10 功能键
        F11,         ///< F11 功能键
        F12,         ///< F12 功能键
        F13,         ///< F13 功能键
        F14,         ///< F14 功能键
        F15,         ///< F15 功能键
        F16,         ///< F16 功能键
        F17,         ///< F17 功能键
        F18,         ///< F18 功能键
        F19,         ///< F19 功能键
        F20,         ///< F20 功能键
        F21,         ///< F21 功能键
        F22,         ///< F22 功能键
        F23,         ///< F23 功能键
        F24,         ///< F24 功能键
        printable,   ///< 可打印字符，查看 ch 字段
    };

    type key = type::unknown;               ///< 按键标识
    codepoint cp;                           ///< 可打印字符的码点
    key_modifier mods = key_modifier::none; ///< 当前按下的修饰键

    /**
     * @brief 检查是否为可打印字符
     * @return 是否为可打印字符
     */
    NEFORCE_NODISCARD bool is_printable() const noexcept { return key == type::printable; }
};

/**
 * @brief 鼠标按钮枚举
 */
enum class mouse_button : uint8_t {
    left,      ///< 左键
    middle,    ///< 中键
    right,     ///< 右键
    wheelup,   ///< 滚轮上滚
    wheeldown, ///< 滚轮下滚
    none,      ///< 无按钮
};

/**
 * @brief 鼠标动作枚举
 */
enum class mouse_action : uint8_t {
    press,   ///< 按下
    release, ///< 释放
    move,    ///< 移动
    wheel,   ///< 滚轮
};

/**
 * @brief 鼠标事件
 *
 * 表示一次鼠标操作，包含坐标、按钮和动作类型。
 * 坐标以字符行列计，左上角为 (0, 0)。
 */
struct mouse_event {
    int x = 0;                                ///< 列坐标（0起始）
    int y = 0;                                ///< 行坐标（0起始）
    mouse_button button = mouse_button::none; ///< 触发按钮
    mouse_action action = mouse_action::move; ///< 动作类型
    key_modifier mods = key_modifier::none;   ///< 当前按下的修饰键
};

/**
 * @brief 终端尺寸变化事件
 */
struct resize_event {
    int width;  ///< 新的终端宽度
    int height; ///< 新的终端高度
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_EVENTS_HPP__
