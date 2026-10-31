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

#include "NeForce/core/container/buffer_chain.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/utility/any.hpp"
#include "NeForce/network/http/http_session.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @typedef http_context
 * @brief 请求上下文，跨过滤器/中间件传递任意数据
 */
using http_context = flat_unordered_map<string, any>;

NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class http_session_handle
 * @brief 惰性会话句柄
 */
class NEFORCE_API http_session_handle {
public:
    /// @brief 会话创建解析器
    using resolver_t = function<http_session*()>;

    http_session_handle() = default;
    explicit http_session_handle(http_session* session) noexcept :
    session_(session) {}

    http_session_handle& operator=(http_session* session) noexcept {
        session_ = session;
        created_ = false;
        return *this;
    }

    http_session_handle& operator=(nullptr_t) noexcept {
        reset();
        return *this;
    }

    /**
     * @brief 获取会话，必要时按需创建
     * @return 会话指针
     */
    NEFORCE_NODISCARD http_session* get() {
        if (session_ == nullptr && resolver_) {
            session_ = resolver_();
            created_ = session_ != nullptr;
        }
        return session_;
    }

    /**
     * @brief 获取已创建的会话，不触发按需创建
     * @return 已创建的会话指针，未创建返回nullptr
     */
    NEFORCE_NODISCARD http_session* materialized() const noexcept { return session_; }

    /**
     * @brief 检查会话是否由本请求按需创建
     * @return 本请求创建了会话返回true
     */
    NEFORCE_NODISCARD bool created() const noexcept { return created_; }

    /**
     * @brief 标记会话由本请求创建
     */
    void mark_created() noexcept { created_ = true; }

    /**
     * @brief 设置按需创建解析器
     * @param resolver 会话解析器
     */
    void set_resolver(resolver_t resolver) { resolver_ = _NEFORCE move(resolver); }

    /**
     * @brief 清空句柄与解析器
     */
    void reset() noexcept {
        session_ = nullptr;
        created_ = false;
        resolver_ = resolver_t{};
    }

    NEFORCE_NODISCARD operator http_session*() { return get(); }
    NEFORCE_NODISCARD operator const http_session*() const noexcept { return session_; }
    NEFORCE_NODISCARD http_session* operator->() { return get(); }
    NEFORCE_NODISCARD const http_session* operator->() const noexcept { return session_; }
    NEFORCE_NODISCARD http_session& operator*() { return *get(); }
    NEFORCE_NODISCARD const http_session& operator*() const noexcept { return *session_; }

