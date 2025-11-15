#ifndef MSTL_FUNCTOR_HPP__
#define MSTL_FUNCTOR_HPP__
#include "type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename Arg, typename Result>
struct MSTL_FUNC_ADAPTER_DEPRECATE unary_function {
	using argument_type = Arg;
	using result_type	= Result;
};

template <typename Arg1, typename Arg2, typename Result>
struct MSTL_FUNC_ADAPTER_DEPRECATE binary_function {
	using first_argument_type	= Arg1;
	using second_argument_type	= Arg2;
	using result_type			= Result;
};


template <typename T = void>
struct plus {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x + y))) {
		return x + y;
	}
};

template <>
struct plus<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
	noexcept(noexcept(static_cast<T1&&>(x) + static_cast<T2&&>(y))){
		return static_cast<T1&&>(x) + static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct minus {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x - y))) {
		return x - y;
	}
};

template <>
struct minus<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) - static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) - static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct multiplies {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x * y))) {
		return x * y;
	}
};

template <>
struct multiplies<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) * static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) * static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct divides {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x / y))) {
		return x / y;
	}
};

template <>
struct divides<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) / static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) / static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct modulus {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= T;

	MSTL_NODISCARD constexpr T operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<T>(x % y))) {
		return x % y;
	}
};

template <>
struct modulus<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) % static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) % static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct negate {
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;

	MSTL_NODISCARD constexpr T operator()(const T& x) const
		noexcept(noexcept(_MSTL declcopy<T>(-x))) {
		return -x;
	}
};

template <>
struct negate<void> {
	template <typename T1>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x) const
		noexcept(noexcept(-static_cast<T1&&>(x))) {
		return -static_cast<T1&&>(x);
	}
};


template <typename T = void>
struct equal_to {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x == y))) {
		return x == y;
	}
};

template <>
struct equal_to<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) == static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) == static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct not_equal_to {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x != y))) {
		return x != y;
	}
};

template <>
struct not_equal_to<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) != static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) != static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct greater {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x > y))) {
		return x > y;
	}
};

template <>
struct greater<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) > static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) > static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct less {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x < y))) {
		return x < y;
	}
};

template <>
struct less<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) < static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) < static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct greater_equal {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x >= y))) {
		return x >= y;
	}
};

template <>
struct greater_equal<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) >= static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) >= static_cast<T2&&>(y);
	}
};

template <typename T = void>
struct less_equal {
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	MSTL_NODISCARD constexpr bool operator()(const T& x, const T& y) const
		noexcept(noexcept(_MSTL declcopy<bool>(x <= y))) {
		return x <= y;
	}
};

template <>
struct less_equal<void> {
	template <typename T1, typename T2>
	MSTL_NODISCARD constexpr decltype(auto) operator()(T1&& x, T2&& y) const
		noexcept(noexcept(static_cast<T1&&>(x) <= static_cast<T2&&>(y))) {
		return static_cast<T1&&>(x) <= static_cast<T2&&>(y);
	}
};


template <typename T>
struct identity {
	template <typename U = T>
	MSTL_NODISCARD constexpr U&& operator()(U&& x) const noexcept {
		return _MSTL forward<U>(x);
	}
};


template <typename Pair>
struct select1st {
#ifdef MSTL_STANDARD_20__
	static_assert(is_pair_v<Pair>, "select1st requires pair type.");
#endif // MSTL_STANDARD_20__

	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Pair;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Pair::first_type;

	MSTL_NODISCARD constexpr const typename Pair::first_type&
		operator()(const Pair& x) const noexcept {
		return x.first;
	}
};

template <typename Pair>
struct select2nd {
#ifdef MSTL_STANDARD_20__
	static_assert(is_pair_v<Pair>, "select2nd requires pair type.");
#endif // MSTL_STANDARD_20__

	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Pair;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Pair::second_type;

	MSTL_NODISCARD constexpr const typename Pair::second_type&
		operator()(const Pair& x) const noexcept {
		return x.second;
	}
};


#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(MSTL_COMPILER_GCC__)
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(MSTL_COMPILER_MSVC__)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

template <typename Predicate>
class MSTL_FUNC_ADAPTER_DEPRECATE unary_negate {
protected:
	Predicate pred;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Predicate::argument_type;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= bool;

	constexpr explicit unary_negate(const Predicate& x) : pred(x) {}
	constexpr bool operator ()(const typename Predicate::argument_type& x) const {
		return !pred(x);
	}
};
template <typename Predicate>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr unary_negate<Predicate> not1(const Predicate& pred) {
	return unary_negate<Predicate>(pred);
}

template <typename Predicate>
class MSTL_FUNC_ADAPTER_DEPRECATE binary_negate {
protected:
	Predicate pred;

public:
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Predicate::first_argument_type;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Predicate::second_argument_type;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= bool;

	constexpr explicit binary_negate(const Predicate& x) : pred(x) {}
	constexpr bool operator ()(const typename Predicate::first_argument_type& x,
		const typename Predicate::second_argument_type& y) const {
		return !pred(x, y);
	}
};
template <typename Predicate>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr binary_negate<Predicate> not2(const Predicate& pred) {
	return binary_negate<Predicate>(pred);
}

