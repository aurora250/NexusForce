#ifndef MSTL_CORE_UTILITY_OPTIONAL_HPP__
#define MSTL_CORE_UTILITY_OPTIONAL_HPP__
#include "../exception/exception.hpp"
#include "../interface/icommon.hpp"
#include "../memory/construct.hpp"
#include <initializer_list>
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(optional_exception, memory_exception, "Access the Null Value of Optional.")


struct nullopt_t {
    constexpr nullopt_t() noexcept = default;
};
static constexpr nullopt_t nullopt;


template <typename T>
class optional;

template <typename T>
struct is_optional : false_type {};
template <typename T>
struct is_optional<optional<T>> : true_type {};

template <typename T>
MSTL_INLINE17 bool is_optional_v = is_optional<T>::value;


template <typename T>
class optional : icommon<optional<T>> {
    static_assert(!is_any_of_v<remove_cv_t<T>, nullopt_t, inplace_construct_tag>,
        "optional do not contains nullopt_t and inplace_construct_tag types.");
    static_assert(is_object_v<T> && !is_array_v<T>, "optional only contains non-array object types.");
    static_assert(!is_reference_v<T>, "optional of reference type should use optional<T&> specialization.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using null_type = nullopt_t;

private:
    template <typename U>
    using is_valid_optional = bool_constant<!is_any_of_v<
        remove_cv_t<U>, nullopt_t, inplace_construct_tag> && is_object_v<U> && !is_array_v<U>>;

    template <typename U>
    using convertible_from_optional = disjunction<
        is_constructible<T, const optional<U>&>,
        is_constructible<T, optional<U>&>,
        is_constructible<T, const optional<U>&&>,
        is_constructible<T, optional<U>&&>,
        is_convertible<const optional<U>&, T>,
        is_convertible<optional<U>&, T>,
        is_convertible<const optional<U>&&, T>,
        is_convertible<optional<U>&&, T>>;

    template <typename U>
    using assignable_from_optional = disjunction<
        is_assignable<T&, const optional<U>&>,
        is_assignable<T&, optional<U>&>,
        is_assignable<T&, const optional<U>&&>,
        is_assignable<T&, optional<U>&&>>;

    bool have_value_ = false;
    aligned_storage_t<sizeof(T), alignof(T)> storage_;

    constexpr T* get_ptr() noexcept {
        return reinterpret_cast<T*>(&storage_);
    }
    constexpr const T* get_ptr() const noexcept {
        return reinterpret_cast<const T*>(&storage_);
    }

    template <typename... Args>
    void construct_value(Args&&... args) {
        _MSTL construct(get_ptr(), _MSTL forward<Args>(args)...);
        have_value_ = true;
    }

    void destroy_value() noexcept {
        if (have_value_) {
            _MSTL destroy(get_ptr());
            have_value_ = false;
        }
    }

public:
    constexpr optional() noexcept = default;

    constexpr optional(null_type) noexcept {}

    MSTL_CONSTEXPR20 optional& operator =(null_type) noexcept {
        reset();
        return *this;
    }

    template <typename U, enable_if_t<
        is_valid_optional<U>::value && !is_same_v<remove_cvref_t<U>, optional> &&
            is_constructible_v<T, U> && is_convertible_v<U, T>, int
    > = 0>
    constexpr optional(U&& value) noexcept(is_nothrow_constructible_v<T, U>)
    : have_value_(true) {
        _MSTL construct(get_ptr(), _MSTL forward<U>(value));
    }

    template <typename U, enable_if_t<
        is_valid_optional<U>::value && !is_same_v<remove_cvref_t<U>, optional> &&
            is_constructible_v<T, U> && !is_convertible_v<U, T>, int
    > = 0>
    constexpr optional(U&& value) noexcept(is_nothrow_constructible_v<T, U>)
    : have_value_(true) {
        _MSTL construct(get_ptr(), _MSTL forward<U>(value));
    }

    template <typename U = T, enable_if_t<!is_same_v<remove_cvref_t<U>, optional>
        && negation_v<conjunction<is_scalar<T>, is_same<T, decay_t<U>>>>
        && is_constructible_v<T, U> && is_assignable_v<T&, U>, int> = 0>
    MSTL_CONSTEXPR20 optional& operator =(U&& value)
    noexcept(is_nothrow_constructible_v<T, U> && is_nothrow_assignable_v<T&, U>) {
        if (have_value_) {
            auto temp = T(_MSTL forward<U>(value));
            *get_ptr() = _MSTL move(temp);
        } else {
            _MSTL construct(get_ptr(), _MSTL forward<U>(value));
            have_value_ = true;
        }
        return *this;
    }

    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, const U&> &&
        is_convertible_v<const U&, T> && convertible_from_optional<U>::value, int> = 0>
    constexpr optional(const optional<U>& x) noexcept(is_nothrow_constructible_v<T, const U&>) {
        if (x) {
            _MSTL construct(get_ptr(), *x);
            have_value_ = true;
        }
    }

    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, const U&> &&
        !is_convertible_v<const U&, T> && convertible_from_optional<U>::value, int> = 0>
    constexpr explicit optional(const optional<U>& x) noexcept(is_nothrow_constructible_v<T, const U&>) {
        if (x) {
            _MSTL construct(get_ptr(), *x);
            have_value_ = true;
        }
    }

    template <typename U = T, enable_if_t<!is_same_v<remove_cvref_t<U>, optional>
        && is_constructible_v<T, const U&> && is_assignable_v<T&, const U&>
        && !convertible_from_optional<U>::value && !assignable_from_optional<U>::value, int> = 0>
    MSTL_CONSTEXPR20 optional& operator =(const optional<U>& x)
    noexcept(is_nothrow_constructible_v<T, const U&> && is_nothrow_assignable_v<T&, const U&>) {
        if (_MSTL addressof(x) == this) return *this;
        if (x) {
            if (have_value_) {
                *get_ptr() = *x;
            } else {
                _MSTL construct(get_ptr(), *x);
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    optional(const optional& other) {
        if (other.have_value_) {
            _MSTL construct(get_ptr(), *other);
            have_value_ = true;
        }
    }

    optional& operator =(const optional& other) {
        if (this != &other) {
            if (other.have_value_) {
                if (have_value_) {
                    *get_ptr() = *other;
                } else {
                    _MSTL construct(get_ptr(), *other);
                    have_value_ = true;
                }
            } else {
                reset();
            }
        }
        return *this;
    }

    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, U> &&
        is_convertible_v<U, T> && convertible_from_optional<U>::value, int> = 0>
    constexpr optional(optional<U>&& x) noexcept(is_nothrow_constructible_v<T, U>) {
        if (x) {
            _MSTL construct(get_ptr(), _MSTL move(*x));
            have_value_ = true;
        }
    }

    template <typename U, enable_if_t<!is_same_v<T, U> && is_constructible_v<T, U> &&
        !is_convertible_v<U, T> && convertible_from_optional<U>::value, int> = 0>
    constexpr optional(optional<U>&& x) noexcept(is_nothrow_constructible_v<T, U>) {
        if (x) {
            _MSTL construct(get_ptr(), _MSTL move(*x));
            have_value_ = true;
        }
    }

    template <typename U = T, enable_if_t<!is_same_v<remove_cvref_t<U>, optional>
        && is_constructible_v<T, U> && is_assignable_v<T&, U>
        && !convertible_from_optional<U>::value && !assignable_from_optional<U>::value, int> = 0>
    MSTL_CONSTEXPR20 optional& operator =(optional<U>&& x)
    noexcept(is_nothrow_constructible_v<T, U> && is_nothrow_assignable_v<T&, U>) {
        if (_MSTL addressof(x) == this) return *this;
        if (x) {
            if (have_value_) {
                *get_ptr() = _MSTL move(*x);
            } else {
                _MSTL construct(get_ptr(), _MSTL move(*x));
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    optional(optional&& other) noexcept {
        if (other.have_value_) {
            _MSTL construct(get_ptr(), _MSTL move(*other));
            have_value_ = true;
            other.reset();
        }
    }

    optional& operator =(optional&& other) noexcept {
        if (this != &other) {
            if (other.have_value_) {
                if (have_value_) {
                    *get_ptr() = _MSTL move(*other);
                } else {
                    _MSTL construct(get_ptr(), _MSTL move(*other));
                    have_value_ = true;
                }
                other.reset();
            } else {
                reset();
            }
        }
        return *this;
    }

    template <typename U, enable_if_t<is_constructible_v<T, U&>, int> = 0>
    constexpr optional(const optional<U&>& other) {
        if (other) {
            _MSTL construct(get_ptr(), *other);
            have_value_ = true;
        }
    }

    template <typename U, enable_if_t<is_assignable_v<T&, U&>, int> = 0>
    MSTL_CONSTEXPR20 optional& operator =(const optional<U&>& other) {
        if (other) {
            if (have_value_) {
                *get_ptr() = *other;
            } else {
                _MSTL construct(get_ptr(), *other);
                have_value_ = true;
            }
        } else {
            reset();
        }
        return *this;
    }

    template <typename ...Types, enable_if_t<is_constructible_v<T, Types...>, int> = 0>
    constexpr explicit optional(inplace_construct_tag, Types&&... args)
    noexcept(is_nothrow_constructible_v<T, Types...>)
    : have_value_(true) {
        _MSTL construct(get_ptr(), _MSTL forward<Types>(args)...);
    }

    template <typename U, typename ...Types, enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Types...>, int> = 0>
    constexpr explicit optional(inplace_construct_tag, std::initializer_list<U> ilist, Types &&...args)
    noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Types...>)
    : have_value_(true) {
        _MSTL construct(get_ptr(), ilist, _MSTL forward<Types>(args)...);
    }

    MSTL_CONSTEXPR20 ~optional() noexcept {
        reset();
    }

    template <typename... Types, enable_if_t<is_constructible_v<T, Types...>, int> = 0>
    MSTL_CONSTEXPR20 void emplace(Types&&... args)
    noexcept(is_nothrow_constructible_v<T, Types...>) {
        reset();
        _MSTL construct(get_ptr(), _MSTL forward<Types>(args)...);
        have_value_ = true;
    }

    template <typename U, typename... Types,
        enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Types...>, int> = 0>
    MSTL_CONSTEXPR20 void emplace(std::initializer_list<U> ilist, Types&&... args)
    noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Types...>) {
        reset();
        _MSTL construct(get_ptr(), ilist, _MSTL forward<Types>(args)...);
        have_value_ = true;
    }

    MSTL_CONSTEXPR20 void reset() noexcept {
        destroy_value();
    }

    MSTL_NODISCARD constexpr bool has_value() const noexcept {
        return have_value_;
    }
    constexpr explicit operator bool() const noexcept {
        return have_value_;
    }

    constexpr const_reference value() const & {
        if (!have_value_) {
            throw_exception(optional_exception());
        }
        return *get_ptr();
    }
    constexpr reference value() & {
        if (!have_value_) {
            throw_exception(optional_exception());
        }
        return *get_ptr();
    }
    constexpr const value_type&& value() const && {
        if (!have_value_) {
            throw_exception(optional_exception());
        }
        return _MSTL move(*get_ptr());
    }
    constexpr value_type&& value() && {
        if (!have_value_) {
            throw_exception(optional_exception());
        }
        return _MSTL move(*get_ptr());
    }

    constexpr value_type value_or(value_type value) const & {
        if (!have_value_) {
            return value;
        }
        return *get_ptr();
    }
    constexpr value_type value_or(value_type value) && noexcept {
        if (!have_value_) {
            return value;
        }
        return _MSTL move(*get_ptr());
    }

    template <typename F, enable_if_t<is_invocable_v<F> && is_copy_constructible_v<T>, int> = 0>
    constexpr optional or_else(F&& f) const & {
        if (have_value_) {
            return *this;
        }
        return _MSTL forward<F>(f)();
    }

    template <typename F, enable_if_t<is_invocable_v<F> && is_move_constructible_v<T>, int> = 0>
    constexpr optional or_else(F&& f) && {
        if (have_value_) {
            return _MSTL move(*this);
        }
        return _MSTL forward<F>(f)();
    }

    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const & {
        if (have_value_) {
            return _MSTL forward<F>(f)(*get_ptr());
        }
        return remove_cvref_t<decltype(f(*get_ptr()))>{};
    }
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) & {
        if (have_value_) {
            return _MSTL forward<F>(f)(*get_ptr());
        }
        return remove_cvref_t<decltype(f(*get_ptr()))>{};
    }
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const && {
        if (have_value_) {
            return _MSTL forward<F>(f)(_MSTL move(*get_ptr()));
        }
        return remove_cvref_t<decltype(f(_MSTL move(*get_ptr())))>{};
    }
    template <typename F>
    constexpr decltype(auto) and_then(F&& f) && {
        if (have_value_) {
            return _MSTL forward<F>(f)(_MSTL move(*get_ptr()));
        }
        return remove_cvref_t<decltype(f(_MSTL move(*get_ptr())))>{};
    }

    template <typename F>
    constexpr auto transform(F&& f) const & -> optional<remove_cvref_t<decltype(f(*get_ptr()))>> {
        if (have_value_) {
            return _MSTL forward<F>(f)(*get_ptr());
        }
        return nullopt;
    }
    template <typename F>
    constexpr auto transform(F&& f) & -> optional<remove_cvref_t<decltype(f(*get_ptr()))>> {
        if (have_value_) {
            return _MSTL forward<F>(f)(*get_ptr());
        }
        return nullopt;
    }
    template <typename F>
    constexpr auto transform(F&& f) const && -> optional<remove_cvref_t<decltype(f(_MSTL move(*get_ptr())))>> {
        if (have_value_) {
            return _MSTL forward<F>(f)(_MSTL move(*get_ptr()));
        }
        return nullopt;
    }
    template <typename F>
    constexpr auto transform(F&& f) && -> optional<remove_cvref_t<decltype(f(_MSTL move(*get_ptr())))>> {
        if (have_value_) {
            return _MSTL forward<F>(f)(_MSTL move(*get_ptr()));
        }
        return nullopt;
    }

    constexpr const_pointer operator ->() const noexcept { return get_ptr(); }
    constexpr pointer operator ->() noexcept { return get_ptr(); }

    constexpr const_reference operator *() const & noexcept { return *get_ptr(); }
    constexpr reference operator *() & noexcept { return *get_ptr(); }
    constexpr const value_type&& operator *() const && noexcept { return _MSTL move(*get_ptr()); }
    constexpr value_type&& operator *() && noexcept { return _MSTL move(*get_ptr()); }

    constexpr bool operator ==(const optional& rhs) const noexcept {
        if (have_value_ != rhs.have_value_) {
            return false;
        }
        if (have_value_) {
            return *get_ptr() == *rhs.get_ptr();
        }
        return true;
    }
    constexpr bool operator <(const optional& rhs) const noexcept {
        if (!have_value_ || !rhs.have_value_) {
            return false;
        }
        return *get_ptr() < *rhs.get_ptr();
    }

    constexpr bool operator ==(nullopt_t) const noexcept { return !have_value_; }
    constexpr bool operator !=(nullopt_t) const noexcept { return have_value_; }
    constexpr bool operator >(nullopt_t) const noexcept { return have_value_; }
    constexpr bool operator <(nullopt_t) const noexcept { return false; }
    constexpr bool operator >=(nullopt_t) const noexcept { return true; }
    constexpr bool operator <=(nullopt_t) const noexcept { return !have_value_; }

    friend constexpr bool operator ==(nullopt_t, const optional& rhs) noexcept {
        return !rhs.have_value_;
    }
    friend constexpr bool operator !=(nullopt_t, const optional& rhs) noexcept {
        return rhs.have_value_;
    }
    friend constexpr bool operator >(nullopt_t, const optional&) noexcept {
        return false;
    }
    friend constexpr bool operator <(nullopt_t, const optional& rhs) noexcept {
        return rhs.have_value_;
    }
    friend constexpr bool operator >=(nullopt_t, const optional& rhs) noexcept {
        return !rhs.have_value_;
    }
    friend constexpr bool operator <=(nullopt_t, const optional&) noexcept {
        return true;
    }

    constexpr size_t to_hash() const noexcept {
        return have_value_ ? hash<T>()(*get_ptr()) : 0;
    }

    MSTL_CONSTEXPR20 void swap(optional& x)
    noexcept(is_nothrow_move_constructible_v<T> && is_nothrow_swappable_v<T>) {
        if(_MSTL addressof(x) == this) return;
        if (have_value_ && x.have_value_) {
            _MSTL swap(*this, x);
        } else if (have_value_) {
            x.emplace(move(**this));
            reset();
        } else if (x.have_value_) {
            emplace(move(*x));
            x.reset();
        }
    }
};


template <typename T>
class optional<T&> : icommon<optional<T&>> {
    static_assert(is_object_v<T> && !is_array_v<T>, "optional<T&> requires T to be an object type.");

public:
    using value_type    = T&;
    using reference     = T&;
    using const_reference = const T&;
    using pointer       = T*;
    using const_pointer = const T*;

    using null_type     = nullopt_t;

private:
    T* ptr_ = nullptr;

    template <typename U>
    using convertible_from_optional_ref = disjunction<
        is_convertible<U&, T&>,
        is_convertible<const U&, T&>>;

public:
    constexpr optional(nullopt_t = nullopt) noexcept {}

    constexpr optional(T& value) noexcept : ptr_(_MSTL addressof(value)) {}

    template <typename U, enable_if_t<is_convertible_v<U&, T&>, int> = 0>
    constexpr optional(U& value) noexcept : ptr_(_MSTL addressof(value)) {}

    template <typename U, enable_if_t<!is_convertible_v<U&, T&> && is_constructible_v<T&, U&>, int> = 0>
    constexpr explicit optional(U& value) noexcept : ptr_(_MSTL addressof(value)) {}

    template <typename U, enable_if_t<convertible_from_optional_ref<U>::value, int> = 0>
    constexpr optional(const _MSTL optional<U&>& other) noexcept : ptr_(other.ptr_) {}

    template <typename U, enable_if_t<
        !convertible_from_optional_ref<U>::value && is_constructible_v<T&, U&>, int> = 0>
    constexpr explicit optional(const _MSTL optional<U&>& other) noexcept : ptr_(other.ptr_) {}

    MSTL_CONSTEXPR20 optional& operator=(nullopt_t) noexcept {
        ptr_ = nullptr;
        return *this;
    }

    template <typename U = T, enable_if_t<is_assignable_v<T&, U&>, int> = 0>
    MSTL_CONSTEXPR20 optional& operator=(U& value) {
        if (ptr_) {
            *ptr_ = value;
        } else {
            ptr_ = _MSTL addressof(value);
        }
        return *this;
    }

    template <typename U, enable_if_t<is_assignable_v<T&, U&>, int> = 0>
    MSTL_CONSTEXPR20 optional& operator=(const _MSTL optional<U&>& other) {
        if (this != _MSTL addressof(other)) {
            if (other.ptr_) {
                if (ptr_) {
                    *ptr_ = *other.ptr_;
                } else {
                    ptr_ = other.ptr_;
                }
            } else {
                ptr_ = nullptr;
            }
        }
        return *this;
    }

    constexpr optional(const optional& other) noexcept = default;
    MSTL_CONSTEXPR20 optional& operator=(const optional& other) noexcept = default;

    constexpr optional(optional&& other) noexcept : ptr_(other.ptr_) {}
    MSTL_CONSTEXPR20 optional& operator=(optional&& other) noexcept {
        ptr_ = other.ptr_;
        return *this;
    }

    template <typename... Types>
    constexpr optional(inplace_construct_tag, Types&&...) = delete;

    ~optional() noexcept = default;

    template <typename U, enable_if_t<is_convertible_v<U&, T&>, int> = 0>
    MSTL_CONSTEXPR20 T& emplace(U& value) noexcept {
        ptr_ = _MSTL addressof(value);
        return *ptr_;
    }

    template <typename U, enable_if_t<!is_convertible_v<U&, T&> && is_constructible_v<T&, U&>, int> = 0>
    MSTL_CONSTEXPR20 T& emplace(U& value) noexcept {
        ptr_ = _MSTL addressof(value);
        return *ptr_;
    }

    MSTL_CONSTEXPR20 void reset() noexcept { ptr_ = nullptr; }
    MSTL_NODISCARD constexpr bool has_value() const noexcept { return ptr_ != nullptr; }
    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    constexpr T& value() const & {
        if (!ptr_) throw_exception(optional_exception());
        return *ptr_;
    }
    constexpr T& value() & {
        if (!ptr_) throw_exception(optional_exception());
        return *ptr_;
    }
    constexpr T& value() const && {
        if (!ptr_) throw_exception(optional_exception());
        return *ptr_;
    }
    constexpr T& value() && {
        if (!ptr_) throw_exception(optional_exception());
        return *ptr_;
    }

    template <typename U>
    constexpr T value_or(U&& default_value) const & {
        if (ptr_) return *ptr_;
        return _MSTL forward<U>(default_value);
    }

    template <typename U>
    constexpr T value_or(U&& default_value) && {
        if (ptr_) return _MSTL move(*ptr_);
        return _MSTL forward<U>(default_value);
    }

    template <typename F, enable_if_t<is_invocable_v<F>, int> = 0>
    constexpr optional or_else(F&& f) const & {
        if (ptr_) return *this;
        return _MSTL forward<F>(f)();
    }

    template <typename F, enable_if_t<is_invocable_v<F>, int> = 0>
    constexpr optional or_else(F&& f) && {
        if (ptr_) return _MSTL move(*this);
        return _MSTL forward<F>(f)();
    }

    template <typename F>
    constexpr decltype(auto) and_then(F&& f) const & {
        if (ptr_) return _MSTL forward<F>(f)(*ptr_);
        return remove_cvref_t<decltype(f(*ptr_))>{};
    }

    template <typename F>
    constexpr auto transform(F&& f) const & -> _MSTL optional<remove_cvref_t<decltype(f(*ptr_))>> {
        if (ptr_) return _MSTL forward<F>(f)(*ptr_);
        return nullopt;
    }

    constexpr T* operator ->() const noexcept { return ptr_; }

    constexpr T& operator *() const & noexcept { return *ptr_; }
    constexpr T& operator *() & noexcept { return *ptr_; }
    constexpr T& operator *() const && noexcept { return *ptr_; }
    constexpr T& operator *() && noexcept { return *ptr_; }

    constexpr bool operator ==(const optional& other) const noexcept {
        if (ptr_ == nullptr || other.ptr_ == nullptr)
            return ptr_ == other.ptr_;
        return *ptr_ == *other.ptr_;
    }
    constexpr bool operator <(const optional& other) const noexcept {
        return ptr_ && other.ptr_ && *ptr_ < *other.ptr_;
    }

    template <typename U>
    constexpr bool operator ==(const _MSTL optional<U&>& other) const noexcept {
        if (ptr_ == nullptr || other.ptr_ == nullptr)
            return ptr_ == other.ptr_;
        return *ptr_ == *other.ptr_;
    }

    friend constexpr bool operator ==(nullopt_t, const optional& rhs) noexcept {
        return rhs.ptr_ == nullptr;
    }
    friend constexpr bool operator !=(nullopt_t, const optional& rhs) noexcept {
        return rhs.ptr_ != nullptr;
    }

    constexpr bool operator ==(nullopt_t) const noexcept { return ptr_ == nullptr; }
    constexpr bool operator !=(nullopt_t) const noexcept { return ptr_ != nullptr; }
    constexpr bool operator >(nullopt_t) const noexcept { return ptr_ != nullptr; }
    constexpr bool operator <(nullopt_t) const noexcept { return false; }
    constexpr bool operator >=(nullopt_t) const noexcept { return true; }
    constexpr bool operator <=(nullopt_t) const noexcept { return ptr_ == nullptr; }

    constexpr size_t to_hash() const noexcept {
        return ptr_ ? hash<remove_cv_t<T>>()(*ptr_) : 0;
    }

    MSTL_CONSTEXPR20 void swap(optional& other) noexcept {
        _MSTL swap(ptr_, other.ptr_);
    }
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T>
optional(T) -> optional<T>;
#endif


template <typename T, enable_if_t<is_constructible_v<decay_t<T>, T>, int> = 0>
constexpr optional<decay_t<T>> make_optional(T&& value)
noexcept(is_nothrow_constructible_v<optional<decay_t<T>>, T>) {
    return optional<decay_t<T>>{ _MSTL forward<T>(value) };
}

template <typename T, typename... Args, enable_if_t<is_constructible_v<T, Args...>, int> = 0>
constexpr optional<T> make_optional(Args&&... args)
noexcept(is_nothrow_constructible_v<T, Args...>) {
    return optional<T>{ inplace_construct_tag{}, _MSTL forward<Args>(args)... };
}

template <typename T, typename U, typename... Args>
constexpr enable_if_t<is_constructible_v<T, std::initializer_list<U>&, Args...>,
optional<T>> make_optional(std::initializer_list<U> ilist, Args&&... args)
noexcept(is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>) {
    return optional<T>{ inplace_construct_tag{}, ilist, _MSTL forward<Args>(args)... };
}

template <typename T>
constexpr optional<T&> make_optional(T& value) noexcept {
    return optional<T&>{value};
}

template <typename T>
constexpr optional<remove_reference_t<T>&> make_optional(T&&) = delete;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_OPTIONAL_HPP__
