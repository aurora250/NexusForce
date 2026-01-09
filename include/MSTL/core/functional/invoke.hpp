#ifndef MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
#define MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
#include "../utility/reference_wrapper.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_TAG__
struct invoke_memfun_ref_tag {
    constexpr invoke_memfun_ref_tag() noexcept = default;
};
struct invoke_memfun_deref_tag {
    constexpr invoke_memfun_deref_tag() noexcept = default;
};
struct invoke_memobj_ref_tag {
    constexpr invoke_memobj_ref_tag() noexcept = default;
};
struct invoke_memobj_deref_tag {
    constexpr invoke_memobj_deref_tag() noexcept = default;
};
struct invoke_other_tag {
    constexpr invoke_other_tag() noexcept = default;
};
MSTL_END_TAG__


template <typename Sign>
struct invoke_result;

template <typename T, typename Tag>
struct invoke_result_true {
    using invoke_type   = Tag;
    using type          = T;
};

struct invoke_result_false {};


MSTL_BEGIN_INNER__
template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_memfun_ref {
private:
    template <typename F, typename T, typename... Args1>
    static invoke_result_true<decltype((_MSTL declval<T>().*_MSTL declval<F>())(_MSTL declval<Args1>()...)),
        _MSTL_TAG invoke_memfun_ref_tag> __test(int);

    template <typename...>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg, Args...>(0));
};

template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_memfun_deref {
private:
    template <typename F, typename T, typename... Args1>
    static invoke_result_true<decltype((*_MSTL declval<T>().*_MSTL declval<F>())(_MSTL declval<Args1>()...)),
        _MSTL_TAG invoke_memfun_deref_tag> __test(int);

    template <typename...>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg, Args...>(0));
};

template <typename MemPtr, typename Arg>
struct __invoke_result_memobj_ref {
private:
    template <typename F, typename T>
    static invoke_result_true<decltype(_MSTL declval<T>().*_MSTL declval<F>()),
        _MSTL_TAG invoke_memobj_ref_tag> __test(int);

    template <typename, typename>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg>(0));
};

template <typename MemPtr, typename Arg>
struct __invoke_result_memobj_deref {
private:
    template <typename F, typename T>
    static invoke_result_true<decltype(*_MSTL declval<T>().*_MSTL declval<F>()),
        _MSTL_TAG invoke_memobj_deref_tag> __test(int);

    template <typename, typename>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg>(0));
};

template <typename MemPtr, typename Arg>
struct __invoke_result_memobj;

template <typename Res, typename Class, typename Arg>
struct __invoke_result_memobj<Res Class::*, Arg> {
    using Argval = remove_cvref_t<Arg>;
    using MemPtr = Res Class::*;
    using type = typename conditional_t<disjunction<
        is_same<Argval, Class>, is_base_of<Class, Argval>>::value,
        __invoke_result_memobj_ref<MemPtr, Arg>,
        __invoke_result_memobj_deref<MemPtr, Arg>>::type;
};

template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_memfun;

template <typename Res, typename Class, typename Arg, typename... Args>
struct __invoke_result_memfun<Res Class::*, Arg, Args...> {
    using MemPtr = Res Class::*;
    using type = typename conditional_t<is_base_of<Class, remove_reference_t<Arg>>::value,
        __invoke_result_memfun_ref<MemPtr, Arg, Args...>,
        __invoke_result_memfun_deref<MemPtr, Arg, Args...>>::type;
};

template <bool, bool, typename F, typename... Args>
struct __invoke_result_dispatch {
    using type = invoke_result_false;
};

template <typename MemPtr, typename Arg>
struct __invoke_result_dispatch<true, false, MemPtr, Arg>
    : __invoke_result_memobj<decay_t<MemPtr>, unwrap_reference_t<Arg>> {};

template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_dispatch<false, true, MemPtr, Arg, Args...>
    : __invoke_result_memfun<decay_t<MemPtr>, unwrap_reference_t<Arg>, Args...> {};

template <typename F, typename... Args>
struct __invoke_result_dispatch<false, false, F, Args...> {
private:
    template<typename F1, typename... Args1>
    static invoke_result_true<
        decltype(_MSTL declval<F1>()(_MSTL declval<Args1>()...)), _MSTL_TAG invoke_other_tag> __test(int);

    template <typename...>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<F, Args...>(0));
};

