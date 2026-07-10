#ifndef NEFORCE_CORE_EXCEPTION_ERROR_CODE_HPP__
#define NEFORCE_CORE_EXCEPTION_ERROR_CODE_HPP__
#include "NeForce/core/exception/error_condition.hpp"
NEFORCE_BEGIN_NAMESPACE__

class error_code : public icommon<error_code> {
private:
    int value_{0};
    const error_category* category_{&system_category()};

public:
    error_code() noexcept = default;

    error_code(int val, const error_category& cat) noexcept :
    value_(val),
    category_(&cat) {}

    error_code(errc e) noexcept { *this = make_error_code(e); }

    void assign(int val, const error_category& cat) noexcept {
        value_ = val;
        category_ = &cat;
    }

    void clear() noexcept {
        value_ = 0;
        category_ = &system_category();
    }

    NEFORCE_NODISCARD int value() const noexcept { return value_; }
    NEFORCE_NODISCARD errc error() const noexcept { return static_cast<errc>(value_); }
    NEFORCE_NODISCARD const error_category& category() const noexcept { return *category_; }

    NEFORCE_NODISCARD error_condition default_error_condition() const noexcept {
        return category_->default_error_condition(value_);
    }

    NEFORCE_NODISCARD string message() const { return category_->message(value_); }

    explicit operator bool() const noexcept { return value_ != 0; }

    NEFORCE_NODISCARD bool equal_to(const error_code& rhs) const noexcept {
        return category_ == rhs.category_ && value_ == rhs.value_;
    }
    NEFORCE_NODISCARD bool less_than(const error_code& rhs) const noexcept {
        if (*category_ < *rhs.category_) {
            return true;
        }
        if (*rhs.category_ < *category_) {
            return false;
        }
        return value_ < rhs.value_;
    }

    NEFORCE_NODISCARD bool operator==(const error_condition& cond) const noexcept {
        return category_->equivalent(value_, cond) || cond.category().equivalent(*this, cond.value());
    }
    NEFORCE_NODISCARD bool operator!=(const error_condition& cond) const noexcept { return !(*this == cond); }

    NEFORCE_NODISCARD size_t to_hash() const noexcept {
        const size_t h1 = hash<const error_category*>{}(category_);
        const size_t h2 = hash<int>{}(value_);
#ifdef NEFORCE_ARCH_BITS_64
        return h1 ^ (h2 << 32 | h2 >> 32);
#else
        return h1 ^ h2;
#endif
    }
};

inline error_code make_error_code(errc e) noexcept { return {static_cast<int>(e), generic_category()}; }

error_code NEFORCE_API last_error() noexcept;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERROR_CODE_HPP__
