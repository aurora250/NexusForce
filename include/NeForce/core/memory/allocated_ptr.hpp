#ifndef NEFORCE_CORE_MEMORY_ALLOCATED_PTR_HPP__
#define NEFORCE_CORE_MEMORY_ALLOCATED_PTR_HPP__

/**
 * @file allocated_ptr.hpp
 * @brief 分配器指针包装器
 *
 * 此文件提供了分配器指针的RAII包装，用于安全管理通过分配器分配的内存。
 */

#include "NeForce/core/memory/allocator_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AllocatedPtr 分配内存指针
 * @brief 管理分配内存的智能指针工具
 * @{
 */

/**
 * @struct allocated_ptr
 * @brief 分配内存指针
 * @tparam Alloc 分配器类型
 *
 * 此类封装了一个分配的内存块及其对应的分配器。
 */
template <typename Alloc> struct allocated_ptr {
    using pointer = typename allocator_traits<Alloc>::pointer;       ///< 分配器指针类型
    using value_type = typename allocator_traits<Alloc>::value_type; ///< 元素类型

private:
    Alloc* alloc_; ///< 指向分配器的指针
    pointer ptr_;  ///< 分配的内存指针

public:
    /**
     * @brief 构造函数
     * @param alloc 分配器引用
     * @param ptr 分配的内存指针
     * @throw 无
     *
     * 从分配器和内存指针构造allocated_ptr。
     */
    allocated_ptr(Alloc& alloc, pointer ptr) noexcept :
    alloc_(_NEFORCE addressof(alloc)),
    ptr_(ptr) {}

    /**
     * @brief 原始指针转换
     * @tparam Ptr 原始指针类型
     * @param alloc 分配器引用
     * @param ptr 原始指针
     *
     * 从原始指针构造，转换为分配器的指针类型。
     */
    template <typename Ptr, typename = enable_if_t<is_same_v<Ptr, value_type*>>>
    allocated_ptr(Alloc& alloc, Ptr ptr) :
    alloc_(_NEFORCE addressof(alloc)),
    ptr_(pointer_traits<pointer>::pointer_to(*ptr)) {}

    /**
     * @brief 移动构造函数
     * @param guard 要移动的allocated_ptr
     */
    allocated_ptr(allocated_ptr&& guard) noexcept :
    alloc_(guard.alloc_),
    ptr_(guard.ptr_) {
        guard.ptr_ = nullptr;
    }

    /**
     * @brief 析构函数
     */
    ~allocated_ptr() {
        if (ptr_ != nullptr) {
            _NEFORCE allocator_traits<Alloc>::deallocate(*alloc_, ptr_, 1);
        }
    }

    /**
     * @brief 设置为空指针
     * @param null 空指针字面量
     * @return 当前对象的引用
     *
     * 放弃内存所有权，不会释放内存。
     */
    allocated_ptr& operator=(nullptr_t null) noexcept {
        ptr_ = nullptr;
        return *this;
    }

    /**
     * @brief 获取原始指针
     * @return 指向元素的原始指针
     *
     * 返回可访问的原始指针，用于直接访问内存。
     */
    value_type* get() { return _NEFORCE to_address(ptr_); }
};

/**
 * @brief 分配内存并创建allocated_ptr
 * @tparam Alloc 分配器类型
 * @param alloc 分配器引用
 * @return 管理分配内存的allocated_ptr
 *
 * 使用分配器分配单个元素内存，并返回管理该内存的allocated_ptr。
 */
template <typename Alloc> allocated_ptr<Alloc> allocate_guarded(Alloc& alloc) {
    return allocated_ptr<Alloc>{alloc, _NEFORCE allocator_traits<Alloc>::allocate(alloc, 1)};
}

/** @} */ // AllocatedPtr

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_ALLOCATED_PTR_HPP__
