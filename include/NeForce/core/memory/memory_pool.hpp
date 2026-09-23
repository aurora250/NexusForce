#ifndef NEFORCE_CORE_MEMORY_MEMORY_POOL_HPP__
#define NEFORCE_CORE_MEMORY_MEMORY_POOL_HPP__

/**
 * @file memory_pool.hpp
 * @brief 高性能内存池
 *
 * 此文件提供了面向通用场景的高性能内存分配器实现。
 * 小对象（不超过 16 KiB）由 16 字节对齐的尺寸类 span 承担，块内不携带任何元数据，
 * 尺寸类与归属信息通过进程级地址映射表（64 KiB 槽位）查询；
 * 大对象直接从操作系统映射，映射参数记录在区域头部。
 * 线程本地缓存承担绝大部分分配与释放，仅当缓存耗尽或溢出时才以批次为单位访问中央堆。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/memory/bit.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup MemoryPool 内存池
 * @brief 高性能内存分配器
 * @{
 */

/**
 * @class memory_pool
 * @brief 高性能内存池
 *
 * 线程本地缓存、按尺寸类组织的中央 span 堆、操作系统虚拟内存层三层结构的内存池
 */
class NEFORCE_API memory_pool {
public:
    /**
     * @struct spinlock
     * @brief 自旋锁
     */
    struct spinlock {
        atomic<uint32_t> value{0}; ///< 锁状态
    };

    /**
     * @struct map_entry
     * @brief 地址映射表条目
     */
    struct map_entry {
        atomic<uint64_t> desc; ///< span 头部或区域头部地址
        atomic<uint64_t> tag;  ///< 归属标识、尺寸类与槽位类型的打包值
    };

    /**
     * @struct span_header
     * @brief span 头部
     */
    struct span_header {
        uint32_t magic;              ///< 魔数
        uint32_t owner_id;           ///< 归属内存池标识
        uint32_t class_index;        ///< 尺寸类下标
        uint32_t span_size;          ///< span 总字节数
        uint32_t block_count;        ///< span 内块总数
        uint32_t flags;              ///< 状态标志，最低位表示已挂入尺寸类链
        atomic<uint32_t> free_count; ///< 位于 span 空闲链上的块数
        atomic<uint32_t> used_count; ///< 已交付给线程缓存或正在使用的块数
        void* free_head;             ///< span 空闲链头
        span_header* next;           ///< 尺寸类 span 链后继
        span_header* prev;           ///< 尺寸类 span 链前驱
        span_header* global_next;    ///< 全 span 链后继
    };

    /**
     * @struct class_state
     * @brief 单个尺寸类的中央堆状态
     */
    struct class_state {
        spinlock lock;        ///< 该尺寸类的锁
        span_header* partial; ///< 含空闲块的 span 链，完全空闲者位于链尾
        span_header* tail;    ///< span 链尾
        size_t span_count;    ///< span 总数
        size_t empty_count;   ///< 完全空闲 span 数
        size_t span_size;     ///< span 大小
        size_t block_count;   ///< 每个 span 的块数
        uint32_t batch;       ///< 单次搬运的块数，按字节目标推导
        uint32_t cache_max;   ///< 线程缓存溢出阈值，取搬运批量的两倍
    };

    /**
     * @brief 尺寸类数量
     */
    static constexpr size_t class_count = 36;

    /**
     * @def MEMORY_POOL_THREAD_REGION_SLOTS
     * @brief 每个线程保留的大对象区域槽位数
     */
    static constexpr size_t thread_region_slots = 4;

    /**
     * @brief 小对象尺寸上限（16 KiB）
     */
    static constexpr size_t small_max = 16384;

    /**
     * @brief 分配器保证的最小对齐（字节）
     */
    static constexpr size_t min_align = 16;

    /**
     * @brief 大对象区域缓存槽位数上限
     */
    static constexpr size_t region_cache_slots = 16;

