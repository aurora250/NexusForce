#ifndef NEFORCE_CORE_FILE_FILE_DIFF_HPP__
#define NEFORCE_CORE_FILE_FILE_DIFF_HPP__

/**
 * @file file_diff.hpp
 * @brief 文件比较与差异分析工具
 *
 * 此文件提供了文件比较和差异分析的功能，支持二进制比较、
 * 文本比较以及详细的差异条目生成。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/path.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class file_diff
 * @brief 文件比较与差异分析工具类
 *
 * 提供静态方法进行文件比较和差异分析。
 *
 * 主要功能：
 * - 二进制文件比较
 * - 文本文件比较（支持忽略大小写和空白字符）
 * - 获取详细的二进制差异列表
 */
class NEFORCE_API file_diff {
public:
    file_diff() = delete;

    using size_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
            unsigned long; ///< 大小类型
#else
            size_t; ///< 大小类型
#endif

    using difference_type = int64_t; ///< 差异偏移量类型

    /**
     * @struct binary_diff_entry
     * @brief 二进制差异条目
     *
     * 描述两个二进制文件中的一个差异点。
     */
    struct binary_diff_entry {
        difference_type offset = 0; ///< 差异在文件中的偏移量（字节）
        byte_t byte1 = 0;           ///< 第一个文件中的字节值
        byte_t byte2 = 0;           ///< 第二个文件中的字节值
        int64_t size_diff = 0;      ///< 文件大小差异（字节）
        bool is_size_diff = false;  ///< 是否为文件大小差异（而非内容差异）
    };

    /**
     * @brief 比较两个文件
     * @param file1 第一个文件路径
     * @param file2 第二个文件路径
     * @param binary 是否使用二进制比较，默认为true
     * @return 文件内容相同返回true，否则返回false
     *
     * 根据binary参数选择二进制比较或文本比较。
     */
    NEFORCE_NODISCARD static bool compare(const path& file1, const path& file2, bool binary = true);

    /**
     * @brief 二进制比较
     * @param file1 第一个文件路径
     * @param file2 第二个文件路径
     * @return 文件内容完全一致返回true
     *
     * 逐字节比较两个文件的内容，要求文件大小相同且每个字节都相等。
     * 使用缓冲读取方式，避免一次性加载整个文件到内存。
     */
    NEFORCE_NODISCARD static bool compare_binary(const path& file1, const path& file2);

    /**
     * @brief 文本比较
     * @param file1 第一个文件路径
     * @param file2 第二个文件路径
     * @param ignore_case 是否忽略大小写
     * @param ignore_whitespace 是否忽略空白字符（空格、制表符、换行等）
     * @return 文件内容相同返回true
     *
     * 按行比较文本文件，支持：
     * - 忽略大小写：将字母统一转换为小写后比较
     * - 忽略空白字符：去除行首尾空白，将连续空白压缩为单个空格
     */
    NEFORCE_NODISCARD static bool compare_text(const path& file1, const path& file2, bool ignore_case = false,
                                               bool ignore_whitespace = false);

    /**
     * @brief 获取二进制差异列表
     * @param file1 第一个文件路径
     * @param file2 第二个文件路径
     * @param max_diffs 最多返回的差异条目数，默认100
     * @return 差异条目列表
     *
     * 逐字节比较两个文件，收集所有差异点。
     * 如果文件大小不同，会包含一个标识大小差异的条目。
     * 限制max_diffs可避免处理超大文件时产生过多条目。
     *
     * @note 差异条目按文件偏移量升序排列
     */
    NEFORCE_NODISCARD static vector<binary_diff_entry> binary_diff(const path& file1, const path& file2,
                                                                   size_type max_diffs = 100);
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_DIFF_HPP__
