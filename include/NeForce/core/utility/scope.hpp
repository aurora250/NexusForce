#ifndef NEFORCE_CORE_UTILITY_SCOPE_HPP__
#define NEFORCE_CORE_UTILITY_SCOPE_HPP__
#include "NeForce/core/numeric/numeric_traits.hpp"
#include "NeForce/core/utility/compressed_pair.hpp"
#include "NeForce/core/exception/exception.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename Func>
class scope_exit {
private:
	compressed_pair<Func, bool> func_pair_;

public:
	template <typename F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, scope_exit> &&
		is_constructible_v<Func, F> &&
		!is_nothrow_constructible_v<Func, F>, int> = 0>
    explicit scope_exit(F&& func)
	try : func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), true) {}
	catch (...) { func(); }

	template <typename F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, scope_exit> &&
		is_constructible_v<Func, F> &&
		is_nothrow_constructible_v<Func, F>, int> = 0>
    explicit scope_exit(F&& func) noexcept
    : func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), true) {}

	scope_exit(const scope_exit&) = delete;
	scope_exit& operator =(const scope_exit&) = delete;

	scope_exit(scope_exit&& rhs)
	noexcept(is_nothrow_move_constructible_v<Func>)
	: func_pair_(move(rhs.func_pair_)) {
		rhs.release();
	}

	scope_exit& operator =(scope_exit&&) = delete;

	~scope_exit() noexcept {
		if (func_pair_.value) {
			func_pair_.get_base()();
		}
	}

	void release() noexcept {
		func_pair_.value = false;
	}
};

#ifdef NEFORCE_STANDARD_17
template <typename Func>
scope_exit(Func) -> scope_exit<Func>;
#endif


template <typename Func>
class scope_fail {
private:
	compressed_pair<Func, int> func_pair_;

public:
	template <typename F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, scope_fail>
		&& is_constructible_v<Func, F>
		&& !is_nothrow_constructible_v<Func, F>, int> = 0>
	explicit scope_fail(F&& func)
	try : func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {}
	catch (...) { func(); }

	template <typename F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, scope_fail>
		&& is_constructible_v<Func, F>
		&& is_nothrow_constructible_v<Func, F>, int> = 0>
	explicit scope_fail(F&& func) noexcept
	: func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {}

	scope_fail(const scope_fail&) = delete;
	scope_fail& operator =(const scope_fail&) = delete;

	scope_fail(scope_fail&& rhs) noexcept
	: func_pair_(move(rhs.func_pair_)) {
		rhs.release();
	}

	scope_fail& operator =(scope_fail&&) = delete;

	~scope_fail() noexcept {
		if (uncaught_exceptions() > func_pair_.value) {
			func_pair_.get_base()();
		}
	}

	void release() noexcept {
		func_pair_.value = numeric_traits<int>::max();
	}
};

#ifdef NEFORCE_STANDARD_17
template <typename Func>
scope_fail(Func) -> scope_fail<Func>;
#endif


template <typename Func>
class scope_success {
private:
	compressed_pair<Func, int> func_pair_;

public:
	template <typename F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, scope_success>
     	&& is_constructible_v<Func, F>
		&& !is_nothrow_constructible_v<Func, F>, int> = 0>
	explicit scope_success(F&& func)
	try : func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {}
	catch (...) { func(); }

	template <typename F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, scope_success>
		&& is_constructible_v<Func, F>
		&& is_nothrow_constructible_v<Func, F>, int> = 0>
	explicit scope_success(F&& func) noexcept
	: func_pair_(exact_arg_construct_tag{}, _NEFORCE forward<F>(func), uncaught_exceptions()) {}

	scope_success(const scope_success&) = delete;
	scope_success& operator =(const scope_success&) = delete;

	scope_success(scope_success&& rhs)
	noexcept(is_nothrow_move_assignable_v<Func>)
	: func_pair_(move(rhs.func_pair_)) {
		rhs.release();
	}

	scope_success& operator =(scope_success&&) = delete;

	~scope_success()
	noexcept(is_nothrow_invocable_v<Func>) {
		if (uncaught_exceptions() <= func_pair_.value) {
			func_pair_.get_base()();
		}
	}

	void release() noexcept {
		func_pair_.value = -numeric_traits<int>::max();
	}
};

#ifdef NEFORCE_STANDARD_17
template <typename Func>
scope_success(Func) -> scope_success<Func>;
#endif

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_SCOPE_HPP__