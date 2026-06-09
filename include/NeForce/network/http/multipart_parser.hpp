#ifndef NEFORCE_NETWORK_HTTP_MULTIPART_PARSER_HPP__
#define NEFORCE_NETWORK_HTTP_MULTIPART_PARSER_HPP__

/**
 * @file multipart_parser.hpp
 * @brief multipart/form-data 请求体解析器
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/utility/byte_size.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct multipart_field
 * @brief multipart/form-data 单个字段
 */
struct multipart_field {
    string name;                           ///< 表单字段名
    string filename;                       ///< 上传文件名
    string content_type;                   ///< 字段的Content-Type
    byte_vector data;                      ///< 字段数据
    unordered_map<string, string> headers; ///< 字段头部

    NEFORCE_NODISCARD bool is_file() const noexcept { return !filename.empty(); }
};

/**
 * @class multipart_parser
 * @brief multipart/form-data 解析器
 *
 * 解析 HTTP multipart/form-data 请求体。
 * 支持文件上传字段和普通表单字段。
 */
class NEFORCE_API multipart_parser {
public:
    /// 最大单字段大小
    byte_size max_field_size{10_MB};
    /// 最大字段数
    size_t max_fields{256};
    /// 最大总请求体大小
    byte_size max_total_size{100_MB};

    multipart_parser() = default;

    /**
     * @brief 解析multipart请求体
     * @param body 原始请求体
     * @param boundary Content-Type中的boundary值（不含"--"前缀）
     * @return 解析出的字段列表，解析失败返回空vector
     */
    vector<multipart_field> parse(string_view body, string_view boundary);

    /**
     * @brief 从Content-Type头提取boundary
     * @param content_type Content-Type头值
     * @return boundary值，不存在返回空string_view
     */
    NEFORCE_NODISCARD static string_view extract_boundary(string_view content_type);
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_MULTIPART_PARSER_HPP__
