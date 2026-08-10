#ifndef NEFORCE_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
#define NEFORCE_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__

/**
 * @file standard_allocator.hpp
 * @brief 标准分配器
 *
 * 此文件提供了标准分配器实现，
 * 用于管理动态内存分配和释放，支持不同对齐要求和编译器优化。
 */

#include "NeForce/core/exception/debug.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/memory/new.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup MemoryAllocator 内存分配器
 * @brief 内存分配器的实现
 * @{
 */

/**
 * @def MEMORY_ALIGN_THRESHHOLD
 * @brief 对齐阈值（16字节）
 *
 * 小于等于此阈值的对齐要求使用普通分配，大于此值的使用对齐分配。
 */
NEFORCE_INLINE17 constexpr size_t MEMORY_ALIGN_THRESHHOLD = 16;

/**
 * @def MEMORY_BIG_ALLOC_THRESHHOLD
 * @brief 大内存分配阈值（4KB）
 *
 * 超过此大小的内存分配被视为大内存分配，使用特殊处理策略。
 */
NEFORCE_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_THRESHHOLD = 4096;

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @typedef alloc_size_t
 * @brief 分配器使用的内存大小类型
 */
using alloc_size_t =
/// @cond
#ifdef NEFORCE_COMPILER_GCC
        uint32_t;
#else
        size_t;
#endif
/// @endcond


#ifdef NEFORCE_COMPILER_MSVC

/**
 * @def MEMORY_BIG_ALLOC_SENTINEL
 * @brief 大内存分配哨兵值
 *
 * 用于调试模式下检测内存破坏。
 */
NEFORCE_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_ALIGN = 32;

/**
 * @def MEMORY_NO_USER_SIZE
 * @brief 非用户数据大小
 *
 * 大内存分配时在用户数据前后添加的额外空间大小，用于存储元数据。
 */
NEFORCE_INLINE17 constexpr size_t MEMORY_NO_USER_SIZE = sizeof(void*) + MEMORY_BIG_ALLOC_ALIGN -
                                                        static_cast<size_t>(1)
#    ifdef NEFORCE_STATE_DEBUG
                                                                * static_cast<size_t>(2)
#    endif
        ;

/**
 * @def MEMORY_BIG_ALLOC_SENTINEL
 * @brief 大内存分配哨兵值
 *
 * 用于调试模式下检测内存破坏。
 */
NEFORCE_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_SENTINEL =
#    ifdef NEFORCE_ARCH_BITS_64
        0xFAFAFAFAFAFAFAFAUL;
#    else
        0xFAFAFAFAUL;
#    endif

#endif // NEFORCE_COMPILER_MSVC


/**
 * @brief 基础分配辅助函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针
 *
 * 处理内存分配的基础函数，包含编译器特定的优化。
 */
template <size_t Align>
NEFORCE_ALLOC_OPTIMIZE NEFORCE_CONSTEXPR20 void* __allocate_aux(const alloc_size_t bytes) {
#ifdef NEFORCE_COMPILER_MSVC
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        const size_t block_size = MEMORY_NO_USER_SIZE + bytes;
        if (block_size <= bytes) {
            NEFORCE_THROW_EXCEPTION(memory_exception("invalid block size."));
        }
        const auto holder = reinterpret_cast<uintptr_t>(operator new(block_size));
        NEFORCE_DEBUG_VERIFY(holder != 0, "invalid argument");
        auto* const ptr = reinterpret_cast<void*>((holder + MEMORY_NO_USER_SIZE) & ~(MEMORY_BIG_ALLOC_ALIGN - 1));
        static_cast<uintptr_t*>(ptr)[-1] = holder;
#    ifdef NEFORCE_STATE_DEBUG
        static_cast<uintptr_t*>(ptr)[-2] = MEMORY_BIG_ALLOC_SENTINEL;
#    endif
        return ptr;
    }
#endif
    return operator new(bytes);
}

#ifdef NEFORCE_STANDARD_17

/**
 * @brief 高对齐要求内存分配分发函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针
 *
 * 使用对齐分配操作符，支持大于阈值的高对齐要求。
 */
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD), int> = 0>
NEFORCE_ALLOC_OPTIMIZE NEFORCE_CONSTEXPR20 void* __allocate_dispatch(const alloc_size_t bytes) {
    size_t align = Align;
#    ifdef NEFORCE_COMPILER_MSVC
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        align = Align > MEMORY_BIG_ALLOC_ALIGN ? Align : MEMORY_BIG_ALLOC_ALIGN;
    }
#    endif
#    if defined(NEFORCE_COMPILER_CLANG) && defined(NEFORCE_STANDARD_20)
    if (_NEFORCE is_constant_evaluated()) {
        return operator new(bytes);
    }
#    endif
    return operator new(bytes, std::align_val_t{align});
}

/**
 * @brief 低对齐要求内存分配分发函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针
 *
 * 使用基础分配函数处理低对齐要求的分配。
 */
