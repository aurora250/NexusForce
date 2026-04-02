#ifndef NEFORCE_CORE_FILE_FILE_MAPPER_HPP__
#define NEFORCE_CORE_FILE_FILE_MAPPER_HPP__

/**
 * @file file_mapper.hpp
 * @brief 内存映射文件管理
 *
 * 此文件提供了内存映射文件的功能，允许将文件内容映射到进程的虚拟地址空间，
 * 从而可以像访问内存一样高效地读写文件。
 */

#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/async/mutex.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作相关类
 * @{
 */

/**
 * @class file_mapper
 * @brief 内存映射文件工具类
 *
 * 管理单个文件的内存映射生命周期，支持映射、解映射、重映射及刷新。
 * 提供对文件内容的直接内存访问，适用于需要频繁随机访问的场景。
 *
 * 主要功能：
 * - 映射文件到内存
 * - 解除映射
 * - 重新映射到不同区域
 * - 刷新映射内容到磁盘
 * - 锁定/解锁映射页到物理内存
 *
 * 性能优势：
 * - 避免系统调用开销
 * - 操作系统自动管理页面缓存
 * - 支持大文件的高效访问
 *
 * @note 映射的文件内容可能与其他进程不同步，需要适当使用flush()同步。
 * @note 多个线程同时操作同一映射区域需要外部同步。
 * @note 不持有文件句柄所有权，句柄生命周期由调用方保证。
 */
class NEFORCE_API file_mapper {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type = ::DWORD;  ///< 大小类型
#else
    using size_type = size_t;   ///< 大小类型
#endif

    using native_handle_type = _NEFORCE native_handle_type; ///< 原生文件句柄类型

    /**
     * @struct map_info
     * @brief 映射状态快照
     *
     * 提供当前映射状态的完整信息，用于调试和状态查询。
     */
    struct map_info {
        void* address = nullptr;    ///< 映射起始地址
        size_type size = 0;         ///< 映射大小（字节）
        size_type offset = 0;       ///< 映射起始偏移
        file_access access = file_access::READ; ///< 访问权限
        bool is_mapped = false;     ///< 是否已映射
    };

private:
    native_handle_type file_handle_;
    void* ptr_ = nullptr;
    size_type size_ = 0;
    size_type offset_ = 0;
    file_access access_ = file_access::READ;
    mutable mutex mutex_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    native_handle_type mapping_handle_ = INVALID_HANDLE_VALUE; ///< 映射句柄
#endif

public:
    /**
     * @brief 构造函数
     * @param file_handle 已打开的文件句柄
     *
     * 创建内存映射管理器，关联指定的文件句柄。
     * 初始状态为未映射。
     */
    explicit file_mapper(native_handle_type file_handle);

    /**
     * @brief 析构函数
     *
     * 自动解除映射并释放相关资源。
     */
    ~file_mapper();

    file_mapper(const file_mapper&) = delete;
    file_mapper& operator =(const file_mapper&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 要移动的对象
     */
    file_mapper(file_mapper&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的对象
     * @return 自身引用
     */
    file_mapper& operator =(file_mapper&& other) noexcept;

    /**
     * @brief 映射文件到内存
     * @param offset 映射起始偏移（字节）
     * @param size 映射长度（字节），0表示映射到文件末尾
     * @param access 访问模式
     * @param hint 访问模式提示，用于优化页面预取
     * @return 映射成功返回true，失败返回false
     *
     * 将文件的指定区域映射到进程地址空间。
     * 如果offset不是系统页面大小的整数倍，会自动对齐。
     * size为0时映射从offset到文件末尾的所有内容。
     *
     * @note 映射后可以通过data()指针直接访问文件内容。
     * @note 写操作可能需要权限。
     */
    bool map(size_type offset = 0, size_type size = 0,
             file_access access = file_access::READ,
             file_map_hint hint = file_map_hint::SEQUENTIAL);

    /**
     * @brief 解除映射
     *
     * 解除当前映射，释放映射内存。
     */
    void unmap() noexcept;

    /**
     * @brief 重新映射到新区域
     * @param new_offset 新的起始偏移
     * @param new_size 新的映射大小
     * @return 重新映射成功返回true
     *
     * 解除当前映射，然后创建新的映射。
     * 保持相同的访问权限。
     */
    bool remap(size_type new_offset, size_type new_size);

    /**
     * @brief 刷新映射区域到磁盘
     * @param async 是否异步刷新
     * @return 刷新成功返回true
     *
     * 将映射内存中修改的内容写回磁盘文件。
     * 异步刷新可提高性能但可能丢失数据。
     */
    bool flush(bool async = false) noexcept;

    /**
     * @brief 锁定/解锁映射页到物理内存
     * @param lock_in_memory true锁定，false解锁
     * @return 操作成功返回true
     *
     * 锁定映射页到物理内存，防止被换出到交换空间。
     * 适用于对延迟敏感的应用场景。
     *
     * @note 解锁时只释放锁定，不会解除映射。
     * @note 可能需要权限。
     */
    bool lock_pages(bool lock_in_memory) const noexcept;

    /**
     * @brief 获取映射起始地址指针
     */
    NEFORCE_NODISCARD void* data() const noexcept { return ptr_; }

    /**
     * @brief 获取映射字节大小
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return size_; }

    /**
     * @brief 获取映射偏移
     */
    NEFORCE_NODISCARD size_type offset() const noexcept { return offset_; }

    /**
     * @brief 获取访问权限
     */
    NEFORCE_NODISCARD file_access access() const noexcept { return access_; }

    /**
     * @brief 检查是否已映射
     */
    NEFORCE_NODISCARD bool is_mapped() const noexcept { return ptr_ != nullptr; }

    /**
     * @brief 获取映射状态快照
     */
    NEFORCE_NODISCARD map_info info() const noexcept;
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_MAPPER_HPP__
