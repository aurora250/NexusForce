#ifndef NEFORCE_CORE_EXCEPTION_ERROR_CATEGORY_HPP__
#define NEFORCE_CORE_EXCEPTION_ERROR_CATEGORY_HPP__
#include "NeForce/core/exception/errc.hpp"
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

class error_code;
class error_condition;

class error_category : public icomparable<error_category> {
public:
    error_category() noexcept = default;
    virtual ~error_category() noexcept = default;

    error_category(const error_category&) = delete;
    error_category& operator=(const error_category&) = delete;

    NEFORCE_NODISCARD virtual const char* name() const noexcept = 0;
    NEFORCE_NODISCARD virtual string message(int32_t ev) const = 0;

    NEFORCE_NODISCARD virtual error_condition default_error_condition(int32_t ev) const noexcept;

    NEFORCE_NODISCARD virtual bool equivalent(int code, const error_condition& condition) const noexcept;
    NEFORCE_NODISCARD virtual bool equivalent(const error_code& code, int condition) const noexcept;

    NEFORCE_NODISCARD bool equal_to(const error_category& rhs) const noexcept { return this == &rhs; }
    NEFORCE_NODISCARD bool less_than(const error_category& rhs) const noexcept {
        return less<const error_category*>()(this, &rhs);
    }
};


class NEFORCE_API generic_error_category final : public error_category {
public:
    NEFORCE_NODISCARD const char* name() const noexcept override { return "generic"; }

    NEFORCE_NODISCARD string message(int32_t ev) const override;

    NEFORCE_NODISCARD error_condition default_error_condition(int32_t ev) const noexcept override;
};

NEFORCE_API const error_category& generic_category() noexcept;

inline error_code make_error_code(errc e) noexcept;
inline error_condition make_error_condition(errc e) noexcept;


class NEFORCE_API system_error_category final : public error_category {
public:
    NEFORCE_NODISCARD const char* name() const noexcept override { return "system"; }

    NEFORCE_NODISCARD string message(int32_t ev) const override;

    NEFORCE_NODISCARD error_condition default_error_condition(int32_t ev) const noexcept override;
};

NEFORCE_API const error_category& system_category() noexcept;

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_EXCEPTION_ERROR_CATEGORY_HPP__
