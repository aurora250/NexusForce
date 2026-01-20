#ifndef MSTL_CORE_MEMORY_SHARED_PTR_HPP__
#define MSTL_CORE_MEMORY_SHARED_PTR_HPP__
#include "../algorithm/compare.hpp"
#include "../async/atomic.hpp"
#include "allocator_traits.hpp"
#include "unique_ptr.hpp"
#include <new>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

struct __smart_ptr_counter {
    _MSTL atomic_ulong strong_count_;
    _MSTL atomic_ulong weak_count_;

    __smart_ptr_counter() noexcept : strong_count_(1), weak_count_(1) {}
    __smart_ptr_counter(__smart_ptr_counter&&) = delete;
    virtual ~__smart_ptr_counter() = default;

    void incref_strong() noexcept {
        strong_count_.fetch_add(1, _MSTL memory_order_relaxed);
    }

    void incref_weak() noexcept {
        weak_count_.fetch_add(1, _MSTL memory_order_relaxed);
    }

    void decref_strong() noexcept {
        if (strong_count_.fetch_sub(1, _MSTL memory_order_acq_rel) == 1) {
            delete_object();
            decref_weak();
        }
    }

    void decref_weak() noexcept {
        if (weak_count_.fetch_sub(1, _MSTL memory_order_acq_rel) == 1) {
            delete_this();
        }
    }

    bool try_incref_strong() noexcept {
        auto strong = strong_count_.load(_MSTL memory_order_relaxed);
        do {
            if (strong == 0) return false;
        } while (!strong_count_.compare_exchange_weak(
            strong, strong + 1,
            _MSTL memory_order_release,
            _MSTL memory_order_relaxed
        ));
        return true;
    }

    MSTL_NODISCARD uint64_t use_count() const noexcept {
        return strong_count_.load(_MSTL memory_order_relaxed);
    }

    MSTL_NODISCARD uint64_t weak_count() const noexcept {
        return weak_count_.load(_MSTL memory_order_relaxed);
    }

protected:
    virtual void delete_object() noexcept = 0;
    virtual void delete_this() noexcept {
        delete this;
    }
};

template <typename T, typename Deleter>
struct __smart_ptr_counter_impl final : __smart_ptr_counter {
    T* ptr_;
    MSTL_NO_UNIQUE_ADDRESS Deleter deleter_;

    explicit __smart_ptr_counter_impl(T* ptr) noexcept : ptr_(ptr) {}

    explicit __smart_ptr_counter_impl(T* ptr, Deleter deleter) noexcept
        : ptr_(ptr), deleter_(_MSTL move(deleter)) {}

    void delete_object() noexcept override {
        deleter_(ptr_);
        ptr_ = nullptr;
    }
};

template <typename T, typename Deleter>
struct __smart_ptr_counter_impl_fused final : __smart_ptr_counter {
    T* ptr_;
    void* mem_;
    MSTL_NO_UNIQUE_ADDRESS Deleter deleter_;

    explicit __smart_ptr_counter_impl_fused(T* ptr, void* mem, Deleter deleter) noexcept
        : ptr_(ptr), mem_(mem), deleter_(_MSTL move(deleter)) {}

    void delete_object() noexcept override {
        deleter_(ptr_);
        ptr_ = nullptr;
    }

    void delete_this() noexcept override {
#if MSTL_STANDARD_17__
        operator delete(mem_, static_cast<std::align_val_t>(
            _MSTL max(alignof(T), alignof(__smart_ptr_counter_impl_fused))));
#else
        operator delete(mem_);
#endif
    }
};

template <typename T, typename Deleter, typename Alloc>
struct __smart_ptr_counter_impl_allocated final : __smart_ptr_counter {
    T* ptr_;
    void* mem_;
    size_t size_;
    MSTL_NO_UNIQUE_ADDRESS Deleter deleter_;
    MSTL_NO_UNIQUE_ADDRESS Alloc allocator_;

    explicit __smart_ptr_counter_impl_allocated(T* ptr, void* mem,
        const size_t size, Deleter deleter, Alloc alloc) noexcept
    : ptr_(ptr), mem_(mem), size_(size),
    deleter_(_MSTL move(deleter)),
    allocator_(_MSTL move(alloc)) {}

    void delete_object() noexcept override {
        deleter_(ptr_);
        ptr_ = nullptr;
    }

