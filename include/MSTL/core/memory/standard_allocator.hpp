#ifndef MSTL_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
#define MSTL_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__

/**
 * @file standard_allocator.hpp
 * @brief MSTL标准分配器
 *
 * 此文件提供了标准分配器实现，
 * 用于管理动态内存分配和释放，支持不同对齐要求和编译器优化。
 */

#include "../numeric/numeric_traits.hpp"
#include "../exception/exception.hpp"
#include "../exception/assertion.hpp"
#include <new>
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup MemoryAllocator 内存分配器
 * @brief MSTL内存分配器的实现
 * @{
 */

/**
 * @def MEMORY_ALIGN_THRESHHOLD
 * @brief 对齐阈值（16字节）
 *
 * 小于等于此阈值的对齐要求使用普通分配，大于此值的使用对齐分配。
 */
MSTL_INLINE17 constexpr size_t MEMORY_ALIGN_THRESHHOLD = 16;

/**
 * @def MEMORY_BIG_ALLOC_THRESHHOLD
 * @brief 大内存分配阈值（4KB）
 *
 * 超过此大小的内存分配被视为大内存分配，使用特殊处理策略。
 */
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_THRESHHOLD = 4096;

/// @cond
MSTL_BEGIN_INNER__

/**
 * @typedef alloc_size_t
 * @brief 分配器使用的内存大小类型
 */
using alloc_size_t = size_t;

/// @cond
// #ifdef MSTL_COMPILER_GCC__
// using alloc_size_t = uint32_t;
// #else
// using alloc_size_t = size_t;
// #endif
/// @endcond


#ifdef MSTL_COMPILER_MSVC__

/**
 * @def MEMORY_BIG_ALLOC_SENTINEL
 * @brief 大内存分配哨兵值
 *
 * 用于调试模式下检测内存破坏。
 */
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_ALIGN = 32;

/**
 * @def MEMORY_NO_USER_SIZE
 * @brief 非用户数据大小
 *
 * 大内存分配时在用户数据前后添加的额外空间大小，用于存储元数据。
 */
MSTL_INLINE17 constexpr size_t MEMORY_NO_USER_SIZE =
    sizeof(void*) + MEMORY_BIG_ALLOC_ALIGN - 1
#ifdef MSTL_STATE_DEBUG__
    * 2
#endif
;

/**
 * @def MEMORY_BIG_ALLOC_SENTINEL
 * @brief 大内存分配哨兵值
 *
 * 用于调试模式下检测内存破坏。
 */
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_SENTINEL =
#ifdef MSTL_DATA_BUS_WIDTH_64__
    0xFAFAFAFAFAFAFAFAUL;
#else
    0xFAFAFAFAUL;
#endif

#endif // MSTL_COMPILER_MSVC__


/**
 * @brief 基础分配辅助函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针
 *
 * 处理内存分配的基础函数，包含编译器特定的优化。
 */
template <size_t Align>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_aux(const alloc_size_t bytes) {
#ifdef MSTL_COMPILER_MSVC__
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        const size_t block_size = MEMORY_NO_USER_SIZE + bytes;
        if (block_size <= bytes)
            throw_exception(memory_exception("invalid block size."));
        const auto holder = reinterpret_cast<uintptr_t>(operator new(block_size));
        MSTL_DEBUG_VERIFY(holder != 0, "invalid argument");
        const auto ptr = reinterpret_cast<void*>(
            (holder + MEMORY_NO_USER_SIZE) & ~(MEMORY_BIG_ALLOC_ALIGN - 1)); // align the memory address
        static_cast<uintptr_t*>(ptr)[-1] = holder;
#ifdef MSTL_STATE_DEBUG__
        static_cast<uintptr_t*>(ptr)[-2] = MEMORY_BIG_ALLOC_SENTINEL;
#endif
        return ptr;
    }
#endif
    return operator new(bytes);
}

#ifdef MSTL_STANDARD_17__

/**
 * @brief 高对齐要求内存分配分发函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针
 *
 * 使用对齐分配操作符，支持大于阈值的高对齐要求。
 */
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD) ,int> = 0>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_dispatch(const alloc_size_t bytes) {
    size_t align = Align;
#ifdef MSTL_COMPILER_MSVC__
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD)
        align = Align > MEMORY_BIG_ALLOC_ALIGN ? Align : MEMORY_BIG_ALLOC_ALIGN;
