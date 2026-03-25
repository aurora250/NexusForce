#ifndef NEFORCE_CORE_REFLECT_FUNCTION_HPP__
#define NEFORCE_CORE_REFLECT_FUNCTION_HPP__
#include "NeForce/core/container//vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/reflect/any.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

class meta_function {
public:
    using invoker = function<any(void*, const vector<any>&)>;

private:
    string_view name_;
    invoker invoker_;
    size_t min_args_ = 0;
    size_t max_args_ = 0;

public:
    meta_function(string_view name, invoker invoker)
    : name_(name), invoker_(move(invoker)) {}

    NEFORCE_NODISCARD string_view name() const noexcept { return name_; }

    any invoke(void* obj, const vector<any>& args) const {
        if (!invoker_) return any{};
        try {
            return invoker_(move(obj), args);
        } catch (...) {
            return any{};
        }
    }

    any invoke(void* obj) const noexcept {
        return invoke(obj, {});
    }

    NEFORCE_NODISCARD size_t min_args() const { return min_args_; }
    NEFORCE_NODISCARD size_t max_args() const { return max_args_; }

    void set_arg_hints(size_t min, size_t max) {
        min_args_ = min;
        max_args_ = max;
    }
};

NEFORCE_END_REFLECT__

NEFORCE_BEGIN_INNER__

template <typename Ret, typename Class, typename... Args, size_t... Is>
Ret invoke_impl(Class* obj, Ret (Class::*func)(Args...), const vector<reflect::any>& args, index_sequence<Is...>) {
    return (obj->*func)(args[Is].template convert<Args>()...);
}

template <typename Ret, typename Class, typename... Args, size_t... Is>
Ret invoke_impl(const Class* obj, Ret (Class::*func)(Args...) const, const vector<reflect::any>& args, index_sequence<Is...>) {
    return (obj->*func)(args[Is].template convert<Args>()...);
}

template <typename Ret, typename Class, typename... Args>
decltype(auto) make_member_invoker(Ret (Class::*func)(Args...)) {
    return [func](void* obj, const vector<reflect::any>& args) -> reflect::any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }

        if constexpr (is_void_v<Ret>) {
            inner::invoke_impl(static_cast<Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
            return reflect::any{};
        } else {
            auto result = inner::invoke_impl(static_cast<Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
            return reflect::any(result);
        }
    };
}

template <typename Ret, typename Class, typename... Args>
decltype(auto) make_const_member_invoker(Ret (Class::*func)(Args...) const) {
    return [func](void* obj, const vector<reflect::any>& args) -> reflect::any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }

        if constexpr (is_void_v<Ret>) {
            inner::invoke_impl(static_cast<const Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
            return reflect::any{};
        } else {
            auto result = inner::invoke_impl(static_cast<const Class*>(obj), func, args, make_index_sequence<sizeof...(Args)>{});
            return reflect::any(result);
        }
    };
}

template <typename Ret, typename... Args>
decltype(auto) make_static_invoker(Ret (*func)(Args...)) {
    return [func](void*, const vector<reflect::any>& args) -> reflect::any {
        if (sizeof...(Args) != args.size()) {
            NEFORCE_THROW_EXCEPTION(value_exception("Argument count mismatch"));
        }

        if constexpr (is_void_v<Ret>) {
            inner::invoke_impl(nullptr, func, args, make_index_sequence<sizeof...(Args)>{});
            return reflect::any{};
        } else {
            auto result = inner::invoke_impl(nullptr, func, args, make_index_sequence<sizeof...(Args)>{});
            return reflect::any(result);
        }
    };
}

NEFORCE_END_INNER__

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_FUNCTION_HPP__