    void delete_this() noexcept override {
        using alloc_traits = allocator_traits<Alloc>;
        using byte_allocator = typename alloc_traits::template alloc_rebind_t<Alloc, byte_t>;
        byte_allocator byte_alloc(allocator_);
        allocator_traits<byte_allocator>::deallocate(byte_alloc, static_cast<byte_t*>(mem_), size_);
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
class weak_ptr;


template <typename T>
class shared_ptr {
public:
    using element_type = T;

private:
    element_type* ptr_ = nullptr;
    _INNER __smart_ptr_counter* owner_ = nullptr;

    explicit shared_ptr(T* ptr, _INNER __smart_ptr_counter* owner) noexcept
    : ptr_(ptr), owner_(owner) {}

    template <typename U>
    friend class shared_ptr;

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend shared_ptr<U> _INNER __make_shared_fused(U*, _INNER __smart_ptr_counter*) noexcept;

public:
    shared_ptr(nullptr_t = nullptr) noexcept {}

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr(U* ptr)
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

    shared_ptr(const shared_ptr& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        if (owner_) owner_->incref_strong();
    }
    shared_ptr& operator =(const shared_ptr& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        if (owner_) owner_->decref_strong();
        ptr_ = x.ptr_;
        owner_ = x.owner_;
        if (owner_) owner_->incref_strong();
        return *this;
    }
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr(const shared_ptr<U>& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        if (owner_) owner_->incref_strong();
    }

    shared_ptr(shared_ptr&& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
    }
    shared_ptr& operator =(shared_ptr&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        if (owner_) owner_->decref_strong();
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
        if (owner_) owner_->incref_strong();
    }
    template <typename U>
    shared_ptr(shared_ptr<U>&& x, T* ptr) noexcept : ptr_(ptr), owner_(x.owner_) {
        x.ptr_ = nullptr;
        x.owner_ = nullptr;
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr& operator =(const shared_ptr<U>& x) noexcept {
        if (owner_) owner_->decref_strong();
        ptr_ = x.ptr_;
        owner_ = x.owner_;
        if (owner_) owner_->incref_strong();
        return *this;
    }
    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    shared_ptr& operator =(shared_ptr<U>&& x) noexcept {
        if (owner_) owner_->decref_strong();
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
        if (owner_) owner_->decref_strong();
        owner_ = nullptr;
        ptr_ = nullptr;
    }
    template <typename U>
    void reset(U* ptr) {
        if (owner_) owner_->decref_strong();
        ptr_ = nullptr;
        owner_ = nullptr;
        ptr_ = ptr;
        owner_ = new _INNER __smart_ptr_counter_impl<U, default_delete<U>>(ptr);
        _INNER __setup_enable_shared_from<T>(ptr_, owner_);
    }
    template <typename U, typename Deleter>
    void reset(U* ptr, Deleter deleter) {
        if (owner_) owner_->decref_strong();
        ptr_ = nullptr;
        owner_ = nullptr;
        ptr_ = ptr;
        owner_ = new _INNER __smart_ptr_counter_impl<U, Deleter>(ptr, _MSTL move(deleter));
        _INNER __setup_enable_shared_from<T>(ptr_, owner_);
    }

    MSTL_NODISCARD long use_count() const noexcept {
        return owner_ ? owner_->use_count() : 0;
    }
    MSTL_NODISCARD bool unique() const noexcept {
        return owner_ ? owner_->use_count() == 1 : true;
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
    MSTL_NODISCARD bool owner_equal(const shared_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }
    template <typename U>
    MSTL_NODISCARD bool owner_equal(const weak_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }

    template <typename U>
    MSTL_NODISCARD bool owner_before(const shared_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }
    template <typename U>
    MSTL_NODISCARD bool owner_before(const weak_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }
};
template <typename T, typename U>
MSTL_NODISCARD bool operator ==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.owner_equal(rhs);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator !=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs == rhs);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator <(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return lhs.owner_before(rhs);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator >(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return rhs < lhs;
}
template <typename T, typename U>
MSTL_NODISCARD bool operator <=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs > rhs);
}
template <typename T, typename U>
MSTL_NODISCARD bool operator >=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) noexcept {
    return !(lhs < rhs);
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
        if (!owner_) throw_exception(memory_exception("smart pointer share failed."));
        owner_->incref_strong();
        return _INNER __make_shared_fused(static_cast<T*>(this), owner_);
    }

