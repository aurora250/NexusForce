#ifndef NEFORCE_TUI_DOM_THEME_HPP__
#define NEFORCE_TUI_DOM_THEME_HPP__

/**
 * @file theme.hpp
 * @brief 主题目录、调色板与颜色工具
 *
 * 主题把界面用到的每种颜色收敛成具名角色，调色板给出一整套角色取值，
 * 主题目录负责内置调色板、用户 JSON 主题与"自动明暗"的解析。
 *
 * @code
 * theme t = make_palette(theme_palette_id::dark);
 * const color accent = t.get(theme_role::accent);
 * @endcode
 */

#include "NeForce/tui/dom/style.hpp"
#include "NeForce/tui/dom/theme_role.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 按调色板构造主题
 * @param id 调色板标识
 * @return 主题
 */
NEFORCE_API theme make_palette(theme_palette_id id);

/**
 * @brief 按名称构造内置调色板
 * @param name 名称（`dark`、`light`、`dark-ansi`、`pink-night`、`pink-day`、`pink-ansi`）
 * @return 主题，未知名称返回 none
 */
NEFORCE_API optional<theme> make_palette(string_view name);

/**
 * @brief 向白色插值
 * @param c 基色
 * @param amount 插值比例（0-1）
 * @return 提亮后的颜色
 */
NEFORCE_API _NEFORCE color lighten(_NEFORCE color c, double amount) noexcept;

/**
 * @brief 向黑色插值
 * @param c 基色
 * @param amount 插值比例（0-1）
 * @return 压暗后的颜色
 */
NEFORCE_API _NEFORCE color darken(_NEFORCE color c, double amount) noexcept;

/**
 * @brief 解析颜色文本
 * @param text 支持 `#rgb`、`#rrggbb`、`#rrggbbaa`、`rgb(r,g,b)`、
 *             `ansi256(n)` 与 16 个 `ansi:name`
 * @return 颜色，格式无效时返回 none
 */
NEFORCE_API optional<_NEFORCE color> parse_color(string_view text) noexcept;

/**
 * @brief 主题目录
 *
 * 汇总内置调色板与用户主题，并解析 `auto` 伪主题，
 * 依据终端背景明暗在 `light` 与 `dark` 之间选择。
 */
class NEFORCE_API theme_catalog {
public:
    /**
     * @brief 获取全局实例
     * @return 目录实例
     */
    static theme_catalog& instance();

    /**
     * @brief 列出可用主题名
     * @return 名称列表
     */
    NEFORCE_NODISCARD vector<string> names() const;

    /**
     * @brief 按名查找主题
     * @param name 主题名（大小写不敏感）
     * @return 主题，未找到返回 none
     */
    NEFORCE_NODISCARD optional<theme> find(string_view name) const;

    /**
     * @brief 解析主题
     * @param name 主题名
     * @return 主题
     */
    NEFORCE_NODISCARD theme resolve(string_view name) const;

    /**
     * @brief 加载目录下的用户主题
     * @param directory 目录路径
     * @return 至少成功加载一个返回 true
     */
    bool load_user_themes(string_view directory);

    /**
     * @brief 从 JSON 文档注册用户主题
     * @param json 主题文档
     * @return 成功返回 true
     *
     * 文档格式：
     * @code{.json}
     * { "name": "sakura", "base": "dark", "colors": { "accent": "#FF9EC7" } }
     * @endcode
     *
     * `base` 必须是内置调色板名；未知颜色键与非法颜色值被忽略。
     */
    bool register_theme(string_view json);

    /**
     * @brief 清除全部用户主题
     */
    void clear_user_themes();

    /**
     * @brief 设置终端背景是否偏亮
     * @param light 浅色背景传 true
     */
    void set_terminal_light(bool light) noexcept;

    /**
     * @brief 终端背景是否偏亮
     * @return 偏亮返回 true
     */
    NEFORCE_NODISCARD bool terminal_light() const noexcept { return terminal_light_; }

    /**
     * @brief 当前用户主题数量
     * @return 数量
     */
    NEFORCE_NODISCARD size_t user_theme_count() const noexcept { return user_themes_.size(); }

private:
    theme_catalog() = default;

    vector<pair<string, theme>> user_themes_; ///< 用户主题
    bool terminal_light_ = false;             ///< 终端背景明暗
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_THEME_HPP__
