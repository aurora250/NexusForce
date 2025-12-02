#ifndef MSTL_CORE_ALGORITHM_TYPE_ERASE_HPP__
#define MSTL_CORE_ALGORITHM_TYPE_ERASE_HPP__
#include "../typeinfo/types.hpp"
#include "../iterator/reverse_iterator.hpp"
#include <initializer_list>
MSTL_BEGIN_NAMESPACE__

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) begin(Container& cont) noexcept(noexcept(cont.begin())) {
	return cont.begin();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) begin(const Container& cont) noexcept(noexcept(cont.begin())) {
	return cont.begin();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) end(Container& cont) noexcept(noexcept(cont.end())) {
	return cont.end();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) end(const Container& cont) noexcept(noexcept(cont.end())) {
	return cont.end();
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* begin(T (&arr)[Size]) noexcept {
	return arr;
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* end(T (&arr)[Size]) noexcept {
	return arr + Size;
}

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) cbegin(const Container& cont) noexcept(noexcept(cont.cbegin())) {
	return cont.cbegin();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) cend(const Container& cont) noexcept(noexcept(cont.cend())) {
	return cont.cend();
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
const T* cbegin(T (&arr)[Size]) noexcept {
    return arr;
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
const T* cend(T (&arr)[Size]) noexcept {
    return arr + Size;
}

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rbegin(Container& cont) noexcept(noexcept(cont.rbegin())) {
	return cont.rbegin();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rbegin(const Container& cont) noexcept(noexcept(cont.rbegin())) {
	return cont.rbegin();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rend(Container& cont) noexcept(noexcept(cont.rend())) {
	return cont.rend();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) rend(const Container& cont) noexcept(noexcept(cont.rend())) {
	return cont.rend();
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<T*> rbegin(T (&arr)[Size]) noexcept {
	return reverse_iterator<T*>(arr + Size);
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<T*> rend(T (&arr)[Size]) noexcept {
	return reverse_iterator<T*>(arr);
}
template <typename T>
MSTL_NODISCARD constexpr
reverse_iterator<const T*> rbegin(std::initializer_list<T> lls) noexcept {
	return reverse_iterator<const T*>(lls.end());
}
template <typename T>
MSTL_NODISCARD constexpr
reverse_iterator<const T*> rend(std::initializer_list<T> lls) noexcept {
	return reverse_iterator<const T*>(lls.begin());
}

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) crbegin(const Container& cont) noexcept(noexcept(cont.crbegin())) {
	return cont.crbegin();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) crend(const Container& cont) noexcept(noexcept(cont.crend())) {
	return cont.crend();
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<const T*> crbegin(T (&arr)[Size]) noexcept {
    return reverse_iterator<const T*>(arr + Size);
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
reverse_iterator<const T*> crend(T (&arr)[Size]) noexcept {
    return reverse_iterator<const T*>(arr);
}

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) size(const Container& cont) noexcept(noexcept(cont.size())) {
	return cont.size();
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) size(T (&)[Size]) noexcept {
	return Size;
}

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) empty(const Container& cont) noexcept(noexcept(cont.empty())) {
	return cont.empty();
}
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
bool empty(const T* ptr) noexcept {
	return ptr != nullptr;
}
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
bool empty(std::initializer_list<T> lls) noexcept {
	return lls.size() == 0;
}


template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) data(Container& cont) noexcept(noexcept(cont.data())) {
	return cont.data();
}
template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) data(const Container& cont) noexcept(noexcept(cont.data())) {
	return cont.data();
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* data(T (& arr)[Size]) noexcept {
	return arr;
}
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
T* data(T* ptr) noexcept {
	return ptr;
}
template <typename T>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
const T* data(std::initializer_list<T> lls) noexcept {
	return lls.begin();
}

template <typename Container>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
decltype(auto) ssize(const Container& cont) noexcept(noexcept(cont.size())) {
	using type = make_signed_t<decltype(cont.size())>;
	return static_cast<common_type_t<ptrdiff_t, type>>(cont.size());
}
template <typename T, size_t Size>
MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr
ptrdiff_t ssize(T (&)[Size]) noexcept {
	return Size;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_ALGORITHM_TYPE_ERASE_HPP__
