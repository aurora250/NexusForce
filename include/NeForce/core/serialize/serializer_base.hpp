#ifndef NEFORCE_CORE_SERIALIZE_SERIALIZER_BASE_HPP__
#define NEFORCE_CORE_SERIALIZE_SERIALIZER_BASE_HPP__

/**
 * @file serializer_base.hpp
 * @brief 序列化器公共基础设施
 *
 * 此文件提供了所有序列化器共用的上下文结构和类型分派工具。
 */

#include "NeForce/core/reflect/reflect.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_SERIALIZE__

/**
 * @defgroup Serializer 序列化
 * @brief 序列化与反序列化组件
 * @{
 */

/**
 * @struct serialize_context
 * @brief 序列化上下文
 *
 * 控制序列化和反序列化的行为。
 */
struct serialize_context {
    bool include_transient = false; ///< 是否包含暂态属性
    bool pretty = false;            ///< 是否美化输出
    uint32_t version = 0;           ///< 序列化版本号
};

/**
 * @brief 判断指定类型 ID 是否为基础算术类型
 * @param tid 类型 ID
 * @return 是基础算术类型返回 true
 */
NEFORCE_NODISCARD inline bool is_arithmetic_type(const reflect::type_id tid) noexcept {
    return tid == reflect::type_id_for<bool>() || tid == reflect::type_id_for<char>() ||
           tid == reflect::type_id_for<short>() || tid == reflect::type_id_for<int>() ||
           tid == reflect::type_id_for<long>() || tid == reflect::type_id_for<long long>() ||
           tid == reflect::type_id_for<unsigned char>() || tid == reflect::type_id_for<unsigned short>() ||
           tid == reflect::type_id_for<unsigned int>() || tid == reflect::type_id_for<unsigned long>() ||
           tid == reflect::type_id_for<unsigned long long>() || tid == reflect::type_id_for<float>() ||
           tid == reflect::type_id_for<double>() || tid == reflect::type_id_for<long double>() ||
#ifdef NEFORCE_STANDARD_20
           tid == reflect::type_id_for<char8_t>() ||
#endif
           tid == reflect::type_id_for<wchar_t>() || tid == reflect::type_id_for<char16_t>() ||
           tid == reflect::type_id_for<char32_t>();
}

/**
 * @brief 判断指定类型 ID 是否为字符类型
 * @param tid 类型 ID
 * @return 是字符类型返回 true
 */
NEFORCE_NODISCARD inline bool is_char_type(const reflect::type_id tid) noexcept {
    return tid == reflect::type_id_for<char>() || tid == reflect::type_id_for<wchar_t>() ||
#ifdef NEFORCE_STANDARD_20
           tid == reflect::type_id_for<char8_t>() ||
#endif
           tid == reflect::type_id_for<char16_t>() || tid == reflect::type_id_for<char32_t>();
}

/**
 * @brief 判断指定类型 ID 是否为字符串类型
 * @param tid 类型 ID
 * @return 是字符串类型返回 true
 */
NEFORCE_NODISCARD inline bool is_string_type(const reflect::type_id tid) noexcept {
    return tid == reflect::type_id_for<string>();
}

/** @} */ // Serialize

NEFORCE_END_SERIALIZE__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SERIALIZE_SERIALIZER_BASE_HPP__
