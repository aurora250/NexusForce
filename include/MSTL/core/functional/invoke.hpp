#ifndef MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
#define MSTL_CORE_FUNCTIONAL_INVOKE_HPP__

/**
 * @file invoke.hpp
 * @brief MSTL统一调用接口
 *
 * 此文件提供了统一的可调用接口，并提供相关的可调用性检查工具。
 */

#include "MSTL/core/utility/reference_wrapper.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup InvokeTags 可调用标签
 * @brief 标识不同调用类型的标签结构
 * @{
 */

/**
 * @struct invoke_memfun_ref_tag
 * @brief 成员函数引用调用标签
 *
 * 用于标记通过对象引用调用成员函数的情况
 */
struct invoke_memfun_ref_tag {
    constexpr invoke_memfun_ref_tag() noexcept = default;
};

/**
 * @struct invoke_memfun_deref_tag
 * @brief 成员函数解引用调用标签
 *
 * 用于标记通过对象指针调用成员函数的情况
 */
struct invoke_memfun_deref_tag {
    constexpr invoke_memfun_deref_tag() noexcept = default;
};

/**
 * @struct invoke_memobj_ref_tag
 * @brief 成员对象引用调用标签
 *
 * 用于标记通过对象引用访问成员对象的情况
 */
struct invoke_memobj_ref_tag {
    constexpr invoke_memobj_ref_tag() noexcept = default;
};

/**
 * @struct invoke_memobj_deref_tag
 * @brief 成员对象解引用调用标签
 *
 * 用于标记通过对象指针访问成员对象的情况
 */
struct invoke_memobj_deref_tag {
    constexpr invoke_memobj_deref_tag() noexcept = default;
};

/**
 * @struct invoke_other_tag
 * @brief 其他类型调用标签
 *
 * 用于标记普通函数调用、函数对象调用等情况
 */
struct invoke_other_tag {
    constexpr invoke_other_tag() noexcept = default;
};

/** @} */ // InvokeTags

/**
 * @defgroup InvokeResult 可调用结果类型
 * @brief 推导函数调用结果类型的工具
 * @{
 */

/**
 * @struct invoke_result
 * @brief 推导函数调用结果类型的主模板
 * @tparam Sign 函数签名类型
 *
 * 使用特化形式F(Args...)指定函数和参数类型。
 * 推导调用F(Args...)的结果类型。
 */
template <typename Sign>
struct invoke_result;

/**
 * @struct invoke_result_true
 * @brief 成功推导到调用结果类型的包装器
 * @tparam T 推导出的调用结果类型
 * @tparam Tag 调用类型标签
 *
 * 包含invoke_type和type两个类型成员，分别表示调用类型和结果类型。
 */
template <typename T, typename Tag>
struct invoke_result_true {
    using invoke_type   = Tag; ///< 调用类型标签
    using type          = T;   ///< 调用结果类型
};

/**
 * @struct invoke_result_false
 * @brief 推导失败的空结构
 *
 * 当无法推导调用结果类型时使用，不包含任何类型成员。
 */
struct invoke_result_false {};


/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __invoke_result_memfun_ref
 * @brief 推导成员函数引用调用的结果类型
 * @tparam MemPtr 成员函数指针类型
 * @tparam Arg 对象参数类型
 * @tparam Args 函数参数类型
 */
template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_memfun_ref {
private:
    template <typename F, typename T, typename... Args1>
    static invoke_result_true<decltype((_MSTL declval<T>().*_MSTL declval<F>())(_MSTL declval<Args1>()...)),
        invoke_memfun_ref_tag> __test(int);

    template <typename...>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg, Args...>(0));
};

/**
 * @struct __invoke_result_memfun_deref
 * @brief 推导成员函数解引用调用的结果类型
 * @tparam MemPtr 成员函数指针类型
 * @tparam Arg 对象指针参数类型
 * @tparam Args 函数参数类型
 */
template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_memfun_deref {
private:
    template <typename F, typename T, typename... Args1>
    static invoke_result_true<decltype((*_MSTL declval<T>().*_MSTL declval<F>())(_MSTL declval<Args1>()...)),
        invoke_memfun_deref_tag> __test(int);

    template <typename...>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg, Args...>(0));
};

