#ifndef NEFORCE_CORE_FILE_ENV_ENV_BUILDER_HPP__
#define NEFORCE_CORE_FILE_ENV_ENV_BUILDER_HPP__

/**
 * @file env_builder.hpp
 * @brief ENV 构建器
 */

#include "NeForce/core/file/env/env_value.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup EnvConfig ENV配置
 * @{
 */

/**
 * @class env_builder
 * @brief 流式构建 ENV 文档
 */
class NEFORCE_API env_builder {
private:
    env_document document_;                               ///< 正在构建的文档
    string current_key_;                                  ///< 待写入的键
    env_variable::quote_type quote_ = env_variable::None; ///< 当前引号模式
    bool exported_ = false;                               ///< 当前导出标记

    void test_exception() const;
    env_builder& assign(string value);

public:
    env_builder();

    env_builder(const env_builder&) = delete;
    env_builder& operator=(const env_builder&) = delete;

    /**
     * @brief 移动构造
     * @param other 源构建器
     */
    env_builder(env_builder&& other) noexcept = default;

    /**
     * @brief 移动赋值
     * @param other 源构建器
     * @return 自身引用
     */
    env_builder& operator=(env_builder&& other) noexcept = default;

    /**
     * @brief 设置键名
     * @param key 键名
     * @return 自身引用
     */
    env_builder& key(string key) noexcept;

    /**
     * @brief 设置为无引号模式
     * @return 自身引用
     */
    env_builder& unquoted() noexcept;

    /**
     * @brief 设置为单引号模式
     * @return 自身引用
     */
    env_builder& single_quoted() noexcept;

    /**
     * @brief 设置为双引号模式
     * @return 自身引用
     */
    env_builder& double_quoted() noexcept;

    /**
     * @brief 设置导出标记
     * @param exported 是否导出
     * @return 自身引用
     */
    env_builder& exported(bool exported = true) noexcept;

    /**
     * @brief 设置当前键的字符串值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(string value);

    /**
     * @brief 设置当前键的 C 字符串值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(const char* value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的字符串视图值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(const string_view value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的整数值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(int value);

    /**
     * @brief 设置当前键的 64 位整数值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(int64_t value);

    /**
     * @brief 设置当前键的双精度浮点值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(double value);

    /**
     * @brief 设置当前键的布尔值
     * @param value 值
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(bool value);

    /**
     * @brief 设置当前键的浮点值，指定精度
     * @param value 值
     * @param precision 小数位数
     * @return 自身引用
     * @throws env_exception 未设置键名时抛出
     */
    env_builder& value(double value, int precision);

    /**
     * @brief 添加注释
     * @param text 注释内容
     * @return 自身引用
     */
    env_builder& comment(string text);

    /**
     * @brief 添加空行
     * @return 自身引用
     */
    env_builder& blank_line();

    /**
     * @brief 添加变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, string value);

    /**
     * @brief 添加字符串变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, const char* value) { return this->key(_NEFORCE move(key)).value(value); }

    /**
     * @brief 添加字符串变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, const string_view value) { return this->key(_NEFORCE move(key)).value(value); }

    /**
     * @brief 添加整数变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, int value);

    /**
     * @brief 添加 64 位整数变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, int64_t value);

    /**
     * @brief 添加浮点变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, double value);

    /**
     * @brief 添加布尔变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add(string key, bool value);

    /**
     * @brief 添加导出变量
     * @param key 键名
     * @param value 值
     * @return 自身引用
     */
    env_builder& add_export(string key, string value);

    /**
     * @brief 构建文档
     * @return 构建完成的文档
     */
    env_document build() noexcept;
};

/** @} */ // EnvConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_ENV_ENV_BUILDER_HPP__
