#ifndef MSTL_CORE_FILE_FILE_HPP__
#define MSTL_CORE_FILE_FILE_HPP__

/**
 * @file file.hpp
 * @brief 文件操作类
 */

#include "MSTL/core/async/mutex.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/file/file_constants.hpp"
#include "MSTL/core/file/path.hpp"
#include "MSTL/core/time/datetime.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <aio.h>
#endif
MSTL_BEGIN_NAMESPACE__

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
class MSTL_API file {
public:
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

        bool operator ==(const line_iterator& b) const noexcept {
            return file_ == b.file_;
        }
        bool operator !=(const line_iterator& b) const noexcept {
            return !(*this == b);
        }
    };

#ifdef MSTL_PLATFORM_WINDOWS__
    using size_type = ::DWORD;           ///< 大小类型
    using difference_type = ::LONGLONG;  ///< 差值类型
    using file_handle = ::HANDLE;        ///< 文件句柄类型
    using time_type = ::FILETIME;        ///< 时间类型
    using aiocb_type = ::OVERLAPPED;     ///< 异步I/O控制块类型
#else
    using size_type = size_t;            ///< 大小类型
    using difference_type = ::off_t;     ///< 差值类型
    using file_handle = int;             ///< 文件句柄类型
    using time_type = ::time_t;          ///< 时间类型
    using aiocb_type = ::aiocb;          ///< 异步I/O控制块类型
#endif

    /**
     * @struct chunk_info
     * @brief 文件块信息
     */
    struct chunk_info {
        difference_type offset;    ///< 块偏移
        size_type size;            ///< 块大小
        size_type chunk_index;     ///< 块索引
    };

    /**
     * @struct map_info
     * @brief 内存映射信息
     */
    struct map_info {
        void* address = nullptr;   ///< 映射地址
        size_type size = 0;        ///< 映射大小
        size_type offset = 0;      ///< 映射偏移
        FILE_ACCESS access = FILE_ACCESS::READ; ///< 访问模式
        bool is_mapped = false;    ///< 是否已映射
    };

    /**
     * @struct binary_diff_entry
     * @brief 二进制差异条目
     */
    struct binary_diff_entry {
        difference_type offset = 0; ///< 差异位置
        byte_t byte1 = 0;           ///< 文件1的字节
        byte_t byte2 = 0;           ///< 文件2的字节
        int64_t size_diff = 0;      ///< 大小差异
        bool is_size_diff = false;  ///< 是否为大小差异
    };

    /**
     * @struct async_context
     * @brief 异步操作上下文
     */
    struct async_context {
        string data{};              ///< 写入数据
        string* buffer = nullptr;   ///< 读取缓冲区
        aiocb_type* cb = nullptr;   ///< 控制块
        bool is_write;              ///< 是否为写入操作

        explicit async_context(string&& d);
        explicit async_context(string* buf);
        ~async_context();
    };

    /**
     * @struct async_result
     * @brief 异步操作结果
     */
    struct async_result {
        bool completed = false;          ///< 是否完成
        size_t bytes_transferred = 0;    ///< 传输字节数
        int error_code = 0;              ///< 错误码
        aiocb_type* cb = nullptr;        ///< 控制块
        async_context* user_context = nullptr;  ///< 用户上下文
    };

