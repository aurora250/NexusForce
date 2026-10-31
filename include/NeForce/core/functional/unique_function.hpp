#ifndef NEFORCE_CORE_FUNCTIONAL_UNIQUE_FUNCTION_HPP__
#define NEFORCE_CORE_FUNCTIONAL_UNIQUE_FUNCTION_HPP__

/**
 * @file unique_function.hpp
 * @brief 仅可移动的函数包装器
 *
 * 此文件提供 `unique_function` 类型擦除函数包装器，
 * 不要求存储的可调用对象可复制，自身也只可移动。
 */

#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup FunctionWrapper 函数包装器
 * @{
 */

/// @cond
template <typename Sign>
class unique_function;
/// @endcond

/// @cond
NEFORCE_BEGIN_INNER__

template <bool Const, bool Rvalue, typename F>
using __unique_qualified_ref = conditional_t<Const, conditional_t<Rvalue, const decay_t<F>&&, const decay_t<F>&>,
                                             conditional_t<Rvalue, decay_t<F>&&, decay_t<F>&>>;

template <bool Invocable, typename Res, typename F, typename... Args>
struct __unique_nothrow_invocable_r_impl : false_type {};

template <typename Res, typename F, typename... Args>
struct __unique_nothrow_invocable_r_impl<true, Res, F, Args...>
: bool_constant<noexcept(_NEFORCE invoke_r<Res>(_NEFORCE declval<F>(), _NEFORCE declval<Args>()...))> {};

template <typename Res, typename F, typename... Args>
using __unique_nothrow_invocable_r =
        __unique_nothrow_invocable_r_impl<is_invocable_r_v<Res, F, Args...>, Res, F, Args...>;

/**
 * @brief 判断可调用对象是否满足给定签名的构造约束
 * @tparam Res 返回类型
 * @tparam Const 是否以 const 方式调用
 * @tparam Rvalue 是否以右值方式调用
 * @tparam Noexcept 签名是否带 noexcept 限定
 * @tparam F 可调用对象类型
 * @tparam Args 参数类型
 */
template <typename Res, bool Const, bool Rvalue, bool Noexcept, typename F, typename... Args>
struct __unique_callable_from
: bool_constant<is_invocable_r_v<Res, __unique_qualified_ref<Const, Rvalue, F>, Args...> &&
                (!Noexcept ||
                 __unique_nothrow_invocable_r<Res, __unique_qualified_ref<Const, Rvalue, F>, Args...>::value)> {};

template <typename Sign, typename F>
struct __unique_function_handler;

template <typename Res, typename... Args, typename F>
struct __unique_function_handler<Res(Args...), F> : __function_base::__manager_base<remove_cv_t<F>> {
private:
    using stored_type = remove_cv_t<F>;
    using base_type = __function_base::__manager_base<stored_type>;

public:
    static bool manage(storage_data& dest, const storage_data& src, const function_operate oper) {
        switch (oper) {
            case function_operate::get_type_info:
                dest.access<const std::type_info*>() = &typeid(stored_type);
                break;
            case function_operate::get_ptr:
                dest.access<stored_type*>() = base_type::get_pointer(src);
                break;
            case function_operate::destroy_ptr:
                base_type::destroy_object(dest);
                break;
            case function_operate::copy_ptr:
                break;
        }
        return false;
    }

    static Res invoke_lvalue(const storage_data& data, Args... args) {
        return _NEFORCE invoke_r<Res>(*base_type::get_pointer(data), _NEFORCE forward<Args>(args)...);
    }

    static Res invoke_const(const storage_data& data, Args... args) {
        return _NEFORCE invoke_r<Res>(*base_type::get_const_pointer(data), _NEFORCE forward<Args>(args)...);
    }

    static Res invoke_rvalue(const storage_data& data, Args... args) {
        return _NEFORCE invoke_r<Res>(_NEFORCE move(*base_type::get_pointer(data)), _NEFORCE forward<Args>(args)...);
    }

    static Res invoke_const_rvalue(const storage_data& data, Args... args) {
        return _NEFORCE invoke_r<Res>(_NEFORCE move(*base_type::get_const_pointer(data)),
                                      _NEFORCE forward<Args>(args)...);
    }

    template <typename Fn>
    static constexpr bool nothrow_init() noexcept {
        return base_type::storage_::value && is_nothrow_constructible_v<stored_type, Fn>;
    }
};

template <typename Handler, typename F>
bool __unique_not_empty(const F& callable) noexcept {
    return Handler::not_empty_function(callable);
}

template <typename Handler, typename Sign>
bool __unique_not_empty(const unique_function<Sign>& callable) noexcept {
    return static_cast<bool>(callable);
}

template <typename Handler, bool Const, bool Rvalue>
struct __unique_invoker;

template <typename Handler>
struct __unique_invoker<Handler, false, false> {
    static constexpr auto get() noexcept { return &Handler::invoke_lvalue; }
};

template <typename Handler>
struct __unique_invoker<Handler, true, false> {
    static constexpr auto get() noexcept { return &Handler::invoke_const; }
};

template <typename Handler>
struct __unique_invoker<Handler, false, true> {
    static constexpr auto get() noexcept { return &Handler::invoke_rvalue; }
};

template <typename Handler>
struct __unique_invoker<Handler, true, true> {
    static constexpr auto get() noexcept { return &Handler::invoke_const_rvalue; }
};

/**
 * @brief 仅可移动函数包装器的公共基类
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 *
 * 提供存储、移动、销毁、交换、空状态判断与目标查询；
 * `operator()` 由派生类按签名限定实现。
 */
template <typename Res, typename... Args>
class __unique_function_base : public __function_base {
public:
    using result_type = Res;

    __unique_function_base() noexcept = default;
    __unique_function_base(nullptr_t) noexcept {}

    __unique_function_base(const __unique_function_base&) = delete;
    __unique_function_base& operator=(const __unique_function_base&) = delete;

    __unique_function_base(__unique_function_base&& other) noexcept { move_from(other); }

    __unique_function_base& operator=(__unique_function_base&& other) noexcept {
        if (this != &other) {
            reset();
            move_from(other);
        }
        return *this;
    }

    ~__unique_function_base() = default;

    void swap(__unique_function_base& other) noexcept {
        _NEFORCE swap(func_, other.func_);
        _NEFORCE swap(manager_, other.manager_);
        _NEFORCE swap(invoker_, other.invoker_);
    }

    explicit operator bool() const noexcept { return manager_ != nullptr; }

    NEFORCE_NODISCARD const std::type_info& target_type() const noexcept {
        if (manager_ != nullptr) {
            storage_data address{};
            manager_(address, func_, function_operate::get_type_info);
            if (const auto* const info = address.access<const std::type_info*>()) {
                return *info;
            }
        }
        return typeid(void);
    }

    template <typename F, enable_if_t<is_object_v<F>, int> = 0>
    F* target() noexcept {
        return const_cast<F*>(static_cast<const __unique_function_base*>(this)->template target<F>());
    }

    template <typename F, enable_if_t<is_object_v<F>, int> = 0>
    const F* target() const noexcept {
        using stored_type = decay_t<F>;
        using handler = __unique_function_handler<Res(Args...), stored_type>;
        if (manager_ == &handler::manage) {
            storage_data address{};
            manager_(address, func_, function_operate::get_ptr);
            return static_cast<const F*>(address.access<const stored_type*>());
        }
        return nullptr;
    }

protected:
    using invoker_type = Res (*)(const storage_data&, Args...);
    invoker_type invoker_{nullptr};

    void reset() noexcept {
        if (manager_ != nullptr) {
            manager_(func_, func_, function_operate::destroy_ptr);
            manager_ = nullptr;
            invoker_ = nullptr;
        }
    }

    void move_from(__unique_function_base& other) noexcept {
        func_ = other.func_;
        manager_ = other.manager_;
        invoker_ = other.invoker_;
        other.manager_ = nullptr;
        other.invoker_ = nullptr;
    }

    Res call(Args... args) const {
        if (manager_ == nullptr) {
            NEFORCE_THROW_EXCEPTION(memory_exception("unique_function pointing to null."));
        }
        return invoker_(func_, _NEFORCE forward<Args>(args)...);
    }
};

template <bool Const, bool Rvalue, bool Noexcept>
struct __unique_qual {
    static constexpr bool is_const = Const;
    static constexpr bool is_rvalue = Rvalue;
    static constexpr bool is_noexcept = Noexcept;
};

template <typename Self, typename Qual, typename Sign>
class __unique_function_impl;

template <typename Self, typename Qual, typename Res, typename... Args>
class __unique_function_impl<Self, Qual, Res(Args...)> : public __unique_function_base<Res, Args...> {
private:
    using base_type = __unique_function_base<Res, Args...>;

    template <typename F>
    using handler_t = __unique_function_handler<Res(Args...), decay_t<F>>;

    template <typename F>
    using callable_t = __unique_callable_from<Res, Qual::is_const, Qual::is_rvalue, Qual::is_noexcept, F, Args...>;

    template <typename F>
    using enable_t = enable_if_t<
            conjunction<callable_t<F>, negation<is_same<decay_t<F>, Self>>, is_constructible<decay_t<F>, F>>::value,
            int>;

public:
    __unique_function_impl() noexcept = default;
    __unique_function_impl(nullptr_t) noexcept {}
    __unique_function_impl(__unique_function_impl&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    __unique_function_impl& operator=(__unique_function_impl&& other) noexcept {
        static_cast<base_type&>(*this) = _NEFORCE move(other);
        return *this;
    }

    template <typename F, enable_t<F> = 0>
    __unique_function_impl(F&& callable) noexcept(handler_t<F>::template nothrow_init<F>()) {
        initialize(_NEFORCE forward<F>(callable));
    }

    template <typename F, enable_t<F> = 0>
    Self& operator=(F&& callable) noexcept(handler_t<F>::template nothrow_init<F>()) {
        Self replacement(_NEFORCE forward<F>(callable));
        this->swap(replacement);
        return static_cast<Self&>(*this);
    }

    Self& operator=(nullptr_t) noexcept {
        this->reset();
        return static_cast<Self&>(*this);
    }

private:
    template <typename F>
    void initialize(F&& callable) noexcept(handler_t<F>::template nothrow_init<F>()) {
        using handler = handler_t<F>;
        if (__unique_not_empty<handler>(callable)) {
            handler::init_func(this->func_, _NEFORCE forward<F>(callable));
            this->manager_ = &handler::manage;
            this->invoker_ = __unique_invoker<handler, Qual::is_const, Qual::is_rvalue>::get();
        }
    }
};

NEFORCE_END_INNER__
/// @endcond


/**
 * @brief 仅可移动的函数包装器
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...)>
: public inner::__unique_function_impl<unique_function<Res(Args...)>, inner::__unique_qual<false, false, false>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...)>,
                                                    inner::__unique_qual<false, false, false>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @throws memory_exception 包装器为空时抛出
     */
    Res operator()(Args... args) { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（const 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) const>
: public inner::__unique_function_impl<unique_function<Res(Args...) const>, inner::__unique_qual<true, false, false>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) const>,
                                                    inner::__unique_qual<true, false, false>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @throws memory_exception 包装器为空时抛出
     */
    Res operator()(Args... args) const { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（左值限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...)&>
: public inner::__unique_function_impl<unique_function<Res(Args...)&>, inner::__unique_qual<false, false, false>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...)&>,
                                                    inner::__unique_qual<false, false, false>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @throws memory_exception 包装器为空时抛出
     */
    Res operator()(Args... args) & { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（const 左值限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) const&>
