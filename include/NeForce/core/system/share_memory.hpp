#ifndef NEFORCE_CORE_SYSTEM_SHARE_MEMORY_HPP__
#define NEFORCE_CORE_SYSTEM_SHARE_MEMORY_HPP__

/**
 * @file share_memory.hpp
 * @brief 共享内存管理工具
 *
 * 此文件提供了跨平台的共享内存创建、映射和管理功能。
 * 支持共享内存创建、打开、映射、同步等操作。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @{
 */

/**
 * @struct share_memory_exception
 * @brief 共享内存操作异常
 */
struct share_memory_exception final : system_exception {
    explicit share_memory_exception(const char* info = "Shared Memory Operation Failed.",
                                    const char* type = static_type, const int code = 0) noexcept :
    system_exception(info, type, code) {}

    explicit share_memory_exception(const exception& e) :
    system_exception(e) {}

    ~share_memory_exception() override = default;
    static constexpr auto static_type = "shared_memory_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup ShareMemory 共享内存
 * @brief 共享内存管理工具
 * @{
 */

/**
 * @class share_memory
 * @brief 共享内存管理类
 *
 * 提供跨进程的共享内存创建、映射和访问功能。
 */
class NEFORCE_API share_memory {
public:
    /**
     * @enum access_mode
     * @brief 访问模式枚举
     */
    enum class access_mode {
        read_only = 0x01,  ///< 只读模式
        read_write = 0x02, ///< 读写模式
    };

    /**
     * @enum open_mode
     * @brief 打开模式枚举
     */
    enum class open_mode {
        create_only,   ///< 仅创建（已存在则失败）
        open_only,     ///< 仅打开（不存在则失败）
        open_or_create ///< 打开或创建
    };

    using native_handle_type = _NEFORCE native_handle_type;

private:
    native_handle_type handle_;                        ///< 映射句柄
    string name_;                                      ///< 共享内存名称
    size_t size_{0};                                   ///< 共享内存大小
    size_t internal_mapped_size_{0};                   ///< 含填充的映射大小
    size_t mapped_size_{0};                            ///< 映射大小
    void* original_mapped_addr_{nullptr};              ///< 原始映射地址
    void* mapped_addr_{nullptr};                       ///< 映射地址
    access_mode access_mode_{access_mode::read_write}; ///< 访问模式
    bool is_open_{false};                              ///< 是否已打开

public:
    /**
     * @brief 默认构造函数
     */
    share_memory() noexcept;

    /**
     * @brief 构造函数
     * @param name 共享内存名称
     * @param size 共享内存大小（字节）
     * @param mode 打开模式
     * @param access 访问模式
     * @throws share_memory_exception 创建或打开失败时抛出
     */
    explicit share_memory(const string& name, size_t size, open_mode mode = open_mode::open_or_create,
                          access_mode access = access_mode::read_write);

    share_memory(const share_memory&) = delete;
    share_memory& operator=(const share_memory&) = delete;

    /**
     * @brief 移动构造函数
     */
    share_memory(share_memory&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     */
    share_memory& operator=(share_memory&& other) noexcept;

    /**
     * @brief 析构函数
     */
    ~share_memory();

    /**
     * @brief 打开或创建共享内存
     * @param name 共享内存名称
     * @param size 共享内存大小（字节）
     * @param mode 打开模式
     * @param access 访问模式
     * @throws share_memory_exception 操作失败时抛出
     */
    void open(const string& name, size_t size, open_mode mode = open_mode::open_or_create,
              access_mode access = access_mode::read_write);

    /**
     * @brief 关闭共享内存
     */
    void close() noexcept;

    /**
     * @brief 映射共享内存到进程地址空间
     * @param offset 映射偏移量（字节）
     * @param length 映射长度（字节，0表示全部）
     * @return 映射地址
     * @throws share_memory_exception 映射失败时抛出
     */
    void* map(size_t offset = 0, size_t length = 0);

    /**
     * @brief 取消映射
     */
    void unmap() noexcept;

    /**
     * @brief 获取映射地址
     * @return 映射地址，未映射则返回nullptr
     */
    NEFORCE_NODISCARD void* data() const noexcept { return mapped_addr_; }

    /**
     * @brief 获取映射地址（类型转换版本）
     * @tparam T 目标类型
     * @return 类型转换后的地址
     */
    template <typename T>
    NEFORCE_NODISCARD T* data() const noexcept {
        return static_cast<T*>(mapped_addr_);
    }

    /**
     * @brief 获取共享内存大小
     * @return 共享内存大小（字节）
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_; }

    /**
     * @brief 获取映射大小
     * @return 映射大小（字节）
     */
    NEFORCE_NODISCARD size_t mapped_size() const noexcept { return mapped_size_; }

    /**
     * @brief 获取共享内存名称
     * @return 共享内存名称
     */
    NEFORCE_NODISCARD const string& name() const noexcept { return name_; }

    /**
     * @brief 检查是否已打开
     * @return 是否已打开
     */
    NEFORCE_NODISCARD bool is_open() const noexcept { return is_open_; }

    /**
     * @brief 检查是否已映射
     * @return 是否已映射
     */
    NEFORCE_NODISCARD bool is_mapped() const noexcept { return mapped_addr_ != nullptr; }

    /**
     * @brief 刷新共享内存到磁盘
     * @param async 是否异步刷新
     * @return 是否成功
     */
    bool flush(bool async = false);

    /**
     * @brief 删除共享内存对象
     * @param name 共享内存名称
     * @return 是否成功删除
     * @note 即使关闭句柄，如果还有其他进程持有句柄，对象仍会存在
     *       可以尝试打开以检查是否真的被移除
     */
    static bool remove(const string& name);

    /**
     * @brief 检查共享内存是否存在
     * @param name 共享内存名称
     * @return 是否存在
     */
    NEFORCE_NODISCARD static bool exists(const string& name);
};

/** @} */ // ShareMemory

NEFORCE_END_NAMESPACE__

#endif // NEFORCE_CORE_SYSTEM_SHARE_MEMORY_HPP__
