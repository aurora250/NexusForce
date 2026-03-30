#ifndef NEFORCE_CORE_FILE_FILESYSTEM_HPP__
#define NEFORCE_CORE_FILE_FILESYSTEM_HPP__
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/file/file_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class filesystem
 * @brief 文件系统操作工具类
 *
 * 提供文件与目录的创建、删除、复制、移动、重命名等操作。
 * 所有方法均为静态方法，无状态。
 */
class NEFORCE_API filesystem {
public:
    filesystem() = delete;

    /**
     * @brief 创建目录（含所有父级目录）
     */
    static bool create_directories(const path& p);

    /**
     * @brief 删除文件
     */
    static bool remove(const path& p) noexcept;

    /**
     * @brief 删除空目录
     */
    static bool remove_directory(const path& p) noexcept;

    /**
     * @brief 删除目录内所有内容
     * @param recursive 是否递归删除子目录
     */
    static bool remove_all_in_directory(const path& p, bool recursive = true) noexcept;

    /**
     * @brief 删除文件或目录（含全部内容）
     */
    static bool remove_all(const path& p) noexcept;

    /**
     * @brief 复制文件
     * @param overwrite 是否覆盖已存在文件
     */
    static bool copy(const path& from, const path& to, bool overwrite = true);

    /**
     * @brief 复制目录
     */
    static bool copy_directory(const path& src, const path& dest, bool overwrite = true);

    /**
     * @brief 移动文件或目录
     */
    static bool move(const path& from, const path& to, bool overwrite = true) noexcept;

    /**
     * @brief 重命名文件或目录
     */
    static bool rename(const path& old_name, const path& new_name) noexcept;

    /**
     * @brief 创建并写入文件
     */
    static bool create_and_write(const path& p, const string& content, bool append = false);

    static size_t size(const path& p) noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILESYSTEM_HPP__