template <size_t Align, enable_if_t<Align <= MEMORY_ALIGN_THRESHHOLD, int> = 0>
NEFORCE_ALLOC_OPTIMIZE NEFORCE_CONSTEXPR20 void* __allocate_dispatch(const alloc_size_t bytes) {
    return inner::__allocate_aux<Align>(bytes);
}

#endif

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 内存分配函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针，如果bytes为0则返回nullptr
 *
 * 内存分配的统一入口。
 */
template <size_t Align>
NEFORCE_ALLOC_OPTIMIZE NEFORCE_CONSTEXPR20 void* allocate(const inner::alloc_size_t bytes) {
    if (bytes == 0) {
        return nullptr;
    }
#ifdef NEFORCE_STANDARD_20
    if (_NEFORCE is_constant_evaluated()) {
        return operator new(bytes);
    }
#endif // NEFORCE_STANDARD_20

#ifdef NEFORCE_STANDARD_17
    return inner::__allocate_dispatch<Align>(bytes);
#else
    return inner::__allocate_aux<Align>(bytes);
#endif // NEFORCE_STANDARD_17
}


/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @brief 基础释放辅助函数
 * @tparam Align 对齐要求
 * @param[in,out] ptr 要释放的内存指针引用
 * @param[in,out] bytes 要释放的字节数引用
 *
 * 处理内存释放的基础函数，包含编译器特定的优化。
 */
template <size_t Align>
void __deallocate_aux(void*& ptr, inner::alloc_size_t& bytes) noexcept {
#ifdef NEFORCE_COMPILER_MSVC
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        bytes += MEMORY_NO_USER_SIZE;
        const uintptr_t* const user_ptr = static_cast<uintptr_t*>(ptr);
        const uintptr_t holder = user_ptr[-1];
        NEFORCE_DEBUG_VERIFY(user_ptr[-2] == MEMORY_BIG_ALLOC_SENTINEL, "invalid sentinel.");
#    ifdef NEFORCE_STATE_DEBUG
        constexpr uintptr_t min_shift = 2 * sizeof(void*);
#    else
        constexpr uintptr_t min_shift = sizeof(void*);
#    endif
        const uintptr_t shift = reinterpret_cast<uintptr_t>(ptr) - holder;
        NEFORCE_DEBUG_VERIFY(shift >= min_shift && shift <= MEMORY_NO_USER_SIZE, "invalid argument.");
        ptr = reinterpret_cast<void*>(holder);
    }
#endif
#if defined(NEFORCE_STANDARD_14) && defined(NEFORCE_COMPILER_MSVC)
    operator delete(ptr, bytes);
#else
    operator delete(ptr);
#endif
}

#ifdef NEFORCE_STANDARD_17

/**
 * @brief 高对齐要求内存释放分发函数
 * @tparam Align 对齐要求
 * @param[in,out] ptr 要释放的内存指针引用
 * @param[in,out] bytes 要释放的字节数引用
 *
 * 使用对齐释放操作符，支持大于阈值的高对齐要求。
 */
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD), int> = 0>
NEFORCE_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, inner::alloc_size_t& bytes) noexcept {
    size_t align = Align;
#    ifdef NEFORCE_COMPILER_MSVC
    if (bytes > MEMORY_BIG_ALLOC_THRESHHOLD) {
        align = Align > MEMORY_BIG_ALLOC_ALIGN ? Align : MEMORY_BIG_ALLOC_ALIGN;
    }
#    endif
#    if defined(NEFORCE_STANDARD_14) && defined(NEFORCE_COMPILER_MSVC)
    operator delete(ptr, bytes, std::align_val_t{align});
#    else
    operator delete(ptr, std::align_val_t{align});
#    endif
}

/**
 * @brief 低对齐要求内存释放分发函数
 * @tparam Align 对齐要求
 * @param[in,out] ptr 要释放的内存指针引用
 * @param[in,out] bytes 要释放的字节数引用
 */
template <size_t Align, enable_if_t<Align <= MEMORY_ALIGN_THRESHHOLD, int> = 0>
NEFORCE_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, inner::alloc_size_t& bytes) noexcept {
    inner::__deallocate_aux<Align>(ptr, bytes);
}

#endif // NEFORCE_STANDARD_17

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 内存释放函数
 * @tparam Align 对齐要求
 * @param ptr 要释放的内存指针
 * @param bytes 要释放的字节数
 *
 * 内存释放的统一入口。
 */
template <size_t Align>
NEFORCE_CONSTEXPR20 void deallocate(void* ptr, inner::alloc_size_t bytes) noexcept {
#ifdef NEFORCE_STANDARD_20
    if (_NEFORCE is_constant_evaluated()) {
        operator delete(ptr);
        return;
    }
#endif // NEFORCE_STANDARD_20

#ifdef NEFORCE_STANDARD_17
    inner::__deallocate_dispatch<Align>(ptr, bytes);
#else
    inner::__deallocate_aux<Align>(ptr, bytes);
#endif // NEFORCE_STANDARD_17
}