    /**
     * @struct options
     * @brief 内存池配置
     */
    struct options {
        size_t thread_cache_max = 64;             ///< 每个尺寸类在线程缓存中保留的块数上限
        size_t thread_cache_batch = 32;           ///< 与中央堆交互时每次搬运的块数下限
        size_t thread_cache_bytes = 4096;         ///< 每个尺寸类在线程缓存中保留的字节目标，决定搬运批量
        size_t thread_cache_max_bytes = 1U << 20; ///< 每个尺寸类线程缓存容量的字节上限，决定溢出阈值
        size_t max_empty_spans = 1;               ///< 每个尺寸类缓存的完全空闲 span 数下限
        size_t max_empty_span_bytes = 2U << 20;   ///< 全池保留完全空闲 span 的字节预算
        size_t max_cached_regions = 8;            ///< 全池大对象区域缓存条目数上限
        size_t thread_region_bytes = 256U << 10;  ///< 线程保留单个大对象区域的字节上限
        bool thread_cache_enabled = true;         ///< 是否启用线程本地缓存
        bool region_cache_enabled = true;         ///< 是否启用大对象区域复用缓存
        bool purge_on_empty = false;              ///< 是否在 span 完全空闲时立即归还操作系统
    };

    /**
     * @struct class_statistics
     * @brief 单个尺寸类的统计信息
     */
    struct class_statistics {
        size_t block_size = 0;    ///< 块大小（字节）
        size_t span_size = 0;     ///< span 大小（字节），按操作系统页大小向上取整
        size_t active_blocks = 0; ///< 已交付使用的块数
        size_t active_bytes = 0;  ///< 已交付使用的字节数
        size_t span_count = 0;    ///< span 总数
        size_t empty_spans = 0;   ///< 完全空闲且被保留的 span 数
    };

    /**
     * @struct statistics
     * @brief 内存池统计信息
     */
    struct statistics {
        size_t active_blocks = 0;              ///< 已交付使用的块总数（仅小对象）
        size_t active_bytes = 0;               ///< 已交付使用的字节总数（仅小对象）
        size_t peak_active_bytes = 0;          ///< 已交付使用字节数的峰值
        size_t mapped_bytes = 0;               ///< 当前自操作系统映射的字节总数
        size_t peak_mapped_bytes = 0;          ///< 自操作系统映射字节数的峰值
        size_t small_mapped_bytes = 0;         ///< 当前由 span 占用的映射字节数
        size_t large_mapped_bytes = 0;         ///< 当前由大对象区域占用的映射字节数
        size_t cached_empty_bytes = 0;         ///< 保留的完全空闲 span 字节数
        size_t cached_region_bytes = 0;        ///< 保留的大对象区域字节数
        size_t os_map_calls = 0;               ///< 自操作系统映射的次数
        size_t os_unmap_calls = 0;             ///< 归还操作系统的次数
        class_statistics classes[class_count]; ///< 各尺寸类统计
    };

public:
    /**
     * @brief 默认构造函数
     *
     * 使用默认配置构造内存池，构造过程不进行任何内存映射。
     */
    memory_pool() noexcept;

    /**
     * @brief 构造函数
     * @param opts 内存池配置
     */
    explicit memory_pool(const options& opts) noexcept;

    /**
     * @brief 析构函数
     * @warning 析构前必须确保所有已分配内存均已释放，且没有线程正在使用本实例
     */
    ~memory_pool();

    memory_pool(const memory_pool&) = delete;
    memory_pool& operator=(const memory_pool&) = delete;
    memory_pool(memory_pool&&) = delete;
    memory_pool& operator=(memory_pool&&) = delete;

