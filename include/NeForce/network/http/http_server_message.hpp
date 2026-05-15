#ifndef NEFORCE_NETWORK_HTTP_HTTP_SERVER_MESSAGE_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_SERVER_MESSAGE_HPP__

/**
 * @file http_server_message.hpp
 * @brief HTTP服务器消息结构
 *
 * 此文件提供了HTTP服务器端请求和响应的数据结构定义。
 * 支持请求解析、参数提取、Cookie管理、会话关联等功能，
 * 以及响应构建、重定向、Cookie设置等功能。
 *
 * 主要功能：
 * - HTTP请求解析（方法、路径、头部、正文）
 * - 请求参数提取（查询参数、表单数据、Cookie）
 * - 会话关联
 * - HTTP响应构建
 * - 重定向支持
 * - Cookie管理
 * - 内容类型设置
 */

#include "NeForce/network/http/http_session.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct http_server_request
 * @brief HTTP服务器请求结构
 *
 * 表示一个HTTP请求，包含请求行、头部、正文以及解析后的参数。
 * 支持从字符串解析请求，并提供便捷的访问方法。
 *
 * 使用示例：
 * @code
 * // 解析请求
 * auto request = http_server_request::parse(
 *     "GET /api/users?id=123 HTTP/1.1\r\n"
 *     "Host: example.com\r\n"
 *     "Cookie: session=abc123\r\n"
 *     "\r\n"
 * );
 *
 * // 获取请求信息
 * if (request.method == http_method::GET()) {
 *     string id = request.parameter("id");
 * }
 *
 * // 获取Cookie
 * string session_id = request.cookie("session");
 *
 * // 获取客户端IP（支持代理）
 * string client_ip = request.client_ip();
 *
 * // 检查是否为AJAX请求
 * if (request.is_ajax()) {
 *     // 返回JSON响应
 * }
 *
 * // 获取会话
 * if (request.has_session()) {
 *     auto* session = request.session;
 *     session->touch();
 * }
 * @endcode
 */
struct NEFORCE_API http_server_request : iobject<http_server_request> {
    http_method method{http_method::GET()}; ///< HTTP方法
    string path{"/"};                       ///< 请求路径
    string version{"HTTP/1.1"};             ///< HTTP版本
    string query;                           ///< 查询字符串
    string body;                            ///< 请求正文

    unordered_map<string, string> headers;    ///< 请求头
    unordered_map<string, string> cookies;    ///< Cookie
    unordered_map<string, string> parameters; ///< 请求参数
    unordered_map<string, string> form_data;  ///< 表单数据

    http_session* session = nullptr; ///< 会话对象

    /**
     * @brief 获取参数值
     * @param name 参数名
     * @return 参数值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view parameter(const string& name) const noexcept {
        const auto it = parameters.find(name);
        return it != parameters.end() ? it->second.view() : "";
    }

    /**
     * @brief 设置参数
     * @param name 参数名
     * @param value 参数值
     */
    void set_parameter(const string& name, string value) { parameters[name] = move(value); }

    /**
     * @brief 检查参数是否存在
     * @param name 参数名
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool has_parameter(const string& name) const noexcept {
        return parameters.find(name) != parameters.end();
    }

    /**
     * @brief 获取Cookie值
     * @param name Cookie名
     * @return Cookie值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view cookie(const string& name) const noexcept {
        const auto it = cookies.find(name);
        return it != cookies.end() ? it->second.view() : "";
    }

    /**
     * @brief 设置Cookie值
     * @param name Cookie名
     * @param value Cookie值
     */
    void set_cookie(const string& name, string value) { cookies[name] = move(value); }

    /**
     * @brief 检查Cookie是否存在
     * @param name Cookie名
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool has_cookie(const string& name) const noexcept { return cookies.find(name) != cookies.end(); }

    /**
     * @brief 获取请求头值
     * @param name 头名称
     * @return 头值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view header(const string& name) const noexcept {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }

    /**
     * @brief 设置请求头
     * @param name 头名称
     * @param value 头值
     */
    void set_header(const string& name, string value) { headers[name] = move(value); }

    /**
     * @brief 检查请求头是否存在
     * @param name 头名称
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool has_header(const string& name) const noexcept { return headers.find(name) != headers.end(); }

    /**
     * @brief 检查是否有关联的有效会话
     * @return 存在有效会话返回true
     */
    NEFORCE_NODISCARD bool has_session() const noexcept { return session != nullptr && session->is_valid(); }

    /**
     * @brief 获取Content-Type
     * @return Content-Type值
     */
    NEFORCE_NODISCARD string_view content_type() const noexcept { return header(http_key::Content_Type()); }

    /**
     * @brief 检查是否为Keep-Alive连接
     * @return 是Keep-Alive返回true
     */
    NEFORCE_NODISCARD bool is_keep_alive() const noexcept {
        const auto conn = header(http_key::Connection());
        return conn == "keep-alive" || conn == "Keep-Alive";
    }

