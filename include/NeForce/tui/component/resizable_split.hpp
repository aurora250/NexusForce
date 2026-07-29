#ifndef NEFORCE_TUI_COMPONENT_RESIZABLE_SPLIT_HPP__
#define NEFORCE_TUI_COMPONENT_RESIZABLE_SPLIT_HPP__

/**
 * @file resizable_split.hpp
 * @brief 可调节分割面板组件
 *
 * 提供 resizable_split 组件，可拖动分隔线调整两个面板的尺寸。
 */

#include "NeForce/tui/component/component.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__
NEFORCE_BEGIN_COMPONENTS__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @addtogroup Components 用户组件
 * @{
 */

/**
 * @brief 创建垂直分割面板（主面板在左）
 * @param main 主面板组件
 * @param back 副面板组件
 * @param main_size 主面板大小
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API resizable_split_left(unique_ptr<component_base> main,
                                                            unique_ptr<component_base> back, int* main_size);

/**
 * @brief 创建垂直分割面板（主面板在右）
 * @param main 主面板组件
 * @param back 副面板组件
 * @param main_size 主面板大小
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API resizable_split_right(unique_ptr<component_base> main,
                                                             unique_ptr<component_base> back, int* main_size);

/**
 * @brief 创建水平分割面板（主面板在上）
 * @param main 主面板组件
 * @param back 副面板组件
 * @param main_size 主面板大小
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API resizable_split_top(unique_ptr<component_base> main,
                                                           unique_ptr<component_base> back, int* main_size);

/**
 * @brief 创建水平分割面板（主面板在下）
 * @param main 主面板组件
 * @param back 副面板组件
 * @param main_size 主面板大小
 * @return 组件指针
 */
unique_ptr<component_base> NEFORCE_API resizable_split_bottom(unique_ptr<component_base> main,
                                                              unique_ptr<component_base> back, int* main_size);

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_RESIZABLE_SPLIT_HPP__
