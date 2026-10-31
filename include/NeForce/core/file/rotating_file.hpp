#ifndef NEFORCE_CORE_FILE_ROTATING_FILE_HPP__
#define NEFORCE_CORE_FILE_ROTATING_FILE_HPP__

/**
 * @file rotating_file.hpp
 * @brief 按大小轮转的原始文件写入器
 *
 * 面向 NDJSON、CSV 等数据文件，不依赖日志事件或格式化器。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/file.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/string/string_view.hpp"
#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup RotatingFile 轮转文件
 * @brief 原始数据文件的大小轮转
 * @{
 */

/**
 * @class rotating_file
 * @brief 按大小轮转的原始文件写入器
 *
 * `rollover` 在轮转时把当前 `base` 改名为 `.1`，历史文件依次后移，
 * 因此 `base` 永远是当前文件；`indexed` 则保留 `base`，在新文件
 * `base.N` 中继续写入。
 *
 * @note `max_bytes` 只限制单次记录之间的边界；单条记录超过上限时仍会完整写入。
 * @note `max_files` 统计的是除当前文件以外的历史文件数。
 * @warning 该类不是线程安全的；多个线程写入同一实例需要调用方加锁。
 */
class NEFORCE_API rotating_file {
public:
    /**
     * @enum naming
     * @brief 轮转后的文件命名方式
     */
    enum class naming : uint8_t {
        rollover, ///< base -> .1 -> .2 ...，base 始终是当前文件
        indexed,  ///< 轮转后写 base.N，N 单调递增
    };

    /**
     * @struct options
     * @brief 轮转文件配置
     */
    struct options {
        uint64_t max_bytes{0};                                         ///< 单文件字节上限，0 表示不限制
        size_t max_files{0};                                           ///< 保留的历史文件数，0 表示不限制
        rotating_file::naming naming{rotating_file::naming::rollover}; ///< 轮转命名方式
        bool append{true};                                             ///< 是否追加已有当前文件
    };


    /**
     * @brief 构造轮转文件
     * @param file 当前文件路径
     * @param opts 轮转配置
     * @throws file_exception 文件无法创建或打开时抛出
     */
    rotating_file(path file, rotating_file::options opts);

    ~rotating_file() = default;

    rotating_file(const rotating_file&) = delete;
    rotating_file& operator=(const rotating_file&) = delete;

    /**
     * @brief 写入原始数据
     * @param data 数据
     * @return 成功返回 true
     *
     * 写入前按 `max_bytes` 检查是否需要轮转；空数据不触发打开或轮转。
     */
    bool write(string_view data);

    /**
     * @brief 刷新文件缓冲区
     * @return 成功返回 true
     */
    bool flush();

    /**
     * @brief 立即轮转一次
     * @return 成功返回 true
     *
     * 供日期轮转等外部策略调用。轮转后当前文件为空。
     */
    bool rotate();

    /**
     * @brief 当前文件已写入字节数
     * @return 字节数
     */
    NEFORCE_NODISCARD uint64_t size() const noexcept;

    /**
     * @brief 当前文件路径
     * @return 路径
     */
    NEFORCE_NODISCARD const path& current_path() const noexcept;

private:
    path base_path_;
    path current_path_;
    rotating_file::options options_;
    file file_;
    size_t current_index_ = 0;
    uint64_t size_ = 0;

    NEFORCE_NODISCARD path path_for(size_t index) const;
    NEFORCE_NODISCARD vector<size_t> rotated_indices() const;
    NEFORCE_NODISCARD vector<path> history_paths() const;

    bool open_at(size_t index, bool truncate);
    void cleanup_old_files();
    bool rotate_rollover();
    bool rotate_indexed();
};

/** @} */ // RotatingFile

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_ROTATING_FILE_HPP__
