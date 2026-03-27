#ifndef NEFORCE_CORE_UTILITY_EXPECTED_HPP__
#define NEFORCE_CORE_UTILITY_EXPECTED_HPP__
#include "NeForce/core/exception/exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_ERROR_BUILD_FINAL_CLASS(expected_exception, memory_exception, "Expected Operation Failed.")

struct inplace_invoke_tag {
    constexpr inplace_invoke_tag() noexcept  = default;
};
struct unexpect_invoke_tag {
    constexpr unexpect_invoke_tag() noexcept  = default;
};

struct unexpect_t {
    explicit unexpect_t() noexcept = default;
};
NEFORCE_INLINE17 constexpr unexpect_t unexpect{};


template <typename T, typename ErrorT, typename = void>
class expected;

template <typename ErrorT>
class unexpected;


template <typename T>
NEFORCE_INLINE17 constexpr bool is_expected = false;
template <typename T, typename ErrorT>
NEFORCE_INLINE17 constexpr bool is_expected<expected<T, ErrorT>> = true;

template <typename T>
NEFORCE_INLINE17 constexpr bool is_unexpected = false;
template <typename T>
NEFORCE_INLINE17 constexpr bool is_unexpected<unexpected<T>> = true;


NEFORCE_BEGIN_INNER__
template <typename Func, typename T>
using expected_invoke_result = remove_cvref_t<invoke_result_t<Func&&, T&&>>;
template <typename Func, typename T>
using expected_transform_result = remove_cv_t<invoke_result_t<Func&&, T&&>>;
template <typename Func>
using expected_invoke_narg_result = remove_cvref_t<invoke_result_t<Func&&>>;
template <typename Func>
using expected_transform_narg_result = remove_cv_t<invoke_result_t<Func&&>>;

template <typename ErrorT>
NEFORCE_INLINE17 constexpr bool can_be_unexpected = is_object_v<ErrorT>
    && !is_array_v<ErrorT> && !is_unexpected<ErrorT>
    && !is_const_v<ErrorT> && !is_volatile_v<ErrorT>;
NEFORCE_END_INNER__


template <typename ErrorT>
class unexpected {
    static_assert(inner::can_be_unexpected<ErrorT>, "ErrorT should be non-array, unexpected, const or volatile type");

private:
    ErrorT error_;

public:
    constexpr unexpected(const unexpected&) = default;
    constexpr unexpected(unexpected&&) = default;

    template <typename Err = ErrorT, typename =
        enable_if_t<!is_same_v<remove_cvref_t<Err>, unexpected>
            && !is_same_v<remove_cvref_t<Err>, inplace_construct_tag>
            && is_constructible_v<ErrorT, Err>>>
    constexpr explicit
    unexpected(Err&& error)
    noexcept(is_nothrow_constructible_v<ErrorT, Err>)
        : error_(_NEFORCE forward<Err>(error)) {}

    template <typename... Args, typename =
        enable_if_t<is_constructible_v<ErrorT, Args...>>>
    constexpr explicit
    unexpected(inplace_construct_tag, Args&&... args)
    noexcept(is_nothrow_constructible_v<ErrorT, Args...>)
        : error_(_NEFORCE forward<Args>(args)...) {}

    template <typename U, typename... Args, typename =
        enable_if_t<is_constructible_v<ErrorT, std::initializer_list<U>&, Args...>>>
    constexpr explicit
    unexpected(inplace_construct_tag, std::initializer_list<U> list, Args&&... args)
    noexcept(is_nothrow_constructible_v<ErrorT, std::initializer_list<U>&, Args...>)
        : error_(list, _NEFORCE forward<Args>(args)...) {}

    constexpr unexpected& operator =(const unexpected&) = default;
    constexpr unexpected& operator =(unexpected&&) = default;

    NEFORCE_NODISCARD constexpr const ErrorT& error() const & noexcept {
        return error_; 
    }

    NEFORCE_NODISCARD constexpr ErrorT& error() & noexcept {
        return error_; 
    }

    NEFORCE_NODISCARD constexpr const ErrorT&& error() const && noexcept {
        return _NEFORCE move(error_);
    }

    NEFORCE_NODISCARD constexpr ErrorT&& error() && noexcept {
        return _NEFORCE move(error_);
    }

    constexpr void swap(unexpected& other) 
    noexcept(is_nothrow_swappable_v<ErrorT>) {
        _NEFORCE swap(error_, other.error_);
    }

    template <typename OtherError>
    NEFORCE_NODISCARD friend constexpr bool
    operator==(const unexpected& lhs, const unexpected<OtherError>& rhs) {
        return lhs.error_ == rhs.error();
    }
};

template <typename ErrorT> 
unexpected(ErrorT) -> unexpected<ErrorT>;


template <typename T>
struct temporary_guard {
    static_assert(is_nothrow_move_constructible_v<T>);

private:
    T* guarded_ptr;
    T temp;

public:
    constexpr explicit temporary_guard(T& value)
    : guarded_ptr(_NEFORCE addressof(value)), temp(_NEFORCE move(value)) {
        _NEFORCE destroy(guarded_ptr);
    }

    NEFORCE_CONSTEXPR20 ~temporary_guard() {
        if (guarded_ptr) NEFORCE_UNLIKELY {
            _NEFORCE construct(guarded_ptr, _NEFORCE move(temp));
        }
    }

    temporary_guard(const temporary_guard&) = delete;
    temporary_guard& operator =(const temporary_guard&) = delete;