#endif
#if defined(MSTL_COMPILER_CLANG__) && defined(MSTL_STANDARD_20__)
    if (_MSTL is_constant_evaluated())
        return operator new(bytes);
#endif
    return operator new(bytes, std::align_val_t{ align });
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
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_dispatch(const alloc_size_t bytes) {
    return _INNER __allocate_aux<Align>(bytes);
}

#endif

MSTL_END_INNER__
/// @endcond

/**
 * @brief 内存分配函数
 * @tparam Align 对齐要求
 * @param bytes 要分配的字节数
 * @return 分配的内存指针，如果bytes为0则返回nullptr
 *
 * 内存分配的统一入口，根据C++标准版本选择不同的实现。
 * 支持编译期常量求值优化。
 */
template <size_t Align>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* allocate(const _INNER alloc_size_t bytes) {
    if (bytes == 0) return nullptr;
#ifdef MSTL_STANDARD_20__
    if (_MSTL is_constant_evaluated())
        return operator new(bytes);
#endif // MSTL_STANDARD_20__

#ifdef MSTL_STANDARD_17__
    return _INNER __allocate_dispatch<Align>(bytes);
#else
    return _INNER __allocate_aux<Align>(bytes);
#endif // MSTL_STANDARD_17__
}


/// @cond
MSTL_BEGIN_INNER__

/**
 * @brief 基础释放辅助函数
 * @tparam Align 对齐要求
 * @param[in,out] ptr 要释放的内存指针引用
 * @param[in,out] bytes 要释放的字节数引用
 *
 * 处理内存释放的基础函数，包含编译器特定的优化。
 */
template <size_t Align>
void __deallocate_aux(void*& ptr, _INNER alloc_size_t& bytes) noexcept {
#ifdef MSTL_COMPILER_MSVC__
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        bytes += MEMORY_NO_USER_SIZE;
        const uintptr_t* const user_ptr = static_cast<uintptr_t*>(ptr);
        const uintptr_t holder = user_ptr[-1];
        MSTL_DEBUG_VERIFY(user_ptr[-2] == MEMORY_BIG_ALLOC_SENTINEL, "invalid sentinel.");
#ifdef MSTL_STATE_DEBUG__
        constexpr uintptr_t min_shift = 2 * sizeof(void*);
#else
        constexpr uintptr_t min_shift = sizeof(void*);
#endif // MSTL_STATE_DEBUG__
        const uintptr_t shift = reinterpret_cast<uintptr_t>(ptr) - holder;
        MSTL_DEBUG_VERIFY(shift >= min_shift && shift <= MEMORY_NO_USER_SIZE, "invalid argument.");
        ptr = reinterpret_cast<void*>(holder);
    }
#endif
    operator delete(ptr
#if defined(MSTL_STANDARD_14__) && defined(MSTL_COMPILER_MSVC__)
        , bytes
#endif
        );
}

#ifdef MSTL_STANDARD_17__

/**
 * @brief 高对齐要求内存释放分发函数
 * @tparam Align 对齐要求
 * @param[in,out] ptr 要释放的内存指针引用
 * @param[in,out] bytes 要释放的字节数引用
 *
 * 使用对齐释放操作符，支持大于阈值的高对齐要求。
 */
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD), int> = 0>
MSTL_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, _INNER alloc_size_t& bytes) noexcept {
    size_t align = Align;
#ifdef MSTL_COMPILER_MSVC__
    if (bytes > MEMORY_BIG_ALLOC_THRESHHOLD) {
        align = Align > MEMORY_BIG_ALLOC_ALIGN ? Align : MEMORY_BIG_ALLOC_ALIGN;
    }
#endif
    operator delete(ptr,
#if defined(MSTL_STANDARD_14__) && defined(MSTL_COMPILER_MSVC__)
        bytes,
#endif
        std::align_val_t{ align });
}

/**
 * @brief 低对齐要求内存释放分发函数
 * @tparam Align 对齐要求
 * @param[in,out] ptr 要释放的内存指针引用
 * @param[in,out] bytes 要释放的字节数引用
 */
template <size_t Align, enable_if_t<Align <= MEMORY_ALIGN_THRESHHOLD, int> = 0>
MSTL_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, _INNER alloc_size_t& bytes) noexcept {
    _INNER __deallocate_aux<Align>(ptr, bytes);
}

