#ifndef NEFORCE_CORE_UTILITY_VARIANT_HPP__
#define NEFORCE_CORE_UTILITY_VARIANT_HPP__

/**
 * @file variant.hpp
 * @brief 变体类型实现
 *
 * 此文件提供了变体类型的实现，用于表示一个类型安全的多类型容器，
 * 可以存储和操作多种不同类型的值
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/exception/exception.hpp"
#include "NeForce/core/functional/invoke.hpp"
#include "NeForce/core/utility/none.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Variant 变体
 * @brief 变体类及其辅助函数实现
 * @{
 */

/**
 * @struct variant_index
 * @brief 获取类型在变体中的索引
 * @tparam Variant 变体类型
 * @tparam T 要查找的类型
 */
template <typename Variant, typename T>
struct variant_index;

#ifdef NEFORCE_STANDARD_14
/**
 * @var variant_index_v
 * @brief variant_index的便捷别名，获取类型在变体中的索引值
 */
template <typename Variant, typename T>
NEFORCE_INLINE17 constexpr size_t variant_index_v = variant_index<Variant, T>::value;
#endif

/**
 * @struct variant_alternative
 * @brief 获取变体中指定索引位置的类型
 * @tparam Variant 变体类型
 * @tparam Idx 索引位置
 */
template <typename Variant, size_t Idx>
struct variant_alternative;

/**
 * @typedef variant_alternative_t
 * @brief variant_alternative的便捷别名，获取变体中指定索引位置的类型
 */
template <typename Variant, size_t Idx>
using variant_alternative_t = typename variant_alternative<Variant, Idx>::type;


/**
 * @struct variant
 * @brief 变体类型主模板
 * @tparam Types 可存储的类型列表
 *
 * variant是一个类型安全的联合体，可以在运行时存储多种不同类型的值。
 * 使用函数指针表实现各种操作，支持构造、赋值、访问、销毁等操作。
 */
template <typename... Types>
struct variant : icommon<variant<Types...>> {
    static_assert(sizeof...(Types) > 0, "variant must have at least one type");

    static_assert(
        !is_any_of<none_t, Types...>::value ||
        (is_any_of<none_t, Types...>::value && is_same<get_first_para_t<Types...>, none_t>::value),
        "if variant holds non, it should be at the first place");

private:
    size_t index_ = 0;  ///< 当前存储类型的索引

    /**
     * @brief 存储值的联合体内存
     *
     * 使用最大对齐和最大大小的字符数组存储实际数据。
     * 对齐方式为所有类型中最大的对齐要求。
     * 大小为所有类型中最大的大小。
     */
    alignas(_NEFORCE max({ alignof(Types)... })) byte_t union_[_NEFORCE max({ sizeof(Types)... })]{};

    using destruct_function = void(*)(byte_t*);  ///< 析构函数指针类型

    /**
     * @brief 获取析构函数表
     * @return 析构函数指针数组
     *
     * 为每个类型生成对应的析构函数，存储在静态数组中。
     */
    static destruct_function* destructors_table() noexcept {
        static destruct_function function_ptrs[sizeof...(Types)] = {
            [](byte_t* union_p) noexcept {
                reinterpret_cast<Types*>(union_p)->~Types();
            }...
        };
        return function_ptrs;
    }

    using copy_construct_function = void(*)(byte_t*, byte_t const*);   ///< 拷贝构造函数指针类型

    /**
     * @brief 获取拷贝构造函数表
     * @return 拷贝构造函数指针数组
     *
     * 为每个类型生成对应的拷贝构造函数，存储在静态数组中。
     */
    static copy_construct_function* copy_constructors_table() noexcept {
        static copy_construct_function function_ptrs[sizeof...(Types)] = {
            [](byte_t* union_dst, byte_t const* union_src) noexcept {
                new (union_dst) Types(*reinterpret_cast<Types const*>(union_src));
            }...
        };
        return function_ptrs;
    }

    using copy_assignment_function = void(*)(byte_t*, byte_t const*);  ///< 拷贝赋值函数指针类型

