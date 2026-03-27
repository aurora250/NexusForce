#ifndef NEFORCE_CORE_MEMORY_ENDIAN_HPP__
#define NEFORCE_CORE_MEMORY_ENDIAN_HPP__
#include "NeForce/core/typeinfo/type_traits.hpp"
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
        false;
#endif

    static constexpr bool is_big_endian = !is_little_endian;


    NEFORCE_NODISCARD static NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    bool is_little_endian_runtime() noexcept {
        constexpr uint16_t test = 0x0001;
        return *reinterpret_cast<const byte_t*>(&test) == 0x01;
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

private:
    template <typename T>
    static constexpr enable_if_t<is_big_endian, T>
    host_to_network_impl1(T value) noexcept { return value; }

    template <typename T> static constexpr
    enable_if_t<sizeof(T) == 2, T>
    host_to_network_impl2(T value) noexcept { return endian::byteswap16(value); }

    template <typename T> static constexpr
    enable_if_t<sizeof(T) == 4, T>
    host_to_network_impl2(T value) noexcept { return endian::byteswap32(value); }

    template <typename T> static constexpr
    enable_if_t<sizeof(T) == 8, T>
    host_to_network_impl2(T value) noexcept { return endian::byteswap64(value); }

    template <typename T> static constexpr
    enable_if_t<sizeof(T) != 2 && sizeof(T) != 4 && sizeof(T) != 8, T>
    host_to_network_impl2(T value) noexcept {
        static_assert(sizeof(T) == 0, "Unsupported type size for endian swap");
        return value;
    }

    template <typename T>
    static constexpr enable_if_t<!is_big_endian, T>
    host_to_network_impl1(T value) noexcept { return endian::host_to_network_impl2(value); }

    template <typename T>
    static constexpr enable_if_t<is_little_endian, T>
    host_to_network_impl3(T value) noexcept { return value; }

    template <typename T>
    static constexpr enable_if_t<!is_little_endian, T>
    host_to_network_impl3(T value) noexcept { return endian::host_to_network_impl2(value); }

public:
    template <typename T>
    static constexpr T host_to_network(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl1(value);
    }

    template <typename T>
    static constexpr T network_to_host(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network(value);
    }

    template <typename T>
    static constexpr T host_to_le(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl3(value);
    }

    template <typename T>
    static constexpr T le_to_host(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_le(value);
    }

    template <typename T>
    static constexpr T host_to_be(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl1(value);
    }

    template <typename T>
    static constexpr T be_to_host(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_be(value);
    }

    template <typename T>
    static constexpr T swap_endian(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl2(value);
    }

    static uint16_t read_le16(const byte_t* data) noexcept {
        return static_cast<uint16_t>(data[0]) |
              (static_cast<uint16_t>(data[1]) << 8);
    }

    static uint32_t read_le32(const byte_t* data) noexcept {
        return static_cast<uint32_t>(data[0]) |
              (static_cast<uint32_t>(data[1]) << 8) |
              (static_cast<uint32_t>(data[2]) << 16) |
              (static_cast<uint32_t>(data[3]) << 24);
    }

    static uint64_t read_le64(const byte_t* data) noexcept {
        return static_cast<uint64_t>(data[0]) |
              (static_cast<uint64_t>(data[1]) << 8) |
              (static_cast<uint64_t>(data[2]) << 16) |
              (static_cast<uint64_t>(data[3]) << 24) |
              (static_cast<uint64_t>(data[4]) << 32) |
              (static_cast<uint64_t>(data[5]) << 40) |
              (static_cast<uint64_t>(data[6]) << 48) |
              (static_cast<uint64_t>(data[7]) << 56);
    }

    static uint16_t read_be16(const byte_t* data) noexcept {
        return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
    }

    static uint32_t read_be32(const byte_t* data) noexcept {
        return (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) |
                static_cast<uint32_t>(data[3]);
    }

    static uint64_t read_be64(const byte_t* data) noexcept {
        return (static_cast<uint64_t>(data[0]) << 56) |
               (static_cast<uint64_t>(data[1]) << 48) |
               (static_cast<uint64_t>(data[2]) << 40) |
               (static_cast<uint64_t>(data[3]) << 32) |
               (static_cast<uint64_t>(data[4]) << 24) |
               (static_cast<uint64_t>(data[5]) << 16) |
               (static_cast<uint64_t>(data[6]) << 8)  |
                static_cast<uint64_t>(data[7]);
    }

    static void write_le16(byte_t* dest, uint16_t value) noexcept {
        dest[0] = static_cast<byte_t>(value & 0xFF);
        dest[1] = static_cast<byte_t>((value >> 8) & 0xFF);
    }

    static void write_le32(byte_t* dest, uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            dest[i] = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        }
    }

    static void write_le64(byte_t* dest, uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            dest[i] = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        }
    }

    static void write_be16(byte_t* dest, uint16_t value) noexcept {
        dest[0] = static_cast<byte_t>((value >> 8) & 0xFF);
        dest[1] = static_cast<byte_t>(value & 0xFF);
    }

    static void write_be32(byte_t* dest, uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            dest[i] = static_cast<byte_t>((value >> ((3 - i) * 8)) & 0xFF);
        }
    }

    static void write_be64(byte_t* dest, uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            dest[i] = static_cast<byte_t>((value >> ((7 - i) * 8)) & 0xFF);
        }
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_ENDIAN_HPP__