    constexpr T&& release() noexcept {
        guarded_ptr = nullptr;
        return _NEFORCE move(temp);
    }
};

template <typename NT, typename OT, typename Arg>
constexpr enable_if_t<is_nothrow_constructible_v<NT, Arg>>
reinitialize(NT* new_val, OT* old_val, Arg&& arg)
noexcept(is_nothrow_constructible_v<NT, Arg>) {
    _NEFORCE destroy(old_val);
    _NEFORCE construct(new_val, _NEFORCE forward<Arg>(arg));
    return;
}

template <typename NT, typename OT, typename Arg>
constexpr enable_if_t<!is_nothrow_constructible_v<NT, Arg> && is_nothrow_move_constructible_v<NT>>
reinitialize(NT* new_val, OT* old_val, Arg&& arg)
noexcept(is_nothrow_constructible_v<NT, Arg>) {
    NT temp(_NEFORCE forward<Arg>(arg));
    _NEFORCE destroy(old_val);
    _NEFORCE construct(new_val, _NEFORCE move(temp));
    return;
}

template <typename NT, typename OT, typename Arg>
constexpr enable_if_t<!is_nothrow_constructible_v<NT, Arg> && !is_nothrow_move_constructible_v<NT>>
reinitialize(NT* new_val, OT* old_val, Arg&& arg)
noexcept(is_nothrow_constructible_v<NT, Arg>) {
    temporary_guard<OT> guard(*old_val);
    _NEFORCE construct(new_val, _NEFORCE forward<Arg>(arg));
    guard.release();
    return;
}

template <typename T, typename ErrorT, typename>
class expected {
    static_assert(!is_reference_v<T>);
    static_assert(!is_function_v<T>);
    static_assert(!is_same_v<remove_cv_t<T>, inplace_construct_tag>);
    static_assert(!is_same_v<remove_cv_t<T>, unexpect_t>);
    static_assert(!is_unexpected<remove_cv_t<T>>);
    static_assert(inner::can_be_unexpected<ErrorT>);

    template <typename U, typename Err, typename UE = unexpected<ErrorT>>
    using constructible_from_expected = disjunction<
        is_constructible<T, expected<U, Err>&>,
        is_constructible<T, expected<U, Err>>,
        is_constructible<T, const expected<U, Err>&>,
        is_constructible<T, const expected<U, Err>>,
        is_convertible<expected<U, Err>&, T>,
        is_convertible<expected<U, Err>, T>,
        is_convertible<const expected<U, Err>&, T>,
        is_convertible<const expected<U, Err>, T>,
        is_constructible<UE, expected<U, Err>&>,
        is_constructible<UE, expected<U, Err>>,
        is_constructible<UE, const expected<U, Err>&>,
        is_constructible<UE, const expected<U, Err>>
    >;

    template <typename U, typename Err>
    static constexpr bool explicit_conversion = disjunction_v<
        negation<is_convertible<U, T>>,
        negation<is_convertible<Err, ErrorT>>
    >;

    template <typename U>
    static constexpr bool same_value = is_same_v<typename U::value_type, T>;

    template <typename U>
    static constexpr bool same_error = is_same_v<typename U::error_type, ErrorT>;

public:
    using value_type = T;
    using error_type = ErrorT;
    using unexpected_type = unexpected<ErrorT>;

    template <typename U>
    using rebind = expected<U, error_type>;

    constexpr expected()
    noexcept(is_nothrow_default_constructible_v<T>)
        : value_(), has_value_(true) {}

    expected(const expected&) = default;

    constexpr expected(const expected& other)
    noexcept(conjunction_v<
        is_nothrow_copy_constructible<T>,
        is_nothrow_copy_constructible<ErrorT>
    >)
    requires is_copy_constructible_v<T> 
        && is_copy_constructible_v<ErrorT>
        && (!is_trivially_copy_constructible_v<T>
            || !is_trivially_copy_constructible_v<ErrorT>)
        : has_value_(other.has_value_) {
        if (has_value_)
            _NEFORCE construct(_NEFORCE addressof(value_), other.value_);
        else
            _NEFORCE construct(_NEFORCE addressof(error_), other.error_);
    }

    expected(expected&&) = default;