template <typename Operation>
class MSTL_FUNC_ADAPTER_DEPRECATE binder1st {
protected:
	Operation op;
	typename Operation::first_argument_type value;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation::second_argument_type;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation::result_type;

	constexpr explicit binder1st(
		const Operation x, const typename Operation::first_argument_type& y) : op(x), value(y) {
	}
	constexpr typename Operation::result_type operator ()(
		const typename Operation::second_argument_type& x) {
		return op(value, x);
	}
};
template <typename Operation, typename T>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr binder1st<Operation> bind1st(const Operation& op, const T& x) {
	return binder1st<Operation>(op, typename Operation::first_argument_type(x));
}

template <typename Operation>
class MSTL_FUNC_ADAPTER_DEPRECATE binder2nd {
protected:
	Operation op;
	typename Operation::second_argument_type value;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation::first_argument_type;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation::result_type;

	constexpr explicit binder2nd(
		const Operation& x, const typename Operation::second_argument_type& y) : op(x), value(y) {
	}
	constexpr typename Operation::result_type operator()(
		const typename Operation::first_argument_type& x) const {
		return op(x, value);
	}
};
template <typename Operation, typename T>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr binder2nd<Operation> bind2nd(const Operation& op, const T& x) {
	return binder2nd<Operation>(op, typename Operation::second_argument_type(x));
}


template <typename Operation1, typename Operation2>
class MSTL_FUNC_ADAPTER_DEPRECATE unary_compose {
protected:
	Operation1 op1;
	Operation2 op2;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation2::argument_type;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation1::result_type;

	constexpr explicit unary_compose(
		const Operation1& x, const Operation2& y) : op1(x), op2(y) {
	}
	constexpr typename Operation1::result_type operator()(
		const typename Operation2::argument_type& x) const {
		return op1(op2(x));
	}
};
template <typename Operation1, typename Operation2>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr unary_compose<Operation1, Operation2> compose1(
	const Operation1& op1, const Operation2& op2) {
	return unary_compose<Operation1, Operation2>(op1, op2);
}

template <typename Operation1, typename Operation2, typename Operation3>
class MSTL_FUNC_ADAPTER_DEPRECATE binary_compose {
protected:
	Operation1 op1;
	Operation2 op2;
	Operation3 op3;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation2::argument_type;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= typename Operation1::result_type;

	constexpr explicit binary_compose(
		const Operation1& x, const Operation2& y, const Operation3& z) :
		op1(x), op2(y), op3(z) {
	}
	constexpr typename Operation1::result_type operator()(
		const typename Operation2::argument_type& x) const {
		return op1(op2(x), op3(x));
	}
};

template <typename Operation1, typename Operation2, typename Operation3>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr binary_compose<Operation1, Operation2, Operation3>
compose2(const Operation1& op1, const Operation2& op2, const Operation3& op3) {
	return binary_compose<Operation1, Operation2, Operation3>(op1, op2, op3);
}


template <typename Arg, typename Result>
class MSTL_FUNC_ADAPTER_DEPRECATE pointer_to_unary_function {
protected:
	Result(*ptr)(Arg);

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Arg;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= Result;

	constexpr pointer_to_unary_function() : ptr(nullptr) {}
	constexpr explicit pointer_to_unary_function(Result(*x)(Arg)) : ptr(x) {}
	constexpr Result operator()(Arg x) const {
		if (ptr == nullptr) return Result();
		return ptr(x);
	}
};
template <typename Arg, typename Result>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr pointer_to_unary_function<Arg, Result> ptr_fun(Result(*x)(Arg)) {
	return pointer_to_unary_function<Arg, Result>(x);
}

template <typename Arg1, typename Arg2, typename Result>
class MSTL_FUNC_ADAPTER_DEPRECATE pointer_to_binary_function {
protected:
	Result(* ptr)(Arg1, Arg2);

public:
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Arg1;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= Arg2;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= Result;

	constexpr pointer_to_binary_function() : ptr(nullptr) {}
	constexpr explicit pointer_to_binary_function(Result(*x)(Arg1, Arg2)) : ptr(x) {}
	constexpr Result operator()(Arg1 x, Arg2 y) const {
		if (ptr == nullptr) return Result();
		return ptr(x, y);
	}
};
template <typename Arg1, typename Arg2, typename Result>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr pointer_to_binary_function<Arg1, Arg2, Result>
ptr_fun(Result(*x)(Arg1, Arg2)) {
	return pointer_to_binary_function<Arg1, Arg2, Result>(x);
}

template <typename S, typename T>
class MSTL_FUNC_ADAPTER_DEPRECATE mem_fun_t {
protected:
	S(T::* f)();

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T*;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= S;

	constexpr explicit mem_fun_t(S(T::* pf)()) : f(pf) {}
	constexpr S operator ()(T* p) const { return (p->*f)(); }
};
template <typename S, typename T>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr mem_fun_t<S, T> mem_fun(S(T::* f)()) {
	return mem_fun_t<S, T>(f);
}

