#ifndef MSTL_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
#define MSTL_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/tags.hpp"
#include "../typeinfo/types.hpp"
#include "../exception/assertion.hpp"
#include "../exception/exception.hpp"
#include <new>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

#ifdef MSTL_COMPILER_MSVC__
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_ALIGN = 32UL;

#ifdef MSTL_STATE_DEBUG__
MSTL_INLINE17 constexpr size_t MEMORY_NO_USER_SIZE = 2 * POINTER_SIZE + MEMORY_BIG_ALLOC_ALIGN - 1;
#else
MSTL_INLINE17 constexpr size_t MEMORY_NO_USER_SIZE = POINTER_SIZE + MEMORY_BIG_ALLOC_ALIGN - 1;
#endif

#ifdef MSTL_DATA_BUS_WIDTH_64__
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_SENTINEL = 0xFAFAFAFAFAFAFAFAUL;
#else
MSTL_INLINE17 constexpr size_t MEMORY_BIG_ALLOC_SENTINEL = 0xFAFAFAFAUL;
#endif
#endif // MSTL_COMPILER_MSVC__


template <size_t Align>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_aux(const size_t bytes) {
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
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD) ,int> = 0>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_dispatch(const size_t bytes) {
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

template <size_t Align, enable_if_t<Align <= MEMORY_ALIGN_THRESHHOLD, int> = 0>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_dispatch(const size_t bytes) {
    return _INNER __allocate_aux<Align>(bytes);
}
#endif

MSTL_END_INNER__

template <size_t Align>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* allocate(const size_t bytes) {
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


MSTL_BEGIN_INNER__

template <size_t>
void __deallocate_aux(void*& ptr, size_t& bytes) noexcept {
#ifdef MSTL_COMPILER_MSVC__
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        bytes += MEMORY_NO_USER_SIZE;
        const uintptr_t* const user_ptr = static_cast<uintptr_t*>(ptr);
        const uintptr_t holder = user_ptr[-1];
        MSTL_DEBUG_VERIFY(user_ptr[-2] == MEMORY_BIG_ALLOC_SENTINEL, "invalid sentinel.");
#ifdef MSTL_STATE_DEBUG__
        constexpr uintptr_t min_shift = 2 * POINTER_SIZE;
#else
        constexpr uintptr_t min_shift = POINTER_SIZE;
#endif // MSTL_STATE_DEBUG__
        const uintptr_t shift = reinterpret_cast<uintptr_t>(ptr) - holder;
        MSTL_DEBUG_VERIFY(shift >= min_shift && shift <= MEMORY_NO_USER_SIZE, "invalid argument.");
        ptr = reinterpret_cast<void*>(holder);
    }
#endif
    operator delete(ptr
#ifdef __cpp_sized_deallocation
        , bytes
#endif
        );
}

#ifdef MSTL_STANDARD_17__
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD), int> = 0>
MSTL_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, size_t& bytes) noexcept {
    size_t align = Align;
#ifdef MSTL_COMPILER_MSVC__
    if (bytes > MEMORY_BIG_ALLOC_THRESHHOLD)
        align = Align > MEMORY_BIG_ALLOC_ALIGN ? Align : MEMORY_BIG_ALLOC_ALIGN;
#endif
    operator delete(ptr,
#ifdef __cpp_sized_deallocation
        bytes,
#endif
        std::align_val_t{ align }
        );
}

template<size_t Align, enable_if_t<Align <= MEMORY_ALIGN_THRESHHOLD, int> = 0>
MSTL_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, size_t& bytes) noexcept {
    _INNER __deallocate_aux<Align>(ptr, bytes);
}
#endif // MSTL_STANDARD_17__

MSTL_END_INNER__

template <size_t Align>
MSTL_CONSTEXPR20 void deallocate(void* ptr, size_t bytes) noexcept {
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

MSTL_BEGIN_INNER__
template <typename T>
constexpr size_t __FINAL_ALIGN_SIZE = alignof(T) > MEMORY_ALIGN_THRESHHOLD ? alignof(T) : MEMORY_ALIGN_THRESHHOLD;
MSTL_END_INNER__


template <typename T>
class standard_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using device_type = allocate_cpu_tag;

    template <typename U>
    struct rebind {
        using other = standard_allocator<U>;
    };

    MSTL_CONSTEXPR20 standard_allocator() noexcept = default;
    template <typename U>
    MSTL_CONSTEXPR20 standard_allocator(const standard_allocator<U>&) noexcept {}
    MSTL_CONSTEXPR20 ~standard_allocator() noexcept = default;
    MSTL_CONSTEXPR20 standard_allocator& operator =(const standard_allocator&) noexcept = default;

    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        constexpr size_t value_size = sizeof(value_type);
        static_assert(value_size > 0, "value type must be complete before allocation called.");
        const size_t alloc_size = value_size * n;
        MSTL_DEBUG_VERIFY(alloc_size <= static_cast<size_t>(-1), "allocation will cause memory overflow.");
        try {
            return static_cast<T*>(_MSTL allocate<_INNER __FINAL_ALIGN_SIZE<T>>(alloc_size));
        } catch (...) {
            throw_exception(allocate_exception("standard allocate failed"));
            return nullptr;
        }
    }

    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return standard_allocator::allocate(1);
    }

    MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type n) noexcept {
        constexpr size_t value_size = sizeof(value_type);
        MSTL_DEBUG_VERIFY(p != nullptr || n == 0, "null pointer cannot point to a block of non-zero size");
        _MSTL deallocate<_INNER __FINAL_ALIGN_SIZE<T>>(p, n * value_size);
    }

    MSTL_CONSTEXPR20 void deallocate(pointer p) noexcept {
        standard_allocator::deallocate(p, 1);
    }
};
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const standard_allocator<T>&, const standard_allocator<U>&) noexcept {
    return true;
}
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const standard_allocator<T>&, const standard_allocator<U>&) noexcept {
    return false;
}

// equal to new_allocator under GNUC and hold a dispatch under MSVC
template <typename T>
using allocator = standard_allocator<T>;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_STANDARD_ALLOCATOR_HPP__
