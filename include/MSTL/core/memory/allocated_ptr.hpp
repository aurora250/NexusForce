#ifndef MSTL_CORE_MEMORY_ALLOCATED_PTR_HPP__
#define MSTL_CORE_MEMORY_ALLOCATED_PTR_HPP__
#include "allocator_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Alloc>
struct allocated_ptr {
    using pointer = typename allocator_traits<Alloc>::pointer;
    using value_type = typename allocator_traits<Alloc>::value_type;

private:
    Alloc* alloc_;
    pointer ptr_;

public:
    allocated_ptr(Alloc& alloc, pointer ptr) noexcept
    : alloc_(_MSTL addressof(alloc)), ptr_(ptr) {}

    template <typename Ptr, typename = enable_if_t<is_same_v<Ptr, value_type*>>>
    allocated_ptr(Alloc& alloc, Ptr ptr)
    : alloc_(_MSTL addressof(alloc)), ptr_(pointer_traits<pointer>::pointer_to(*ptr)) {}

    allocated_ptr(allocated_ptr&& guard) noexcept
    : alloc_(guard.alloc_), ptr_(guard.ptr_) {
        guard.ptr_ = nullptr;
    }

    ~allocated_ptr() {
        if (ptr_ != nullptr) {
          _MSTL allocator_traits<Alloc>::deallocate(*alloc_, ptr_, 1);
        }
    }

    allocated_ptr& operator =(nullptr_t) noexcept {
        ptr_ = nullptr;
        return *this;
    }

    value_type* get() {
        return _MSTL to_address(ptr_);
    }
};

template <typename Alloc>
allocated_ptr<Alloc> allocate_guarded(Alloc& alloc) {
    return { alloc, _MSTL allocator_traits<Alloc>::allocate(alloc, 1) };
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_ALLOCATED_PTR_HPP__
