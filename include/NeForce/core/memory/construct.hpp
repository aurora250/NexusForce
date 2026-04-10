#ifndef NEFORCE_CORE_MEMORY_CONSTRUCT_HPP__
#define NEFORCE_CORE_MEMORY_CONSTRUCT_HPP__

/**
 * @file construct.hpp
 * @brief 内存构造和销毁函数
 *
 * 此文件提供了对象构造和销毁的通用函数，支持在已分配的内存上构造对象，
 * 以及批量销毁对象。包括对平凡可析构类型的优化。
 */

#include "NeForce/core/typeinfo/concepts.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup InplaceMemoryFunction 内存就地操作函数
 * @brief 在指定内存位置构造对象
 * @{
 */

/**
 * @brief 在指定内存位置构造对象
 * @tparam T 要构造的对象类型
 * @tparam Args 构造参数类型
 * @param ptr 指向已分配内存的指针
 * @param args 构造参数
 * @return 指向构造对象的void指针
 *
 * 使用定位new在指定内存位置构造对象，支持完美转发构造参数。
 * 仅当T可以从Args...构造时才启用此重载。
 */
template <typename T, typename... Args>
NEFORCE_CONSTEXPR20 T* construct(T* ptr, Args&&... args) noexcept(is_nothrow_constructible_v<T, Args...>) {
    static_assert(is_constructible_v<T, Args...>, "T must be constructible with arguments");
    return static_cast<T*>(new (static_cast<void*>(ptr)) T(_NEFORCE forward<Args>(args)...));
}

/**
 * @brief 销毁单个对象
 * @tparam T 对象类型
 * @param pointer 指向要销毁对象的指针
 *
 * 显式调用对象的析构函数，但不释放内存。
 */
template <typename T>
NEFORCE_CONSTEXPR20 void destroy(T* pointer) noexcept(is_nothrow_destructible_v<T>) {
    pointer->~T();
}

/**
 * @brief 销毁迭代器范围内的对象序列
 * @tparam Iterator 迭代器类型
 * @param first 范围的起始迭代器
 * @param last 范围的结束迭代器
 *
 * 遍历迭代器范围，对每个元素调用析构函数。
 * 仅当迭代器值类型非平凡可析构时才启用此重载。
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 enable_if_t<is_iter_v<Iterator> && !is_trivially_destructible_v<iter_value_t<Iterator>>>
destroy(Iterator first, Iterator last) noexcept(is_nothrow_destructible_v<iter_value_t<Iterator>>) {
    for (; first < last; ++first) {
        _NEFORCE destroy(&*first);
    }
    return;
}

/**
 * @brief 销毁迭代器范围内的对象序列
 * @tparam Iterator 迭代器类型
 * @param first 范围的起始迭代器
 * @param last 范围的结束迭代器
 *
 * 对于平凡可析构的类型，不需要执行任何操作，直接返回。
 * 这是对平凡可析构类型的优化。
 */
template <typename Iterator>
NEFORCE_CONSTEXPR20 enable_if_t<is_iter_v<Iterator> && is_trivially_destructible_v<iter_value_t<Iterator>>>
destroy(Iterator first, Iterator last) noexcept {
    return;
}


template <typename T>
struct temporary_guard {
    static_assert(is_nothrow_move_constructible_v<T>, "T must be nothrow move constructible");

private:
    T* guarded_ptr;
    T temp;

public:
    constexpr explicit temporary_guard(T& value) :
    guarded_ptr(_NEFORCE addressof(value)),
    temp(_NEFORCE move(value)) {
        _NEFORCE destroy(guarded_ptr);
    }

    NEFORCE_CONSTEXPR20 ~temporary_guard() {
        if (guarded_ptr) {
            _NEFORCE construct(guarded_ptr, _NEFORCE move(temp));
        }
    }

    temporary_guard(const temporary_guard&) = delete;
    temporary_guard& operator=(const temporary_guard&) = delete;

    constexpr T&& release() noexcept {
        guarded_ptr = nullptr;
        return _NEFORCE move(temp);
    }
};

template <typename NT, typename OT, typename Arg>
constexpr enable_if_t<is_nothrow_constructible_v<NT, Arg>>
reinitialize(NT* new_val, OT* old_val, Arg&& arg) noexcept(is_nothrow_constructible_v<NT, Arg>) {
    _NEFORCE destroy(old_val);
    _NEFORCE construct(new_val, _NEFORCE forward<Arg>(arg));
    return;
}

template <typename NT, typename OT, typename Arg>
constexpr enable_if_t<!is_nothrow_constructible_v<NT, Arg> && is_nothrow_move_constructible_v<NT>>
reinitialize(NT* new_val, OT* old_val, Arg&& arg) noexcept(is_nothrow_constructible_v<NT, Arg>) {
    NT temp(_NEFORCE forward<Arg>(arg));
    _NEFORCE destroy(old_val);
    _NEFORCE construct(new_val, _NEFORCE move(temp));
    return;
}

template <typename NT, typename OT, typename Arg>
constexpr enable_if_t<!is_nothrow_constructible_v<NT, Arg> && !is_nothrow_move_constructible_v<NT>>
reinitialize(NT* new_val, OT* old_val, Arg&& arg) noexcept(is_nothrow_constructible_v<NT, Arg>) {
    temporary_guard<OT> guard(*old_val);
    _NEFORCE construct(new_val, _NEFORCE forward<Arg>(arg));
    guard.release();
    return;
}

/** @} */ // InplaceMemoryFunction

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_CONSTRUCT_HPP__