    /**
     * @brief 分配内存
     * @param bytes 请求字节数，为 0 时按 1 字节处理
     * @param align 对齐要求
     * @return 已分配内存指针
     * @throws allocate_exception 当操作系统拒绝提供内存时抛出
     */
    NEFORCE_NODISCARD void* allocate(size_t bytes, size_t align = min_align) {
        void* block = try_allocate(bytes, align);
        if (block == nullptr) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("memory pool allocate failed"));
        }
        return block;
    }

    /**
     * @brief 分配内存（不抛出异常）
     * @param bytes 请求字节数，为 0 时按 1 字节处理
     * @param align 对齐要求
     * @return 已分配内存指针，失败时返回 nullptr
     */
    NEFORCE_NODISCARD void* try_allocate(size_t bytes, size_t align = min_align) noexcept {
        const size_t request = bytes == 0 ? 1 : bytes;
        if (request <= small_max && align <= min_align) {
            const size_t class_index = size_to_class(request);
            if (class_index < class_count) {
                thread_cache& cache = current_cache();
                if (cache.owner_id == id_ && cache.entries[class_index].count != 0) {
                    void* block = cache.entries[class_index].list;
                    cache.entries[class_index].list = *static_cast<void**>(block);
                    --cache.entries[class_index].count;
                    return block;
                }
                return allocate_small(cache, class_index);
            }
        }
        return allocate_large(request, align == 0 ? min_align : align);
    }

    /**
     * @brief 释放内存
     * @param ptr 待释放指针，可为 nullptr
     * @param bytes 分配时的请求字节数
     */
    void deallocate(void* ptr, size_t bytes = 0) noexcept;

    /**
     * @brief 调整内存大小
     * @param ptr 待调整指针，可为 nullptr
     * @param bytes 新的请求字节数
     * @param align 对齐要求
     * @return 调整后的内存指针
     * @throws allocate_exception 当需要重新分配且操作系统拒绝提供内存时抛出
     */
    NEFORCE_NODISCARD void* reallocate(void* ptr, size_t bytes, size_t align = min_align);

    /**
     * @brief 查询指针指向内存的可用字节数
     * @param ptr 已分配指针
     * @return 可用字节数，指针不属于本实例时返回 0
     */
    NEFORCE_NODISCARD size_t usable_size(const void* ptr) const noexcept;

    /**
     * @brief 判断指针是否属于本实例
     * @param ptr 待判定指针
     * @return 属于本实例返回 true
     */
    NEFORCE_NODISCARD bool owns(const void* ptr) const noexcept;

    /**
     * @brief 归还当前线程的缓存
     */
    void flush_thread_cache() noexcept;

    /**
     * @brief 归还所有可归还内存
     */
    void purge() noexcept;

    /**
     * @brief 收集统计信息
     * @return 统计信息快照
     * @note 结果为近似快照
     */
    NEFORCE_NODISCARD statistics stats() noexcept;

    /**
     * @brief 校验内存池内部一致性
     * @return 一致返回 true
     */
    NEFORCE_NODISCARD bool verify() noexcept;

    /**
     * @brief 查询本实例标识
     */
    NEFORCE_NODISCARD uint32_t id() const noexcept;

    /**
     * @brief 查询配置
     * @return 配置引用
     */
    NEFORCE_NODISCARD const options& config() const noexcept;

    /**
     * @brief 查询尺寸类对应的块大小
     * @param class_index 尺寸类下标
     * @return 块大小（字节），下标越界时返回 0
     */
    NEFORCE_NODISCARD static size_t block_size(size_t class_index) noexcept;

    /**
     * @brief 查询请求字节数对应的尺寸类
     * @param bytes 请求字节数
     * @return 尺寸类下标，超过小对象上限时返回 class_count
     */
    NEFORCE_NODISCARD static size_t size_to_class(size_t bytes) noexcept {
        if (bytes <= 256) {
            return small_size_class_table[bytes == 0 ? 0 : bytes - 1];
        }
        if (bytes > small_max) {
            return class_count;
        }
        const auto value = bytes - 1;
        const auto width = static_cast<size_t>(bit_width(static_cast<uintptr_t>(value)));
        const size_t index = (value >> shift_by_width[width]) + offset_by_width[width];
        return index < class_count ? index : class_count;
    }