    /**
     * @brief 获取客户端真实IP
     * @return IP地址
     *
     * 优先从X-Forwarded-For头获取，其次从X-Real-IP头获取。
     * 适用于经过代理的请求。
     */
    NEFORCE_NODISCARD string_view client_ip() const;

    /**
     * @brief 获取User-Agent
     * @return User-Agent值
     */
    NEFORCE_NODISCARD string_view user_agent() const noexcept { return header("User-Agent"); }

    /**
     * @brief 获取Referer
     * @return Referer值
     */
    NEFORCE_NODISCARD string_view referer() const noexcept { return header("Referer"); }

    /**
     * @brief 检查是否为AJAX请求
     * @return 是AJAX请求返回true
     *
     * 检查X-Requested-With头是否为XMLHttpRequest。
     */
    NEFORCE_NODISCARD bool is_ajax() const noexcept { return header("X-Requested-With") == "XMLHttpRequest"; }

    /**
     * @brief 清空请求所有数据
     */
    void clear();

    /**
     * @brief 从字符串解析HTTP请求
     * @param str HTTP请求字符串
     * @return 解析后的请求对象
     * @throws http_exception 请求格式无效时抛出
     *
     * 解析请求行、头部和正文。
     * 自动解析Cookie头。
     */
    NEFORCE_NODISCARD static http_server_request parse(string_view str);

    /**
     * @brief 序列化为HTTP请求字符串
     * @return HTTP请求字符串
     */
    NEFORCE_NODISCARD string to_string() const;
};


/**
 * @struct http_server_response
 * @brief HTTP服务器响应结构
 *
 * 表示一个HTTP响应，包含状态行、头部和正文。
 * 支持重定向、Cookie设置、内容类型设置等。
 *
 * 使用示例：
 * @code
 * http_server_response response;
 *
 * // 设置状态
 * response.status = http_status::S2_OK;
 * response.status_message = "OK";
 *
 * // 设置内容
 * response.body = "<html><body>Hello World</body></html>";
 * response.set_content_type(http_content::HTML());
 *
 * // 设置Cookie
 * http_cookie cookie;
 * cookie.name = "session";
 * cookie.value = "abc123";
 * cookie.path = "/";
 * cookie.http_only = true;
 * response.cookies.push_back(cookie);
 *
 * // 重定向
 * // response.redirect_url = "https://example.com/new-page";
 *
 * // 序列化响应
 * string response_str = response.to_string();
 * @endcode
 */
struct NEFORCE_API http_server_response : istringify<http_server_response> {
    string version{"HTTP/1.1"};                    ///< HTTP版本
    http_status status{http_status::S4_NOT_FOUNT}; ///< HTTP状态码
    string status_message;                         ///< 状态消息
    unordered_map<string, string> headers;         ///< 响应头
    vector<http_cookie> cookies;                   ///< 设置的Cookie
    string body;                                   ///< 响应正文
    string redirect_url;                           ///< 重定向URL
    string forward_path;                           ///< 转发路径

    /**
     * @brief 默认构造函数
     *
     * 设置默认Content-Type为text/plain，Connection为close。
     */
    http_server_response() {
        headers[http_key::Content_Type()] = http_content::PLAIN_TEXT().to_string() + "; charset=utf-8";
        headers[http_key::Connection()] = "close";
    }

    /**
     * @brief 获取响应头值
     * @param name 头名称
     * @return 头值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view header(const string& name) const noexcept {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }

    /**
     * @brief 设置响应头
     * @param name 头名称
     * @param value 头值
     */
    void set_header(const string& name, string value) { headers[name] = move(value); }

    /**
     * @brief 检查响应头是否存在
     * @param name 头名称
     * @return 存在返回true
     */
    NEFORCE_NODISCARD bool has_header(const string& name) const noexcept { return headers.find(name) != headers.end(); }

    /**
     * @brief 设置Content-Type
     * @param value HTTP内容类型对象
     */
    void set_content_type(http_content value) { headers[http_key::Content_Type()] = move(value).content(); }

    /**
     * @brief 设置Content-Type
     * @param value 内容类型字符串
     */
    void set_content_type(string value) { headers[http_key::Content_Type()] = move(value); }

    /**
     * @brief 序列化为HTTP响应字符串
     * @return HTTP响应字符串
     *
     * 支持正常响应和重定向响应。
     * 自动添加Content-Length头。
     */
    NEFORCE_NODISCARD string to_string() const;
};

/**
 * @typedef http_request
 * @brief HTTP请求类型别名
 */
using http_request = http_server_request;

/**
 * @typedef http_response
 * @brief HTTP响应类型别名
 */
using http_response = http_server_response;

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_SERVER_MESSAGE_HPP__