    constexpr expected(expected&& other)
    noexcept(conjunction_v<
        is_nothrow_move_constructible<T>,
        is_nothrow_move_constructible<ErrorT>
    >)
    requires (is_move_constructible_v<T>) && (is_move_constructible_v<ErrorT>)
        && (!is_trivially_move_constructible_v<T> || !is_trivially_move_constructible_v<ErrorT>)
        : has_value_(other.has_value_) {
        if (has_value_)
            _NEFORCE construct(_NEFORCE addressof(value_), _NEFORCE move(other).value_);
        else
            _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE move(other).error_);
    }

    template <typename U, typename Gr>
    requires (is_constructible_v<T, const U&>)
        && (is_constructible_v<ErrorT, const Gr&>)
        && (!constructible_from_expected<U, Gr>::value)
    constexpr explicit(explicit_conversion<const U&, const Gr&>)
    expected(const expected<U, Gr>& other)
    noexcept(conjunction_v<
        is_nothrow_constructible<T, const U&>,
        is_nothrow_constructible<ErrorT, const Gr&>
    >)
        : has_value_(other.has_value_) {
        if (has_value_)
            _NEFORCE construct(_NEFORCE addressof(value_), other.value_);
        else
            _NEFORCE construct(_NEFORCE addressof(error_), other.error_);
    }

    template <typename U, typename Gr>
    requires is_constructible_v<T, U>
        && is_constructible_v<ErrorT, Gr>
        && (!constructible_from_expected<U, Gr>::value)
    constexpr explicit(explicit_conversion<U, Gr>)
    expected(expected<U, Gr>&& other)
    noexcept(conjunction_v<
        is_nothrow_constructible<T, U>,
        is_nothrow_constructible<ErrorT, Gr>
    >)
        : has_value_(other.has_value_) {
        if (has_value_)
            _NEFORCE construct(_NEFORCE addressof(value_), _NEFORCE move(other).value_);
        else
            _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE move(other).error_);
    }

    template <typename U = T>
    requires (!is_same_v<remove_cvref_t<U>, expected>)
        && (!is_same_v<remove_cvref_t<U>, inplace_construct_tag>)
        && (!is_unexpected<remove_cvref_t<U>>)
        && is_constructible_v<T, U>
    constexpr explicit(!is_convertible_v<U, T>)
    expected(U&& val)
    noexcept(is_nothrow_constructible_v<T, U>)
        : value_(_NEFORCE forward<U>(val)), has_value_(true) {}

    template <typename Gr = ErrorT>
    requires is_constructible_v<ErrorT, const Gr&>
    constexpr explicit(!is_convertible_v<const Gr&, ErrorT>)
    expected(const unexpected<Gr>& unex)
    noexcept(is_nothrow_constructible_v<ErrorT, const Gr&>)
        : error_(unex.error()), has_value_(false) {}

    template <typename Gr = ErrorT>
    requires is_constructible_v<ErrorT, Gr>
    constexpr explicit(!is_convertible_v<Gr, ErrorT>)
    expected(unexpected<Gr>&& unex)
    noexcept(is_nothrow_constructible_v<ErrorT, Gr>)
        : error_(_NEFORCE move(unex).error()), has_value_(false) {}

    template <typename... Args>
    requires is_constructible_v<T, Args...>
    constexpr explicit
    expected(inplace_construct_tag, Args&&... args)
    noexcept(is_nothrow_constructible_v<T, Args...>)
        : value_(_NEFORCE forward<Args>(args)...), has_value_(true) {}

    template <typename U, typename... Args>
    requires is_constructible_v<T, std::initializer_list<U>&, Args...>
    constexpr explicit
    expected(inplace_construct_tag, std::initializer_list<U> list, Args&&... args)
    noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
        : value_(list, _NEFORCE forward<Args>(args)...), has_value_(true) {}

    template <typename... Args>
    requires is_constructible_v<ErrorT, Args...>
    constexpr explicit
    expected(unexpect_t, Args&&... args)
    noexcept(is_nothrow_constructible_v<ErrorT, Args...>)
        : error_(_NEFORCE forward<Args>(args)...), has_value_(false) {}

    template <typename U, typename... Args>
    requires is_constructible_v<ErrorT, std::initializer_list<U>&, Args...>
    constexpr explicit
    expected(unexpect_t, std::initializer_list<U> list, Args&&... args)
    noexcept(is_nothrow_constructible_v<ErrorT, std::initializer_list<U>&, Args...>)
        : error_(list, _NEFORCE forward<Args>(args)...), has_value_(false) {}

    constexpr ~expected() = default;

    NEFORCE_CONSTEXPR20 ~expected()
    requires (!is_trivially_destructible_v<T>)
        || (!is_trivially_destructible_v<ErrorT>) {
        if (has_value_)
            _NEFORCE destroy(_NEFORCE addressof(value_));
        else
            _NEFORCE destroy(_NEFORCE addressof(error_));
    }

    constexpr expected& operator =(const expected& other)
    noexcept(conjunction_v<
        is_nothrow_copy_constructible<T>,
        is_nothrow_copy_constructible<ErrorT>,
        is_nothrow_copy_assignable<T>,
        is_nothrow_copy_assignable<ErrorT>
    >)
    requires is_copy_assignable_v<T> 
        && is_copy_constructible_v<T>
        && is_copy_assignable_v<ErrorT> 
        && is_copy_constructible_v<ErrorT>
        && (is_nothrow_move_constructible_v<T>
            || is_nothrow_move_constructible_v<ErrorT>) {
        if (other.has_value_)
            assign_value(other.value_);
        else
            assign_error(other.error_);
        return *this;
    }

    constexpr expected& operator =(expected&& other)
    noexcept(conjunction_v<
        is_nothrow_move_constructible<T>,
        is_nothrow_move_constructible<ErrorT>,
        is_nothrow_move_assignable<T>,
        is_nothrow_move_assignable<ErrorT>
    >)
    requires is_move_assignable_v<T> 
        && is_move_constructible_v<T>
        && is_move_assignable_v<ErrorT> 
        && is_move_constructible_v<ErrorT>
        && (is_nothrow_move_constructible_v<T>
            || is_nothrow_move_constructible_v<ErrorT>) {
        if (other.has_value_)
            assign_value(_NEFORCE move(other.value_));
        else
            assign_error(_NEFORCE move(other.error_));
        return *this;
    }

    template <typename U = T>
    requires (!is_same_v<expected, remove_cvref_t<U>>)
        && (!is_unexpected<remove_cvref_t<U>>)
        && is_constructible_v<T, U> 
        && is_assignable_v<T&, U>
        && (is_nothrow_constructible_v<T, U>
            || is_nothrow_move_constructible_v<T>
            || is_nothrow_move_constructible_v<ErrorT>)
    constexpr expected& operator =(U&& val) {
        assign_value(_NEFORCE forward<U>(val));
        return *this;
    }

    template <typename Gr>
    requires is_constructible_v<ErrorT, const Gr&>
        && is_assignable_v<ErrorT&, const Gr&>
        && (is_nothrow_constructible_v<ErrorT, const Gr&>
            || is_nothrow_move_constructible_v<T>
            || is_nothrow_move_constructible_v<ErrorT>)
    constexpr expected& operator =(const unexpected<Gr>& unex) {
        assign_error(unex.error());
        return *this;
    }

    template <typename Gr>
    requires is_constructible_v<ErrorT, Gr>
        && is_assignable_v<ErrorT&, Gr>
        && (is_nothrow_constructible_v<ErrorT, Gr>
            || is_nothrow_move_constructible_v<T>
            || is_nothrow_move_constructible_v<ErrorT>)
    constexpr expected& operator =(unexpected<Gr>&& unex) {
        assign_error(_NEFORCE move(unex).error());
        return *this;
    }

    template <typename... Args>
    requires is_nothrow_constructible_v<T, Args...>
    constexpr T& emplace(Args&&... args) noexcept {
        if (has_value_)
            _NEFORCE destroy(_NEFORCE addressof(value_));
        else {
            _NEFORCE destroy(_NEFORCE addressof(error_));
            has_value_ = true;
        }
        _NEFORCE construct(_NEFORCE addressof(value_), _NEFORCE forward<Args>(args)...);
        return value_;
    }

    template <typename U, typename... Args>
    requires is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>
    constexpr T& emplace(std::initializer_list<U> list, Args&&... args) noexcept {
        if (has_value_)
            _NEFORCE destroy(_NEFORCE addressof(value_));
        else {
            _NEFORCE destroy(_NEFORCE addressof(error_));
            has_value_ = true;
        }
        _NEFORCE construct(_NEFORCE addressof(value_), list, _NEFORCE forward<Args>(args)...);
        return value_;
    }

    constexpr void swap(expected& other)
    noexcept(conjunction_v<
        is_nothrow_move_constructible<T>,
        is_nothrow_move_constructible<ErrorT>,
        is_nothrow_swappable<T&>,
        is_nothrow_swappable<ErrorT&>
    >)
    requires is_swappable_v<T> 
        && is_swappable_v<ErrorT>
        && is_move_constructible_v<T>
        && is_move_constructible_v<ErrorT>
        && (is_nothrow_move_constructible_v<T>
            || is_nothrow_move_constructible_v<ErrorT>) {
        if (has_value_) {
            if (other.has_value_) {
                using _NEFORCE swap;
                swap(value_, other.value_);
            } else {
                swap_value_error(other);
            }
        } else {
            if (other.has_value_)
                other.swap_value_error(*this);
            else {
                using _NEFORCE swap;
                swap(error_, other.error_);
            }
        }
    }

    NEFORCE_NODISCARD constexpr const T* operator->() const noexcept {
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
        return _NEFORCE addressof(value_);
    }

    NEFORCE_NODISCARD constexpr T* operator->() noexcept {
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
        return _NEFORCE addressof(value_);
    }

    NEFORCE_NODISCARD constexpr const T& operator*() const & noexcept {
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
        return value_;
    }

    NEFORCE_NODISCARD constexpr T& operator*() & noexcept {
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
        return value_;
    }

    NEFORCE_NODISCARD constexpr const T&& operator*() const && noexcept {
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
        return _NEFORCE move(value_);
    }

    NEFORCE_NODISCARD constexpr T&& operator*() && noexcept {
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
        return _NEFORCE move(value_);
    }

    NEFORCE_NODISCARD constexpr explicit operator bool() const noexcept {
        return has_value_; 
    }

    NEFORCE_NODISCARD constexpr bool has_value() const noexcept {
        return has_value_; 
    }

    constexpr const T& value() const & {
        if (has_value_) NEFORCE_LIKELY
            return value_;
        NEFORCE_THROW_EXCEPTION(expected_exception(error_));
    }

    constexpr T& value() & {
        if (has_value_) NEFORCE_LIKELY
            return value_;
        NEFORCE_THROW_EXCEPTION(expected_exception(error_));
    }

    constexpr const T&& value() const && {
        if (has_value_) NEFORCE_LIKELY
            return _NEFORCE move(value_);
        NEFORCE_THROW_EXCEPTION(expected_exception(error_));
    }

    constexpr T&& value() && {
        if (has_value_) NEFORCE_LIKELY
            return _NEFORCE move(value_);
        NEFORCE_THROW_EXCEPTION(expected_exception(error_));
    }

    constexpr const ErrorT& error() const & noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return error_;
    }

    constexpr ErrorT& error() & noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return error_;
    }

    constexpr const ErrorT&& error() const && noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return _NEFORCE move(error_);
    }

    constexpr ErrorT&& error() && noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return _NEFORCE move(error_);
    }

    template <typename U>
    constexpr T value_or(U&& alt) const &
    noexcept(conjunction_v<
        is_nothrow_copy_constructible<T>,
        is_nothrow_convertible<U, T>
    >) {
        static_assert(is_copy_constructible_v<T>);
        static_assert(is_convertible_v<U, T>);

        if (has_value_)
            return value_;
        return static_cast<T>(_NEFORCE forward<U>(alt));
    }

    template <typename U>
    constexpr T value_or(U&& alt) &&
    noexcept(conjunction_v<
        is_nothrow_move_constructible<T>,
        is_nothrow_convertible<U, T>
    >) {
        static_assert(is_move_constructible_v<T>);
        static_assert(is_convertible_v<U, T>);

        if (has_value_)
            return _NEFORCE move(value_);
        return static_cast<T>(_NEFORCE forward<U>(alt));
    }

    template <typename Gr = ErrorT>
    constexpr ErrorT error_or(Gr&& alt) const & {
        static_assert(is_copy_constructible_v<ErrorT>);
        static_assert(is_convertible_v<Gr, ErrorT>);

        if (has_value_)
            return _NEFORCE forward<Gr>(alt);
        return error_;
    }

    template <typename Gr = ErrorT>
    constexpr ErrorT error_or(Gr&& alt) && {
        static_assert(is_move_constructible_v<ErrorT>);
        static_assert(is_convertible_v<Gr, ErrorT>);

        if (has_value_)
            return _NEFORCE forward<Gr>(alt);
        return _NEFORCE move(error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT&>
    constexpr auto and_then(Func&& func) & {
        using Res = inner::expected_invoke_result<Func, T&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::error_type, ErrorT>,
            "Func must return an expected with same error type");

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), value_);
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT&>
    constexpr auto and_then(Func&& func) const & {
        using Res = inner::expected_invoke_result<Func, const T&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::error_type, ErrorT>,
            "Func must return an expected with same error type");

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), value_);
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT>
    constexpr auto and_then(Func&& func) && {
        using Res = inner::expected_invoke_result<Func, T&&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::error_type, ErrorT>,
            "Func must return an expected with same error type");

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(value_));
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT>
    constexpr auto and_then(Func&& func) const && {
        using Res = inner::expected_invoke_result<Func, const T&&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::error_type, ErrorT>,
            "Func must return an expected with same error type");

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(value_));
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<T, T&>
    constexpr auto or_else(Func&& func) & {
        using Res = inner::expected_invoke_result<Func, ErrorT&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::value_type, T>,
            "Func must return an expected with same value type");

        if (has_value())
            return Res(inplace_construct_tag{}, value_);
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
    }

    template <typename Func> 
    requires is_constructible_v<T, const T&>
    constexpr auto or_else(Func&& func) const & {
        using Res = inner::expected_invoke_result<Func, const ErrorT&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::value_type, T>,
            "Func must return an expected with same value type");

        if (has_value())
            return Res(inplace_construct_tag{}, value_);
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
    }

    template <typename Func> 
    requires is_constructible_v<T, T>
    constexpr auto or_else(Func&& func) && {
        using Res = inner::expected_invoke_result<Func, ErrorT&&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::value_type, T>,
            "Func must return an expected with same value type");

        if (has_value())
            return Res(inplace_construct_tag{}, _NEFORCE move(value_));
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<T, const T>
    constexpr auto or_else(Func&& func) const && {
        using Res = inner::expected_invoke_result<Func, const ErrorT&&>;
        static_assert(is_expected<Res>,
            "Func must return an expected type");
        static_assert(is_same_v<typename Res::value_type, T>,
            "Func must return an expected with same value type");

        if (has_value())
            return Res(inplace_construct_tag{}, _NEFORCE move(value_));
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT&>
    constexpr auto transform(Func&& func) & {
        using U = inner::expected_transform_result<Func, T&>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), value_);
            });
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT&>
    constexpr auto transform(Func&& func) const & {
        using U = inner::expected_transform_result<Func, const T&>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), value_);
            });
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT>
    constexpr auto transform(Func&& func) && {
        using U = inner::expected_transform_result<Func, T>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(value_));
            });
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT>
    constexpr auto transform(Func&& func) const && {
        using U = inner::expected_transform_result<Func, const T>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(value_));
            });
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<T, T&>
    constexpr auto transform_error(Func&& func) & {
        using Gr = inner::expected_transform_result<Func, ErrorT&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res(inplace_construct_tag{}, value_);
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
            });
    }

    template <typename Func> 
    requires is_constructible_v<T, const T&>
    constexpr auto transform_error(Func&& func) const & {
        using Gr = inner::expected_transform_result<Func, const ErrorT&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res(inplace_construct_tag{}, value_);
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
            });
    }

    template <typename Func> 
    requires is_constructible_v<T, T>
    constexpr auto transform_error(Func&& func) && {
        using Gr = inner::expected_transform_result<Func, ErrorT&&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res(inplace_construct_tag{}, _NEFORCE move(value_));
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
            });
    }

    template <typename Func> 
    requires is_constructible_v<T, const T>
    constexpr auto transform_error(Func&& func) const && {
        using Gr = inner::expected_transform_result<Func, const ErrorT&&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res(inplace_construct_tag{}, _NEFORCE move(value_));
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
            });
    }

    template <typename U, typename Err2>
    requires (!is_void_v<U>)
    friend constexpr bool operator==(
        const expected& lhs, 
        const expected<U, Err2>& rhs) {
        if (lhs.has_value())
            return rhs.has_value() && bool(*lhs == *rhs);
        else
            return !rhs.has_value() && bool(lhs.error() == rhs.error());
    }

    template <typename U>
    friend constexpr bool operator==(const expected& lhs, const U& val) {
        return lhs.has_value() && bool(*lhs == val);
    }

    template <typename Err2>
    friend constexpr bool operator==(
        const expected& lhs, 
        const unexpected<Err2>& unex) {
        return !lhs.has_value() && bool(lhs.error() == unex.error());
    }

    friend constexpr void swap(expected& lhs, expected& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
#ifdef NEFORCE_STANDARD_20
    requires requires { lhs.swap(rhs); }
#endif
    {
        lhs.swap(rhs);
    }

private:
    template <typename, typename, typename> friend class expected;

    template <typename U>
    constexpr void assign_value(U&& val) {
        if (has_value_)
            value_ = _NEFORCE forward<U>(val);
        else {
            reinitialize(_NEFORCE addressof(value_),
                        _NEFORCE addressof(error_),
                        _NEFORCE forward<U>(val));
            has_value_ = true;
        }
    }

    template <typename U>
    constexpr void assign_error(U&& err) {
        if (has_value_) {
            reinitialize(_NEFORCE addressof(error_),
                        _NEFORCE addressof(value_),
                        _NEFORCE forward<U>(err));
            has_value_ = false;
        } else {
            error_ = _NEFORCE forward<U>(err);
        }
    }

    constexpr void swap_value_error(expected& other)
    noexcept(conjunction_v<
        is_nothrow_move_constructible<ErrorT>,
        is_nothrow_move_constructible<T>
    >) {
        if constexpr (is_nothrow_move_constructible_v<ErrorT>) {
            temporary_guard<ErrorT> guard(other.error_);
            _NEFORCE construct(_NEFORCE addressof(other.value_), _NEFORCE move(value_));
            other.has_value_ = true;
            _NEFORCE destroy(_NEFORCE addressof(value_));
            _NEFORCE construct(_NEFORCE addressof(error_), guard.release());
            has_value_ = false;
        } else {
            temporary_guard<T> guard(value_);
            _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE move(other.error_));
            has_value_ = false;
            _NEFORCE destroy(_NEFORCE addressof(other.error_));
            _NEFORCE construct(_NEFORCE addressof(other.value_), guard.release());
            other.has_value_ = true;
        }
    }

    template <typename Func>
    explicit constexpr expected(inplace_invoke_tag, Func&& func)
        : value_(_NEFORCE forward<Func>(func)()), has_value_(true) {}

    template <typename Func>
    explicit constexpr expected(unexpect_invoke_tag, Func&& func)
        : error_(_NEFORCE forward<Func>(func)()), has_value_(false) {}

    union {
        T value_;
        ErrorT error_;
    };

    bool has_value_;
};

