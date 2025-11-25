#include <MSTL/core/compound/any.hpp>
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

any& any::operator =(any&& rh) noexcept {
    if (!rh.has_value()) reset();
    else if (this != &rh) {
        reset();
        ArgT arg{};
        arg.any_ptr_ = this;
        rh.manage_(SWAP, &rh, &arg);
    }
    return *this;
}

MSTL_NODISCARD const std::type_info& any::type() const noexcept {
    if (!has_value()) return typeid(void);
    ArgT arg{};
    manage_(GET_TYPE_INFO, this, &arg);
    return *arg.type_ptr_;
}

void any::swap(any& rh) noexcept {
    if (!has_value() && !rh.has_value()) return;
    if (has_value() && rh.has_value()) {
        if (this == &rh) return;
        any tmp;
        ArgT arg{};
        arg.any_ptr_ = &tmp;
        rh.manage_(SWAP, &rh, &arg);
        arg.any_ptr_ = &rh;
        manage_(SWAP, this, &arg);
        arg.any_ptr_ = this;
        tmp.manage_(SWAP, &tmp, &arg);
    }
    else {
        any* emp = !has_value() ? this : &rh;
        any* full = !has_value() ? &rh : this;
        ArgT arg{};
        arg.any_ptr_ = emp;
        full->manage_(SWAP, full, &arg);
    }
}

MSTL_END_NAMESPACE__