private:
    /**
     * @struct region_header
     * @brief 大对象区域头部
     */
    struct region_header {
        uint32_t magic;             ///< 魔数
        uint32_t owner_id;          ///< 归属内存池标识
        uint64_t region_size;       ///< 映射区域总字节数
        uint32_t payload_offset;    ///< 载荷相对区域起始的偏移
        uint32_t flags;             ///< 状态标志，最低位表示已进入区域缓存
        void* base;                 ///< 区域起始地址
        region_header* next;        ///< 区域缓存链后继
        region_header* global_next; ///< 全区域链后继
        region_header* global_prev; ///< 全区域链前驱
    };

    /**
     * @struct region_cache_entry
     * @brief 大对象区域缓存条目
     */
    struct region_cache_entry {
        region_header* region; ///< 缓存区域头部
        size_t region_size;    ///< 区域字节数
    };


    /**
     * @struct thread_cache
     * @brief 线程本地缓存
     */
    struct class_cache {
        void* list;     ///< 该尺寸类的空闲块链头
        uint32_t count; ///< 该尺寸类的空闲块数量
        uint32_t pad_;  ///< 对齐填充，使每个尺寸类恰好占用一条缓存行
    };

    /**
     * @struct thread_cache
     * @brief 线程缓存项
     */
    struct thread_cache {
        uint32_t owner_id;                               ///< 当前绑定内存池标识，0 表示未绑定
        uint32_t region_count;                           ///< 本线程保留的大对象区域数
        class_cache entries[class_count];                ///< 各尺寸类的空闲块链，按类成对存放
        region_cache_entry regions[thread_region_slots]; ///< 本线程保留的大对象区域
    };

    void initialize(const options& opts) noexcept;
    void release_all() noexcept;
    bool bind_cache(thread_cache& cache) noexcept;
    void* refill(thread_cache& cache, size_t class_index) noexcept;
    void return_blocks(size_t class_index, void* head, size_t count) noexcept;
    span_header* create_span(size_t class_index) noexcept;
    void destroy_span(span_header* span) noexcept;
    void* allocate_large(size_t bytes, size_t align) noexcept;
    void deallocate_large(region_header* region) noexcept;
    void return_region(region_header* region) noexcept;
    void release_region(region_header* region) noexcept;
    void* allocate_small(thread_cache& cache, size_t class_index) noexcept;
    static thread_cache& current_cache() noexcept;
    static void release_thread_cache(thread_cache& cache) noexcept;
    static memory_pool* pool_for_id(uint32_t id) noexcept;

    static constexpr uint8_t small_size_class_table[256] = {
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
            1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
            3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
            4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  6,
            6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,
            8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
            8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
            10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
            11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11};

    static constexpr uint8_t shift_by_width[16] = {4, 4, 4, 4, 4, 4, 4, 4, 5, 6, 7, 8, 9, 10, 11, 11};
    static constexpr uint8_t offset_by_width[16] = {0, 0, 0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 20, 24, 28, 28};

    uint32_t id_;
    uint32_t alive_;
    options options_;
    class_state classes_[class_count];
    span_header* all_spans_;
    spinlock registry_lock_;
    region_header* all_regions_;
    region_cache_entry region_cache_[region_cache_slots];
    size_t region_cache_count_;
    spinlock region_lock_;
    atomic<size_t> os_mapped_bytes_;
    atomic<size_t> peak_mapped_bytes_;
    atomic<size_t> small_mapped_bytes_;
    atomic<size_t> large_mapped_bytes_;
    atomic<size_t> active_bytes_;
    atomic<size_t> empty_bytes_;
    atomic<size_t> peak_active_bytes_;
    atomic<size_t> os_map_calls_;
    atomic<size_t> os_unmap_calls_;
};

