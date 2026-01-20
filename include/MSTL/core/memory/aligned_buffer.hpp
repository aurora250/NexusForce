#ifndef MSTL_CORE_MEMORY_ALIGNED_BUFFER_HPP__
#define MSTL_CORE_MEMORY_ALIGNED_BUFFER_HPP__

/**
 * @file aligned_buffer.hpp
 * @brief MSTL对齐缓冲区实现
 *
 * 此文件提供了对齐缓冲区的实现，用于在栈上创建具有正确对齐的内存块，
 * 适用于需要在栈上分配对齐内存的场景，如小型对象优化等。
 */

#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup AlignedBuffer 对齐缓冲区
 * @brief 提供类型安全的内存对齐缓冲区
 * @{
 */

/**
 * @struct aligned_buffer
 * @brief 对齐缓冲区模板类
 * @tparam T 要存储的类型
 *
 * 提供类型安全的对齐内存缓冲区，确保内存的对齐方式与类型T一致。
 */
template <typename T>
struct aligned_buffer : aligned_storage<sizeof(T), alignof(T)> {
    /**
     * @brief 实际的存储缓冲区
     *
     * 使用aligned_storage_t确保内存具有正确的对齐方式。
     */
    aligned_storage_t<sizeof(T), alignof(T)> storage;

    /**
     * @brief 默认构造函数
     *
     * 创建一个未初始化的对齐缓冲区。
     */
    aligned_buffer() = default;

    /**
     * @brief nullptr_t构造函数
     * @param null nullptr_t参数
     *
     * 允许从nullptr构造对齐缓冲区，创建一个未初始化的缓冲区。
     */
    aligned_buffer(nullptr_t null) {}

    /**
     * @brief 获取缓冲区的原始地址
     * @return 指向缓冲区内存的void指针
     */
    void* addr() noexcept { return static_cast<void*>(&storage); }

    /**
     * @brief 获取缓冲区的原始常量地址
     * @return 指向缓冲区内存的const void指针
     */
    const void* addr() const noexcept { return static_cast<const void*>(&storage); }

    /**
     * @brief 获取缓冲区的类型化指针
     * @return 指向缓冲区内存的T类型指针
     *
     * 将原始内存解释为类型T的指针，但不构造对象。
     */
    T* ptr() noexcept { return static_cast<T*>(addr()); }

    /**
     * @brief 获取缓冲区的类型化常量指针
     * @return 指向缓冲区内存的const T类型指针
     */
    const T* ptr() const noexcept { return static_cast<const T*>(addr()); }
};

/** @} */ // AlignedBuffer

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_ALIGNED_BUFFER_HPP__