    /**
     * @brief 获取拷贝赋值函数表
     * @return 拷贝赋值函数指针数组
     *
     * 为每个类型生成对应的拷贝赋值函数，存储在静态数组中。
     */
    static copy_assignment_function* copy_assigment_functions_table() noexcept {
        static copy_assignment_function function_ptrs[sizeof...(Types)] = {
            [](byte_t* union_dst, byte_t const* union_src) noexcept {
                *reinterpret_cast<Types*>(union_dst) = *reinterpret_cast<Types const*>(union_src);
            }...
        };
        return function_ptrs;
    }

    using move_construct_function = void(*)(byte_t*, const byte_t*);  ///< 移动构造函数指针类型

    /**
     * @brief 获取移动构造函数表
     * @return 移动构造函数指针数组
     *
     * 为每个类型生成对应的移动构造函数，存储在静态数组中。
     */
    static move_construct_function* move_constructors_table() noexcept {
        static move_construct_function function_ptrs[sizeof...(Types)] = {
            [](byte_t* union_dst, const byte_t* union_src) noexcept {
                new (union_dst) Types(_NEFORCE move(*reinterpret_cast<const Types*>(union_src)));
            }...
        };
        return function_ptrs;
    }

    using move_assignment_function = void(*)(byte_t*, byte_t*);  ///< 移动赋值函数指针类型

    /**
     * @brief 获取移动赋值函数表
     * @return 移动赋值函数指针数组
     *
     * 为每个类型生成对应的移动赋值函数，存储在静态数组中。
     */
    static move_assignment_function* move_assigment_functions_table() noexcept {
        static move_assignment_function function_ptrs[sizeof...(Types)] = {
            [](byte_t* union_dst, byte_t* union_src) noexcept {
                *reinterpret_cast<Types*>(union_dst) = _NEFORCE move(*reinterpret_cast<Types*>(union_src));
            }...
        };
        return function_ptrs;
    }

    template <typename Lambda>
    using const_visitor_function = common_type_t<
        _NEFORCE invoke_result_t<Lambda, Types const&>...>(*)(byte_t const*, Lambda&&);  ///< 常量访问者函数指针类型

    /**
     * @brief 获取常量访问者函数表
     * @return 常量访问者函数指针数组
     * @tparam Lambda 访问者类型
     *
     * 为每个类型生成对应的常量访问者函数，存储在静态数组中。
     */
    template <typename Lambda>
    static const_visitor_function<Lambda>* const_visitors_table() noexcept {
        static const_visitor_function<Lambda> function_ptrs[sizeof...(Types)] = {
            [](byte_t const* union_p, Lambda&& lambda) -> _NEFORCE invoke_result_t<Lambda, Types const&> {
                return _NEFORCE invoke(_NEFORCE forward<Lambda>(lambda), *reinterpret_cast<Types const*>(union_p));
            }...
        };
        return function_ptrs;
    }

    template <typename Lambda>
    using visitor_function = common_type_t<
        _NEFORCE invoke_result_t<Lambda, Types&>...>(*)(byte_t*, Lambda&&);  ///< 访问者函数指针类型

    /**
     * @brief 获取访问者函数表
     * @return 访问者函数指针数组
     * @tparam Lambda 访问者类型
     *
     * 为每个类型生成对应的访问者函数，存储在静态数组中。
     */
    template <typename Lambda>
    static visitor_function<Lambda>* visitors_table() noexcept {
        static visitor_function<Lambda> function_ptrs[sizeof...(Types)] = {
            [](byte_t* union_p, Lambda&& lambda) -> common_type_t<_NEFORCE invoke_result_t<Lambda, Types&>...> {
                return _NEFORCE invoke(_NEFORCE forward<Lambda>(lambda), *reinterpret_cast<Types*>(union_p));
            }...
        };
        return function_ptrs;
    }

    template <size_t I, typename... Args, enable_if_t<
        is_constructible_v<variant_alternative_t<variant, I>, Args...>, int> = 0>
    constexpr bool try_construct_impl_aux_aux(Args&&... args) {
        index_ = I;
        new (union_) variant_alternative_t<variant, I>(_NEFORCE forward<Args>(args)...);
        return true;
    }

    template <size_t I, typename... Args, enable_if_t<
        !is_constructible_v<variant_alternative_t<variant, I>, Args...>, int> = 0>
    constexpr bool try_construct_impl_aux_aux(Args&&... args) {
        return variant::try_construct_impl<I + 1>(_NEFORCE forward<Args>(args)...);
    }

