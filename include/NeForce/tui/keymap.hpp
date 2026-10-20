#ifndef NEFORCE_TUI_KEYMAP_HPP__
#define NEFORCE_TUI_KEYMAP_HPP__

/**
 * @file keymap.hpp
 * @brief 快捷键表
 *
 * 提供分层的快捷键注册与派发：
 * 全局快捷键先于组件树处理，组件看到的按键因此只包含"未被全局消费"的部分。
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/tui/events.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/// @cond
class keymap;
/// @endcond

/**
 * @brief 快捷键层
 */
enum class shortcut_layer : uint8_t {
    global,  ///< 全局层，最先处理
    overlay, ///< 弹层/模态层，其次处理
    focus,   ///< 焦点层，最后处理
};

/**
 * @brief 快捷键绑定
 */
struct key_binding {
    key_event::type key = key_event::type::unknown; ///< 按键；`printable` 时匹配 `character`
    key_modifier required = key_modifier::none;     ///< 必须同时按下的修饰键
    key_modifier forbidden = key_modifier::none;    ///< 必须未按下的修饰键
    char32_t character = 0;                         ///< 可打印字符（0 表示不匹配字符，只看 key）
    function<bool(const key_event&)> handler;       ///< 处理函数，返回 true 表示已消费
    string description;                             ///< 解释文案
};

/**
 * @brief 快捷键表
 *
 * @code
 * keymap map;
 * map.bind(shortcut_layer::global, {.key = key_event::type::printable, .required = key_modifier::ctrl,
 *                                  .character = U'c', .handler = [](const key_event&) { ...; return true; }});
 * if (!map.dispatch(event)) { ... 继续派发给组件树 ... }
 * @endcode
 */
class NEFORCE_API keymap {
public:
    /**
     * @brief 注册快捷键
     * @param layer 所属层
     * @param binding 绑定
     */
    void bind(shortcut_layer layer, key_binding binding);

    /**
     * @brief 注册全局快捷键
     * @param binding 绑定
     */
    void bind(key_binding binding) { bind(shortcut_layer::global, _NEFORCE move(binding)); }

    /**
     * @brief 按键匹配
     * @param binding 绑定
     * @param e 键盘事件
     * @return 是否匹配
     */
    NEFORCE_NODISCARD static bool matches(const key_binding& binding, const key_event& e) noexcept;

    /**
     * @brief 派发按键
     * @param e 键盘事件
     * @return true 表示已被某个绑定消费
     */
    bool dispatch(const key_event& e);

    /**
     * @brief 清空指定层
     * @param layer 层
     */
    void clear(shortcut_layer layer);

    /**
     * @brief 清空全部绑定
     */
    void clear();

    /**
     * @brief 指定层的绑定数量
     * @param layer 层
     * @return 数量
     */
    NEFORCE_NODISCARD size_t count(shortcut_layer layer) const noexcept;

    /**
     * @brief 全部绑定数量
     * @return 数量
     */
    NEFORCE_NODISCARD size_t count() const noexcept;

    /**
     * @brief 获取指定层的解释文案
     * @param layer 层
     * @return 文案列表（按注册顺序）
     */
    NEFORCE_NODISCARD vector<string> hints(shortcut_layer layer) const;

private:
    vector<key_binding> global_;
    vector<key_binding> overlay_;
    vector<key_binding> focus_;

    NEFORCE_NODISCARD vector<key_binding>& layer_of(shortcut_layer layer) noexcept;
    NEFORCE_NODISCARD const vector<key_binding>& layer_of(shortcut_layer layer) const noexcept;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_KEYMAP_HPP__
