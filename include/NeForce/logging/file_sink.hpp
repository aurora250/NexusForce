#ifndef NEFORCE_LOGGING_FILE_SINK_HPP__
#define NEFORCE_LOGGING_FILE_SINK_HPP__

/**
 * @file file_sink.hpp
 * @brief 日志文件输出目标
 *
 * 支持：
 * - 按大小轮转
 * - 按日期轮转
 * - max_files 保留策略
 * - 自动创建父目录
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/file/file.hpp"
#include "NeForce/logging/log_sink.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Logging 日志系统
 * @{
 */

/**
 * @class file_sink
 * @brief 文件日志输出目标
 *
 * 文件名格式：basename[.date][.index]
 * 例如：app.log.2026-05-28.3
 */
class NEFORCE_API file_sink final : public log_sink {
private:
    file file_;                 ///< 底层文件对象
    mutex mutex_;               ///< 写文件互斥锁
    path base_filename_;        ///< 基础文件名
    string current_date_;       ///< 当前日期字符串
    size_t max_file_size_;      ///< 单文件最大字节数
    size_t current_size_;       ///< 当前文件已写入字节数
    int file_index_;            ///< 当前轮转索引
    bool enable_date_rotation_; ///< 是否启用日期轮转
    size_t max_files_;          ///< 最大保留文件数，0=无限制

    void open_new_file();
    void cleanup_old_files();

    void rotate_file();
    void rotate_by_date(string today);

public:
    /**
     * @brief 构造文件输出目标
     * @param filename 基础文件名（相对或绝对路径）
     * @param max_file_size 文件大小上限（默认 10MB）
     * @param enable_date_rotation 是否启用日期轮转（默认 true）
     * @param max_files 最大保留文件数（默认 0=无限制）
     */
    explicit file_sink(path filename, size_t max_file_size = static_cast<size_t>(10 * 1024 * 1024),
                       bool enable_date_rotation = true, size_t max_files = 0);

    /**
     * @brief 输出日志到文件
     * @param event 日志事件
     *
     * 写入前检查日期轮转和大小轮转条件。
     */
    void log(const log_event& event) override;

    /** @brief 刷新文件缓冲区到磁盘 */
    void flush() override;
};

/** @} */ // Logging

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_LOGGING_FILE_SINK_HPP__