    NEFORCE_NODISCARD bool operator==(nullptr_t) const noexcept { return session_ == nullptr; }
    NEFORCE_NODISCARD bool operator!=(nullptr_t) const noexcept { return session_ != nullptr; }
    NEFORCE_NODISCARD explicit operator bool() const noexcept { return session_ != nullptr; }

private:
    http_session* session_{nullptr}; ///< 已创建的会话
    bool created_{false};            ///< 会话是否由本请求创建
    resolver_t resolver_;            ///< 按需创建解析器
};

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
 *     http_session* session = request.session;
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

    flat_unordered_map<string, string> headers;    ///< 请求头
    flat_unordered_map<string, string> cookies;    ///< Cookie
    flat_unordered_map<string, string> parameters; ///< 请求参数
    flat_unordered_map<string, string> form_data;  ///< 表单数据
    flat_unordered_map<string, string> trailers;   ///< Trailer头部

    http_session_handle session; ///< 会话句柄
    http_context context;        ///< 请求上下文

    /**
     * @brief 获取参数值
     * @param name 参数名
     * @return 参数值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view parameter(const string& name) const {
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
    NEFORCE_NODISCARD bool has_parameter(const string& name) const { return parameters.find(name) != parameters.end(); }

    /**
     * @brief 获取Cookie值
     * @param name Cookie名
     * @return Cookie值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view cookie(const string& name) const {
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
    NEFORCE_NODISCARD bool has_cookie(const string& name) const { return cookies.find(name) != cookies.end(); }

    /**
     * @brief 获取请求头值
     * @param name 头名称
     * @return 头值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view header(const string& name) const {
        const auto it = headers.find(name);
        return it != headers.end() ? it->second.view() : "";
    }

    /**
     * @brief 按名称获取请求头，名称不区分大小写
     * @param name 头名称
     * @return 头值视图，不存在返回空视图
     */
    NEFORCE_NODISCARD string_view header_ci(string_view name) const;

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
    NEFORCE_NODISCARD bool has_header(const string& name) const { return headers.find(name) != headers.end(); }

    /**
     * @brief 检查是否有关联的有效会话
     * @return 存在有效会话返回true
     */
    NEFORCE_NODISCARD bool has_session() const noexcept {
        const auto* current = session.materialized();
        return current != nullptr && current->is_valid();
    }

    /**
     * @brief 设置按需会话创建解析器
     * @param resolver 会话解析器
     */
    void set_session_resolver(http_session_handle::resolver_t resolver) {
        session.set_resolver(_NEFORCE move(resolver));
    }

    /**
     * @brief 获取Content-Type
     * @return Content-Type值
     */
    NEFORCE_NODISCARD string_view content_type() const { return header(http_key::Content_Type()); }

    /**
     * @brief 检查是否为Keep-Alive连接
     * @return 是Keep-Alive返回true
     */
    NEFORCE_NODISCARD bool is_keep_alive() const {
        const auto conn = header(http_key::Connection());
        const string conn_lower = string(conn).lowercase();
        if (conn_lower == "close") {
            return false;
        }
        // HTTP/1.1+ defaults to keep-alive; HTTP/1.0 requires explicit keep-alive
        if (!version.starts_with("HTTP/1.0")) {
            return true;
        }
        return conn_lower.starts_with("keep-alive");
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
    NEFORCE_NODISCARD string_view user_agent() const { return header("User-Agent"); }

    /**
     * @brief 获取Referer
     * @return Referer值
     */
    NEFORCE_NODISCARD string_view referer() const { return header("Referer"); }

    /**
     * @brief 检查是否为AJAX请求
     * @return 是AJAX请求返回true
     *
     * 检查X-Requested-With头是否为XMLHttpRequest。
     */
    NEFORCE_NODISCARD bool is_ajax() const { return header("X-Requested-With") == "XMLHttpRequest"; }

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
 * response.status = http_status::ok;
 * response.status_message = "OK";
 *
 * // 设置内容
 * response.body = "<html><body>Hello World</body></html>";
 * response.set_content_type(http_content::HTML_TEXT());
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
    string version{"HTTP/1.1"};                  ///< HTTP版本
    http_status status{http_status::not_found};  ///< HTTP状态码
    string status_message;                       ///< 状态消息
    flat_unordered_map<string, string> headers;  ///< 响应头
    vector<http_cookie> cookies;                 ///< 设置的Cookie
    string body;                                 ///< 响应正文
    memory_view<const char> body_ref;            ///< 外部正文视图
    string redirect_url;                         ///< 重定向URL
    string forward_path;                         ///< 转发路径
    flat_unordered_map<string, string> trailers; ///< Trailer头部
    bool chunked_encoding{false};                ///< 是否使用chunked传输编码
    bool suppress_content_length{false};         ///< 是否禁止输出Content-Length
    string raw_response;                         ///< 原始 HTTP 响应
    bool close_after_send{false};                ///< 发送后是否关闭前端连接

    /**
     * @brief 默认构造函数
     *
     * 设置默认Content-Type为text/plain，Connection为close。
     */
    http_server_response() {
        headers[http_key::Content_Type()] = http_content::PLAIN_TEXT().to_string() + "; charset=utf-8";
    }

    /**
     * @brief 获取响应头值
     * @param name 头名称
     * @return 头值，不存在返回空字符串
     */
    NEFORCE_NODISCARD string_view header(const string& name) const {
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
    NEFORCE_NODISCARD bool has_header(const string& name) const { return headers.find(name) != headers.end(); }

    /**
     * @brief 检查当前状态码是否允许携带响应正文
     * @return 允许携带正文返回true
     *
     * 1xx、204、205 与 304 响应不允许携带正文（RFC 9110 §6.4.1、§15.3.6）。
     */
    NEFORCE_NODISCARD bool allows_body() const noexcept {
        const auto code = static_cast<uint16_t>(status);
        return code >= 200 && code != 204 && code != 205 && code != 304;
    }

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

    /**
     * @brief 获取实际响应正文视图
     */
    NEFORCE_NODISCARD string_view body_view() const {
        return body_ref.empty() ? string_view(body.data(), body.size()) : string_view(body_ref.data(), body_ref.size());
    }

    /**
     * @brief 直接引用外部正文，避免拷贝
     * @param view 正文视图，生命周期必须覆盖响应发送
     */
    void set_body_view(string_view view) {
        body_ref = memory_view<const char>(view.data(), view.size());
        body.clear();
    }

    /**
     * @brief 构建响应头字符串
     */
    NEFORCE_NODISCARD string build_header_string() const;

    // TODO: Problem Details (RFC 9457) — add set_problem_detail(type, title, detail, instance) for standardized error responses
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
