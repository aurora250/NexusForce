#ifndef MSTL_CORE_FUNCTIONAL_BIND_HPP__
#define MSTL_CORE_FUNCTIONAL_BIND_HPP__
#include "invoke.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__

template <typename Res, typename... Args>
struct unary_or_binary_function {};

template <typename Res, typename T1>
struct unary_or_binary_function<Res, T1>
    : _MSTL unary_function<T1, Res> {};

template <typename Res, typename T1, typename T2>
struct unary_or_binary_function<Res, T1, T2>
    : _MSTL binary_function<T1, T2, Res> {};


template <typename Sign>
struct mem_func_traits;

template <typename Res, typename Class, typename... Args>
struct mem_func_traits_base {
    using result_type = Res;
    using maybe_type = unary_or_binary_function<Res, Class*, Args...>;
    using arity = integral_constant<size_t, sizeof...(Args)>;
};

#define MSTL_MEMFUNC_TRAITS_BASE(CV, REF, LVAL, RVAL) \
template <typename Res, typename Class, typename... Args> \
struct mem_func_traits<Res (Class::*)(Args...) CV REF> \
    : mem_func_traits_base<Res, CV Class, Args...> { \
    using vararg = false_type; \
}; \
template <typename Res, typename Class, typename... Args> \
struct mem_func_traits<Res (Class::*)(Args..., ...) CV REF> \
    : mem_func_traits_base<Res, CV Class, Args...> { \
    using vararg = true_type; \
};

#define MSTL_MEMFUNC_TRAITS(REF, LVAL, RVAL) \
MSTL_MEMFUNC_TRAITS_BASE(, REF, LVAL, RVAL) \
MSTL_MEMFUNC_TRAITS_BASE(const, REF, LVAL, RVAL) \
MSTL_MEMFUNC_TRAITS_BASE(volatile, REF, LVAL, RVAL) \
MSTL_MEMFUNC_TRAITS_BASE(const volatile, REF, LVAL, RVAL)

MSTL_MEMFUNC_TRAITS( , true_type, true_type)
MSTL_MEMFUNC_TRAITS(&, true_type, false_type)
MSTL_MEMFUNC_TRAITS(&&, false_type, true_type)

#ifdef MSTL_STANDARD_17__
MSTL_MEMFUNC_TRAITS(noexcept, true_type, true_type)
MSTL_MEMFUNC_TRAITS(& noexcept, true_type, false_type)
MSTL_MEMFUNC_TRAITS(&& noexcept, false_type, true_type)
#endif

#undef MSTL_MEMFUNC_TRAITS_BASE
#undef MSTL_MEMFUNC_TRAITS


template <typename Func, typename = void_t<>>
struct maybe_get_result_type {};

template <typename Func>
struct maybe_get_result_type<Func, void_t<typename Func::result_type>> {
    using result_type = typename Func::result_type;
};


template <typename Func>
struct __weak_result_type_impl : maybe_get_result_type<Func> {};

template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(Args...)> {
    using result_type = Res;
};
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(Args..., ...)> {
    using result_type = Res;
};
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(*)(Args...)> {
    using result_type = Res;
};
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(*)(Args..., ...)> {
    using result_type = Res;
};

#ifdef MSTL_STANDARD_17__
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(Args...) noexcept> {
    using result_type = Res;
};
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(Args..., ...) noexcept> {
    using result_type = Res;
};
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(*)(Args...) noexcept> {
    using result_type = Res;
};
template <typename Res, typename... Args>
struct __weak_result_type_impl<Res(*)(Args..., ...) noexcept> {
    using result_type = Res;
};
#endif


template <typename Func, bool = is_member_function_pointer<Func>::value>
struct __weak_result_type_memfun
    : __weak_result_type_impl<Func> {};

template <typename MemFunPtr>
struct __weak_result_type_memfun<MemFunPtr, true> {
    using result_type = typename mem_func_traits<MemFunPtr>::result_type;
};

