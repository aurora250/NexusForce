#ifndef NEFORCE_TUI_DOM_THEME_ROLE_HPP__
#define NEFORCE_TUI_DOM_THEME_ROLE_HPP__

/**
 * @file theme_role.hpp
 * @brief 主题颜色角色与内置调色板标识
 *
 * 只依赖基础类型，供 style.hpp 与 theme.hpp 共用，避免循环包含。
 * 角色名与用户主题文件中的 JSON 键名一一对应。
 */

#include "NeForce/core/string/string_view.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 颜色角色
 *
 * 分组：品牌/焦点、文本/中性、表面、状态、diff、工具点、语法、图表、chrome、子代理。
 */
enum class theme_role : uint8_t {
    auto_accept,                   ///< 角色 `autoAccept`
    bash_border,                   ///< 角色 `bashBorder`
    accent,                        ///< 角色 `accent`
    tool_name_mutate,              ///< 角色 `toolNameMutate`
    tool_name_exec,                ///< 角色 `toolNameExec`
    accent_shimmer,                ///< 角色 `accentShimmer`
    activity,                      ///< 角色 `activity`
    activity_shimmer,              ///< 角色 `activityShimmer`
    permission,                    ///< 角色 `permission`
    permission_shimmer,            ///< 角色 `permissionShimmer`
    plan_mode,                     ///< 角色 `planMode`
    ide,                           ///< 角色 `ide`
    prompt_border,                 ///< 角色 `promptBorder`
    prompt_border_shimmer,         ///< 角色 `promptBorderShimmer`
    text,                          ///< 角色 `text`
    inverse_text,                  ///< 角色 `inverseText`
    inactive,                      ///< 角色 `inactive`
    inactive_shimmer,              ///< 角色 `inactiveShimmer`
    subtle,                        ///< 角色 `subtle`
    suggestion,                    ///< 角色 `suggestion`
    remember,                      ///< 角色 `remember`
    background,                    ///< 角色 `background`
    success,                       ///< 角色 `success`
    error,                         ///< 角色 `error`
    warning,                       ///< 角色 `warning`
    merged,                        ///< 角色 `merged`
    warning_shimmer,               ///< 角色 `warningShimmer`
    diff_added,                    ///< 角色 `diffAdded`
    diff_removed,                  ///< 角色 `diffRemoved`
    diff_added_dimmed,             ///< 角色 `diffAddedDimmed`
    diff_removed_dimmed,           ///< 角色 `diffRemovedDimmed`
    diff_added_word,               ///< 角色 `diffAddedWord`
    diff_removed_word,             ///< 角色 `diffRemovedWord`
    tool_card_background,          ///< 角色 `toolCardBackground`
    tool_card_background_dim,      ///< 角色 `toolCardBackgroundDim`
    tool_dot_exec,                 ///< 角色 `toolDotExec`
    tool_dot_read,                 ///< 角色 `toolDotRead`
    tool_dot_write,                ///< 角色 `toolDotWrite`
    tool_dot_web,                  ///< 角色 `toolDotWeb`
    tool_dot_task,                 ///< 角色 `toolDotTask`
    syntax_keyword,                ///< 角色 `syntaxKeyword`
    syntax_string,                 ///< 角色 `syntaxString`
    syntax_comment,                ///< 角色 `syntaxComment`
    syntax_number,                 ///< 角色 `syntaxNumber`
    syntax_function,               ///< 角色 `syntaxFunction`
    syntax_type,                   ///< 角色 `syntaxType`
    syntax_variable,               ///< 角色 `syntaxVariable`
    syntax_operator,               ///< 角色 `syntaxOperator`
    syntax_punctuation,            ///< 角色 `syntaxPunctuation`
    syntax_constant,               ///< 角色 `syntaxConstant`
    professional_blue,             ///< 角色 `professionalBlue`
    chrome_yellow,                 ///< 角色 `chromeYellow`
    context_bar_system,            ///< 角色 `contextBarSystem`
    context_bar_prompt,            ///< 角色 `contextBarPrompt`
    context_bar_assistant,         ///< 角色 `contextBarAssistant`
    context_bar_thinking,          ///< 角色 `contextBarThinking`
    context_bar_tools,             ///< 角色 `contextBarTools`
    ignition,                      ///< 角色 `ignition`
    ignition_dim,                  ///< 角色 `ignitionDim`
    cursor,                        ///< 角色 `cursor`
    mascot_body,                   ///< 角色 `mascotBody`
    input_background,              ///< 角色 `inputBackground`
    user_message_background,       ///< 角色 `userMessageBackground`
    user_message_background_hover, ///< 角色 `userMessageBackgroundHover`
    message_actions_background,    ///< 角色 `messageActionsBackground`
    selection_bg,                  ///< 角色 `selectionBg`
    bash_message_background_color, ///< 角色 `bashMessageBackgroundColor`
    memory_background_color,       ///< 角色 `memoryBackgroundColor`
    rate_limit_fill,               ///< 角色 `rate_limit_fill`
    rate_limit_empty,              ///< 角色 `rate_limit_empty`
    fast_mode,                     ///< 角色 `fastMode`
    fast_mode_shimmer,             ///< 角色 `fastModeShimmer`
    user_prompt_label,             ///< 角色 `userPromptLabel`
    subagent_bullet,               ///< 角色 `subagentBullet`
    subagent_description,          ///< 角色 `subagentDescription`
    subagent_model,                ///< 角色 `subagentModel`
    subagent_elapsed,              ///< 角色 `subagentElapsed`
    subagent_tool_name,            ///< 角色 `subagentToolName`
    subagent_status_running,       ///< 角色 `subagentStatusRunning`
    subagent_status_completed,     ///< 角色 `subagentStatusCompleted`
    subagent_status_failed,        ///< 角色 `subagentStatusFailed`
};

/**
 * @brief 角色总数
 */
constexpr size_t theme_role_count = 81;

/**
 * @brief 内置调色板标识
 */
enum class theme_palette_id : uint8_t {
    legacy_dark,  ///< 旧版暗色
    legacy_light, ///< 旧版亮色
    dark,         ///< 雾蓝·暗
    light,        ///< 雾蓝·亮
    dark_ansi,    ///< 雾蓝·16 色回退
    pink_night,   ///< 樱·夜
    pink_day,     ///< 樱·日
    pink_ansi,    ///< 樱·16 色回退
    count_,       ///< 调色板数量
};

/**
 * @brief 角色对应的 JSON 键名
 * @param role 角色
 * @return 键名（camelCase），越界时返回空串
 */
NEFORCE_API string_view theme_role_name(theme_role role) noexcept;

/**
 * @brief 按 JSON 键名解析角色
 * @param name 键名（大小写不敏感）
 * @return 角色，未知时返回 none
 */
NEFORCE_API optional<theme_role> parse_theme_role(string_view name) noexcept;

/**
 * @brief 调色板名称
 * @param id 调色板标识
 * @return 名称（如 `dark`、`pink-night`），越界时返回空串
 */
NEFORCE_API string_view palette_name(theme_palette_id id) noexcept;

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_DOM_THEME_ROLE_HPP__