    shared_ptr<T const> shared_from_this() const {
        static_assert(is_base_of_v<enable_shared_from_this, T>, "shared from T requires derived class");
        if (!owner_) throw_exception(memory_exception("smart pointer share failed."));
        owner_->incref_strong();
        return _INNER __make_shared_fused(static_cast<T const*>(this), owner_);
    }

    template <typename U>
    friend void __set_enable_shared_from(enable_shared_from_this<U>*, _INNER __smart_ptr_counter*);
};


template <typename T, typename... Args, enable_if_t<
    !is_unbounded_array_v<T> && is_constructible_v<T, Args...>, int> = 0
>
shared_ptr<T> make_shared(Args&&... args) {
    auto const deleter = [](T* ptr) noexcept { ptr->~T(); };
    using Counter = _INNER __smart_ptr_counter_impl_fused<T, decltype(deleter)>;
    constexpr size_t align = _MSTL max(alignof(T), alignof(Counter));
    constexpr size_t offset = (sizeof(Counter) + align - 1) & ~(align - 1);
    constexpr size_t size = offset + sizeof(T);
#if MSTL_STANDARD_17__
    void* mem = ::operator new(size, static_cast<std::align_val_t>(align));
    auto* counter = static_cast<Counter*>(mem);
#else
    void* mem = ::operator new(size + align - 1);
    size_t aligned_addr = (reinterpret_cast<size_t>(mem) + (align - 1)) & ~(align - 1);
    Counter* counter = reinterpret_cast<Counter*>(aligned_addr);
#endif
    T* object = reinterpret_cast<T*>(reinterpret_cast<byte_t*>(counter) + offset);
    try {
        _MSTL construct(object, _MSTL forward<Args>(args)...);
    } catch (...) {
#if MSTL_STANDARD_17__
        operator delete(mem, static_cast<std::align_val_t>(align));
#else
        operator delete(mem);
#endif
        throw_exception(memory_exception("shared ptr construction failed."));
    }
    new (counter) Counter(object, mem, deleter);
    _INNER __setup_enable_shared_from(object, counter);
    return _INNER __make_shared_fused(object, counter);
}

template <typename T, typename Alloc, typename... Args, enable_if_t<
    !is_array_v<T> && is_constructible_v<T, Args...>, int> = 0>
shared_ptr<T> allocate_shared(Alloc& alloc, Args&&... args) {
    auto deleter = [](T* p) { p->~T(); };
    using ControlBlock = _INNER __smart_ptr_counter_impl_allocated<T, decltype(deleter), Alloc>;

    const size_t align = _MSTL max(alignof(ControlBlock), alignof(T));
    const size_t offset = (sizeof(ControlBlock) + align - 1) & ~(align - 1);
    const size_t total_size = offset + sizeof(T);
    const size_t raw_size = total_size + align - 1;

    using alloc_traits = allocator_traits<remove_cv_t<Alloc>>;
    using byte_allocator = typename alloc_traits::template alloc_rebind_t<Alloc, byte_t>;
    byte_allocator byte_alloc(alloc);

    byte_t* raw_mem = allocator_traits<byte_allocator>::allocate(byte_alloc, raw_size);

    const uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw_mem);
    const uintptr_t aligned_addr = (raw_addr + align - 1) & ~static_cast<uintptr_t>(align - 1);
    auto aligned_mem = reinterpret_cast<byte_t*>(aligned_addr);
    T* object_ptr = reinterpret_cast<T*>(aligned_mem + offset);

    try {
        allocator_traits<Alloc>::construct(alloc, object_ptr, _MSTL forward<Args>(args)...);
    } catch (...) {
        allocator_traits<byte_allocator>::deallocate(byte_alloc, raw_mem, raw_size);
        throw;
    }

    ControlBlock* ctrl_block = nullptr;
    try {
        ctrl_block = ::new (aligned_mem) ControlBlock(object_ptr, raw_mem, raw_size, deleter, alloc);
    } catch (...) {
        allocator_traits<Alloc>::destroy(alloc, object_ptr);
        allocator_traits<byte_allocator>::deallocate(byte_alloc, raw_mem, raw_size);
        throw;
    }

    _INNER __setup_enable_shared_from(object_ptr, ctrl_block);
    return _INNER __make_shared_fused(object_ptr, ctrl_block);
}

