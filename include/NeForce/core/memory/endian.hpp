#ifndef NEFORCE_CORE_MEMORY_ENDIAN_HPP__
#define NEFORCE_CORE_MEMORY_ENDIAN_HPP__
#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct endian {
public:
    static constexpr bool is_little_endian =
#ifdef NEFORCE_PLATFORM_WINDOWS
        true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
        __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
        __BYTE_ORDER == __LITTLE_ENDIAN;
#else
        return false;
#endif

    static constexpr bool is_big_endian = !is_little_endian;

    static bool is_little_endian_runtime() noexcept {
        constexpr uint16_t test = 0x0001;
        return *reinterpret_cast<const uint8_t*>(&test) == 0x01;
    }

    static constexpr uint16_t byteswap16(uint16_t value) noexcept {
        return (value >> 8) | (value << 8);
    }

    static constexpr uint32_t byteswap32(uint32_t value) noexcept {
        return ((value >> 24) & 0x000000FF) |
               ((value >> 8)  & 0x0000FF00) |
               ((value << 8)  & 0x00FF0000) |
               ((value << 24) & 0xFF000000);
    }

    static constexpr uint64_t byteswap64(uint64_t value) noexcept {
        return ((value >> 56) & 0x00000000000000FF) |
               ((value >> 40) & 0x000000000000FF00) |
               ((value >> 24) & 0x0000000000FF0000) |
               ((value >> 8)  & 0x00000000FF000000) |
               ((value << 8)  & 0x000000FF00000000) |
               ((value << 24) & 0x0000FF0000000000) |
               ((value << 40) & 0x00FF000000000000) |
               ((value << 56) & 0xFF00000000000000);
    }

    template <typename T>
    static constexpr T host_to_network(T value) noexcept {
        if constexpr (is_big_endian) {
            return value;
        } else {
            if constexpr (sizeof(T) == 2) return byteswap16(value);
            if constexpr (sizeof(T) == 4) return byteswap32(value);
            if constexpr (sizeof(T) == 8) return byteswap64(value);
            return value;
        }
    }

    template <typename T>
    static constexpr T network_to_host(T value) noexcept {
        return host_to_network(value);
    }

    template <typename T>
    static constexpr T host_to_le(T value) noexcept {
        if constexpr (is_little_endian) {
            return value;
        } else {
            if constexpr (sizeof(T) == 2) return byteswap16(value);
            if constexpr (sizeof(T) == 4) return byteswap32(value);
            if constexpr (sizeof(T) == 8) return byteswap64(value);
            return value;
        }
    }

    template <typename T>
    static constexpr T le_to_host(T value) noexcept {
        return host_to_le(value);
    }

    template <typename T>
    static constexpr T host_to_be(T value) noexcept {
        if constexpr (is_big_endian) {
            return value;
        } else {
            if constexpr (sizeof(T) == 2) return byteswap16(value);
            if constexpr (sizeof(T) == 4) return byteswap32(value);
            if constexpr (sizeof(T) == 8) return byteswap64(value);
            return value;
        }
    }

    template <typename T>
    static constexpr T be_to_host(T value) noexcept {
        return host_to_be(value);
    }

    template <typename T>
    static constexpr T swap_endian(T value) noexcept {
        if constexpr (sizeof(T) == 2) return byteswap16(value);
        if constexpr (sizeof(T) == 4) return byteswap32(value);
        if constexpr (sizeof(T) == 8) return byteswap64(value);
        return value;
    }

    static uint16_t read_le16(const uint8_t* data) noexcept {
        return static_cast<uint16_t>(data[0]) |
               (static_cast<uint16_t>(data[1]) << 8);
    }

    static uint32_t read_le32(const uint8_t* data) noexcept {
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[3]) << 24);
    }

    static uint64_t read_le64(const uint8_t* data) noexcept {
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(data[i]) << (i * 8);
        }
        return value;
    }

    static uint16_t read_be16(const uint8_t* data) noexcept {
        return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
    }

    static uint32_t read_be32(const uint8_t* data) noexcept {
        return (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) |
               static_cast<uint32_t>(data[3]);
    }

    static uint64_t read_be64(const uint8_t* data) noexcept {
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(data[i]) << ((7 - i) * 8);
        }
        return value;
    }

    static void write_le16(uint8_t* dest, uint16_t value) noexcept {
        dest[0] = static_cast<uint8_t>(value & 0xFF);
        dest[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }

    static void write_le32(uint8_t* dest, uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            dest[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
    }

    static void write_le64(uint8_t* dest, uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            dest[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
    }

    static void write_be16(uint8_t* dest, uint16_t value) noexcept {
        dest[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
        dest[1] = static_cast<uint8_t>(value & 0xFF);
    }

    static void write_be32(uint8_t* dest, uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            dest[i] = static_cast<uint8_t>((value >> ((3 - i) * 8)) & 0xFF);
        }
    }

    static void write_be64(uint8_t* dest, uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            dest[i] = static_cast<uint8_t>((value >> ((7 - i) * 8)) & 0xFF);
        }
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_ENDIAN_HPP__
