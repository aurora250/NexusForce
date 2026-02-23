#ifndef MSTL_PLUGIN_PLUGIN_ENTRY_HPP__
#define MSTL_PLUGIN_PLUGIN_ENTRY_HPP__

/**
 * @file plugin_entry.hpp
 * @brief 插件入口点定义
 *
 * 此文件定义了插件必须实现的入口函数。
 * 插件需要导出 create_plugin 和 destroy_plugin 函数，
 * 供插件管理器加载和卸载插件。
 */

#include "MSTL/plugin/iplugin.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Plugin 插件
 * @brief 动态加载插件管理
 * @{
 */

/**
 * @def MSTL_PLUGIN_CREATE_FUNC
 * @brief 插件创建函数名称
 *
 * 插件必须导出的创建函数名，用于实例化插件对象。
 */
#define MSTL_PLUGIN_CREATE_FUNC "create_plugin"

/**
 * @def MSTL_PLUGIN_DESTROY_FUNC
 * @brief 插件销毁函数名称
 *
 * 插件必须导出的销毁函数名，用于释放插件对象。
 */
#define MSTL_PLUGIN_DESTROY_FUNC "destroy_plugin"

extern "C" {
    /**
     * @brief 创建插件实例
     * @return 指向新创建的插件对象的指针
     *
     * 此函数必须由插件实现并导出，用于创建插件实例。
     * 返回的指针将由 destroy_plugin 函数销毁。
     */
    iplugin* create_plugin();

    /**
     * @brief 销毁插件实例
     * @param p 要销毁的插件对象指针
     *
     * 此函数必须由插件实现并导出，用于销毁通过 create_plugin
     * 创建的插件对象。释放所有相关资源。
     */
    void destroy_plugin(iplugin* p);
}

/** @} */ // Plugin

MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_PLUGIN_ENTRY_HPP__
