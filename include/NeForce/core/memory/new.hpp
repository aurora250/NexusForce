#ifndef NEFORCE_CORE_MEMORY_NEW_HPP__
#define NEFORCE_CORE_MEMORY_NEW_HPP__
#include "NeForce/core/typeinfo/type_traits.hpp"
#include <new>
#ifdef NEFORCE_USING_MEMORY_POOL_OVERRIDE

/**
 * @defgroup GlobalAllocator 全局分配器
 * @brief 全局 operator new / delete 替换
 *
 * 本组声明仅在构建选项 NEXUSFORCE_MEMORY_POOL_GLOBAL_OVERRIDE 打开时生效，
 * 用于把标准全局分配也交给内存池。启用它需要满足全部条件：
 *
 * - 编译期：进程内所有会跨模块传递所有权的 C++ 模块都必须同样启用本覆盖，否则一个模块在池上分配、另一个模块用 CRT 释放，会出现未定义行为；
 * - 运行库：全部模块必须使用动态 C 运行库（/MD 或 /MDd）；
 * - 第三方模块：预编译的 C++ DLL 无法参与本覆盖，必须改为静态链接或移出进程；
 *
 * @note 只使用本库容器的进程无需打开本覆盖。
 * @warning 不满足上述条件时不要启用，跨模块释放会破坏堆。
 * @{
 */

/**
 * @brief 全局分配函数
 * @param size 请求字节数
 * @return 已分配内存指针
 * @throws allocate_exception 当内存不足时抛出
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(_NEFORCE size_t size);

/**
 * @brief 全局数组分配函数
 * @param size 请求字节数
 * @return 已分配内存指针
 * @throws allocate_exception 当内存不足时抛出
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](_NEFORCE size_t size);

/**
 * @brief 全局分配函数（不抛出异常）
 * @param size 请求字节数
 * @return 已分配内存指针，失败时返回 nullptr
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(_NEFORCE size_t size, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局数组分配函数（不抛出异常）
 * @param size 请求字节数
 * @return 已分配内存指针，失败时返回 nullptr
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](_NEFORCE size_t size, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局释放函数
 * @param ptr 待释放指针
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr) noexcept;

/**
 * @brief 全局数组释放函数
 * @param ptr 待释放指针
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr) noexcept;

/**
 * @brief 全局释放函数（带尺寸）
 * @param ptr 待释放指针
 * @param size 分配时的字节数
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, _NEFORCE size_t size) noexcept;

/**
 * @brief 全局数组释放函数（带尺寸）
 * @param ptr 待释放指针
 * @param size 分配时的字节数
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, _NEFORCE size_t size) noexcept;

/**
 * @brief 全局释放函数（不抛出异常版本）
 * @param ptr 待释放指针
 * @param tag 不抛出标记
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局数组释放函数（不抛出异常版本）
 * @param ptr 待释放指针
 * @param tag 不抛出标记
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局对齐分配函数
 * @param size 请求字节数
 * @param align 对齐要求
 * @return 已分配内存指针
 * @throws allocate_exception 当内存不足时抛出
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(_NEFORCE size_t size, std::align_val_t align);

/**
 * @brief 全局数组对齐分配函数
 * @param size 请求字节数
 * @param align 对齐要求
 * @return 已分配内存指针
 * @throws allocate_exception 当内存不足时抛出
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](_NEFORCE size_t size, std::align_val_t align);

/**
 * @brief 全局对齐分配函数（不抛出异常）
 * @param size 请求字节数
 * @param align 对齐要求
 * @param tag 不抛出标记
 * @return 已分配内存指针，失败时返回 nullptr
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new(_NEFORCE size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局数组对齐分配函数（不抛出异常）
 * @param size 请求字节数
 * @param align 对齐要求
 * @param tag 不抛出标记
 * @return 已分配内存指针，失败时返回 nullptr
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void* operator new[](_NEFORCE size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局对齐释放函数
 * @param ptr 待释放指针
 * @param align 分配时的对齐要求
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, std::align_val_t align) noexcept;

/**
 * @brief 全局数组对齐释放函数
 * @param ptr 待释放指针
 * @param align 分配时的对齐要求
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, std::align_val_t align) noexcept;

/**
 * @brief 全局对齐释放函数（带尺寸）
 * @param ptr 待释放指针
 * @param size 分配时的字节数
 * @param align 分配时的对齐要求
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, _NEFORCE size_t size, std::align_val_t align) noexcept;

/**
 * @brief 全局数组对齐释放函数（带尺寸）
 * @param ptr 待释放指针
 * @param size 分配时的字节数
 * @param align 分配时的对齐要求
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, _NEFORCE size_t size, std::align_val_t align) noexcept;

/**
 * @brief 全局对齐释放函数（不抛出异常版本）
 * @param ptr 待释放指针
 * @param align 分配时的对齐要求
 * @param tag 不抛出标记
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete(void* ptr, std::align_val_t align, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局数组对齐释放函数（不抛出异常版本）
 * @param ptr 待释放指针
 * @param align 分配时的对齐要求
 * @param tag 不抛出标记
 */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