template <typename T, enable_if_t<!is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared_for_overwrite() {
    auto const deleter = [](T* ptr) noexcept { ptr->~T(); };
    using Counter = _INNER __smart_ptr_counter_impl_fused<T, decltype(deleter)>;
    constexpr size_t align = _MSTL max(alignof(T), alignof(Counter));
    constexpr size_t offset = (sizeof(Counter) + align - 1) & ~(align - 1);
    constexpr size_t size = offset + sizeof(T);
#if MSTL_STANDARD_17__
    void* mem = operator new(size, static_cast<std::align_val_t>(align));
    auto* counter = static_cast<Counter*>(mem);
#else
    void* mem = ::operator new(size + align - 1);
    size_t aligned_addr = (reinterpret_cast<size_t>(mem) + (align - 1)) & ~(align - 1);
    Counter* counter = reinterpret_cast<Counter*>(aligned_addr);
#endif
    T* object = reinterpret_cast<T*>(reinterpret_cast<char*>(counter) + offset);
    try{
        _MSTL construct(object);
    } catch (...) {
#if MSTL_STANDARD_17__
        operator delete(mem, static_cast<std::align_val_t>(align));
#else
        operator delete(mem);
#endif
        throw_exception(memory_exception("shared ptr construction failed."));
    }
    new (counter) Counter(object, mem, deleter);
    _INNER __setup_enable_shared_from(object, counter);
    return _INNER __make_shared_fused(object, counter);
}

template <typename T, typename Alloc, enable_if_t<!is_array_v<T>, int> = 0>
shared_ptr<T> allocate_shared_for_overwrite(Alloc& alloc) {
    auto deleter = [](T* p) { p->~T(); };
    using ControlBlock = _INNER __smart_ptr_counter_impl_allocated<T, decltype(deleter), Alloc>;

    const size_t align = _MSTL max(alignof(ControlBlock), alignof(T));
    const size_t offset = (sizeof(ControlBlock) + align - 1) & ~(align - 1);
    const size_t total_size = offset + sizeof(T);
    const size_t raw_size = total_size + align - 1;

    using alloc_traits = allocator_traits<Alloc>;
    using byte_allocator = typename alloc_traits::template alloc_rebind_t<Alloc, byte_t>;
    byte_allocator byte_alloc(alloc);

    byte_t *raw_mem = allocator_traits<byte_allocator>::allocate(byte_alloc, raw_size);

    const uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw_mem);
    const uintptr_t aligned_addr = (raw_addr + align - 1) & ~static_cast<uintptr_t>(align - 1);
    auto aligned_mem = reinterpret_cast<byte_t*>(aligned_addr);
    T* object_ptr = reinterpret_cast<T*>(aligned_mem + offset);

    try {
        ::new (object_ptr) T;
    } catch (...) {
        allocator_traits<byte_allocator>::deallocate(byte_alloc, raw_mem, raw_size);
        throw;
    }

    ControlBlock* ctrl_block = nullptr;
    try {
        ctrl_block = ::new (aligned_mem) ControlBlock(object_ptr, raw_mem, raw_size, deleter, alloc);
    } catch (...) {
        object_ptr->~T();
        allocator_traits<byte_allocator>::deallocate(byte_alloc, raw_mem, raw_size);
        throw;
    }

    _INNER __setup_enable_shared_from(object_ptr, ctrl_block);
    return _INNER __make_shared_fused(object_ptr, ctrl_block);
}

template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared(const size_t len) {
    using value = remove_extent_t<T>;
    auto* tmp = new value[len]();
    try {
        return shared_ptr<T>(tmp);
    } catch (...) {
        delete[] tmp;
        throw_exception(memory_exception("shared ptr construction failed."));
    }
    return nullptr;
}

template <typename T, enable_if_t<is_unbounded_array_v<T>, int> = 0>
shared_ptr<T> make_shared_for_overwrite(const size_t len) {
    using value = remove_extent_t<T>;
    auto* tmp = new value[len];
    try {
        return shared_ptr<T>(tmp);
    } catch (...) {
        delete[] tmp;
        throw_exception(memory_exception("shared ptr construction failed."));
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
    MSTL_CONSTEXPR20 size_t operator ()(const shared_ptr<T>& ptr) const
    noexcept(noexcept(_MSTL declval<_MSTL hash<T*>>()(_MSTL declval<T*>()))) {
        return hash<T*>()(ptr.get());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_SHARED_PTR_HPP__
