#ifndef NEFORCE_CORE_FILE_TOML_BUILDER_HPP__
#define NEFORCE_CORE_FILE_TOML_BUILDER_HPP__

/**
 * @file toml_builder.hpp
 * @brief TOML配置格式构建器
 *
 * 此文件提供了TOML配置格式的流式构建器实现。
 * 支持链式调用和函数式编程方式构建TOML格式。
 */

#include "NeForce/core/file/toml/toml_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup TomlConfig TOML配置
 * @{
 */

/**
 * @class toml_builder
 * @brief TOML配置构建器
 *
 * 提供流式接口构建TOML配置格式，支持以下特性：
 * - 链式调用构建表格和数组
 * - 自动类型转换（整数、浮点数、布尔值、字符串）
 * - 函数式构建嵌套结构
 * - 集合类型自动转换（可迭代对象、映射表）
 * - 上下文管理（自动维护当前表格/数组状态）
 * - 支持所有TOML字符串类型（Basic, Literal, Multi-line）
 * - 支持所有TOML日期时间类型
 */
class NEFORCE_API toml_builder {
private:
    /**
     * @enum range_type
     * @brief 上下文类型枚举
     */
    enum range_type {
        table,        ///< 标准表格
        inline_table, ///< 内联表格
        array         ///< 数组
    };

    /**
     * @struct frame
     * @brief 上下文帧结构
     *
     * 保存当前构建的上下文信息，用于维护表格/数组的嵌套关系。
     */
    struct frame {
        range_type type = table;    ///< 上下文类型
        toml_value* node = nullptr; ///< 当前容器节点
        string pending_key;         ///< 待写入成员的键
    };

    vector<frame> frames_;   ///< 上下文栈
    toml_document document_; ///< 承载 arena 与根值

private:
    /**
     * @brief 将值写入当前上下文
     * @param value 要写入的值
     * @throws toml_exception 当上下文无效或键名缺失、重复时抛出
     */
    void push_value(toml_value value);

    /**
     * @brief 打开容器并把容器节点挂到当前上下文
     * @param is_table 是否为表格
     * @param is_inline 表格是否内联
     * @return 容器节点
     * @throws toml_exception 当上下文无效或键名缺失、重复时抛出
     */
    toml_value* open_container(bool is_table, bool is_inline);

    /**
     * @brief 关闭当前容器
     * @param expected 期望的上下文类型
     * @throws toml_exception 上下文类型不匹配或试图关闭根表格时抛出
     */
    void close_container(range_type expected);

    /**
     * @brief 映射表类型的实现（转换为内联表格）
     * @tparam Map 映射表类型
     * @param maplike 映射表对象
     * @return 自身引用
     */
    template <typename Map>
    enable_if_t<is_maplike_v<Map>, toml_builder&> value_iterable_impl(const Map& maplike) {
        begin_inline_table();
        for (const auto& pair: maplike) {
            this->key(pair.first).value(pair.second);
        }
        end_inline_table();
        return *this;
    }

    /**
     * @brief 一般可迭代类型的实现（转换为数组）
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用
     */
    template <typename Iterable>
    enable_if_t<!is_maplike_v<Iterable>, toml_builder&> value_iterable_impl(const Iterable& iterable) {
        begin_array();
        for (const auto& element: iterable) {
            this->value(element);
        }
        end_array();
        return *this;
    }

    /**
     * @brief 获取或创建表格路径
     * @param path 表格路径
     * @return 目标表格指针
     * @throws toml_exception 当路径中的键已存在但不是表格时抛出
     */
    NEFORCE_NODISCARD toml_value* get_or_create_table_path(const vector<string>& path);

    /**
     * @brief 获取或创建表格数组
     * @param path 表格数组路径
     * @return 目标数组指针
     * @throws toml_exception 当路径中的键已存在但不是数组时抛出
     */
    NEFORCE_NODISCARD toml_value* get_or_create_array_for_array_table(const vector<string>& path);

public:
    /**
     * @brief 构造函数
     *
     * 创建一个新的构建器实例，初始化根表格。
     */
    toml_builder();

    toml_builder(const toml_builder&) = delete;
    toml_builder& operator=(const toml_builder&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源构建器
     */
    toml_builder(toml_builder&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源构建器
     * @return 自身引用
     */
    toml_builder& operator=(toml_builder&& other) noexcept = default;

    /**
     * @brief 设置当前键名
     * @param key 键名字符串
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当不在表格上下文中时抛出
     */
    toml_builder& key(string key);

    /**
     * @brief 开始一个标准表格（单键版本）
     * @param name 表格名称
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当路径无效时抛出
     */
    toml_builder& begin_table(const string& name);

    /**
     * @brief 开始一个标准表格（路径版本）
     * @param path 表格路径
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当路径无效时抛出
     *
     * 格式：[parent.child.grandchild]
     */
    toml_builder& begin_table(const vector<string>& path);

    /**
     * @brief 结束当前标准表格
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当没有表格可结束或试图结束根表格时抛出
     */
    toml_builder& end_table();

    /**
     * @brief 开始一个内联表格
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效或键名缺失时抛出
     *
     * 格式：{ key = value, key2 = value2 }
     */
    toml_builder& begin_inline_table();

    /**
     * @brief 结束当前内联表格
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当不在内联表格上下文中时抛出
     */
    toml_builder& end_inline_table();

    /**
     * @brief 开始一个数组
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效或键名缺失时抛出
     *
     * 格式：[element1, element2, element3]
     */
    toml_builder& begin_array();

    /**
     * @brief 结束当前数组
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当不在数组上下文中时抛出
     */
    toml_builder& end_array();

    /**
     * @brief 开始一个表格数组（单键版本）
     * @param name 表格数组名称
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当路径无效时抛出
     *
     * 格式：[[array_name]]
     */
    toml_builder& begin_array_table(const string& name);

    /**
     * @brief 开始一个表格数组（路径版本）
     * @param path 表格数组路径
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当路径无效时抛出
     *
     * 格式：[[parent.child.array]]
     */
    toml_builder& begin_array_table(const vector<string>& path);

    /**
     * @brief 结束当前表格数组
     * @return 自身引用，支持链式调用
     *
     * 等同于 end_table()
     */
    toml_builder& end_array_table();

    /**
     * @brief 设置布尔值
     * @param value 布尔值
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const bool value) {
        push_value(toml_value::make_boolean(value));
        return *this;
    }

    /**
     * @brief 设置空值占位（映射为 false）
     * @param np 空指针标记
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(nullptr_t np) {
        (void) np;
        return this->value(false);
    }

    /**
     * @brief 设置64位整数值
     * @param value 64位整数
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const int64_t value) {
        push_value(toml_value::make_integer(value));
        return *this;
    }

    /**
     * @brief 设置整数值
     * @param value 整数
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const int value) { return this->value(static_cast<int64_t>(value)); }

    /**
     * @brief 设置双精度浮点数值
     * @param value 双精度浮点数
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const double value) {
        push_value(toml_value::make_float(value));
        return *this;
    }

    /**
     * @brief 设置字符串值（基本字符串类型）
     * @param value 字符串值
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const string& value) {
        push_value(document_.make_string_value(string_view(value.data(), value.size()), toml_basic_string));
        return *this;
    }

    /**
     * @brief 设置C字符串值
     * @param value C字符串
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const char* value) {
        push_value(document_.make_string_value(string_view(value), toml_basic_string));
        return *this;
    }

    /**
     * @brief 设置字符串视图值
     * @param value 字符串视图
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const string_view value) {
        push_value(document_.make_string_value(value, toml_basic_string));
        return *this;
    }

    /**
     * @brief 设置已构建的TOML值
     * @param value TOML值
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const toml_value value) {
        push_value(value);
        return *this;
    }

    /**
     * @brief 设置指定类型的字符串值
     * @param value 字符串值
     * @param type 字符串类型（Basic, Literal, MultiBasic, MultiLiteral）
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value_string(const string_view value, const toml_string_type type) {
        push_value(document_.make_string_value(value, type));
        return *this;
    }

    /**
     * @brief 设置指定类型的字符串值
     * @param value 字符串值
     * @param type 字符串类型（Basic, Literal, MultiBasic, MultiLiteral）
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value_string(const string_view value, const toml_string::string_type type) {
        return value_string(value, static_cast<toml_string_type>(type));
    }

    /**
     * @brief 设置指定类型的日期时间值
     * @param value 日期时间字符串
     * @param type 日期时间类型
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value_datetime(const string_view value, const toml_datetime_type type) {
        push_value(document_.make_datetime_value(value, type));
        return *this;
    }

    /**
     * @brief 设置指定类型的日期时间值
     * @param value 日期时间字符串
     * @param type 日期时间类型
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value_datetime(const string_view value, const toml_datetime::datetime_type type) {
        return value_datetime(value, static_cast<toml_datetime_type>(type));
    }

    /**
     * @brief 设置可迭代对象的值
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用，支持链式调用
     *
     * 根据类型自动转换为数组或内联表格：
     * - 映射表类型（如unordered_map）转换为内联表格
     * - 其他可迭代类型转换为数组
     */
    template <typename Iterable, enable_if_t<is_iterable_v<Iterable>, int> = 0>
    toml_builder& value_iterable(const Iterable& iterable) {
        return value_iterable_impl(iterable);
    }

    /**
     * @brief 使用函数式方式构建表格值
     * @param build_func 构建函数，接收toml_builder引用
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     *
     * 在独立的作用域内构建一个标准表格。
     */
    toml_builder& value_table(const function<void(toml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建内联表格值
     * @param build_func 构建函数，接收toml_builder引用
     * @return 自身引用，支持链式调用
     *
     * 在独立的作用域内构建一个内联表格。
     */
    toml_builder& value_inline_table(const function<void(toml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建数组值
     * @param build_func 构建函数，接收toml_builder引用
     * @return 自身引用，支持链式调用
     *
     * 在独立的作用域内构建一个数组。
     */
    toml_builder& value_array(const function<void(toml_builder&)>& build_func);

    /**
     * @brief 构建TOML文档
     * @return 构建完成的文档，根值为表格
     * @throws toml_exception 当存在未闭合的上下文时抛出
     */
    toml_document build();
};

/** @} */ // TomlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TOML_BUILDER_HPP__