template <typename T, typename ErrorT>
class expected<T, ErrorT, enable_if_t<is_void_v<T>>> {
    static_assert(inner::can_be_unexpected<ErrorT>);

    template <typename U, typename Err, typename UE = unexpected<ErrorT>>
    static constexpr bool constructible_from_expected = disjunction_v<
        is_constructible<UE, expected<U, Err>&>,
        is_constructible<UE, expected<U, Err>>,
        is_constructible<UE, const expected<U, Err>&>,
        is_constructible<UE, const expected<U, Err>>
    >;

    template <typename U>
    static constexpr bool same_value = is_same_v<typename U::value_type, T>;

    template <typename U>
    static constexpr bool same_error = is_same_v<typename U::error_type, ErrorT>;

    template <typename, typename, typename> friend class expected;

private:
    union {
        struct {} void_;
        ErrorT error_;
    };

    bool has_value_;

    template <typename U>
    constexpr void assign_error(U&& err) {
        if (has_value_) {
            _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE forward<U>(err));
            has_value_ = false;
        } else {
            error_ = _NEFORCE forward<U>(err);
        }
    }

    template <typename Func>
    explicit constexpr expected(inplace_invoke_tag, Func&& func)
        : void_(), has_value_(true) {
        _NEFORCE forward<Func>(func)();
    }

    template <typename Func>
    explicit constexpr expected(unexpect_invoke_tag, Func&& func)
        : error_(_NEFORCE forward<Func>(func)()), has_value_(false) {}