template <typename F, typename... Args>
struct __invoke_result_aux : __invoke_result_dispatch<
    is_member_object_pointer<remove_reference_t<F>>::value,
    is_member_function_pointer<remove_reference_t<F>>::value,
    F, Args...>::type {};

MSTL_END_INNER__

template <typename F, typename... Args>
struct invoke_result<F(Args...)> : _INNER __invoke_result_aux<F, Args...> {};

template <typename F, typename... Args>
using invoke_result_t = typename _INNER __invoke_result_aux<F, Args...>::type;


template <typename T>
struct is_location_invariant : is_trivially_copyable<T>::type {};

#ifdef MSTL_STANDARD_14__
template <typename T>
MSTL_INLINE17 constexpr bool is_location_invariant_v = is_location_invariant<T>::value;
#endif


MSTL_BEGIN_INNER__

template <typename Result, typename Ret, bool = is_void<Ret>::value, typename = void>
struct __is_invocable_aux : false_type {};

template <typename Result, typename Ret>
struct __is_invocable_aux<Result, Ret, true, void_t<typename Result::type>> : true_type {};

#ifdef MSTL_COMPILER_GCC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#elif defined(MSTL_COMPILER_CLANG__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wctor-dtor-privacy"
#elif defined(MSTL_COMPILER_MSVC__)
#pragma warning(push)
#pragma warning(disable : 4624)
#endif

template <typename Result, typename Ret>
struct __is_invocable_aux<Result, Ret, false, void_t<typename Result::type>> {
private:
    using Res_t = typename Result::type;

    template <typename T, bool Nothrow = noexcept(_MSTL declvoid<T>(_MSTL declval<Res_t>())),
        typename = decltype(_MSTL declvoid<T>(_MSTL declval<Res_t>())), bool Dangle =
#if defined(MSTL_COMPILER_GCC__) && defined(MSTL_PLATFORM_WINDOWS__)
        __reference_converts_from_temporary(T, Res_t)
#else
        false
#endif
    >
    static bool_constant<Nothrow && !Dangle> __test(int);

    template <typename T, bool = false>
    static false_type __test(...);

public:
    using type = decltype(__test<Ret, true>(1));
};

#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic pop
#elif defined(MSTL_COMPILER_GCC__)
#pragma GCC diagnostic pop
#elif defined(MSTL_COMPILER_MSVC__)
#pragma warning(pop)
#endif

MSTL_END_INNER__


template <typename F, typename... Args>
struct is_invocable : _INNER __is_invocable_aux<_INNER __invoke_result_aux<F, Args...>, void>::type {};

#ifdef MSTL_STANDARD_14__
template <typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_invocable_v = is_invocable<F, Args...>::value;
#endif


template <typename Ret, typename F, typename... Args>
struct is_invocable_r : _INNER __is_invocable_aux<_INNER __invoke_result_aux<F, Args...>, Ret>::type {};

#ifdef MSTL_STANDARD_14__
template <typename Ret, typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_invocable_r_v = is_invocable_r<Ret, F, Args...>::value;
#endif


MSTL_BEGIN_INNER__

template <typename F, typename T, typename... Args>
constexpr bool __invoke_is_nothrow_dispatch(_MSTL_TAG invoke_memfun_ref_tag) {
    return noexcept((_MSTL declval<unwrap_reference_t<T>>().*_MSTL declval<F>())(_MSTL declval<Args>()...));
}
template <typename F, typename T, typename... Args>
constexpr bool __invoke_is_nothrow_dispatch(_MSTL_TAG invoke_memfun_deref_tag) {
    return noexcept((*_MSTL declval<T>().*_MSTL declval<F>())(_MSTL declval<Args>()...));
}
template <typename F, typename T>
constexpr bool __invoke_is_nothrow_dispatch(_MSTL_TAG invoke_memobj_ref_tag) {
    return noexcept(_MSTL declval<unwrap_reference_t<T>>().*_MSTL declval<F>());
}
template <typename F, typename T>
constexpr bool __invoke_is_nothrow_dispatch(_MSTL_TAG invoke_memobj_deref_tag) {
    return noexcept(*_MSTL declval<T>().*_MSTL declval<F>());
}
template <typename F, typename... Args>
constexpr bool __invoke_is_nothrow_dispatch(_MSTL_TAG invoke_other_tag) {
    return noexcept(_MSTL declval<F>()(_MSTL declval<Args>()...));
}

template <typename Result, typename F, typename... Args>
struct __invoke_is_nothrow : bool_constant<
    __invoke_is_nothrow_dispatch<F, Args...> (typename Result::invoke_type{})> {};

