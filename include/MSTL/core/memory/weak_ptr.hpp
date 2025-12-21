#ifndef MSTL_CORE_MEMORY_WEAK_PTR_HPP__
#define MSTL_CORE_MEMORY_WEAK_PTR_HPP__
#include "shared_ptr.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T>
class weak_ptr {
public:
    using element_type = T;

private:
    element_type* ptr_ = nullptr;
    _INNER __smart_ptr_counter* owner_ = nullptr;

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend class shared_ptr;

public:
    weak_ptr(nullptr_t = nullptr) noexcept {}

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const shared_ptr<U>& sp) noexcept
        : ptr_(sp.get()), owner_(reinterpret_cast<_INNER __smart_ptr_counter*>(sp.owner_)) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    weak_ptr(const weak_ptr& wp) noexcept
        : ptr_(wp.ptr_), owner_(wp.owner_) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(const weak_ptr<U>& wp) noexcept
        : ptr_(wp.ptr_), owner_(wp.owner_) {
        if (owner_) {
            owner_->incref_weak();
        }
    }

    weak_ptr(weak_ptr&& wp) noexcept
        : ptr_(wp.ptr_), owner_(wp.owner_) {
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr(weak_ptr<U>&& wp) noexcept
        : ptr_(wp.ptr_), owner_(wp.owner_) {
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
    }

    ~weak_ptr() noexcept {
        reset();
    }

    weak_ptr& operator =(const weak_ptr& wp) noexcept {
        if (_MSTL addressof(wp) == this) return *this;
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        if (owner_) owner_->incref_weak();
        return *this;
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator =(const weak_ptr<U>& wp) noexcept {
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        if (owner_) owner_->incref_weak();
        return *this;
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator =(const shared_ptr<U>& sp) noexcept {
        if (owner_) owner_->decref_weak();
        ptr_ = sp.get();
        owner_ = reinterpret_cast<_INNER __smart_ptr_counter*>(sp.owner_);
        if (owner_) owner_->incref_weak();
        return *this;
    }

    weak_ptr& operator =(weak_ptr&& wp) noexcept {
        if (_MSTL addressof(wp) == this) return *this;
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
        return *this;
    }

    template <typename U, enable_if_t<is_convertible_v<U*, T*>, int> = 0>
    weak_ptr& operator =(weak_ptr<U>&& wp) noexcept {
        if (owner_) owner_->decref_weak();
        ptr_ = wp.ptr_;
        owner_ = wp.owner_;
        wp.ptr_ = nullptr;
        wp.owner_ = nullptr;
        return *this;
    }

    void reset() noexcept {
        if (owner_) {
            owner_->decref_weak();
            owner_ = nullptr;
        }
        ptr_ = nullptr;
    }

    void swap(weak_ptr& wp) noexcept {
        if (_MSTL addressof(wp) == this) return;
        _MSTL swap(ptr_, wp.ptr_);
        _MSTL swap(owner_, wp.owner_);
    }

    MSTL_NODISCARD long use_count() const noexcept {
        return owner_ ? static_cast<long>(owner_->use_count()) : 0;
    }

    MSTL_NODISCARD bool expired() const noexcept {
        return use_count() == 0;
    }

    MSTL_NODISCARD shared_ptr<T> lock() const noexcept {
        if (owner_ && owner_->try_incref_strong()) {
            return shared_ptr<T>(ptr_, owner_);
        }
        return shared_ptr<T>();
    }

    template <typename U>
    MSTL_NODISCARD bool owner_equal(const weak_ptr<U>& rhs) const noexcept {
        return owner_ == rhs.owner_;
    }
    template <typename U>
    MSTL_NODISCARD bool owner_equal(const shared_ptr<U>& rhs) const noexcept {
        return owner_ == reinterpret_cast<_INNER __smart_ptr_counter*>(rhs.owner_);
    }

    template <typename U>
    MSTL_NODISCARD bool owner_before(const weak_ptr<U>& rhs) const noexcept {
        return owner_ < rhs.owner_;
    }
    template <typename U>
    MSTL_NODISCARD bool owner_before(const shared_ptr<U>& rhs) const noexcept {
        return owner_ < reinterpret_cast<_INNER __smart_ptr_counter*>(rhs.owner_);
    }
};

template <typename T>
void swap(weak_ptr<T>& lhs, weak_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}


template <typename T>
struct owner_less;

template <typename T>
struct owner_less<shared_ptr<T>> {
    using is_transparent = void;

    MSTL_NODISCARD bool operator()(const shared_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    MSTL_NODISCARD bool operator()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    MSTL_NODISCARD bool operator()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

template <typename T>
struct owner_less<weak_ptr<T>> {
    using is_transparent = void;

    MSTL_NODISCARD bool operator()(const weak_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    MSTL_NODISCARD bool operator()(const weak_ptr<T>& lhs, const shared_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    MSTL_NODISCARD bool operator()(const shared_ptr<T>& lhs, const weak_ptr<T>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

template <>
struct owner_less<void> {
    using is_transparent = void;

    template <typename T, typename U>
    MSTL_NODISCARD bool operator()(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    template <typename T, typename U>
    MSTL_NODISCARD bool operator()(const shared_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    template <typename T, typename U>
    MSTL_NODISCARD bool operator()(const weak_ptr<T>& lhs, const shared_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }

    template <typename T, typename U>
    MSTL_NODISCARD bool operator()(const weak_ptr<T>& lhs, const weak_ptr<U>& rhs) const noexcept {
        return lhs.owner_before(rhs);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_WEAK_PTR_HPP__
