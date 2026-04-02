#ifndef NEFORCE_CORE_MEMORY_ENDIAN_HPP__
#define NEFORCE_CORE_MEMORY_ENDIAN_HPP__

/**
 * @file endian.hpp
 * @brief 端序转换工具
 *
 * 此文件提供了跨平台的端序（字节序）检测和转换功能。
 * 支持大端序（Big Endian）和小端序（Little Endian）之间的转换，
 * 以及主机序与网络序之间的转换。
 *
 * 端序说明：
 * - 小端序：低字节存储在低地址
 * - 大端序：高字节存储在低地址
 */

#include "NeForce/core/typeinfo/type_traits.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Endian 端序操作
 * @brief 端序判断和转换操作
 * @{
 */

/**
 * @struct endian
 * @brief 端序转换工具结构体
 *
 * 提供编译时和运行时的端序检测，以及各种端序转换函数。
 * 支持整型类型的字节序转换和原始字节数组的读写操作。
 */
struct endian {
public:
    /**
     * @brief 编译时检测是否为小端序
     */
    static constexpr bool is_little_endian =
#ifdef NEFORCE_PLATFORM_WINDOWS
            true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
            __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
            __BYTE_ORDER == __LITTLE_ENDIAN;
#else
            false;
#    warning "Unsupported costexpr endian type"
#endif

    /**
     * @brief 编译时检测是否为大端序
     */
    static constexpr bool is_big_endian = !is_little_endian;

    /**
     * @brief 运行时检测是否为小端序
     */
    NEFORCE_NODISCARD static NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE bool is_little_endian_runtime() noexcept {
        constexpr uint16_t test = 0x0001;
        return *reinterpret_cast<const byte_t*>(&test) == 0x01;
    }

    /**
     * @brief 16位整数字节序反转
     * @param value 输入值
     * @return 字节序反转后的值
     */
    static constexpr uint16_t byteswap16(uint16_t value) noexcept { return (value >> 8) | (value << 8); }

    /**
     * @brief 32位整数字节序反转
     * @param value 输入值
     * @return 字节序反转后的值
     */
    static constexpr uint32_t byteswap32(uint32_t value) noexcept {
        return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) |
               ((value << 24) & 0xFF000000);
    }

    /**
     * @brief 64位整数字节序反转
     * @param value 输入值
     * @return 字节序反转后的值
     */
    static constexpr uint64_t byteswap64(uint64_t value) noexcept {
        return ((value >> 56) & 0x00000000000000FF) | ((value >> 40) & 0x000000000000FF00) |
               ((value >> 24) & 0x0000000000FF0000) | ((value >> 8) & 0x00000000FF000000) |
               ((value << 8) & 0x000000FF00000000) | ((value << 24) & 0x0000FF0000000000) |
               ((value << 40) & 0x00FF000000000000) | ((value << 56) & 0xFF00000000000000);
    }

private:
    template <typename T>
    static constexpr enable_if_t<is_big_endian, T> host_to_network_impl1(T value) noexcept {
        return value;
    }

    template <typename T>
    static constexpr enable_if_t<sizeof(T) == 2, T> host_to_network_impl2(T value) noexcept {
        return endian::byteswap16(value);
    }

    template <typename T>
    static constexpr enable_if_t<sizeof(T) == 4, T> host_to_network_impl2(T value) noexcept {
        return endian::byteswap32(value);
    }

    template <typename T>
    static constexpr enable_if_t<sizeof(T) == 8, T> host_to_network_impl2(T value) noexcept {
        return endian::byteswap64(value);
    }

    template <typename T>
    static constexpr enable_if_t<sizeof(T) != 2 && sizeof(T) != 4 && sizeof(T) != 8, T>
    host_to_network_impl2(T value) noexcept {
        static_assert(sizeof(T) == 0, "Unsupported type size for endian swap");
        return value;
    }

    template <typename T>
    static constexpr enable_if_t<!is_big_endian, T> host_to_network_impl1(T value) noexcept {
        return endian::host_to_network_impl2(value);
    }

    template <typename T>
    static constexpr enable_if_t<is_little_endian, T> host_to_network_impl3(T value) noexcept {
        return value;
    }

    template <typename T>
    static constexpr enable_if_t<!is_little_endian, T> host_to_network_impl3(T value) noexcept {
        return endian::host_to_network_impl2(value);
    }

public:
    /**
     * @brief 主机序转网络序
     * @tparam T 整数类型（支持2、4、8字节）
     * @param value 主机序的值
     * @return 网络序的值
     *
     * 网络字节序为大端序。
     * 大端序系统直接返回，小端序系统执行字节序反转。
     */
    template <typename T>
    static constexpr T host_to_network(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl1(value);
    }

    /**
     * @brief 网络序转主机序
     * @tparam T 整数类型
     * @param value 网络序的值
     * @return 主机序的值
     *
     * 网络序转主机序与主机序转网络序相同（对称操作）。
     */
    template <typename T>
    static constexpr T network_to_host(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network(value);
    }

    /**
     * @brief 主机序转小端序
     * @tparam T 整数类型
     * @param value 主机序的值
     * @return 小端序的值
     *
     * 小端序系统直接返回，大端序系统执行字节序反转。
     */
    template <typename T>
    static constexpr T host_to_le(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl3(value);
    }

    /**
     * @brief 小端序转主机序
     * @tparam T 整数类型
     * @param value 小端序的值
     * @return 主机序的值
     *
     * 小端序转主机序与主机序转小端序相同（对称操作）。
     */
    template <typename T>
    static constexpr T le_to_host(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_le(value);
    }

    /**
     * @brief 主机序转大端序
     * @tparam T 整数类型
     * @param value 主机序的值
     * @return 大端序的值
     *
     * 大端序系统直接返回，小端序系统执行字节序反转。
     */
    template <typename T>
    static constexpr T host_to_be(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl1(value);
    }

    /**
     * @brief 大端序转主机序
     * @tparam T 整数类型
     * @param value 大端序的值
     * @return 主机序的值
     *
     * 大端序转主机序与主机序转大端序相同（对称操作）。
     */
    template <typename T>
    static constexpr T be_to_host(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_be(value);
    }

    /**
     * @brief 字节序反转（不区分端序）
     * @tparam T 整数类型
     * @param value 输入值
     * @return 字节序反转后的值
     *
     * 强制反转字节序，不关心当前平台字节序。
     */
    template <typename T>
    static constexpr T swap_endian(T value) noexcept {
        static_assert(is_integral_v<T>, "T must be an integral type");
        return endian::host_to_network_impl2(value);
    }

    /**
     * @brief 读取16位小端整数
     * @param data 字节数组指针
     * @return 读取的16位整数
     */
    static uint16_t read_le16(const byte_t* data) noexcept {
        return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    }

    /**
     * @brief 读取32位小端整数
     * @param data 字节数组指针
     * @return 读取的32位整数
     */
    static uint32_t read_le32(const byte_t* data) noexcept {
        return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
    }

    /**
     * @brief 读取64位小端整数
     * @param data 字节数组指针
     * @return 读取的64位整数
     */
    static uint64_t read_le64(const byte_t* data) noexcept {
        return static_cast<uint64_t>(data[0]) | (static_cast<uint64_t>(data[1]) << 8) |
               (static_cast<uint64_t>(data[2]) << 16) | (static_cast<uint64_t>(data[3]) << 24) |
               (static_cast<uint64_t>(data[4]) << 32) | (static_cast<uint64_t>(data[5]) << 40) |
               (static_cast<uint64_t>(data[6]) << 48) | (static_cast<uint64_t>(data[7]) << 56);
    }

    /**
     * @brief 读取16位大端整数
     * @param data 字节数组指针
     * @return 读取的16位整数
     */
    static uint16_t read_be16(const byte_t* data) noexcept {
        return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
    }

    /**
     * @brief 读取32位大端整数
     * @param data 字节数组指针
     * @return 读取的32位整数
     */
    static uint32_t read_be32(const byte_t* data) noexcept {
        return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
    }

    /**
     * @brief 读取64位大端整数
     * @param data 字节数组指针
     * @return 读取的64位整数
     */
    static uint64_t read_be64(const byte_t* data) noexcept {
        return (static_cast<uint64_t>(data[0]) << 56) | (static_cast<uint64_t>(data[1]) << 48) |
               (static_cast<uint64_t>(data[2]) << 40) | (static_cast<uint64_t>(data[3]) << 32) |
               (static_cast<uint64_t>(data[4]) << 24) | (static_cast<uint64_t>(data[5]) << 16) |
               (static_cast<uint64_t>(data[6]) << 8) | static_cast<uint64_t>(data[7]);
    }

    /**
     * @brief 写入16位小端整数
     * @param dest 目标字节数组指针
     * @param value 要写入的值
     */
    static void write_le16(byte_t* dest, uint16_t value) noexcept {
        dest[0] = static_cast<byte_t>(value & 0xFF);
        dest[1] = static_cast<byte_t>((value >> 8) & 0xFF);
    }

    /**
     * @brief 写入32位小端整数
     * @param dest 目标字节数组指针
     * @param value 要写入的值
     */
    static void write_le32(byte_t* dest, uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            dest[i] = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        }
    }

    /**
     * @brief 写入64位小端整数
     * @param dest 目标字节数组指针
     * @param value 要写入的值
     */
    static void write_le64(byte_t* dest, uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            dest[i] = static_cast<byte_t>((value >> (i * 8)) & 0xFF);
        }
    }

    /**
     * @brief 写入16位大端整数
     * @param dest 目标字节数组指针
     * @param value 要写入的值
     */
    static void write_be16(byte_t* dest, uint16_t value) noexcept {
        dest[0] = static_cast<byte_t>((value >> 8) & 0xFF);
        dest[1] = static_cast<byte_t>(value & 0xFF);
    }

    /**
     * @brief 写入32位大端整数
     * @param dest 目标字节数组指针
     * @param value 要写入的值
     */
    static void write_be32(byte_t* dest, uint32_t value) noexcept {
        for (int i = 0; i < 4; ++i) {
            dest[i] = static_cast<byte_t>((value >> ((3 - i) * 8)) & 0xFF);
        }
    }

    /**
     * @brief 写入64位大端整数
     * @param dest 目标字节数组指针
     * @param value 要写入的值
     */
    static void write_be64(byte_t* dest, uint64_t value) noexcept {
        for (int i = 0; i < 8; ++i) {
            dest[i] = static_cast<byte_t>((value >> ((7 - i) * 8)) & 0xFF);
        }
    }
};

/** @} */ // Endian

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_ENDIAN_HPP__
