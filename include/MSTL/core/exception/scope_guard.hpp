#ifndef MSTL_CORE_EXCEPTION_SCOPE_GUARD_HPP__
#define MSTL_CORE_EXCEPTION_SCOPE_GUARD_HPP__
#include "../typeinfo/type_traits.hpp"
#include <exception> // std::uncaught_exceptions()
MSTL_BEGIN_NAMESPACE__

#ifdef __COUNTER__
    #define MSTL_ANONYMOUS_VAR(prefix) prefix ## __COUNTER__
#else
    #define MSTL_ANONYMOUS_VAR(prefix) prefix ## __LINE__
#endif


template <typename F>
class scope_guard {
public:
    scope_guard() = delete;
    scope_guard(const scope_guard&) = delete;
    scope_guard& operator=(const scope_guard&) = delete;

    scope_guard(scope_guard&& other) noexcept
    : func_(_MSTL move(other.func_)), dismissed_(other.dismissed_) {
        other.dismissed_ = true;
    }

    explicit scope_guard(F&& f)
    : func_(_MSTL forward<F>(f)), dismissed_(false) {}

    ~scope_guard() {
        if (!dismissed_) {
            func_();
        }
    }

    void dismiss() noexcept { dismissed_ = true; }

private:
    F func_;
    bool dismissed_;
};

template <typename F>
scope_guard<F> make_scope_guard(F&& f) {
    return scope_guard<F>(_MSTL forward<F>(f));
}


template <typename F>
class scope_guard_success {
public:
    scope_guard_success() = delete;
    scope_guard_success(const scope_guard_success&) = delete;
    scope_guard_success& operator=(const scope_guard_success&) = delete;

    scope_guard_success(scope_guard_success&& other) noexcept
    : func_(_MSTL move(other.func_)), dismissed_(other.dismissed_),
    uncaught_on_entry_(other.uncaught_on_entry_) {
        other.dismissed_ = true;
    }

    explicit scope_guard_success(F&& f)
    : func_(_MSTL forward<F>(f)), dismissed_(false),
    uncaught_on_entry_(std::uncaught_exceptions()) {}

    ~scope_guard_success() {
        if (!dismissed_ && std::uncaught_exceptions() <= uncaught_on_entry_) {
            func_();
        }
    }

    void dismiss() noexcept { dismissed_ = true; }

private:
    F func_;
    bool dismissed_;
    int uncaught_on_entry_;
};

template <typename F>
scope_guard_success<F> make_scope_guard_success(F&& f) {
    return scope_guard_success<F>(_MSTL forward<F>(f));
}


template <typename F>
class scope_guard_failure {
public:
    scope_guard_failure() = delete;
    scope_guard_failure(const scope_guard_failure&) = delete;
    scope_guard_failure& operator=(const scope_guard_failure&) = delete;

    scope_guard_failure(scope_guard_failure&& other) noexcept
        : func_(_MSTL move(other.func_)), dismissed_(other.dismissed_),
          uncaught_on_entry_(other.uncaught_on_entry_) {
        other.dismissed_ = true;
    }

    explicit scope_guard_failure(F&& f)
        : func_(_MSTL forward<F>(f)), dismissed_(false),
          uncaught_on_entry_(std::uncaught_exceptions()) {}

    ~scope_guard_failure() {
        if (!dismissed_ && std::uncaught_exceptions() > uncaught_on_entry_) {
            func_();
        }
    }

    void dismiss() noexcept { dismissed_ = true; }

private:
    F func_;
    bool dismissed_;
    int uncaught_on_entry_;
};

template <typename F>
scope_guard_failure<F> make_scope_guard_failure(F&& f) {
    return scope_guard_failure<F>(_MSTL forward<F>(f));
}


#define scope_guard(...) \
    auto MSTL_ANONYMOUS_VAR(scope_guard_) = make_scope_guard([&]() { __VA_ARGS__; })

#define scope_guard_success(...) \
    auto MSTL_ANONYMOUS_VAR(scope_guard_success_) = make_scope_guard_success([&]() { __VA_ARGS__; })

#define scope_guard_failure(...) \
    auto MSTL_ANONYMOUS_VAR(scope_guard_failure_) = make_scope_guard_failure([&]() { __VA_ARGS__; })

#define defer scope_guard

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_EXCEPTION_SCOPE_GUARD_HPP__