template <typename Func, typename Class>
struct __weak_result_type_memfun<Func Class::*, false> {};


template <typename Func>
struct weak_result_type : __weak_result_type_memfun<remove_cv_t<Func>> {};


template <typename MemberPtr, bool IsMemFunc = is_member_function_pointer_v<MemberPtr>>
class mem_func_base
    : public _INNER mem_func_traits<MemberPtr>::maybe_type {
    using Traits = _INNER mem_func_traits<MemberPtr>;
    using Arity = typename Traits::arity;
    using Varargs = typename Traits::vararg;

    template <typename Func, typename... BoundArgs>
    friend struct bind_check_arity;

    MemberPtr ptr_;

public:
    using result_type = typename Traits::result_type;

    explicit constexpr mem_func_base(MemberPtr pmf) noexcept : ptr_(pmf) {}

    template <typename... Args>
    MSTL_CONSTEXPR20 auto operator()(Args&&... args) const
    noexcept(noexcept(_MSTL invoke(ptr_, _MSTL forward<Args>(args)...)))
    -> decltype(_MSTL invoke(ptr_, _MSTL forward<Args>(args)...)) {
        return _MSTL invoke(ptr_, _MSTL forward<Args>(args)...);
    }
};

template <typename MemberObjPtr>
class mem_func_base<MemberObjPtr, false> {
    using Arity = integral_constant<size_t, 0>;
    using Varargs = false_type;

    template <typename Func, typename... BoundArgs>
    friend struct bind_check_arity;

    MemberObjPtr ptr_;

public:
    explicit constexpr mem_func_base(MemberObjPtr pm) noexcept : ptr_(pm) {}

    template <typename T>
    MSTL_CONSTEXPR20 auto operator()(T&& obj) const
    noexcept(noexcept(_MSTL invoke(ptr_, _MSTL forward<T>(obj))))
    -> decltype(_MSTL invoke(ptr_, _MSTL forward<T>(obj))) {
        return _MSTL invoke(ptr_, _MSTL forward<T>(obj));
    }
};

template <typename MemberPointer>
struct mem_func;

template <typename Res, typename Class>
struct mem_func<Res Class::*> : mem_func_base<Res Class::*> {
    using mem_func_base<Res Class::*>::mem_func_base;
};

MSTL_END_INNER__


template <typename T>
struct is_bind_expression : false_type {};
template <typename T>
MSTL_INLINE17 constexpr bool is_bind_expression_v = is_bind_expression<T>::value;

template <typename T>
struct is_placeholder : uint32_constant<0> {};
template <typename T>
MSTL_INLINE17 constexpr uint32_t is_placeholder_v = is_placeholder<T>::value;


template <uint32_t Num>
struct placeholder {};

namespace placeholders {
    MSTL_INLINE17 constexpr placeholder<1>  p1{};
    MSTL_INLINE17 constexpr placeholder<2>  p2{};
    MSTL_INLINE17 constexpr placeholder<3>  p3{};
    MSTL_INLINE17 constexpr placeholder<4>  p4{};
    MSTL_INLINE17 constexpr placeholder<5>  p5{};
    MSTL_INLINE17 constexpr placeholder<6>  p6{};
    MSTL_INLINE17 constexpr placeholder<7>  p7{};
    MSTL_INLINE17 constexpr placeholder<8>  p8{};
    MSTL_INLINE17 constexpr placeholder<9>  p9{};
    MSTL_INLINE17 constexpr placeholder<10> p10{};
    MSTL_INLINE17 constexpr placeholder<11> p11{};
    MSTL_INLINE17 constexpr placeholder<12> p12{};
    MSTL_INLINE17 constexpr placeholder<13> p13{};
    MSTL_INLINE17 constexpr placeholder<14> p14{};
    MSTL_INLINE17 constexpr placeholder<15> p15{};
    MSTL_INLINE17 constexpr placeholder<16> p16{};
    MSTL_INLINE17 constexpr placeholder<17> p17{};
    MSTL_INLINE17 constexpr placeholder<18> p18{};
    MSTL_INLINE17 constexpr placeholder<19> p19{};
    MSTL_INLINE17 constexpr placeholder<20> p20{};
    MSTL_INLINE17 constexpr placeholder<21> p21{};
    MSTL_INLINE17 constexpr placeholder<22> p22{};
    MSTL_INLINE17 constexpr placeholder<23> p23{};
    MSTL_INLINE17 constexpr placeholder<24> p24{};
    MSTL_INLINE17 constexpr placeholder<25> p25{};
    MSTL_INLINE17 constexpr placeholder<26> p26{};
    MSTL_INLINE17 constexpr placeholder<27> p27{};
    MSTL_INLINE17 constexpr placeholder<28> p28{};
    MSTL_INLINE17 constexpr placeholder<29> p29{};
}