/**
 * @struct __invoke_result_memobj_ref
 * @brief 推导成员对象引用访问的结果类型
 * @tparam MemPtr 成员对象指针类型
 * @tparam Arg 对象参数类型
 */
template <typename MemPtr, typename Arg>
struct __invoke_result_memobj_ref {
private:
    template <typename F, typename T>
    static invoke_result_true<decltype(_MSTL declval<T>().*_MSTL declval<F>()),
        invoke_memobj_ref_tag> __test(int);

    template <typename, typename>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg>(0));
};

/**
 * @struct __invoke_result_memobj_deref
 * @brief 推导成员对象解引用访问的结果类型
 * @tparam MemPtr 成员对象指针类型
 * @tparam Arg 对象指针参数类型
 */
template <typename MemPtr, typename Arg>
struct __invoke_result_memobj_deref {
private:
    template <typename F, typename T>
    static invoke_result_true<decltype(*_MSTL declval<T>().*_MSTL declval<F>()),
        invoke_memobj_deref_tag> __test(int);

    template <typename, typename>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<MemPtr, Arg>(0));
};

/**
 * @struct __invoke_result_memobj
 * @brief 成员对象调用结果类型推导的分发器
 * @tparam MemPtr 成员对象指针类型
 * @tparam Arg 对象参数类型
 *
 * 根据对象类型决定使用引用访问还是解引用访问。
 */
template <typename MemPtr, typename Arg>
struct __invoke_result_memobj;

/**
 * @brief 成员对象指针特化的结果类型推导
 * @tparam Res 成员类型
 * @tparam Class 类类型
 * @tparam Arg 对象参数类型
 */
template <typename Res, typename Class, typename Arg>
struct __invoke_result_memobj<Res Class::*, Arg> {
    using Argval = remove_cvref_t<Arg>;
    using MemPtr = Res Class::*;
    using type = typename conditional_t<disjunction<
        is_same<Argval, Class>, is_base_of<Class, Argval>>::value,
        __invoke_result_memobj_ref<MemPtr, Arg>,
        __invoke_result_memobj_deref<MemPtr, Arg>>::type;
};

/**
 * @struct __invoke_result_memfun
 * @brief 成员函数调用结果类型推导的分发器
 * @tparam MemPtr 成员函数指针类型
 * @tparam Arg 对象参数类型
 * @tparam Args 函数参数类型
 */
template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_memfun;

/**
 * @brief 成员函数指针特化的结果类型推导
 * @tparam Res 返回类型
 * @tparam Class 类类型
 * @tparam Arg 对象参数类型
 * @tparam Args 函数参数类型
 */
template <typename Res, typename Class, typename Arg, typename... Args>
struct __invoke_result_memfun<Res Class::*, Arg, Args...> {
    using MemPtr = Res Class::*;
    using type = typename conditional_t<is_base_of<Class, remove_reference_t<Arg>>::value,
        __invoke_result_memfun_ref<MemPtr, Arg, Args...>,
        __invoke_result_memfun_deref<MemPtr, Arg, Args...>>::type;
};

/**
 * @struct __invoke_result_dispatch
 * @brief 调用结果类型推导的主分发器
 * @tparam IsMemObj 是否为成员对象指针
 * @tparam IsMemFun 是否为成员函数指针
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 */
template <bool IsMemObj, bool IsMemFun, typename F, typename... Args>
struct __invoke_result_dispatch {
    using type = invoke_result_false;
};

/// 成员对象指针的分发特化
template <typename MemPtr, typename Arg>
struct __invoke_result_dispatch<true, false, MemPtr, Arg>
    : __invoke_result_memobj<decay_t<MemPtr>, unwrap_reference_t<Arg>> {};

/// 成员函数指针的分发特化
template <typename MemPtr, typename Arg, typename... Args>
struct __invoke_result_dispatch<false, true, MemPtr, Arg, Args...>
    : __invoke_result_memfun<decay_t<MemPtr>, unwrap_reference_t<Arg>, Args...> {};

/// 其他可调用对象的分发特化
template <typename F, typename... Args>
struct __invoke_result_dispatch<false, false, F, Args...> {
private:
    template <typename F1, typename... Args1>
    static invoke_result_true<
        decltype(_MSTL declval<F1>()(_MSTL declval<Args1>()...)), invoke_other_tag> __test(int);

