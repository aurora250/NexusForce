#ifndef NEFORCE_CORE_FILE_PATH_HPP__
#define NEFORCE_CORE_FILE_PATH_HPP__

/**
 * @file path.hpp
 * @brief 文件路径类
 *
 * 此文件提供了跨平台的文件路径操作类，
 * 支持路径的解析、规范化、组合、比较以及文件系统操作。
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/interface/istringify.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

class NEFORCE_API path_tree;


/**
 * @class path
 * @brief 文件路径类
 *
 * 提供跨平台的文件路径操作，包括：
 * - 路径组件访问（父路径、文件名、扩展名）
 * - 路径规范化（解析.和..）
 * - 路径组合（/操作符）
 * - 文件系统操作（创建、删除、复制、移动）
 * - 路径比较和哈希
 */
class NEFORCE_API path : public icommon<path>, public istringify<path> {
public:
    /**
     * @brief 文件操作缓冲区大小
     *
     * 默认的I/O缓冲区大小，设置为8KB以获得较好的性能平衡。
     */
    static constexpr size_t buffer_size = 8192;

    /**
     * @brief 路径分隔符集合
     *
     * 用于路径分割操作，包含所有可能的路径分隔符。
     */
    static constexpr auto spliter =
#ifdef NEFORCE_PLATFORM_WINDOWS
            "\\/";
#elif defined(NEFORCE_PLATFORM_LINUX)
            "/";
#endif

    /**
     * @brief 系统首选路径分隔符
     *
     * 用于构建系统原生路径。
     */
    static constexpr char preferred_separator = spliter[0];


    /**
     * @class split_iterator
     * @brief 路径分割迭代器
     *
     * 将文件路径分割为各个组件（如目录名、文件名）的迭代器。
     * 支持Windows和Linux的路径格式：
     * - Windows: "C:\Windows\System32" -> "C:", "Windows", "System32"
     * - Linux: "/usr/local/bin" -> "", "usr", "local", "bin"
     *
     * 使用前向迭代器接口，适用于范围for循环和标准算法。
     */
    class split_iterator {
    public:
        using value_type = string_view;                 ///< 值类型
        using reference = value_type;                   ///< 引用类型
        using pointer = void;                           ///< 指针类型
        using iterator_category = forward_iterator_tag; ///< 迭代器类别
        using difference_type = ptrdiff_t;              ///< 差值类型

    private:
        const string* path_ = nullptr; ///< 被遍历的路径字符串
        size_t start_ = 0;             ///< 当前组件的起始位置
        size_t end_ = 0;               ///< 当前组件的结束位置
        bool done_ = true;             ///< 是否已完成遍历
        string current_part_;          ///< 当前组件的字符串

        /**
         * @brief 查找下一个路径组件
         *
         * 从当前位置开始查找下一个路径组件。
         */
        void find_next();

    public:
        /**
         * @brief 默认构造函数
         *
         * 构造一个结束迭代器。
         */
        split_iterator() noexcept = default;

        /**
         * @brief 构造函数
         * @param path 要遍历的路径字符串指针
         * @param pos 起始位置（默认为0）
         *
         * 构造一个从指定位置开始遍历路径的迭代器。
         * 如果路径为空或起始位置无效，则构造为结束迭代器。
         */
        explicit split_iterator(const string* path, const size_t pos = 0) :
        path_(path),
        start_(pos),
        done_(false) {
            if (!path_ || path_->empty() || start_ >= path_->size()) {
                done_ = true;
                return;
            }
            find_next();
        }

        /**
         * @brief 解引用操作符
         * @return 当前路径组件的字符串视图
         *
         * 返回当前组件的字符串视图，不包含路径分隔符。
         */
        reference operator*() const noexcept { return current_part_.view(); }

        /**
         * @brief 前置递增操作符
         * @return 递增后的迭代器引用
         *
         * 移动到下一个路径组件。
         */
        split_iterator& operator++() {
            if (done_) {
                return *this;
            }
            start_ = end_ + 1;
            if (start_ >= path_->size()) {
                done_ = true;
                current_part_ = {};
            } else {
                find_next();
            }
            return *this;
        }