template <typename F, typename... Args>
using __bind_invoke_is_nothrow = __invoke_is_nothrow<__invoke_result_aux<F, Args...>, F, Args...>;

MSTL_END_INNER__


template <typename F, typename... Args>
struct is_nothrow_invocable : conjunction<
    is_invocable<F, Args...>, _INNER __bind_invoke_is_nothrow<F, Args...>>::type {};

#ifdef MSTL_STANDARD_14__
template <typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_nothrow_invocable_v = is_nothrow_invocable<F, Args...>::value;
#endif


template <typename F, typename... Args>
struct is_predicate : bool_constant<
    is_invocable<F, Args...>::value && is_convertible<invoke_result_t<F, Args...>, bool>::value> {};

#ifdef MSTL_STANDARD_14__
template <typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_predicate_v = is_predicate<F, Args...>::value;
#endif


MSTL_BEGIN_INNER__

template <typename T, typename U = unwrap_reference_t<T>>
constexpr U&& __invoke_forward(remove_reference_t<T>& t) noexcept {
    return static_cast<U&&>(t);
}

template <typename Res, typename F, typename... Args>
MSTL_CONSTEXPR14 Res __invoke_dispatch(_MSTL_TAG invoke_other_tag, F&& f, Args&&... args) {
    return _MSTL forward<F>(f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
MSTL_CONSTEXPR14 Res __invoke_dispatch(_MSTL_TAG invoke_memfun_ref_tag, MemFun&& f, T&& t, Args&&... args) {
    return (_INNER __invoke_forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
MSTL_CONSTEXPR14 Res __invoke_dispatch(_MSTL_TAG invoke_memfun_deref_tag, MemFun&& f, T&& t, Args&&... args){
    return (*_MSTL forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemPtr, typename T>
MSTL_CONSTEXPR14 Res __invoke_dispatch(_MSTL_TAG invoke_memobj_ref_tag, MemPtr&& f, T&& t) {
    return _INNER __invoke_forward<T>(t).*f;
}
template <typename Res, typename MemPtr, typename T>
MSTL_CONSTEXPR14 Res __invoke_dispatch(_MSTL_TAG invoke_memobj_deref_tag, MemPtr&& f, T&& t) {
    return *_MSTL forward<T>(t).*f;
}

MSTL_END_INNER__

template <typename Callable, typename... Args>
MSTL_CONSTEXPR14 typename _INNER __invoke_result_aux<Callable, Args...>::type
invoke(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
    using result = _INNER __invoke_result_aux<Callable, Args...>;
    using type = typename result::type;
    using tag = typename result::invoke_type;
    return _INNER __invoke_dispatch<type>(tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
}


MSTL_BEGIN_INNER__

template <typename T, typename Tag, typename Res, typename Callable, typename... Args>
MSTL_CONSTEXPR14 enable_if_t<is_invocable_r<Res, Callable, Args...>::value && is_void<Res>::value, Res>
__invoke_r_dispatch(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
	__invoke_dispatch<T>(Tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
	return;
}

template <typename T, typename Tag, typename Res, typename Callable, typename... Args>
MSTL_CONSTEXPR14 enable_if_t<is_invocable_r<Res, Callable, Args...>::value && !is_void<Res>::value, Res>
__invoke_r_dispatch(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
	return __invoke_dispatch<T>(Tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
}

MSTL_END_INNER__

template <typename Res, typename Callable, typename... Args>
MSTL_CONSTEXPR14 enable_if_t<is_invocable_r<Res, Callable, Args...>::value, Res>
invoke_r(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
    using result = _INNER  __invoke_result_aux<Callable, Args...>;
    using type = typename result::type;
    using tag = typename result::invoke_type;
    return _INNER __invoke_r_dispatch<type, tag, Res, Callable, Args...>(
    	_MSTL forward<Callable>(f), _MSTL forward<Args>(args)...
	);
}

template <typename Callable, typename... Args, typename Res = invoke_result_t<Callable, Args...>>
MSTL_CONSTEXPR14 enable_if_t<is_invocable_r<Res, Callable, Args...>::value, Res>
invoke_ra(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
    using result = _INNER  __invoke_result_aux<Callable, Args...>;
    using type = typename result::type;
    using tag = typename result::invoke_type;
    return _INNER __invoke_r_dispatch<type, tag, Res, Callable, Args...>(
        _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...
    );
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