: public inner::__unique_function_impl<unique_function<Res(Args...) const&>, inner::__unique_qual<true, false, false>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) const&>,
                                                    inner::__unique_qual<true, false, false>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @throws memory_exception 包装器为空时抛出
     */
    Res operator()(Args... args) const& { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（右值限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) &&>
: public inner::__unique_function_impl<unique_function<Res(Args...) &&>, inner::__unique_qual<false, true, false>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) &&>,
                                                    inner::__unique_qual<false, true, false>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @throws memory_exception 包装器为空时抛出
     */
    Res operator()(Args... args) && { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（const 右值限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) const&&>
: public inner::__unique_function_impl<unique_function<Res(Args...) const&&>, inner::__unique_qual<true, true, false>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) const&&>,
                                                    inner::__unique_qual<true, true, false>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @throws memory_exception 包装器为空时抛出
     */
    Res operator()(Args... args) const&& { return this->call(_NEFORCE forward<Args>(args)...); }
};

#ifdef NEFORCE_STANDARD_17

/**
 * @brief 仅可移动的函数包装器（noexcept 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) noexcept>
: public inner::__unique_function_impl<unique_function<Res(Args...) noexcept>, inner::__unique_qual<false, false, true>,
                                       Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) noexcept>,
                                                    inner::__unique_qual<false, false, true>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @warning 包装器为空时调用会终止进程
     */
    Res operator()(Args... args) noexcept { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（const noexcept 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) const noexcept>
