#ifndef NEFORCE_CORE_FILE_FILE_HPP__
#define NEFORCE_CORE_FILE_FILE_HPP__

/**
 * @file file.hpp
 * @brief 文件操作类
 *
 * 此文件提供了完整的文件操作接口。
 */

#include "NeForce/core/exception/error_code.hpp"
#include "NeForce/core/file/file_async.hpp"
#include "NeForce/core/file/file_diff.hpp"
#include "NeForce/core/file/file_info.hpp"
#include "NeForce/core/file/file_locker.hpp"
#include "NeForce/core/file/file_mapper.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class file
 * @brief 文件操作类
 *
 * 支持移动语义，不支持拷贝，线程安全
 *
 * 主要特性：
 * - 同步/异步IO操作
 * - 内存映射
 * - 区域锁定
 * - 属性管理
 * - 比较和差异分析
 * - 自动缓冲和预取优化
 *
 * 线程安全，支持移动语义，不支持拷贝。
 */
class NEFORCE_API file {
public:
    /**
     * @brief 文件操作缓冲区大小
     *
     * 默认的I/O缓冲区大小。其会根据文件大小自动调整。
     */
    static constexpr size_t buffer_size = 8192;

    /**
     * @class line_iterator
     * @brief 行迭代器
     *
     * 提供按行遍历文件内容的迭代器接口，只读。
     */
    class line_iterator {
    public:
        using value_type = string;                    ///< 元素类型
        using reference = const string&;              ///< 引用类型
        using pointer = const string*;                ///< 指针类型
        using iterator_category = input_iterator_tag; ///< 迭代器类别
        using difference_type = ptrdiff_t;            ///< 差值类型

    private:
        const file* file_ = nullptr;  ///< 关联的文件对象
        mutable string current_line_; ///< 当前行内容

    public:
        /**
         * @brief 默认构造函数（结束迭代器）
         */
        line_iterator() = default;

        /**
         * @brief 构造函数
         * @param f 文件对象指针
         */
        explicit line_iterator(const file* f);

        /**
         * @brief 解引用操作符
         * @return 当前行的引用
         */
        reference operator*() const noexcept { return current_line_; }

        /**
         * @brief 成员访问操作符
         * @return 当前行指针
         */
        pointer operator->() const noexcept { return &current_line_; }

        /**
         * @brief 前置递增操作符
         * @return 递增后的迭代器
         */
        line_iterator& operator++();

        /**
         * @brief 后置递增操作符
         * @return 递增前的迭代器
         */
        line_iterator operator++(int);

        /**
         * @brief 相等比较操作符
         * @param rhs 另一个迭代器
         * @return 相等返回true
         */
        bool operator==(const line_iterator& rhs) const noexcept { return file_ == rhs.file_; }

        /**
         * @brief 不等比较操作符
         * @param rhs 另一个迭代器
         * @return 不等返回true
         */
        bool operator!=(const line_iterator& rhs) const noexcept { return !(*this == rhs); }
    };

#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type = ::DWORD;           ///< 大小类型
    using difference_type = ::LONGLONG;  ///< 偏移量类型
    using native_handle_type = ::HANDLE; ///< 原生文件句柄类型
#else
    using size_type = size_t;        ///< 大小类型
    using difference_type = ::off_t; ///< 偏移量类型
    using native_handle_type = int;  ///< 原生文件句柄类型
#endif

    /**
     * @struct chunk_info
     * @brief 文件块信息
     */
    struct chunk_info {
        difference_type offset; ///< 块偏移
        size_type size;         ///< 块大小
        size_type index;        ///< 块索引
    };

private:
    native_handle_type handle_; ///< 文件句柄
    path path_{};               ///< 文件路径
    bool opened_ = false;       ///< 是否已打开
    bool append_mode_ = false;  ///< 是否为追加模式

    size_type buffer_size_ = buffer_size;    ///< 缓冲区大小
    mutable byte_vector read_buffer_{};      ///< 读缓冲区
    mutable size_type read_buffer_pos_ = 0;  ///< 读缓冲区位置
    mutable size_type read_buffer_size_ = 0; ///< 读缓冲区有效数据大小
    mutable byte_vector write_buffer_{};     ///< 写缓冲区
    mutable size_type write_buffer_pos_ = 0; ///< 写缓冲区位置

    mutable error_code last_error_code_; ///< 最后错误码

    unique_ptr<file_mapper> map_;    ///< 内存映射对象
    unique_ptr<file_locker> locker_; ///< 文件锁对象
    unique_ptr<file_info> info_;     ///< 文件信息对象
    unique_ptr<file_async> async_;   ///< 异步I/O对象

private:
    void init_sub_objects() noexcept;
    void reset_sub_objects() noexcept;

