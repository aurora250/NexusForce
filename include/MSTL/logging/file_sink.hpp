#ifndef MSTL_LOGGING_FILE_SINK_HPP__
#define MSTL_LOGGING_FILE_SINK_HPP__
#include "MSTL/core/async/mutex.hpp"
#include "MSTL/core/file/file.hpp"
#include "MSTL/logging/log_sink.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Logging 日志系统
 * @brief 日志记录和管理功能
 * @{
 */

/**
 * @class file_sink
 * @brief 文件日志输出目标
 *
 * 将日志写入文件，支持以下特性：
 * - 按大小轮转：当文件达到指定大小时自动创建新文件
 * - 按日期轮转：每天创建新的日志文件
 * - 文件索引：轮转时自动增加文件索引
 *
 * 文件名格式：
 * - 基础文件名
 * - 如果启用日期轮转：添加日期后缀
 * - 如果启用大小轮转：添加文件索引
 */
class MSTL_API file_sink final : public log_sink {
private:
    file file_;                     ///< 当前日志文件
    mutex mutex_;                   ///< 互斥锁，保证线程安全
    path base_filename_;            ///< 基础文件名
    string current_date_;           ///< 当前日期
    size_t max_file_size_;          ///< 文件大小上限（字节）
    size_t current_size_;           ///< 当前文件大小
    int file_index_;                ///< 当前文件索引
    bool enable_date_rotation_;     ///< 是否启用日期轮转

    /**
     * @brief 打开新的日志文件
     *
     * 根据当前配置（日期、索引）构建文件名并打开文件。
     * 如果文件已存在，以追加模式打开。
     *
     * @throws file_exception 文件打开失败时抛出
     */
    void open_new_file();

    /**
     * @brief 按大小轮转日志文件
     *
     * 关闭当前文件，增加索引，创建新的日志文件。
     */
    void rotate_file();

    /**
     * @brief 按日期轮转日志文件
     * @param today 当前日期字符串
     *
     * 当日期变化时调用，重置索引，创建新日期的日志文件。
     */
    void rotate_by_date(string today);

public:
 /**
  * @brief 构造函数
  * @param filename 基础文件名
  * @param max_file_size 文件大小上限（默认10MB）
  * @param enable_date_rotation 是否启用日期轮转（默认true）
  *
  * 创建文件日志输出目标，并立即打开日志文件。
  *
  * @throws file_exception 文件创建失败时抛出
  */
    explicit file_sink(
        path filename,
        size_t max_file_size = 10 * 1024 * 1024,
        bool enable_date_rotation = true);

    /**
     * @brief 写入日志事件
     * @param event 要写入的日志事件
     *
     * 将格式化后的日志写入文件。如果文件大小超过限制或日期变化，
     * 会自动进行轮转。线程安全。
     */
    void log(const log_event& event) override;

    /**
     * @brief 刷新文件缓冲区
     *
     * 确保所有日志都被写入磁盘。线程安全。
     */
    void flush() override;
};

/** @} */ // Logging

MSTL_END_NAMESPACE__
#endif // MSTL_LOGGING_FILE_SINK_HPP__
