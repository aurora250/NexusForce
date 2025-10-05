#include <MSTL/core/string.hpp>
MSTL_BEGIN_NAMESPACE__

MSTL_BEGIN_INNER__
#ifdef MSTL_DATA_BUS_WIDTH_64__
static void __address_offset(uintptr_t& mask, int& shift) {
    mask = 0xF000000000000000ULL;
    shift = 60;
}
#else
static void __address_offset(uintptr_t& mask, int& shift) {
    mask = 0xF0000000UL;
    shift = 28;
}
#endif
MSTL_END_INNER__

string address_string(const void* p) {
    if (p == nullptr) return {"nullptr"};
    const uintptr_t addr_val = reinterpret_cast<uintptr_t>(p);
    constexpr size_t ptr_byte_size = sizeof(void*);
    constexpr size_t hex_digit_count = ptr_byte_size * 2;
    constexpr char hex_digits[] = "0123456789abcdef";

    string result;
    result.reserve(2 + hex_digit_count);
    result += "0x";

    uintptr_t mask;
    int shift;
    _INNER __address_offset(mask, shift);

    for (size_t i = 0; i < hex_digit_count; ++i) {
        const uint8_t digit = static_cast<uint8_t>((addr_val & mask) >> shift);
        result += hex_digits[digit];
        mask >>= 4;
        shift -= 4;
    }
    return result;
}

MSTL_END_NAMESPACE__
