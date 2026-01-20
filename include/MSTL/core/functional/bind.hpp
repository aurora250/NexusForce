#ifndef MSTL_CORE_FUNCTIONAL_BIND_HPP__
#define MSTL_CORE_FUNCTIONAL_BIND_HPP__

/**
 * @file bind.hpp
 * @brief MSTL函数绑定工具
 *
 * 此文件提供了函数绑定相关的实现，用于创建可调用对象的适配器，支持参数绑定和占位符。
 */

#include "invoke.hpp"
MSTL_BEGIN_NAMESPACE__

/// @cond
MSTL_BEGIN_INNER__

template <typename Res, typename... Args>
struct unary_or_binary_function {};

template <typename Res, typename T1>
struct unary_or_binary_function<Res, T1>
    : _MSTL unary_function<T1, Res> {};

template <typename Res, typename T1, typename T2>
struct unary_or_binary_function<Res, T1, T2>
    : _MSTL binary_function<T1, T2, Res> {};


/**
 * @brief 成员函数特性萃取
 * @tparam Sign 成员函数签名类型
 */
template <typename Sign>
struct mem_func_traits;

template <typename Res, typename Class, typename... Args>
struct mem_func_traits_base {
    using result_type = Res;                                            ///< 返回值类型
    using maybe_type = unary_or_binary_function<Res, Class*, Args...>;  ///< 可能的基类类型
    using arity = integral_constant<size_t, sizeof...(Args)>;           ///< 参数数量
};

#define __MSTL_MEMFUNC_TRAITS_BASE(CV, REF) \
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

#define MSTL_MEMFUNC_TRAITS(REF, P) \
__MSTL_MEMFUNC_TRAITS_BASE(, REF) \
__MSTL_MEMFUNC_TRAITS_BASE(const, REF) \
__MSTL_MEMFUNC_TRAITS_BASE(volatile, REF) \
__MSTL_MEMFUNC_TRAITS_BASE(const volatile, REF)

MSTL_MEMFUNC_TRAITS(,)
MSTL_MEMFUNC_TRAITS(&,)
MSTL_MEMFUNC_TRAITS(&&,)

#ifdef MSTL_STANDARD_17__
MSTL_MEMFUNC_TRAITS(noexcept,)
MSTL_MEMFUNC_TRAITS(& noexcept,)
MSTL_MEMFUNC_TRAITS(&& noexcept,)
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

/**
 * @struct weak_result_type
 * @brief 弱结果类型萃取器
 * @tparam Func 函数类型
 *
 * 提取函数的结果类型，支持函数指针、成员函数指针和函数对象。
 */
template <typename Func>
struct weak_result_type : __weak_result_type_memfun<remove_cv_t<Func>> {};


/**
 * @class mem_func_base
 * @brief 成员函数包装器基类
 * @tparam MemberPtr 成员指针类型
 * @tparam IsMemFunc 是否为成员函数指针
 */
template <typename MemberPtr, bool IsMemFunc = is_member_function_pointer_v<MemberPtr>>
class mem_func_base : public _INNER mem_func_traits<MemberPtr>::maybe_type {
    using Traits = _INNER mem_func_traits<MemberPtr>;  ///< 特性类型
    using Arity = typename Traits::arity;              ///< 参数数量
    using Varargs = typename Traits::vararg;           ///< 是否可变参数

    template <typename Func, typename... BoundArgs>
    friend struct bind_check_arity;

    MemberPtr ptr_;  ///< 成员指针

public:
    using result_type = typename Traits::result_type;

    /**
     * @brief 构造函数
     * @param pmf 成员函数指针
     */
    explicit constexpr mem_func_base(MemberPtr pmf) noexcept : ptr_(pmf) {}

