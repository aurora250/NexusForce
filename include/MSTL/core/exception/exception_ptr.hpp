#ifndef MSTL_CORE_MEMORY_EXCEPTION_PTR_HPP__
#define MSTL_CORE_MEMORY_EXCEPTION_PTR_HPP__
#include "../async/atomic.hpp"
#include "../memory/unique_ptr.hpp"
#include <typeinfo>
MSTL_BEGIN_NAMESPACE__

class exception_ptr;

exception_ptr MSTL_API current_exception() noexcept;

template <typename Ex>
exception_ptr make_exception_ptr(Ex) noexcept;

void MSTL_API rethrow_exception(const exception_ptr &);

MSTL_BEGIN_INNER__
template <typename Ex>
class exception_handler;
MSTL_END_INNER__


class exception_wrapper {
public:
    virtual ~exception_wrapper() = default;
    virtual void rethrow() const = 0;
    virtual const std::type_info& type() const noexcept = 0;
    virtual unique_ptr<exception_wrapper> clone() const = 0;
};

template <typename Ex>
class typed_exception_wrapper final : public exception_wrapper {
    Ex exception_;

public:
    typed_exception_wrapper(const Ex& ex) : exception_(ex) {}
    typed_exception_wrapper(Ex&& ex) : exception_(_MSTL move(ex)) {}

    MSTL_ALWAYS_INLINE void rethrow() const override {
        throw exception_;
    }
    MSTL_ALWAYS_INLINE const std::type_info& type() const noexcept override {
        return typeid(Ex);
    }
    MSTL_ALWAYS_INLINE unique_ptr<exception_wrapper> clone() const override {
        return _MSTL make_unique<typed_exception_wrapper>(exception_);
    }
};


struct exception_control_block {
    unique_ptr<exception_wrapper> wrapper;
    atomic_int ref_count{1};

    explicit exception_control_block(unique_ptr<exception_wrapper> w)
    : wrapper(_MSTL move(w)) {}

    MSTL_ALWAYS_INLINE void add_ref() noexcept {
        ref_count.fetch_add(1, memory_order_relaxed);
    }

    MSTL_ALWAYS_INLINE void release() noexcept {
        if (ref_count.fetch_sub(1, memory_order_acq_rel) == 1) {
            delete this;
        }
    }
};


class exception_ptr {
private:
    exception_control_block* ecb_{nullptr};

    explicit exception_ptr(exception_control_block* cb) noexcept
    : ecb_(cb) {
        if (ecb_) ecb_->add_ref();
    }

    template <typename Ex>
    friend exception_ptr make_exception_ptr(Ex) noexcept;

    friend exception_ptr MSTL_API current_exception() noexcept;

    friend void MSTL_API rethrow_exception(const exception_ptr &);

    template <typename Ex>
    friend class _INNER exception_handler;

public:
    exception_ptr(nullptr_t = nullptr) noexcept {}

    exception_ptr(const exception_ptr& other) noexcept
    : ecb_(other.ecb_) {
        if (ecb_) ecb_->add_ref();
    }

    exception_ptr(exception_ptr&& other) noexcept
    : ecb_(other.ecb_) {
        other.ecb_ = nullptr;
    }

    ~exception_ptr() noexcept {
        if (ecb_) ecb_->release();
    }

    exception_ptr& operator=(const exception_ptr& other) noexcept {
        if (this != &other) {
            exception_ptr temp(other);
            swap(temp);
        }
        return *this;
    }

    exception_ptr& operator=(exception_ptr&& other) noexcept {
        if (this != &other) {
            exception_ptr temp(_MSTL move(other));
            swap(temp);
        }
        return *this;
    }

    void swap(exception_ptr& other) noexcept {
        _MSTL swap(ecb_, other.ecb_);
    }

    explicit operator bool() const noexcept {
        return ecb_ != nullptr;
    }

    friend bool operator==(const exception_ptr& lhs, const exception_ptr& rhs) noexcept {
        return lhs.ecb_ == rhs.ecb_;
    }
    friend bool operator!=(const exception_ptr& lhs, const exception_ptr& rhs) noexcept {
        return !(lhs == rhs);
    }

    friend bool operator==(const exception_ptr& ptr, nullptr_t) noexcept {
        return !ptr;
    }
    friend bool operator==(nullptr_t, const exception_ptr& ptr) noexcept {
        return !ptr;
    }

    friend bool operator!=(const exception_ptr& ptr, nullptr_t) noexcept {
        return static_cast<bool>(ptr);
    }
    friend bool operator!=(nullptr_t, const exception_ptr& ptr) noexcept {
        return static_cast<bool>(ptr);
    }

    const std::type_info* exception_type() const noexcept {
        if (!ecb_ || !ecb_->wrapper) {
            return nullptr;
        }
        return &ecb_->wrapper->type();
    }
};

inline void swap(exception_ptr& lhs, exception_ptr& rhs) noexcept {
    lhs.swap(rhs);
}


MSTL_BEGIN_INNER__

template <typename Ex>
class exception_handler {
public:
    static exception_ptr handle(Ex &&ex) noexcept {
        try {
            auto wrapper = _MSTL make_unique<typed_exception_wrapper<decay_t<Ex>>>(_MSTL forward<Ex>(ex));
            auto control_block = new exception_control_block(_MSTL move(wrapper));
            return exception_ptr(control_block);
        } catch (...) {
            return exception_ptr();
        }
    }
};

MSTL_END_INNER__

template <typename Ex>
exception_ptr make_exception_ptr(Ex ex) noexcept {
    return _INNER exception_handler<Ex>::handle(_MSTL move(ex));
}

MSTL_BEGIN_INNER__

void MSTL_API set_current_exception(exception_ptr ptr) noexcept;

template <typename Ex>
exception_ptr capture_exception(Ex&& ex) noexcept {
    auto ptr = make_exception_ptr(_MSTL forward<Ex>(ex));
    _INNER set_current_exception(ptr);
    return ptr;
}

MSTL_END_INNER__

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_EXCEPTION_PTR_HPP__
