#ifndef NEFORCE_CORE_FILE_TOML_BUILDER_HPP__
#define NEFORCE_CORE_FILE_TOML_BUILDER_HPP__

/**
 * @file toml_builder.hpp
 * @brief toml配置格式构建器
 *
 * 此文件提供了toml配置格式的流式构建器实现。
 * 支持链式调用和函数式编程方式构建toml格式。
 */

#include "NeForce/core/container/stack.hpp"
#include "NeForce/core/file/toml/toml_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup TomlConfig toml配置
 * @brief toml配置格式管理
 * @{
 */

/**
 * @class toml_builder
 * @brief toml配置构建器
 *
 * 提供流式接口构建toml配置格式，支持以下特性：
 * - 链式调用构建表格和数组
 * - 自动类型转换（整数、浮点数、布尔值、字符串）
 * - 函数式构建嵌套结构
 * - 集合类型自动转换（可迭代对象、映射表）
 * - 上下文管理（自动维护当前表格/数组状态）
 * - 支持所有toml字符串类型（Basic, Literal, Multi-line）
 * - 支持所有toml日期时间类型
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
        range_type type = table; ///< 上下文类型
        union {
            toml_table* table_ptr = nullptr; ///< 当前表格指针
            toml_array* array_ptr;           ///< 当前数组指针
        };

        frame() = default;
        frame(const range_type t, toml_table* tbl) :
        type(t),
        table_ptr(tbl) {}
        frame(const range_type t, toml_array* arr) :
        type(t),
        array_ptr(arr) {}

        frame(const frame&) = default;
        frame& operator=(const frame&) = default;
        frame(frame&&) = default;
        frame& operator=(frame&&) = default;
        ~frame() = default;
    };

    stack<frame> contexts_;       ///< 上下文栈
    unique_ptr<toml_table> root_; ///< 根表格
    string current_key_;          ///< 当前键名

private:
    /**
     * @brief 值设置的通用实现
     * @tparam T toml值类型
     * @param value 要设置的值指针
     * @return 自身引用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     *
     * 根据当前上下文决定值的放置位置：
     * - 数组中：作为新元素添加
     * - 表格中：与当前键名配对添加
     */
    template <typename T>
    toml_builder& value_impl(unique_ptr<T> value) {
        if (contexts_.empty()) {
            NEFORCE_THROW_EXCEPTION(toml_exception("Cannot add value to root (root must be a table)"));
        }

        const auto& top = contexts_.top();
        if (top.type == array) {
            top.array_ptr->add_element(_NEFORCE move(value));
        } else if (top.type == table || top.type == inline_table) {
            if (current_key_.empty()) {
                NEFORCE_THROW_EXCEPTION(toml_exception("No key set for value in table"));
            }
            if (top.table_ptr->has_member(current_key_)) {
                NEFORCE_THROW_EXCEPTION(toml_exception(("Duplicate key: " + current_key_).data()));
            }
            top.table_ptr->add_member(current_key_, _NEFORCE move(value));
            current_key_.clear();
        }
        return *this;
    }

    /**
     * @brief 可迭代类型的分派函数
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用
     */
    template <typename Iterable>
    enable_if_t<is_iterable_v<Iterable>, toml_builder&> value_iterable_dispatch(const Iterable& iterable) {
        return this->value_iterable_impl(iterable);
    }

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
     *
     * 沿路径获取表格，如果不存在则创建。
     */
    toml_table* get_or_create_table_path(const vector<string>& path) const;

    /**
     * @brief 获取或创建表格数组
     * @param path 表格数组路径
     * @return 目标数组指针
     * @throws toml_exception 当路径中的键已存在但不是数组时抛出
     *
     * 沿路径获取或创建用于表格数组的数组。
     */
    toml_array* get_or_create_array_for_array_table(const vector<string>& path) const;

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
    toml_builder(toml_builder&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源构建器
     * @return 自身引用
     */
    toml_builder& operator=(toml_builder&& other) = default;

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
     * @brief 设置null值
     * @param np 空指针标记
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(nullptr_t np) { return value_impl(make_unique<toml_boolean>(false)); }

    /**
     * @brief 设置布尔值
     * @param value 布尔值
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const bool value) { return value_impl(make_unique<toml_boolean>(value)); }

    /**
     * @brief 设置64位整数值
     * @param value 64位整数
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const int64_t value) { return value_impl(make_unique<toml_integer>(value)); }

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
    toml_builder& value(const double value) { return value_impl(make_unique<toml_float>(value)); }

    /**
     * @brief 设置字符串值（基本字符串类型）
     * @param value 字符串值
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(string value) {
        return value_impl(make_unique<toml_string>(_NEFORCE move(value), toml_string::Basic));
    }

    /**
     * @brief 设置C字符串值
     * @param value C字符串
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const char* value) { return this->value(string(value)); }

    /**
     * @brief 设置字符串视图值
     * @param value 字符串视图
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(const string_view value) { return this->value(string(value)); }

    /**
     * @brief 设置已构建的toml值
     * @param value toml值指针
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value(unique_ptr<toml_value>&& value) { return value_impl(_NEFORCE move(value)); }

    /**
     * @brief 设置指定类型的字符串值
     * @param value 字符串值
     * @param type 字符串类型（Basic, Literal, MultiBasic, MultiLiteral）
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value_string(string value, toml_string::string_type type) {
        return value_impl(make_unique<toml_string>(_NEFORCE move(value), type));
    }

    /**
     * @brief 设置指定类型的日期时间值
     * @param value 日期时间字符串
     * @param type 日期时间类型
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    toml_builder& value_datetime(const string_view value, toml_datetime::datetime_type type) {
        return value_impl(make_unique<toml_datetime>(value, type));
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
    template <typename Iterable>
    toml_builder& value(const Iterable& iterable) {
        return this->value_iterable_dispatch(iterable);
    }

    /**
     * @brief 使用函数式方式构建表格值
     * @param build_func 构建函数，接收toml_builder引用
     * @return 自身引用，支持链式调用
     * @throws toml_exception 当上下文无效、键名缺失或键重复时抛出
     *
     * 在独立的作用域内构建一个标准表格。
     */
    toml_builder& value_table(_NEFORCE function<void(toml_builder&)>&& build_func);

    /**
     * @brief 使用函数式方式构建内联表格值
     * @param build_func 构建函数，接收toml_builder引用
     * @return 自身引用，支持链式调用
     *
     * 在独立的作用域内构建一个内联表格。
     */
    toml_builder& value_inline_table(_NEFORCE function<void(toml_builder&)>&& build_func);

    /**
     * @brief 使用函数式方式构建数组值
     * @param build_func 构建函数，接收toml_builder引用
     * @return 自身引用，支持链式调用
     *
     * 在独立的作用域内构建一个数组。
     */
    toml_builder& value_array(_NEFORCE function<void(toml_builder&)>&& build_func);

    /**
     * @brief 构建toml文档
     * @return 构建完成的根表格
     * @throws toml_exception 当存在未闭合的上下文时抛出
     *
     * 完成构建过程，返回构建好的根表格。
     * 调用前必须确保所有上下文都已正确闭合。
     */
    unique_ptr<toml_table> build();
};

/** @} */ // TomlConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_TOML_BUILDER_HPP__
