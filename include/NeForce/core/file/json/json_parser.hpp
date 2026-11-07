#ifndef NEFORCE_CORE_FILE_JSON_JSON_PARSER_HPP__
#define NEFORCE_CORE_FILE_JSON_JSON_PARSER_HPP__

/**
 * @file json_parser.hpp
 * @brief JSON 解析器
 */

#include "NeForce/core/file/json/json_value.hpp"
#include "NeForce/core/utility/optional.hpp"
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
 * @class json_parser
 * @brief JSON 解析器
 */
class NEFORCE_API json_parser {
public:
    /**
     * @brief 构造零拷贝解析器
     * @param text 输入文本
     * @note text 生命周期需覆盖文档使用期
     */
    explicit json_parser(const string_view text) noexcept :
    text_(text) {}

    /**
     * @brief 构造自包含解析器
     * @param text 输入文本
     * @note 解析器持有 text 副本
     */
    explicit json_parser(string text) noexcept :
    owned_(move(text)),
    owning_(true) {
        text_ = string_view(owned_.data(), owned_.size());
    }

    ~json_parser() = default;

    json_parser(const json_parser&) = delete;
    json_parser& operator=(const json_parser&) = delete;

    /**
     * @brief 解析整份文本
     * @return 解析出的文档
     * @throws json_exception 语法错误或存在尾随内容时抛出
     */
    json_document parse();

    /**
     * @brief 尝试解析整份文本
     * @return 成功时返回文档，失败时返回空
     */
    optional<json_document> try_parse() noexcept;

private:
    static constexpr size_t hint_depth = 24;
    static constexpr uint8_t hint_confirm_times = 2;

    json_value parse_value(json_document& doc, const char*& cursor, const char* end, size_t depth);
    json_value parse_object(json_document& doc, const char*& cursor, const char* end, size_t depth);
    json_value parse_array(json_document& doc, const char*& cursor, const char* end, size_t depth);
    json_value parse_string(json_document& doc, const char*& cursor, const char* end);
    json_value parse_number(json_document& doc, const char*& cursor, const char* end);
    json_value parse_keyword(json_document& doc, const char*& cursor, const char* end);

    NEFORCE_NODISCARD size_t reserve_hint(const uint16_t* counts, const uint8_t* confirms,
                                          size_t depth) const noexcept {
        if (depth >= hint_depth || confirms[depth] < hint_confirm_times) {
            return 0;
        }
        return counts[depth];
    }

    void update_reserve_hint(uint16_t* counts, uint8_t* confirms, size_t depth, size_t count) noexcept {
        if (depth >= hint_depth) {
            return;
        }
        if (counts[depth] == count) {
            if (confirms[depth] < hint_confirm_times) {
                ++confirms[depth];
            }
            return;
        }
        counts[depth] = count <= 65535 ? static_cast<uint16_t>(count) : 0;
        confirms[depth] = 0;
    }

    void decode_string(const char* begin, const char* finish, string& out);

    NEFORCE_NODISCARD size_t offset_of(const char* cursor) const noexcept {
        return static_cast<size_t>(cursor - text_.data());
    }

    string decode_scratch_;                    ///< 转义解码缓冲区
    uint16_t object_sizes_[hint_depth] = {};   ///< 各深度最近一次对象的成员个数
    uint16_t array_sizes_[hint_depth] = {};    ///< 各深度最近一次数组的元素个数
    uint8_t object_confirms_[hint_depth] = {}; ///< 对象尺寸的连续命中次数
    uint8_t array_confirms_[hint_depth] = {};  ///< 数组尺寸的连续命中次数
    string_view text_;                         ///< 输入视图
    string owned_;                             ///< 自包含模式下的输入副本
    size_t position_ = 0;                      ///< 出错时记录的扫描位置
    bool owning_ = false;                      ///< 是否为自包含模式
};

/** @} */ // JsonConfig

/** @} */ // ConfigFormat

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_JSON_JSON_PARSER_HPP__
