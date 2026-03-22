#ifndef NEFORCE_CORE_REFLECT_REFLECT_HPP__
#define NEFORCE_CORE_REFLECT_REFLECT_HPP__
#include "NeForce/core/reflect/registry.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

template <typename T>
class type_builder {
private:
    meta_type& meta_;

    template <typename... Args, size_t... Is>
    static any create_with_args(const vector<any>& args, index_sequence<Is...>) {
        return any(T(args[Is].template convert<Args>()...));
    }

public:
    explicit type_builder(string_view name)
    : meta_(registry::instance().register_type<T>(name)) {}

    template <typename Base>
    type_builder& base() {
        meta_.base_type(&registry::instance().register_type<Base>(type_name<Base>::value));
        return *this;
    }

    type_builder& base(string_view base_name) {
        meta_.base_type(base_name);
        return *this;
    }

    template <typename U>
    type_builder& property(string_view name, U T::*member) {
        meta_property::getter getter = [member](void* obj) -> any {
            auto* instance = static_cast<T*>(obj);
            return any(instance->*member);
        };

        meta_property::setter setter = [member](void* obj, const any& value) {
            auto* instance = static_cast<T*>(obj);
            auto* val = value.cast<U>();
            if (val) {
                instance->*member = *val;
            }
        };

        meta_.property(name, type_name_v<U>.to_hash(), move(getter), move(setter));
        return *this;
    }

    template <typename Ret, typename... Args>
    type_builder& function(string_view name, Ret(T::*func)(Args...)) {
        auto invoker = _INNER make_member_invoker(func);
        auto* meta_func = meta_.function(name, _NEFORCE move(invoker));
        if (meta_func) {
            meta_func->set_arg_hints(sizeof...(Args), sizeof...(Args));
        }
        return *this;
    }

    template <typename Ret, typename... Args>
    type_builder& function(string_view name, Ret(T::*func)(Args...) const) {
        auto invoker = _INNER make_const_member_invoker(func);
        auto* meta_func = meta_.function(name, _NEFORCE move(invoker));
        if (meta_func) {
            meta_func->set_arg_hints(sizeof...(Args), sizeof...(Args));
        }
        return *this;
    }

    template <typename Ret, typename... Args>
    type_builder& static_function(string_view name, Ret (*func)(Args...)) {
        auto invoker = _INNER make_static_invoker(func);
        auto* meta_func = meta_.function(name, _NEFORCE move(invoker));
        meta_func->set_arg_hints(sizeof...(Args), sizeof...(Args));
        return *this;
    }

    type_builder& constructor() {
        static_assert(is_default_constructible_v<T>, "Constructor must be default constructible");
        meta_.constructor([](const vector<any>&) -> any {
            return any(T{});
        });
        return *this;
    }

    template <typename... Args>
    type_builder& constructor() {
        static_assert(is_constructible_v<T, Args...>, "Constructor must be constructible from Args");
        meta_.constructor([](const vector<any>& args) -> any {
            if (args.size() != sizeof...(Args)) {
                NEFORCE_THROW_EXCEPTION(value_exception("Constructor argument count mismatch"));
            }
            return type_builder::create_with_args<Args...>(args, make_index_sequence<sizeof...(Args)>{});
        });
        return *this;
    }

    meta_type& meta() noexcept { return meta_; }
    const meta_type& meta() const noexcept { return meta_; }
};

template <typename T>
type_builder<T> reflect(string_view name) {
    return type_builder<T>(name);
}


#define REFLECT_REGISTER_N(Class, Name) \
    static auto _reflect_##Class = _REFLECT reflect<Class>(Name)

#define REFLECT_REGISTER_N_DERIVED(Class, Base, Name) \
    static auto _reflect_##Class = []() { \
        _REFLECT registry::instance().resolve_all_bases(); \
        return _REFLECT reflect<Class>(#Class).template base<Base>(); \
    }()

#define REFLECT_REGISTER(Class) \
    REFLECT_REGISTER_N(Class, #Class)

#define REFLECT_REGISTER_DERIVED(Class, Base) \
    REFLECT_REGISTER_N_DERIVED(Class, Base, #Class)

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_REFLECT_HPP__