template <uint32_t Num>
struct is_placeholder<placeholder<Num>> : uint32_constant<Num> {};
template <uint32_t Num>
struct is_placeholder<const placeholder<Num>> : uint32_constant<Num> {};

MSTL_BEGIN_INNER__

template <typename Arg,
    bool IsBindExp = is_bind_expression_v<Arg>,
    bool IsPlaceholder = (is_placeholder_v<Arg> > 0)>
class mu;

template <typename T>
class mu<reference_wrapper<T>, false, false> {
public:
    template <typename CVRef, typename Tuple>
    MSTL_CONSTEXPR20 T& operator()(CVRef& arg, Tuple&) const volatile {
        return arg.get();
    }
};

template <typename Arg>
class mu<Arg, true, false> {
public:
    template <typename CVArg, typename... Args>
    MSTL_CONSTEXPR20 auto operator()(CVArg& arg, tuple<Args...>& tuple_ref) const volatile
    -> decltype(arg(declval<Args>()...)) {
        using Indexes = build_index_tuple_t<sizeof...(Args)>;
        return call(arg, tuple_ref, Indexes());
    }

private:
    template <typename CVArg, typename... Args, size_t... Indexes>
    MSTL_CONSTEXPR20 auto call(CVArg& arg, tuple<Args...>& tuple_ref,
        const index_tuple<Indexes...>&) const volatile
    -> decltype(arg(declval<Args>()...)) {
        return arg(_MSTL get<Indexes>(_MSTL move(tuple_ref))...);
    }
};


template <size_t I, typename Tuple>
using safe_tuple_element_t = enable_if_t<(I < tuple_size_v<Tuple>), tuple_element_t<I, Tuple>>;

template <typename Arg>
class mu<Arg, false, true> {
public:
    template <typename Tuple>
    MSTL_CONSTEXPR20 safe_tuple_element_t<(is_placeholder<Arg>::value - 1), Tuple>&&
    operator()(const volatile Arg&, Tuple& tuple_ref) const volatile {
        return _MSTL get<(is_placeholder<Arg>::value - 1)>(_MSTL move(tuple_ref));
    }
};

template <typename Arg>
class mu<Arg, false, false> {
public:
    template <typename CVArg, typename Tuple>
    MSTL_CONSTEXPR20 CVArg&& operator()(CVArg&& arg, Tuple&) const volatile {
        return _MSTL forward<CVArg>(arg);
    }
};

MSTL_END_INNER__


template <typename Sign>
class binder;

template <typename Func, typename... BoundArgs>
class binder<Func(BoundArgs...)> : public _INNER weak_result_type<Func> {
private:
    using BoundIndexes = build_index_tuple_t<sizeof...(BoundArgs)>;

    Func functor_;
    tuple<BoundArgs...> bound_args_;

private:
    template <typename BoundArg, typename CallArgs>
    struct mu_result {
        using type = decltype(_INNER mu<remove_cv_t<BoundArg>>()(
            _MSTL declval<BoundArg&>(), _MSTL declval<CallArgs&>()));
    };