template <typename S, typename T>
class MSTL_FUNC_ADAPTER_DEPRECATE const_mem_fun_t {
protected:
	S(T::* f)() const;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= const T*;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= S;

	constexpr explicit const_mem_fun_t(S(T::* pf)() const) : f(pf) {}
	constexpr S operator ()(const T* p) const { return (p->*f)(); }
};
template <typename S, typename T>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr const_mem_fun_t<S, T> mem_fun(S(T::* f)() const) {
	return const_mem_fun_t<S, T>(f);
}

template <typename S, typename T>
class MSTL_FUNC_ADAPTER_DEPRECATE mem_fun_ref_t {
protected:
	S(T::* f)();

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T&;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= S;

	constexpr explicit mem_fun_ref_t(S(T::* pf)()) : f(pf) {}
	constexpr S operator ()(T& r) const { return (r.*f)(); }
};
template <typename S, typename T>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr mem_fun_ref_t<S, T> mem_fun_ref(S(T::* f)()) {
	return mem_fun_ref_t<S, T>(f);
}

template <typename S, typename T>
class MSTL_FUNC_ADAPTER_DEPRECATE const_mem_fun_ref_t {
protected:
	S(T::* f)() const;

public:
	using argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= const T&;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE	= S;

	constexpr explicit const_mem_fun_ref_t(S(T::* pf)() const) : f(pf) {}
	constexpr S operator ()(const T& r) const { return (r.*f)(); }
};
template <typename S, typename T>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr const_mem_fun_ref_t<S, T> mem_fun_ref(S(T::* f)() const) {
	return const_mem_fun_ref_t<S, T>(f);
}

template <typename S, typename T, typename A>
class MSTL_FUNC_ADAPTER_DEPRECATE mem_fun1_t {
protected:
	S(T::* f)(A);

public:
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T*;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= A;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= S;

	constexpr explicit mem_fun1_t(S(T::* pf)(A)) : f(pf) {}
	constexpr S operator()(T* p, A x) const { return (p->*f)(x); }
};
template <typename S, typename T, typename A>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr mem_fun1_t<S, T, A> mem_fun1(S(T::* f)(A)) {
	return mem_fun1_t<S, T, A>(f);
}

template <typename S, typename T, typename A>
class MSTL_FUNC_ADAPTER_DEPRECATE const_mem_fun1_t {
protected:
	S(T::* f)(A) const;

public:
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= const T*;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= A;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= S;

	MSTL_FUNC_ADAPTER_DEPRECATE constexpr explicit const_mem_fun1_t(S(T::* pf)(A) const) : f(pf) {}
	MSTL_FUNC_ADAPTER_DEPRECATE constexpr S operator()(const T* p, A x) const { return (p->*f)(x); }
};
template <typename S, typename T, typename A>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr const_mem_fun1_t<S, T, A> mem_fun1(S(T::* f)(A) const) {
	return const_mem_fun1_t<S, T, A>(f);
}

template <typename S, typename T, typename A>
class mem_fun1_ref_t : public binary_function<T, A, S> {
protected:
	S(T::* f)(A);

public:
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= T&;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= A;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= S;

	MSTL_FUNC_ADAPTER_DEPRECATE constexpr explicit mem_fun1_ref_t(S(T::* pf)(A)) : f(pf) {}
	MSTL_FUNC_ADAPTER_DEPRECATE constexpr S operator()(T& r, A x) const { return (r.*f)(x); }
};
template <typename S, typename T, typename A>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr mem_fun1_ref_t<S, T, A> mem_fun1_ref(S(T::* f)(A)) {
	return mem_fun1_ref_t<S, T, A>(f);
}

template <typename S, typename T, typename A>
class const_mem_fun1_ref_t : public binary_function<T, A, S> {
protected:
	S(T::* f)(A) const;

public:
	using first_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= const T&;
	using second_argument_type MSTL_FUNC_ADAPTER_DEPRECATE	= A;
	using result_type MSTL_FUNC_ADAPTER_DEPRECATE			= S;

	MSTL_FUNC_ADAPTER_DEPRECATE constexpr explicit const_mem_fun1_ref_t(S(T::* pf)(A) const) : f(pf) {}
	MSTL_FUNC_ADAPTER_DEPRECATE constexpr S operator()(const T& r, A x) const { return (r.*f)(x); }
};
template <typename S, typename T, typename A>
MSTL_FUNC_ADAPTER_DEPRECATE constexpr const_mem_fun1_ref_t<S, T, A> mem_fun1_ref(S(T::* f)(A) const) {
	return const_mem_fun1_ref_t<S, T, A>(f);
}

#ifdef MSTL_COMPILER_CLANG__
#pragma clang diagnostic pop
#elif defined(MSTL_COMPILER_GCC__)
#pragma GCC diagnostic pop
#elif defined(MSTL_COMPILER_MSVC__)
#pragma warning(pop)
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_FUNCTOR_HPP__
