#ifndef MSTL_CORE_UTILITY_REFERENCE_WRAPPER_HPP__
#define MSTL_CORE_UTILITY_REFERENCE_WRAPPER_HPP__
#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__
template <typename T>
void __ref_wrapper_construct_aux(type_identity_t<T&>) noexcept;
template <typename T>
void __ref_wrapper_construct_aux(type_identity_t<T&&>) = delete;
MSTL_END_INNER__

template <typename, typename, typename = void>
struct ref_wrapper_constructable_from : false_type {};

template <typename T, typename U>
struct ref_wrapper_constructable_from<T, U,
    void_t<decltype(_INNER __ref_wrapper_construct_aux<T>(_MSTL declval<U>()))>> : true_type {};

#ifdef MSTL_STANDARD_14__
template <typename T, typename U>
MSTL_INLINE17 constexpr bool ref_wrapper_constructable_from_v = ref_wrapper_constructable_from<T, U>::value;
#endif


MSTL_BEGIN_INNER__
template <typename F, typename... Args>
struct __invoke_result_aux;
MSTL_END_INNER__

template <typename F, typename... Args>
struct is_nothrow_invocable;

template <typename Callable, typename... Args>
MSTL_CONSTEXPR14 typename _INNER __invoke_result_aux<Callable, Args...>::type
invoke(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value);


template <typename T>
class reference_wrapper {
public:
    static_assert(is_object<T>::value || is_function<T>::value,
        "reference_wrapper requires an object or function type.");

    using type = T;

private:
    T* ptr_{};

public:
    template <typename U, enable_if_t<
        conjunction<negation<is_same<remove_cvref_t<U>, reference_wrapper>>,
            ref_wrapper_constructable_from<T, U>>::value, int> = 0>
    MSTL_CONSTEXPR14 reference_wrapper(U&& x)
        noexcept(noexcept(_INNER __ref_wrapper_construct_aux<T>(_MSTL declval<U>()))) {
        T& ref = static_cast<U&&>(x);
        ptr_ = _MSTL addressof(ref);
    }
    MSTL_CONSTEXPR14 operator T &() const noexcept {
        return *ptr_;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR14 T& get() const noexcept {
        return *ptr_;
    }
    template <typename... Args>
    MSTL_CONSTEXPR14 typename _INNER __invoke_result_aux<T&, Args...>::type
    operator()(Args&&... args) const noexcept(is_nothrow_invocable<T&, Args...>::value) {
        return invoke(get(), _MSTL forward<Args>(args)...);
    }
};

#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T>
reference_wrapper(T&) -> reference_wrapper<T>;
#endif


template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<T> ref(T& val) noexcept {
    return reference_wrapper<T>(val);
}
template <typename T>
void ref(const T&&) = delete;

template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<T> ref(reference_wrapper<T> wrapper) noexcept {
    return wrapper;
}

template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<const T> cref(const T& val) noexcept {
    return reference_wrapper<const T>(val);
}
template <typename T>
void cref(const T&&) = delete;

template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<const T> cref(reference_wrapper<T> wrapper) noexcept {
    return wrapper;
}


template <typename T>
struct unwrap_reference {
    using type = T;
};
template <typename T>
struct unwrap_reference<reference_wrapper<T>> {
    using type = T&;
};
template <typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;


template <typename T>
using unwrap_ref_decay_t = unwrap_reference_t<decay_t<T>>;
template <typename T>
struct unwrap_ref_decay {
    using type = unwrap_ref_decay_t<T>;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_REFERENCE_WRAPPER_HPP__