: public inner::__unique_function_impl<unique_function<Res(Args...) const noexcept>,
                                       inner::__unique_qual<true, false, true>, Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) const noexcept>,
                                                    inner::__unique_qual<true, false, true>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @warning 包装器为空时调用会终止进程
     */
    Res operator()(Args... args) const noexcept { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（左值 noexcept 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) & noexcept>
: public inner::__unique_function_impl<unique_function<Res(Args...) & noexcept>,
                                       inner::__unique_qual<false, false, true>, Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) & noexcept>,
                                                    inner::__unique_qual<false, false, true>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @warning 包装器为空时调用会终止进程
     */
    Res operator()(Args... args) & noexcept { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（const 左值 noexcept 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) const & noexcept>
: public inner::__unique_function_impl<unique_function<Res(Args...) const & noexcept>,
                                       inner::__unique_qual<true, false, true>, Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) const & noexcept>,
                                                    inner::__unique_qual<true, false, true>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @warning 包装器为空时调用会终止进程
     */
    Res operator()(Args... args) const& noexcept { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（右值 noexcept 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) && noexcept>
: public inner::__unique_function_impl<unique_function<Res(Args...) && noexcept>,
                                       inner::__unique_qual<false, true, true>, Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) && noexcept>,
                                                    inner::__unique_qual<false, true, true>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @warning 包装器为空时调用会终止进程
     */
    Res operator()(Args... args) && noexcept { return this->call(_NEFORCE forward<Args>(args)...); }
};

