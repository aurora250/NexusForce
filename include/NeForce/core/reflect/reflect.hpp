#ifndef NEFORCE_CORE_REFLECT_REFLECT_HPP__
#define NEFORCE_CORE_REFLECT_REFLECT_HPP__

/**
 * @file reflect.hpp
 * @brief 反射系统主入口
 *
 * 此文件提供了反射系统的统一入口，包含类型构建器和便利宏。
 * 用户通过reflect函数和宏来注册类型的反射信息。
 */

#include "NeForce/core/reflect/registry.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_REFLECT__

/**
 * @defgroup Reflection 反射系统
 * @brief 运行时类型反射系统
 * @{
 */

/**
 * @class type_builder
 * @brief 类型反射构建器
 * @tparam T 要注册的类型
 *
 * 提供链式API用于构建类型的反射信息。
 * 通过反射函数创建，用于注册基类、属性、函数和构造函数。
 */
template <typename T> class type_builder {
private:
    meta_type& meta_; ///< 关联的元数据对象

    template <typename... Args, size_t... Is>
    static meta_any create_with_args(const vector<meta_any>& args, index_sequence<Is...>) {
        return meta_any(T(args[Is].template convert<Args>()...));
    }

public:
    /**
     * @brief 构造函数
     * @param name 类型名称
     */
    explicit type_builder(string_view name) :
    meta_(registry::instance().register_type<T>(name)) {}

    /**
     * @brief 添加基类（已解析）
     * @tparam Base 基类类型
     * @return 自身引用
     */
    template <typename Base> type_builder& base() {
        meta_.base_type(&registry::instance().register_type<Base>(type_name<Base>::value));
        return *this;
    }

    /**
     * @brief 添加基类（延迟解析）
     * @param base_name 基类名称
     * @return 自身引用
     */
    type_builder& base(string_view base_name) {
        meta_.base_type(base_name);
        return *this;
    }

    /**
     * @brief 添加属性
     * @tparam U 属性类型
     * @param name 属性名称
     * @param member 成员指针
     * @return 自身引用
     */
    template <typename U> type_builder& property(string_view name, U T::* member) {
        meta_property::getter getter = [member](void* obj) -> meta_any {
            auto* instance = static_cast<T*>(obj);
            return meta_any(instance->*member);
        };

        meta_property::setter setter = [member](void* obj, const meta_any& value) {
            auto* instance = static_cast<T*>(obj);
            auto* val = value.cast<U>();
            if (val) {
                instance->*member = *val;
            }
        };

        meta_.property(name, type_name_v<U>.to_hash(), move(getter), move(setter));
        return *this;
    }

    /**
     * @brief 添加成员函数
     * @tparam Ret 返回值类型
     * @tparam Args 参数类型列表
     * @param name 函数名称
     * @param func 成员函数指针
     * @return 自身引用
     */
    template <typename Ret, typename... Args> type_builder& function(string_view name, Ret (T::*func)(Args...)) {
        auto invoker = inner::make_member_invoker(func);
        auto* meta_func = meta_.function(name, _NEFORCE move(invoker));
        if (meta_func) {
            meta_func->set_arg_hints(sizeof...(Args), sizeof...(Args));
        }
        return *this;
    }

    /**
     * @brief 添加常量成员函数
     * @tparam Ret 返回值类型
     * @tparam Args 参数类型列表
     * @param name 函数名称
     * @param func 常量成员函数指针
     * @return 自身引用
     */
    template <typename Ret, typename... Args> type_builder& function(string_view name, Ret (T::*func)(Args...) const) {
        auto invoker = inner::make_const_member_invoker(func);
        auto* meta_func = meta_.function(name, _NEFORCE move(invoker));
        if (meta_func) {
            meta_func->set_arg_hints(sizeof...(Args), sizeof...(Args));
        }
        return *this;
    }

    /**
     * @brief 添加静态函数
     * @tparam Ret 返回值类型
     * @tparam Args 参数类型列表
     * @param name 函数名称
     * @param func 函数指针
     * @return 自身引用
     */
    template <typename Ret, typename... Args> type_builder& static_function(string_view name, Ret (*func)(Args...)) {
        auto invoker = inner::make_static_invoker(func);
        auto* meta_func = meta_.function(name, _NEFORCE move(invoker));
        meta_func->set_arg_hints(sizeof...(Args), sizeof...(Args));
        return *this;
    }

    /**
     * @brief 添加默认构造函数
     * @return 自身引用
     *
     * 要求类型是默认可构造的。
     */
    type_builder& constructor() {
        static_assert(is_default_constructible_v<T>, "Constructor must be default constructible");
        meta_.constructor([](const vector<meta_any>&) -> meta_any { return meta_any(T{}); });
        return *this;
    }

    /**
     * @brief 添加带参数构造函数
     * @tparam Args 参数类型列表
     * @return 自身引用
     *
     * 要求类型可从Args类型列表构造。
     */
    template <typename... Args> type_builder& constructor() {
        static_assert(is_constructible_v<T, Args...>, "Constructor must be constructible from Args");
        meta_.constructor([](const vector<meta_any>& args) -> meta_any {
            if (args.size() != sizeof...(Args)) {
                NEFORCE_THROW_EXCEPTION(value_exception("Constructor argument count mismatch"));
            }
            return type_builder::create_with_args<Args...>(args, make_index_sequence<sizeof...(Args)>{});
        });
        return *this;
    }

    /**
     * @brief 获取元数据对象
     * @return 元数据引用
     */
    NEFORCE_NODISCARD meta_type& meta() noexcept { return meta_; }

    /**
     * @brief 获取常量元数据对象
     * @return 元数据常量引用
     */
    NEFORCE_NODISCARD const meta_type& meta() const noexcept { return meta_; }
};

/**
 * @brief 创建类型反射构建器
 * @tparam T 要注册的类型
 * @param name 类型名称
 * @return 类型构建器
 */
template <typename T> type_builder<T> reflect(string_view name) { return type_builder<T>(name); }


/**
 * @brief 注册类型（无基类）
 * @param Class 类名
 * @param Name 注册名
 *
 * 注册一个没有基类的类型。
 */
#define NEFORCE_REFLECT_REGISTER_N(Class, Name) \
    static auto _neforce_reflect_##Class = _NEFORCE reflect::reflect<Class>(Name)

/**
 * @brief 注册类型（带基类）
 * @param Class 类名
 * @param Base 基类名
 * @param Name 注册名
 *
 * 注册一个有基类的类型，自动解析继承关系。
 */
#define NEFORCE_REFLECT_REGISTER_N_DERIVED(Class, Base, Name)                  \
    static auto _neforce_reflect_##Class = []() {                              \
        _NEFORCE reflect::registry::instance().resolve_all_bases();            \
        return _NEFORCE reflect::reflect<Class>(#Class).template base<Base>(); \
    }()

/**
 * @brief 注册类型（无基类）
 * @param Class 类名
 *
 * 注册一个没有基类的类型。
 */
#define NEFORCE_REFLECT_REGISTER(Class) NEFORCE_REFLECT_REGISTER_N(Class, #Class)

/**
 * @brief 注册类型（带基类）
 * @param Class 类名
 * @param Base 基类名
 *
 * 注册一个有基类的类型，自动解析继承关系。
 */
#define NEFORCE_REFLECT_REGISTER_DERIVED(Class, Base) NEFORCE_REFLECT_REGISTER_N_DERIVED(Class, Base, #Class)

/** @} */ // Reflection

NEFORCE_END_REFLECT__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_REFLECT_REFLECT_HPP__