/**
 * @brief 获取进程级系统内存池
 * @return 系统内存池引用
 */
NEFORCE_NODISCARD memory_pool& NEFORCE_API system_memory_pool() noexcept;


/**
 * @class pool_allocator
 * @brief 内存池分配器适配器
 * @tparam T 要分配的元素类型
 * @note 可绑定到任意 memory_pool 实例
 */
template <typename T>
class pool_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    using value_type = T;              ///< 元素类型
    using pointer = T*;                ///< 指针类型
    using const_pointer = const T*;    ///< 常量指针类型
    using size_type = size_t;          ///< 大小类型
    using difference_type = ptrdiff_t; ///< 差值类型

    /**
     * @struct rebind
     * @brief 重新绑定模板
     * @tparam U 新的元素类型
     */
    template <typename U>
    struct rebind {
        using other = pool_allocator<U>;
    };

    /**
     * @brief 默认构造函数
     *
     * 绑定系统内存池
     */
    pool_allocator() noexcept :
    pool_(&_NEFORCE system_memory_pool()) {}

    /**
     * @brief 构造函数
     * @param pool 绑定的内存池实例
     */
    explicit pool_allocator(memory_pool& pool) noexcept :
    pool_(&pool) {}

    /**
     * @brief 从其他元素类型的分配器转换构造
     * @tparam U 源元素类型
     * @param other 源分配器
     */
    template <typename U>
    pool_allocator(const pool_allocator<U>& other) noexcept :
    pool_(other.pool()) {}

    /**
     * @brief 分配内存
     * @return 指向分配内存的指针
     * @throws allocate_exception 当内存分配失败时抛出
     */
    NEFORCE_ALLOC_NODISCARD pointer allocate() {
        return static_cast<pointer>(pool_->allocate(sizeof(value_type), alignof(value_type)));
    }

    /**
     * @brief 分配内存
     * @param n 元素数量
     * @return 指向分配内存的指针
     * @throws allocate_exception 当内存分配失败时抛出
     */
    NEFORCE_ALLOC_NODISCARD pointer allocate(size_type n) {
        return static_cast<pointer>(pool_->allocate(sizeof(value_type) * n, alignof(value_type)));
    }

    /**
     * @brief 释放内存
     * @param p 待释放指针
     */
    void deallocate(pointer p) noexcept { pool_->deallocate(p, sizeof(value_type)); }

    /**
     * @brief 释放内存
     * @param p 待释放指针
     * @param n 元素数量
     */
    void deallocate(pointer p, size_type n) noexcept { pool_->deallocate(p, sizeof(value_type) * n); }

    /**
     * @brief 查询可分配的最大元素数量
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1) / sizeof(value_type); }

    /**
     * @brief 查询绑定的内存池
     * @return 内存池指针
     */
    NEFORCE_NODISCARD memory_pool* pool() const noexcept { return pool_; }

private:
    memory_pool* pool_;
};

/**
 * @brief 比较两个分配器是否相等
 * @tparam T 第一个分配器元素类型
 * @tparam U 第二个分配器元素类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 绑定同一内存池时返回 true
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator==(const pool_allocator<T>& lhs, const pool_allocator<U>& rhs) noexcept {
    return lhs.pool() == rhs.pool();
}

/**
 * @brief 比较两个分配器是否不等
 * @tparam T 第一个分配器元素类型
 * @tparam U 第二个分配器元素类型
 * @param lhs 左操作数
 * @param rhs 右操作数
 * @return 绑定不同内存池时返回 true
 */
template <typename T, typename U>
NEFORCE_NODISCARD bool operator!=(const pool_allocator<T>& lhs, const pool_allocator<U>& rhs) noexcept {
    return lhs.pool() != rhs.pool();
}

/** @} */ // MemoryPool

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_MEMORY_POOL_HPP__