public:
    using value_type = T;
    using error_type = ErrorT;
    using unexpected_type = unexpected<ErrorT>;

    template <typename U>
    using rebind = expected<U, error_type>;

    constexpr expected() noexcept
        : void_(), has_value_(true) {}

    expected(const expected&) = default;

    constexpr expected(const expected& other)
    noexcept(is_nothrow_copy_constructible_v<ErrorT>)
    requires is_copy_constructible_v<ErrorT>
        && (!is_trivially_copy_constructible_v<ErrorT>)
        : void_(), has_value_(other.has_value_) {
        if (!has_value_)
            _NEFORCE construct(_NEFORCE addressof(error_), other.error_);
    }

    expected(expected&&) = default;

    constexpr expected(expected&& other)
    noexcept(is_nothrow_move_constructible_v<ErrorT>)
    requires is_move_constructible_v<ErrorT>
        && (!is_trivially_move_constructible_v<ErrorT>)
        : void_(), has_value_(other.has_value_) {
        if (!has_value_)
            _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE move(other).error_);
    }

    template <typename U, typename Gr>
    requires is_void_v<U>
        && is_constructible_v<ErrorT, const Gr&>
        && (!constructible_from_expected<U, Gr>)
    constexpr explicit(!is_convertible_v<const Gr&, ErrorT>)
    expected(const expected<U, Gr>& other)
    noexcept(is_nothrow_constructible_v<ErrorT, const Gr&>)
        : void_(), has_value_(other.has_value_) {
        if (!has_value_)
            _NEFORCE construct(_NEFORCE addressof(error_), other.error_);
    }

    template <typename U, typename Gr>
    requires is_void_v<U>
        && is_constructible_v<ErrorT, Gr>
        && (!constructible_from_expected<U, Gr>)
    constexpr explicit(!is_convertible_v<Gr, ErrorT>)
    expected(expected<U, Gr>&& other)
    noexcept(is_nothrow_constructible_v<ErrorT, Gr>)
        : void_(), has_value_(other.has_value_) {
        if (!has_value_)
            _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE move(other).error_);
    }

    template <typename Gr = ErrorT>
    requires is_constructible_v<ErrorT, const Gr&>
    constexpr explicit(!is_convertible_v<const Gr&, ErrorT>)
    expected(const unexpected<Gr>& unex)
    noexcept(is_nothrow_constructible_v<ErrorT, const Gr&>)
        : error_(unex.error()), has_value_(false) {}

    template <typename Gr = ErrorT>
    requires is_constructible_v<ErrorT, Gr>
    constexpr explicit(!is_convertible_v<Gr, ErrorT>)
    expected(unexpected<Gr>&& unex)
    noexcept(is_nothrow_constructible_v<ErrorT, Gr>)
        : error_(_NEFORCE move(unex).error()), has_value_(false) {}

    constexpr explicit expected(inplace_construct_tag) noexcept
        : expected() {}

    template <typename... Args>
    requires is_constructible_v<ErrorT, Args...>
    constexpr explicit
    expected(unexpect_t, Args&&... args)
    noexcept(is_nothrow_constructible_v<ErrorT, Args...>)
        : error_(_NEFORCE forward<Args>(args)...), has_value_(false) {}

    template <typename U, typename... Args>
    requires is_constructible_v<ErrorT, std::initializer_list<U>&, Args...>
    constexpr explicit
    expected(unexpect_t, std::initializer_list<U> list, Args&&... args)
    noexcept(is_nothrow_constructible_v<ErrorT, std::initializer_list<U>&, Args...>)
        : error_(list, _NEFORCE forward<Args>(args)...), has_value_(false) {}

    NEFORCE_CONSTEXPR20 ~expected() = default;

    NEFORCE_CONSTEXPR20 ~expected() requires (!is_trivially_destructible_v<ErrorT>) {
        if (!has_value_)
            _NEFORCE destroy(_NEFORCE addressof(error_));
    }

    constexpr expected& operator =(const expected& other)
    noexcept(conjunction_v<
        is_nothrow_copy_constructible<ErrorT>,
        is_nothrow_copy_assignable<ErrorT>
    >)
    requires is_copy_constructible_v<ErrorT>
        && is_copy_assignable_v<ErrorT> {
        if (other.has_value_)
            emplace();
        else
            assign_error(other.error_);
        return *this;
    }

    constexpr expected& operator =(expected&& other)
    noexcept(conjunction_v<
        is_nothrow_move_constructible<ErrorT>,
        is_nothrow_move_assignable<ErrorT>
    >)
    requires is_move_constructible_v<ErrorT>
        && is_move_assignable_v<ErrorT> {
        if (other.has_value_)
            emplace();
        else
            assign_error(_NEFORCE move(other.error_));
        return *this;
    }

    template <typename Gr>
    requires is_constructible_v<ErrorT, const Gr&>
        && is_assignable_v<ErrorT&, const Gr&>
    constexpr expected& operator =(const unexpected<Gr>& unex) {
        assign_error(unex.error());
        return *this;
    }

    template <typename Gr>
    requires is_constructible_v<ErrorT, Gr>
        && is_assignable_v<ErrorT&, Gr>
    constexpr expected& operator =(unexpected<Gr>&& unex) {
        assign_error(_NEFORCE move(unex.error()));
        return *this;
    }

    constexpr void emplace() noexcept {
        if (!has_value_) {
            _NEFORCE destroy(_NEFORCE addressof(error_));
            has_value_ = true;
        }
    }

    constexpr void swap(expected& other)
    noexcept(conjunction_v<
        is_nothrow_swappable<ErrorT&>,
        is_nothrow_move_constructible<ErrorT>
    >)
    requires is_swappable_v<ErrorT> 
        && is_move_constructible_v<ErrorT> {
        if (has_value_) {
            if (!other.has_value_) {
                _NEFORCE construct(_NEFORCE addressof(error_), _NEFORCE move(other.error_));
                _NEFORCE destroy(_NEFORCE addressof(other.error_));
                has_value_ = false;
                other.has_value_ = true;
            }
        } else {
            if (other.has_value_) {
                _NEFORCE construct(_NEFORCE addressof(other.error_), _NEFORCE move(error_));
                _NEFORCE destroy(_NEFORCE addressof(error_));
                has_value_ = true;
                other.has_value_ = false;
            } else {
                using _NEFORCE swap;
                swap(error_, other.error_);
            }
        }
    }

    NEFORCE_NODISCARD constexpr explicit operator bool() const noexcept {
        return has_value_; 
    }

    NEFORCE_NODISCARD constexpr bool has_value() const noexcept {
        return has_value_; 
    }

    constexpr void operator*() const noexcept { 
        NEFORCE_CONSTEXPR_ASSERT(has_value_);
    }

    constexpr void value() const & {
        if (has_value_) NEFORCE_LIKELY
            return;
        NEFORCE_THROW_EXCEPTION(expected_exception(error_));
    }

    constexpr void value() && {
        if (has_value_) NEFORCE_LIKELY
            return;
        NEFORCE_THROW_EXCEPTION(expected_exception(error_));
    }

    constexpr const ErrorT& error() const & noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return error_;
    }

    constexpr ErrorT& error() & noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return error_;
    }

    constexpr const ErrorT&& error() const && noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return _NEFORCE move(error_);
    }

    constexpr ErrorT&& error() && noexcept {
        NEFORCE_CONSTEXPR_ASSERT(!has_value_);
        return _NEFORCE move(error_);
    }

    template <typename Gr = ErrorT>
    constexpr ErrorT error_or(Gr&& alt) const & {
        static_assert(is_copy_constructible_v<ErrorT>);
        static_assert(is_convertible_v<Gr, ErrorT>);

        if (has_value_)
            return _NEFORCE forward<Gr>(alt);
        return error_;
    }

    template <typename Gr = ErrorT>
    constexpr ErrorT error_or(Gr&& alt) && {
        static_assert(is_move_constructible_v<ErrorT>);
        static_assert(is_convertible_v<Gr, ErrorT>);

        if (has_value_)
            return _NEFORCE forward<Gr>(alt);
        return _NEFORCE move(error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT&>
    constexpr auto and_then(Func&& func) & {
        using Res = inner::expected_invoke_narg_result<Func>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::error_type, ErrorT>);

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func));
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT&>
    constexpr auto and_then(Func&& func) const & {
        using Res = inner::expected_invoke_narg_result<Func>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::error_type, ErrorT>);

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func));
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT>
    constexpr auto and_then(Func&& func) && {
        using Res = inner::expected_invoke_narg_result<Func>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::error_type, ErrorT>);

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func));
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT>
    constexpr auto and_then(Func&& func) const && {
        using Res = inner::expected_invoke_narg_result<Func>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::error_type, ErrorT>);

        if (has_value())
            return _NEFORCE invoke(_NEFORCE forward<Func>(func));
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func>
    constexpr auto or_else(Func&& func) & {
        using Res = inner::expected_invoke_result<Func, ErrorT&>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::value_type, T>);

        if (has_value())
            return Res();
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
    }

    template <typename Func>
    constexpr auto or_else(Func&& func) const & {
        using Res = inner::expected_invoke_result<Func, const ErrorT&>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::value_type, T>);

        if (has_value())
            return Res();
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
    }

    template <typename Func>
    constexpr auto or_else(Func&& func) && {
        using Res = inner::expected_invoke_result<Func, ErrorT&&>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::value_type, T>);

        if (has_value())
            return Res();
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
    }

    template <typename Func>
    constexpr auto or_else(Func&& func) const && {
        using Res = inner::expected_invoke_result<Func, const ErrorT&&>;
        static_assert(is_expected<Res>);
        static_assert(is_same_v<typename Res::value_type, T>);

        if (has_value())
            return Res();
        else
            return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT&>
    constexpr auto transform(Func&& func) & {
        using U = inner::expected_transform_narg_result<Func>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, _NEFORCE forward<Func>(func));
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT&>
    constexpr auto transform(Func&& func) const & {
        using U = inner::expected_transform_narg_result<Func>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, _NEFORCE forward<Func>(func));
        else
            return Res(unexpect, error_);
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, ErrorT>
    constexpr auto transform(Func&& func) && {
        using U = inner::expected_transform_narg_result<Func>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, _NEFORCE forward<Func>(func));
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func> 
    requires is_constructible_v<ErrorT, const ErrorT>
    constexpr auto transform(Func&& func) const && {
        using U = inner::expected_transform_narg_result<Func>;
        using Res = expected<U, ErrorT>;

        if (has_value())
            return Res(inplace_invoke_tag{}, _NEFORCE forward<Func>(func));
        else
            return Res(unexpect, _NEFORCE move(error_));
    }

    template <typename Func>
    constexpr auto transform_error(Func&& func) & {
        using Gr = inner::expected_transform_result<Func, ErrorT&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res();
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
            });
    }

    template <typename Func>
    constexpr auto transform_error(Func&& func) const & {
        using Gr = inner::expected_transform_result<Func, const ErrorT&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res();
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), error_);
            });
    }

    template <typename Func>
    constexpr auto transform_error(Func&& func) && {
        using Gr = inner::expected_transform_result<Func, ErrorT&&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res();
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
            });
    }

    template <typename Func>
    constexpr auto transform_error(Func&& func) const && {
        using Gr = inner::expected_transform_result<Func, const ErrorT&&>;
        using Res = expected<T, Gr>;

        if (has_value())
            return Res();
        else
            return Res(unexpect_invoke_tag{}, [&]() {
                return _NEFORCE invoke(_NEFORCE forward<Func>(func), _NEFORCE move(error_));
            });
    }

    template <typename U, typename Err2>
    requires is_void_v<U>
    friend constexpr bool operator==(
        const expected& lhs, 
        const expected<U, Err2>& rhs) {
        if (lhs.has_value())
            return rhs.has_value();
        else
            return !rhs.has_value() && bool(lhs.error() == rhs.error());
    }

    template <typename Err2>
    friend constexpr bool operator==(
        const expected& lhs, 
        const unexpected<Err2>& unex) {
        return !lhs.has_value() && bool(lhs.error() == unex.error());
    }

    friend constexpr void swap(expected& lhs, expected& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
#ifdef NEFORCE_STANDARD_20
    requires requires { lhs.swap(rhs); }
#endif
    {
        lhs.swap(rhs);
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_EXPECTED_HPP__