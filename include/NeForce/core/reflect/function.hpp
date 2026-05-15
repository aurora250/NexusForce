#ifndef NEFORCE_CORE_REFLECT_FUNCTION_HPP__
#define NEFORCE_CORE_REFLECT_FUNCTION_HPP__

/**
 * @file function.hpp
 * @brief 函数反射元数据
 *
 * 此文件提供了函数反射的元数据类，用于描述和调用成员函数和静态函数。
 */

#include "NeForce/core/container//vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/reflect/any.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @class meta_function
 * @brief 函数反射元数据类
 *
 * 描述一个可调用函数的元信息，支持通过反射调用该函数。
 */
class meta_function {
public:
    using invoker = function<meta_any(void*, const vector<meta_any>&)>; ///< 函数调用器类型

private:
    string_view name_;    ///< 函数名称
    invoker invoker_;     ///< 调用器
    size_t min_args_ = 0; ///< 最小参数数量
    size_t max_args_ = 0; ///< 最大参数数量

public:
    /**
     * @brief 构造函数
     * @param name 函数名称
     * @param invoker 调用器
     */
    meta_function(string_view name, invoker invoker) :
    name_(name),
    invoker_(move(invoker)) {}

    /**
     * @brief 获取函数名称
     * @return 名称视图
     */
    NEFORCE_NODISCARD string_view name() const noexcept { return name_; }

    /**
     * @brief 调用函数
     * @param obj 对象指针
     * @param args 参数列表
     * @return 返回值包装为meta_any
     */
    meta_any invoke(void* obj, const vector<meta_any>& args) const {
        if (!invoker_) {
            return meta_any{};
        }
        try {
            return invoker_(move(obj), args);
        } catch (...) {
            return meta_any{};
        }
    }

    /**
     * @brief 调用无参函数
     * @param obj 对象指针
     * @return 返回值包装为meta_any
     */
    meta_any invoke(void* obj) const noexcept { return invoke(obj, {}); }

    /**
     * @brief 获取最小参数数量
     * @return 最小参数数量
     */
    NEFORCE_NODISCARD size_t min_args() const { return min_args_; }

    /**
     * @brief 获取最大参数数量
     * @return 最大参数数量
     */
    NEFORCE_NODISCARD size_t max_args() const { return max_args_; }

    /**
     * @brief 设置参数数量提示
     * @param min 最小参数数量
     * @param max 最大参数数量
     */
    void set_arg_hints(size_t min, size_t max) {
        min_args_ = min;
        max_args_ = max;
    }
};

/** @} */ // Reflection

NEFORCE_END_REFLECT__

NEFORCE_BEGIN_INNER__

template <typename Ret, typename Class, typename... Args, size_t... Is>
Ret invoke_impl(Class* obj, Ret (Class::*func)(Args...), const vector<reflect::meta_any>& args,
                index_sequence<Is...> /*unused*/) {
    return (obj->*func)(args[Is].template convert<Args>()...);
}

template <typename Ret, typename Class, typename... Args, size_t... Is>
Ret invoke_impl(const Class* obj, Ret (Class::*func)(Args...) const, const vector<reflect::meta_any>& args,
                index_sequence<Is...> /*unused*/) {
    return (obj->*func)(args[Is].template convert<Args>()...);
}

template <typename Ret, typename Class, typename... Args, enable_if_t<is_void_v<Ret>, int> = 0>
decltype(auto) make_member_invoker(Ret (Class::*func)(Args...)) {
    return [func](void* obj, const vector<reflect::meta_any>& args) -> reflect::meta_any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }
        inner::invoke_impl(static_cast<Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
        return reflect::meta_any{};
    };
}

template <typename Ret, typename Class, typename... Args, enable_if_t<!is_void_v<Ret>, int> = 0>
decltype(auto) make_member_invoker(Ret (Class::*func)(Args...)) {
    return [func](void* obj, const vector<reflect::meta_any>& args) -> reflect::meta_any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }
        auto result = inner::invoke_impl(static_cast<Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
        return reflect::meta_any(result);
    };
}

template <typename Ret, typename Class, typename... Args, enable_if_t<is_void_v<Ret>, int> = 0>
decltype(auto) make_const_member_invoker(Ret (Class::*func)(Args...) const) {
    return [func](void* obj, const vector<reflect::meta_any>& args) -> reflect::meta_any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }
        inner::invoke_impl(static_cast<const Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
        return reflect::meta_any{};
    };
}

template <typename Ret, typename Class, typename... Args, enable_if_t<!is_void_v<Ret>, int> = 0>
decltype(auto) make_const_member_invoker(Ret (Class::*func)(Args...) const) {
    return [func](void* obj, const vector<reflect::meta_any>& args) -> reflect::meta_any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }
        auto result =
                inner::invoke_impl(static_cast<const Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
        return reflect::meta_any(result);
    };
}

template <typename Ret, typename... Args, enable_if_t<is_void_v<Ret>, int> = 0>
decltype(auto) make_static_invoker(Ret (*func)(Args...)) {
    return [func](void*, const vector<reflect::meta_any>& args) -> reflect::meta_any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }
        auto result = inner::invoke_impl(nullptr, func, args, make_index_sequence<sizeof...(Args)>{});
        return reflect::meta_any(result);
    };
}

template <typename Ret, typename... Args, enable_if_t<!is_void_v<Ret>, int> = 0>
decltype(auto) make_static_invoker(Ret (*func)(Args...)) {
    return [func](void*, const vector<reflect::meta_any>& args) -> reflect::meta_any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }
        auto result = inner::invoke_impl(nullptr, func, args, make_index_sequence<sizeof...(Args)>{});
        return reflect::meta_any(result);
    };
}

NEFORCE_END_INNER__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_FUNCTION_HPP__
