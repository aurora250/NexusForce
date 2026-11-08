#ifndef NEFORCE_CORE_MEMORY_BUMP_ARENA_HPP__
#define NEFORCE_CORE_MEMORY_BUMP_ARENA_HPP__

/**
 * @file bump_arena.hpp
 * @brief 按块分配、整块释放的线性分配器
 */

#include "NeForce/core/memory/standard_allocator.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup BumpArena 分块线性分配器
 * @{
 */

/**
 * @class bump_arena
 * @brief 按块分配、整块释放的线性分配器
 *
 * 所有对象都从按块申请的缓冲区中切分，析构时只释放整块。
 */
class bump_arena {
public:
    static constexpr size_t default_alignment = 8;                    ///< 默认对齐要求
    static constexpr size_t natural_alignment = alignof(max_align_t); ///< 平台自然对齐要求

    bump_arena() noexcept = default;
    ~bump_arena() { release(); }

    bump_arena(const bump_arena&) = delete;
    bump_arena& operator=(const bump_arena&) = delete;

    /**
     * @brief 移动构造
     * @param other 源对象
     */
    bump_arena(bump_arena&& other) noexcept :
    head_(other.head_),
    current_(other.current_),
    used_(other.used_),
    capacity_(other.capacity_) {
        other.head_ = nullptr;
        other.current_ = nullptr;
        other.used_ = 0;
        other.capacity_ = 0;
    }

    /**
     * @brief 移动赋值
     * @param other 源对象
     * @return 自身引用
     */
    bump_arena& operator=(bump_arena&& other) noexcept {
        if (this != &other) {
            release();
            head_ = other.head_;
            current_ = other.current_;
            used_ = other.used_;
            capacity_ = other.capacity_;
            other.head_ = nullptr;
            other.current_ = nullptr;
            other.used_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    /**
     * @brief 分配未初始化的连续内存
     * @param count 元素个数
     * @param size 单元素字节数
     * @param alignment 对齐要求，必须是 2 的幂
     * @return 内存起始地址
     * @throws allocate_exception 分配失败、参数非法或对齐要求无效时抛出
     */
    NEFORCE_NODISCARD void* allocate(const size_t count, const size_t size,
                                     const size_t alignment = default_alignment) {
        if (count == 0 || size == 0) {
            return nullptr;
        }
        if (!valid_alignment(alignment)) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("bump_arena: invalid alignment"));
        }
        if (count > static_cast<size_t>(-1) / size) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("bump_arena: allocation size overflow"));
        }
        const size_t bytes = count * size;
        const size_t mask = alignment - 1;
        size_t padding = 0;
        if (current_ == nullptr || (reinterpret_cast<uintptr_t>(current_) & mask) != 0) {
            grow(bytes, alignment);
        } else {
            padding = (alignment - (used_ & mask)) & mask;
            if (padding > capacity_ - used_ || bytes > capacity_ - used_ - padding) {
                grow(bytes + padding, alignment);
                padding = 0;
            }
        }
        used_ += padding;
        void* result = current_ + used_;
        used_ += bytes;
        return result;
    }

    /**
     * @brief 预留首块容量
     * @param bytes 期望的首块可用字节数
     * @param alignment 首块数据的对齐要求，必须是 2 的幂
     * @throws allocate_exception 分配失败、对齐要求无效或块大小无法表示时抛出
     * @note 只在尚未分配任何块时生效
     */
    void reserve(const size_t bytes, const size_t alignment = natural_alignment) {
        if (head_ != nullptr || bytes == 0) {
            return;
        }
        if (!valid_alignment(alignment)) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("bump_arena: invalid alignment"));
        }
        grow_capacity(bytes, alignment);
    }

    /**
     * @brief 尝试把最近一次分配原地扩容
     * @param pointer 原分配地址
     * @param old_bytes 原字节数
     * @param new_bytes 新字节数
     * @return 扩容成功返回 true，地址不变；否则返回 false，调用方需重新分配并拷贝
     * @note 仅当该分配是当前块的最后一次分配且块内余量足够时成功
     */
    NEFORCE_NODISCARD bool try_extend(const void* pointer, const size_t old_bytes, const size_t new_bytes) noexcept {
        if (pointer == nullptr || current_ == nullptr) {
            return false;
        }
        const auto base = reinterpret_cast<uintptr_t>(current_);
        const auto address = reinterpret_cast<uintptr_t>(pointer);
        if (address < base || address - base > used_) {
            return false;
        }
        const auto offset = static_cast<size_t>(address - base);
        if (used_ - offset != old_bytes || new_bytes > capacity_ - offset) {
            return false;
        }
        used_ = offset + new_bytes;
        return true;
    }

private:
    struct chunk {
        chunk* next;
        size_t bytes;
    };

    chunk* head_ = nullptr;     ///< 块链表头
    byte_t* current_ = nullptr; ///< 当前块写入位置
    size_t used_ = 0;           ///< 当前块已用字节数
    size_t capacity_ = 0;       ///< 当前块可用字节数

    NEFORCE_NODISCARD static constexpr bool valid_alignment(const size_t alignment) noexcept {
        return alignment != 0 && (alignment & (alignment - 1)) == 0;
    }

    void grow(const size_t bytes, const size_t alignment) {
        constexpr size_t chunk_size = static_cast<size_t>(64) * 1024;
        grow_capacity(bytes > chunk_size ? bytes : chunk_size, alignment);
    }

    void grow_capacity(const size_t capacity, const size_t alignment) {
        const size_t mask = alignment - 1;
        if (capacity > static_cast<size_t>(-1) - sizeof(chunk) - mask) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("bump_arena: chunk size overflow"));
        }
        const size_t total = sizeof(chunk) + mask + capacity;
        if (total > static_cast<size_t>(standard_allocator<byte_t>::max_size())) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("bump_arena: chunk exceeds allocator limit"));
        }
        auto* memory = static_cast<byte_t*>(standard_allocator<byte_t>::allocate(total));
        const auto base = reinterpret_cast<uintptr_t>(memory);
        const uintptr_t data = (base + sizeof(chunk) + mask) & ~static_cast<uintptr_t>(mask);
        const auto offset = static_cast<size_t>(data - base);
        auto* block = reinterpret_cast<chunk*>(memory);
        block->next = head_;
        block->bytes = total;
        head_ = block;
        current_ = memory + offset;
        used_ = 0;
        capacity_ = total - offset;
    }

    void release() noexcept {
        chunk* block = head_;
        while (block != nullptr) {
            chunk* next = block->next;
            standard_allocator<byte_t>::deallocate(reinterpret_cast<byte_t*>(block), block->bytes);
            block = next;
        }
        head_ = nullptr;
        current_ = nullptr;
        used_ = 0;
        capacity_ = 0;
    }
};

/** @} */ // BumpArena

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_BUMP_ARENA_HPP__
