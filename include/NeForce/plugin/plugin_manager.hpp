#ifndef NEFORCE_PLUGIN_PLUGIN_MANAGER_HPP__
#define NEFORCE_PLUGIN_PLUGIN_MANAGER_HPP__

/**
 * @file plugin_manager.hpp
 * @brief 插件管理器
 *
 * 此文件提供了插件管理器的实现，负责插件的加载、卸载、
 * 生命周期管理和查询功能。
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/system/dynamic_library.hpp"
#include "NeForce/plugin/plugin_entry.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Plugin 插件
 * @brief 动态加载插件管理
 * @{
 */

/**
 * @class plugin_manager
 * @brief 插件管理器单例类
 *
 * 负责插件的加载、卸载和生命周期管理。
 * 维护插件实例、动态库句柄和插件名称之间的映射关系。
 */
class NEFORCE_API plugin_manager {
public:
    using library_ptr = unique_ptr<dynamic_library>; ///< 动态库指针类型

private:
    mutable mutex mutex_;                             ///< 互斥锁
    unordered_map<string, plugin_ptr> plugins_;       ///< 插件名称到实例的映射
    unordered_map<string, library_ptr> libraries_;    ///< 库路径到动态库的映射
    unordered_map<string, string> plugin_to_library_; ///< 插件名称到库路径的映射

    /**
     * @brief 私有构造函数
     */
    plugin_manager() = default;

public:
    plugin_manager(const plugin_manager&) = delete;
    plugin_manager& operator=(const plugin_manager&) = delete;
    plugin_manager(plugin_manager&&) = delete;
    plugin_manager& operator=(plugin_manager&&) = delete;

    /**
     * @brief 析构函数，关闭所有插件
     */
    ~plugin_manager();

    /**
     * @brief 获取单例实例
     * @return 插件管理器实例引用
     */
    static plugin_manager& instance() {
        static plugin_manager manager;
        return manager;
    }

    /**
     * @brief 从目录加载所有插件
     * @param pth 插件目录路径
     * @return 成功加载的插件数量
     * @throws value_exception 目录无效时抛出
     * @throws system_exception 插件已加载或加载失败时抛出
     *
     * 加载指定目录中所有符合条件的动态库文件。
     */
    size_t load_plugins(const string& pth);

    /**
     * @brief 加载单个插件
     * @param pth 插件库文件路径
     * @throws system_exception 插件已加载或加载失败时抛出
     *
     * 加载指定的动态库，获取插件的创建和销毁函数，
     * 创建插件实例并注册到管理器。
     */
    void load_plugin(string_view pth);

    /**
     * @brief 卸载插件
     * @param name 插件名称
     * @return 是否成功卸载
     *
     * 关闭指定的插件，释放相关资源。
     */
    bool unload_plugin(const string& name);

    /**
     * @brief 获取插件实例
     * @param name 插件名称
     * @return 插件指针，如果插件不存在返回nullptr
     */
    NEFORCE_NODISCARD iplugin* get_plugin(const string& name);

    /**
     * @brief 获取所有已加载插件名称
     * @return 插件名称列表
     */
    NEFORCE_NODISCARD vector<string> list_plugins() const;

    /**
     * @brief 初始化所有插件
     *
     * 调用所有已加载插件的 initialize 方法。
     */
    void initialize_all();

    /**
     * @brief 关闭所有插件
     *
     * 调用所有已加载插件的 shutdown 方法，
     * 并清理所有资源。
     */
    void shutdown_all() noexcept;
};

/** @} */ // Plugin

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_PLUGIN_PLUGIN_MANAGER_HPP__