/**
 * @brief 仅可移动的函数包装器（const 右值 noexcept 限定）
 * @tparam Res 返回类型
 * @tparam Args 参数类型
 */
template <typename Res, typename... Args>
class unique_function<Res(Args...) const && noexcept>
: public inner::__unique_function_impl<unique_function<Res(Args...) const && noexcept>,
                                       inner::__unique_qual<true, true, true>, Res(Args...)> {
private:
    using base_type = inner::__unique_function_impl<unique_function<Res(Args...) const && noexcept>,
                                                    inner::__unique_qual<true, true, true>, Res(Args...)>;

public:
    using base_type::base_type;
    using base_type::operator=;

    unique_function() noexcept = default;

    /**
     * @brief 移动构造
     * @param other 被移动的包装器，移动后为空
     */
    unique_function(unique_function&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 被移动的包装器，移动后为空
     * @return 当前对象的引用
     */
    unique_function& operator=(unique_function&& other) noexcept = default;

    /**
     * @brief 调用存储的可调用对象
     * @param args 调用参数
     * @return 调用结果
     * @warning 包装器为空时调用会终止进程
     */
    Res operator()(Args... args) const&& noexcept { return this->call(_NEFORCE forward<Args>(args)...); }
};

#endif

template <typename Sign>
bool operator==(const unique_function<Sign>& f, nullptr_t np) noexcept {
    static_cast<void>(np);
    return !static_cast<bool>(f);
}

template <typename Sign>
bool operator==(nullptr_t np, const unique_function<Sign>& f) noexcept {
    return f == np;
}

template <typename Sign>
bool operator!=(const unique_function<Sign>& f, nullptr_t np) noexcept {
    static_cast<void>(np);
    return static_cast<bool>(f);
}

template <typename Sign>
bool operator!=(nullptr_t np, const unique_function<Sign>& f) noexcept {
    return f != np;
}

/** @} */ // FunctionWrapper

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FUNCTIONAL_UNIQUE_FUNCTION_HPP__
