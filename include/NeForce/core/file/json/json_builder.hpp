#ifndef NEFORCE_CORE_FILE_JSON_JSON_BUILDER_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_BUILDER_HPP__

/**
 * @file json_builder.hpp
 * @brief JSON 文档构建器
 */

#include "NeForce/core/file/json/json_value.hpp"
#include "NeForce/core/functional/function.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @addtogroup ConfigFormat 配置格式操作
 * @{
 */

/**
 * @addtogroup JsonConfig JSON配置
 * @{
 */

/**
 * @class json_builder
 * @brief 流式构建 JSON 文档
 */
class NEFORCE_API json_builder {
public:
    json_builder() = default;
    ~json_builder() = default;

    json_builder(const json_builder&) = delete;
    json_builder& operator=(const json_builder&) = delete;
    json_builder(json_builder&&) noexcept = default;
    json_builder& operator=(json_builder&&) noexcept = default;

    /**
     * @brief 开始一个对象
     * @return 自身引用
     * @throws json_exception 层级不匹配时抛出
     */
    json_builder& begin_object();

    /**
     * @brief 开始一个数组
     * @return 自身引用
     * @throws json_exception 层级不匹配时抛出
     */
    json_builder& begin_array();

    /**
     * @brief 结束当前对象
     * @return 自身引用
     * @throws json_exception 层级不匹配时抛出
     */
    json_builder& end_object();

    /**
     * @brief 结束当前数组
     * @return 自身引用
     * @throws json_exception 层级不匹配时抛出
     */
    json_builder& end_array();

    /**
     * @brief 设置下一个成员的键
     * @param key 键
     * @return 自身引用
     * @throws json_exception 当前不在对象内时抛出
     */
    json_builder& key(const string& key);

    /**
     * @brief 设置下一个成员的键
     * @param key 键
     * @return 自身引用
     */
    json_builder& key(const char* key) { return this->key(string(key)); }

    /**
     * @brief 设置下一个成员的键
     * @param key 键视图
     * @return 自身引用
     */
    json_builder& key(const string_view key) { return this->key(string(key)); }

    /**
     * @brief 写入空值
     * @return 自身引用
     */
    json_builder& value(nullptr_t) { return this->value(json_value::make_null()); }

    /**
     * @brief 写入字符串
     * @param text 字符串
     * @return 自身引用
     */
    json_builder& value(const string& text) { return this->value_string(string_view(text.data(), text.size())); }

    /**
     * @brief 写入字符串
     * @param text 字符串
     * @return 自身引用
     */
    json_builder& value(const char* text) { return this->value_string(string_view(text)); }

    /**
     * @brief 写入字符串
     * @param text 字符串视图
     * @return 自身引用
     */
    json_builder& value(const string_view text) { return this->value_string(text); }

    /**
     * @brief 写入浮点数
     * @param number 浮点值
     * @return 自身引用
     */
    json_builder& value(const float64_t number) { return this->value(json_value::make_real(number)); }

    /**
     * @brief 写入浮点数
     * @param number 浮点值
     * @return 自身引用
     */
    json_builder& value(const float32_t number) { return this->value(static_cast<float64_t>(number)); }

    /**
     * @brief 写入有符号整数
     * @param number 整数值
     * @return 自身引用
     */
    json_builder& value(const int64_t number) { return this->value(json_value::make_integer(number)); }

    /**
     * @brief 写入有符号整数
     * @param number 整数值
     * @return 自身引用
     */
    json_builder& value(const int32_t number) { return this->value(static_cast<int64_t>(number)); }

    /**
     * @brief 写入无符号整数
     * @param number 无符号整数值
     * @return 自身引用
     */
    json_builder& value(const uint64_t number) { return this->value(json_value::make_uinteger(number)); }

    /**
     * @brief 写入布尔值
     * @param state 布尔值
     * @return 自身引用
     */
    json_builder& value(const bool state) { return this->value(json_value::make_bool(state)); }

    /**
     * @brief 写入已构造的值
     * @param value 值
     * @return 自身引用
     */
    json_builder& value(const json_value& value);

    /**
     * @brief 写入映射表，转换为 JSON 对象
     * @tparam Map 映射表类型
     * @param table 映射表
     * @return 自身引用
     */
    template <typename Map, enable_if_t<is_iterable_v<Map> && is_maplike_v<Map>, int> = 0>
    json_builder& value(const Map& table) {
        this->begin_object();
        for (const auto& pair: table) {
            this->key(pair.first).value(pair.second);
        }
        this->end_object();
        return *this;
    }

    /**
     * @brief 写入可迭代对象，转换为 JSON 数组
     * @tparam Iterable 可迭代类型
     * @param iterable 可迭代对象
     * @return 自身引用
     */
    template <typename Iterable, enable_if_t<is_iterable_v<Iterable> && !is_maplike_v<Iterable>, int> = 0>
    json_builder& value(const Iterable& iterable) {
        this->begin_array();
        for (const auto& element: iterable) {
            this->value(element);
        }
        this->end_array();
        return *this;
    }

    /**
     * @brief 用回调构建一个对象
     * @param build_func 构建函数
     * @return 自身引用
     */
    json_builder& value_object(const function<void(json_builder&)>& build_func);

    /**
     * @brief 用回调构建一个数组
     * @param build_func 构建函数
     * @return 自身引用
     */
    json_builder& value_array(const function<void(json_builder&)>& build_func);

    /**
     * @brief 完成构建
     * @return 构建出的文档
     * @throws json_exception 容器未闭合时抛出
     */
    json_document build();

private:
    struct frame {
        bool is_object = false;     ///< 是否为对象
        json_value* node = nullptr; ///< 容器节点
        string pending_key;         ///< 待写入成员的键
    };

    json_builder& value_string(string_view text);
    void push_value(json_value value);

    vector<frame> frames_;   ///< 容器栈
    json_document document_; ///< 承载 arena 与根值
    bool has_root_ = false;  ///< 根值是否已就绪
};

/** @} */ // JsonConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_BUILDER_HPP__