#endif // MSTL_STANDARD_17__

MSTL_END_INNER__
/// @endcond

/**
 * @brief 内存释放函数
 * @tparam Align 对齐要求
 * @param ptr 要释放的内存指针
 * @param bytes 要释放的字节数
 *
 * 内存释放的统一入口，根据C++标准版本选择不同的实现。
 * 支持编译期常量求值优化。
 */
template <size_t Align>
MSTL_CONSTEXPR20 void deallocate(void* ptr, _INNER alloc_size_t bytes) noexcept {
#ifdef MSTL_STANDARD_20__
    if (_MSTL is_constant_evaluated()) {
        operator delete(ptr);
        return;
    }
#endif // MSTL_STANDARD_20__

#ifdef MSTL_STANDARD_17__
    _INNER __deallocate_dispatch<Align>(ptr, bytes);
#else
    _INNER __deallocate_aux<Align>(ptr, bytes);
#endif // MSTL_STANDARD_17__
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
    using value_type = T;       ///< 元素类型
    using pointer    = T*;      ///< 指针类型
    using size_type  = _INNER alloc_size_t;  ///< 大小类型

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
    static constexpr _INNER alloc_size_t FINAL_ALIGN_SIZE =
        alignof(T) > MEMORY_ALIGN_THRESHHOLD ? alignof(T) : MEMORY_ALIGN_THRESHHOLD;\

    static constexpr size_type VALUE_SIZE = sizeof(value_type);  ///< 单个元素大小

public:
    MSTL_CONSTEXPR20 standard_allocator() noexcept = default;  ///< 默认构造函数

    /**
     * @brief 从其他分配器类型转换构造
     * @tparam U 源分配器元素类型
     */
    template <typename U>
    MSTL_CONSTEXPR20 standard_allocator(const standard_allocator<U>&) noexcept {}

    MSTL_CONSTEXPR20 ~standard_allocator() noexcept = default;  ///< 析构函数

    MSTL_CONSTEXPR20 standard_allocator& operator =(const standard_allocator&) noexcept = default;  ///< 赋值运算符

    /**
     * @brief 分配指定数量的元素内存
     * @param n 要分配的元素数量
     * @return 指向分配内存的指针
     * @throws allocate_exception 如果内存分配失败
     *
     * 分配 n 个 T 类型的连续内存空间。
     */
    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        const size_type alloc_size = VALUE_SIZE * n;
        MSTL_DEBUG_VERIFY(
            alloc_size <= numeric_traits<size_type>::max(),
            "allocation will cause memory overflow.");
        try {
            return static_cast<T*>(_MSTL allocate<FINAL_ALIGN_SIZE>(alloc_size));
        } catch (...) {
            throw_exception(allocate_exception("standard allocate failed"));
        }
        MSTL_UNREACHABLE;
    }

    /**
     * @brief 分配单个元素内存
     * @return 指向分配内存的指针
     * @throws allocate_exception 如果内存分配失败
     */
    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return standard_allocator::allocate(1);
    }

    /**
     * @brief 释放先前分配的内存
     * @param p 要释放的内存指针
     * @param n 先前分配的元素数量
     * @note p 必须为 nullptr 或先前由 allocate() 返回的指针
     */
    MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type n) noexcept {
        MSTL_DEBUG_VERIFY(p != nullptr || n == 0, "null pointer cannot point to a block of non-zero size");
        _MSTL deallocate<FINAL_ALIGN_SIZE>(p, n * VALUE_SIZE);
    }

    /**
     * @brief 释放单个元素内存
     * @param p 要释放的内存指针
     */
    MSTL_CONSTEXPR20 void deallocate(pointer p) noexcept {
        standard_allocator::deallocate(p, 1);
    }
};

/**
 * @brief 比较两个分配器是否相等
 * @tparam T 第一个分配器元素类型
 * @tparam U 第二个分配器元素类型
 * @return 总是返回 true
 */
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const standard_allocator<T>&, const standard_allocator<U>&) noexcept {
    return true;
}

/**
 * @brief 比较两个分配器是否不等
 * @tparam T 第一个分配器元素类型
 * @tparam U 第二个分配器元素类型
 * @return 总是返回 false
 */
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const standard_allocator<T>&, const standard_allocator<U>&) noexcept {
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
