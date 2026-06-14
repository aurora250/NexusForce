#ifndef NEFORCE_CORE_SERIALIZE_JSON_SERIALIZER_HPP__
#define NEFORCE_CORE_SERIALIZE_JSON_SERIALIZER_HPP__

/**
 * @file json_serializer.hpp
 * @brief JSON 序列化器
 *
 * 此文件提供了基于反射系统的 JSON 序列化器。
 * 可将任意已注册反射类型的对象序列化为 JSON 格式，
 * 或从 JSON 反序列化重建对象。
 */

#include "NeForce/core/file/json/json_builder.hpp"
#include "NeForce/core/serialize/serializer_base.hpp"
#include "NeForce/core/serialize/serialize_exception.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SERIALIZE__

/**
 * @addtogroup Serializer 序列化
 * @{
 */

/**
 * @brief JSON 序列化器
 *
 * 将反射注册的对象转换为 JSON 表示，或从 JSON 重建对象。
 * 自动遍历 meta_type 的属性列表，跳过暂态属性。
 */
class json_serializer {
public:
    /**
     * @brief 将反射对象序列化为 JSON 值树
     * @param obj 要序列化的对象
     * @param ctx 序列化上下文
     * @return JSON 值树的根节点
     */
    static unique_ptr<json_value> serialize(const reflect::meta_any& obj, const serialize_context& ctx = {});

    /**
     * @brief 将反射对象序列化为 JSON 字符串
     * @param obj 要序列化的对象
     * @param ctx 序列化上下文
     * @return JSON 字符串
     */
    static string to_string(const reflect::meta_any& obj, const serialize_context& ctx = {});

    /**
     * @brief 从 JSON 值树反序列化对象
     * @param value JSON 值树
     * @param type 目标类型元数据
     * @param ctx 序列化上下文
     * @return 重建的对象
     */
    static reflect::meta_any deserialize(const json_value& value, const reflect::meta_type& type,
                                         const serialize_context& ctx = {});

    /**
     * @brief 从 JSON 字符串反序列化对象
     * @param json_str JSON 字符串
     * @param type 目标类型元数据
     * @param ctx 序列化上下文
     * @return 重建的对象
     */
    static reflect::meta_any from_string(const string& json_str, const reflect::meta_type& type,
                                         const serialize_context& ctx = {});
};

/** @} */ // Serializer

NEFORCE_END_SERIALIZE__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SERIALIZE_JSON_SERIALIZER_HPP__
