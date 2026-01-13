#ifndef MSTL_CORE_UTILITY_REFERENCE_WRAPPER_HPP__
#define MSTL_CORE_UTILITY_REFERENCE_WRAPPER_HPP__

/**
 * @file reference_wrapper.hpp
 * @brief MSTL引用包装器
 * @namespace MSTL
 * @ingroup ReferenceWrapper
 *
 * 此文件提供了引用包装器的实现，用于在容器和算法中存储引用，提供类型安全的引用包装功能。
 */

#include "../typeinfo/type_traits.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup RefWrapperConstruction 引用包装器构造检查
 * @brief 检查类型是否可以构造引用包装器的辅助工具
 * @{
 */

MSTL_BEGIN_INNER__
/// @cond

// 引用包装器构造检查的辅助函数
/**
 * @brief 检查是否可以构造左值引用包装器
 * @tparam T 目标类型
 * @note 仅用于SFINAE检测，不实现函数体
 */
template <typename T>
void __ref_wrapper_construct_aux(type_identity_t<T&>) noexcept;

/**
 * @brief 禁止构造右值引用包装器
 * @tparam T 目标类型
 * @note 删除右值版本，防止悬垂引用
 */
template <typename T>
void __ref_wrapper_construct_aux(type_identity_t<T&&>) = delete;

/// @endcond
MSTL_END_INNER__

/**
 * @struct ref_wrapper_constructable_from
 * @brief 检查是否可以从类型U构造reference_wrapper<T>
 * @tparam T 引用包装的目标类型
 * @tparam U 源类型
 * @tparam Dummy SFINAE参数，默认为void
 */
template <typename T, typename U, typename Dummy = void>
struct ref_wrapper_constructable_from : false_type {};

/// @cond
template <typename T, typename U>
struct ref_wrapper_constructable_from<T, U, void_t<
    decltype(_INNER __ref_wrapper_construct_aux<T>(_MSTL declval<U>()))>>
    : true_type {};
/// @endcond

#ifdef MSTL_STANDARD_14__
/**
 * @var ref_wrapper_constructable_from_v
 * @brief ref_wrapper_constructable_from的便捷变量模板
 */
template <typename T, typename U>
MSTL_INLINE17 constexpr bool ref_wrapper_constructable_from_v = ref_wrapper_constructable_from<T, U>::value;
#endif

/** @} */ // RefWrapperConstruction

MSTL_BEGIN_INNER__
template <typename F, typename... Args>
struct __invoke_result_aux;
MSTL_END_INNER__

template <typename F, typename... Args>
struct is_nothrow_invocable;

template <typename Callable, typename... Args>
MSTL_CONSTEXPR14 typename _INNER __invoke_result_aux<Callable, Args...>::type
invoke(Callable&& f, Args&&... args)
noexcept(is_nothrow_invocable<Callable, Args...>::value);

/**
 * @defgroup ReferenceWrapper 引用包装器类
 * @brief reference_wrapper类的定义和实现
 * @{
 */

/**
 * @class reference_wrapper
 * @brief 引用包装器类模板
 * @tparam T 被包装的类型
 *
 * 将引用包装为可复制的值类型，可以在容器中存储引用。
 * 支持隐式转换为原始引用，以及函数调用运算符。
 */
template <typename T>
class reference_wrapper {
public:
    static_assert(is_object<T>::value || is_function<T>::value,
        "reference_wrapper requires an object or function type.");

    using type = T; ///< 包装的类型

private:
    T* ptr_{}; ///< 指向包装对象的指针

public:
    /**
     * @brief 构造函数
     * @tparam U 源类型
     * @param x 要包装的引用
     * @throws 如果地址获取操作抛出异常
     *
     * 从任意可以转换为T引用的类型构造引用包装器。
     * 禁止从右值构造，防止悬垂引用。
     */
    template <typename U, enable_if_t<
        conjunction<negation<is_same<remove_cvref_t<U>, reference_wrapper>>,
            ref_wrapper_constructable_from<T, U>>::value, int> = 0>
    MSTL_CONSTEXPR14 reference_wrapper(U&& x)
        noexcept(noexcept(_INNER __ref_wrapper_construct_aux<T>(_MSTL declval<U>()))) {
        T& ref = static_cast<U&&>(x);
        ptr_ = _MSTL addressof(ref);
    }

    /**
     * @brief 隐式转换运算符
     * @return 包装的引用
     *
     * 允许reference_wrapper隐式转换为T&，方便使用。
     */
    MSTL_CONSTEXPR14 operator T &() const noexcept {
        return *ptr_;
    }

    /**
     * @brief 获取包装的引用
     * @return 包装的引用
     */
    MSTL_NODISCARD MSTL_CONSTEXPR14 T& get() const noexcept {
        return *ptr_;
    }

    /**
     * @brief 函数调用运算符
     * @tparam Args 参数类型
     * @param args 调用参数
     * @return 函数调用结果
     *
     * 如果T是可调用类型，可以通过reference_wrapper直接调用包装的函数。
     */
    template <typename... Args>
    MSTL_CONSTEXPR14 typename _INNER __invoke_result_aux<T&, Args...>::type
    operator ()(Args&&... args) const noexcept(is_nothrow_invocable<T&, Args...>::value) {
        return _MSTL invoke(this->get(), _MSTL forward<Args>(args)...);
    }
};

#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T>
reference_wrapper(T&) -> reference_wrapper<T>;
#endif

/** @} */ // ReferenceWrapper

/**
 * @defgroup RefHelperFunctions 引用辅助函数
 * @brief 创建引用包装器的便捷函数
 * @{
 */

/**
 * @brief 创建引用包装器
 * @tparam T 引用类型
 * @param val 要包装的左值引用
 * @return reference_wrapper<T>包装器
 * @note 禁止对右值使用，防止悬垂引用
 */
template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<T> ref(T& val) noexcept {
    return reference_wrapper<T>(val);
}

/**
 * @brief 删除const右值的ref重载
 * @tparam T 类型
 * @note 防止对const右值创建引用包装器
 */
template <typename T>
void ref(const T&&) = delete;

/**
 * @brief 重新包装已存在的引用包装器
 * @tparam T 引用类型
 * @param wrapper 已存在的引用包装器
 * @return 相同的引用包装器
 */
template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<T> ref(reference_wrapper<T> wrapper) noexcept {
    return wrapper;
}

/**
 * @brief 创建const引用包装器
 * @tparam T 引用类型
 * @param val 要包装的const左值引用
 * @return reference_wrapper<const T>包装器
 * @note 用于创建只读引用包装
 */
template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<const T> cref(const T& val) noexcept {
    return reference_wrapper<const T>(val);
}

/**
 * @brief 删除const右值的cref重载
 * @tparam T 类型
 */
template <typename T>
void cref(const T&&) = delete;

/**
 * @brief 重新包装为const引用包装器
 * @tparam T 引用类型
 * @param wrapper 已存在的引用包装器
 * @return const版本的引用包装器
 *
 * 将非const引用包装器转换为const版本。
 */
template <typename T>
MSTL_NODISCARD constexpr reference_wrapper<const T> cref(reference_wrapper<T> wrapper) noexcept {
    return wrapper;
}

/** @} */ // RefHelperFunctions

/**
 * @defgroup UnwrapReference 引用解包
 * @brief 解包引用包装器的类型特性
 * @{
 */

/**
 * @struct unwrap_reference
 * @brief 解包引用包装器，获取原始引用类型
 * @tparam T 要解包的类型
 */
template <typename T>
struct unwrap_reference {
    using type = T;
};

/**
 * @brief reference_wrapper的特化版本
 * @tparam T 包装的类型
 */
template <typename T>
struct unwrap_reference<reference_wrapper<T>> {
    using type = T&;
};

/**
 * @typedef unwrap_reference_t
 * @brief unwrap_reference的便捷别名
 */
template <typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;


/**
 * @typedef unwrap_ref_decay
 * @brief 先退化类型，再解包引用包装器
 * @tparam T 输入类型
 */
template <typename T>
struct unwrap_ref_decay {
    using type = unwrap_reference_t<decay_t<T>>;
};

/**
 * @typedef unwrap_ref_decay_t
 * @brief unwrap_ref_decay的便捷别名
 */
template <typename T>
using unwrap_ref_decay_t = typename unwrap_ref_decay<T>::type;

/** @} */ // UnwrapReference

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_UTILITY_REFERENCE_WRAPPER_HPP__