    template <typename BoundArg, typename CallArgs>
    using mu_result_t = typename mu_result<BoundArg, CallArgs>::type;

    template <typename CallArgs>
    using result_type = invoke_result_t<
        Func&,
        mu_result_t<BoundArgs, CallArgs>...
    >;

    template <typename CallArgs>
    using result_type_const = invoke_result_t<
        const Func&,
        mu_result_t<const BoundArgs, CallArgs>...
    >;

    template <typename BoundArg, typename CallArgs>
    using mu_type = decltype(_INNER mu<remove_cv_t<BoundArg>>()(
        _MSTL declval<BoundArg&>(), _MSTL declval<CallArgs&>()));

    template <typename CallArgs>
    using dependent = enable_if_t<static_cast<bool>(tuple_size_v<CallArgs> + 1), Func>;

private:
    template <typename Res, typename... Args, size_t... Indexes>
    MSTL_CONSTEXPR20 Res call(tuple<Args...>&& args, index_tuple<Indexes...>) {
        return _MSTL invoke(functor_,
            _INNER mu<BoundArgs>()(_MSTL get<Indexes>(bound_args_), args)...
        );
    }

    template <typename Res, typename... Args, _MSTL size_t... Indexes>
    MSTL_CONSTEXPR20 Res call_const(tuple<Args...>&& args, index_tuple<Indexes...>) const {
        return _MSTL invoke(functor_,
            _INNER mu<BoundArgs>()(_MSTL get<Indexes>(bound_args_), args)...
        );
    }

public:
    template <typename... Args>
    explicit MSTL_CONSTEXPR20 binder(const Func& func, Args&&... args)
        : functor_(func), bound_args_(_MSTL forward<Args>(args)...) {}

    template <typename... Args>
    explicit MSTL_CONSTEXPR20 binder(Func&& func, Args&&... args)
        : functor_(_MSTL move(func)), bound_args_(_MSTL forward<Args>(args)...) {}

    binder(const binder&) = default;
    binder(binder&&) = default;

    template <typename... Args>
    MSTL_CONSTEXPR20 auto operator()(Args&&... args)
    -> result_type<tuple<Args&&...>> {
        using Res = result_type<tuple<Args&&...>>;
        return binder::call<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    template <typename... Args>
    MSTL_CONSTEXPR20 auto operator()(Args&&... args) const
    -> result_type_const<tuple<Args&&...>> {
        using Res = result_type_const<tuple<Args&&...>>;
        return binder::call_const<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }
};


template <typename Res, typename Sign>
class bindrer;

template <typename Res, typename Func, typename... BoundArgs>
class bindrer<Res, Func(BoundArgs...)> {
private:
    using BoundIndexes = build_index_tuple_t<sizeof...(BoundArgs)>;

    Func functor_;
    tuple<BoundArgs...> bound_args_;

private:
    template <typename Result, typename... Args, size_t... Indexes>
    MSTL_CONSTEXPR20 Result call(tuple<Args...>&& args, index_tuple<Indexes...>) {
        return _MSTL invoke_r<Res>(functor_, _INNER mu<BoundArgs>()(
            _MSTL get<Indexes>(bound_args_), args)...);
    }

    template <typename Result, typename... Args, _MSTL size_t... Indexes>
    MSTL_CONSTEXPR20 Result call(tuple<Args...>&& args, index_tuple<Indexes...>) const {
        return _MSTL invoke_r<Res>(functor_, _INNER mu<BoundArgs>()(
            _MSTL get<Indexes>(bound_args_), args)...);
    }

public:
    using result_type = Res;

    template <typename... Args>
    explicit MSTL_CONSTEXPR20 bindrer(const Func& func, Args&&... args)
        : functor_(func), bound_args_(_MSTL forward<Args>(args)...) {}

