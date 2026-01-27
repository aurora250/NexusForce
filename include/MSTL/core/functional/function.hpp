#ifndef MSTL_CORE_FUNCTIONAL_FUNCTION_HPP__
#define MSTL_CORE_FUNCTIONAL_FUNCTION_HPP__

/**
 * @file function.hpp
 * @brief MSTL通用函数包装器
 *
 * 此文件提供了通用函数包装器，支持存储、复制和调用任意可调用对象。
 */

#include "MSTL/core/exception/exception.hpp"
#include "MSTL/core/functional/invoke.hpp"
#include <typeinfo>
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup FunctionWrapper 函数包装器
 * @brief 通用函数包装器及相关工具
 * @{
 */

/**
 * @brief 函数包装器主模板声明
 * @tparam Sign 函数签名类型
 *
 * 提供类型安全的函数包装，支持任意可调用对象。
 */
template <typename Sign>
class function;

/// @cond
MSTL_BEGIN_INNER__

/**
 * @enum FUNCTION_OPERATE
 * @brief 函数管理器操作类型枚举
 *
 * 定义函数管理器支持的操作类型，用于统一管理函数对象的生命周期。
 */
enum class FUNCTION_OPERATE {
	GET_TYPE_INFO,   ///< 获取类型信息操作
	GET_PTR,         ///< 获取指针操作
	COPY_PTR,        ///< 复制指针操作
	DESTROY_PTR      ///< 销毁指针操作
};


/**
 * @class __undefined_util
 * @brief 未定义工具类
 *
 * 用于成员指针类型的占位类，无实际用途。
 */
class __undefined_util;

/**
 * @union __nocopy_type
 * @brief 不可复制类型联合体
 *
 * 用于存储不同类型指针的联合体，确保正确的内存对齐。
 */
union __nocopy_type {
	void* object_;
	const void* const_object_;
	void (* function_pointer_)();
	void (__undefined_util::* member_pointer_)();
};

/**
 * @union storage_data
 * @brief 存储数据联合体
 *
 * 提供类型安全的存储访问，支持不同类型数据的存储和访问。
 */
union storage_data {
	MSTL_NODISCARD void* access() noexcept { return &data_[0]; }
	MSTL_NODISCARD const void* access() const noexcept { return &data_[0]; }

	template <typename T>
	MSTL_NODISCARD T& access() noexcept { return *static_cast<T*>(access()); }

	template <typename T>
	MSTL_NODISCARD const T& access() const noexcept { return *static_cast<const T*>(access()); }

	__nocopy_type unused_;  ///< 未使用的nocopy类型
	byte_t data_[sizeof(__nocopy_type)]; ///< 原始字节数据存储
};

/**
 * @class __function_base
 * @brief 函数包装器基类
 *
 * 提供函数包装器的基本存储和管理功能，处理函数对象的生命周期管理。
 */
class __function_base {
public:
	static constexpr size_t max_size_ = sizeof(__nocopy_type);   ///< 最大内联存储大小
	static constexpr size_t max_align_ = alignof(__nocopy_type); ///< 最大对齐要求

	/**
	 * @class __manager_base
	 * @brief 管理器基类模板
	 * @tparam F 管理的函数对象类型
	 *
	 * 提供函数对象的存储、复制、销毁等基本管理功能。
	 */
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

/**
 * @class __function_manage_handler
 * @brief 函数管理处理器模板
 * @tparam Sign 函数签名
 * @tparam F 函数对象类型
 *
 * 提供特定函数签名的调用和管理功能。
 */
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
/// @endcond

/**
 * @brief 通用函数包装器类模板
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 *
 * 提供类型安全的函数包装，支持存储、复制和调用任意可调用对象。
 */
template <typename Res, typename... Args>
class function<Res(Args...)> : _INNER __function_base {
private:
	using invoker_type = Res (*)(const _INNER storage_data&, Args&&...); ///< 调用器类型

private:
	invoker_type invoker_ = nullptr; ///< 调用器函数指针

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
	using result_type = Res;  ///< 返回类型别名

	/**
	 * @brief 默认构造函数
	 * @param null 空指针字面量
	 */
	function(nullptr_t null = nullptr) noexcept
	: __function_base() {}

	/**
	 * @brief 复制构造函数
	 * @param x 要复制的function对象
	 */
	function(const function& x) : __function_base() {
		if (static_cast<bool>(x)) {
			x.manager_(func_, x.func_, _INNER FUNCTION_OPERATE::COPY_PTR);
			invoker_ = x.invoker_;
			manager_ = x.manager_;
		}
	}

	/**
	 * @brief 移动构造函数
	 * @param x 要移动的function对象
	 */
	function(function&& x) noexcept
	: __function_base(), invoker_(x.invoker_) {
		if (static_cast<bool>(x)) {
			func_ = x.func_;
			manager_ = x.manager_;
			x.manager_ = nullptr;
			x.invoker_ = nullptr;
		}
	}

	/**
	 * @brief 从任意可调用对象构造
	 * @tparam F 可调用对象类型
	 * @param f 可调用对象
	 */
	template <typename F, enable_if_t<callable_t<F>::value, int> = 0>
	function(F&& f) noexcept(handler_t<F>::template nothrow_init<F>())
	: __function_base() {
		static_assert(
			is_copy_constructible_v<decay_t<F>> &&
			is_constructible_v<decay_t<F>, F>,
			"target of function must be constructible");

		using handler = handler_t<F>;
		if (handler::not_empty_function(f)) {
			handler::init_func(func_, _MSTL forward<F>(f));
			invoker_ = &handler::invoke;
			manager_ = &handler::manage;
	    }
	}

	/**
	 * @brief 复制赋值运算符
	 * @param x 要赋值的function对象
	 * @return 当前对象的引用
	 */
	function& operator =(const function& x) {
		function(x).swap(*this);
		return *this;
	}

	/**
	 * @brief 移动赋值运算符
	 * @param x 要移动的function对象
	 * @return 当前对象的引用
	 */
	function& operator =(function&& x) noexcept {
		function(_MSTL move(x)).swap(*this);
		return *this;
	}

	/**
	 * @brief 空指针赋值运算符
	 * @param null 空指针字面量
	 * @return 当前对象的引用
	 */
	function& operator =(nullptr_t null) noexcept {
		if (manager_) {
			manager_(func_, func_, _INNER FUNCTION_OPERATE::DESTROY_PTR);
			manager_ = nullptr;
			invoker_ = nullptr;
		}
		return *this;
	}

	/**
	 * @brief 从任意可调用对象赋值
	 * @tparam F 可调用对象类型
	 * @param f 可调用对象
	 * @return 当前对象的引用
	 */
	template <typename F, enable_if_t<callable_t<F>::value, int> = 0>
	function& operator =(F&& f) noexcept(handler_t<F>::template nothrow_init<F>()) {
		function(_MSTL forward<F>(f)).swap(*this);
		return *this;
	}

	/**
	 * @brief 从引用包装器赋值
	 * @tparam F 可调用对象类型
	 * @param f 引用包装器
	 * @return 当前对象的引用
	 */
	template <typename F>
	function& operator =(reference_wrapper<F> f) noexcept {
		function(f).swap(*this);
		return *this;
	}

	/**
	 * @brief 交换两个function对象
	 * @param x 要交换的function对象
	 */
	void swap(function& x) noexcept {
		_MSTL swap(func_, x.func_);
		_MSTL swap(manager_, x.manager_);
		_MSTL swap(invoker_, x.invoker_);
	}

	/**
	 * @brief 转换为布尔值
	 * @return 是否非空
	 */
	explicit operator bool() const noexcept { return !empty(); }

	/**
	 * @brief 函数调用运算符
	 * @param args 调用参数
	 * @return 调用结果
	 * @throw memory_exception 如果function为空
	 */
	Res operator ()(Args&&... args) const
    noexcept(noexcept(invoker_(func_, _MSTL forward<Args>(args)...))) {
		if (empty()) {
		    throw_exception(memory_exception("functional pointing to null."));
		}
		return invoker_(func_, _MSTL forward<Args>(args)...);
	}

	/**
	 * @brief 获取目标类型信息
	 * @return 目标类型信息
	 */
	MSTL_NODISCARD const std::type_info& target_type() const noexcept {
		if (manager_) {
			_INNER storage_data result{};
			manager_(result, func_, _INNER FUNCTION_OPERATE::GET_TYPE_INFO);
			if (const auto info = result.access<const std::type_info*>()) {
				return *info;
			}
		}
		return typeid(void);
	}

	/**
	 * @brief 获取目标对象的常量指针
	 * @tparam F 目标类型
	 * @return 目标对象的常量指针，如果类型不匹配则返回nullptr
	 */
	template <typename F>
	const F* target() const noexcept {
		return __target_impl<F>();
	}

	/**
	 * @brief 获取目标对象的指针
	 * @tparam F 目标类型
	 * @return 目标对象的指针，如果类型不匹配则返回nullptr
	 */
	template <typename F>
	F* target() noexcept {
		const F* f = const_cast<const function*>(this)->target<F>();
		return *const_cast<F**>(&f);
	}
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
/// @cond
MSTL_BEGIN_INNER__

template <typename>
struct __function_guide_helper;

template <typename Result, typename Class, typename... Args>
struct __function_guide_helper<Result (Class::*)(Args...)> {
	using type = Result(Args...);
};

MSTL_END_INNER__
/// @endcond

template <typename Res, typename... Args>
function(Res(*)(Args...)) -> function<Res(Args...)>;

template <typename Func, typename Sign = typename _INNER __function_guide_helper<
	remove_function_qualifiers_t<decltype(&Func::operator ())>>::type>
function(Func) -> function<Sign>;

#endif


/**
 * @brief 等于空指针比较
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 * @param f function对象
 * @param null 空指针字面量
 * @return function是否为空
 */
template <typename Res, typename... Args>
bool operator ==(const function<Res(Args...)>& f, nullptr_t null) noexcept {
	return !static_cast<bool>(f);
}

/**
 * @brief 等于空指针比较
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 * @param null 空指针字面量
 * @param f function对象
 * @return function是否为空
 */
template <typename Res, typename... Args>
bool operator ==(nullptr_t null, const function<Res(Args...)>& f) noexcept {
	return !static_cast<bool>(f);
}

/**
 * @brief 不等于空指针比较
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 * @param f function对象
 * @param null 空指针字面量
 * @return function是否非空
 */
template <typename Res, typename... Args>
bool operator !=(const function<Res(Args...)>& f, nullptr_t null) noexcept {
	return static_cast<bool>(f);
}

/**
 * @brief 不等于空指针比较
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 * @param null 空指针字面量
 * @param f function对象
 * @return function是否非空
 */
template <typename Res, typename... Args>
bool operator !=(nullptr_t null, const function<Res(Args...)>& f) noexcept {
	return static_cast<bool>(f);
}

/** @} */ // FunctionWrapper

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FUNCTIONAL_FUNCTION_HPP__
