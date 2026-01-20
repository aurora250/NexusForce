#ifndef MSTL_CORE_ALGORITHM_TYPE_ERASE_HPP__
#define MSTL_CORE_ALGORITHM_TYPE_ERASE_HPP__

/**
 * @file type_erase.hpp
 * @brief MSTL类型擦除辅助函数
 *
 * 此文件提供了类型擦除辅助函数实现，
 * 用于统一访问不同容器的迭代器、大小、数据等接口。
 */

#include "../iterator/reverse_iterator.hpp"
#include <initializer_list>
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup TypeErasureFunctions 类型擦除函数
 * @brief 容器和数组的通用访问函数
 * @{
 */

/**
 * @brief 获取容器的起始迭代器
 * @tparam Container 容器类型
 * @param cont 容器引用
 * @return 容器的起始迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) begin(Container& cont)
noexcept(noexcept(cont.begin())) {
	return cont.begin();
}

/**
 * @brief 获取容器的结束迭代器
 * @tparam Container 容器类型
 * @param cont 容器引用
 * @return 容器的结束迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) end(Container& cont)
noexcept(noexcept(cont.end())) {
	return cont.end();
}

/**
 * @brief 获取const容器的起始迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的起始迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) begin(const Container& cont)
noexcept(noexcept(cont.begin())) {
	return cont.begin();
}

/**
 * @brief 获取const容器的结束迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的结束迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) end(const Container& cont)
noexcept(noexcept(cont.end())) {
	return cont.end();
}

/**
 * @brief 获取数组的起始指针
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指向数组首元素的指针
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* begin(T (&arr)[Size]) noexcept {
	return arr;
}

/**
 * @brief 获取数组的结束指针
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指向数组末尾的指针
 * @note 末尾指针是最后一个元素之后的位置
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* end(T (&arr)[Size]) noexcept {
	return arr + Size;
}

/**
 * @brief 获取const容器的const起始迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的const起始迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) cbegin(const Container& cont) noexcept(noexcept(cont.cbegin())) {
	return cont.cbegin();
}

/**
 * @brief 获取const容器的const结束迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的const结束迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) cend(const Container& cont) noexcept(noexcept(cont.cend())) {
	return cont.cend();
}

/**
 * @brief 获取const数组的const起始指针
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr const数组引用
 * @return 指向数组首元素的const指针
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
const T* cbegin(T (&arr)[Size]) noexcept {
    return arr;
}

/**
 * @brief 获取const数组的const结束指针
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr const数组引用
 * @return 指向数组末尾的const指针
 * @note 末尾指针是最后一个元素之后的位置
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
const T* cend(T (&arr)[Size]) noexcept {
    return arr + Size;
}

/**
 * @brief 获取容器的反向起始迭代器
 * @tparam Container 容器类型
 * @param cont 容器引用
 * @return 容器的反向起始迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rbegin(Container& cont) noexcept(noexcept(cont.rbegin())) {
	return cont.rbegin();
}

/**
 * @brief 获取const容器的反向起始迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的反向起始迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rend(Container& cont) noexcept(noexcept(cont.rend())) {
	return cont.rend();
}

/**
 * @brief 获取容器的反向结束迭代器
 * @tparam Container 容器类型
 * @param cont 容器引用
 * @return 容器的反向结束迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rbegin(const Container& cont) noexcept(noexcept(cont.rbegin())) {
	return cont.rbegin();
}

/**
 * @brief 获取const容器的反向结束迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的反向结束迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rend(const Container& cont) noexcept(noexcept(cont.rend())) {
	return cont.rend();
}

/**
 * @brief 获取数组的反向起始迭代器
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 数组的反向起始迭代器
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<T*> rbegin(T (&arr)[Size]) noexcept {
	return reverse_iterator<T*>(arr + Size);
}

/**
 * @brief 获取数组的反向结束迭代器
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 数组的反向结束迭代器
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<T*> rend(T (&arr)[Size]) noexcept {
	return reverse_iterator<T*>(arr);
}

/**
 * @brief 获取初始化列表的反向起始迭代器
 * @tparam T 元素类型
 * @param lls 初始化列表
 * @return 初始化列表的反向起始迭代器
 */
template <typename T>
MSTL_NODISCARD constexpr
reverse_iterator<const T*> rbegin(std::initializer_list<T> lls) noexcept {
	return reverse_iterator<const T*>(lls.end());
}

/**
 * @brief 获取初始化列表的反向结束迭代器
 * @tparam T 元素类型
 * @param lls 初始化列表
 * @return 初始化列表的反向结束迭代器
 */
template <typename T>
MSTL_NODISCARD constexpr
reverse_iterator<const T*> rend(std::initializer_list<T> lls) noexcept {
	return reverse_iterator<const T*>(lls.begin());
}

/**
 * @brief 获取const容器的const反向起始迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的const反向起始迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) crbegin(const Container& cont) noexcept(noexcept(cont.crbegin())) {
	return cont.crbegin();
}

/**
 * @brief 获取const容器的const反向结束迭代器
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return const容器的const反向结束迭代器
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) crend(const Container& cont) noexcept(noexcept(cont.crend())) {
	return cont.crend();
}

/**
 * @brief 获取const数组的const反向起始迭代器
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr const数组引用
 * @return const数组的const反向起始迭代器
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<const T*> crbegin(T (&arr)[Size]) noexcept {
    return reverse_iterator<const T*>(arr + Size);
}

/**
 * @brief 获取const数组的const反向结束迭代器
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr const数组引用
 * @return const数组的const反向结束迭代器
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<const T*> crend(T (&arr)[Size]) noexcept {
    return reverse_iterator<const T*>(arr);
}

/**
 * @brief 获取容器的大小
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return 容器中元素的数量
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) size(const Container& cont) noexcept(noexcept(cont.size())) {
	return cont.size();
}

/**
 * @brief 获取数组的大小
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 数组的大小
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
size_t size(T (& arr)[Size]) noexcept {
	return Size;
}

/**
 * @brief 获取初始化列表的大小
 * @tparam T 元素类型
 * @param lls 初始化列表
 * @return 初始化列表的大小
 */
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
size_t size(std::initializer_list<T> lls) noexcept {
	return lls.size();
}

/**
 * @brief 获取容器的有符号大小
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return 容器中元素的数量
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) ssize(const Container& cont) noexcept(noexcept(cont.size())) {
	using type = make_signed_t<decltype(cont.size())>;
	return static_cast<common_type_t<ptrdiff_t, type>>(cont.size());
}

/**
 * @brief 获取数组的有符号大小
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 数组的大小
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
ptrdiff_t ssize(T (& arr)[Size]) noexcept {
	return Size;
}

/**
 * @brief 获取初始化列表的有符号大小
 * @tparam T 元素类型
 * @param lls 初始化列表
 * @return 初始化列表的大小
 */
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
ptrdiff_t ssize(std::initializer_list<T> lls) noexcept {
	return lls.size();
}

/**
 * @brief 检查容器是否为空
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return 如果容器为空则返回true，否则返回false
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
bool empty(const Container& cont) noexcept(noexcept(cont.empty())) {
	return cont.empty();
}

/**
 * @brief 检查指针是否非空
 * @tparam T 指针指向的类型
 * @param ptr 指针
 * @return 如果指针非空则返回true，否则返回false
 */
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
bool empty(const T* ptr) noexcept {
	return ptr != nullptr;
}

/**
 * @brief 检查初始化列表是否为空
 * @tparam T 元素类型
 * @param lls 初始化列表
 * @return 如果初始化列表为空则返回true，否则返回false
 */
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
bool empty(std::initializer_list<T> lls) noexcept {
	return lls.size() == 0;
}

/**
 * @brief 获取容器的底层数据指针
 * @tparam Container 容器类型
 * @param cont 容器引用
 * @return 指向容器底层数据的指针
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) data(Container& cont)
noexcept(noexcept(cont.data())) {
	return cont.data();
}

/**
 * @brief 获取const容器的const底层数据指针
 * @tparam Container 容器类型
 * @param cont const容器引用
 * @return 指向const容器底层数据的const指针
 */
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) data(const Container& cont)
noexcept(noexcept(cont.data())) {
	return cont.data();
}

/**
 * @brief 获取数组的底层数据指针
 * @tparam T 数组元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指向数组首元素的指针
 */
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* data(T (& arr)[Size]) noexcept {
	return arr;
}

/**
 * @brief 获取指针本身
 * @tparam T 指针指向的类型
 * @param ptr 指针
 * @return 原指针
 */
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* data(T* ptr) noexcept {
	return ptr;
}

/**
 * @brief 获取初始化列表的底层数据指针
 * @tparam T 元素类型
 * @param lls 初始化列表
 * @return 指向初始化列表数据的const指针
 */
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
const T* data(std::initializer_list<T> lls) noexcept {
	return lls.begin();
}

/** @} */ // TypeErasureFunctions

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_TYPE_ERASE_HPP__