    template <typename... Args>
    explicit MSTL_CONSTEXPR20 bindrer(Func&& func, Args&&... args)
        : functor_(_MSTL move(func)), bound_args_(_MSTL forward<Args>(args)...) {}

    bindrer(const bindrer&) = default;
    bindrer(bindrer&&) = default;

    template <typename... Args>
    MSTL_CONSTEXPR20 result_type operator()(Args&&... args) {
        return bindrer::call<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    template <typename... Args>
    MSTL_CONSTEXPR20 result_type operator()(Args&&... args) const {
        return bindrer::call<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    template <typename... Args>
    void operator()(Args&&...) const volatile = delete;
};

template <typename Sign>
struct is_bind_expression<binder<Sign>> : true_type {};
template <typename Sign>
struct is_bind_expression<const binder<Sign>> : true_type {};
template <typename Sign>
struct is_bind_expression<volatile binder<Sign>> : true_type {};
template <typename Sign>
struct is_bind_expression<const volatile binder<Sign>> : true_type {};
template <typename Res, typename Sign>
struct is_bind_expression<bindrer<Res, Sign>> : true_type {};
template <typename Res, typename Sign>
struct is_bind_expression<const bindrer<Res, Sign>> : true_type {};
template <typename Res, typename Sign>
struct is_bind_expression<volatile bindrer<Res, Sign>> : true_type {};
template <typename Res, typename Sign>
struct is_bind_expression<const volatile bindrer<Res, Sign>> : true_type {};

MSTL_BEGIN_INNER__

template <typename Func, typename... BoundArgs>
struct bind_check_arity {};

template <typename Ret, typename... Args, typename... BoundArgs>
struct bind_check_arity<Ret (*)(Args...), BoundArgs...> {
    static_assert(sizeof...(BoundArgs) == sizeof...(Args),
        "Wrong number of arguments for function");
};

template <typename Ret, typename... Args, typename... BoundArgs>
struct bind_check_arity<Ret (*)(Args..., ...), BoundArgs...> {
    static_assert(sizeof...(BoundArgs) >= sizeof...(Args),
        "Wrong number of arguments for function");
};

template <typename T, typename Class, typename... BoundArgs>
struct bind_check_arity<T Class::*, BoundArgs...> {
    using Arity = typename mem_func<T Class::*>::Arity;
    using Varargs = typename mem_func<T Class::*>::Varargs;
    static_assert(Varargs::value
        ? sizeof...(BoundArgs) >= Arity::value + 1
        : sizeof...(BoundArgs) == Arity::value + 1,
        "Wrong number of arguments for pointer-to-member");
};

MSTL_END_INNER__

template <bool IntLike, typename Func, typename... BoundArgs>
struct bind_helper
    : _INNER bind_check_arity<decay_t<Func>, BoundArgs...> {
    using func_type = decay_t<Func>;
    using type = binder<func_type(decay_t<BoundArgs>...)>;
};

template <typename Func, typename... BoundArgs>
struct bind_helper<true, Func, BoundArgs...> {};

template <bool IntLike, typename Func, typename... BoundArgs>
using bind_helper_t = typename bind_helper<IntLike, Func, BoundArgs...>::type;


template <typename Func, typename... BoundArgs>
MSTL_NODISCARD MSTL_CONSTEXPR20 bind_helper_t<is_integral_like<Func>::value, Func, BoundArgs...>
bind(Func&& func, BoundArgs&&... args) {
    return bind_helper_t<false, Func, BoundArgs...>(
            _MSTL forward<Func>(func),
            _MSTL forward<BoundArgs>(args)...
        );
}


template <typename Res, typename Func, typename... BoundArgs>
struct bindr_helper
    : _INNER bind_check_arity<decay_t<Func>, BoundArgs...> {
    using type = bindrer<Res, decay_t<Func>(decay_t<BoundArgs>...)>;
};

template <typename Res, typename Func, typename... BoundArgs>
using bindr_helper_t = typename bindr_helper<Res, Func, BoundArgs...>::type;


template <typename Res, typename Func, typename... BoundArgs>
MSTL_NODISCARD MSTL_CONSTEXPR20 bindr_helper_t<Res, Func, BoundArgs...>
bind(Func&& func, BoundArgs&&... args) {
    return bindr_helper_t<Res, Func, BoundArgs...>(
            _MSTL forward<Func>(func),
            _MSTL forward<BoundArgs>(args)...
        );
}


template <typename Func, typename... BoundArgs>
struct binder_front {
    static_assert(is_move_constructible<Func>::value, "Func should be move constructible");
#ifdef MSTL_STANDARD_17__
    static_assert((is_move_constructible_v<BoundArgs> && ...), "Args should be move constructible");
#endif

private:
    using BoundIndices = index_sequence_for<BoundArgs...>;

    Func func_;
    tuple<BoundArgs...> bound_args_;

private:
    template <typename T, size_t... Indices, typename... CallArgs>
    static constexpr decltype(auto)
    call(T&& bind_object, index_sequence<Indices...>, CallArgs&&... call_args) {
        return _MSTL invoke(_MSTL forward<T>(bind_object).func_,
            _MSTL get<Indices>(_MSTL forward<T>(bind_object).bound_args_)...,
            _MSTL forward<CallArgs>(call_args)...);
    }

public:
    template <typename Fn, typename... Args>
    explicit constexpr
    binder_front(int, Fn&& func, Args&&... args)
    noexcept(conjunction<
        is_nothrow_constructible<Func, Fn>,
        is_nothrow_constructible<BoundArgs, Args>...>::value)
    : func_(_MSTL forward<Fn>(func)), bound_args_(_MSTL forward<Args>(args)...) {
        static_assert(sizeof...(Args) == sizeof...(BoundArgs), "Wrong number of arguments");
    }

    binder_front(const binder_front&) = default;
    binder_front& operator =(const binder_front&) = default;
    binder_front(binder_front&&) = default;
    binder_front& operator =(binder_front&&) = default;
    ~binder_front() = default;

    template <typename... CallArgs>
    constexpr invoke_result_t<Func&, BoundArgs&..., CallArgs...>
    operator()(CallArgs&&... call_args) &
    noexcept(is_nothrow_invocable_v<Func&, BoundArgs&..., CallArgs...>) {
        return binder_front::call(*this, BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }

    template <typename... CallArgs>
    constexpr invoke_result_t<const Func&, const BoundArgs&..., CallArgs...>
    operator()(CallArgs&&... call_args) const &
    noexcept(is_nothrow_invocable_v<const Func&, const BoundArgs&..., CallArgs...>) {
        return binder_front::call(*this, BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }

    template <typename... CallArgs>
    constexpr invoke_result_t<Func, BoundArgs..., CallArgs...>
    operator()(CallArgs&&... call_args) &&
    noexcept(is_nothrow_invocable_v<Func, BoundArgs..., CallArgs...>) {
        return binder_front::call(_MSTL move(*this), BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }

    template <typename... CallArgs>
    constexpr invoke_result_t<const Func, const BoundArgs..., CallArgs...>
    operator()(CallArgs&&... call_args) const &&
    noexcept(is_nothrow_invocable_v<const Func, const BoundArgs..., CallArgs...>) {
        return binder_front::call(_MSTL move(*this), BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }
};

template <typename Func, typename... Args>
using binder_front_type = binder_front<decay_t<Func>, decay_t<Args>...>;

template <typename Func, typename... Args>
MSTL_NODISCARD constexpr binder_front_type<Func, Args...>
bind_front(Func&& func, Args&&... args)
noexcept(is_nothrow_constructible<binder_front_type<Func, Args...>, int, Func, Args...>::value) {
    return binder_front_type<Func, Args...>(0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_BIND_HPP__