    /**
     * @brief 调用操作符
     * @tparam Args 参数类型
     * @param args 参数
     * @return 调用结果
     */
    template <typename... Args>
    MSTL_CONSTEXPR20 auto operator ()(Args&&... args) const
    noexcept(noexcept(_MSTL invoke(ptr_, _MSTL forward<Args>(args)...)))
    -> decltype(_MSTL invoke(ptr_, _MSTL forward<Args>(args)...)) {
        return _MSTL invoke(ptr_, _MSTL forward<Args>(args)...);
    }
};

/**
 * @brief 成员对象包装器特化
 */
template <typename MemberObjPtr>
class mem_func_base<MemberObjPtr, false> {
    using Arity = integral_constant<size_t, 0>;
    using Varargs = false_type;

    template <typename Func, typename... BoundArgs>
    friend struct bind_check_arity;

    MemberObjPtr ptr_;

public:
    /**
     * @brief 构造函数
     * @param pm 成员对象指针
     */
    explicit constexpr mem_func_base(MemberObjPtr pm) noexcept : ptr_(pm) {}

    /**
     * @brief 调用操作符
     * @tparam T 对象类型
     * @param obj 对象
     * @return 成员访问结果
     */
    template <typename T>
    MSTL_CONSTEXPR20 auto operator ()(T&& obj) const
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
/// @endcond


/**
 * @defgroup BindTraits 绑定特性
 * @brief 绑定表达式相关的类型特性
 * @{
 */

/**
 * @struct is_bind_expression
 * @brief 判断是否为绑定表达式
 * @tparam T 待判断类型
 */
template <typename T>
struct is_bind_expression : false_type {};

/**
 * @var is_bind_expression_v
 * @brief is_bind_expression的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_bind_expression_v = is_bind_expression<T>::value;


/**
 * @struct placeholder
 * @brief 占位符类型
 * @tparam Num 占位符编号
 */
template <uint32_t Num>
struct placeholder : uint32_constant<Num> {};

/**
 * @var placeholder_v
 * @brief placeholder值的便捷变量模板
 */
template <uint32_t Num>
MSTL_INLINE17 constexpr uint32_t placeholder_v = placeholder<Num>::value;


/**
 * @struct is_placeholder
 * @brief 判断是否为占位符
 * @tparam T 待检查类型
 */
template <typename T>
struct is_placeholder;

template <uint32_t Num>
struct is_placeholder<placeholder<Num>> : true_type {};

/**
 * @var is_placeholder_v
 * @brief is_placeholder的便捷变量模板
 */
template <typename T>
MSTL_INLINE17 constexpr bool is_placeholder_v = is_placeholder<T>::value;

/**
 * @namespace placeholders
 * @brief 占位符预定义实例命名空间
 *
 * 提供预定义的占位符对象p1到p29，用于bind表达式。
 */
namespace placeholders {
    MSTL_INLINE17 constexpr placeholder<1>  p1{};   ///< 占位符1
    MSTL_INLINE17 constexpr placeholder<2>  p2{};   ///< 占位符2
    MSTL_INLINE17 constexpr placeholder<3>  p3{};   ///< 占位符3
    MSTL_INLINE17 constexpr placeholder<4>  p4{};   ///< 占位符4
    MSTL_INLINE17 constexpr placeholder<5>  p5{};   ///< 占位符5
    MSTL_INLINE17 constexpr placeholder<6>  p6{};   ///< 占位符6
    MSTL_INLINE17 constexpr placeholder<7>  p7{};   ///< 占位符7
    MSTL_INLINE17 constexpr placeholder<8>  p8{};   ///< 占位符8
    MSTL_INLINE17 constexpr placeholder<9>  p9{};   ///< 占位符9
    MSTL_INLINE17 constexpr placeholder<10> p10{};  ///< 占位符10
    MSTL_INLINE17 constexpr placeholder<11> p11{};  ///< 占位符11
    MSTL_INLINE17 constexpr placeholder<12> p12{};  ///< 占位符12
    MSTL_INLINE17 constexpr placeholder<13> p13{};  ///< 占位符13
    MSTL_INLINE17 constexpr placeholder<14> p14{};  ///< 占位符14
    MSTL_INLINE17 constexpr placeholder<15> p15{};  ///< 占位符15
    MSTL_INLINE17 constexpr placeholder<16> p16{};  ///< 占位符16
    MSTL_INLINE17 constexpr placeholder<17> p17{};  ///< 占位符17
    MSTL_INLINE17 constexpr placeholder<18> p18{};  ///< 占位符18
    MSTL_INLINE17 constexpr placeholder<19> p19{};  ///< 占位符19
    MSTL_INLINE17 constexpr placeholder<20> p20{};  ///< 占位符20
    MSTL_INLINE17 constexpr placeholder<21> p21{};  ///< 占位符21
    MSTL_INLINE17 constexpr placeholder<22> p22{};  ///< 占位符22
    MSTL_INLINE17 constexpr placeholder<23> p23{};  ///< 占位符23
    MSTL_INLINE17 constexpr placeholder<24> p24{};  ///< 占位符24
    MSTL_INLINE17 constexpr placeholder<25> p25{};  ///< 占位符25
    MSTL_INLINE17 constexpr placeholder<26> p26{};  ///< 占位符26
    MSTL_INLINE17 constexpr placeholder<27> p27{};  ///< 占位符27
    MSTL_INLINE17 constexpr placeholder<28> p28{};  ///< 占位符28
    MSTL_INLINE17 constexpr placeholder<29> p29{};  ///< 占位符29
}


/// @cond
MSTL_BEGIN_INNER__

/**
 * @class bind_arg_mapper
 * @brief 参数映射器模板
 * @tparam Arg 参数类型
 * @tparam IsBindExp 是否为bind表达式
 * @tparam IsPlaceholder 是否为占位符
 *
 * 根据参数类型的不同特性，提供不同的参数映射策略：
 * 1. reference_wrapper：解引用获取底层引用
 * 2. bind表达式：递归调用该bind表达式
 * 3. 占位符：从参数元组中提取对应位置的参数
 * 4. 普通参数：直接转发
 */
template <typename Arg,
    bool IsBindExp = is_bind_expression_v<Arg>,
    bool IsPlaceholder = is_placeholder_v<Arg>>
class bind_arg_mapper;

template <typename T>
class bind_arg_mapper<reference_wrapper<T>, false, false> {
public:
    template <typename CVRef, typename Tuple>
    MSTL_CONSTEXPR20 T& operator ()(CVRef& arg, Tuple&) const volatile {
        return arg.get();
    }
};

template <typename Arg>
class bind_arg_mapper<Arg, true, false> {
public:
    template <typename CVArg, typename... Args>
    MSTL_CONSTEXPR20 auto operator ()(CVArg& arg, tuple<Args...>& tuple_ref) const volatile
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


template <typename Arg>
class bind_arg_mapper<Arg, false, true> {
    template <size_t I, typename Tuple>
    using safe_tuple_element_t = enable_if_t<
        (I < tuple_size_v<Tuple>),
        tuple_element_t<I, Tuple>>;

public:
    template <typename Tuple>
    MSTL_CONSTEXPR20 safe_tuple_element_t<(is_placeholder_v<Arg> - 1), Tuple>&&
    operator ()(const volatile Arg&, Tuple& tuple_ref) const volatile {
        return _MSTL get<(is_placeholder<Arg>::value - 1)>(_MSTL move(tuple_ref));
    }
};

template <typename Arg>
class bind_arg_mapper<Arg, false, false> {
public:
    template <typename CVArg, typename Tuple>
    MSTL_CONSTEXPR20 CVArg&& operator ()(CVArg&& arg, Tuple&) const volatile {
        return _MSTL forward<CVArg>(arg);
    }
};

MSTL_END_INNER__
/// @endcond

/** @} */ // BindTraits

/**
 * @defgroup FunctionBinders 绑定函数
 * @brief 函数绑定和部分应用的工具
 * @{
 */

/**
 * @class binder
 * @brief 通用函数绑定器
 * @tparam Sign 函数签名模板参数
 *
 * 实现通用的函数绑定功能，支持任意可调用对象和参数绑定。
 * 提供占位符参数重排、嵌套绑定等功能。
 */
template <typename Sign>
class binder;

/**
 * @brief 通用函数绑定器的部分特化
 * @tparam Func 可调用对象类型
 * @tparam BoundArgs 绑定的参数类型
 *
 * 存储函数对象和绑定的参数，当被调用时，将绑定的参数和调用时的参数
 * 组合后调用原始函数。支持占位符参数重排。
 */
template <typename Func, typename... BoundArgs>
class binder<Func(BoundArgs...)> : public _INNER weak_result_type<Func> {
private:
    using BoundIndexes = build_index_tuple_t<sizeof...(BoundArgs)>;  ///< 绑定参数的索引序列

