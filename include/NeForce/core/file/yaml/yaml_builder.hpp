#ifndef NEFORCE_CORE_FILE_YAML_YAML_BUILDER_HPP__
#define NEFORCE_CORE_FILE_YAML_YAML_BUILDER_HPP__

/**
 * @file yaml_builder.hpp
 * @brief YAML 配置构建器
 *
 * 提供流式接口构建 YAML 文档。
 */

#include "NeForce/core/container/vector.hpp"
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
 */
class NEFORCE_API yaml_builder {
private:
    /**
     * @enum range_type
     * @brief 上下文类型
     */
    enum range_type {
        mapping = 0, ///< 映射
        sequence = 1 ///< 序列
    };

    /**
     * @struct frame
     * @brief 构建上下文
     */
    struct frame {
        range_type type = mapping;  ///< 上下文类型
        yaml_value* node = nullptr; ///< 容器节点
        string pending_key;         ///< 待写入键
    };

    /**
     * @struct anchor_entry
     * @brief 锚点表项
     */
    struct anchor_entry {
        string name;      ///< 锚点名
        yaml_value value; ///< 锚点值
    };

    yaml_document document_;          ///< 当前文档
    vector<frame> contexts_;          ///< 上下文栈
    vector<yaml_document> documents_; ///< 已完成文档
    string pending_anchor_;           ///< 待应用锚点
    string pending_tag_;              ///< 待应用标签
    vector<anchor_entry> anchors_;    ///< 锚点表

private:
    yaml_builder& push_value(yaml_value value);
    void begin_container(range_type type, bool flow);
    void end_container(range_type type);
    void apply_pending_metadata(yaml_value& value);
    NEFORCE_NODISCARD yaml_value* new_root_mapping();

    template <typename Iterable>
    enable_if_t<is_maplike_v<Iterable>, yaml_builder&> value_iterable_impl(const Iterable& iterable) {
        begin_flow_mapping();
        for (const auto& pair: iterable) {
            this->key(pair.first).value(pair.second);
        }
        end_mapping();
        return *this;
    }

    template <typename Iterable>
    enable_if_t<!is_maplike_v<Iterable>, yaml_builder&> value_iterable_impl(const Iterable& iterable) {
        begin_sequence();
        for (const auto& element: iterable) {
            this->value(element);
        }
        end_sequence();
        return *this;
    }

public:
    /**
     * @brief 构造构建器
     */
    yaml_builder();

