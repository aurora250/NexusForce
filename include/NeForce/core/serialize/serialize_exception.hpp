#ifndef NEFORCE_CORE_SERIALIZE_SERIALIZE_EXCEPTION_HPP__
#define NEFORCE_CORE_SERIALIZE_SERIALIZE_EXCEPTION_HPP__

/**
 * @file serialize_exception.hpp
 * @brief 序列化异常类型
 *
 * 此文件提供了序列化和反序列化过程中抛出的异常类型。
 */

#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup Exceptions 异常类集
 * @{
 */

/**
 * @struct serialize_exception
 * @brief 序列化异常
 */
struct serialize_exception final : value_exception {
    explicit serialize_exception(const char* info = "Serialization failed.", const int code = 0) noexcept :
    value_exception(info, "serialize_exception", code) {}
};

/**
 * @struct deserialize_exception
 * @brief 反序列化异常
 */
struct deserialize_exception : value_exception {
    string property_path; ///< 失败的属性路径

    explicit deserialize_exception(const char* info = "Deserialization failed.", string path = "",
                                   const int code = 0) noexcept :
    value_exception(info, "deserialize_exception", code),
    property_path(move(path)) {}
};

/**
 * @struct missing_required_exception
 * @brief 反序列化时缺少必填属性异常
 */
struct missing_required_exception final : deserialize_exception {
    explicit missing_required_exception(const char* property_name, const int code = 0) :
    deserialize_exception("Missing required property.", property_name, code) {}
};

/**
 * @struct type_mismatch_exception
 * @brief 序列化类型不匹配异常
 */
struct type_mismatch_exception final : deserialize_exception {
    explicit type_mismatch_exception(const char* expected_type, const char* actual_type, const int code = 0) :
    deserialize_exception("Type mismatch.", expected_type, code) {}
};

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SERIALIZE_SERIALIZE_EXCEPTION_HPP__
