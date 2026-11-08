#ifndef NEFORCE_CORE_FILE_INI_INI_BUILDER_HPP__
#define NEFORCE_CORE_FILE_INI_INI_BUILDER_HPP__

/**
 * @file ini_builder.hpp
 * @brief INI 构建器
 */

#include "NeForce/core/file/ini/ini_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup IniConfig INI配置
 * @{
 */

/**
 * @class ini_builder
 * @brief 流式构建 INI 文档
 */
class NEFORCE_API ini_builder {
private:
    ini_document document_;               ///< 正在构建的文档
    ini_section_data* current_ = nullptr; ///< 当前节
    string current_key_;                  ///< 待写入的键

    void test_exception() const;

public:
    /**
     * @brief 构造构建器，当前节为全局节
     */
    ini_builder();

    ini_builder(const ini_builder&) = delete;
    ini_builder& operator=(const ini_builder&) = delete;

    /**
     * @brief 移动构造
     * @param other 源构建器
     */
    ini_builder(ini_builder&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 源构建器
     * @return 自身引用
     */
    ini_builder& operator=(ini_builder&& other) noexcept = default;

    /**
     * @brief 开始一个节
     * @param name 节名称
     * @return 自身引用
     * @note 同名节会被重置
     */
    ini_builder& begin_section(const string& name);

    /**
     * @brief 结束当前节，回到全局节
     * @return 自身引用
     */
    ini_builder& end_section();

    /**
     * @brief 设置键名
     * @param key 键名
     * @return 自身引用
     */
    ini_builder& key(const string& key);

    /**
     * @brief 设置当前键的字符串值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(string value);

    /**
     * @brief 设置当前键的 C 字符串值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(const char* value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的字符串视图值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(const string_view value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的整数值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(int value);

    /**
     * @brief 设置当前键的 64 位整数值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(int64_t value);

    /**
     * @brief 设置当前键的双精度浮点值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(double value);

    /**
     * @brief 设置当前键的布尔值
     * @param value 值
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(bool value);

    /**
     * @brief 设置当前键的浮点值，指定精度
     * @param value 值
     * @param precision 小数位数
     * @return 自身引用
     * @throws ini_exception 未设置键名时抛出
     */
    ini_builder& value(double value, int precision);

    /**
     * @brief 在函数作用域内构建一个节
     * @param name 节名称
     * @param func 构建函数
     * @return 自身引用
     */
    ini_builder& value_section(const string& name, function<void(ini_builder&)> func);

    /**
     * @brief 构建文档
     * @return 构建完成的文档
     */
    ini_document build() noexcept;
};

/** @} */ // IniConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_INI_INI_BUILDER_HPP__
