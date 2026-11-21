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
NEFORCE_ERROR_BUILD_FINAL_CLASS(serialize_exception, value_exception, "Serialization failed.")

/**
 * @struct deserialize_exception
 * @brief 反序列化异常
 */
struct deserialize_exception : value_exception {
    string property_path; ///< 失败的属性路径

    explicit deserialize_exception(const char* info = "Deserialization failed.", string path = "") noexcept :
    value_exception(info),
    property_path(move(path)) {}

    explicit deserialize_exception(const exception& e) :
    value_exception(e) {}

    ~deserialize_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "deserialize_exception"; }
    NEFORCE_NODISCARD exception* clone() const override { return new deserialize_exception(*this); }
    NEFORCE_NORETURN void rethrow() const override { throw *this; }
};

/**
 * @struct missing_required_exception
 * @brief 反序列化时缺少必填属性异常
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(missing_required_exception, deserialize_exception, "Missing required property.")

/**
 * @struct type_mismatch_exception
 * @brief 序列化类型不匹配异常
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(type_mismatch_exception, deserialize_exception, "Type mismatch.")

/** @} */ // Exceptions

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_SERIALIZE_SERIALIZE_EXCEPTION_HPP__
