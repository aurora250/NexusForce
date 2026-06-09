#ifndef NEFORCE_NETWORK_HTTP_HTTP_RANGE_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_RANGE_HPP__

/**
 * @file http_range.hpp
 * @brief HTTP 范围请求（Range Requests）工具函数
 */

#include "NeForce/core/functional/function.hpp"
#include "NeForce/network/http/http_server_message.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @brief 字节范围结构体
 */
struct byte_range {
    uint64_t start; ///< 起始偏移
    uint64_t end;   ///< 结束偏移（含）
};

/**
 * @brief 解析HTTP Range头部值
 * @param range_header Range头部值（不含"bytes="前缀）
 * @param file_size 文件总大小
 * @param max_ranges 最大允许的范围数量，超出时拒绝整个请求
 * @return 解析出的字节范围列表，无效返回空vector
 */
vector<byte_range> NEFORCE_API parse_ranges(string_view range_header, uint64_t file_size, size_t max_ranges = 100);

/**
 * @brief 构建Content-Range头部值
 * @param range 字节范围
 * @param total_size 总大小
 * @return Content-Range值，如 "bytes 0-1023/4096"
 */
string NEFORCE_API build_content_range(const byte_range& range, uint64_t total_size);

/**
 * @brief 构建multipart/byteranges响应体
 * @param ranges 多个字节范围
 * @param content_type 文件的MIME类型
 * @param boundary MIME分隔边界字符串
 * @param get_range_body 获取指定范围内容的回调函数
 * @param total_size
 * @return multipart响应体字符串
 */
string NEFORCE_API build_multipart_ranges(const vector<byte_range>& ranges, string_view content_type,
                                          string_view boundary, function<string(const byte_range&)> get_range_body,
                                          uint64_t total_size = 0);

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_RANGE_HPP__
