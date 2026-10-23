#ifndef NEFORCE_SRC_TUI_DOM_THEME_INTERNAL_HPP__
#define NEFORCE_SRC_TUI_DOM_THEME_INTERNAL_HPP__
#include "NeForce/tui/dom/theme_role.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

extern string_view const role_names[theme_role_count];
extern const unsigned int palette_colors[static_cast<size_t>(theme_palette_id::count_)][theme_role_count];

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SRC_TUI_DOM_THEME_INTERNAL_HPP__
