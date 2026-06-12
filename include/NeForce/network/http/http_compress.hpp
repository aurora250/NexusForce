#ifndef NEFORCE_NETWORK_HTTP_HTTP_COMPRESS_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_COMPRESS_HPP__

/**
 * @file http_compress.hpp
 * @brief HTTP 响应压缩过滤器（gzip / deflate）
 */

#ifdef NEFORCE_SUPPORT_ZLIB
#    include "NeForce/compress/zlib_compress.hpp"
#    include "NeForce/core/utility/byte_size.hpp"
#    include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class compress_filter
 * @brief HTTP响应压缩过滤器
 *
 * 根据请求的Accept-Encoding头部，自动压缩响应体。
 * 支持gzip和deflate编码。
 */
class NEFORCE_API compress_filter final : public http_filter {
public:
    bool enabled = true;      ///< 是否启用压缩
    byte_size min_size{1_KB}; ///< 最小压缩阈值

    compress_filter() = default;

    void do_filter(http_request& request, http_response& response) override {}
    void post_filter(http_request& request, http_response& response) override;
    NEFORCE_NODISCARD string name() const override { return "compress_filter"; }

    // TODO: Brotli compression support — add 'br' to Accept-Encoding negotiation, Brotli offers ~20% better compression than gzip
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_NETWORK_HTTP_HTTP_COMPRESS_HPP__
