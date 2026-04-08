#ifndef NEFORCE_CORE_FILE_JSON_JSON_BUILDER_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_BUILDER_HPP__

/**
 * @file json_builder.hpp
 * @brief json配置格式构建器
 *
 * 此文件提供了json配置格式的流式构建器实现。
 * 支持链式调用和函数式编程方式构建json格式。
 */

#include "NeForce/core/container/stack.hpp"
#include "NeForce/core/file/json/json_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup JsonConfig json配置
 * @brief json配置格式管理
 * @{
 */

/**
 * @class json_builder
 * @brief json配置构建器
 *
 * 提供流式接口构建json配置格式，支持以下特性：
 * - 链式调用构建对象和数组
 * - 自动类型转换（整数、浮点数、布尔值、字符串）
 * - 函数式构建嵌套结构
 * - 集合类型自动转换（可迭代对象、映射表）
 * - 上下文管理（自动维护当前对象/数组状态）
 */
class NEFORCE_API json_builder {
private:
    /**
     * @enum range_type
     * @brief 上下文类型枚举
     */
    enum range_type {
        object,
        array
    };

    /**
     * @struct frame
     * @brief 上下文帧结构
     *
     * 保存当前构建的上下文信息，用于维护对象/数组的嵌套关系。
     */
    struct frame {
        range_type type = object; ///< 上下文类型
        union {
            json_object* object_ptr = nullptr; ///< 当前对象指针
            json_array* array_ptr;             ///< 当前数组指针
        };

        frame() = default;
        frame(const range_type t, json_object* obj) :
        type(t),
        object_ptr(obj) {}
        frame(const range_type t, json_array* arr) :
        type(t),
        array_ptr(arr) {}

        frame(const frame&) = default;
        frame& operator=(const frame&) = default;
        frame(frame&&) = default;
        frame& operator=(frame&&) = default;
        ~frame() = default;
    };

    stack<frame> contexts_;       ///< 上下文栈
    unique_ptr<json_value> root_; ///< 根节点
    string current_key_;          ///< 当前对象的键名

private:
    /**
     * @brief 值设置的通用实现
     * @tparam T json值类型
     * @param value 要设置的值指针
     * @return 自身引用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     *
     * 根据当前上下文决定值的放置位置：
     * - 顶层：设置为根节点
     * - 数组中：作为新元素添加
     * - 对象中：与当前键名配对添加
     */
    template <typename T>
    json_builder& value_impl(unique_ptr<T> value) {
        if (contexts_.empty()) {
            if (root_) {
                NEFORCE_THROW_EXCEPTION(json_exception("Multiple root values not allowed"));
            }
            root_ = _NEFORCE move(value);
        } else {
            const auto& top = contexts_.top();
            if (top.type == array) {
                top.array_ptr->add_element(_NEFORCE move(value));
            } else if (top.type == object) {
                if (current_key_.empty()) {
                    NEFORCE_THROW_EXCEPTION(json_exception("No key set for value in object"));
                }
                top.object_ptr->add_member(current_key_, _NEFORCE move(value));
                current_key_.clear();
            }
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
    enable_if_t<is_iterable_v<Iterable>, json_builder&> value_iterable_dispatch(const Iterable& iterable) {
        return this->value_iterable_impl(iterable);
    }

    /**
     * @brief 映射表类型的实现（转换为json对象）
     * @tparam Map 映射表类型
     * @param maplike 映射表对象
     * @return 自身引用
     */
    template <typename Map>
    enable_if_t<is_maplike_v<Map>, json_builder&> value_iterable_impl(const Map& maplike) {
        begin_object();
        for (const auto& pair: maplike) {
            this->key(pair.first).value(pair.second);
        }
        end_object();
        return *this;
    }

    /**
     * @brief 一般可迭代类型的实现（转换为json数组）
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用
     */
    template <typename Iterable>
    enable_if_t<!is_maplike_v<Iterable>, json_builder&> value_iterable_impl(const Iterable& iterable) {
        begin_array();
        for (const auto& element: iterable) {
            this->value(element);
        }
        end_array();
        return *this;
    }

public:
    /**
     * @brief 默认构造函数
     */
    json_builder() = default;

    json_builder(const json_builder&) = delete;
    json_builder& operator=(const json_builder&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源构建器
     */
    json_builder(json_builder&& other) = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源构建器
     * @return 自身引用
     */
    json_builder& operator=(json_builder&& other) = default;

    /**
     * @brief 开始一个json对象
     * @return 自身引用，支持链式调用
     * @throws json_exception 当根节点已存在或键名缺失时抛出
     *
     * 创建一个新的json对象，并压入上下文栈。
     * 如果当前在顶层，该对象将成为根节点；
     * 否则作为当前上下文的值添加。
     */
    json_builder& begin_object();

    /**
     * @brief 开始一个json数组
     * @return 自身引用，支持链式调用
     * @throws json_exception 当根节点已存在或键名缺失时抛出
     *
     * 创建一个新的json数组，并压入上下文栈。
     * 如果当前在顶层，该数组将成为根节点；
     * 否则作为当前上下文的值添加。
     */
    json_builder& begin_array();

    /**
     * @brief 结束当前json对象
     * @return 自身引用，支持链式调用
     * @throws json_exception 当没有对象可关闭或存在未完成的键值对时抛出
     *
     * 弹出上下文栈顶的对象，返回上一层。
     * 必须与 begin_object 配对使用。
     */
    json_builder& end_object();

    /**
     * @brief 结束当前json数组
     * @return 自身引用，支持链式调用
     * @throws json_exception 当没有数组可关闭时抛出
     *
     * 弹出上下文栈顶的数组，返回上一层。
     * 必须与 begin_array 配对使用。
     */
    json_builder& end_array();

    /**
     * @brief 设置当前对象的键名
     * @param key 键名
     * @return 自身引用，支持链式调用
     * @throws json_exception 当不在对象上下文中或键名已设置时抛出
     *
     * 为后续的值设置键名，仅在对象上下文中有效。
     */
    json_builder& key(const string& key);

    /**
     * @brief 设置null值
     * @param np 空指针标记
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(nullptr_t np) { return value_impl(make_unique<json_null>()); }

    /**
     * @brief 设置字符串值
     * @param value 字符串值
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(const string& value) { return value_impl(make_unique<json_string>(value)); }

    /**
     * @brief 设置C字符串值
     * @param value C字符串
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(const char* value) { return this->value(string(value)); }

    /**
     * @brief 设置字符串视图值
     * @param value 字符串视图
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(const string_view value) { return this->value(string(value)); }

    /**
     * @brief 设置双精度浮点数值
     * @param value 双精度浮点数
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(const double value) { return value_impl(make_unique<json_number>(value)); }

    /**
     * @brief 设置整数值
     * @param value 整数
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(const int value) { return value_impl(make_unique<json_number>(static_cast<double>(value))); }

    /**
     * @brief 设置布尔值
     * @param value 布尔值
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(const bool value) { return value_impl(make_unique<json_bool>(value)); }

    /**
     * @brief 设置已构建的json值
     * @param value json值指针
     * @return 自身引用，支持链式调用
     * @throws json_exception 当上下文无效或键名缺失时抛出
     */
    json_builder& value(unique_ptr<json_value>&& value) { return value_impl(_NEFORCE move(value)); }

    /**
     * @brief 设置可迭代对象的值
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用，支持链式调用
     *
     * 根据类型自动转换为json对象或数组：
     * - 映射表类型转换为对象
     * - 其他可迭代类型转换为数组
     */
    template <typename Iterable>
    json_builder& value(const Iterable& iterable) {
        return this->value_iterable_dispatch(iterable);
    }

    /**
     * @brief 使用函数式方式构建对象值
     * @param build_func 构建函数，接收json_builder引用
     * @return 自身引用，支持链式调用
     *
     * 在独立的作用域内构建一个对象，构建完成后自动闭合。
     */
    json_builder& value_object(const function<void(json_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建数组值
     * @param build_func 构建函数，接收json_builder引用
     * @return 自身引用，支持链式调用
     *
     * 在独立的作用域内构建一个数组，构建完成后自动闭合。
     */
    json_builder& value_array(const function<void(json_builder&)>& build_func);

    /**
     * @brief 构建json文档
     * @return 构建完成的json值根节点
     * @throws json_exception 当存在未闭合的结构或未完成的键值对时抛出
     *
     * 完成构建过程，返回构建好的json值。
     * 调用前必须确保所有对象/数组都已闭合，且没有未完成的键值对。
     */
    unique_ptr<json_value> build();
};

/** @} */ // JsonConfig

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_BUILDER_HPP__
