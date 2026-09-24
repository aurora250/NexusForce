#include <NeForce/core/memory/new.hpp>
#ifdef NEFORCE_USING_MEMORY_POOL_OVERRIDE
#    include <NeForce/core/algorithm/compare.hpp>
#    include <NeForce/core/exception/exception.hpp>
#    include <NeForce/core/memory/memory_pool.hpp>

namespace {
    constexpr _NEFORCE size_t default_alignment = _NEFORCE max<size_t>(alignof(_NEFORCE max_align_t), 16);

    NEFORCE_ALWAYS_INLINE void* pool_allocate(const _NEFORCE size_t size, const _NEFORCE size_t alignment) noexcept {
        return _NEFORCE system_memory_pool().try_allocate(size, alignment);
    }

    NEFORCE_ALWAYS_INLINE void* pool_allocate_nothrow(const _NEFORCE size_t size,
                                                      const _NEFORCE size_t alignment) noexcept {
        return _NEFORCE system_memory_pool().try_allocate(size, alignment);
    }

    NEFORCE_ALWAYS_INLINE void pool_release(void* ptr) noexcept { _NEFORCE system_memory_pool().deallocate(ptr); }

    NEFORCE_ALWAYS_INLINE _NEFORCE size_t aligned_size(const std::align_val_t alignment) noexcept {
        const auto value = static_cast<_NEFORCE size_t>(alignment);
        return value < 16 ? 16 : value;
    }

    NEFORCE_ALWAYS_INLINE _NEFORCE size_t aligned_size(const _NEFORCE align_t alignment) noexcept {
        const auto value = static_cast<_NEFORCE size_t>(alignment);
        return value < 16 ? 16 : value;
    }
} // namespace


// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(const _NEFORCE size_t size) {
    void* block = pool_allocate(size, default_alignment);
    if (block == nullptr) {
        throw _NEFORCE allocate_exception();
    }
    return block;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](const _NEFORCE size_t size) { return ::operator new(size); }

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(const _NEFORCE size_t size, const std::nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, default_alignment);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](const _NEFORCE size_t size, const std::nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, default_alignment);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr) noexcept { pool_release(ptr); }

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr) noexcept { pool_release(ptr); }

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, const _NEFORCE size_t size) noexcept {
    static_cast<void>(size);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, const _NEFORCE size_t size) noexcept {
    static_cast<void>(size);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, const std::nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, const std::nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(const _NEFORCE size_t size, const std::align_val_t align) {
    void* block = pool_allocate(size, aligned_size(align));
    if (block == nullptr) {
        throw _NEFORCE allocate_exception();
    }
    return block;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](const _NEFORCE size_t size, const std::align_val_t align) { return ::operator new(size, align); }

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(const _NEFORCE size_t size, const std::align_val_t align, const std::nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, aligned_size(align));
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](const _NEFORCE size_t size, const std::align_val_t align, const std::nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, aligned_size(align));
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, const std::align_val_t align) noexcept {
    static_cast<void>(align);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, const std::align_val_t align) noexcept {
    static_cast<void>(align);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, const _NEFORCE size_t size, const std::align_val_t align) noexcept {
    static_cast<void>(size);
    static_cast<void>(align);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, const _NEFORCE size_t size, const std::align_val_t align) noexcept {
    static_cast<void>(size);
    static_cast<void>(align);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, const std::align_val_t align, const std::nothrow_t& tag) noexcept {
    static_cast<void>(align);
    static_cast<void>(tag);
    pool_release(ptr);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, const std::align_val_t align, const std::nothrow_t& tag) noexcept {
    static_cast<void>(align);
    static_cast<void>(tag);
    pool_release(ptr);
}

void operator delete(void* ptr, const _NEFORCE size_t size, const std::align_val_t align,
                     const std::nothrow_t& tag) noexcept {
    static_cast<void>(size);
    static_cast<void>(align);
    static_cast<void>(tag);
    pool_release(ptr);
}

void operator delete[](void* ptr, const _NEFORCE size_t size, const std::align_val_t align,
                       const std::nothrow_t& tag) noexcept {
    static_cast<void>(size);
    static_cast<void>(align);
    static_cast<void>(tag);
    pool_release(ptr);
}

void* operator new(const _NEFORCE size_t size, const _NEFORCE nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, default_alignment);
}

void* operator new[](const _NEFORCE size_t size, const _NEFORCE nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, default_alignment);
}

void* operator new(const _NEFORCE size_t size, const _NEFORCE align_t align) {
    void* block = pool_allocate(size, aligned_size(align));
    if (block == nullptr) {
        throw _NEFORCE allocate_exception();
    }
    return block;
}

void* operator new[](const _NEFORCE size_t size, const _NEFORCE align_t align) { return ::operator new(size, align); }

void* operator new(const _NEFORCE size_t size, const _NEFORCE align_t align, const _NEFORCE nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, aligned_size(align));
}

void* operator new[](const _NEFORCE size_t size, const _NEFORCE align_t align, const _NEFORCE nothrow_t& tag) noexcept {
    static_cast<void>(tag);
    return pool_allocate_nothrow(size, aligned_size(align));
}

#endif // NEFORCE_USING_MEMORY_POOL_OVERRIDE