    template <typename...>
    static invoke_result_false __test(...);

public:
    using type = decltype(__test<F, Args...>(0));
};

/**
 * @struct __invoke_result_aux
 * @brief 调用结果类型推导的入口点
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 */
template <typename F, typename... Args>
struct __invoke_result_aux : __invoke_result_dispatch<
    is_member_object_pointer<remove_reference_t<F>>::value,
    is_member_function_pointer<remove_reference_t<F>>::value,
    F, Args...>::type {};

MSTL_END_INNER__
/// @endcond


/**
 * @brief invoke_result的特化版本
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 */
template <typename F, typename... Args>
struct invoke_result<F(Args...)> : _INNER __invoke_result_aux<F, Args...> {};

/**
 * @typedef invoke_result_t
 * @brief invoke_result的便捷别名
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 */
template <typename F, typename... Args>
using invoke_result_t = typename _INNER __invoke_result_aux<F, Args...>::type;

/** @} */ // InvokeResult

/**
 * @defgroup InvocableChecks 可调用性检查
 * @brief 检查类型是否可调用以及调用特性
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct __is_invocable_aux
 * @brief 可调用性检查的辅助模板
 * @tparam Result 调用结果类型推导结果
 * @tparam Ret 期望的返回类型
 * @tparam IsVoid 期望的返回类型是否为void
 * @tparam Dummy SFINAE参数
 */
template <typename Result, typename Ret, bool IsVoid = is_void<Ret>::value, typename Dummy = void>
struct __is_invocable_aux : false_type {};

/// 返回类型为void的特化
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

/// 返回类型非void的特化
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
/// @endcond


/**
 * @struct is_invocable
 * @brief 判断类型是否可调用
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 *
 * 检查是否可以使用给定的参数调用F，不关心返回类型。
 */
