#ifndef MSTL_CORE_FUNCTIONAL_FUNCTION_HPP__
#define MSTL_CORE_FUNCTIONAL_FUNCTION_HPP__
#include "../exception/exception.hpp"
#include "invoke.hpp"
#include <typeinfo>
MSTL_BEGIN_NAMESPACE__

template <typename Sign>
class function;

MSTL_BEGIN_INNER__

enum class FUNCTION_OPERATE {
	GET_TYPE_INFO, GET_PTR, COPY_PTR, DESTROY_PTR
};


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
	unsigned char data_[sizeof(__nocopy_type)];
};


class __function_base {
public:
    static constexpr size_t max_size_ = sizeof(__nocopy_type);
    static constexpr size_t max_align_ = alignof(__nocopy_type);

    template <typename F>
	class __manager_base {
    protected:
		static constexpr bool stored_ = is_location_invariant_v<F> && sizeof(F) <= max_size_
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
class function<Res(Args...)> : _INNER __function_base, public iswappable<function<Res(Args...)>> {
private:
	template <typename F, bool IsSelf = is_same_v<remove_cvref_t<F>, function>>
	using enable_decay_t = typename enable_if_t<!IsSelf, decay<F>>::type;

	template <typename F, typename = void>
	struct callable_t : false_type {};

	template <typename F>
	struct callable_t<F, enable_if_t<
		!is_same_v<remove_cvref_t<F>, function> &&
		is_invocable_r_v<Res, decay_t<F>&, Args...>
	>> : true_type {};

	template <typename F>
	using handler_t = _INNER __function_manage_handler<Res(Args...), decay_t<F>>;

	using invoker_type = Res (*)(const _INNER storage_data&, Args&&...);

	invoker_type invoker_ = nullptr;

	template <typename F, enable_if_t<is_object_v<F>, int> = 0>
	MSTL_ALWAYS_INLINE const F* __target_impl() const noexcept {
		if (manager_ == &_INNER __function_handler_dispatch<Res(Args...), F>::manage
			|| (manager_ && typeid(F) == target_type())) {
			_INNER storage_data ptr{};
			manager_(ptr, func_, _INNER FUNCTION_OPERATE::GET_PTR);
			return ptr.access<const F*>();
		}
		return nullptr;
	}
	template <typename F, enable_if_t<!is_object_v<F>, int> = 0>
	MSTL_ALWAYS_INLINE const F* __target_impl() const noexcept {
		return nullptr;
	}

public:
	using result_type = Res;

	function(nullptr_t = nullptr) noexcept : __function_base() {}

	function(const function& x) : __function_base() {
		if (static_cast<bool>(x)) {
			x.manager_(func_, x.func_, _INNER FUNCTION_OPERATE::COPY_PTR);
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
			manager_(func_, func_, _INNER FUNCTION_OPERATE::DESTROY_PTR);
			manager_ = nullptr;
			invoker_ = nullptr;
		}
		return *this;
	}

	template <typename F, enable_if_t<callable_t<F>::value, int> = 0>
	function& operator =(F&& f) noexcept(handler_t<F>::template nothrow_init<F>()) {
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

	Res operator ()(Args&&... args) const
    noexcept(noexcept(invoker_(func_, _MSTL forward<Args>(args)...))) {
		if (empty()) throw_exception(memory_exception("functional pointing to null."));
		return invoker_(func_, _MSTL forward<Args>(args)...);
	}

	MSTL_NODISCARD const std::type_info& target_type() const noexcept {
		if (manager_) {
			_INNER storage_data result{};
			manager_(result, func_, _INNER FUNCTION_OPERATE::GET_TYPE_INFO);
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
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
MSTL_BEGIN_INNER__

template <typename>
struct __function_guide_helper {};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...)> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) noexcept> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) &> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) & noexcept> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) const> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) const noexcept> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) const &> {
	using type = Result(Args...);
};

template<typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...) const & noexcept> {
	using type = Result(Args...);
};

MSTL_END_INNER__

template<typename Res, typename... Args>
function(Res(*)(Args...)) -> function<Res(Args...)>;

template<typename Func, typename Sign = typename
_INNER __function_guide_helper<decltype(&Func::operator())>::type>
function(Func) -> function<Sign>;

#endif


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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_FUNCTION_HPP__
