#ifndef MSTL_CORE_FUNCTIONAL_FUNCTOR_ADAPTER_HPP__
#define MSTL_CORE_FUNCTIONAL_FUNCTOR_ADAPTER_HPP__
#include "functor.hpp"
MSTL_BEGIN_NAMESPACE__

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
#endif // MSTL_CORE_FUNCTIONAL_FUNCTOR_ADAPTER_HPP__
