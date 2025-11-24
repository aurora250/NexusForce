#ifndef MSTL_CORE_MEMORY_ALIGNED_BUFFER_HPP__
#define MSTL_CORE_MEMORY_ALIGNED_BUFFER_HPP__
#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
struct aligned_buffer : aligned_storage<sizeof(T), alignof(T)> {
    aligned_storage_t<sizeof(T), alignof(T)> storage;

    aligned_buffer() = default;
    aligned_buffer(nullptr_t) {}

    void* addr() noexcept { return static_cast<void*>(&storage); }
    const void* addr() const noexcept { return static_cast<const void*>(&storage); }

    T* ptr() noexcept { return static_cast<T*>(addr()); }
    const T* ptr() const noexcept { return static_cast<const T*>(addr()); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_ALIGNED_BUFFER_HPP__
