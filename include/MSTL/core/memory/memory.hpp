#ifndef MSTL_CORE_MEMORY_MEMORY_HPP__
#define MSTL_CORE_MEMORY_MEMORY_HPP__
#include "../string/char_types.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_API size_t get_available_memory() noexcept;


#ifdef MSTL_COMPILER_GNUC__
	#define MSTL_MEMORY_BARRIER(p) __asm__ volatile ("" : : "m" (*p) : "memory");
#elif defined(MSTL_COMPILER_MSVC__)
	#define MSTL_MEMORY_BARRIER(p) ::_ReadWriteBarrier();
#else
	#define MSTL_MEMORY_BARRIER(p)
#endif

#ifdef MSTL_COMPILER_MSVC__
namespace masm {
	// These functions are defined in memory.asm and be packaged in masm namespace
	extern "C" {
		void* __cdecl masm_memory_copy(void* dest, const void* src, size_t count);
		void* __cdecl masm_memory_copy_offset(void* dest, const void* src, size_t count);
		void* __cdecl masm_memory_char_copy(void* dest, const void* src, int chr, size_t count);
		int __cdecl masm_memory_compare(const void* lh, const void* rh, size_t count);
		int __cdecl masm_memory_compare_ignore_case(const void* ptr1, const void* rh, size_t count);
		void* __cdecl masm_memory_char(const void* dest, int value, size_t count);
		void* __cdecl masm_memory_move(void* dest, const void* src, size_t count);
		void* __cdecl masm_memory_set(void* dest, int value, size_t count);
		void __cdecl masm_memory_zero(void* dest, size_t count);
		void __cdecl masm_explicit_memory_zero(void* dest, size_t count);
		void* __cdecl masm_memory_in_memory(void* data, size_t data_len, const void* pattern, size_t pattern_len);
		void* __cdecl masm_memory_frobnicate(void* s, size_t n);
	}
}
#endif

// copy from source memory to destination memory with specific length.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memcpy.
constexpr void* memory_copy(void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;
	if (count == 0) return dest;

	void* res = dest;
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	while (count--) {
		*dest_v = *src_v;
		dest_v++;
		src_v++;
	}
	return res;
}

// it`s similar with std::mempcpy
constexpr void* memory_copy_offset(void* MSTL_RESTRICT dest, const void* MSTL_RESTRICT src, size_t count) noexcept {
	if (dest == nullptr || src == nullptr) return nullptr;

	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	while (count--) {
		*dest_v = *src_v;
		dest_v++;
		src_v++;
	}
	return (void*) dest_v;
}

// copy from source memory to destination memory with specific length if not encounter target character.
// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memccpy.
constexpr void* memory_char_copy(void* dest, const void* src, const int chr, size_t count) noexcept {
    if (dest == nullptr || src == nullptr) return nullptr;
    const auto target = static_cast<byte_t>(chr);
    auto dest_v = static_cast<volatile byte_t*>(dest);
    auto src_v = static_cast<const volatile byte_t*>(src);

    while (count--) {
        const byte_t current = *src_v;
        *dest_v = current;
        if (current == target) {
            return (void*) (++dest_v);
        }
        dest_v++;
        src_v++;
    }
    return nullptr;
}

// compare with left-hand memory and right-hand memory in a specific length.
// return a positive number when left-hand memory is greater, a negative number when right-hand memory is greater
// and return zero when they are equal in specific length.
// it`s similar with std::memcmp.
MSTL_PURE_FUNCTION constexpr int memory_compare(const void* lh, const void* rh, size_t count) noexcept {
	if (lh == nullptr && rh == nullptr) return 0;
	if (lh == nullptr) return -1;
    if (rh == nullptr) return 1;

	while (count--) {
		if (*static_cast<const byte_t*>(lh) != *static_cast<const byte_t*>(rh))
			return *static_cast<const byte_t*>(lh) - *static_cast<const byte_t*>(rh);
		lh = static_cast<const byte_t*>(lh) + 1;
		rh = static_cast<const byte_t*>(rh) + 1;
	}
	return 0;
}