    Func functor_;                          ///< 存储的函数对象
    tuple<BoundArgs...> bound_args_;        ///< 存储的绑定参数

private:
    /**
     * @struct arg_mapper_result
     * @brief 参数映射结果类型计算器
     * @tparam BoundArg 绑定参数类型
     * @tparam CallArgs 调用参数元组类型
     */
    template <typename BoundArg, typename CallArgs>
    struct arg_mapper_result {
        using type = decltype(_INNER bind_arg_mapper<remove_cv_t<BoundArg>>()(
            _MSTL declval<BoundArg&>(), _MSTL declval<CallArgs&>()));
    };

    /**
     * @typedef arg_mapper_result_t
     * @brief arg_mapper_result的便捷别名
     */
    template <typename BoundArg, typename CallArgs>
    using arg_mapper_result_t = typename arg_mapper_result<BoundArg, CallArgs>::type;

    /**
     * @typedef result_type
     * @brief 调用结果的类型
     * @tparam CallArgs 调用参数元组类型
     */
    template <typename CallArgs>
    using result_type = invoke_result_t<
        Func&,
        arg_mapper_result_t<BoundArgs, CallArgs>...
    >;

    /**
     * @typedef result_type_const
     * @brief const调用结果的类型
     * @tparam CallArgs 调用参数元组类型
     */
    template <typename CallArgs>
    using result_type_const = invoke_result_t<
        const Func&,
        arg_mapper_result_t<const BoundArgs, CallArgs>...
    >;

