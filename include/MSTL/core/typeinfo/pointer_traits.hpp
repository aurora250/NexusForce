#ifndef MSTL_CORE_UTILITY_POINTER_TRAITS_HPP__
#define MSTL_CORE_UTILITY_POINTER_TRAITS_HPP__
#include "type_traits.hpp"
#include "types.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename>
struct get_first_parameter;

template <template <typename, typename...> class T, typename First, typename... Rest>
struct get_first_parameter<T<First, Rest...>> {
    using type = First;
};

template <typename Ptr>
using get_first_parameter_t = typename get_first_parameter<Ptr>::type;


template <typename, typename = void>
struct get_ptr_difference_type {
    using type = ptrdiff_t;
};

template <typename T>
struct get_ptr_difference_type<T, enable_if_t<is_same_v<typename T::difference_type, typename T::difference_type>>> {
    using type = typename T::difference_type;
};

template <typename T>
using get_ptr_difference_type_t = typename get_ptr_difference_type<T>::type;


template <typename, typename>
struct replace_first_parameter;

template <typename NewFirst, template <typename, typename...> class T, typename First, typename... Rest>
struct replace_first_parameter<NewFirst, T<First, Rest...>> {
    using type = T<NewFirst, Rest...>;
};

template <typename T, typename U>
using replace_first_parameter_t = typename replace_first_parameter<T, U>::type;


template <typename T, typename U, typename = void>
struct get_rebind_type {
    using type = replace_first_parameter_t<U, T>;
};

template <typename T, typename U>
struct get_rebind_type<T, U, enable_if_t<is_same_v<typename T::template rebind<U>, typename T::template rebind<U>>>> {
    using type = typename T::template rebind<U>;
};

template <typename T, typename U>
using get_rebind_type_t = typename get_rebind_type<T, U>::type;


MSTL_BEGIN_INNER__

template <typename Ptr, typename Elem>
struct __ptr_traits_base {
    using pointer = Ptr;
    using element_type = Elem;
    using difference_type = get_ptr_difference_type_t<Ptr>;
    using reference = conditional_t<is_void_v<Elem>, char, Elem>&;

    template <typename U>
    using rebind = typename get_rebind_type<Ptr, U>::type;

    MSTL_NODISCARD static constexpr pointer pointer_to(reference x)
        noexcept(noexcept(Ptr::pointer_to(x))) {
        return Ptr::pointer_to(x);
    }
};

template <typename, typename = void, typename = void>
struct __ptr_traits_extract {};

template <typename T, typename U>
struct __ptr_traits_extract<T, U, void_t<get_first_parameter_t<T>>>
    : __ptr_traits_base<T, typename get_first_parameter<T>::type> {
};

template <typename T>
struct __ptr_traits_extract<T, void_t<typename T::element_type>, void>
    : __ptr_traits_base<T, typename T::element_type> {
};

MSTL_END_INNER__

template <typename T>
struct pointer_traits : _INNER __ptr_traits_extract<T> {};

template <typename T>
struct pointer_traits<T*> {
    using pointer = T*;
    using element_type = T;
    using difference_type = ptrdiff_t;
    using reference = conditional_t<is_void_v<T>, char, T>&;

    template <typename U>
    using rebind = U*;

    MSTL_NODISCARD static constexpr pointer pointer_to(reference x) noexcept {
        return _MSTL addressof(x);
    }
};

template <typename Ptr, typename T>
using pointer_rebind = typename pointer_traits<Ptr>::template rebind<T>;

template <typename Ptr>
constexpr decltype(auto) ptr_const_cast(Ptr ptr) noexcept {
    using T = typename pointer_traits<Ptr>::element_type;
    using NonConst = remove_const_t<T>;
    using Dest = typename pointer_traits<Ptr>::template rebind<NonConst>;

    return pointer_traits<Dest>::pointer_to(const_cast<NonConst&>(*ptr));
}
template <typename T>
constexpr decltype(auto) ptr_const_cast(T* ptr) noexcept {
    return const_cast<remove_const_t<T>*>(ptr);
}


MSTL_BEGIN_INNER__

template <typename T>
constexpr T* __to_address(T* ptr) noexcept {
    static_assert(!is_function_v<T>, "not a function pointer");
    return ptr;
}

template <typename Ptr>
constexpr decltype(auto) __to_address(const Ptr& ptr) noexcept {
    return pointer_traits<Ptr>::to_address(ptr);
}

template <typename Ptr, typename... None, enable_if_t<!has_base_v<Ptr>, int> = 0>
constexpr decltype(auto) __to_address(const Ptr& ptr, None...) noexcept {
    return __to_address(ptr.operator->());
}

template <typename Ptr, typename... None, enable_if_t<has_base_v<Ptr>, int> = 0>
constexpr decltype(auto) __to_address(const Ptr& ptr, None...) noexcept {
    return __to_address(ptr.base().operator->());
}

MSTL_END_INNER__


template <typename T>
constexpr T* to_address(T* ptr) noexcept {
    return _INNER __to_address(ptr);
}

template <typename Ptr>
constexpr decltype(auto) to_address(const Ptr& ptr) noexcept {
    return _INNER __to_address(ptr);
}


template <typename T, typename = void>
struct get_pointer_type {
    using type = typename T::value_type*;
};
template <typename T>
struct get_pointer_type<T, void_t<typename T::pointer>> {
    using type = typename T::pointer;
};

template <typename T, typename = void>
struct get_difference_type {
    using pointer = typename get_pointer_type<T>::type;
    using type = typename pointer_traits<pointer>::difference_type;
};
template <typename T>
struct get_difference_type<T, void_t<typename T::difference_type>> {
    using type = typename T::difference_type;
};

template <typename T, typename = void>
struct get_size_type {
    using type = make_unsigned_t<typename get_difference_type<T>::type>;
};
template <typename T>
struct get_size_type<T, void_t<typename T::size_type>> {
    using type = typename T::size_type;
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_POINTER_TRAITS_HPP__