// compare with left-hand memory and right-hand memory in a specific length but ignored case of characters.
// return a positive number when left-hand memory is greater, a negative number when right-hand memory is greater
// and return zero when they are equal in specific length.
// it`s similar with std::memicmp.
MSTL_PURE_FUNCTION MSTL_CONSTEXPR20 int memory_compare_ignore_case(
	const void* lh, const void *rh, const size_t count) noexcept {
	if ((lh == nullptr && rh == nullptr) || count == 0) return 0;
	if (lh == nullptr) return -1;
	if (rh == nullptr) return 1;

	const auto lh_v = static_cast<const volatile byte_t*>(lh);
	const auto rh_v = static_cast<const volatile byte_t*>(rh);

    for (size_t i = 0; i < count; ++i) {
        const byte_t c1 = static_cast<byte_t>(_MSTL to_lowercase(static_cast<char>(lh_v[i])));
        const byte_t c2 = static_cast<byte_t>(_MSTL to_lowercase(static_cast<char>(rh_v[i])));
        if (c1 != c2) {
            return static_cast<int>(c1) - static_cast<int>(c2);
        }
    }
    return 0;
}

// return a pointer which is pointing to the first place that equal to target value in a specific length.
// if parameter pointer is nullptr, return nullptr. if not found, return nullptr.
// it`s similar with std::memchr.
MSTL_PURE_FUNCTION constexpr void* memory_char(const void* dest, const int value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;
	auto p = static_cast<const byte_t *>(dest);
	while (count--) {
		if (*p == static_cast<byte_t>(value))
			return const_cast<byte_t*>(p);
		p++;
	}
	return nullptr;
}

// if any parameter pointer is nullptr, return nullptr.
// it`s similar with std::memmove.
constexpr void* memory_move(void* dest, const void* src, size_t count) noexcept {
	if(dest == nullptr || src == nullptr) return nullptr;

	void* res = dest;
	auto dest_v = static_cast<volatile byte_t*>(dest);
	auto src_v = static_cast<const volatile byte_t*>(src);
	if (dest_v < src_v) {
		while (count--) {
			*dest_v = *src_v;
			dest_v = dest_v + 1;
			src_v = src_v + 1;
		}
	} else if (dest_v > src_v) {
		while (count--) {
			*(dest_v + count) = *(src_v + count);
		}
	}
	return res;
}

// fill the destination memory with target value in the specific length.
// if parameter pointer is nullptr, return nullptr.
// it`s similar with std::memset.
constexpr void* memory_set(void* dest, const int value, size_t count) noexcept {
	if(dest == nullptr) return nullptr;

	void* ret = static_cast<byte_t *>(dest);
	auto dest_v = static_cast<volatile byte_t *>(dest);
	while (count--) {
		*dest_v = static_cast<byte_t>(value);
		dest_v = dest_v + 1;
	}
	return ret;
}

// clear the destination memory with zero in the specific length.
// if parameter pointer is nullptr, do nothing.
// it`s similar with std::bzero.
constexpr void memory_zero(void* dest, const size_t count) noexcept {
	if (dest == nullptr) return;

	const auto dest_v = static_cast<volatile byte_t*>(dest);
	for (size_t i = 0; i < count; ++i) {
		dest_v[i] = 0;
	}
}

// std::memmem
constexpr void* memory_in_memory(const void * data, const size_t data_len,
	const void* pattern, const size_t pattern_len) noexcept {
	if (data == nullptr || pattern == nullptr || data_len == 0 || pattern_len == 0 || pattern_len > data_len) {
		return nullptr;
	}
	const auto data_ptr = static_cast<const byte_t*>(data);
	const auto pattern_ptr = static_cast<const byte_t*>(pattern);
	const size_t last_possible = data_len - pattern_len + 1;

	for (size_t i = 0; i < last_possible; ++i) {
		if (data_ptr[i] == pattern_ptr[0]) {
			bool match = true;
			for (size_t j = 1; j < pattern_len; ++j) {
				if (data_ptr[i + j] != pattern_ptr[j]) {
					match = false;
					break;
				}
			}
			if (match) {
				return const_cast<byte_t*>(data_ptr + i);
			}
		}
	}
	return nullptr;
}

// std::memfrob
constexpr void* memory_frobnicate(void* s, const size_t n) noexcept {
	if (s == nullptr || n == 0) return s;
	const auto s_v = static_cast<volatile byte_t*>(s);
	for (size_t i = 0; i < n; i++) {
		s_v[i] ^= 42;
	}
	return s;
}

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_MEMORY_HPP__
