#ifndef NEFORCE_CORE_FILE_INI_INI_BUILDER_HPP__
#define NEFORCE_CORE_FILE_INI_INI_BUILDER_HPP__

/**
 * @file ini_builder.hpp
 * @brief ini配置格式构建器
 *
 * 此文件提供了ini配置格式的流式构建器实现。
 * 支持链式调用方式构建ini格式，
 * 包括节(section)的创建、键值对的设置，以及嵌套节的定义。
 */

#include "NeForce/core/file/ini/ini_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup IniConfig ini配置
 * @brief ini配置格式管理
 * @{
 */

/**
 * @class ini_builder
 * @brief ini配置格式构建器
 *
 * 提供流式接口构建ini配置格式，支持以下特性：
 * - 链式调用设置节和键值对
 * - 支持多种数据类型自动转换
 * - 支持嵌套节的函数式定义
 * - 浮点数精度控制
 * - 自动维护当前节上下文
 */
class NEFORCE_API ini_builder {
private:
    unique_ptr<ini_document> root_;   ///< 正在构建的文档对象
    ini_section* current_section_ = nullptr;   ///< 当前正在操作的节
    string current_key_;   ///< 当前正在设置的键名

    /**
     * @brief 测试当前上下文是否有效
     * @throws ini_exception 当没有有效的节或键名时抛出
     *
     * 检查是否存在有效的节和键名设置。
     * 如果上下文无效则抛出异常。
     */
    void test_exception() const;

public:
    /**
     * @brief 构造函数
     *
     * 创建一个新的构建器实例，初始化空文档，
     * 并将当前节设置为全局节。
     */
    ini_builder();

    ini_builder(const ini_builder&) = delete;
    ini_builder& operator =(const ini_builder&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源构建器
     */
    ini_builder(ini_builder&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源构建器
     * @return 自身引用
     */
    ini_builder& operator =(ini_builder&& other) = default;

    /**
     * @brief 开始一个新节
     * @param name 节名称
     * @return 自身引用，支持链式调用
     *
     * 创建指定名称的新节，并将其设置为当前节。
     * 后续的键值对操作将添加到此节中。
     */
    ini_builder& begin_section(const string& name);

    /**
     * @brief 结束当前节
     * @return 自身引用，支持链式调用
     *
     * 将当前节切换回全局节。
     */
    ini_builder& end_section();

    /**
     * @brief 设置当前键名
     * @param key 属性键名
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节上下文时抛出
     *
     * 设置当前要操作的键名，后续的 value 调用将使用此键名。
     */
    ini_builder& key(const string& key);

    /**
     * @brief 设置当前键的值（字符串版本）
     * @param value 属性值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(string value);

    /**
     * @brief 设置当前键的值（C字符串版本）
     * @param value 属性值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(const char* value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的值（字符串视图版本）
     * @param value 属性值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(const string_view value) { return this->value(string(value)); }

    /**
     * @brief 设置当前键的值（整数版本）
     * @param value 整数值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(int value);

    /**
     * @brief 设置当前键的值（64位整数版本）
     * @param value 64位整数值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(int64_t value);

    /**
     * @brief 设置当前键的值（浮点数版本）
     * @param value 浮点数值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(double value);

    /**
     * @brief 设置当前键的值（布尔值版本）
     * @param value 布尔值
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(bool value);

    /**
     * @brief 设置当前键的值（浮点数版本，指定精度）
     * @param value 浮点数值
     * @param precision 小数精度
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当没有有效的节或键名时抛出
     */
    ini_builder& value(double value, int precision);

    /**
     * @brief 使用函数式方式定义值节
     * @param name 节名称
     * @param func 配置函数，接收ini_builder引用作为参数
     * @return 自身引用，支持链式调用
     * @throws ini_exception 当节创建或配置过程中出错时抛出
     *
     * 在一个独立的函数作用域内定义节的内容，
     * 函数执行完毕后自动返回当前节。
     */
    ini_builder& value_section(const string& name, _NEFORCE function<void(ini_builder&)>&& func);

    /**
     * @brief 构建文档
     * @return 构建完成的文档对象
     *
     * 完成构建过程，返回构建好的ini文档。
     * 调用后构建器状态被移动，不应继续使用。
     */
    unique_ptr<ini_document> build() noexcept;
};

/** @} */ // IniConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_INI_INI_BUILDER_HPP__