    /**
     * @typedef arg_mapper_type
     * @brief 参数映射器的类型
     * @tparam BoundArg 绑定参数类型
     * @tparam CallArgs 调用参数元组类型
     */
    template <typename BoundArg, typename CallArgs>
    using arg_mapper_type = decltype(_INNER bind_arg_mapper<remove_cv_t<BoundArg>>()(
        _MSTL declval<BoundArg&>(), _MSTL declval<CallArgs&>()));

    /**
     * @typedef dependent
     * @brief 依赖类型，用于SFINAE
     * @tparam CallArgs 调用参数元组类型
     */
    template <typename CallArgs>
    using dependent = enable_if_t<static_cast<bool>(tuple_size_v<CallArgs> + 1), Func>;

private:
    /**
     * @brief 调用辅助函数
     * @tparam Res 返回类型
     * @tparam Args 调用参数类型
     * @tparam Indexes 索引序列
     * @param args 调用参数元组
     * @param idx 索引序列对象
     * @return 调用结果
     */
    template <typename Res, typename... Args, size_t... Indexes>
    MSTL_CONSTEXPR20 Res call(tuple<Args...>&& args, index_tuple<Indexes...> idx) {
        return _MSTL invoke(functor_,
            _INNER bind_arg_mapper<BoundArgs>()(_MSTL get<Indexes>(bound_args_), args)...
        );
    }