void operator delete[](void* ptr, std::align_val_t align, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局对齐释放函数（带尺寸的不抛出异常版本）
 * @param ptr 待释放指针
 * @param size 分配时的字节数
 * @param align 分配时的对齐要求
 * @param tag 不抛出标记
 */
void operator delete(void* ptr, _NEFORCE size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept;

/**
 * @brief 全局数组对齐释放函数（带尺寸的不抛出异常版本）
 * @param ptr 待释放指针
 * @param size 分配时的字节数
 * @param align 分配时的对齐要求
 * @param tag 不抛出标记
 */
void operator delete[](void* ptr, _NEFORCE size_t size, std::align_val_t align, const std::nothrow_t& tag) noexcept;


/**
 * @brief 全局分配函数（不抛出异常）
 * @param size 请求字节数
 * @return 已分配内存指针，失败时返回 nullptr
 */
void* operator new(_NEFORCE size_t size, const _NEFORCE nothrow_t& tag) noexcept;

/**
 * @brief 全局数组分配函数（不抛出异常）
 * @param size 请求字节数
 * @return 已分配内存指针，失败时返回 nullptr
 */
void* operator new[](_NEFORCE size_t size, const _NEFORCE nothrow_t& tag) noexcept;

/**
 * @brief 全局对齐分配函数
 * @param size 请求字节数
 * @param align 对齐要求
 * @return 已分配内存指针
 * @throws allocate_exception 当内存不足时抛出
 */
void* operator new(_NEFORCE size_t size, _NEFORCE align_t align);

/**
 * @brief 全局数组对齐分配函数
 * @param size 请求字节数
 * @param align 对齐要求
 * @return 已分配内存指针
 * @throws allocate_exception 当内存不足时抛出
 */
void* operator new[](_NEFORCE size_t size, _NEFORCE align_t align);

/**
 * @brief 全局对齐分配函数（不抛出异常）
 * @param size 请求字节数
 * @param align 对齐要求
 * @param tag 不抛出标记
 * @return 已分配内存指针，失败时返回 nullptr
 */
void* operator new(_NEFORCE size_t size, _NEFORCE align_t align, const _NEFORCE nothrow_t& tag) noexcept;

/**
 * @brief 全局数组对齐分配函数（不抛出异常）
 * @param size 请求字节数
 * @param align 对齐要求
 * @param tag 不抛出标记
 * @return 已分配内存指针，失败时返回 nullptr
 */
void* operator new[](_NEFORCE size_t size, _NEFORCE align_t align, const _NEFORCE nothrow_t& tag) noexcept;

/** @} */ // GlobalAllocator

#endif // NEFORCE_USING_MEMORY_POOL_OVERRIDE

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