/**
 * @class standard_allocator
 * @brief 标准分配器类
 * @tparam T 要分配的元素类型
 *
 * 提供类型安全的内存分配和释放，支持对齐优化。
 */
template <typename T>
class standard_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    using value_type = T;                  ///< 元素类型
    using pointer = T*;                    ///< 指针类型
    using size_type = inner::alloc_size_t; ///< 大小类型

    /**
     * @struct rebind
     * @brief 重新绑定模板
     * @tparam U 新的元素类型
     *
     * 允许容器重新绑定分配器到其他类型。
     */
    template <typename U>
    struct rebind {
        using other = standard_allocator<U>;
    };

private:
    /// 最终对齐大小：取类型对齐要求和阈值中的较大值
    static constexpr inner::alloc_size_t align_size = alignof(T) > MEMORY_ALIGN_THRESHHOLD ? alignof(T)
                                                                                           : MEMORY_ALIGN_THRESHHOLD;

public:
    NEFORCE_CONSTEXPR20 standard_allocator() noexcept = default; ///< 默认构造函数

    /**
     * @brief 从其他分配器类型转换构造
     * @tparam U 源分配器元素类型
     */
    template <typename U>
    NEFORCE_CONSTEXPR20 standard_allocator(const standard_allocator<U>& /*unused*/) noexcept {}

    NEFORCE_CONSTEXPR20 ~standard_allocator() noexcept = default; ///< 析构函数

    NEFORCE_CONSTEXPR20 standard_allocator& operator=(const standard_allocator&) noexcept = default; ///< 赋值运算符

    /**
     * @brief 分配指定数量的元素内存
     * @param n 要分配的元素数量
     * @return 指向分配内存的指针
     * @throws allocate_exception 如果内存分配失败
     *
     * 分配 n 个 T 类型的连续内存空间。
     */
    NEFORCE_ALLOC_NODISCARD NEFORCE_CONSTEXPR20 NEFORCE_ALLOC_OPTIMIZE static pointer allocate(const size_type n) {
        const size_type alloc_size = sizeof(value_type) * n;
        NEFORCE_DEBUG_VERIFY(alloc_size <= static_cast<size_type>(-1), "allocation will cause memory overflow.");
        try {
            return static_cast<T*>(_NEFORCE allocate<align_size>(alloc_size));
        } catch (...) {
            NEFORCE_THROW_EXCEPTION(allocate_exception("standard allocate failed"));
        }
        unreachable();
    }

    /**
     * @brief 分配单个元素内存
     * @return 指向分配内存的指针
     * @throws allocate_exception 如果内存分配失败
     */
    NEFORCE_ALLOC_NODISCARD NEFORCE_CONSTEXPR20 NEFORCE_ALLOC_OPTIMIZE static pointer allocate() {
        return standard_allocator::allocate(1);
    }

    /**
     * @brief 释放先前分配的内存
     * @param p 要释放的内存指针
     * @param n 先前分配的元素数量
     * @note p 必须为 nullptr 或先前由 allocate() 返回的指针
     */
    NEFORCE_CONSTEXPR20 static void deallocate(pointer p, const size_type n) noexcept {
        NEFORCE_DEBUG_VERIFY(p != nullptr || n == 0, "null pointer cannot point to a block of non-zero size");
        _NEFORCE deallocate<align_size>(p, n * sizeof(value_type));
    }

    /**
     * @brief 释放单个元素内存
     * @param p 要释放的内存指针
     */
    NEFORCE_CONSTEXPR20 static void deallocate(pointer p) noexcept { standard_allocator::deallocate(p, 1); }

    /**
     * @brief 获取分配器可分配的最大元素数量
     * @return 最大可分配元素数量
     */
    NEFORCE_NODISCARD static constexpr size_type max_size() noexcept {
        return static_cast<size_type>(-1) / sizeof(value_type);
    }
};

/**
 * @brief 比较两个分配器是否相等
 * @tparam T 第一个分配器元素类型
 * @tparam U 第二个分配器元素类型
 * @return 总是返回 true
 */
template <typename T, typename U>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator==(const standard_allocator<T>& /*unused*/,
                                                      const standard_allocator<U>& /*unused*/) noexcept {
    return true;
}

/**
 * @brief 比较两个分配器是否不等
 * @tparam T 第一个分配器元素类型
 * @tparam U 第二个分配器元素类型
 * @return 总是返回 false
 */
template <typename T, typename U>
NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator!=(const standard_allocator<T>& /*unused*/,
                                                      const standard_allocator<U>& /*unused*/) noexcept {
    return false;
}


/**
 * @typedef allocator
 * @brief 标准分配器别名
 * @tparam T 元素类型
 *
 * 为 standard_allocator 提供的类型别名，用于统一接口。
 */
template <typename T>
using allocator = standard_allocator<T>;

/** @} */ // MemoryAllocator

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
