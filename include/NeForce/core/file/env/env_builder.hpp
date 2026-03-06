#ifndef NEFORCE_CORE_FILE_ENV_ENV_BUILDER_HPP__
#define NEFORCE_CORE_FILE_ENV_ENV_BUILDER_HPP__

/**
 * @file env_builder.hpp
 * @brief env配置格式构建器
 *
 * 此文件提供了env配置格式的流式构建器实现。
 * 支持链式调用方式构建env格式，
 * 包括变量定义、注释、引号类型和导出标记的设置。
 */

#include "NeForce/core/file/env/env_value.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup EnvConfig env配置
 * @brief env配置格式管理
 * @{
 */

/**
 * @class env_builder
 * @brief env配置格式构建器
 *
 * 提供流式接口构建env配置格式，支持以下特性：
 * - 链式调用设置变量键值对
 * - 支持多种数据类型自动转换
 * - 可配置引号类型
 * - 支持export标记
 * - 可添加注释和空行
 * - 浮点数精度控制
 */
class NEFORCE_API env_builder {
private:
    unique_ptr<env_document> root_;    ///< 正在构建的文档对象
    string current_key_;               ///< 当前正在设置的键名
    env_variable::quote_type current_quote_type_ = env_variable::None;  ///< 当前值的引号类型
    bool current_exported_ = false;    ///< 当前变量是否导出

public:
    /**
     * @brief 构造函数
     *
     * 创建一个新的构建器实例，初始化空文档。
     */
    env_builder();

    env_builder(const env_builder&) = delete;
    env_builder& operator =(const env_builder&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源构建器
     */
    env_builder(env_builder&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源构建器
     * @return 自身引用
     */
    env_builder& operator =(env_builder&& other) noexcept = default;

    /**
     * @brief 设置当前键名
     * @param key 变量键名
     * @return 自身引用，支持链式调用
     */
    env_builder& key(string key) noexcept;

    /**
     * @brief 设置为无引号模式
     * @return 自身引用，支持链式调用
     */
    env_builder& unquoted() noexcept;

    /**
     * @brief 设置为单引号模式
     * @return 自身引用，支持链式调用
     */
    env_builder& single_quoted() noexcept;

    /**
     * @brief 设置为双引号模式
     * @return 自身引用，支持链式调用
     */
    env_builder& double_quoted() noexcept;

    /**
     * @brief 设置导出标记
     * @param exported 是否导出，默认为true
     * @return 自身引用，支持链式调用
     */
    env_builder& exported(bool exported = true) noexcept;

    /**
     * @brief 设置当前键的值（字符串版本）
     * @param value 变量值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(string value);

    /**
     * @brief 设置当前键的值（C字符串版本）
     * @param value 变量值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(const char* value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的值（字符串视图版本）
     * @param value 变量值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(const string_view value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的值（整数版本）
     * @param value 整数值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(int value);

    /**
     * @brief 设置当前键的值（64位整数版本）
     * @param value 64位整数值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(int64_t value);

    /**
     * @brief 设置当前键的值（浮点数版本）
     * @param value 浮点数值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(double value);

    /**
     * @brief 设置当前键的值（布尔值版本）
     * @param value 布尔值
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(bool value);

    /**
     * @brief 设置当前键的值（浮点数版本，指定精度）
     * @param value 浮点数值
     * @param precision 小数精度
     * @return 自身引用，支持链式调用
     * @throws env_exception 当未设置键名时抛出
     */
    env_builder& value(double value, int precision);

    /**
     * @brief 添加注释
     * @param text 注释文本
     * @return 自身引用，支持链式调用
     */
    env_builder& comment(string text) noexcept;

    /**
     * @brief 添加空行
     * @return 自身引用，支持链式调用
     */
    env_builder& blank_line() noexcept;

    /**
     * @brief 直接添加键值对（字符串版本）
     * @param key 变量键名
     * @param value 变量值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, string value);

    /**
     * @brief 直接添加键值对（字符串视图版本）
     * @param key 变量键名
     * @param value 变量值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, const string_view value) { return add(move(key), string(value)); }

    /**
     * @brief 直接添加键值对（C字符串版本）
     * @param key 变量键名
     * @param value 变量值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, const char* value) { return add(move(key), string(value)); }

    /**
     * @brief 直接添加键值对（整数版本）
     * @param key 变量键名
     * @param value 整数值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, int value);

    /**
     * @brief 直接添加键值对（64位整数版本）
     * @param key 变量键名
     * @param value 64位整数值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, int64_t value);

    /**
     * @brief 直接添加键值对（浮点数版本）
     * @param key 变量键名
     * @param value 浮点数值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, double value);

    /**
     * @brief 直接添加键值对（布尔值版本）
     * @param key 变量键名
     * @param value 布尔值
     * @return 自身引用，支持链式调用
     */
    env_builder& add(string key, bool value);

    /**
     * @brief 添加导出变量
     * @param key 变量键名
     * @param value 变量值
     * @return 自身引用，支持链式调用
     *
     * 快捷方法，自动设置export标记。
     */
    env_builder& add_export(string key, string value);

    /**
     * @brief 构建文档
     * @return 构建完成的文档对象
     *
     * 完成构建过程，返回构建好的环境变量文档。
     * 调用后构建器状态被移动，不应继续使用。
     */
    unique_ptr<env_document> build() noexcept;
};

/** @} */ // EnvConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_ENV_ENV_BUILDER_HPP__