        /**
         * @brief 后置递增操作符
         * @return 递增前的迭代器副本
         */
        split_iterator operator++(int) {
            split_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        /**
         * @brief 相等比较操作符
         * @param b 另一个迭代器
         * @return 两个迭代器是否相等
         *
         * 两个迭代器相等当且仅当：
         * - 两者都处于结束状态，或者
         * - 指向同一个路径且当前位置相同
         */
        NEFORCE_NODISCARD bool operator==(const split_iterator& b) const noexcept {
            if (done_ && b.done_) {
                return true;
            }
            if (path_ != b.path_) {
                return false;
            }
            return start_ == b.start_;
        }

        /**
         * @brief 不等比较操作符
         * @param b 另一个迭代器
         * @return 两个迭代器是否不等
         */
        NEFORCE_NODISCARD bool operator!=(const split_iterator& b) const noexcept { return !(*this == b); }
    };

private:
    string path_{}; ///< 存储的路径字符串

public:
    /**
     * @brief 默认构造函数，创建空路径
     */
    path() = default;

    /**
     * @brief 从字符串构造路径
     * @param path 路径字符串
     */
    explicit path(string path) noexcept :
    path_(_NEFORCE move(path)) {}

    /**
     * @brief 从字符串视图构造路径
     * @param path 路径字符串视图
     */
    explicit path(const string_view path) :
    path_(path) {}

    /**
     * @brief 从C风格字符串构造路径
     * @param path 路径C字符串
     */
    explicit path(const char* path) :
    path_(path) {}

    path(const path&) = default;
    path(path&&) noexcept = default;
    path& operator=(const path&) = default;
    path& operator=(path&&) noexcept = default;

    /**
     * @brief 获取路径字符串
     * @return 路径字符串常量引用
     */
    NEFORCE_NODISCARD const string& str() const noexcept { return path_; }

    /**
     * @brief 获取路径字符串视图
     * @return 路径字符串视图
     */
    NEFORCE_NODISCARD string_view view() const noexcept { return path_.view(); }

    /**
     * @brief 获取C风格字符串
     * @return 路径C字符串指针
     */
    NEFORCE_NODISCARD const char* data() const noexcept { return path_.data(); }

    /**
     * @brief 检查路径是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return path_.empty(); }

    /**
     * @brief 获取起始路径组件迭代器
     * @return 指向第一个组件的迭代器
     */
    NEFORCE_NODISCARD split_iterator begin() const { return split_iterator(&path_, 0); }

    /**
     * @brief 获取结束路径组件迭代器
     * @return 结束迭代器
     */
    NEFORCE_NODISCARD split_iterator end() const { return split_iterator(); }

    /**
     * @brief 获取父路径
     * @return 父路径对象
     */
    NEFORCE_NODISCARD path parent_path() const;

    /**
     * @brief 获取文件名
     * @return 文件名字符串视图
     *
     * 返回路径的最后一部分。
     */
    NEFORCE_NODISCARD string_view filename() const noexcept;

    /**
     * @brief 获取文件主名（不含扩展名）
     * @return 文件主名字符串视图
     *
     * 返回文件名去除最后一个点及之后的部分。
     */
    NEFORCE_NODISCARD string_view stem() const noexcept;

    /**
     * @brief 获取文件扩展名
     * @return 扩展名字符串视图
     *
     * 返回文件名的最后一个点之后的部分。
     */
    NEFORCE_NODISCARD string_view extension() const noexcept;

    /**
     * @brief 规范化路径
     * @return 规范化后的路径
     *
     * 解析路径中的"."和".."，移除多余的分隔符。
     */
    NEFORCE_NODISCARD path lexically_normal() const;

    /**
     * @brief 获取绝对路径
     * @param base 基础路径
     * @return 绝对路径
     *
     * 将相对路径转换为绝对路径。
     */
    NEFORCE_NODISCARD path absolute(const path& base = current_path()) const;

    /**
     * @brief 获取相对于另一路径的路径
     * @param base 基础路径
     * @return 相对路径
     *
     * 计算当前路径相对于base路径的相对路径。
     */
    NEFORCE_NODISCARD path relative(const path& base) const;

