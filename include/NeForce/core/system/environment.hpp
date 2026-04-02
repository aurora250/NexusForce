#ifndef NEFORCE_CORE_SYSTEM_ENV_VARIABLE_HPP__
#define NEFORCE_CORE_SYSTEM_ENV_VARIABLE_HPP__

/**
 * @file environment.hpp
 * @brief 环境变量管理工具
 *
 * 此文件提供了跨平台的环境变量访问和管理功能。
 * 支持获取、设置、删除环境变量，以及获取常用系统路径。
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup EnvironmentVariables 环境变量
 * @brief 环境变量访问和管理工具
 * @{
 */

/**
 * @class environment
 * @brief 环境变量管理类
 *
 * 提供静态方法用于访问和修改环境变量，所有操作都是线程安全的。
 */
struct NEFORCE_API environment {
    /**
     * @brief 路径分隔符
     */
    static constexpr char delimiter =
#ifdef NEFORCE_PLATFORM_WINDOWS
            ';';
#else
            ':';
#endif

    /**
     * @brief 获取环境变量值
     * @param name 环境变量名
     * @return 环境变量值，不存在返回空字符串
     */
    static string get(const string& name);

    /**
     * @brief 设置环境变量
     * @param name 环境变量名
     * @param value 环境变量值
     * @param overwrite 是否覆盖已有值
     * @return 是否设置成功
     */
    static bool set(const string& name, const string& value, bool overwrite = true);

    /**
     * @brief 删除环境变量
     * @param name 环境变量名
     * @return 是否删除成功
     */
    static bool unset(const string& name);

    /**
     * @brief 检查环境变量是否存在
     * @param name 环境变量名
     * @return 是否存在
     */
    static bool exists(const string& name);

    /**
     * @brief 获取所有环境变量
     * @return 环境变量名到值的映射
     */
    static unordered_map<string, string> all_envs();

    /**
     * @brief 获取PATH环境变量的路径列表
     * @return 路径列表
     */
    static vector<string> path_list();

    /**
     * @brief 向PATH环境变量添加路径
     * @param path 要添加的路径
     * @param position 添加位置（0：开头，其他：末尾）
     * @return 是否添加成功
     */
    static bool add_to_path(const string& path, int position = 1);

    /**
     * @brief 获取当前工作目录
     * @return 当前目录路径
     * @throws system_exception 获取失败时抛出
     */
    static string current_directory();

    /**
     * @brief 获取当前用户名
     * @return 用户名
     */
    static string current_user();

    /**
     * @brief 获取临时目录路径
     * @return 临时目录路径
     *
     * 按照优先级尝试：
     * - Windows: TMP, TEMP, USERPROFILE
     * - Linux: TMPDIR, TEMP, TMP, /tmp
     */
    static string temp_directory();

    /**
     * @brief 获取用户主目录路径
     * @return 主目录路径
     *
     * 按照优先级尝试：
     * - Windows: USERPROFILE, HOMEDRIVE+HOMEPATH
     * - Linux: HOME
     */
    static string home_directory();
};

/** @} */ // EnvironmentVariables

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SYSTEM_ENV_VARIABLE_HPP__