private:
    MSTL_ALWAYS_INLINE static const file_handle& INVALID_HANDLE() noexcept {
        static const auto INVALID_HANDLE =
#ifdef MSTL_PLATFORM_WINDOWS__
            INVALID_HANDLE_VALUE;
#elif defined(MSTL_PLATFORM_LINUX__)
            -1;
#endif
        return INVALID_HANDLE;
    }

    file_handle handle_ = INVALID_HANDLE();    ///< 文件句柄
    _MSTL path path_{};                        ///< 文件路径
    bool opened_ = false;                      ///< 是否已打开
    bool append_mode_ = false;                 ///< 是否为追加模式

    size_type buffer_size_ = FILE_BUFFER_SIZE;   ///< 缓冲区大小
    mutable vector<byte_t> read_buffer_{};     ///< 读缓冲区
    mutable size_type read_buffer_pos_ = 0;    ///< 读缓冲区位置
    mutable size_type read_buffer_size_ = 0;   ///< 读缓冲区有效数据大小
    mutable vector<byte_t> write_buffer_{};    ///< 写缓冲区
    mutable size_type write_buffer_pos_ = 0;   ///< 写缓冲区位置

    mutable mutex map_mutex_;                  ///< 映射操作互斥锁
    void* mapped_ptr_ = nullptr;               ///< 映射地址
    size_type mapped_size_ = 0;                ///< 映射大小
    size_type mapped_offset_ = 0;              ///< 映射偏移
    FILE_ACCESS mapped_access_ = FILE_ACCESS::READ;  ///< 映射访问模式
    size_type is_mapped_ = false;               ///< 是否已映射
#ifdef MSTL_PLATFORM_WINDOWS__
    ::HANDLE mapping_handle_ = INVALID_HANDLE_VALUE;  ///< 映射句柄
#endif

    mutable mutex async_mutex_;                  ///< 异步操作互斥锁
    mutable vector<aiocb_type*> async_operations_;   ///< 异步操作列表
    mutable unordered_map<aiocb_type*, async_context*> async_contexts_;   ///< 异步上下文映射

    mutable string last_error_msg_;              ///< 最后错误信息
    mutable int last_error_code_ = 0;            ///< 最后错误码

private:
    bool complete_async_result(async_result& result, size_type bytes_transferred);
    bool check_async_completion(async_result& result);

    bool flush_write_buffer() const noexcept;
    bool fill_read_buffer() const noexcept;

    static datetime filetime_to_datetime(const time_type& ft) noexcept;
    static time_type datetime_to_filetime(const datetime& dt) noexcept;
    static string get_last_error_msg();

    void set_last_error() const;

    void adjust_buffer_size();

