#ifndef NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__

/**
 * @file http_client_message.hpp
 * @brief HTTP客户端消息结构
 *
 * 此文件提供了HTTP客户端请求和响应的数据结构定义。
 * 支持请求构建、响应解析、定时统计等功能。
 *
 * 主要功能：
 * - HTTP请求构建
 * - HTTP响应解析
 * - 分块传输支持
 * - Cookie解析
 * - 请求/响应时间统计
 * - 响应状态分类
 */

#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/http/http_session.hpp"
#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct http_client_response
 * @brief HTTP客户端响应结构
 *
 * 表示一个HTTP响应，包含状态码、头部、正文以及性能统计信息。
 */
struct http_client_response {
public:
    uint16_t http_version_major = 1;               ///< HTTP主版本号
    uint16_t http_version_minor = 1;               ///< HTTP次版本号
    bool chunked = false;                          ///< 是否使用分块传输编码
    uint64_t content_length = 0;                   ///< Content-Length值
    string effective_url;                          ///< 最终请求的URL
    int redirect_count = 0;                        ///< 重定向次数
    milliseconds total_time{0};                    ///< 总耗时
    milliseconds connect_time{0};                  ///< 连接建立耗时
    milliseconds send_time{0};                     ///< 发送请求耗时
    milliseconds receive_time{0};                  ///< 接收响应耗时
    http_status status = http_status::S2_OK;       ///< HTTP状态码
    string status_message;                         ///< 状态消息
    unordered_map<string, vector<string>> headers; ///< 响应头
    string body;                                   ///< 响应正文
    vector<http_cookie> cookies;                   ///< 解析后的Cookie

    /**
     * @brief 获取第一个响应头值
     * @param key 头名称
     * @return 头值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view header(const string& key) const {
        const auto it = headers.find(key);
        if (it == headers.end() || it->second.empty()) {
            return "";
        }
        return it->second[0].view();
    }

    /**
     * @brief 获取所有同名响应头值
     * @param key 头名称
     * @return 头值列表，不存在返回空列表
     */
    NEFORCE_NODISCARD const vector<string>& headers_all(const string& key) const {
        static const vector<string> empty;
        const auto it = headers.find(key);
        return it != headers.end() ? it->second : empty;
    }

    /**
     * @brief 检查响应头是否存在
     * @param key 头名称
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool has_header(const string& key) const { return headers.find(key) != headers.end(); }

    /**
     * @brief 检查是否为成功响应（2xx）
     * @return 成功返回true
     */
    NEFORCE_NODISCARD bool is_success() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 200 && code < 300;
    }

    /**
     * @brief 检查是否为重定向响应（3xx）
     * @return 重定向返回true
     */
    NEFORCE_NODISCARD bool is_redirect() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 300 && code < 400;
    }

    /**
     * @brief 检查是否为客户端错误响应（4xx）
     * @return 客户端错误返回true
     */
    NEFORCE_NODISCARD bool is_client_error() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 400 && code < 500;
    }

    /**
     * @brief 检查是否为服务器错误响应（5xx）
     * @return 服务器错误返回true
     */
    NEFORCE_NODISCARD bool is_server_error() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 500 && code < 600;
    }

    /**
     * @brief 获取Content-Type
     * @return Content-Type值
     */
    NEFORCE_NODISCARD string_view content_type() const { return header("Content-Type"); }
};


/**
 * @struct http_client_request
 * @brief HTTP客户端请求结构
 *
 * 表示一个HTTP请求，包含方法、URL、头部、查询参数和正文。
 */
struct NEFORCE_API http_client_request {
    http_method method{http_method::GET()};     ///< HTTP方法
    string host;                                ///< 主机名
    ports port;                                 ///< 端口号
    string scheme{"http"};                      ///< 协议
    string path = "/";                          ///< 请求路径
    string version = "HTTP/1.1";                ///< HTTP版本
    unordered_map<string, string> headers;      ///< 请求头
    unordered_map<string, string> query_params; ///< 查询参数
    string body;                                ///< 请求正文

    /**
     * @brief 获取请求头值
     * @param key 头名称
     * @return 头值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view header(const string& key) const {
        const auto it = headers.find(key);
        if (it == headers.end() || it->second.empty()) {
            return "";
        }
        return it->second.view();
    }

    /**
     * @brief 设置请求头
     * @param key 头名称
     * @param value 头值
     */
    void set_header(const string& key, string value) { headers[key] = _NEFORCE move(value); }

    /**
     * @brief 添加查询参数
     * @param key 参数名
     * @param value 参数值
     */
    void add_query_param(const string& key, string value) { query_params[key] = _NEFORCE move(value); }

    /**
     * @brief 构建完整请求路径
     * @return 包含查询参数的完整路径
     *
     * 将path和query_params组合成完整路径。
     * 查询参数会自动进行URL编码。
     */
    NEFORCE_NODISCARD string build_full_path() const;
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CLIENT_MESSAGE_HPP__
