#ifndef NEFORCE_CORE_FILE_FILE_HPP__
#define NEFORCE_CORE_FILE_FILE_HPP__

/**
 * @file file.hpp
 * @brief 文件操作类
 */

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
 * 提供完整的文件操作接口，包括：
 * - 同步/异步读写
 * - 内存映射文件
 * - 文件锁定
 * - 文件属性管理
 * - 文件比较和差异分析
 *
 * 线程安全，支持移动语义，不支持拷贝。
 */
class NEFORCE_API file {
public:
    /**
     * @brief 文件操作缓冲区大小
     *
     * 默认的I/O缓冲区大小，设置为8KB以获得较好的性能平衡。
     */
    static constexpr size_t buffer_size = 8192;


    class line_iterator {
    public:
        using value_type = string;
        using reference = const string&;
        using pointer = const string*;
        using iterator_category = input_iterator_tag;
        using difference_type = ptrdiff_t;

    private:
        const file* file_ = nullptr;
        mutable string current_line_;

    public:
        line_iterator() = default;
        explicit line_iterator(const file* f);

        reference operator *() const noexcept { return current_line_; }
        pointer operator ->() const noexcept { return &current_line_; }

        line_iterator& operator ++();
        line_iterator operator ++(int);

        bool operator ==(const line_iterator& rhs) const noexcept {
            return file_ == rhs.file_;
        }
        bool operator !=(const line_iterator& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type          = ::DWORD;
    using difference_type    = ::LONGLONG;
    using native_handle_type = ::HANDLE;
#else
    using size_type          = size_t;
    using difference_type    = ::off_t;
    using native_handle_type = int;
#endif

    /**
     * @struct chunk_info
     * @brief 文件块信息
     */
    struct chunk_info {
        difference_type offset;    ///< 块偏移
        size_type size;            ///< 块大小
        size_type index;           ///< 块索引
    };

private:
    native_handle_type handle_;    ///< 文件句柄
    path path_{};         ///< 文件路径
    bool opened_ = false;          ///< 是否已打开
    bool append_mode_ = false;     ///< 是否为追加模式

    size_type buffer_size_ = buffer_size;      ///< 缓冲区大小
    mutable byte_vector read_buffer_{};        ///< 读缓冲区
    mutable size_type read_buffer_pos_ = 0;    ///< 读缓冲区位置
    mutable size_type read_buffer_size_ = 0;   ///< 读缓冲区有效数据大小
    mutable byte_vector write_buffer_{};       ///< 写缓冲区
    mutable size_type write_buffer_pos_ = 0;   ///< 写缓冲区位置

    mutable string last_error_msg_;            ///< 最后错误信息
    mutable int last_error_code_ = 0;          ///< 最后错误码

    unique_ptr<file_mapper> map_;
    unique_ptr<file_locker> locker_;
    unique_ptr<file_info> info_;
    unique_ptr<file_async> async_;

private:
    void init_sub_objects() noexcept;
    void reset_sub_objects() noexcept;

    bool flush_write_buffer() const noexcept;
    bool fill_read_buffer() const noexcept;

    void set_last_error() const;

    void adjust_buffer_size();

public:
    /**
     * @brief 默认构造函数
     */
    file();

    /**
     * @brief 构造函数（打开文件）
     * @param pth 文件路径
     * @param append 是否为追加模式
     * @param access 访问模式
     * @param share_mode 共享模式
     * @param creation 创建方式
     * @param attributes 文件属性
     */
    explicit file(
        path pth, bool append = false,
        file_access access = file_access::READ_WRITE,
        file_shared share_mode = file_shared::SHARE_READ,
        file_creation creation = file_creation::OPEN_EXIST,
        file_attri attributes = file_attri::NORMAL);

    file(const file&) = delete;
    file& operator =(const file&) = delete;

    file(file&& other) noexcept;
    file& operator =(file&& other) noexcept;

    /**
     * @brief 析构函数
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
     * @return 是否成功
     */
    bool open(path pth, bool append = false,
              file_access access = file_access::READ_WRITE,
              file_shared share_mode = file_shared::SHARE_READ_WRITE,
              file_creation creation = file_creation::OPEN_EXIST,
              file_attri attributes = file_attri::NORMAL);

    /**
     * @brief 重新打开文件（使用原有路径）
     * @param append 是否为追加模式
     * @param access 访问模式
     * @param share_mode 共享模式
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 是否成功
     */
    bool open(bool append = false,
              file_access access = file_access::READ_WRITE,
              file_shared share_mode = file_shared::SHARE_READ_WRITE,
              file_creation creation = file_creation::OPEN_EXIST,
              file_attri attributes = file_attri::NORMAL);

    /**
     * @brief 关闭文件
     */
    void close() noexcept;

    /**
     * @brief 刷新缓冲区
     * @return 是否成功
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
     * @return 行字符串
     */
    NEFORCE_NODISCARD string read_line() const;

    /**
     * @brief 读取所有行
     * @return 行向量
     */
    NEFORCE_NODISCARD vector<string> read_lines() const;

    /**
     * @brief 读取文件块
     * @param chunk_size 块大小
     * @return 块字符串向量
     */
    vector<string> read_chunks(size_type chunk_size = buffer_size * 16) const;

    /**
     * @brief 写入文件块
     * @param chunks 块向量
     * @return 是否成功
     */
    bool write_chunks(const vector<string>& chunks);

    /**
     * @brief 获取块信息
     * @param chunk_size 块大小
     * @return 块信息向量
     */
    NEFORCE_NODISCARD vector<chunk_info> chunks_info(size_type chunk_size) const;

    /**
     * @brief 移动文件指针
     * @param distance 移动距离
     * @param method 移动方式
     * @return 是否成功
     */
    bool seek(difference_type distance, file_pointer method = file_pointer::END) const noexcept;

    /**
     * @brief 获取当前文件指针位置
     * @return 当前位置，-1表示错误
     */
    difference_type tell() const noexcept;

    /**
     * @brief 获取系统文件指针位置
     * @return 系统当前位置
     */
    difference_type system_tell() const noexcept;

    /**
     * @brief 预取数据到缓存
     * @param hint_size 提示大小
     * @return 是否成功
     */
    bool prefetch(size_type hint_size = 0) const noexcept;

    /**
     * @brief 截断文件
     * @param size 新大小
     * @return 是否成功
     */
    bool truncate(difference_type size) const noexcept;

    /**
     * @brief 获取文件大小
     * @return 文件大小
     */
    NEFORCE_NODISCARD size_type size() const;

    /**
     * @brief 获取文件大小
     * @param out_size 输出大小
     * @return 是否成功
     */
    bool size(size_type& out_size) const;

    /**
     * @brief 获取文件大小（64位）
     * @return 文件大小
     */
    NEFORCE_NODISCARD uint64_t size64() const;

    /**
     * @brief 获取原生文件句柄
     * @return 文件句柄
     */
    NEFORCE_NODISCARD native_handle_type native_handle() const noexcept {
        return handle_;
    }

    /**
     * @brief 获取文件路径
     * @return 路径引用
     */
    NEFORCE_NODISCARD const path& file_path() const noexcept {
        return path_;
    }

    /**
     * @brief 检查文件是否已打开
     * @return 是否已打开
     */
    NEFORCE_NODISCARD bool is_opened() const noexcept {
        return opened_;
    }

    /**
     * @brief 检查是否为追加模式
     * @return 是否为追加模式
     */
    NEFORCE_NODISCARD bool is_append() const noexcept {
        return append_mode_;
    }


    /**
     * @brief 获取最后错误信息
     * @return 错误信息
     */
    NEFORCE_NODISCARD const string& last_error() const noexcept {
        return last_error_msg_;
    }

    /**
     * @brief 获取最后错误码
     * @return 错误码
     */
    NEFORCE_NODISCARD int last_error_code() const noexcept {
        return last_error_code_;
    }

    /**
     * @brief 清除错误状态
     */
    void clear_error() noexcept;

    /**
     * @brief 获取行迭代器起始
     * @return 行迭代器
     */
    NEFORCE_NODISCARD line_iterator begin_lines() const {
        return line_iterator(this);
    }

    /**
     * @brief 获取行迭代器结束
     * @return 结束迭代器
     */
    NEFORCE_NODISCARD line_iterator end_lines() const {
        return {};
    }

    /**
     * @brief 内存映射操作
     */
    NEFORCE_NODISCARD file_mapper& mapper() noexcept { return *map_; }
    NEFORCE_NODISCARD const file_mapper& mapper() const noexcept { return *map_; }

    /**
     * @brief 文件锁操作
     */
    NEFORCE_NODISCARD file_locker& locker() noexcept { return *locker_; }
    NEFORCE_NODISCARD const file_locker& locker() const noexcept { return *locker_; }

    /**
     * @brief 文件属性与时间
     */
    NEFORCE_NODISCARD file_info& info() noexcept { return *info_; }
    NEFORCE_NODISCARD const file_info& info() const noexcept { return *info_; }

    /**
     * @brief 异步 I/O
     */
    NEFORCE_NODISCARD file_async& async() noexcept { return *async_; }
    NEFORCE_NODISCARD const file_async& async() const noexcept { return *async_; }
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_HPP__