public:
    /**
     * @brief 默认构造函数
     */
    file() = default;

    /**
     * @brief 构造函数（打开文件）
     * @param p 文件路径
     * @param access 访问模式
     * @param share_mode 共享模式
     * @param creation 创建方式
     * @param attributes 文件属性
     * @param append 是否为追加模式
     */
    explicit file(
        _MSTL path p,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        bool append = false) : path_(_MSTL move(p)) {
        this->open(path_, append, access, share_mode, creation, attributes);
    }

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
     * @param p 文件路径
     * @param append 是否为追加模式
     * @param access 访问模式
     * @param share_mode 共享模式
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 是否成功
     */
    bool open(_MSTL path p, bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ_WRITE,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

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
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ_WRITE,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

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
     * @brief 获取原生文件句柄
     * @return 文件句柄
     */
    MSTL_NODISCARD file_handle native_handle() const noexcept {
        return handle_;
    }

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
     * @param str 输出字符串
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read(string& str, size_type size) const;

    /**
     * @brief 读取数据到字符串（自动调整大小）
     * @param str 输出字符串
     * @return 实际读取的字节数
     */
    size_type read(string& str) const;

    /**
     * @brief 读取整个文件
     * @return 文件内容字符串
     */
    MSTL_NODISCARD string read() const;

    /**
     * @brief 读取二进制数据到缓冲区
     * @param buffer 缓冲区指针
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read_binary(void* buffer, size_type size) const;

    /**
     * @brief 读取二进制数据到字符串
     * @param str 输出字符串
     * @param size 要读取的大小
     * @return 实际读取的字节数
     */
    size_type read_binary(string& str, size_type size) const;

    /**
     * @brief 读取二进制数据到字符串（自动调整大小）
     * @param str 输出字符串
     * @return 实际读取的字节数
     */
    size_type read_binary(string& str) const;

    /**
     * @brief 读取整个文件（二进制模式）
     * @return 文件内容字符串
     */
    MSTL_NODISCARD string read_binary() const;

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
    MSTL_NODISCARD string read_line() const;

    /**
     * @brief 读取所有行
     * @return 行向量
     */
    MSTL_NODISCARD vector<string> read_lines() const;

    /**
     * @brief 读取文件块
     * @param chunk_size 块大小
     * @return 块字符串向量
     */
    vector<string> read_chunks(size_type chunk_size = FILE_BUFFER_SIZE * 16) const;

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
    MSTL_NODISCARD vector<chunk_info> chunks_info(size_type chunk_size) const;

    /**
     * @brief 异步读取
     * @param buffer 输出缓冲区
     * @param size 要读取的大小
     * @param offset 读取偏移（-1表示当前位置）
     * @return 异步结果
     */
    async_result async_read(string& buffer, size_type size, difference_type offset = -1) const;

    /**
     * @brief 异步写入
     * @param data 要写入的数据
     * @param size 要写入的大小
     * @param offset 写入偏移（-1表示当前位置）
     * @return 异步结果
     */
    async_result async_write(string data, size_type size, difference_type offset = -1);

    /**
     * @brief 等待异步操作完成
     * @param result 异步结果
     * @param timeout_ms 超时时间（毫秒）
     * @return 是否完成
     */
    bool wait_async(async_result& result, uint32_t timeout_ms = numeric_traits<uint32_t>::max());

    /**
     * @brief 取消异步操作
     * @param result 异步结果
     */
    void cancel_async(async_result& result);

    /**
     * @brief 获取文件大小
     * @return 文件大小
     */
    MSTL_NODISCARD size_type size() const;

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
    MSTL_NODISCARD uint64_t size64() const;

    /**
     * @brief 获取文件路径
     * @return 路径引用
     */
    MSTL_NODISCARD const _MSTL path& path() const noexcept {
        return path_;
    }

    /**
     * @brief 检查文件是否已打开
     * @return 是否已打开
     */
    MSTL_NODISCARD bool is_opened() const noexcept {
        return opened_;
    }

    /**
     * @brief 检查是否为追加模式
     * @return 是否为追加模式
     */
    MSTL_NODISCARD bool is_append() const noexcept {
        return append_mode_;
    }


    /**
     * @brief 获取最后错误信息
     * @return 错误信息
     */
    MSTL_NODISCARD string last_error() const noexcept {
        return last_error_msg_;
    }

    /**
     * @brief 获取最后错误码
     * @return 错误码
     */
    MSTL_NODISCARD int last_error_code() const noexcept {
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
    MSTL_NODISCARD line_iterator begin_lines() const {
        return line_iterator(this);
    }

    /**
     * @brief 获取行迭代器结束
     * @return 结束迭代器
     */
    MSTL_NODISCARD line_iterator end_lines() const {
        return {};
    }

    /**
     * @brief 比较两个文件
     * @param file1 文件1路径
     * @param file2 文件2路径
     * @param binary 是否使用二进制比较
     * @return 是否相等
     */
    MSTL_NODISCARD static bool compare(const _MSTL path& file1, const _MSTL path& file2, bool binary = true);

    /**
     * @brief 二进制比较两个文件
     * @param file1 文件1路径
     * @param file2 文件2路径
     * @return 是否相等
     */
    MSTL_NODISCARD static bool compare_binary(const _MSTL path& file1, const _MSTL path& file2);

    /**
     * @brief 文本比较两个文件
     * @param file1 文件1路径
     * @param file2 文件2路径
     * @param ignore_case 忽略大小写
     * @param ignore_whitespace 忽略空白
     * @return 是否相等
     */
    MSTL_NODISCARD static bool compare_text(
        const _MSTL path& file1, const _MSTL path& file2,
        bool ignore_case = false, bool ignore_whitespace = false);

    /**
     * @brief 获取二进制差异
     * @param file1 文件1路径
     * @param file2 文件2路径
     * @param max_diffs 最大差异数
     * @return 差异条目向量
     */
    MSTL_NODISCARD static vector<binary_diff_entry> binary_diff(
        const _MSTL path& file1,
        const _MSTL path& file2,
        size_type max_diffs = 100);

    /**
     * @brief 移动文件指针
     * @param distance 移动距离
     * @param method 移动方式
     * @return 是否成功
     */
    bool seek(difference_type distance, FILE_POINTER method = FILE_POINTER::END) const noexcept;

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
     * @brief 锁定文件区域
     * @param offset 起始偏移
     * @param length 锁定长度（0表示到文件尾）
     * @param mode 锁定模式
     * @return 是否成功
     */
    bool lock(difference_type offset, difference_type length, FILE_LOCK mode = FILE_LOCK::EXCLUSIVE) const noexcept;

    /**
     * @brief 解锁文件区域
     * @param offset 起始偏移
     * @param length 锁定长度
     * @return 是否成功
     */
    bool unlock(difference_type offset, difference_type length) const noexcept;

    /**
     * @brief 尝试锁定文件区域
     * @param offset 起始偏移
     * @param length 锁定长度
     * @param mode 锁定模式
     * @return 是否成功
     */
    bool try_lock(difference_type offset, difference_type length, FILE_LOCK mode) const noexcept;

    /**
     * @brief 检查文件区域是否被锁定
     * @param offset 起始偏移
     * @param length 锁定长度
     * @param lock_out 输出锁定类型
     * @return 是否被锁定
     */
    MSTL_NODISCARD bool is_locked(
        difference_type offset,
        difference_type length,
        FILE_LOCK* lock_out) const noexcept;

    /**
     * @brief 锁定整个文件
     * @param mode 锁定模式
     * @return 是否成功
     */
    bool lock_whole(FILE_LOCK mode) const noexcept;

    /**
     * @brief 解锁整个文件
     * @return 是否成功
     */
    bool unlock_whole() const noexcept;

    /**
     * @brief 映射文件到内存
     * @param offset 映射偏移
     * @param size 映射大小（0表示到文件尾）
     * @param access 访问模式
     * @param hint 访问提示
     * @return 是否成功
     */
    bool map(size_type offset = 0, size_type size = 0,
             FILE_ACCESS access = FILE_ACCESS::READ,
             FILE_MAP_HINT hint = FILE_MAP_HINT::SEQUENTIAL);

    /**
     * @brief 解除映射
     */
    void unmap() noexcept;

    /**
     * @brief 重新映射
     * @param new_offset 新偏移
     * @param new_size 新大小
     * @return 是否成功
     */
    bool remap(size_type new_offset, size_type new_size);

    /**
     * @brief 刷新映射区域
     * @param async 是否异步
     * @return 是否成功
     */
    bool flush_mapped(bool async = false);

    /**
     * @brief 锁定/解锁映射页
     * @param lock_in_memory 是否锁定
     * @return 是否成功
     */
    bool lock_mapped_pages(bool lock_in_memory) const noexcept;

    /**
     * @brief 获取映射信息
     * @return 映射信息
     */
    MSTL_NODISCARD map_info map_infos() const noexcept;

    /**
     * @brief 获取映射数据指针
     * @return 映射地址
     */
    MSTL_NODISCARD void* mapped_data() const noexcept {
        return mapped_ptr_;
    }

    /**
     * @brief 获取映射大小
     * @return 映射大小
     */
    MSTL_NODISCARD size_type mapped_size() const noexcept {
        return mapped_size_;
    }

    /**
     * @brief 获取映射偏移
     * @return 映射偏移
     */
    MSTL_NODISCARD size_type mapped_offset() const noexcept {
        return mapped_offset_;
    }

    /**
     * @brief 获取映射访问模式
     * @return 访问模式
     */
    MSTL_NODISCARD FILE_ACCESS mapped_access() const noexcept {
        return mapped_access_;
    }

    /**
     * @brief 检查是否已映射
     * @return 是否已映射
     */
    MSTL_NODISCARD bool is_mapped() const noexcept {
        return mapped_ptr_ != nullptr;
    }

    /**
     * @brief 获取文件属性
     * @return 文件属性
     */
    MSTL_NODISCARD FILE_ATTRI attributes() const noexcept;

    /**
     * @brief 设置文件属性
     * @param attr 文件属性
     * @return 是否成功
     */
    bool set_attributes(FILE_ATTRI attr) noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    /**
     * @brief 获取文件创建时间
     * @return 创建时间
     */
    MSTL_NODISCARD datetime creation_time() const noexcept;
#endif

    /**
     * @brief 获取最后访问时间
     * @return 最后访问时间
     */
    MSTL_NODISCARD datetime last_access_time() const noexcept;

    /**
     * @brief 获取最后修改时间
     * @return 最后修改时间
     */
    MSTL_NODISCARD datetime last_write_time() const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    /**
     * @brief 设置所有时间
     * @param create 创建时间
     * @param access 访问时间
     * @param write 修改时间
     * @return 是否成功
     */
    bool set_all_times(const datetime& create, const datetime& access, const datetime& write) noexcept;
#else
    /**
     * @brief 设置所有时间
     * @param access 访问时间
     * @param write 修改时间
     * @return 是否成功
     */
    bool set_all_times(const datetime& access, const datetime& write) noexcept;
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
    /**
     * @brief 设置创建时间
     * @param dt 创建时间
     * @return 是否成功
     */
    bool set_creation_time(const datetime& dt) noexcept;
#endif

    /**
     * @brief 设置最后访问时间
     * @param dt 访问时间
     * @return 是否成功
     */
    bool set_last_access_time(const datetime& dt) noexcept;

    /**
     * @brief 设置最后修改时间
     * @param dt 修改时间
     * @return 是否成功
     */
    bool set_last_write_time(const datetime& dt) noexcept;

    /**
     * @brief 获取文件大小
     * @param p 文件路径
     * @return 文件大小
     */
    MSTL_NODISCARD static size_type size(const _MSTL path& p);

    /**
     * @brief 获取文件大小
     * @param p 文件路径
     * @param size 输出大小
     * @return 是否成功
     */
    static bool size(const _MSTL path& p, size_type& size);

    /**
     * @brief 创建并写入文件
     * @param p 文件路径
     * @param content 内容
     * @param append 是否追加
     * @return 是否成功
     */
    static bool create_and_write(const _MSTL path& p, const string& content, bool append = false);

    /**
     * @brief 读取文件内容
     * @param p 文件路径
     * @param content 输出内容
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 是否成功
     */
    static bool read(
        const _MSTL path& p, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    /**
     * @brief 读取文件内容
     * @param p 文件路径
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 文件内容
     */
    MSTL_NODISCARD static string read(
        const _MSTL path& p,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    /**
     * @brief 读取二进制文件
     * @param p 文件路径
     * @param content 输出内容
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 是否成功
     */
    static bool read_binary(
        const _MSTL path& p, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    /**
     * @brief 读取二进制文件
     * @param p 文件路径
     * @param creation 创建方式
     * @param attributes 文件属性
     * @return 文件内容
     */
    MSTL_NODISCARD static string read_binary(
        const _MSTL path& p,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);
};


/**
 * @class file_lock_guard
 * @brief 文件锁守卫
 *
 * RAII风格的自动文件锁管理类。
 */
class MSTL_API file_lock_guard {
public:
    using difference_type = file::difference_type;

private:
 file& file_;                ///< 文件引用
 difference_type offset_;    ///< 锁定偏移
 difference_type length_;    ///< 锁定长度
 bool locked_;               ///< 是否已锁定

public:
    /**
     * @brief 构造函数（自动加锁）
     * @param f 文件对象
     * @param offset 锁定偏移
     * @param length 锁定长度
     * @param mode 锁定模式
     */
    file_lock_guard(file& f, difference_type offset, difference_type length, FILE_LOCK mode);

    /**
     * @brief 析构函数（自动解锁）
     */
    ~file_lock_guard();

    file_lock_guard(const file_lock_guard&) = delete;
    file_lock_guard& operator =(const file_lock_guard&) = delete;

    /**
     * @brief 检查是否已锁定
     * @return 是否已锁定
     */
    MSTL_NODISCARD bool is_locked() const { return locked_; }

    /**
     * @brief 手动解锁
     * @return 是否成功
     */
    bool unlock();
};

/** @} */ // File

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_HPP__