    /**
     * @brief 获取当前工作目录
     * @return 当前工作目录路径
     */
    static path current_path();

    /**
     * @brief 获取临时目录路径
     * @return 临时目录路径
     */
    static path temp_directory_path();

    /**
     * @brief 路径连接赋值操作符
     * @param other 要连接的路径
     * @return 自身引用
     *
     * 将另一个路径连接到当前路径，自动处理分隔符。
     */
    path& operator/=(const path& other);

    /**
     * @brief 路径连接赋值操作符（字符串视图版本）
     * @param other 要连接的路径字符串视图
     * @return 自身引用
     */
    path& operator/=(string_view other);

    /**
     * @brief 路径连接操作符
     * @param other 要连接的路径
     * @return 连接后的新路径
     */
    path operator/(const path& other) const;

    /**
     * @brief 路径连接操作符（字符串视图版本）
     * @param pth 要连接的路径字符串视图
     * @return 连接后的新路径
     */
    path operator/(string_view pth) const;

    /**
     * @brief 扫描此路径（必须为目录）并返回路径树
     * @return 路径树
     */
    NEFORCE_NODISCARD path_tree to_tree() const;

    /**
     * @brief 获取直接子路径列表（非递归）
     * @param include_hidden 是否包含隐藏条目
     * @return 直接子路径列表
     */
    NEFORCE_NODISCARD vector<path> children(bool include_hidden = false) const;

    /**
     * @brief 获取直接子文件路径列表
     * @param include_hidden 是否包含隐藏文件
     * @return 子文件路径列表
     */
    NEFORCE_NODISCARD vector<path> child_files(bool include_hidden = false) const;

    /**
     * @brief 获取直接子目录路径列表
     * @param include_hidden 是否包含隐藏目录
     * @return 子目录路径列表
     */
    NEFORCE_NODISCARD vector<path> child_dirs(bool include_hidden = false) const;

    /**
     * @brief 检查路径是否存在
     * @return 是否存在
     */
    NEFORCE_NODISCARD bool exists() const noexcept;

    /**
     * @brief 检查路径是否为目录
     * @return 是否为目录
     */
    NEFORCE_NODISCARD bool is_directory() const noexcept;

    /**
     * @brief 检查路径是否为普通文件
     * @return 是否为文件
     */
    NEFORCE_NODISCARD bool is_file() const noexcept;

    /**
     * @brief 检查路径是否存在
     * @param path 要检查的路径字符串
     * @return 是否存在
     */
    NEFORCE_NODISCARD static bool exists(const string& path) noexcept;

    /**
     * @brief 检查路径是否为目录
     * @param path 要检查的路径字符串
     * @return 是否为目录
     */
    NEFORCE_NODISCARD static bool is_directory(const string& path) noexcept;

    /**
     * @brief 检查路径是否为文件
     * @param path 要检查的路径字符串
     * @return 是否为文件
     */
    NEFORCE_NODISCARD static bool is_file(const string& path) noexcept;

    /**
     * @brief 获取文件扩展名
     * @param path 路径字符串视图
     * @return 扩展名字符串视图
     */
    NEFORCE_NODISCARD static string_view extension(string_view path) noexcept;

    /**
     * @brief 转换为字符串视图
     */
    explicit operator string_view() const noexcept { return path_.view(); }

    /**
     * @brief 相等比较
     * @param rhs 另一个路径
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool operator==(const path& rhs) const;

    /**
     * @brief 小于比较
     * @param rhs 另一个路径
     * @return 是否小于
     */
    NEFORCE_NODISCARD bool operator<(const path& rhs) const;

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD size_t to_hash() const;

    /**
     * @brief 转换为字符串
     * @return 规范化后的路径字符串
     */
    NEFORCE_NODISCARD string to_string() const { return lexically_normal().str(); }

    /**
     * @brief 交换两个路径
     * @param other 要交换的路径
     */
    void swap(path& other) noexcept { path_.swap(other.path_); }
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_PATH_HPP__