    /**
     * @brief const调用辅助函数
     * @tparam Res 返回类型
     * @tparam Args 调用参数类型
     * @tparam Indexes 索引序列
     * @param args 调用参数元组
     * @param idx 索引序列对象
     * @return 调用结果
     */
    template <typename Res, typename... Args, _MSTL size_t... Indexes>
    MSTL_CONSTEXPR20 Res call_const(tuple<Args...>&& args, index_tuple<Indexes...> idx) const {
        return _MSTL invoke(functor_,
            _INNER bind_arg_mapper<BoundArgs>()(_MSTL get<Indexes>(bound_args_), args)...
        );
    }

public:
    /**
     * @brief 构造函数
     * @tparam Args 绑定参数类型
     * @param func 要绑定的函数对象
     * @param args 绑定的参数
     */
    template <typename... Args>
    explicit MSTL_CONSTEXPR20 binder(Func&& func, Args&&... args)
    : functor_(_MSTL forward<Func>(func)), bound_args_(_MSTL forward<Args>(args)...) {}

    binder(const binder&) = default;  ///< 复制构造函数
    binder(binder&&) = default;       ///< 移动构造函数

    /**
     * @brief 调用操作符
     * @tparam Args 调用参数类型
     * @param args 调用参数
     * @return 函数调用结果
     */
    template <typename... Args>
    MSTL_CONSTEXPR20 auto operator ()(Args&&... args)
    -> result_type<tuple<Args&&...>> {
        using Res = result_type<tuple<Args&&...>>;
        return binder::call<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    /**
     * @brief const调用操作符
     * @tparam Args 调用参数类型
     * @param args 调用参数
     * @return 函数调用结果
     */
    template <typename... Args>
    MSTL_CONSTEXPR20 auto operator ()(Args&&... args) const
    -> result_type_const<tuple<Args&&...>> {
        using Res = result_type_const<tuple<Args&&...>>;
        return binder::call_const<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    template <typename... Args>
    void operator ()(Args&&... args) const volatile = delete;
};


/**
 * @class bindrer
 * @brief 指定返回类型的函数绑定器
 * @tparam Res 指定的返回类型
 * @tparam Sign 函数签名模板参数
 *
 * 与binder类似，用于强制类型转换。
 */
template <typename Res, typename Sign>
class bindrer;

/**
 * @brief 指定返回类型的函数绑定器部分特化
 * @tparam Res 指定的返回类型
 * @tparam Func 可调用对象类型
 * @tparam BoundArgs 绑定的参数类型
 */
template <typename Res, typename Func, typename... BoundArgs>
class bindrer<Res, Func(BoundArgs...)> {
private:
    using BoundIndexes = build_index_tuple_t<sizeof...(BoundArgs)>;  ///< 绑定参数的索引序列

    Func functor_;                          ///< 存储的函数对象
    tuple<BoundArgs...> bound_args_;        ///< 存储的绑定参数

private:
    /**
     * @brief 调用辅助函数
     * @tparam Result 返回类型
     * @tparam Args 调用参数类型
     * @tparam Indexes 索引序列
     * @param args 调用参数元组
     * @param idx 索引序列对象
     * @return 调用结果
     */
    template <typename Result, typename... Args, size_t... Indexes>
    MSTL_CONSTEXPR20 Result call(tuple<Args...>&& args, index_tuple<Indexes...> idx) {
        return _MSTL invoke_r<Res>(functor_, _INNER bind_arg_mapper<BoundArgs>()(
            _MSTL get<Indexes>(bound_args_), args)...);
    }

    /**
     * @brief const调用辅助函数
     * @tparam Result 返回类型
     * @tparam Args 调用参数类型
     * @tparam Indexes 索引序列
     * @param args 调用参数元组
     * @param idx 索引序列对象
     * @return 调用结果
     */
    template <typename Result, typename... Args, _MSTL size_t... Indexes>
    MSTL_CONSTEXPR20 Result call(tuple<Args...>&& args, index_tuple<Indexes...> idx) const {
        return _MSTL invoke_r<Res>(functor_, _INNER bind_arg_mapper<BoundArgs>()(
            _MSTL get<Indexes>(bound_args_), args)...);
    }

public:
    using result_type = Res;  ///< 指定的返回类型

    /**
     * @brief 构造函数
     * @tparam Args 绑定参数类型
     * @param func 要绑定的函数对象
     * @param args 绑定的参数
     */
    template <typename... Args>
    explicit MSTL_CONSTEXPR20 bindrer(Func&& func, Args&&... args)
    : functor_(_MSTL forward<Func>(func)), bound_args_(_MSTL forward<Args>(args)...) {}

    bindrer(const bindrer&) = default;  ///< 复制构造函数
    bindrer(bindrer&&) = default;       ///< 移动构造函数

    /**
     * @brief 调用操作符
     * @tparam Args 调用参数类型
     * @param args 调用参数
     * @return 转换为Res类型的函数调用结果
     */
    template <typename... Args>
    MSTL_CONSTEXPR20 result_type operator ()(Args&&... args) {
        return bindrer::call<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    /**
     * @brief const调用操作符
     * @tparam Args 调用参数类型
     * @param args 调用参数
     * @return 转换为Res类型的函数调用结果
     */
    template <typename... Args>
    MSTL_CONSTEXPR20 result_type operator ()(Args&&... args) const {
        return bindrer::call<Res>(
            _MSTL forward_as_tuple(_MSTL forward<Args>(args)...),
            BoundIndexes());
    }

    template <typename... Args>
    void operator ()(Args&&... args) const volatile = delete;
};


template <typename Sign>
struct is_bind_expression<binder<Sign>> : true_type {};
template <typename Res, typename Sign>
struct is_bind_expression<bindrer<Res, Sign>> : true_type {};

#define __MSTL_EXPAND_BIND_EXP(CV) \
template <typename Sign> \
struct is_bind_expression<CV binder<Sign>> : true_type {}; \
template <typename Res, typename Sign> \
struct is_bind_expression<CV bindrer<Res, Sign>> : true_type {};

MSTL_MACRO_RANGES_CV(__MSTL_EXPAND_BIND_EXP)
#undef __MSTL_EXPAND_BIND_EXP

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct bind_check_arity
 * @brief 绑定参数数量检查器
 * @tparam Func 函数类型
 * @tparam BoundArgs 绑定参数类型
 *
 * 检查绑定参数数量是否与函数期望的参数数量匹配。
 */
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
/// @endcond

/**
 * @struct bind_helper
 * @brief bind辅助类型推导器
 * @tparam IntLike 是否为整数类型
 * @tparam Func 函数类型
 * @tparam BoundArgs 绑定参数类型
 */
template <bool IntLike, typename Func, typename... BoundArgs>
struct bind_helper
    : _INNER bind_check_arity<decay_t<Func>, BoundArgs...> {
    using func_type = decay_t<Func>;  ///< 函数类型
    using type = binder<func_type(decay_t<BoundArgs>...)>;  ///< 推导出的binder类型
};

template <typename Func, typename... BoundArgs>
struct bind_helper<true, Func, BoundArgs...> {};

/**
 * @typedef bind_helper_t
 * @brief bind_helper的便捷别名
 */
template <bool IntLike, typename Func, typename... BoundArgs>
using bind_helper_t = typename bind_helper<IntLike, Func, BoundArgs...>::type;


/**
 * @brief bind函数
 * @tparam Func 可调用对象类型
 * @tparam BoundArgs 绑定的参数类型
 * @param func 要绑定的函数对象
 * @param args 绑定的参数
 * @return 绑定后的函数对象
 *
 * 创建函数绑定器，支持占位符参数重排。返回的绑定器可以存储并稍后调用。
 *
 * @deprecated 此函数已被标记为弃用，建议使用lambda表达式或bind_front替代。
 */
template <typename Func, typename... BoundArgs>
MSTL_DEPRECATE_FOR("use lambda or bind_front instead of bind")
MSTL_NODISCARD constexpr bind_helper_t<is_integral_like<Func>::value, Func, BoundArgs...>
bind(Func&& func, BoundArgs&&... args) {
    return bind_helper_t<false, Func, BoundArgs...>(
            _MSTL forward<Func>(func),
            _MSTL forward<BoundArgs>(args)...
        );
}


/**
 * @struct bindr_helper
 * @brief 指定返回类型的bind辅助类型推导器
 * @tparam Res 指定的返回类型
 * @tparam Func 函数类型
 * @tparam BoundArgs 绑定参数类型
 */
template <typename Res, typename Func, typename... BoundArgs>
struct bindr_helper
    : _INNER bind_check_arity<decay_t<Func>, BoundArgs...> {
    using type = bindrer<Res, decay_t<Func>(decay_t<BoundArgs>...)>;
};

/**
 * @typedef bindr_helper_t
 * @brief bindr_helper的便捷别名
 */
template <typename Res, typename Func, typename... BoundArgs>
using bindr_helper_t = typename bindr_helper<Res, Func, BoundArgs...>::type;


/**
 * @brief 指定返回类型的bind函数
 * @tparam Res 指定的返回类型
 * @tparam Func 可调用对象类型
 * @tparam BoundArgs 绑定的参数类型
 * @param func 要绑定的函数对象
 * @param args 绑定的参数
 * @return 绑定后的函数对象
 *
 * 创建指定返回类型的函数绑定器，调用时会强制将结果转换为Res类型。
 */
template <typename Res, typename Func, typename... BoundArgs>
MSTL_DEPRECATE_FOR("use lambda or bind_front instead of bind")
MSTL_NODISCARD constexpr bindr_helper_t<Res, Func, BoundArgs...>
bind(Func&& func, BoundArgs&&... args) {
    return bindr_helper_t<Res, Func, BoundArgs...>(
            _MSTL forward<Func>(func),
            _MSTL forward<BoundArgs>(args)...
        );
}


/**
 * @class binder_front
 * @brief 前向参数绑定器
 * @tparam Func 可调用对象类型
 * @tparam BoundArgs 绑定的参数类型
 *
 * 将参数绑定到函数的前几个位置，不支持占位符参数重排。
 */
template <typename Func, typename... BoundArgs>
struct binder_front {
    static_assert(is_move_constructible<Func>::value, "Func should be move constructible");
#ifdef MSTL_STANDARD_17__
    static_assert((is_move_constructible_v<BoundArgs> && ...), "Args should be move constructible");
#endif

private:
    using BoundIndices = index_sequence_for<BoundArgs...>;  ///< 绑定参数的索引序列

    Func func_;                            ///< 存储的函数对象
    tuple<BoundArgs...> bound_args_;       ///< 存储的绑定参数

private:
    /**
     * @brief 调用辅助函数
     * @tparam T binder_front对象的类型
     * @tparam Indices 索引序列
     * @tparam CallArgs 调用参数类型
     * @param bind_object binder_front对象
     * @param idx 索引序列对象
     * @param call_args 调用参数
     * @return 函数调用结果
     */
    template <typename T, size_t... Indices, typename... CallArgs>
    static constexpr decltype(auto)
    call(T&& bind_object, index_sequence<Indices...> idx, CallArgs&&... call_args) {
        return _MSTL invoke(_MSTL forward<T>(bind_object).func_,
            _MSTL get<Indices>(_MSTL forward<T>(bind_object).bound_args_)...,
            _MSTL forward<CallArgs>(call_args)...);
    }

public:
    /**
     * @brief 构造函数
     * @tparam Fn 函数类型
     * @tparam Args 绑定参数类型
     * @param p 占位符参数
     * @param func 要绑定的函数对象
     * @param args 绑定的参数
     */
    template <typename Fn, typename... Args>
    explicit constexpr
    binder_front(int p, Fn&& func, Args&&... args)
    noexcept(conjunction<
        is_nothrow_constructible<Func, Fn>,
        is_nothrow_constructible<BoundArgs, Args>...>::value)
    : func_(_MSTL forward<Fn>(func)), bound_args_(_MSTL forward<Args>(args)...) {
        static_assert(sizeof...(Args) == sizeof...(BoundArgs), "Wrong number of arguments");
    }

    binder_front(const binder_front&) = default;              ///< 复制构造函数
    binder_front& operator =(const binder_front&) = default;  ///< 复制赋值运算符
    binder_front(binder_front&&) = default;                   ///< 移动构造函数
    binder_front& operator =(binder_front&&) = default;       ///< 移动赋值运算符

    ~binder_front() = default;  ///< 析构函数

    /**
     * @brief 左值调用操作符
     * @tparam CallArgs 调用参数类型
     * @param call_args 调用参数
     * @return 函数调用结果
     */
    template <typename... CallArgs>
    constexpr invoke_result_t<Func&, BoundArgs&..., CallArgs...>
    operator ()(CallArgs&&... call_args) &
    noexcept(is_nothrow_invocable_v<Func&, BoundArgs&..., CallArgs...>) {
        return binder_front::call(*this, BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }

    /**
     * @brief const左值调用操作符
     * @tparam CallArgs 调用参数类型
     * @param call_args 调用参数
     * @return 函数调用结果
     */
    template <typename... CallArgs>
    constexpr invoke_result_t<const Func&, const BoundArgs&..., CallArgs...>
    operator ()(CallArgs&&... call_args) const &
    noexcept(is_nothrow_invocable_v<const Func&, const BoundArgs&..., CallArgs...>) {
        return binder_front::call(*this, BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }

    /**
     * @brief 右值调用操作符
     * @tparam CallArgs 调用参数类型
     * @param call_args 调用参数
     * @return 函数调用结果
     */
    template <typename... CallArgs>
    constexpr invoke_result_t<Func, BoundArgs..., CallArgs...>
    operator ()(CallArgs&&... call_args) &&
    noexcept(is_nothrow_invocable_v<Func, BoundArgs..., CallArgs...>) {
        return binder_front::call(_MSTL move(*this), BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }

    /**
     * @brief const右值调用操作符
     * @tparam CallArgs 调用参数类型
     * @param call_args 调用参数
     * @return 函数调用结果
     */
    template <typename... CallArgs>
    constexpr invoke_result_t<const Func, const BoundArgs..., CallArgs...>
    operator ()(CallArgs&&... call_args) const &&
    noexcept(is_nothrow_invocable_v<const Func, const BoundArgs..., CallArgs...>) {
        return binder_front::call(_MSTL move(*this), BoundIndices(), _MSTL forward<CallArgs>(call_args)...);
    }
};

/**
 * @typedef binder_front_type
 * @brief binder_front类型的便捷别名
 * @tparam Func 函数类型
 * @tparam Args 绑定参数类型
 */
template <typename Func, typename... Args>
using binder_front_type = binder_front<decay_t<Func>, decay_t<Args>...>;

/**
 * @brief bind_front函数
 * @tparam Func 可调用对象类型
 * @tparam Args 绑定的参数类型
 * @param func 要绑定的函数对象
 * @param args 绑定的参数
 * @return 前向绑定后的函数对象
 *
 * 创建前向参数绑定器，将参数绑定到函数的前几个位置。
 */
template <typename Func, typename... Args>
MSTL_NODISCARD constexpr binder_front_type<Func, Args...>
bind_front(Func&& func, Args&&... args)
noexcept(is_nothrow_constructible<binder_front_type<Func, Args...>, int, Func, Args...>::value) {
    return binder_front_type<Func, Args...>(0, _MSTL forward<Func>(func), _MSTL forward<Args>(args)...);
}

/** @} */ // FunctionBinders

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_BIND_HPP__
