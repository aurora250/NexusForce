#ifndef NEFORCE_CORE_FILE_YAML_YAML_BUILDER_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_BUILDER_HPP__

/**
 * @file yaml_builder.hpp
 * @brief YAML配置格式构建器
 *
 * 此文件提供了YAML配置格式的流式构建器实现。
 * 支持链式调用和函数式编程方式构建YAML格式。
 * 遵循YAML 1.2标准。
 */

#include "NeForce/core/container/stack.hpp"
#include "NeForce/core/file/yaml/yaml_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup YamlConfig YAML配置
 * @{
 */

/**
 * @class yaml_builder
 * @brief YAML配置构建器
 *
 * 提供流式接口构建YAML配置格式，支持以下特性：
 * - 链式调用构建映射和序列
 * - 自动类型转换（整数、浮点数、布尔值、字符串）
 * - 函数式构建嵌套结构
 * - 集合类型自动转换（可迭代对象、映射表）
 * - 上下文管理（自动维护当前映射/序列状态）
 * - 支持所有YAML字符串类型（Plain, SingleQuoted, DoubleQuoted, Literal, Folded）
 * - 支持时间戳类型
 * - 支持锚点、别名和标签
 * - 支持块样式和流样式
 * - 支持多文档
 */
class NEFORCE_API yaml_builder {
private:
    /**
     * @struct frame
     * @brief 上下文帧结构
     *
     * 保存当前构建的上下文信息，用于维护映射/序列的嵌套关系。
     */
    struct frame {
        enum context_type {
            mapping,
            sequence
        };

        context_type type = mapping;
        union {
            yaml_mapping* map_ptr = nullptr;
            yaml_sequence* seq_ptr;
        };

        frame() = default;
        frame(const context_type t, yaml_mapping* m) :
        type(t),
        map_ptr(m) {}
        frame(const context_type t, yaml_sequence* s) :
        type(t),
        seq_ptr(s) {}

        frame(const frame&) = default;
        frame& operator=(const frame&) = default;
        frame(frame&&) = default;
        frame& operator=(frame&&) = default;
        ~frame() = default;
    };

    stack<frame> contexts_;                                 ///< 上下文栈
    shared_ptr<yaml_value> root_;                           ///< 根值
    string current_key_;                                    ///< 当前键名
    string pending_anchor_;                                 ///< 待应用的锚点名
    string pending_tag_;                                    ///< 待应用的标签
    unordered_map<string, shared_ptr<yaml_value>> anchors_; ///< 锚点注册表
    vector<shared_ptr<yaml_value>> documents_;              ///< 多文档存储

    /**
     * @brief 应用待处理的锚点和标签到值上
     * @param value 要应用的值
     */
    void apply_pending_metadata(const shared_ptr<yaml_value>& value);

    /**
     * @brief 将值添加到当前上下文
     * @tparam T YAML值类型
     * @param value 要添加的值
     * @return 自身引用
     * @throws yaml_exception 当上下文无效、键名缺失或键重复时抛出
     */
    template <typename T>
    yaml_builder& value_impl(shared_ptr<T> value) {
        apply_pending_metadata(value);

        if (contexts_.empty()) {
            NEFORCE_THROW_EXCEPTION(yaml_exception("Cannot add value: no active context"));
        }

        const auto& top = contexts_.top();
        if (top.type == frame::sequence) {
            top.seq_ptr->add_element(value);
        } else if (top.type == frame::mapping) {
            if (current_key_.empty()) {
                NEFORCE_THROW_EXCEPTION(yaml_exception("No key set for value in mapping"));
            }
            if (top.map_ptr->has_member(current_key_)) {
                NEFORCE_THROW_EXCEPTION(yaml_exception(("Duplicate key: " + current_key_).data()));
            }
            top.map_ptr->add_member(current_key_, value);
            current_key_.clear();
        }
        return *this;
    }

    template <typename Iterable>
    enable_if_t<is_maplike_v<Iterable>> value_iterable_impl(const Iterable& iterable) {
        begin_flow_mapping();
        for (const auto& pair: iterable) {
            this->key(pair.first).value(pair.second);
        }
        end_mapping();
        return;
    }

    template <typename Iterable>
    enable_if_t<!is_maplike_v<Iterable>> value_iterable_impl(const Iterable& iterable) {
        begin_sequence();
        for (const auto& element: iterable) {
            this->value(element);
        }
        end_sequence();
        return;
    }

    /**
     * @brief 将新容器添加到父上下文并压入上下文栈
     */
    void add_to_parent_and_push(const shared_ptr<yaml_value>& container, frame f);

public:
    /**
     * @brief 构造函数
     *
     * 创建一个新的构建器实例。默认根为块样式映射。
     */
    yaml_builder();

    yaml_builder(const yaml_builder&) = delete;
    yaml_builder& operator=(const yaml_builder&) = delete;

    /**
     * @brief 移动构造函数
     */
    yaml_builder(yaml_builder&& other) = default;

    /**
     * @brief 移动赋值运算符
     */
    yaml_builder& operator=(yaml_builder&& other) = default;

    /**
     * @brief 设置当前键名
     * @param key 键名字符串
     * @return 自身引用，支持链式调用
     * @throws yaml_exception 当不在映射上下文中时抛出
     */
    yaml_builder& key(string key);

    /**
     * @brief 开始一个映射（块样式）
     * @return 自身引用，支持链式调用
     * @throws yaml_exception 当上下文无效或键名缺失时抛出
     */
    yaml_builder& begin_mapping();

    /**
     * @brief 开始一个块样式映射
     * @return 自身引用，支持链式调用
     */
    yaml_builder& begin_block_mapping();

    /**
     * @brief 开始一个流样式映射
     * @return 自身引用，支持链式调用
     */
    yaml_builder& begin_flow_mapping();

    /**
     * @brief 结束当前映射
     * @return 自身引用，支持链式调用
     * @throws yaml_exception 当不在映射上下文中时抛出
     */
    yaml_builder& end_mapping();

    /**
     * @brief 开始一个序列（块样式）
     * @return 自身引用，支持链式调用
     * @throws yaml_exception 当上下文无效或键名缺失时抛出
     */
    yaml_builder& begin_sequence();

    /**
     * @brief 开始一个块样式序列
     * @return 自身引用，支持链式调用
     */
    yaml_builder& begin_block_sequence();

    /**
     * @brief 开始一个流样式序列
     * @return 自身引用，支持链式调用
     */
    yaml_builder& begin_flow_sequence();

    /**
     * @brief 结束当前序列
     * @return 自身引用，支持链式调用
     * @throws yaml_exception 当不在序列上下文中时抛出
     */
    yaml_builder& end_sequence();

    /**
     * @brief 设置null值
     * @param np 空指针标记
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(nullptr_t np) { return value_impl(make_shared<yaml_null>()); }

    /**
     * @brief 设置布尔值
     * @param v 布尔值
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(const bool v) { return value_impl(make_shared<yaml_boolean>(v)); }

    /**
     * @brief 设置64位整数值
     * @param v 64位整数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(const int64_t v) { return value_impl(make_shared<yaml_integer>(v)); }

    /**
     * @brief 设置整数值
     * @param v 整数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(const int v) { return value_impl(make_shared<yaml_integer>(static_cast<int64_t>(v))); }

    /**
     * @brief 设置双精度浮点数值
     * @param v 双精度浮点数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(const double v) { return value_impl(make_shared<yaml_float>(v)); }

    /**
     * @brief 设置字符串值（纯文本样式）
     * @param v 字符串值
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(string v) { return value_impl(make_shared<yaml_string>(_NEFORCE move(v), yaml_string::Plain)); }

    /**
     * @brief 设置C字符串值
     * @param v C字符串
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(const char* v) { return this->value(string(v)); }

    /**
     * @brief 设置字符串视图值
     * @param v 字符串视图
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(const string_view v) { return this->value(string(v)); }

    /**
     * @brief 设置已构建的YAML值
     * @param v YAML值指针
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value(shared_ptr<yaml_value> v) { return value_impl(_NEFORCE move(v)); }

    /**
     * @brief 设置时间戳值
     * @param dt 日期时间对象
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_datetime(const datetime& dt);

    /**
     * @brief 设置指定类型的字符串值
     * @param v 字符串值
     * @param style 字符串样式
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_string(string v, yaml_string::string_style style);

    /**
     * @brief 设置可迭代对象的值
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用，支持链式调用
     *
     * 根据类型自动转换为序列或内联映射：
     * - 映射表类型转换为流样式映射
     * - 其他可迭代类型转换为序列
     */
    template <typename Iterable, enable_if_t<is_iterable_v<Iterable>, int> = 0>
    yaml_builder& value_iterable(const Iterable& iterable) {
        value_iterable_impl(iterable);
        return *this;
    }

    /**
     * @brief 使用函数式方式构建映射值
     * @param build_func 构建函数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_mapping(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建块样式映射值
     * @param build_func 构建函数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_block_mapping(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建流样式映射值
     * @param build_func 构建函数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_flow_mapping(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建序列值
     * @param build_func 构建函数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_sequence(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建块样式序列值
     * @param build_func 构建函数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_block_sequence(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式方式构建流样式序列值
     * @param build_func 构建函数
     * @return 自身引用，支持链式调用
     */
    yaml_builder& value_flow_sequence(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 为下一个值设置锚点
     * @param name 锚点名
     * @return 自身引用，支持链式调用
     *
     * 锚点将应用到下一个通过value()或begin_*添加的值上。
     */
    yaml_builder& anchor(string name);

    /**
     * @brief 为下一个值设置标签
     * @param t 标签字符串（如"!mytag"、"!!str"）
     * @return 自身引用，支持链式调用
     *
     * 标签将应用到下一个通过value()或begin_*添加的值上。
     */
    yaml_builder& tag(string t);

    /**
     * @brief 添加一个别名引用
     * @param name 锚点名
     * @return 自身引用，支持链式调用
     * @throws yaml_exception 当锚点不存在时抛出
     *
     * 引用之前通过anchor()设置的同名节点。
     */
    yaml_builder& alias(const string& name);

    /**
     * @brief 开始新文档（多文档模式）
     * @return 自身引用，支持链式调用
     *
     * 保存当前文档并开始新文档。新文档的根默认为块样式映射。
     */
    yaml_builder& begin_document();

    /**
     * @brief 构建YAML文档树
     * @return 构建完成的根值
     * @throws yaml_exception 当存在未闭合的上下文时抛出
     */
    shared_ptr<yaml_value> build();

    /**
     * @brief 构建多文档YAML文档树
     * @return 所有文档的根值列表
     * @throws yaml_exception 当存在未闭合的上下文时抛出
     */
    vector<shared_ptr<yaml_value>> build_documents();
};

/** @} */ // YamlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_BUILDER_HPP__
