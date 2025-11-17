#ifndef MSTL_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
#define MSTL_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
#include "../utility/pointer_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename U, typename = void>
struct get_alloc_rebind_type {
    using type = typename replace_first_parameter<U, T>::type;
};
template <typename T, typename U>
struct get_alloc_rebind_type<T, U, void_t<typename T::template rebind<U>::other>> {
    using type = typename T::template rebind<U>::other;
};


template <typename Alloc>
struct allocator_traits {
    using allocator_type    = Alloc;
    using value_type        = typename Alloc::value_type;
    using pointer           = typename get_pointer_type<Alloc>::type;
    using size_type         = typename get_size_type<Alloc>::type;
    using difference_type   = typename get_difference_type<Alloc>::type;
    using device_type       = typename Alloc::device_type;

    template <typename U>
    using rebind_alloc  = typename get_alloc_rebind_type<Alloc, U>::type;
    template <typename U>
    using rebind_traits = allocator_traits<rebind_alloc<U>>;

    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(
        Alloc& alloc, const size_type n) {
        return alloc.allocate(n);
    }
    static MSTL_CONSTEXPR20 void deallocate(Alloc& alloc, pointer ptr, size_type n) {
        alloc.deallocate(ptr, n);
    }
};

template <typename Alloc>
using alloc_ptr_t = typename allocator_traits<Alloc>::pointer;
template <typename Alloc>
using alloc_size_t = typename allocator_traits<Alloc>::size_type;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_ALLOCATOR_TRAITS_HPP__