    yaml_builder(const yaml_builder&) = delete;
    yaml_builder& operator=(const yaml_builder&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 源构建器
     */
    yaml_builder(yaml_builder&& other) noexcept = default;

    /**
     * @brief 移动赋值运算符
     * @param other 源构建器
     * @return 自身引用
     */
    yaml_builder& operator=(yaml_builder&& other) noexcept = default;

    /**
     * @brief 设置当前键名
     * @param key 键名
     * @return 自身引用
     * @throws yaml_exception 不在映射上下文或键名重复时抛出
     */
    yaml_builder& key(string key);

    /**
     * @brief 开始块样式映射
     * @return 自身引用
     */
    yaml_builder& begin_mapping();

    /**
     * @brief 开始块样式映射
     * @return 自身引用
     */
    yaml_builder& begin_block_mapping();

    /**
     * @brief 开始流样式映射
     * @return 自身引用
     */
    yaml_builder& begin_flow_mapping();

    /**
     * @brief 结束映射
     * @return 自身引用
     */
    yaml_builder& end_mapping();

    /**
     * @brief 开始块样式序列
     * @return 自身引用
     */
    yaml_builder& begin_sequence();

    /**
     * @brief 开始块样式序列
     * @return 自身引用
     */
    yaml_builder& begin_block_sequence();

    /**
     * @brief 开始流样式序列
     * @return 自身引用
     */
    yaml_builder& begin_flow_sequence();

    /**
     * @brief 结束序列
     * @return 自身引用
     */
    yaml_builder& end_sequence();

    /**
     * @brief 设置空值
     * @param np 空指针标记
     * @return 自身引用
     */
    yaml_builder& value(nullptr_t np) {
        (void) np;
        return push_value(document_.make_null_value());
    }

    /**
     * @brief 设置布尔值
     * @param v 布尔值
     * @return 自身引用
     */
    yaml_builder& value(const bool v) { return push_value(document_.make_boolean_value(v)); }

    /**
     * @brief 设置64位整数
     * @param v 整数
     * @return 自身引用
     */
    yaml_builder& value(const int64_t v) { return push_value(document_.make_integer_value(v)); }

    /**
     * @brief 设置整数
     * @param v 整数
     * @return 自身引用
     */
    yaml_builder& value(const int v) { return push_value(document_.make_integer_value(static_cast<int64_t>(v))); }

    /**
     * @brief 设置浮点数
     * @param v 浮点数
     * @return 自身引用
     */
    yaml_builder& value(const double v) { return push_value(document_.make_float_value(v)); }

    /**
     * @brief 设置纯文本字符串
     * @param v 字符串
     * @return 自身引用
     */
    yaml_builder& value(string v) {
        return push_value(document_.make_string_value(string_view(v.data(), v.size()), yaml_string::Plain, true));
    }

    /**
     * @brief 设置C字符串
     * @param v 字符串
     * @return 自身引用
     */
    yaml_builder& value(const char* v) {
        return push_value(document_.make_string_value(string_view(v), yaml_string::Plain, true));
    }

    /**
     * @brief 设置字符串视图
     * @param v 字符串视图
     * @return 自身引用
     */
    yaml_builder& value(const string_view v) {
        return push_value(document_.make_string_value(v, yaml_string::Plain, true));
    }

    /**
     * @brief 设置已有值
     * @param v YAML值
     * @return 自身引用
     */
    yaml_builder& value(yaml_value v) { return push_value(v); }

    /**
     * @brief 设置时间戳
     * @param dt 日期时间
     * @return 自身引用
     */
    yaml_builder& value_datetime(const datetime& dt) { return push_value(document_.make_timestamp_value(dt)); }

    /**
     * @brief 设置指定样式的字符串
     * @param v 字符串
     * @param style 字符串样式
     * @return 自身引用
     */
    yaml_builder& value_string(string v, yaml_string::string_style style) {
        return push_value(document_.make_string_value(string_view(v.data(), v.size()), style, true));
    }

    /**
     * @brief 设置可迭代对象
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用
     */
    template <typename Iterable, enable_if_t<is_iterable_v<Iterable>, int> = 0>
    yaml_builder& value_iterable(const Iterable& iterable) {
        return value_iterable_impl(iterable);
    }

    /**
     * @brief 使用函数式接口构建映射
     * @param build_func 构建函数
     * @return 自身引用
     */
    yaml_builder& value_mapping(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式接口构建块样式映射
     * @param build_func 构建函数
     * @return 自身引用
     */
    yaml_builder& value_block_mapping(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式接口构建流样式映射
     * @param build_func 构建函数
     * @return 自身引用
     */
    yaml_builder& value_flow_mapping(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式接口构建序列
     * @param build_func 构建函数
     * @return 自身引用
     */
    yaml_builder& value_sequence(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式接口构建块样式序列
     * @param build_func 构建函数
     * @return 自身引用
     */
    yaml_builder& value_block_sequence(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 使用函数式接口构建流样式序列
     * @param build_func 构建函数
     * @return 自身引用
     */
    yaml_builder& value_flow_sequence(const function<void(yaml_builder&)>& build_func);

    /**
     * @brief 为下一个值设置锚点
     * @param name 锚点名
     * @return 自身引用
     */
    yaml_builder& anchor(string name);

    /**
     * @brief 为下一个值设置标签
     * @param t 标签
     * @return 自身引用
     */
    yaml_builder& tag(string t);

    /**
     * @brief 插入锚点引用
     * @param name 锚点名
     * @return 自身引用
     * @throws yaml_exception 锚点未定义时抛出
     */
    yaml_builder& alias(const string& name);

    /**
     * @brief 开始新文档
     * @return 自身引用
     * @throws yaml_exception 当前存在未闭合上下文时抛出
     */
    yaml_builder& begin_document();

    /**
     * @brief 构建文档
     * @return 构建完成的文档
     * @throws yaml_exception 存在未闭合上下文时抛出
     */
    NEFORCE_NODISCARD yaml_document build();

    /**
     * @brief 构建全部文档
     * @return 文档列表
     * @throws yaml_exception 存在未闭合上下文时抛出
     */
    NEFORCE_NODISCARD vector<yaml_document> build_documents();
};

/** @} */ // YamlConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_YAML_YAML_BUILDER_HPP__
