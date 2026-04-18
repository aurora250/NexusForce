#ifndef NEFORCE_CORE_FILE_FILESYSTEM_HPP__
#define NEFORCE_CORE_FILE_FILESYSTEM_HPP__

/**
 * @file filesystem.hpp
 * @brief 文件系统操作工具类
 *
 * 此文件提供了文件和目录的常用操作，
 * 包括创建、删除、复制、移动、重命名等。
 */

#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/utility/byte_size.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作相关类
 * @{
 */

/**
 * @class filesystem
 * @brief 文件系统操作工具类
 *
 * 提供文件与目录的创建、删除、复制、移动、重命名等操作。
 *
 * @note 文件系统操作可能因权限不足而失败。
 */
class NEFORCE_API filesystem {
public:
    filesystem() = delete;

    /**
     * @brief 创建目录（含所有父级目录）
     * @param p 要创建的目录路径
     * @return 创建成功返回true，如果目录已存在也返回true
     *
     * 自动创建路径中所有不存在的父级目录。
     */
    static bool create_directories(const path& p);

    /**
     * @brief 删除文件
     * @param p 要删除的文件路径
     * @return 删除成功返回true，文件不存在或为目录返回false
     *
     * 只删除文件，不能用于删除目录。
     * 如果文件具有只读属性，在Windows上会删除失败。
     */
    static bool remove(const path& p) noexcept;

    /**
     * @brief 删除空目录
     * @param p 要删除的目录路径
     * @return 删除成功返回true，目录不存在或非空返回false
     *
     * 只能删除空目录。如需删除非空目录，请使用remove_all()。
     */
    static bool remove_directory(const path& p) noexcept;

    /**
     * @brief 删除目录内所有内容
     * @param p 目录路径
     * @param recursive 是否递归删除子目录，默认为true
     * @return 删除成功返回true
     *
     * 清空目录中的所有文件和子目录，但不删除目录本身。
     * 如果recursive为false，只删除目录中的文件，保留子目录。
     */
    static bool remove_all_in_directory(const path& p, bool recursive = true);

    /**
     * @brief 删除文件或目录
     * @param p 要删除的文件或目录路径
     * @return 删除成功返回true
     *
     * 如果是文件，直接删除；
     * 如果是目录，递归删除目录及其所有内容。
     */
    static bool remove_all(const path& p);

    /**
     * @brief 复制文件
     * @param from 源文件路径
     * @param to 目标文件路径
     * @param overwrite 是否覆盖已存在的目标文件，默认为true
     * @return 复制成功返回true
     *
     * 复制文件内容，保留文件权限和时间戳。
     * 如果to是目录，文件将被复制到该目录下。
     * 目标目录不存在时会自动创建。
     */
    static bool copy(const path& from, const path& to, bool overwrite = true);

    /**
     * @brief 复制目录
     * @param src 源目录路径
     * @param dest 目标目录路径
     * @param overwrite 是否覆盖已存在的文件，默认为true
     * @return 复制成功返回true
     *
     * 递归复制整个目录树，包括所有子目录和文件。
     * 目标目录不存在时会自动创建。
     */
    static bool copy_directory(const path& src, const path& dest, bool overwrite = true);

    /**
     * @brief 移动文件或目录
     * @param from 源路径
     * @param to 目标路径
     * @param overwrite 是否覆盖已存在的目标，默认为true
     * @return 移动成功返回true
     *
     * 跨文件系统时，会执行复制后删除原文件。
     * 支持文件和目录的移动。
     */
    static bool move(const path& from, const path& to, bool overwrite = true);

    /**
     * @brief 重命名文件或目录
     * @param old_name 原路径
     * @param new_name 新路径
     * @return 重命名成功返回true
     *
     * 要求新路径在同一文件系统内。
     */
    static bool rename(const path& old_name, const path& new_name);

    /**
     * @brief 创建并写入文件
     * @param p 文件路径
     * @param content 要写入的内容
     * @param append 是否追加到文件末尾，默认为false
     * @return 写入成功返回true
     *
     * 自动创建文件所在的目录（如果不存在）。
     * 如果append为true，在文件末尾追加内容。
     */
    static bool create_and_write(const path& p, const string& content, bool append = false);

    /**
     * @brief 获取文件大小
     * @param p 文件路径
     * @return 文件大小，如果文件不存在或获取失败返回0
     *
     * 对于目录，返回0。
     * 对于大文件可能返回截断值。
     */
    static byte_size size(const path& p) noexcept;
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILESYSTEM_HPP__
