#ifndef NEFORCE_CORE_UTILITY_UUID_HPP__
#define NEFORCE_CORE_UTILITY_UUID_HPP__

/**
 * @file uuid.hpp
 * @brief UUID实现
 *
 * 此文件提供了UUID的生成和操作功能，支持UUID版本4（随机）和版本7（时间戳排序）。
 * UUID是一个128位的唯一标识符，广泛用于分布式系统中的标识生成。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下标准规范：
 *
 * - **IETF RFC 4122**：UUID 版本 4 的定义（基于随机数）
 *   https://www.rfc-editor.org/rfc/rfc4122.html
 * - **IETF RFC 9562**：UUID 版本 7 的定义（基于 Unix 时间戳排序）
 *   https://www.rfc-editor.org/rfc/rfc9562.html
 *
 * 此外，UUID 的结构与编码方式也符合以下 ISO 标准：
 * - **ISO/IEC 9834-8:2014**：信息技术 — 对象标识符解析系统 — 第8部分：UUID 的生成与注册
 *   https://www.iso.org/standard/62795.html
 * - **ISO/IEC 18004:2024** 相关附录
 *   https://www.iso.org/standard/83358.html
 *
 * @section version_details 版本细节
 * - **版本 4**：变体位为 10（0b10），版本位为 4（0b0100）
 * - **版本 7**：变体位为 10（0b10），版本位为 7（0b0111）
 *   时间戳为 Unix 毫秒（占高 48 位），后跟 12 位随机计数器与 62 位随机数
 */

#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/core/numeric/random.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup UUID UUID
 * @brief UUID生成与解析工具
 * @{
 */

/**
 * @class uuid
 * @brief 通用唯一标识符类
 *
 * UUID格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36字符，包括4个连字符)
 * 表示一个128位的UUID，支持版本4和版本7的生成，以及从字符串/字节数组的解析。
 */
class NEFORCE_API uuid : public istringify<uuid> {
private:
    array<byte_t, 16> data_; ///< 16字节的UUID原始数据

    static random_mt& tl_rng() noexcept;

public:
    /**
     * @brief 默认构造函数
     *
     * 创建全零UUID（nil UUID）。
     */
    uuid() noexcept = default;

    /**
     * @brief 从16字节数组构造UUID
     * @param bytes 16字节的只读内存视图
     *
     * 直接复制字节数据构造UUID，不验证版本和格式。
     */
    explicit uuid(memory_view<const byte_t, 16> bytes) noexcept;

    /**
     * @brief 从字符串构造UUID
     * @param bytes UUID字符串
     * @throws value_exception 字符串格式无效时抛出
     *
     * 支持36字符带连字符或32字符无连字符格式
     */
    explicit uuid(string_view bytes);

    /**
     * @brief 生成UUID版本4（随机）
     *
     * 基于随机数生成UUID，设置版本位为4，变体位为10。
     */
    void generate_v4() noexcept;

    /**
     * @brief 生成UUID版本7（时间戳排序）
     *
     * 基于Unix毫秒时间戳和随机数生成UUID，具有时间单调性。
     */
    void generate_v7() noexcept;

    /**
     * @brief 获取UUID版本号
     * @return 版本号（4或7）
     */
    NEFORCE_NODISCARD int version() const noexcept { return (data_[6] >> 4) & 0x0F; }

    /**
     * @brief 检查是否为版本4
     * @return 是版本4返回true
     */
    NEFORCE_NODISCARD bool is_v4() const noexcept { return version() == 4; }

    /**
     * @brief 检查是否为版本7
     * @return 是版本7返回true
     */
    NEFORCE_NODISCARD bool is_v7() const noexcept { return version() == 7; }

    /**
     * @brief 获取UUID版本7的时间戳
     * @return 包含Unix毫秒时间戳的optional，如果不是版本7返回空
     *
     * 从UUID版本7中提取前48位的时间戳信息。
     */
    NEFORCE_NODISCARD optional<uint64_t> timestamp_v7() const noexcept;

    /**
     * @brief 转换为标准UUID字符串
     * @return 格式为"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"的字符串
     *
     * 将UUID转换为36字符的标准格式，包含4个连字符。
     */
    NEFORCE_NODISCARD string to_string() const;

    /**
     * @brief 获取UUID的字节视图
     * @return 16字节的只读内存视图
     */
    NEFORCE_NODISCARD memory_view<const byte_t, 16> bytes() const noexcept {
        return memory_view<const byte_t, 16>(data_);
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个字节的迭代器
     */
    NEFORCE_NODISCARD auto begin() const noexcept { return data_.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个字节之后的迭代器
     */
    NEFORCE_NODISCARD auto end() const noexcept { return data_.end(); }

    /**
     * @brief 静态方法：生成版本4 UUID
     * @return 新生成的版本4 UUID
     */
    static uuid v4() noexcept;

    /**
     * @brief 静态方法：生成版本7 UUID
     * @return 新生成的版本7 UUID
     */
    static uuid v7() noexcept;
};

/** @} */ // UUID


NEFORCE_BEGIN_LITERALS__

/**
 * @defgroup UserLiterals 字面量
 * @brief 用户定义字面量支持
 * @{
 */

/**
 * @brief UUID字面量运算符
 * @param str UUID字符串
 * @param len 字符串长度
 * @return 解析后的UUID对象
 * @throws value_exception 字符串格式无效时抛出
 */
NEFORCE_NODISCARD inline uuid operator""_uuid(const char* str, size_t len) { return uuid(string_view(str, len)); }

/** @} */ // UserLiterals

NEFORCE_END_LITERALS__


/**
 * @defgroup HashPrimary 哈希模板
 * @brief 哈希函数的模板和基础定义
 * @{
 */

/**
 * @brief uuid的哈希特化
 */
template <>
struct hash<uuid> {
    size_t operator()(const uuid& uuid) const noexcept {
        const auto& bytes = uuid.bytes();
        size_t hash = 0;
        for (size_t i = 0; i < 16; i += sizeof(size_t)) {
            size_t part = 0;
            memory_copy(&part, bytes.data() + i, sizeof(size_t));
            hash ^= part;
        }
        return hash;
    }
};

/** @} */ // HashPrimary

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_UUID_HPP__
