#ifndef MSTL_CORE_MEMORY_BUILTIN_ALLOCATOR_HPP__
#define MSTL_CORE_MEMORY_BUILTIN_ALLOCATOR_HPP__
#include "../config/exception.hpp"
#include "../typeinfo/types.hpp"
#include "../typeinfo/type_traits.hpp"
#include "../typeinfo/tags.hpp"
#include <cstdlib> // std::malloc
MSTL_BEGIN_NAMESPACE__

template <typename T>
class ctype_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using device_type = allocate_cpu_tag;
    using self = ctype_allocator<T>;

    template <typename U>
    struct rebind {
        using other = ctype_allocator<U>;
    };

    MSTL_CONSTEXPR20 ctype_allocator() noexcept = default;
    template <typename U>
    MSTL_CONSTEXPR20 ctype_allocator(const ctype_allocator<U>&) noexcept {}
    MSTL_CONSTEXPR20 ~ctype_allocator() noexcept = default;
    MSTL_CONSTEXPR20 self& operator =(const self&) noexcept = default;

    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        if (n == 0) return nullptr;
        pointer ptr = nullptr;
        try {
            ptr = static_cast<pointer>(std::malloc(n * sizeof(T)));
        } catch (...) {
            throw_exception(allocate_exception());
        }
        return ptr;
    }
    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return self::allocate(1);
    }
    MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type = 1) noexcept {
        if(p) std::free(p);
    }
};
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const ctype_allocator<T>&, const ctype_allocator<U>&) noexcept {
    return true;
}
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const ctype_allocator<T>&, const ctype_allocator<U>&) noexcept {
    return false;
}

template <typename T>
class new_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using device_type = allocate_cpu_tag;
    using self = new_allocator<T>;

    template <typename U>
    struct rebind {
        using other = new_allocator<U>;
    };

    MSTL_CONSTEXPR20 new_allocator() noexcept = default;
    template <typename U>
    MSTL_CONSTEXPR20 new_allocator(const new_allocator<U>&) noexcept {}
    MSTL_CONSTEXPR20 ~new_allocator() noexcept = default;
    MSTL_CONSTEXPR20 self& operator =(const self&) noexcept = default;

    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        if (n == 0) return nullptr;
        pointer ptr = nullptr;
        try {
            ptr = static_cast<pointer>(operator new(n * sizeof(T)));
        } catch (...) {
            throw_exception(allocate_exception());
        }
        return ptr;
    }
    MSTL_ALLOC_NODISCARD MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return self::allocate(1);
    }
    MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type = 1) noexcept {
        if(p) operator delete(p);
    }
};
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const new_allocator<T>&, const new_allocator<U>&) noexcept {
    return true;
}
template <typename T, typename U>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const new_allocator<T>&, const new_allocator<U>&) noexcept {
    return false;
}


#ifdef MSTL_SUPPORT_CUDA__
// manage memories on GPU
template <typename T>
class cuda_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using device_type = allocate_gpu_tag;
    using self = cuda_allocator<T>;

    template <typename U>
    struct rebind {
        using other = cuda_allocator<U>;
    };

    cuda_allocator() noexcept = default;
    template <typename U>
    cuda_allocator(const cuda_allocator<U>&) noexcept {}
    ~cuda_allocator() noexcept = default;
    self& operator =(const self&) noexcept = default;

    static MSTL_NODISCARD pointer allocate(const size_type n) {
        if (n == 0) return nullptr;

        pointer data_device = nullptr;
        const size_t bytes = n * sizeof(T);
        cudaError_t err = cudaMalloc(reinterpret_cast<void**>(&data_device), bytes);
        if (err != cudaSuccess || !data_device) {
            Exception(CUDAMemoryError(err));
        }
        return data_device;
    }
    static MSTL_NODISCARD pointer allocate() {
        return allocate(1);
    }

    static void deallocate(pointer p, size_type n = 0) noexcept {
        if (p) cudaFree(p);
    }
};
template <typename T, typename U>
MSTL_NODISCARD bool operator ==(
    const cuda_allocator<T>&, const cuda_allocator<U>&) noexcept {
    return true;
}
template <typename T, typename U>
MSTL_NODISCARD bool operator !=(
    const cuda_allocator<T>&, const cuda_allocator<U>&) noexcept {
    return false;
}
#endif


MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_BUILTIN_ALLOCATOR_HPP__
