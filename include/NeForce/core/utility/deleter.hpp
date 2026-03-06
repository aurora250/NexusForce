#ifndef NEFORCE_CORE_UTILITY_DELETER_HPP__
#define NEFORCE_CORE_UTILITY_DELETER_HPP__

/**
 * @file deleter.hpp
 * @brief 智能指针删除器
 *
 * 此文件提供了智能指针使用的默认删除器，支持单个对象和数组的删除。
 * 删除器是可定制的，可以通过模板特化或派生来提供自定义的删除逻辑。
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Deleter 删除器
 * @brief 智能指针使用的删除器
 * @{
 */

/**
 * @struct default_delete
 * @brief 默认删除器
 * @tparam T 元素类型
 *
 * 使用delete运算符释放单个对象的默认删除器。
 */
template <typename T>
struct default_delete {
    constexpr default_delete() noexcept = default;  ///< 默认构造函数

    /**
     * @brief 从其他default_delete转换构造
     * @tparam U 可转换为T*的类型
     */
    template <typename U, enable_if_t<is_convertible<U*, T*>::value, int> = 0>
    NEFORCE_CONSTEXPR20 default_delete(const default_delete<U>&) noexcept {}

    /**
     * @brief 删除操作符
     * @param ptr 要删除的指针
     */
    NEFORCE_CONSTEXPR20 void operator ()(const T* ptr) const noexcept {
        delete ptr;
    }

    /**
     * @brief 重新绑定到其他类型的删除器
     * @tparam U 新的元素类型
     * @return 绑定到U的新删除器
     */
    template <typename U>
    NEFORCE_CONSTEXPR20 default_delete<U> rebind() && noexcept {
        return default_delete<U>();
    }
};

/**
 * @brief 数组特化的默认删除器
 * @tparam T 数组元素类型
 */
template <typename T>
struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;  ///< 默认构造函数

    /**
     * @brief 从其他数组删除器转换构造
     * @tparam U 可转换为T的数组类型
     */
    template <typename U, enable_if_t<is_convertible<U(*)[], T(*)[]>::value, int> = 0>
    NEFORCE_CONSTEXPR20 default_delete(const default_delete<U[]>&) noexcept {}

    /**
     * @brief 删除操作符
     * @tparam U 数组元素类型
     * @param ptr 要删除的数组指针
     */
    template <typename U, enable_if_t<is_convertible<U(*)[], T(*)[]>::value, int> = 0>
    NEFORCE_CONSTEXPR20 void operator ()(U* ptr) const noexcept {
        delete [] ptr;
    }

    /**
     * @brief 重新绑定到其他数组类型的删除器
     * @tparam U 新的数组元素类型
     * @return 绑定到U[]的新删除器
     */
    template <typename U>
    NEFORCE_CONSTEXPR20 default_delete<U[]> rebind() && noexcept {
        return default_delete<U[]>();
    }
};

/** @} */ // Deleter

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_DELETER_HPP__
