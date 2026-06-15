#ifndef NEFORCE_CORE_SERIALIZE_BINARY_SERIALIZER_HPP__
#define NEFORCE_CORE_SERIALIZE_BINARY_SERIALIZER_HPP__

/**
 * @file binary_serializer.hpp
 * @brief 二进制序列化器
 *
 * 此文件提供了基于反射系统的二进制序列化器。
 * 采用大端字节序（网络字节序）存储，确保跨平台兼容。
 *
 * 格式：
 *   [Magic: 4B "NEBF"] [Version: 2B]
 *   [TypeCount: 4B] [TypeTable...]
 *   [ObjectCount: 4B] [DataSection...]
 *   DataSection:
 *     [TypeID: 8B] [PropCount: 4B]
 *     For each property:
 *       [NameLen: 2B] [Name: N] [TypeTag: 1B] [Value: var]
 */

#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/serialize/serializer_base.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SERIALIZE__

/**
 * @addtogroup Serializer 序列化
 * @{
 */

/**
 * @class binary_serializer
 * @brief 二进制序列化器
 *
 * 将反射注册的对象序列化为大端字节序的二进制格式，
 * 或从二进制数据重建对象。
 */
class NEFORCE_API binary_serializer {
public:
    using buffer = vector<byte_t>; ///< 二进制缓冲区类型

    /**
     * @brief 二进制格式魔数 "NEBF"
     */
    static constexpr uint32_t MAGIC = 0x4E454246;

    /**
     * @brief 当前格式版本
     */
    static constexpr uint16_t VERSION = 1;

    /**
     * @brief 将反射对象序列化为二进制
     * @param obj 要序列化的对象
     * @param ctx 序列化上下文
     * @return 二进制数据
     */
    static buffer serialize(const reflect::meta_any& obj, const serialize_context& ctx = {});

    /**
     * @brief 从二进制数据反序列化对象
     * @param data 二进制数据起始指针
     * @param size 数据长度
     * @return 重建的对象
     */
    static reflect::meta_any deserialize(const byte_t* data, size_t size);
};

/** @} */ // Serializer

NEFORCE_END_SERIALIZE__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SERIALIZE_BINARY_SERIALIZER_HPP__