    template <size_t I, typename... Args, enable_if_t<(I < sizeof...(Types)), int> = 0>
    constexpr bool try_construct_impl_aux(Args&&... args) {
        return variant::try_construct_impl_aux_aux<I>(_NEFORCE forward<Args>(args)...);
    }

    template <size_t I, typename... Args, enable_if_t<(I >= sizeof...(Types)), int> = 0>
    constexpr bool try_construct_impl_aux(Args&&...) {
        return false;
    }

    template <size_t I, typename... Args>
    constexpr bool try_construct_impl(Args&&... args) {
        return variant::try_construct_impl_aux<I>(_NEFORCE forward<Args>(args)...);
    }

    template <size_t I = 0, typename... Args>
    constexpr bool try_construct(Args&&... args) {
        return variant::try_construct_impl<I>(_NEFORCE forward<Args>(args)...);
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 默认构造变体，存储第一个类型的默认值。
     */
    NEFORCE_CONSTEXPR20 variant()
    noexcept(is_nothrow_default_constructible_v<variant_alternative_t<variant, 0>>) {
        new (union_) variant_alternative_t<variant, 0>();
    }

    /**
     * @brief 从特定类型值构造
     * @tparam T 值类型
     * @param value 要存储的值
     *
     * 从给定类型的值移动构造变体。
     */
    template <typename T, enable_if_t<disjunction_v<is_same<T, Types>...>, int> = 0>
    NEFORCE_CONSTEXPR20 explicit variant(T&& value)
    noexcept(is_nothrow_move_constructible_v<T>)
    : index_(variant_index_v<variant, T>) {
        T* p = reinterpret_cast<T*>(union_);
        new (p) T(_NEFORCE forward<T>(value));
    }

    /**
     * @brief 拷贝构造函数
     * @param other 要拷贝的变体
     *
     * 从另一个变体拷贝构造当前变体。
     */
    NEFORCE_CONSTEXPR20 variant(const variant& other)
    : index_(other.index_) {
        copy_constructors_table()[index()](union_, other.union_);
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 要拷贝的变体
     * @return 当前变体的引用
     *
     * 将另一个变体的值拷贝赋值给当前变体。
     */
    NEFORCE_CONSTEXPR20 variant& operator =(const variant& other) {
        if(_NEFORCE addressof(other) == this) return *this;
        index_ = other.index_;
        copy_assigment_functions_table()[index()](union_, other.union_);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 要移动的变体
     *
     * 从另一个变体移动构造当前变体。
     */
    NEFORCE_CONSTEXPR20 variant(variant&& other) noexcept
    : index_(other.index_) {
        move_constructors_table()[index()](union_, other.union_);
    }

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的变体
     * @return 当前变体的引用
     *
     * 将另一个变体的值移动赋值给当前变体。
     */
    NEFORCE_CONSTEXPR20 variant& operator =(variant&& other) noexcept {
        if(_NEFORCE addressof(other) == this) return *this;
        index_ = other.index_;
        move_assigment_functions_table()[index()](union_, other.union_);
        return *this;
    }

    /**
     * @brief 参数列表原地构造函数
     * @tparam Idx 要构造的类型的索引
     * @tparam Args 参数类型
     * @param args 构造参数
     *
     * 在指定索引位置直接构造对象。
     */
    template <size_t Idx, typename... Args,
        enable_if_t<is_constructible_v<variant_alternative_t<variant, Idx>, Args...>, int> = 0>
    NEFORCE_CONSTEXPR20 explicit variant(inplace_construct_tag, Args&&... args)
    noexcept(is_nothrow_constructible_v<variant_alternative_t<variant, Idx>, Args...>)
    : index_(Idx) {
        new (union_) variant_alternative_t<variant, Idx>(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 初始化列表原地构造函数
     * @tparam Idx 要构造的类型的索引
     * @tparam U 初始化列表元素类型
     * @tparam Args 参数类型
     * @param ilist 初始化列表
     * @param args 构造参数
     *
     * 使用初始化列表在指定索引位置构造对象。
     */
    template <size_t Idx, typename U, typename... Args,
        enable_if_t<is_constructible_v<variant_alternative_t<variant, Idx>, std::initializer_list<U>&, Args...>, int> = 0>
    NEFORCE_CONSTEXPR20 explicit variant(inplace_construct_tag, std::initializer_list<U> ilist, Args&&... args)
    noexcept(is_nothrow_constructible_v<variant_alternative_t<variant, Idx>, std::initializer_list<U>&, Args...>)
    : index_(Idx) {
        new (union_) variant_alternative_t<variant, Idx>(ilist, _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 通用构造函数
     * @tparam Args 参数类型
     * @param args 构造参数
     *
     * 尝试使用给定参数构造变体，如果无法构造任何类型，则使用第一个类型的默认值。
     */
    template <typename... Args, enable_if_t<disjunction_v<is_constructible<Types, Args...>...>, int> = 0>
    variant(Args&&... args) {
        if (!variant::try_construct(_NEFORCE forward<Args>(args)...)) {
            index_ = 0;
            new (union_) variant_alternative_t<variant, 0>();
        }
    }

    /**
     * @brief 析构函数
     *
     * 调用当前存储类型的析构函数。
     */
    NEFORCE_CONSTEXPR20 ~variant() noexcept {
        destructors_table()[index()](union_);
    }

    /**
     * @brief 访问变体值
     * @tparam Lambda 访问者类型
     * @param lambda 访问者函数
     * @return 访问者函数的返回值
     *
     * 使用访问者模式访问当前存储的值。
     */
    template <typename Lambda, enable_if_t<conjunction_v<is_invocable<Lambda, Types&>...>, int> = 0>
    NEFORCE_CONSTEXPR20 common_type_t<invoke_result_t<Lambda, Types&>...> visit(Lambda&& lambda)
    noexcept(conjunction_v<is_nothrow_invocable<Lambda, Types&>...>) {
        return visitors_table<Lambda>()[index()](union_, _NEFORCE forward<Lambda>(lambda));
    }

    /**
     * @brief 常量访问变体值
     * @tparam Lambda 访问者类型
     * @param lambda 访问者函数
     * @return 访问者函数的返回值
     *
     * 使用访问者模式访问当前存储的常量值。
     */
    template <typename Lambda, enable_if_t<conjunction_v<is_invocable<Lambda, const Types&>...>, int> = 0>
    NEFORCE_CONSTEXPR20 common_type_t<invoke_result_t<Lambda, const Types&>...> visit(Lambda&& lambda) const
    noexcept(conjunction_v<is_nothrow_invocable<Lambda, const Types&>...>) {
        return const_visitors_table<Lambda>()[index()](union_, _NEFORCE forward<Lambda>(lambda));
    }

    /**
     * @brief 获取当前存储类型的索引
     * @return 当前存储类型的索引
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_t index() const noexcept {
        return index_;
    }

    /**
     * @brief 检查是否存储特定类型的值
     * @tparam T 要检查的类型
     * @return 如果存储的是指定类型返回true，否则返回false
     */
    template <typename T, enable_if_t<is_any_of_v<T, Types...>, int> = 0>
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool holds_alternative() const noexcept {
        return variant_index_v<variant, T> == index_;
    }

    /**
     * @brief 获取指定索引位置的引用
     * @tparam Idx 索引位置
     * @return 指定位置元素的引用
     * @throws value_exception 如果索引不匹配
     */
    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    NEFORCE_CONSTEXPR20 variant_alternative_t<variant, Idx>& get() {
        if(index_ != Idx) {
            NEFORCE_THROW_EXCEPTION(value_exception("Template index not match."));
        }
        return *reinterpret_cast<variant_alternative_t<variant, Idx>*>(union_);
    }

    /**
     * @brief 获取指定类型值的引用
     * @tparam T 类型
     * @return 指定类型值的引用
     * @throws value_exception 如果类型不匹配
     */
    template <typename T>
    NEFORCE_CONSTEXPR20 T& get() {
        return variant::get<variant_index_v<variant, T>>();
    }

    /**
     * @brief 获取指定索引位置的常量引用
     * @tparam Idx 索引位置
     * @return 指定位置元素的常量引用
     * @throws value_exception 如果索引不匹配
     */
    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    NEFORCE_CONSTEXPR20 variant_alternative_t<variant, Idx> const& get() const {
        if(index_ != Idx) {
            NEFORCE_THROW_EXCEPTION(value_exception("Template index not match."));
        }
        return *reinterpret_cast<variant_alternative_t<variant, Idx> const*>(union_);
    }

    /**
     * @brief 获取指定类型值的常量引用
     * @tparam T 类型
     * @return 指定类型值的常量引用
     * @throws value_exception 如果类型不匹配
     */
    template <typename T>
    NEFORCE_CONSTEXPR20 T const& get() const {
        return get<variant_index_v<variant, T>>();
    }

    /**
     * @brief 如果存在，获取指定索引位置的指针
     * @tparam Idx 索引位置
     * @return 指定位置元素的指针，如果索引不匹配返回nullptr
     */
    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    NEFORCE_CONSTEXPR20 variant_alternative_t<variant, Idx>* get_if() noexcept {
        if (index_ != Idx) return nullptr;
        return reinterpret_cast<variant_alternative_t<variant, Idx>*>(union_);
    }

    /**
     * @brief 如果存在，获取指定类型值的指针
     * @tparam T 类型
     * @return 指定类型值的指针，如果类型不匹配返回nullptr
     */
    template <typename T>
    NEFORCE_CONSTEXPR20 T* get_if() noexcept {
        return get_if<variant_index_v<variant, T>>();
    }

    /**
     * @brief 如果存在，获取指定索引位置的常量指针
     * @tparam Idx 索引位置
     * @return 指定位置元素的常量指针，如果索引不匹配返回nullptr
     */
    template <size_t Idx, enable_if_t<(Idx < sizeof...(Types)), int> = 0>
    NEFORCE_CONSTEXPR20 variant_alternative_t<variant, Idx> const* get_if() const noexcept {
        if (index_ != Idx) return nullptr;
        return reinterpret_cast<variant_alternative_t<variant, Idx> const*>(union_);
    }

    /**
     * @brief 如果存在，获取指定类型值的常量指针
     * @tparam T 类型
     * @return 指定类型值的常量指针，如果类型不匹配返回nullptr
     */
    template <typename T>
    NEFORCE_CONSTEXPR20 T const* get_if() const noexcept {
        return get_if<variant_index_v<variant, T>>();
    }

    /**
     * @brief 在指定位置构造新值
     * @tparam Idx 索引位置
     * @tparam Args 参数类型
     * @param args 构造参数
     *
     * 销毁当前值并在指定位置构造新值。
     */
    template <size_t Idx, typename... Args,
        enable_if_t<(Idx < sizeof...(Types)) && is_constructible_v<variant_alternative_t<variant, Idx>, Args...>, int> = 0>
    NEFORCE_CONSTEXPR20 void emplace(Args&&... args)
    noexcept(is_nothrow_constructible_v<variant_alternative_t<variant, Idx>, Args...>) {
        destructors_table()[index()](union_);
        index_ = Idx;
        new (union_) variant_alternative_t<variant, Idx>(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 构造指定类型的新值
     * @tparam T 类型
     * @tparam Args 参数类型
     * @param args 构造参数
     *
     * 销毁当前值并在指定类型位置构造新值。
     */
    template <typename T, typename... Args, enable_if_t<is_constructible_v<T, Args...>, int> = 0>
    NEFORCE_CONSTEXPR20 void emplace(Args&&... args)
    noexcept(is_nothrow_constructible_v<T, Args...>) {
        variant::emplace<variant_index_v<variant, T>>(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 交换两个变体的内容
     * @param other 要交换的变体
     *
     * 交换当前变体和另一个变体的值。
     */
    NEFORCE_CONSTEXPR20 void swap(variant& other) noexcept {
        if (_NEFORCE addressof(other) == this) return;

        size_t this_index = index_;
        const size_t other_index = other.index_;
        alignas(_NEFORCE max({ alignof(Types)... })) byte_t temp_union[_NEFORCE max({ sizeof(Types)... })];
        move_constructors_table()[this_index](reinterpret_cast<byte_t*>(temp_union), union_);
        destructors_table()[this_index](union_);

        move_constructors_table()[other_index](union_, other.union_);
        destructors_table()[other_index](other.union_);
        move_constructors_table()[this_index](other.union_, reinterpret_cast<byte_t*>(temp_union));
        destructors_table()[this_index](reinterpret_cast<byte_t*>(temp_union));

        index_ = other_index;
        other.index_ = this_index;
    }

    /**
     * @brief 相等比较运算符
     * @param rhs 要比较的变体
     * @return 如果两个变体相等返回true，否则返回false
     *
     * 两个变体相等当且仅当它们存储相同类型的值且值相等。
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator ==(const variant& rhs) const {
        if (index_ != rhs.index_) return false;
        return this->visit([&](const auto& value) {
            return rhs.visit([&](const auto& other_value) {
                return value == other_value;
            });
        });
    }

    /**
     * @brief 小于比较运算符
     * @param rhs 要比较的变体
     * @return 如果当前变体小于另一个变体返回true，否则返回false
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool operator <(const variant& rhs) const {
        if (index_ != rhs.index_) return false;
        return this->visit([&](const auto& value) {
            return rhs.visit([&](const auto& other_value) {
                return value < other_value;
            });
        });
    }

    /**
     * @brief 计算变体的哈希值
     * @return 变体的哈希值
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_t to_hash() const;
};

#ifdef NEFORCE_STANDARD_17
template <typename... Args>
variant(Args...) -> variant<Args...>;
#endif

/// @cond
template <typename T, typename ...Types>
struct variant_alternative<variant<T, Types...>, 0> {
    using type = T;
};
template <typename T, typename ...Types, size_t Idx>
struct variant_alternative<variant<T, Types...>, Idx> {
    using type = typename variant_alternative<variant<Types...>, Idx - 1>::type;
};

template <typename T, typename ...Types>
struct variant_index<variant<T, Types...>, T> {
    static constexpr size_t value = 0;
};
template <typename T0, typename T, typename ...Types>
struct variant_index<variant<T0, Types...>, T> {
    static constexpr size_t value = variant_index<variant<Types...>, T>::value + 1;
};
/// @endcond

/**
 * @brief 获取变体中指定索引位置的引用
 * @tparam Idx 索引位置
 * @tparam Types 变体类型列表
 * @param v 变体对象
 * @return 指定位置元素的引用
 */
template <size_t Idx, typename... Types>
NEFORCE_CONSTEXPR20 variant_alternative_t<variant<Types...>, Idx>& get(variant<Types...>& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<T&>(static_cast<variant_type&>(v).template get<Idx>());
}

/**
 * @brief 获取变体中指定索引位置的常量引用
 * @tparam Idx 索引位置
 * @tparam Types 变体类型列表
 * @param v 变体对象
 * @return 指定位置元素的常量引用
 */
template <size_t Idx, typename... Types>
NEFORCE_CONSTEXPR20 const variant_alternative_t<variant<Types...>, Idx>& get(const variant<Types...>& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<const T&>(static_cast<const variant_type&>(v).template get<Idx>());
}

/**
 * @brief 获取变体中指定索引位置的右值引用
 * @tparam Idx 索引位置
 * @tparam Types 变体类型列表
 * @param v 变体对象
 * @return 指定位置元素的右值引用
 */
template <size_t Idx, typename... Types>
NEFORCE_CONSTEXPR20 variant_alternative_t<variant<Types...>, Idx>&& get(variant<Types...>&& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<T&&>(static_cast<variant_type&&>(v).template get<Idx>());
}

/**
 * @brief 获取变体中指定索引位置的常量右值引用
 * @tparam Idx 索引位置
 * @tparam Types 变体类型列表
 * @param v 变体对象
 * @return 指定位置元素的常量右值引用
 */
template <size_t Idx, typename... Types>
NEFORCE_CONSTEXPR20 const variant_alternative_t<variant<Types...>, Idx>&& get(const variant<Types...>&& v) {
    using T = variant_alternative_t<variant<Types...>, Idx>;
    using variant_type = variant<Types...>;
    return static_cast<const T&&>(static_cast<const variant_type&&>(v).template get<Idx>());
}


/// @cond

NEFORCE_BEGIN_INNER__
struct __variant_elem_hasher {
    template <typename T>
    constexpr size_t operator ()(const T& value) const {
        return hash<decay_t<T>>{}(value);
    }
};
NEFORCE_END_INNER__

template <typename... Types>
NEFORCE_CONSTEXPR20 size_t variant<Types...>::to_hash() const {
    constexpr _INNER __variant_elem_hasher hasher{};
    return variant::visit(hasher);
}

/// @endcond

/** @} */ // Variant

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_VARIANT_HPP__
