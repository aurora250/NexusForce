#ifndef NEFORCE_TUI_COMPONENT_CONTAINERS_HPP__
#define NEFORCE_TUI_COMPONENT_CONTAINERS_HPP__

/**
 * @file containers.hpp
 * @brief 容器组件
 *
 * 提供容器组件。容器管理子组件列表，处理方向键导航和焦点传递。
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
 * @brief 容器组件工厂
 */
struct NEFORCE_API container {
    /**
     * @brief 垂直容器
     * @param children 子组件列表
     * @param selected 选中索引的外部指针
     * @return 组件指针
     */
    static unique_ptr<component_base> vertical(vector<unique_ptr<component_base>> children, int* selected = nullptr);

    /**
     * @brief 水平容器
     * @param children 子组件列表
     * @param selected 选中索引的外部指针
     * @return 组件指针
     */
    static unique_ptr<component_base> horizontal(vector<unique_ptr<component_base>> children, int* selected = nullptr);

    /**
     * @brief TAB 容器
     * @param children 子组件列表
     * @param selected 选中索引的外部指针
     * @return 组件指针
     */
    static unique_ptr<component_base> tab(vector<unique_ptr<component_base>> children, int* selected);

    /**
     * @brief 层叠容器
     * @param children 子组件列表
     * @param selected 选中索引的外部指针
     * @return 组件指针
     */
    static unique_ptr<component_base> stacked(vector<unique_ptr<component_base>> children, int* selected = nullptr);
};

/** @} */ // Components

/** @} */ // TUI

NEFORCE_END_COMPONENTS__
NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_COMPONENT_CONTAINERS_HPP__
