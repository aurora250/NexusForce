#ifndef MSTL_CORE_MEMORY_SHARED_PTR_HPP__
#define MSTL_CORE_MEMORY_SHARED_PTR_HPP__
#include "../algorithm/algobase.hpp"
#include "unique_ptr.hpp"
#include "construct.hpp"
#include <atomic> // std::atomic_ulong
MSTL_BEGIN_NAMESPACE__

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
#if MSTL_STANDARD_17__
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
    shared_ptr(const shared_ptr<U>& x) noexcept : ptr_(x.ptr_), owner_(x.owner_) {
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
    }
    catch (...) {
#if MSTL_STANDARD_17__
        operator delete(mem, static_cast<std::align_val_t>(align));
#else
        operator delete(mem);
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
    }
    catch (...) {
#if MSTL_STANDARD_17__
        operator delete(mem, static_cast<std::align_val_t>(align));
#else
        operator delete(mem);
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
    MSTL_CONSTEXPR20 size_t operator ()(const shared_ptr<T>& ptr) const
    noexcept(noexcept(_MSTL declval<_MSTL hash<
        typename shared_ptr<T>::pointer>>()(_MSTL declval<typename shared_ptr<T>::pointer>()))) {
        return hash<T>()(ptr.get());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_SHARED_PTR_HPP__