    bool flush_write_buffer() const noexcept;
    bool fill_read_buffer() const;

    void set_last_error() const;

    void adjust_buffer_size();

public:
    /**
     * @brief 默认构造函数
     *
     * 创建未打开的文件对象。
     */
    file();

    /**
     * @brief 构造函数（打开文件）
     * @param pth 文件路径
     * @param append 是否为追加模式，默认为false
     * @param access 访问模式，默认为读写
     * @param share_mode 共享模式，默认为只读共享
     * @param creation 创建方式，默认为打开已存在文件
     * @param attributes 文件属性，默认为普通
     *
     * 创建并打开文件。
     */
    explicit file(path pth, bool append = false, file_access access = file_access::READ_WRITE,
                  file_shared share_mode = file_shared::SHARE_READ, file_creation creation = file_creation::OPEN_EXIST,
                  file_attri attributes = file_attri::NORMAL);

    file(const file&) = delete;
    file& operator=(const file&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的文件对象
     */
    file(file&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的文件对象
     * @return 自身引用
     */
    file& operator=(file&& other) noexcept;

    /**
     * @brief 析构函数
     *
     * 自动关闭文件，刷新缓冲区。
     */
    ~file();

    /**
     * @brief 打开文件
     * @param pth 文件路径
     * @param append 是否为追加模式
     * @param access 访问模式
     * @param share_mode 共享模式
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 打开成功返回true
     */
    bool open(path pth, bool append = false, file_access access = file_access::READ_WRITE,
              file_shared share_mode = file_shared::SHARE_READ_WRITE,
              file_creation creation = file_creation::OPEN_EXIST, file_attri attributes = file_attri::NORMAL);

    /**
     * @brief 重新打开文件（使用原有路径）
     * @param append 是否为追加模式
     * @param access 访问模式
     * @param share_mode 共享模式
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 打开成功返回true
     */
    bool open(bool append = false, file_access access = file_access::READ_WRITE,
              file_shared share_mode = file_shared::SHARE_READ_WRITE,
              file_creation creation = file_creation::OPEN_EXIST, file_attri attributes = file_attri::NORMAL);

    /**
     * @brief 关闭文件
     *
     * 刷新缓冲区，释放所有资源。
     */
    void close() noexcept;

    /**
     * @brief 刷新缓冲区
     * @return 成功返回true
     *
     * 将写缓冲区内容写入磁盘，同时刷新操作系统缓存。
     */
    bool flush() noexcept;

    /**
     * @brief 写入数据
     * @param data 要写入的数据
     * @param size 要写入的大小
     * @return 实际写入的字节数
     */
    size_type write(const string& data, size_type size);

    /**
     * @brief 写入字符串
     * @param data 要写入的字符串
     * @return 实际写入的字节数
     */
    size_type write(const string& data);

    /**
     * @brief 写入二进制数据
     * @param data 数据指针
     * @param size 数据大小
     * @return 实际写入的字节数
     */
    size_type write(const void* data, size_type size);

    /**
     * @brief 读取数据到缓冲区
     * @param buffer 缓冲区指针
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read(void* buffer, size_type size) const;

    /**
     * @brief 读取数据到字符串
     * @param out 输出字符串
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read(string& out, size_type size) const;

    /**
     * @brief 读取数据到字符串（自动调整大小）
     * @param out 输出字符串
     * @return 实际读取的字节数
     */
    size_type read(string& out) const;

    /**
     * @brief 读取整个文件
     * @return 文件内容字符串
     */
    NEFORCE_NODISCARD string read() const;

    /**
     * @brief 读取二进制数据到缓冲区
     * @param out 缓冲区指针
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read_binary(void* out, size_type size) const;

    /**
     * @brief 读取二进制数据到字符串
     * @param out 输出字符串
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read_binary(string& out, size_type size) const;

    /**
     * @brief 读取二进制数据到字符串（自动调整大小）
     * @param out 输出字符串
     * @return 实际读取的字节数
     */
    size_type read_binary(string& out) const;

    /**
     * @brief 读取整个文件（二进制模式）
     * @return 文件内容字符串
     */
    NEFORCE_NODISCARD string read_binary() const;

    /**
     * @brief 读取一行
     * @param line 输出行
     * @return 是否成功读取（包括空行）
     */
    bool read_line(string& line) const;

    /**
     * @brief 读取一行
     * @return 行字符串，文件末尾返回空字符串
     */
    NEFORCE_NODISCARD string read_line() const;

    /**
     * @brief 读取所有行
     * @return 行字符串向量
     *
     * 将整个文件按行分割，适用于中等大小文件。
     */
    NEFORCE_NODISCARD vector<string> read_lines() const;

    /**
     * @brief 读取文件块
     * @param chunk_size 块大小（字节）
     * @return 块字符串向量
     *
     * 将文件按固定大小分块，适用于流式处理大文件。
     */
    vector<string> read_chunks(size_type chunk_size = buffer_size * 16) const;

    /**
     * @brief 写入文件块
     * @param chunks 块向量
     * @return 写入成功返回true
     */
    bool write_chunks(const vector<string>& chunks);

    /**
     * @brief 获取块信息
     * @param chunk_size 块大小
     * @return 块信息向量
     *
     * 计算文件分块的信息，不实际读取数据。
     */
    NEFORCE_NODISCARD vector<chunk_info> chunks_info(size_type chunk_size) const;

    /**
     * @brief 移动文件指针
     * @param distance 移动距离（字节）
     * @param method 移动方式
     * @return 移动成功返回true
     *
     * 追加模式下只能seek到文件末尾。
     */
    bool seek(difference_type distance, file_pointer method = file_pointer::END) const;

    /**
     * @brief 获取当前文件指针位置
     * @return 当前位置（字节），-1表示错误
     *
     * 返回逻辑文件指针位置（考虑缓冲区）。
     */
    difference_type tell() const;

    /**
     * @brief 获取系统文件指针位置
     * @return 系统当前位置（字节），-1表示错误
     *
     * 返回操作系统层面的文件指针位置，不考虑缓冲区。
     */
    difference_type system_tell() const;

    /**
     * @brief 预取数据到缓存
     * @param hint_size 提示预取大小（字节）
     * @return 成功返回true
     *
     * 建议操作系统将后续数据预加载到内存，提高顺序访问性能。
     */
    bool prefetch(size_type hint_size = 0) const;

    /**
     * @brief 截断文件
     * @param size 新文件大小（字节）
     * @return 成功返回true
     *
     * 将文件截断到指定大小，多余数据丢失。
     * 如果新大小大于原文件，文件会扩展（内容未定义）。
     */
    bool truncate(difference_type size) const;

    /**
     * @brief 获取文件大小
     * @return 文件大小（字节），超过4GB返回0
     */
    NEFORCE_NODISCARD size_type size() const;

    /**
     * @brief 获取文件大小
     * @param out_size 输出文件大小（字节）
     * @return 成功返回true
     */
    bool size(size_type& out_size) const;

    /**
     * @brief 获取文件大小
     * @return 文件大小（字节），支持大文件
     */
    NEFORCE_NODISCARD uint64_t size64() const;

    /**
     * @brief 获取原生文件句柄
     * @return 文件句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept { return handle_; }

    /**
     * @brief 获取文件路径
     * @return 路径引用
     */
    NEFORCE_NODISCARD const path& file_path() const noexcept { return path_; }

    /**
     * @brief 检查文件是否已打开
     * @return 是否已打开
     */
    NEFORCE_NODISCARD bool is_opened() const noexcept { return opened_; }

    /**
     * @brief 检查是否为追加模式
     * @return 是否为追加模式
     */
    NEFORCE_NODISCARD bool is_append() const noexcept { return append_mode_; }

    /**
     * @brief 获取最后错误码
     * @return 错误码
     */
    NEFORCE_NODISCARD const error_code& last_error_code() const noexcept { return last_error_code_; }

    /**
     * @brief 清除错误状态
     */
    void clear_error() noexcept;

    /**
     * @brief 获取行迭代器起始
     * @return 行迭代器
     */
    NEFORCE_NODISCARD line_iterator begin_lines() const { return line_iterator(this); }

    /**
     * @brief 获取行迭代器结束
     * @return 结束迭代器
     */
    NEFORCE_NODISCARD line_iterator end_lines() const { return {}; }

    /**
     * @brief 获取内存映射对象
     * @return 内存映射对象引用
     */
    NEFORCE_NODISCARD file_mapper& mapper() noexcept { return *map_; }
    NEFORCE_NODISCARD const file_mapper& mapper() const noexcept { return *map_; }

    /**
     * @brief 获取文件锁对象
     * @return 文件锁对象引用
     */
    NEFORCE_NODISCARD file_locker& locker() noexcept { return *locker_; }
    NEFORCE_NODISCARD const file_locker& locker() const noexcept { return *locker_; }

    /**
     * @brief 获取文件信息对象
     * @return 文件信息对象引用
     */
    NEFORCE_NODISCARD file_info& info() noexcept { return *info_; }
    NEFORCE_NODISCARD const file_info& info() const noexcept { return *info_; }

    /**
     * @brief 获取异步I/O对象
     * @return 异步I/O对象引用
     */
    NEFORCE_NODISCARD file_async& async() noexcept { return *async_; }
    NEFORCE_NODISCARD const file_async& async() const noexcept { return *async_; }
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_HPP__
