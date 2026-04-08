#ifndef NEFORCE_CORE_EXCEPTION_ERROR_CONDITION_HPP__
#define NEFORCE_CORE_EXCEPTION_ERROR_CONDITION_HPP__
#include "NeForce/core/exception/error_category.hpp"
NEFORCE_BEGIN_NAMESPACE__

class error_condition : icomparable<error_condition> {
private:
    int value_;
    const error_category* category_;

public:
    error_condition() noexcept
    : value_(0), category_(&generic_category()) {}

    error_condition(int val, const error_category& cat) noexcept
    : value_(val), category_(&cat) {}

    error_condition(errc e) noexcept {
        *this = make_error_condition(e);
    }

    void assign(int val, const error_category& cat) noexcept {
        value_ = val;
        category_ = &cat;
    }

    void clear() noexcept {
        value_ = 0;
        category_ = &generic_category();
    }

    NEFORCE_NODISCARD int value() const noexcept { return value_; }
    NEFORCE_NODISCARD const error_category& category() const noexcept { return *category_; }
    NEFORCE_NODISCARD string message() const { return category_->message(value_); }

    explicit operator bool() const noexcept { return value_ != 0; }

    NEFORCE_NODISCARD bool operator==(const error_condition& rhs) const noexcept {
        return category_ == rhs.category_ && value_ == rhs.value_;
    }
    NEFORCE_NODISCARD bool operator<(const error_condition& rhs) const noexcept {
        if (*category_ < *rhs.category_) return true;
        if (*rhs.category_ < *category_) return false;
        return value_ < rhs.value_;
    }
};


inline error_condition make_error_condition(errc e) noexcept {
    return {static_cast<int>(e), generic_category()};
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERROR_CONDITION_HPP__
