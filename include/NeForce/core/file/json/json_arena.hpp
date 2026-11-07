#ifndef NEFORCE_CORE_FILE_JSON_JSON_ARENA_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_ARENA_HPP__

/**
 * @file json_arena.hpp
 * @brief JSON 文档的分块 bump 分配器
 */

#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup JsonConfig JSON配置
 * @{
 */

/**
 * @class json_arena
 * @brief 按块分配、整块释放的线性分配器
 *
 * DOM 的所有节点、成员数组与字符串都从 arena 分配，析构时只释放整块，
 * 因此无论文档多大、嵌套多深，销毁都是 O(块数) 且不递归。
 */
class json_arena {
public:
    json_arena() noexcept = default;

    ~json_arena() { release(); }

    json_arena(const json_arena&) = delete;
    json_arena& operator=(const json_arena&) = delete;

    /**
     * @brief 移动构造
     * @param other 源对象
     */
    json_arena(json_arena&& other) noexcept :
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
    json_arena& operator=(json_arena&& other) noexcept {
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
     * @return 内存起始地址
     * @throws memory_exception 分配失败时抛出
     * @note 内存按 8 字节对齐，块内连续
     */
    NEFORCE_NODISCARD void* allocate(const size_t count, const size_t size) {
        const size_t bytes = count * size;
        if (bytes == 0) {
            return nullptr;
        }
        const size_t aligned = (bytes + 7) & ~static_cast<size_t>(7);
        if (current_ == nullptr || used_ + aligned > capacity_) {
            grow(aligned);
        }
        void* result = current_ + used_;
        used_ += aligned;
        return result;
    }

    /**
     * @brief 预留首块容量
     * @param bytes 期望的首块可用字节数
     * @throws memory_exception 分配失败时抛出
     * @note 只在尚未分配任何块时生效；按输入规模一次性分块，避免小文档也占用整块容量
     */
    void reserve(const size_t bytes) {
        if (head_ != nullptr || bytes == 0) {
            return;
        }
        grow_capacity(bytes);
    }

    /**
     * @brief 尝试把最近一次分配原地扩容
     * @param pointer 原分配地址
     * @param old_bytes 原字节数
     * @param new_bytes 新字节数
     * @return 扩容成功返回 true，此时地址不变；否则返回 false，调用方需重新分配并拷贝
     * @note 仅当该分配是当前块的最后一次分配且块内余量足够时成功
     */
    NEFORCE_NODISCARD bool try_extend(const void* pointer, const size_t old_bytes, const size_t new_bytes) noexcept {
        if (pointer == nullptr || current_ == nullptr) {
            return false;
        }
        const size_t old_aligned = (old_bytes + 7) & ~static_cast<size_t>(7);
        const size_t new_aligned = (new_bytes + 7) & ~static_cast<size_t>(7);
        const auto* tail = static_cast<const byte_t*>(pointer) + old_aligned;
        if (tail != current_ + used_ || used_ - old_aligned + new_aligned > capacity_) {
            return false;
        }
        used_ = used_ - old_aligned + new_aligned;
        return true;
    }

private:
    static constexpr size_t chunk_size_ = static_cast<size_t>(64) * 1024;

    struct chunk {
        chunk* next;
        alignas(8) byte_t data[1];
    };

    chunk* head_ = nullptr;     ///< 块链表头
    byte_t* current_ = nullptr; ///< 当前块写入位置
    size_t used_ = 0;           ///< 当前块已用字节数
    size_t capacity_ = 0;       ///< 当前块容量

    void grow(const size_t bytes) { grow_capacity(bytes > chunk_size_ ? bytes : chunk_size_); }

    void grow_capacity(const size_t capacity) {
        void* memory = ::operator new(sizeof(chunk) + capacity);
        auto* block = static_cast<chunk*>(memory);
        block->next = head_;
        head_ = block;
        current_ = block->data;
        used_ = 0;
        capacity_ = capacity;
    }

    void release() noexcept {
        chunk* block = head_;
        while (block != nullptr) {
            chunk* next = block->next;
            ::operator delete(block);
            block = next;
        }
        head_ = nullptr;
        current_ = nullptr;
        used_ = 0;
        capacity_ = 0;
    }
};

/** @} */ // JsonConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_ARENA_HPP__
