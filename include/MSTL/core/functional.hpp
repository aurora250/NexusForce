#ifndef MSTL_FUNCTION_HPP__
#define MSTL_FUNCTION_HPP__
#include "memory.hpp"
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
    return (__invoke_forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemFun, typename T, typename... Args>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_memfun_deref_tag, MemFun&& f, T&& t, Args&&... args){
    return (*_MSTL forward<T>(t).*f)(_MSTL forward<Args>(args)...);
}
template <typename Res, typename MemPtr, typename T>
constexpr Res __invoke_dispatch(_MSTL_TAG invoke_memobj_ref_tag, MemPtr&& f, T&& t) {
    return __invoke_forward<T>(t).*f;
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


MSTL_BEGIN_INNER__

template <template <typename...> class, typename, typename>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v = false;

template <template <typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, tuple<U...>> = Trait<T, U...>::value;

template <template <typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, tuple<U...>&> = Trait<T, U&...>::value;

template <template <typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, const tuple<U...>> = Trait<T, const U...>::value;

template<template<typename...> class Trait, typename T, typename... U>
MSTL_INLINE17 constexpr bool __apply_unpack_tuple_v<Trait, T, const tuple<U...>&> = Trait<T, const U&...>::value;

template <template <typename...> class Trait, typename T, typename Tuple>
struct __apply_unpack_tuple : bool_constant<__apply_unpack_tuple_v<Trait, T, Tuple>> {};


template <typename F, typename Tuple, size_t... Idx>
constexpr decltype(auto) __apply_impl(F&& f, Tuple&& t, _MSTL index_sequence<Idx...>) {
    return _MSTL invoke(_MSTL forward<F>(f), _MSTL get<Idx>(_MSTL forward<Tuple>(t))...);
}

MSTL_END_INNER__

template <typename F, typename Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
noexcept(_INNER __apply_unpack_tuple<_MSTL is_nothrow_invocable, F, Tuple>::value) {
    using Indices = make_index_sequence<tuple_size_v<remove_reference_t<Tuple>>>;
    return _INNER __apply_impl(_MSTL forward<F>(f), _MSTL forward<Tuple>(t), Indices{});
}


enum class FUNCTION_OPERATE {
	GET_TYPE_INFO, GET_PTR, COPY_PTR, DESTROY_PTR
};

template <typename Sign>
class function;


MSTL_BEGIN_INNER__

class __undefined_util;

union __nocopy_type {
	void* object_;
	const void* const_object_;
	void (* function_pointer_)();
	void (__undefined_util::* member_pointer_)();
};

union storage_data {
	MSTL_NODISCARD void* access() noexcept { return &data_[0]; }
	MSTL_NODISCARD const void* access() const noexcept { return &data_[0]; }

	template <typename T>
	MSTL_NODISCARD T& access() noexcept { return *static_cast<T*>(access()); }

	template <typename T>
	MSTL_NODISCARD const T& access() const noexcept { return *static_cast<const T*>(access()); }

	__nocopy_type unused_;
	byte_t data_[sizeof(__nocopy_type)];
};


class __function_base {
public:
    static constexpr size_t max_size_ = sizeof(__nocopy_type);
    static constexpr size_t max_align_ = alignof(__nocopy_type);

    template <typename F>
	class __manager_base {
    protected:
		static constexpr bool stored_ = is_trivially_copyable_v<F> && sizeof(F) <= max_size_
			&& alignof(F) <= max_align_ && max_align_ % alignof(F) == 0;

    private:
    	template <typename U, bool = stored_>
	    struct __get_pointer_impl;

    	template <typename U>
	    struct __get_pointer_impl<U, true> {
    		static U* get(const storage_data& src) noexcept {
    			const U& f = src.access<U>();
    			return const_cast<U*>(_MSTL addressof(f));
    		}
    	};

    	template <typename U>
	    struct __get_pointer_impl<U, false> {
    		static U* get(const storage_data& src) noexcept {
    			return src.access<U*>();
    		}
    	};

    protected:
    	using storage_ = bool_constant<stored_>;

    	static F* get_pointer(const storage_data& src) noexcept {
    		return __get_pointer_impl<F>::get(src);
    	}

    private:
		template <typename Fn>
		static void create(storage_data& data, Fn&& f, true_type) {
			::new(data.access()) F(_MSTL forward<Fn>(f));
		}
		template <typename Fn>
		static void create(storage_data& data, Fn&& f, false_type) {
			data.access<F*>() = new F(_MSTL forward<Fn>(f));
		}

		static void destroy(storage_data& data, true_type) {
			data.access<F>().~F();
		}
		static void destroy(storage_data& data, false_type) {
			delete data.access<F*>();
		}

    public:
		static bool manage(storage_data& dest, const storage_data& src, const FUNCTION_OPERATE oper) {
			switch (oper) {
				case FUNCTION_OPERATE::GET_TYPE_INFO:
					dest.access<const std::type_info*>() = &typeid(F);
					break;
				case FUNCTION_OPERATE::GET_PTR:
					dest.access<F*>() = __manager_base::get_pointer(src);
					break;
				case FUNCTION_OPERATE::COPY_PTR:
					__manager_base::init_func(dest, *const_cast<const F*>(__manager_base::get_pointer(src)));
					break;
				case FUNCTION_OPERATE::DESTROY_PTR:
					__manager_base::destroy(dest, storage_());
					break;
			}
			return false;
		}

		template <typename Fn>
    	static void init_func(storage_data& func, Fn&& f)
		noexcept(conjunction_v<storage_, is_nothrow_constructible<F, Fn>>) {
			__manager_base::create(func, _MSTL forward<Fn>(f), storage_());
		}

		template <typename Sign>
		static bool not_empty_function(const _MSTL function<Sign>& f) noexcept {
			return static_cast<bool>(f);
		}
		template <typename T>
		static bool not_empty_function(T* fptr) noexcept {
			return fptr != nullptr;
		}
		template <typename Class, typename T>
		static bool not_empty_function(T Class::* mptr) noexcept {
			return mptr != nullptr;
		}
		template <typename T>
		static bool not_empty_function(const T&) noexcept {
			return true;
		}
    };

	using manage_type = bool (*)(storage_data&, const storage_data&, FUNCTION_OPERATE);

	storage_data func_{};
	manage_type manager_ = nullptr;


    __function_base() = default;
    ~__function_base() {
		if (manager_)
			manager_(func_, func_, FUNCTION_OPERATE::DESTROY_PTR);
    }

    MSTL_NODISCARD bool empty() const { return !manager_; }
};

template <typename Sign, typename F>
class __function_manage_handler;

template <typename Res, typename F, typename... Args>
class __function_manage_handler<Res(Args...), F>
	: public __function_base::__manager_base<F> {
private:
	using base_type = __function_base::__manager_base<F>;
public:
	static bool manage(storage_data& dest, const storage_data& src,
		FUNCTION_OPERATE oper) {
		switch (oper) {
			case FUNCTION_OPERATE::GET_TYPE_INFO:
				dest.access<const std::type_info*>() = &typeid(F);
				break;
			case FUNCTION_OPERATE::GET_PTR:
				dest.access<F*>() = base_type::get_pointer(src);
				break;
			default:
				base_type::manage(dest, src, oper);
		}
		return false;
	}

	static Res invoke(const storage_data& f, Args&&... args) {
      	return _MSTL invoke_r<Res>(*base_type::get_pointer(f), _MSTL forward<Args>(args)...);
	}

	template <typename Fn>
	static constexpr bool nothrow_init() noexcept {
		return conjunction_v<typename base_type::storage_, is_nothrow_constructible<F, Fn>>;
	}
};

template <>
class __function_manage_handler<void, void> {
public:
	static bool manage(storage_data&, const storage_data&, FUNCTION_OPERATE) {
		return false;
	}
};

template <typename Sign, typename F, bool Valid = is_object_v<F>>
struct __function_handler_dispatch : __function_manage_handler<Sign, remove_cv_t<F>> {};

template <typename Sign, typename F>
struct __function_handler_dispatch<Sign, F, false> : __function_manage_handler<void, void> {};

MSTL_END_INNER__


template <typename Res, typename... Args>
class function<Res(Args...)> : _INNER __function_base {
private:
	template <typename F, bool IsSelf = is_same_v<remove_cvref_t<F>, function>>
	using enable_decay_t = typename enable_if_t<!IsSelf, decay<F>>::type;

	template <typename F, typename DF = enable_decay_t<F>, typename Res1 = _INNER __invoke_result_aux<DF&, Args...>>
	struct callable_t : _INNER __is_invocable_aux<Res1, Res>::type {};

	template<typename F>
	using handler_t = _INNER __function_manage_handler<Res(Args...), decay_t<F>>;

	using invoker_type = Res (*)(const _INNER storage_data&, Args&&...);

	invoker_type invoker_ = nullptr;

	template <typename F, enable_if_t<is_object_v<F>, int> = 0>
	const F* __target_impl() const noexcept {
		if (manager_ == &_INNER __function_handler_dispatch<Res(Args...), F>::manage
			|| (manager_ && typeid(F) == target_type())) {
			_INNER storage_data ptr{};
			manager_(ptr, func_, FUNCTION_OPERATE::GET_PTR);
			return ptr.access<const F*>();
		}
		return nullptr;
	}
	template <typename F, enable_if_t<!is_object_v<F>, int> = 0>
	const F* __target_impl() const noexcept {
		return nullptr;
	}

public:
	using result_type = Res;

	function(nullptr_t = nullptr) noexcept : __function_base() {}

	function(const function& x) : __function_base() {
		if (static_cast<bool>(x)) {
			x.manager_(func_, x.func_, FUNCTION_OPERATE::COPY_PTR);
			invoker_ = x.invoker_;
			manager_ = x.manager_;
		}
	}

	function(function&& x) noexcept : __function_base(), invoker_(x.invoker_) {
		if (static_cast<bool>(x)) {
			func_ = x.func_;
			manager_ = x.manager_;
			x.manager_ = nullptr;
			x.invoker_ = nullptr;
		}
	}

	template <typename F, enable_if_t<callable_t<F>::value, int> = 0>
	function(F&& f) noexcept(handler_t<F>::template nothrow_init<F>())
	: __function_base() {
		static_assert(is_copy_constructible_v<decay_t<F>> && is_constructible_v<decay_t<F>, F>,
			"target of function must be constructible");

		using handler = handler_t<F>;
		if (handler::not_empty_function(f)) {
			handler::init_func(func_, _MSTL forward<F>(f));
			invoker_ = &handler::invoke;
			manager_ = &handler::manage;
	    }
	}

	function& operator =(const function& x) {
		function(x).swap(*this);
		return *this;
	}
	function& operator =(function&& x) noexcept {
		function(_MSTL move(x)).swap(*this);
		return *this;
	}
	function& operator =(nullptr_t) noexcept {
		if (manager_) {
			manager_(func_, func_, FUNCTION_OPERATE::DESTROY_PTR);
			manager_ = nullptr;
			invoker_ = nullptr;
		}
		return *this;
	}

	template <typename F, enable_if_t<callable_t<F>::value, int> = 0>
	function& operator =(F&& f) noexcept(handler_t<F>::template _S_nothrow_init<F>()) {
		function(_MSTL forward<F>(f)).swap(*this);
		return *this;
	}
	template <typename F>
	function& operator =(reference_wrapper<F> f) noexcept {
		function(f).swap(*this);
		return *this;
	}

	void swap(function& x) noexcept {
		_MSTL swap(func_, x.func_);
		_MSTL swap(manager_, x.manager_);
		_MSTL swap(invoker_, x.invoker_);
	}

	explicit operator bool() const noexcept { return !empty(); }

	Res operator ()(Args&&... args) const {
		if (empty()) Exception(MemoryError("functional pointing to null."));
		return invoker_(func_, _MSTL forward<Args>(args)...);
	}

	MSTL_NODISCARD const std::type_info& target_type() const noexcept {
		if (manager_) {
			_INNER storage_data result{};
			manager_(result, func_, FUNCTION_OPERATE::GET_TYPE_INFO);
			if (const auto info = result.access<const std::type_info*>())
				return *info;
		}
		return typeid(void);
	}

	template <typename F>
	const F* target() const noexcept {
		return __target_impl<F>();
	}
	template <typename F>
	F* target() noexcept {
		const F* f = const_cast<const function*>(this)->target<F>();
		return *const_cast<F**>(&f);
	}
};

template <typename Res, typename... Args>
bool operator ==(const function<Res(Args...)>& f, nullptr_t) noexcept {
	return !static_cast<bool>(f);
}
template <typename Res, typename... Args>
bool operator ==(nullptr_t, const function<Res(Args...)>& f) noexcept {
	return !static_cast<bool>(f);
}
template <typename Res, typename... Args>
bool operator !=(const function<Res(Args...)>& f, nullptr_t) noexcept {
	return static_cast<bool>(f);
}
template <typename Res, typename... Args>
bool operator !=(nullptr_t, const function<Res(Args...)>& f) noexcept {
	return static_cast<bool>(f);
}
template <typename Res, typename... Args>
void swap(function<Res(Args...)>& lh, function<Res(Args...)>& rh) noexcept {
	lh.swap(rh);
}

MSTL_END_NAMESPACE__
#endif // MSTL_FUNCTION_HPP__
