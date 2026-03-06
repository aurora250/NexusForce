#ifndef NEFORCE_CORE_FILE_FILE_WATCHER_HPP__
#define NEFORCE_CORE_FILE_FILE_WATCHER_HPP__

/**
 * @file file_watcher.hpp
 * @brief 文件系统监视器
 *
 * 此文件提供了文件系统事件监视功能。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/functional/function.hpp"
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
private:
    using callback_t = function<void(const path&, FILE_WATCH_EVENT)>;  ///< 事件回调类型

    path watch_path_;                    ///< 监视的目录路径
    bool recursive_;                     ///< 是否递归监视子目录
    atomic<bool> watching_{false};       ///< 是否正在监视
    atomic<bool> stopping_{false};       ///< 是否正在停止
    callback_t callback_;                ///< 事件回调函数
    FILE_WATCH_EVENT current_events_{FILE_WATCH_EVENT::ALL};  ///< 当前监视的事件类型
    vector<char> buffer_;                ///< 事件数据缓冲区
    mutex callback_mutex_;               ///< 回调函数互斥锁

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::HANDLE dir_handle_ = INVALID_HANDLE_VALUE;      ///< 目录句柄
    ::HANDLE completion_port_ = INVALID_HANDLE_VALUE; ///< I/O完成端口
    ::OVERLAPPED overlapped_{};                        ///< 重叠I/O结构
#else
    int inotify_fd_ = -1;        ///< inotify文件描述符
    int watch_descriptor_ = -1;  ///< 监视描述符
    int event_fd_ = -1;          ///< 事件通知文件描述符
#endif

    thread watch_thread_;        ///< 监视线程

    /**
     * @brief 监视线程函数
     *
     * 阻塞等待文件系统事件，当事件发生时调用回调函数。
     */
    void watch_thread_func();

public:
    /**
     * @brief 构造函数
     * @param watch_path 要监视的目录路径
     * @param recursive 是否递归监视子目录
     * @throws system_exception 路径不存在或不是目录时抛出
     */
    explicit file_watcher(const path& watch_path, bool recursive = false);

    /**
     * @brief 析构函数
     *
     * 停止监视并清理资源。
     */
    ~file_watcher();

    file_watcher(const file_watcher&) = delete;
    file_watcher& operator =(const file_watcher&) = delete;

    /**
     * @brief 开始监视
     * @param callback 事件回调函数
     * @param events 要监视的事件类型
     * @return 是否成功启动
     *
     * 启动一个后台线程开始监视文件系统事件。
     * 当指定的事件发生时，回调函数会被调用，参数为文件路径和事件类型。
     */
    bool start(callback_t callback, FILE_WATCH_EVENT events = FILE_WATCH_EVENT::ALL);

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
    NEFORCE_NODISCARD const path& watch_path() const noexcept {
        return watch_path_;
    }

    /**
     * @brief 获取当前监视的事件类型
     * @return 事件类型
     */
    NEFORCE_NODISCARD FILE_WATCH_EVENT current_events() const noexcept {
        return current_events_;
    }

    /**
     * @brief 检查是否正在监视
     * @return 是否正在监视
     */
    NEFORCE_NODISCARD bool is_watching() const noexcept {
        return watching_.load();
    }

    /**
     * @brief 检查是否递归监视子目录
     * @return 是否递归
     */
    NEFORCE_NODISCARD bool is_recursive() const noexcept {
        return recursive_;
    }

    /**
     * @brief 更新监视的事件类型
     * @param events 新的事件类型
     * @return 是否成功更新
     *
     * 如果正在监视中，会重启监视器以应用新的事件类型。
     */
    bool update_watch(FILE_WATCH_EVENT events);

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
