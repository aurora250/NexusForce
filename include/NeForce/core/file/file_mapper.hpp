#ifndef NEFORCE_CORE_FILE_FILE_MAPPER_HPP__
#define NEFORCE_CORE_FILE_FILE_MAPPER_HPP__
#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/async/mutex.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class file_mapper
 * @brief 内存映射文件工具类
 *
 * 管理单个文件的内存映射生命周期，支持映射、解映射、重映射及刷新。
 */
class NEFORCE_API file_mapper {
public:
#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type = ::DWORD;
#else
    using size_type = size_t;
#endif

    using native_handle_type = _NEFORCE native_handle_type;

    /**
     * @struct map_info
     * @brief 映射状态快照
     */
    struct map_info {
        void* address = nullptr;
        size_type size = 0;
        size_type offset = 0;
        file_access access = file_access::READ;
        bool is_mapped = false;
    };

private:
    native_handle_type file_handle_;
    void* ptr_ = nullptr;
    size_type size_ = 0;
    size_type offset_ = 0;
    file_access access_ = file_access::READ;
    mutable mutex mutex_;

#ifdef NEFORCE_PLATFORM_WINDOWS
    native_handle_type mapping_handle_ = INVALID_HANDLE_VALUE;
#endif

public:
    explicit file_mapper(native_handle_type file_handle);
    ~file_mapper();

    file_mapper(const file_mapper&) = delete;
    file_mapper& operator =(const file_mapper&) = delete;
    file_mapper(file_mapper&& other) noexcept;
    file_mapper& operator =(file_mapper&& other) noexcept;

    /**
     * @brief 映射文件到内存
     * @param offset 映射起始偏移
     * @param size 映射长度（0 表示映射到文件末尾）
     * @param access 访问模式
     * @param hint 访问模式提示
     */
    bool map(size_type offset = 0, size_type size = 0,
             file_access access = file_access::READ,
             file_map_hint hint = file_map_hint::SEQUENTIAL);

    /**
     * @brief 解除映射
     */
    void unmap() noexcept;

    /**
     * @brief 重新映射到新区域
     */
    bool remap(size_type new_offset, size_type new_size);

    /**
     * @brief 刷新映射区域到磁盘
     * @param async 是否异步刷新
     */
    bool flush(bool async = false) noexcept;

    /**
     * @brief 锁定/解锁映射页到物理内存
     */
    bool lock_pages(bool lock_in_memory) const noexcept;

    NEFORCE_NODISCARD void* data() const noexcept { return ptr_; }
    NEFORCE_NODISCARD size_type size() const noexcept { return size_; }
    NEFORCE_NODISCARD size_type offset() const noexcept { return offset_; }
    NEFORCE_NODISCARD file_access access() const noexcept { return access_; }
    NEFORCE_NODISCARD bool is_mapped() const noexcept { return ptr_ != nullptr; }
    NEFORCE_NODISCARD map_info info() const noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_MAPPER_HPP__
