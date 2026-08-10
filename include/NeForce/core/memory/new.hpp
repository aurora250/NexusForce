#ifndef NEFORCE_CORE_MEMORY_NEW_HPP__
#define NEFORCE_CORE_MEMORY_NEW_HPP__
#include "NeForce/core/typeinfo/type_traits.hpp"
#include <new>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @brief 获取经过编译器优化屏障的指针
 * @tparam T 指针指向的对象类型
 * @param ptr 需要屏蔽编译器优化的指针
 * @return 指针
 * @warning 调用者必须保证内存中已存在有效对象
 * @warning 不能是 void 或函数类型
 *
 * 防止编译器基于对象生命周期或别名分析进行优化。
 */
template <typename T>
NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr T* launder(T* ptr) noexcept {
    static_assert(!is_function_v<T> && !is_void_v<T>, "T must not be void and not function.");
    return __builtin_launder(ptr);
}

#ifdef NEFORCE_STANDARD_17
template <typename Ret, typename... Args>
void launder(Ret (*)(Args...) noexcept) = delete;
#endif
template <typename Ret, typename... Args>
void launder(Ret (*)(Args...)) = delete;

void launder(void*) = delete;
void launder(const void*) = delete;
void launder(volatile void*) = delete;
void launder(const volatile void*) = delete;


/**
 * @brief 断言指针指向的内存中存在有效对象
 * @tparam T 期望的对象类型
 * @param p 指向内存的指针
 * @return 指针
 * @warning 调用者必须保证内存中已存在有效对象
 * @note 不构造对象
 */
template <typename T>
NEFORCE_ALWAYS_INLINE constexpr T* assume_lifetime(void* p) noexcept {
    return _NEFORCE launder(static_cast<T*>(p));
}

NEFORCE_END_NAMESPACE__
#endif //NEFORCE_CORE_MEMORY_NEW_HPP__
