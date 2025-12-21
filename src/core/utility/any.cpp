#include <MSTL/core/utility/any.hpp>
MSTL_BEGIN_NAMESPACE__

any::any(const any& x) {
    if (!x.has_value()) manage_ = nullptr;
    else {
        ArgT arg{};
        arg.any_ptr_ = this;
        x.manage_(COPY, &x, &arg);
    }
}

any::any(any&& x) noexcept {
    if (!x.has_value()) manage_ = nullptr;
    else {
        ArgT arg{};
        arg.any_ptr_ = this;
        x.manage_(SWAP, &x, &arg);
    }
}

any& any::operator =(any&& rhs) noexcept {
    if (!rhs.has_value()) reset();
    else if (this != &rhs) {
        reset();
        ArgT arg{};
        arg.any_ptr_ = this;
        rhs.manage_(SWAP, &rhs, &arg);
    }
    return *this;
}

MSTL_NODISCARD const std::type_info& any::type() const noexcept {
    if (!has_value()) return typeid(void);
    ArgT arg{};
    manage_(GET_TYPE_INFO, this, &arg);
    return *arg.type_ptr_;
}

void any::swap(any& rhs) noexcept {
    if (!has_value() && !rhs.has_value()) return;
    if (has_value() && rhs.has_value()) {
        if (this == &rhs) return;
        any tmp;
        ArgT arg{};
        arg.any_ptr_ = &tmp;
        rhs.manage_(SWAP, &rhs, &arg);
        arg.any_ptr_ = &rhs;
        manage_(SWAP, this, &arg);
        arg.any_ptr_ = this;
        tmp.manage_(SWAP, &tmp, &arg);
    }
    else {
        any* emp = !has_value() ? this : &rhs;
        any* full = !has_value() ? &rhs : this;
        ArgT arg{};
        arg.any_ptr_ = emp;
        full->manage_(SWAP, full, &arg);
    }
}

MSTL_END_NAMESPACE__
