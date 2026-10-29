#ifndef NEFORCE_CORE_FILE_FILE_WATCHER_HPP__
#define NEFORCE_CORE_FILE_FILE_WATCHER_HPP__

/**
 * @file file_watcher.hpp
 * @brief 文件系统监视器
 *
 * 此文件提供了文件系统事件监视功能。
 */

#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class file_watcher
 * @brief 文件系统监视器
 *
 * 监视指定目录中的文件变化事件，支持递归监视子目录。
 * 当文件被创建、删除、修改、重命名或访问时，触发回调函数。
 */
class NEFORCE_API file_watcher {
public:
    /**
     * @enum event
     * @brief 文件监视事件枚举
     */
    enum class event {
        created = 0x01,  ///< 文件创建事件
        deleted = 0x02,  ///< 文件删除事件
        modified = 0x04, ///< 文件修改事件
        renamed = 0x08,  ///< 文件重命名事件
        /**
         * @brief 文件访问事件
         * @warning FreeBSD 的 kqueue 后端不支持此事件
         * @warning Windows 上受 NTFS 最后访问时间策略与延迟聚合影响，可能长时间不触发
         */
        accessed = 0x10,
        all = created | deleted | modified | renamed | accessed ///< 所有事件
    };

private:
    using callback_t = function<void(const path&, event)>; ///< 事件回调类型

    struct impl;
    unique_ptr<impl> impl_;

public:
    /**
     * @brief 构造函数
     * @param watch_path 要监视的目录路径
     * @param recursive 是否递归监视子目录
     * @throws system_exception 路径不存在或不是目录时抛出
     */
    explicit file_watcher(path watch_path, bool recursive = false);

    /**
     * @brief 析构函数
     *
     * 停止监视并清理资源。
     */
    ~file_watcher();

    file_watcher(const file_watcher&) = delete;
    file_watcher& operator=(const file_watcher&) = delete;
    file_watcher(file_watcher&&) = delete;
    file_watcher& operator=(file_watcher&&) = delete;

    /**
     * @brief 开始监视
     * @param callback 事件回调函数
     * @param events 要监视的事件类型
     * @return 是否成功启动
     *
     * 启动一个后台线程开始监视文件系统事件。
     * 当指定的事件发生时，回调函数会被调用，参数为文件路径和事件类型。
     *
     * @note 重启 start() 存在短暂的事件窗口，重启期间发生的事件不会被补发。
     */
    bool start(callback_t callback, file_watcher::event events = file_watcher::event::all);

    /**
     * @brief 停止监视
     *
     * 停止监视线程并清理相关资源。
     */
    void stop();

    /**
     * @brief 获取监视的目录路径
     * @return 目录路径
     */
    NEFORCE_NODISCARD const path& watch_path() const noexcept;

    /**
     * @brief 获取当前监视的事件类型
     * @return 事件类型
     */
    NEFORCE_NODISCARD file_watcher::event current_events() const noexcept;

    /**
     * @brief 检查是否正在监视
     * @return 是否正在监视
     */
    NEFORCE_NODISCARD bool is_watching() const noexcept;

    /**
     * @brief 检查是否递归监视子目录
     * @return 是否递归
     */
    NEFORCE_NODISCARD bool is_recursive() const noexcept;

    /**
     * @brief 更新监视的事件类型
     * @param events 新的事件类型
     * @return 是否成功更新
     *
     * 如果正在监视中，会重启监视器以应用新的事件类型。
     */
    bool update_watch(file_watcher::event events);

    /**
     * @brief 更新递归设置
     * @param recursive 新的递归设置
     * @return 是否成功更新
     *
     * 如果正在监视中，会重启监视器以应用新的递归设置。
     */
    bool update_recursive(bool recursive);
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_WATCHER_HPP__
