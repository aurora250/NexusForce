#ifndef MSTL_MEMORY_HPP__
#define MSTL_MEMORY_HPP__
#include "algobase.hpp"
#include "tuple.hpp"
#include <atomic>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_trivially_copy_assignable_v<iter_val_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _MSTL copy(first, last, result);
}

template <typename Iterator1, typename Iterator2, enable_if_t<
    !is_trivially_copy_assignable_v<iter_val_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_copy_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    Iterator2 cur = result;
    try {
        for (; first != last; ++first, ++cur)
            _MSTL construct(&*cur, *first);
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        Exception(MemoryError("uninitialized copy failed."));
    }
    return cur;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 uninitialized_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _INNER __uninitialized_copy_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_copy_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator2 cur = result;
    try {
        for (; count > 0; --count, ++first, ++cur)
            _MSTL construct(&*cur, *first);
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        Exception(MemoryError("uninitialized copy n failed."));
    }
    return pair<Iterator1, Iterator2>(first, cur);
}

template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_copy_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator1 last = first + count;
    return _MSTL make_pair(last, _MSTL uninitialized_copy(first, last, result));
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> uninitialized_copy_n(
    Iterator1 first, size_t count, Iterator2 result) {
    return _INNER __uninitialized_copy_n_aux(first, count, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator, typename T, enable_if_t<
    is_trivially_copy_assignable_v<iter_val_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 void __uninitialized_fill_aux(Iterator first, Iterator last, T&& x) {
    _MSTL fill(first, last, _MSTL forward<T>(x));
}

template <typename Iterator, typename T, enable_if_t<
    !is_trivially_copy_assignable_v<iter_val_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 void __uninitialized_fill_aux(Iterator first, Iterator last, T&& x) {
    Iterator cur = first;
    try {
        for (; cur != last; ++cur)
            _MSTL construct(&*cur, _MSTL forward<T>(x));
    }
    catch (...) {
        for (; cur != first; --cur)
            _MSTL destroy(&*cur);
        Exception(MemoryError("uninitialized fill failed."));
    }
}
MSTL_END_INNER__

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 void uninitialized_fill(Iterator first, Iterator last, T&& x) {
    _INNER __uninitialized_fill_aux(first, last, _MSTL forward<T>(x));
}


MSTL_BEGIN_INNER__
template <typename Iterator, typename T, enable_if_t<
    is_trivially_copy_assignable_v<iter_val_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 Iterator __uninitialized_fill_n_aux(Iterator first, size_t n, T&& x) {
    return _MSTL fill_n(first, n, _MSTL forward<T>(x));
}

template <typename Iterator, typename T, enable_if_t<
    !is_trivially_copy_assignable_v<iter_val_t<Iterator>>, int> = 0>
MSTL_CONSTEXPR20 Iterator __uninitialized_fill_n_aux(Iterator first, size_t n, T&& x) {
    Iterator cur = first;
    try{
        for (; n > 0; --n, ++cur)
            _MSTL construct(&*cur, _MSTL forward<T>(x));
    }
    catch (...) {
        for (; cur != first; --cur)
            _MSTL destroy(&*cur);
        Exception(MemoryError("uninitialized fill n failed."));
    }
    return cur;
}
MSTL_END_INNER__

template <typename Iterator, typename T, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
MSTL_CONSTEXPR20 Iterator uninitialized_fill_n(Iterator first, size_t n, T&& x) {
    return _INNER __uninitialized_fill_n_aux(first, n, _MSTL forward<T>(x));
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<
    is_trivially_copy_assignable_v<iter_val_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _MSTL move(first, last, result);
}

template <typename Iterator1, typename Iterator2, enable_if_t<
    !is_trivially_copy_assignable_v<iter_val_t<Iterator1>>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 __uninitialized_move_aux(Iterator1 first, Iterator1 last, Iterator2 result) {
    Iterator2 cur = result;
    try{
        for (; first != last; ++first, ++cur)
            _MSTL construct(&*cur, _MSTL move(*first));
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        Exception(MemoryError("uninitialized move failed."));
    }
    return cur;
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 Iterator2 uninitialized_move(Iterator1 first, Iterator1 last, Iterator2 result) {
    return _INNER __uninitialized_move_aux(first, last, result);
}


MSTL_BEGIN_INNER__
template <typename Iterator1, typename Iterator2, enable_if_t<!is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_move_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator2 cur = result;
    try{
        for (; count > 0; --count, ++first, ++cur)
            _MSTL construct(&*cur, _MSTL move(*first));
    }
    catch (...) {
        for (; result != cur; --cur)
            _MSTL destroy(&*cur);
        Exception(MemoryError("uninitialized move n failed."));
    }
    return pair<Iterator1, Iterator2>(first, cur);
}

template <typename Iterator1, typename Iterator2, enable_if_t<is_ranges_rnd_iter_v<Iterator1>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> __uninitialized_move_n_aux(
    Iterator1 first, size_t count, Iterator2 result) {
    Iterator1 last = first + count;
    return _MSTL make_pair(last, _MSTL uninitialized_move(first, last, result));
}
MSTL_END_INNER__

template <typename Iterator1, typename Iterator2, enable_if_t<
    is_ranges_input_iter_v<Iterator1> && is_ranges_fwd_iter_v<Iterator2>, int> = 0>
MSTL_CONSTEXPR20 pair<Iterator1, Iterator2> uninitialized_move_n(
    Iterator1 first, size_t count, Iterator2 result) {
    return _INNER __uninitialized_move_n_aux(first, count, result);
}


template <typename Iterator, typename T = iter_val_t<Iterator>>
struct temporary_buffer {
    static_assert(is_ranges_fwd_iter_v<Iterator>, "temporary buffer requires forward iterator types.");

public:
    using value_type        = T;
    using pointer           = value_type*;
    using const_pointer     = const value_type*;
    using reference         = value_type&;
    using const_reference   = const value_type&;
    using size_type         = ptrdiff_t;
    using difference_type   = ptrdiff_t;

private:
    size_type original_len_ = 0;
    size_type len_ = 0;
    pointer buffer_ = nullptr;

private:
    MSTL_CONSTEXPR20 void allocate_buffer() {
        original_len_ = len_;
        buffer_ = 0;
        if (len_ > static_cast<size_type>(UINT32_MAX_SIZE / sizeof(value_type)))
            len_ = UINT32_MAX_SIZE / sizeof(value_type);

        while (len_ > 0) {
            buffer_ = static_cast<pointer>(std::malloc(len_ * sizeof(value_type)));
            if (buffer_) break;
            len_ /= 2;
        }
    }

    template <typename U = value_type, enable_if_t<is_trivially_copy_assignable_v<U>, int> = 0>
    MSTL_CONSTEXPR20 void initialize_buffer(const U&) noexcept {}
    template <typename U = value_type, enable_if_t<!is_trivially_copy_assignable_v<U>, int> = 0>
    MSTL_CONSTEXPR20 void initialize_buffer(const U& val) {
        _MSTL uninitialized_fill_n(buffer_, len_, val);
    }

public:
    temporary_buffer(const temporary_buffer&) = delete;
    void operator =(const temporary_buffer&) = delete;

    MSTL_CONSTEXPR20 temporary_buffer(Iterator first, Iterator last) {
        try {
            len_ = _MSTL distance(first, last);
            this->allocate_buffer();
            if (len_ > 0) this->initialize_buffer(*first);
        }
        catch (...) {
            std::free(buffer_);
            buffer_ = 0;
            len_ = 0;
            throw;
        }
    }

    MSTL_CONSTEXPR20 ~temporary_buffer() {
        _MSTL destroy(buffer_, buffer_ + len_);
        std::free(buffer_);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const noexcept { return len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type requested_size() const noexcept { return original_len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer begin() noexcept { return buffer_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer end() noexcept { return buffer_ + len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer cbegin() const noexcept { return buffer_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer cend() const noexcept { return buffer_ + len_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const noexcept { return len_ == 0; }
};


MSTL_BEGIN_INNER__

template <typename Ptr, typename Elem>
struct __ptr_traits_base {
    using pointer = Ptr;
    using element_type = Elem;
    using difference_type = get_ptr_difference_type_t<Ptr>;
    using reference = conditional_t<is_void_v<Elem>, char, Elem>&;

    template <typename U>
    using rebind = typename get_rebind_type<Ptr, U>::type;

    MSTL_NODISCARD static constexpr pointer pointer_to(reference x)
        noexcept(noexcept(Ptr::pointer_to(x))) {
        return Ptr::pointer_to(x);
    }
};

template <typename, typename = void, typename = void>
struct __ptr_traits_extract {};

template <typename T, typename U>
struct __ptr_traits_extract<T, U, void_t<typename get_first_parameter<T>::type>>
    : __ptr_traits_base<T, typename get_first_parameter<T>::type> {
};

template <typename T>
struct __ptr_traits_extract<T, void_t<typename T::element_type>, void>
    : __ptr_traits_base<T, typename T::element_type> {
};

MSTL_END_INNER__

template <typename T>
struct pointer_traits : _INNER __ptr_traits_extract<T> {};

template <typename T>
struct pointer_traits<T*> {
    using pointer = T*;
    using element_type = T;
    using difference_type = ptrdiff_t;
    using reference = conditional_t<is_void_v<T>, char, T>&;

    template <typename U>
    using rebind = U*;

    MSTL_NODISCARD static constexpr pointer pointer_to(reference x) noexcept {
        return _MSTL addressof(x);
    }
};

template <typename Ptr>
constexpr decltype(auto) ptr_const_cast(Ptr ptr) noexcept {
    using T = typename pointer_traits<Ptr>::element_type;
    using NonConst = remove_const_t<T>;
    using Dest = typename pointer_traits<Ptr>::template rebind<NonConst>;

    return pointer_traits<Dest>::pointer_to(const_cast<NonConst&>(*ptr));
}
template <typename T>
constexpr decltype(auto) ptr_const_cast(T* ptr) noexcept {
    return const_cast<remove_const_t<T>*>(ptr);
}


template <typename T, typename = void>
struct get_pointer_type {
    using type = typename T::value_type*;
};
template <typename T>
struct get_pointer_type<T, void_t<typename T::pointer>> {
    using type = typename T::pointer;
};

template <typename T, typename = void>
struct get_difference_type {
    using pointer = typename get_pointer_type<T>::type;
    using type = typename pointer_traits<pointer>::difference_type;
};
template <typename T>
struct get_difference_type<T, void_t<typename T::difference_type>> {
    using type = typename T::difference_type;
};

template <typename T, typename = void>
struct get_size_type {
    using type = make_unsigned_t<typename get_difference_type<T>::type>;
};
template <typename T>
struct get_size_type<T, void_t<typename T::size_type>> {
    using type = typename T::size_type;
};

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


MSTL_BEGIN_INNER__

template <size_t>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_aux(const size_t bytes) {
#ifdef MSTL_COMPILER_MSVC__
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD) {
        const size_t block_size = MEMORY_NO_USER_SIZE + bytes;
        if (block_size <= bytes)
            Exception(MemoryError("invalid block size."));
        const auto holder = reinterpret_cast<uintptr_t>(::operator new(block_size));
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
    return ::operator new(bytes);
}

#ifdef MSTL_VERSION_17__
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD) ,int> = 0>
MSTL_ALLOC_OPTIMIZE MSTL_CONSTEXPR20 void* __allocate_dispatch(const size_t bytes) {
    size_t align = Align;
#ifdef MSTL_COMPILER_MSVC__
    if (bytes >= MEMORY_BIG_ALLOC_THRESHHOLD)
        align = _MSTL max(Align, MEMORY_BIG_ALLOC_ALIGN);
#endif
#if defined(MSTL_COMPILER_CLANG__) && defined(MSTL_VERSION_20__)
    if (_MSTL is_constant_evaluated())
        return ::operator new(bytes);
#endif
    return ::operator new(bytes, std::align_val_t{ align });
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
#ifdef MSTL_VERSION_20__
    if (_MSTL is_constant_evaluated())
        return ::operator new(bytes);
#endif // MSTL_VERSION_20__

#ifdef MSTL_VERSION_17__
    return _INNER __allocate_dispatch<Align>(bytes);
#else
    return _INNER __allocate_aux<Align>(bytes);
#endif // MSTL_VERSION_17__
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
#if defined(MSTL_VERSION_17__) && !defined(MSTL_COMPILER_CLANG__)
    ::operator delete(ptr, bytes);
#else
    ::operator delete(ptr);
#endif
}

#ifdef MSTL_VERSION_17__
template <size_t Align, enable_if_t<(Align > MEMORY_ALIGN_THRESHHOLD), int> = 0>
MSTL_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, size_t& bytes) noexcept {
    size_t align = Align;
#ifdef MSTL_COMPILER_MSVC__
    if (bytes > MEMORY_BIG_ALLOC_THRESHHOLD)
        align = _MSTL max(Align, MEMORY_BIG_ALLOC_ALIGN);
#endif
#ifdef MSTL_COMPILER_CLANG__
    ::operator delete(ptr, std::align_val_t{ align });
#else
    ::operator delete(ptr, bytes, std::align_val_t{ align });
#endif
}

template<size_t Align, enable_if_t<Align <= MEMORY_ALIGN_THRESHHOLD, int> = 0>
MSTL_CONSTEXPR20 void __deallocate_dispatch(void*& ptr, size_t& bytes) noexcept {
    _INNER __deallocate_aux<Align>(ptr, bytes);
}
#endif // MSTL_VERSION_17__

MSTL_END_INNER__

template <size_t Align>
MSTL_CONSTEXPR20 void deallocate(void* ptr, size_t bytes) noexcept {
#ifdef MSTL_VERSION_20__
    if (_MSTL is_constant_evaluated()) {
        ::operator delete(ptr);
        return;
    }
#endif // MSTL_VERSION_20__

#ifdef MSTL_VERSION_17__
    _INNER __deallocate_dispatch<Align>(ptr, bytes);
#else
    _INNER __deallocate_aux<Align>(ptr, bytes);
#endif // MSTL_VERSION_17__
}

MSTL_BEGIN_INNER__
template <typename T>
constexpr size_t __FINAL_ALIGN_SIZE = _MSTL max(alignof(T), MEMORY_ALIGN_THRESHHOLD);
MSTL_END_INNER__


MSTL_ERROR_BUILD_FINAL_CLASS(AllocateError, MemoryError, "Memory Allocation Failed.")


struct allocate_cpu_tag {
    constexpr allocate_cpu_tag() noexcept = default;
    ~allocate_cpu_tag() noexcept = default;
};

struct allocate_gpu_tag {
    constexpr allocate_gpu_tag() noexcept = default;
    ~allocate_gpu_tag() noexcept = default;
};


template <typename T>
class standard_allocator {
    static_assert(is_allocable_v<T>, "allocator can`t alloc void, reference, function or const type.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using device_type = allocate_cpu_tag;
    using self = standard_allocator<T>;

    template <typename U>
    struct rebind {
        using other = standard_allocator<U>;
    };

    MSTL_CONSTEXPR20 standard_allocator() noexcept = default;
    template <typename U>
    MSTL_CONSTEXPR20 standard_allocator(const standard_allocator<U>&) noexcept {}
    MSTL_CONSTEXPR20 ~standard_allocator() noexcept = default;
    MSTL_CONSTEXPR20 self& operator =(const self&) noexcept = default;

    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        constexpr size_t value_size = sizeof(value_type);
        static_assert(value_size > 0, "value type must be complete before allocation called.");
        const size_t alloc_size = value_size * n;
        MSTL_DEBUG_VERIFY(alloc_size <= UINT64_MAX_SIZE, "allocation will cause memory overflow.");
        return static_cast<T*>(_MSTL allocate<_INNER __FINAL_ALIGN_SIZE<T>>(alloc_size));
    }

    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return self::allocate(1);
    }

    static MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type n) noexcept {
        constexpr size_t value_size = sizeof(value_type);
        MSTL_DEBUG_VERIFY(p != nullptr || n == 0, "null pointer cannot point to a block of non-zero size");
        _MSTL deallocate<_INNER __FINAL_ALIGN_SIZE<T>>(p, n * value_size);
    }

    static MSTL_CONSTEXPR20 void deallocate(const pointer p) noexcept {
        self::deallocate(p, 1);
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

    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        if (n == 0) return nullptr;
        pointer ptr = nullptr;
        try {
            ptr = static_cast<pointer>(std::malloc(n * sizeof(T)));
        }
        catch (...) {
            Exception(AllocateError());
        }
        return ptr;
    }
    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return self::allocate(1);
    }
    static MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type = 1) noexcept {
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

    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate(const size_type n) {
        if (n == 0) return nullptr;
        pointer ptr = nullptr;
        try {
            ptr = static_cast<pointer>(::operator new(n * sizeof(T)));
        }
        catch (...) {
            Exception(AllocateError());
        }
        return ptr;
    }
    MSTL_ALLOC_NODISCARD static MSTL_CONSTEXPR20 MSTL_ALLOC_OPTIMIZE pointer allocate() {
        return self::allocate(1);
    }
    static MSTL_CONSTEXPR20 void deallocate(pointer p, const size_type = 1) noexcept {
        if(p) ::operator delete(p);
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


// equal to new_allocator under GNUC and hold a dispatch under MSVC
template <typename T>
using allocator = standard_allocator<T>;

template <typename Alloc>
using alloc_ptr_t = typename allocator_traits<Alloc>::pointer;
template <typename Alloc>
using alloc_size_t = typename allocator_traits<Alloc>::size_type;


template <typename T>
struct default_delete {
    constexpr default_delete() noexcept = default;

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
	MSTL_CONSTEXPR23 default_delete(const default_delete<U>&) noexcept {}

    MSTL_CONSTEXPR23 void operator()(const T* ptr) const {
	    static_assert(is_allocable_v<T>, "can not delete types which can`t be allocated.");
	    delete ptr;
    }
};

template <typename T>
struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;

    template <typename U, enable_if_t<is_convertible_v<U(*)[], T(*)[]>, int> = 0>
    MSTL_CONSTEXPR23 default_delete(const default_delete<U[]>&) noexcept {}

    template <typename U, enable_if_t<is_convertible_v<U(*)[], T(*)[]>, int> = 0>
    MSTL_CONSTEXPR23 void operator ()(U* ptr) const{
	    static_assert(is_allocable_v<T>, "can not delete types which can`t be allocated.");
	    delete [] ptr;
	}
};


MSTL_BEGIN_INNER__

template <typename T, typename Deleter>
class __unique_ptr_impl {
private:
    template <typename U, typename E, typename = void>
	struct inner_ptr {
	    using type = U*;
	};
    template <typename U, typename E>
	struct inner_ptr<U, E, void_t<typename remove_reference_t<E>::pointer>> {
	    using type = typename remove_reference<E>::type::pointer;
	};

public:
    using DeleterConstraint = enable_if<is_default_constructible_v<Deleter>>;

    using pointer = typename inner_ptr<T, Deleter>::type;

private:
    _MSTL tuple<pointer, Deleter> tup{};

    static_assert(!is_rvalue_reference_v<Deleter>,
        "deleter type of unique_ptr must be a function object type or an lvalue reference type");

public:
    __unique_ptr_impl() = default;
    MSTL_CONSTEXPR23 __unique_ptr_impl(pointer ptr) : tup() { get_ptr() = ptr; }

    template <typename Del>
    MSTL_CONSTEXPR23 __unique_ptr_impl(pointer ptr, Del&& del)
	: tup(ptr, _MSTL forward<Del>(del)) {}

    MSTL_CONSTEXPR23 __unique_ptr_impl(__unique_ptr_impl&& ptr) noexcept
    : tup(_MSTL move(ptr.tup)) { ptr.get_ptr() = nullptr; }

    MSTL_CONSTEXPR23 __unique_ptr_impl& operator =(__unique_ptr_impl&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<Deleter>(x.get_deleter());
	    return *this;
    }

    MSTL_CONSTEXPR23 pointer& get_ptr() noexcept { return _MSTL get<0>(tup); }
    MSTL_CONSTEXPR23 pointer get_ptr() const noexcept { return _MSTL get<0>(tup); }
    MSTL_CONSTEXPR23 Deleter& get_deleter() noexcept { return _MSTL get<1>(tup); }
    MSTL_CONSTEXPR23 const Deleter& get_deleter() const noexcept { return _MSTL get<1>(tup); }

    MSTL_CONSTEXPR23 void reset(pointer ptr) noexcept {
	    const pointer old = get_ptr();
	    get_ptr() = ptr;
	    if (old)
	        get_deleter()(old);
    }

    MSTL_CONSTEXPR23 pointer release() noexcept {
	    pointer p = get_ptr();
	    get_ptr() = nullptr;
	    return p;
    }

    MSTL_CONSTEXPR23 void swap(__unique_ptr_impl& x) noexcept {
	    _MSTL swap(get_ptr(), x.get_ptr());
	    _MSTL swap(get_deleter(), x.get_deleter());
    }
};

template <typename T, typename Deleter,
    bool = is_move_constructible_v<Deleter>,
    bool = is_move_assignable_v<Deleter>>
struct __unique_ptr_data : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = default;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = default;
};

template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, true, false> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = default;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = delete;
};

template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, false, true> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = delete;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = default;
};

template <typename T, typename Deleter>
struct __unique_ptr_data<T, Deleter, false, false> : __unique_ptr_impl<T, Deleter> {
    using base_type = __unique_ptr_impl<T, Deleter>;
    using pointer = typename base_type::pointer;

    __unique_ptr_data() = default;
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr) : base_type(ptr) {}

    template <typename Del>
    MSTL_CONSTEXPR23 __unique_ptr_data(pointer ptr, Del&& del) : base_type(ptr, _MSTL forward<Del>(del)) {}

    __unique_ptr_data(__unique_ptr_data&&) = delete;
    __unique_ptr_data& operator =(__unique_ptr_data&&) = delete;
};

MSTL_END_INNER__


template <typename T, typename Deleter = default_delete<T>>
class unique_ptr {
private:
    template <typename U>
	using DeleterConstraint = typename _INNER __unique_ptr_impl<T, U>::DeleterConstraint::type;

    _INNER __unique_ptr_data<T, Deleter> data_{};

public:
    using pointer       = typename _INNER __unique_ptr_impl<T, Deleter>::pointer;
    using element_type  = T;
    using deleter_type  = Deleter;

private:
    template <typename U, typename E>
	using safe_conversion = conjunction<
	    is_convertible<typename unique_ptr<U, E>::pointer, pointer>, negation<is_array<U>>>;

public:
    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
    MSTL_CONSTEXPR23 unique_ptr(pointer p) noexcept : data_(p) {}

    template <typename Del = deleter_type, enable_if_t<is_copy_constructible_v<Del>, int> = 0>
    MSTL_CONSTEXPR23 unique_ptr(pointer ptr, const deleter_type& del) noexcept : data_(ptr, del) {}

    template <typename Del = deleter_type, enable_if_t<is_move_constructible_v<Del>, int> = 0>
    MSTL_CONSTEXPR23 unique_ptr(pointer ptr, Del&& del) noexcept : data_(ptr, _MSTL move(del)) {}

    template<typename Del = deleter_type, typename DelMoveRef = remove_reference_t<Del>>
    MSTL_CONSTEXPR23 unique_ptr(pointer, enable_if_t<is_lvalue_reference_v<Del>, DelMoveRef&&>) = delete;

    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
	constexpr unique_ptr(nullptr_t = nullptr) noexcept : data_() {}

    MSTL_CONSTEXPR23 unique_ptr(unique_ptr&&) = default;

    template <typename U, typename E, enable_if_t<conjunction_v<safe_conversion<U, E>,
        conditional_t<is_reference_v<Deleter>, is_same<E, Deleter>, is_convertible<E, Deleter>>>, int> = 0>
    MSTL_CONSTEXPR23 unique_ptr(unique_ptr<U, E>&& x) noexcept
	    : data_(x.release(), _MSTL forward<E>(x.get_deleter())) {}

    ~unique_ptr() noexcept {
	    static_assert(is_invocable_v<deleter_type&, pointer>, "deleter of unique_ptr must be invocable with a pointer");
	    auto& ptr = data_.get_ptr();
	    if (ptr != nullptr)
	        get_deleter()(_MSTL move(ptr));
	    ptr = pointer();
    }

    unique_ptr& operator =(unique_ptr&&) = default;

    template <typename U, typename E, enable_if_t<conjunction_v<
        safe_conversion<U, E>, is_assignable<deleter_type&, E&&>>, int> = 0>
	MSTL_CONSTEXPR23 unique_ptr& operator =(unique_ptr<U, E>&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<E>(x.get_deleter());
	    return *this;
	}

    MSTL_CONSTEXPR23 unique_ptr& operator =(nullptr_t) noexcept {
	    reset();
	    return *this;
    }

    MSTL_CONSTEXPR23 add_lvalue_reference_t<element_type> operator *() const
    noexcept(noexcept(*_MSTL declval<pointer>())) {
	    MSTL_DEBUG_VERIFY(get() != pointer(), "_MSTL add_lvalue_reference_t<element_type> failed");
	    return *get();
    }
    MSTL_CONSTEXPR23 pointer operator ->() const noexcept {
	    MSTL_DEBUG_VERIFY(get() != pointer(), "_MSTL operator->() failed");
	    return get();
    }

    MSTL_CONSTEXPR23 pointer get() const noexcept { return data_.get_ptr(); }
    MSTL_CONSTEXPR23 deleter_type& get_deleter() noexcept { return data_.get_deleter(); }
    MSTL_CONSTEXPR23 const deleter_type& get_deleter() const noexcept { return data_.get_deleter(); }

    MSTL_CONSTEXPR23 explicit operator bool() const noexcept {
        return get() == pointer() ? false : true;
    }

    MSTL_CONSTEXPR23 pointer release() noexcept { return data_.release(); }
    MSTL_CONSTEXPR23 void reset(pointer ptr = pointer()) noexcept {
	    static_assert(is_invocable_v<deleter_type&, pointer>,
	        "deleter of unique_ptr must be invocable with a pointer");
	    data_.reset(_MSTL move(ptr));
    }

    MSTL_CONSTEXPR23 void swap(unique_ptr& x) noexcept {
	    static_assert(is_swappable_v<Deleter>, "deleter must be swappable.");
	    data_.swap(x.data_);
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator =(const unique_ptr&) = delete;
};

template <typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
    template <typename _Up>
    using DeleterConstraint = typename _INNER __unique_ptr_impl<T, _Up>::DeleterConstraint::type;

    _INNER __unique_ptr_data<T, Deleter> data_;

public:
    using pointer	        = typename _INNER __unique_ptr_impl<T, Deleter>::pointer;
    using element_type    = T;
    using deleter_type    = Deleter;

    template <typename U, typename E, typename UP = unique_ptr<U, E>,
        typename UP_pointer = typename UP::pointer,
        typename UP_element_type = typename UP::element_type>
	using safe_conversion = conjunction<is_array<U>, is_same<pointer, element_type*>,
          is_same<UP_pointer, UP_element_type*>, is_convertible<UP_element_type(*)[], element_type(*)[]>>;

    template <typename U>
    using safe_conversion_raw = conjunction<disjunction<disjunction<is_same<U, pointer>, is_same<U, nullptr_t>>,
        conjunction<is_pointer<U>, is_same<pointer, element_type*>,
            is_convertible<remove_pointer_t<U>(*)[],element_type(*)[]>>>>;

    template <typename U, typename Del = Deleter, typename = DeleterConstraint<Del>,
        enable_if_t<safe_conversion_raw<U>::value, int> = 0>
    MSTL_CONSTEXPR23 explicit unique_ptr(U ptr) noexcept : data_(ptr) {}

    template <typename U, typename Del = deleter_type,
        enable_if_t<conjunction_v<safe_conversion_raw<U>, is_copy_constructible<Del>>, int> = 0>
    MSTL_CONSTEXPR23 unique_ptr(U ptr, const deleter_type& del) noexcept : data_(ptr, del) {}

    template <typename U, typename Del = deleter_type,
        enable_if_t<conjunction_v<safe_conversion_raw<U>, is_move_constructible<Del>>, int> = 0>
    MSTL_CONSTEXPR23 unique_ptr(U ptr, enable_if_t<!is_lvalue_reference_v<Del>, Del&&> del) noexcept
    	: data_(_MSTL move(ptr), _MSTL move(del)) {}

    template <typename U, typename Del = deleter_type, typename DelMoveRef = remove_reference_t<Del>,
        enable_if_t<safe_conversion_raw<U>::value, int> = 0>
	unique_ptr(U, enable_if_t<is_lvalue_reference_v<Del>, DelMoveRef&&>) = delete;

    unique_ptr(unique_ptr&&) = default;

    template <typename Del = Deleter, typename = DeleterConstraint<Del>>
	constexpr unique_ptr(nullptr_t = nullptr) noexcept : data_() {}

    template <typename U, typename E, enable_if_t<conjunction_v<safe_conversion<U, E>,
	       conditional_t<is_reference_v<Deleter>, is_same<E, Deleter>, is_convertible<E, Deleter>>>, int> = 0>
    MSTL_CONSTEXPR23 unique_ptr(unique_ptr<U, E>&& x) noexcept
	    : data_(x.release(), _MSTL forward<E>(x.get_deleter())) {}

    MSTL_CONSTEXPR20 ~unique_ptr() {
	    auto& ptr = data_.get_ptr();
	    if (ptr != nullptr)
	        get_deleter()(ptr);
	    ptr = pointer();
    }

    unique_ptr& operator =(unique_ptr&&) = default;

    template <typename U, typename E, enable_if_t<conjunction_v<
        safe_conversion<U, E>, is_assignable<deleter_type&, E&&>, int>> = 0>
    MSTL_CONSTEXPR23 unique_ptr& operator =(unique_ptr<U, E>&& x) noexcept {
	    reset(x.release());
	    get_deleter() = _MSTL forward<E>(x.get_deleter());
	    return *this;
	}

    MSTL_CONSTEXPR23 unique_ptr& operator =(nullptr_t) noexcept {
	    reset();
	    return *this;
    }

    MSTL_CONSTEXPR23 add_lvalue_reference_t<element_type> operator [](size_t idx) const {
	    MSTL_DEBUG_VERIFY(get() != pointer(), "_MSTL add_lvalue_reference_t<element_type> failed");
	    return get()[idx];
    }

    MSTL_CONSTEXPR23 pointer get() const noexcept { return data_.get_ptr(); }
    MSTL_CONSTEXPR23 deleter_type& get_deleter() noexcept { return data_.get_deleter(); }
    MSTL_CONSTEXPR23 const deleter_type& get_deleter() const noexcept { return data_.get_deleter(); }

    MSTL_CONSTEXPR23 explicit operator bool() const noexcept {
        return get() == pointer() ? false : true;
    }

    MSTL_CONSTEXPR23 pointer release() noexcept { return data_.release(); }
    template <typename U, enable_if_t<conjunction_v<disjunction<is_same<U, pointer>, conjunction<
        is_same<pointer, element_type*>, is_pointer<U>, is_convertible<remove_pointer_t<U>(*)[],element_type(*)[]>>>>, int> = 0>
    MSTL_CONSTEXPR23 void reset(U ptr) noexcept { data_.reset(_MSTL move(ptr)); }
    MSTL_CONSTEXPR23 void reset(nullptr_t = nullptr) noexcept { reset(pointer()); }

    MSTL_CONSTEXPR23 void swap(unique_ptr& x) noexcept {
	    static_assert(is_swappable_v<Deleter>, "deleter must be swappable");
	    data_.swap(x.data_);
    }

    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator =(const unique_ptr&) = delete;
};

template <typename T, typename Deleter, enable_if_t<is_swappable_v<Deleter> && is_swappable_v<T>, int> = 0>
void swap(unique_ptr<T, Deleter>& lh, unique_ptr<T, Deleter>& rh) noexcept {
    lh.swap(rh);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator ==(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return lh.get() == rh.get();
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator ==(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return !lh;
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator ==(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return !rh;
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator !=(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return lh.get() != rh.get();
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator !=(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return static_cast<bool>(lh);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator !=(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return static_cast<bool>(rh);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator <(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    using common_t = common_type_t<typename unique_ptr<T, D>::pointer, typename unique_ptr<U, E>::pointer>;
    return _MSTL less<common_t>()(lh.get(), rh.get());
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator <(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(lh.get(), nullptr);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator <(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(nullptr, rh.get());
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator >(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return rh.get() < lh.get();
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator >(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(nullptr, lh.get());
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator >(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return _MSTL less<typename unique_ptr<T, D>::pointer>()(rh.get(), nullptr);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator <=(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return !(lh > rh);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator <=(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return !(lh > nullptr);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator <=(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return !(nullptr > rh);
}

template <typename T, typename D, typename U, typename E>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator >=(
    const unique_ptr<T, D>& lh, const unique_ptr<U, E>& rh) {
    return !(lh < rh);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator >=(
    const unique_ptr<T, D>& lh, nullptr_t) {
    return !(lh < nullptr);
}
template <typename T, typename D>
MSTL_NODISCARD MSTL_CONSTEXPR23 bool operator >=(
    nullptr_t, const unique_ptr<T, D>& rh) {
    return !(nullptr < rh);
}

template <typename T, typename Deleter>
struct hash<unique_ptr<T, Deleter>> {
    MSTL_CONSTEXPR23 size_t operator ()(const unique_ptr<T, Deleter>& ptr) const
    noexcept(noexcept(_MSTL declval<_MSTL hash<
        typename unique_ptr<T, Deleter>::pointer>>()(_MSTL declval<typename unique_ptr<T, Deleter>::pointer>()))) {
        return hash<T>()(ptr.get());
    }
};


template <typename T, typename... Args, enable_if_t<!is_array_v<T>, int> = 0>
MSTL_CONSTEXPR23 unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(_MSTL forward<Args>(args)...));
}
template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_CONSTEXPR23 unique_ptr<T> make_unique(const size_t len) {
    return unique_ptr<T>(new remove_extent_t<T>[len]());
}
template <typename T, typename... Args, enable_if_t<is_bounded_array_v<T>, int> = 0>
unique_ptr<T> make_unique(Args&&...) = delete;


template <typename T, enable_if_t<!is_array_v<T>, int> = 0>
MSTL_CONSTEXPR23 unique_ptr<T> make_unique_for_overwrite() {
    return unique_ptr<T>(new T());
}
template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
MSTL_CONSTEXPR23 unique_ptr<T> make_unique_for_overwrite(const size_t len) {
    return unique_ptr<T>(new remove_extent_t<T>[len]());
}
template <typename T, typename... Args, enable_if_t<is_bounded_array_v<T>, int> = 0>
unique_ptr<T> make_unique_for_overwrite(Args&&...) = delete;


MSTL_BEGIN_INNER__

struct __smart_ptr_counter {
    std::atomic_ulong count_;

    __smart_ptr_counter() noexcept : count_(1) {}
    __smart_ptr_counter(__smart_ptr_counter&&) = delete;
    virtual ~__smart_ptr_counter() = default;

    void incref() noexcept {
        count_.fetch_add(1, std::memory_order_relaxed);
    }
    void decref() noexcept {
        if (count_.fetch_sub(1, std::memory_order_relaxed) == 1) {
            delete this;
        }
    }
    MSTL_NODISCARD uint64_t countref() const noexcept {
        return count_.load(std::memory_order_relaxed);
    }
};

template <typename T, typename Deleter>
struct __smart_ptr_counter_impl final : __smart_ptr_counter {
    T* ptr_;
    MSTL_NO_UNIQUE_ADDRESS Deleter deleter_;

    explicit __smart_ptr_counter_impl(T* ptr) noexcept : ptr_(ptr) {}

    explicit __smart_ptr_counter_impl(T* ptr, Deleter deleter) noexcept
        : ptr_(ptr), deleter_(_MSTL move(deleter)) {}

    ~__smart_ptr_counter_impl() noexcept override {
        deleter_(ptr_);
    }
};

template <typename T, typename Deleter>
struct __smart_ptr_counter_impl_fused final : __smart_ptr_counter {
    T* ptr_;
    void* mem_;
    MSTL_NO_UNIQUE_ADDRESS Deleter deleter_;

    explicit __smart_ptr_counter_impl_fused(T* ptr, void* mem, Deleter deleter) noexcept
        : ptr_(ptr), mem_(mem), deleter_(_MSTL move(deleter)) {}

    ~__smart_ptr_counter_impl_fused() noexcept override {
        deleter_(ptr_);
    }

    void operator delete(void* mem) noexcept {
#if MSTL_VERSION_17__
        ::operator delete(mem, static_cast<std::align_val_t>(
            _MSTL max(alignof(T), alignof(__smart_ptr_counter_impl_fused))));
#else
        ::operator delete(mem);
#endif
    }
};

MSTL_END_INNER__


template <typename T>
struct enable_shared_from_this;

template <typename T>
class shared_ptr;


MSTL_BEGIN_INNER__

template <typename T>
void __set_enable_shared_from(_MSTL enable_shared_from_this<T>* ptr, __smart_ptr_counter* owner) {
    ptr->owner_ = owner;
}
template <typename T, _MSTL enable_if_t<_MSTL is_base_of_v<enable_shared_from_this<T>, T>, int> = 0>
void __setup_enable_shared_from(T* ptr, __smart_ptr_counter* owner) {
    (__set_enable_shared_from)(static_cast<_MSTL enable_shared_from_this<T>*>(ptr), owner);
}
template <typename T, _MSTL enable_if_t<!_MSTL is_base_of_v<enable_shared_from_this<T>, T>, int> = 0>
void __setup_enable_shared_from(T*, __smart_ptr_counter*) {}

template <typename T>
_MSTL shared_ptr<T> __make_shared_fused(T* ptr, __smart_ptr_counter* owner) noexcept {
    return _MSTL shared_ptr<T>(ptr, owner);
}

MSTL_END_INNER__

template <typename T>
class shared_ptr {
private:
    T* ptr_ = nullptr;
    _INNER __smart_ptr_counter* owner_ = nullptr;

    explicit shared_ptr(T* ptr, _INNER __smart_ptr_counter* owner) noexcept : ptr_(ptr), owner_(owner) {}

    template <typename>
    friend class shared_ptr;

    template <typename U>
    friend shared_ptr<U> _INNER __make_shared_fused(U*, _INNER __smart_ptr_counter*) noexcept;

public:
    using element_type  = T;
    using pointer       = T*;
    using self          = shared_ptr<T>;

    shared_ptr(nullptr_t = nullptr) noexcept {}

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(U* ptr)
    : ptr_(ptr), owner_(new _INNER __smart_ptr_counter_impl<U, default_delete<U>>(ptr)) {
        _INNER __setup_enable_shared_from<T>(ptr_, owner_);
    }

    template <typename U, typename Deleter, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(U* ptr, Deleter deleter)
    : ptr_(ptr), owner_(new _INNER __smart_ptr_counter_impl<U, Deleter>(ptr, _MSTL move(deleter))) {
        _INNER __setup_enable_shared_from<T>(ptr_, owner_);
    }

    template <typename U, typename Deleter, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(unique_ptr<U, Deleter>&& ptr)
    : shared_ptr(ptr.release(), ptr.get_deleter()) {}

    shared_ptr(const self& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        if (owner_) owner_->incref();
    }
    self& operator =(const self& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        if (owner_) owner_->decref();
        ptr_ = x.ptr_;
        owner_ = x.owner_;
        if (owner_) owner_->incref();
        return *this;
    }
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(const shared_ptr<U>& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        if (owner_) owner_->incref();
    }

    shared_ptr(self&& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
    }
    self& operator =(self&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        if (owner_) owner_->decref();
        ptr_ = x.ptr_;
        owner_ = x.owner_;
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
        return *this;
    }
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    explicit shared_ptr(shared_ptr<U>&& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
    }

    template <typename U>
    shared_ptr(const shared_ptr<U>& x, T* ptr) noexcept : ptr_(ptr), owner_(x.owner_) {
        if (owner_) owner_->incref();
    }
    template <typename U>
    shared_ptr(shared_ptr<U>&& x, T* ptr) noexcept : ptr_(ptr), owner_(x.owner_) {
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr& operator =(const shared_ptr<U>& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        if (owner_) owner_->decref();
        ptr_ = x.ptr_;
        owner_ = x.owner_;
        if (owner_) owner_->incref();
        return *this;
    }
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr& operator =(shared_ptr<U>&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        if (owner_) owner_->decref();
        ptr_ = x.ptr_;
        owner_ = x.owner_;
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
        return *this;
    }

    ~shared_ptr() noexcept {
        reset();
    }

    void reset() noexcept {
        if (owner_) owner_->decref();
        owner_ = nullptr;
        ptr_ = nullptr;
    }
    template <typename U>
    void reset(U* ptr) {
        if (owner_) owner_->decref();
        ptr_ = nullptr;
        owner_ = nullptr;
        ptr_ = ptr;
        owner_ = new _INNER __smart_ptr_counter_impl<U, default_delete<U>>(ptr);
        _INNER __setup_enable_shared_from<T>(ptr_, owner_);
    }
    template <typename U, typename Deleter>
    void reset(U* ptr, Deleter deleter) {
        if (owner_) owner_->decref();
        ptr_ = nullptr;
        owner_ = nullptr;
        ptr_ = ptr;
        owner_ = new _INNER __smart_ptr_counter_impl<U, Deleter>(ptr, _MSTL move(deleter));
        _INNER __setup_enable_shared_from<T>(ptr_, owner_);
    }

    MSTL_NODISCARD long use_count() const noexcept {
        return owner_ ? owner_->countref() : 0;
    }
    MSTL_NODISCARD bool unique() const noexcept {
        return owner_ ? owner_->countref() == 1 : true;
    }

    void swap(shared_ptr& x) noexcept {
        if (_MSTL addressof(x) == this) return;
        _MSTL swap(ptr_, x.ptr_);
        _MSTL swap(owner_, x.owner_);
    }

    MSTL_NODISCARD T* get() const noexcept {
        return ptr_;
    }
    MSTL_NODISCARD T* operator ->() const noexcept {
        return ptr_;
    }
    MSTL_NODISCARD add_lvalue_reference_t<T> operator *() const noexcept {
        return *ptr_;
    }

    MSTL_NODISCARD explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }
    template <typename U>
    MSTL_NODISCARD bool owner_equal(const shared_ptr<U>& rh) const noexcept {
        return owner_ == rh.owner_;
    }
    template <typename U>
    MSTL_NODISCARD bool owner_before(const shared_ptr<U>& rh) const noexcept {
        return owner_ < rh.owner_;
    }
};
template <typename T, typename U>
MSTL_NODISCARD bool operator ==(const shared_ptr<T>& lh, const shared_ptr<U>& rh) noexcept {
    return lh.owner_equal(rh);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator !=(const shared_ptr<T>& lh, const shared_ptr<U>& rh) noexcept {
    return !(lh == rh);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator <(const shared_ptr<T>& lh, const shared_ptr<U>& rh) noexcept {
    return lh.owner_before(rh);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator >(const shared_ptr<T>& lh, const shared_ptr<U>& rh) noexcept {
    return rh < lh;
}
template <typename T, typename U>
MSTL_NODISCARD bool operator <=(const shared_ptr<T>& lh, const shared_ptr<U>& rh) noexcept {
    return !(lh > rh);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator >=(const shared_ptr<T>& lh, const shared_ptr<U>& rh) noexcept {
    return !(lh < rh);
}


template <typename T>
class shared_ptr<T[]> : shared_ptr<T> {
public:
    using shared_ptr<T>::shared_ptr;

    add_lvalue_reference_t<T> operator [](size_t idx) {
        return this->get()[idx];
    }
};


template <typename T>
struct enable_shared_from_this {
private:
    _INNER __smart_ptr_counter* owner_;

protected:
    enable_shared_from_this() noexcept : owner_(nullptr) {}

    shared_ptr<T> shared_from_this() {
        static_assert(is_base_of_v<enable_shared_from_this, T>, "shared from T requires derived class");
        if (!owner_) Exception(MemoryError("smart pointer share failed."));
        owner_->incref();
        return _INNER __make_shared_fused(static_cast<T*>(this), owner_);
    }

    shared_ptr<T const> shared_from_this() const {
        static_assert(is_base_of_v<enable_shared_from_this, T>, "shared from T requires derived class");
        if (!owner_) Exception(MemoryError("smart pointer share failed."));
        owner_->incref();
        return _INNER __make_shared_fused(static_cast<T const*>(this), owner_);
    }

    template <typename U>
    friend void __set_enable_shared_from(enable_shared_from_this<U>*, _INNER __smart_ptr_counter*);
};


template <typename T, typename... Args, enable_if_t<!is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared(Args&&... args) {
    auto const deleter = [](T* ptr) noexcept { ptr->~T(); };
    using Counter = _INNER __smart_ptr_counter_impl_fused<T, decltype(deleter)>;
    constexpr size_t align = _MSTL max(alignof(T), alignof(Counter));
    constexpr size_t offset = (sizeof(Counter) + align - 1) & ~(align - 1);
    constexpr size_t size = offset + sizeof(T);
#if MSTL_VERSION_17__
    void* mem = ::operator new(size, static_cast<std::align_val_t>(align));
    auto* counter = static_cast<Counter*>(mem);
#else
    void* mem = ::operator new(size + align - 1);
    size_t aligned_addr = (reinterpret_cast<size_t>(mem) + (align - 1)) & ~(align - 1);
    Counter* counter = reinterpret_cast<Counter*>(aligned_addr);
#endif
    T* object = reinterpret_cast<T*>(reinterpret_cast<char*>(counter) + offset);
    try {
        _MSTL construct(object, _MSTL forward<Args>(args)...);
    }
    catch (...) {
#if MSTL_VERSION_17__
        ::operator delete(mem, static_cast<std::align_val_t>(align));
#else
        ::operator delete(mem);
#endif
        Exception(MemoryError("shared ptr construction failed."));
    }
    new (counter) Counter(object, mem, deleter);
    _INNER __setup_enable_shared_from(object, counter);
    return _INNER __make_shared_fused(object, counter);
}

template <typename T, enable_if_t<!is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared_for_overwrite() {
    auto const deleter = [](T* ptr) noexcept { ptr->~T(); };
    using Counter = _INNER __smart_ptr_counter_impl_fused<T, decltype(deleter)>;
    constexpr size_t align = _MSTL max(alignof(T), alignof(Counter));
    constexpr size_t offset = (sizeof(Counter) + align - 1) & ~(align - 1);
    constexpr size_t size = offset + sizeof(T);
#if MSTL_VERSION_17__
    void* mem = ::operator new(size, static_cast<std::align_val_t>(align));
    auto* counter = static_cast<Counter*>(mem);
#else
    void* mem = ::operator new(size + align - 1);
    size_t aligned_addr = (reinterpret_cast<size_t>(mem) + (align - 1)) & ~(align - 1);
    Counter* counter = reinterpret_cast<Counter*>(aligned_addr);
#endif
    T* object = reinterpret_cast<T*>(reinterpret_cast<char*>(counter) + offset);
    try{
        _MSTL construct(object);
    }
    catch (...) {
#if MSTL_VERSION_17__
        ::operator delete(mem, static_cast<std::align_val_t>(align));
#else
        ::operator delete(mem);
#endif
        Exception(MemoryError("shared ptr construction failed."));
    }
    new (counter) Counter(object, mem, deleter);
    _INNER __setup_enable_shared_from(object, counter);
    return _INNER __make_shared_fused(object, counter);
}

template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared(const size_t len) {
    using value = remove_extent_t<T>;
    auto* tmp = new value[len]();
    try {
        return shared_ptr<T>(tmp);
    }
    catch (...) {
        delete[] tmp;
        Exception(MemoryError("shared ptr construction failed."));
    }
    return nullptr;
}

template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared_for_overwrite(const size_t len) {
    using value = remove_extent_t<T>;
    auto* tmp = new value[len];
    try {
        return shared_ptr<T>(tmp);
    }
    catch (...) {
        delete[] tmp;
        Exception(MemoryError("shared ptr construction failed."));
    }
    return nullptr;
}


template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& ptr) {
    return shared_ptr<T>(ptr, static_cast<T*>(ptr.get()));
}
template <typename T, typename U>
shared_ptr<T> const_pointer_cast(const shared_ptr<U>& ptr) {
    return shared_ptr<T>(ptr, const_cast<T*>(ptr.get()));
}
template <typename T, typename U>
shared_ptr<T> reinterpret_pointer_cast(const shared_ptr<U>& ptr) {
    return shared_ptr<T>(ptr, reinterpret_cast<T*>(ptr.get()));
}
template <typename T, typename U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& ptr) {
    T* tmp = dynamic_cast<T*>(ptr.get());
    if (tmp != nullptr) return shared_ptr<T>(ptr, tmp);
    return nullptr;
}

template <typename T>
struct hash<shared_ptr<T>> {
    MSTL_CONSTEXPR23 size_t operator ()(const shared_ptr<T>& ptr) const
    noexcept(noexcept(_MSTL declval<_MSTL hash<
        typename shared_ptr<T>::pointer>>()(_MSTL declval<typename shared_ptr<T>::pointer>()))) {
        return hash<T>()(ptr.get());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_MEMORY_HPP__
