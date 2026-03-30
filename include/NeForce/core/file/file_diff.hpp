#ifndef NEFORCE_CORE_FILE_FILE_DIFF_HPP__
#define NEFORCE_CORE_FILE_FILE_DIFF_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/file/path.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class file_diff
 * @brief 文件比较与差异分析工具类
 *
 * 所有方法为静态方法，无状态。
 */
class NEFORCE_API file_diff {
public:
    file_diff() = delete;

    using size_type =
#ifdef NEFORCE_PLATFORM_WINDOWS
        unsigned long;
#else
        size_t;
#endif

    using difference_type = int64_t;

    /**
     * @struct binary_diff_entry
     * @brief 二进制差异条目
     */
    struct binary_diff_entry {
        difference_type offset = 0;
        byte_t byte1 = 0;
        byte_t byte2 = 0;
        int64_t size_diff = 0;
        bool is_size_diff = false;
    };

    /**
     * @brief 比较两个文件
     * @param binary 是否使用二进制比较
     */
    NEFORCE_NODISCARD static bool compare(const path& file1, const path& file2, bool binary = true);

    /**
     * @brief 二进制比较
     */
    NEFORCE_NODISCARD static bool compare_binary(const path& file1, const path& file2);

    /**
     * @brief 文本比较
     * @param ignore_case        忽略大小写
     * @param ignore_whitespace  忽略空白字符
     */
    NEFORCE_NODISCARD static bool compare_text(
        const path& file1, const path& file2,
        bool ignore_case = false,
        bool ignore_whitespace = false);

    /**
     * @brief 获取二进制差异列表
     * @param max_diffs 最多返回条目数
     */
    NEFORCE_NODISCARD static vector<binary_diff_entry> binary_diff(
        const path& file1, const path& file2,
        size_type max_diffs = 100);
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_DIFF_HPP__
