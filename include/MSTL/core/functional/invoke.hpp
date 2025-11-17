#ifndef MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
#define MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
#include "../utility/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

template <typename T, typename U = unwrap_reference_t<T>>
constexpr U&& __invoke_forward(remove_reference_t<T>& t) noexcept {
    return static_cast<U&&>(t);
}

template <typename Res, typename F, typename... Args>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_other_tag, F&& f, Args&&... args) {
    return _MSTL forward<F>(f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_memfun_ref_tag, MemFun&& f, T&& t, Args&&... args) {
    return (_INNER __invoke_forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_memfun_deref_tag, MemFun&& f, T&& t, Args&&... args){
    return (*_MSTL forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemPtr, typename T>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_memobj_ref_tag, MemPtr&& f, T&& t) {
    return _INNER __invoke_forward<T>(t).*f;
}
template <typename Res, typename MemPtr, typename T>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_memobj_deref_tag, MemPtr&& f, T&& t) {
    return *_MSTL forward<T>(t).*f;
}

MSTL_END_INNER__

template <typename Callable, typename... Args>
constexpr typename _INNER __invoke_result_aux<Callable, Args...>::type
invoke(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
    using result = _INNER __invoke_result_aux<Callable, Args...>;
    using type = typename result::type;
    using tag = typename result::invoke_type;
    return _INNER __invoke_dispatch<type>(tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
}


MSTL_BEGIN_INNER__

template <typename T, typename Tag, typename Res, typename Callable, typename... Args>
constexpr enable_if_t<is_invocable_r_v<Res, Callable, Args...> && is_void_v<Res>, Res>
__invoke_r_dispatch(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable_v<Callable, Args...>) {
	__invoke_dispatch<T>(Tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
	return;
}

template <typename T, typename Tag, typename Res, typename Callable, typename... Args>
constexpr enable_if_t<is_invocable_r_v<Res, Callable, Args...> && !is_void_v<Res>, Res>
__invoke_r_dispatch(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable_v<Callable, Args...>) {
	return __invoke_dispatch<T>(Tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
}

MSTL_END_INNER__

template <typename Res, typename Callable, typename... Args>
constexpr enable_if_t<is_invocable_r_v<Res, Callable, Args...>, Res>
invoke_r(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable_v<Callable, Args...>) {
    using result = _INNER  __invoke_result_aux<Callable, Args...>;
    using type = typename result::type;
    using tag = typename result::invoke_type;
    return _INNER __invoke_r_dispatch<type, tag, Res, Callable, Args...>(
    	_MSTL forward<Callable>(f), _MSTL forward<Args>(args)...
	);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