template <typename F, typename... Args>
struct is_invocable : _INNER __is_invocable_aux<_INNER __invoke_result_aux<F, Args...>, void>::type {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_invocable_v
 * @brief is_invocable的便捷变量模板
 */
template <typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_invocable_v = is_invocable<F, Args...>::value;
#endif


/**
 * @struct is_invocable_r
 * @brief 判断类型是否可调用并返回指定类型
 * @tparam Ret 期望的返回类型
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 *
 * 检查是否可以使用给定的参数调用F，并且返回类型可以转换为Ret。
 */
template <typename Ret, typename F, typename... Args>
struct is_invocable_r : _INNER __is_invocable_aux<_INNER __invoke_result_aux<F, Args...>, Ret>::type {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_invocable_r_v
 * @brief is_invocable_r的便捷变量模板
 */
template <typename Ret, typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_invocable_r_v = is_invocable_r<Ret, F, Args...>::value;
#endif


/// @cond
MSTL_BEGIN_INNER__

template <typename F, typename T, typename... Args>
constexpr bool __invoke_is_nothrow_dispatch(invoke_memfun_ref_tag) {
    return noexcept((_MSTL declval<unwrap_reference_t<T>>().*_MSTL declval<F>())(_MSTL declval<Args>()...));
}
template <typename F, typename T, typename... Args>
constexpr bool __invoke_is_nothrow_dispatch(invoke_memfun_deref_tag) {
    return noexcept((*_MSTL declval<T>().*_MSTL declval<F>())(_MSTL declval<Args>()...));
}
template <typename F, typename T>
constexpr bool __invoke_is_nothrow_dispatch(invoke_memobj_ref_tag) {
    return noexcept(_MSTL declval<unwrap_reference_t<T>>().*_MSTL declval<F>());
}
template <typename F, typename T>
constexpr bool __invoke_is_nothrow_dispatch(invoke_memobj_deref_tag) {
    return noexcept(*_MSTL declval<T>().*_MSTL declval<F>());
}
template <typename F, typename... Args>
constexpr bool __invoke_is_nothrow_dispatch(invoke_other_tag) {
    return noexcept(_MSTL declval<F>()(_MSTL declval<Args>()...));
}

/**
 * @struct __invoke_is_nothrow
 * @brief 检查调用是否无异常的辅助模板
 * @tparam Result 调用结果类型推导结果
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 */
template <typename Result, typename F, typename... Args>
struct __invoke_is_nothrow : bool_constant<
    __invoke_is_nothrow_dispatch<F, Args...> (typename Result::invoke_type{})> {};

/**
 * @typedef __bind_invoke_is_nothrow
 * @brief 绑定invoke_is_nothrow的便捷别名
 */
template <typename F, typename... Args>
using __bind_invoke_is_nothrow = __invoke_is_nothrow<__invoke_result_aux<F, Args...>, F, Args...>;

MSTL_END_INNER__
/// @endcond


/**
 * @struct is_nothrow_invocable
 * @brief 判断调用是否不会抛出异常
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 *
 * 检查是否可以使用给定的参数调用F，并且调用过程不会抛出异常。
 */
template <typename F, typename... Args>
struct is_nothrow_invocable : conjunction<
    is_invocable<F, Args...>, _INNER __bind_invoke_is_nothrow<F, Args...>>::type {};

#ifdef MSTL_STANDARD_14__
/**
 * @var is_nothrow_invocable_v
 * @brief is_nothrow_invocable的便捷变量模板
 */
template <typename F, typename... Args>
MSTL_INLINE17 constexpr bool is_nothrow_invocable_v = is_nothrow_invocable<F, Args...>::value;
#endif

/** @} */ // InvocableChecks

/**
 * @defgroup InvokeFunction 可调用函数
 * @brief 可调用函数系列函数的实现
 * @{
 */

/// @cond
MSTL_BEGIN_INNER__

template <typename T, typename U = unwrap_reference_t<T>>
constexpr U&& __invoke_forward(remove_reference_t<T>& t) noexcept {
    return static_cast<U&&>(t);
}

template <typename Res, typename F, typename... Args>
MSTL_CONSTEXPR14 Res __invoke_dispatch(invoke_other_tag, F&& f, Args&&... args) {
    return _MSTL forward<F>(f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
MSTL_CONSTEXPR14 Res __invoke_dispatch(invoke_memfun_ref_tag, MemFun&& f, T&& t, Args&&... args) {
    return (_INNER __invoke_forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
MSTL_CONSTEXPR14 Res __invoke_dispatch(invoke_memfun_deref_tag, MemFun&& f, T&& t, Args&&... args){
    return (*_MSTL forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemPtr, typename T>
MSTL_CONSTEXPR14 Res __invoke_dispatch(invoke_memobj_ref_tag, MemPtr&& f, T&& t) {
    return _INNER __invoke_forward<T>(t).*f;
}
template <typename Res, typename MemPtr, typename T>
MSTL_CONSTEXPR14 Res __invoke_dispatch(invoke_memobj_deref_tag, MemPtr&& f, T&& t) {
    return *_MSTL forward<T>(t).*f;
}

MSTL_END_INNER__
/// @endcond


/**
 * @brief 统一调用接口
 * @tparam Callable 可调用对象类型
 * @tparam Args 参数类型
 * @param f 可调用对象
 * @param args 调用参数
 * @return 调用结果
 *
 * 统一调用接口，支持以下所有调用形式：
 * 1. 普通函数调用：f(args...)
 * 2. 成员函数调用：(obj.*f)(args...) 或 (ptr->*f)(args...)
 * 3. 成员对象访问：obj.*f 或 ptr->*f
 * 4. 函数对象调用
 */
template <typename Callable, typename... Args>
MSTL_CONSTEXPR14 typename _INNER __invoke_result_aux<Callable, Args...>::type
invoke(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value) {
    using result = _INNER __invoke_result_aux<Callable, Args...>;
    using type = typename result::type;
    using tag = typename result::invoke_type;
    return _INNER __invoke_dispatch<type>(tag{}, _MSTL forward<Callable>(f), _MSTL forward<Args>(args)...);
}


/// @cond
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
/// @endcond


/**
 * @brief 带返回类型检查的统一调用接口
 * @tparam Res 期望的返回类型
 * @tparam Callable 可调用对象类型
 * @tparam Args 参数类型
 * @param f 可调用对象
 * @param args 调用参数
 * @return 调用结果，转换为Res类型
 *
 * 与invoke类似，但额外检查返回类型是否可以转换为Res。
 */
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

/** @} */ // InvokeFunction

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_INVOKE_HPP__
