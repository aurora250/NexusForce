#ifndef NEFORCE_NETWORK_HTTP_HTTP_CONSTANTS_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CONSTANTS_HPP__

/**
 * @file http_constants.hpp
 * @brief HTTP协议常量定义
 *
 * 此文件提供了HTTP协议相关的常量定义，包括：
 * - HTTP状态码枚举
 * - HTTP方法定义
 * - 内容类型定义
 * - Cookie名称常量
 * - 协议分隔符
 */

#include "NeForce/core/interface/istringify.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @struct http_exception
 * @extends network_exception
 * @brief HTTP操作异常
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(http_exception, network_exception, "Http Actions Failed");

/** @} */ // Exceptions

NEFORCE_BEGIN_HTTP__

/**
 * @defgroup HTTP HTTP
 * @brief HTTP协议及操作
 *
 * 本模块提供了完整的 HTTP/1.1 协议支持，涵盖客户端与服务端消息结构、Cookie与会话管理、
 * 路由分发、中间件过滤器链以及 WebSocket 升级协议。
 *
 * @section standards 遵循的国际标准
 * 本实现严格遵循以下 IETF RFC 与相关标准规范：
 *
 * **HTTP/1.1 核心协议：**
 * - **IETF RFC 9110**：HTTP Semantics（HTTP 语义）
 *   https://www.rfc-editor.org/rfc/rfc9110.html
 * - **IETF RFC 9112**：HTTP/1.1（消息语法与路由）
 *   https://www.rfc-editor.org/rfc/rfc9112.html
 *
 * **HTTP/2 核心协议：**
 * - **IETF RFC 7540**：HTTP/2（帧层、流、连接管理）
 *   https://www.rfc-editor.org/rfc/rfc7540.html
 * - **IETF RFC 7541**：HPACK（HTTP/2 头部压缩算法）
 *   https://www.rfc-editor.org/rfc/rfc7541.html
 *
 * **HTTP 扩展能力：**
 * - **IETF RFC 9110 §8**：内容编码协商（Accept-Encoding / Content-Encoding）
 * - **IETF RFC 9110 §14**：字节范围请求（Range / Content-Range / 206 Partial Content）
 * - **IETF RFC 9110 §9.3.6**：CONNECT 方法（TCP 隧道代理）
 *
 * **HTTP 状态码与头字段注册：**
 * - **IANA HTTP Status Code Registry**：HTTP 状态码注册表
 *   https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml
 * - **IANA Message Headers Registry**：HTTP 头字段注册表
 *   https://www.iana.org/assignments/message-headers/message-headers.xhtml
 *
 * **HTTP 认证与安全：**
 * - **IETF RFC 9110 §11**：HTTP 认证框架
 * - **IETF RFC 6797**：HTTP Strict Transport Security (HSTS)
 *   https://www.rfc-editor.org/rfc/rfc6797.html
 * - **CSRF 防护**：Double-Submit Cookie 模式（防御跨站请求伪造攻击）
 * - **Session Fixation 防护**：会话标识符再生（regenerate_id）
 *
 * **TLS / SNI 安全传输：**
 * - **IETF RFC 6066 §3**：Server Name Indication（TLS 扩展）
 *   https://www.rfc-editor.org/rfc/rfc6066.html
 * - **IETF RFC 8446 §4.4.2**：TLS 1.3 中的 SNI 支持
 *   https://www.rfc-editor.org/rfc/rfc8446.html
 *
 * **Cookie 与会话管理：**
 * - **IETF RFC 6265**：HTTP State Management Mechanism（Cookie 规范）
 *   https://www.rfc-editor.org/rfc/rfc6265.html
 * - **IETF RFC 6265 §5.3**：SameSite Cookies（更新于 RFC 6265bis）
 *
 * **CORS 跨域资源共享：**
 * - **W3C Fetch Living Standard**：CORS 协议定义
 *   https://fetch.spec.whatwg.org/#http-cors-protocol
 *
 * **WebSocket 协议：**
 * - **IETF RFC 6455**：The WebSocket Protocol
 *   https://www.rfc-editor.org/rfc/rfc6455.html
 * - **IETF RFC 7692**：WebSocket Per-Message Deflate 扩展（permessage-deflate）
 *   https://www.rfc-editor.org/rfc/rfc7692.html
 *
 * **MIME 类型规范：**
 * - **IANA Media Types Registry**：MIME 类型注册表
 *   https://www.iana.org/assignments/media-types/media-types.xhtml
 * - **IETF RFC 6838**：Media Type Specifications and Registration Procedures
 *   https://www.rfc-editor.org/rfc/rfc6838.html
 *
 * @section http_status_categories HTTP 状态码分类
 * 根据 RFC 9110 §15，HTTP 状态码按百位数字分类：
 *
 * | 类别   | 状态码范围 | 含义           | 典型状态码                           |
 * |--------|------------|----------------|--------------------------------------|
 * | 1xx    | 100 – 199  | 信息响应       | 100 Continue, 101 Switching Protocols|
 * | 2xx    | 200 – 299  | 成功           | 200 OK, 201 Created, 204 No Content  |
 * | 3xx    | 300 – 399  | 重定向         | 301 Moved Permanently, 302 Found, 304 Not Modified |
 * | 4xx    | 400 – 499  | 客户端错误     | 400 Bad Request, 403 Forbidden, 404 Not Found |
 * | 5xx    | 500 – 599  | 服务器错误     | 500 Internal Server Error, 502 Bad Gateway, 503 Service Unavailable |
 *
 * @section http_methods HTTP 请求方法
 * 根据 RFC 9110 §9，本模块支持以下标准 HTTP 方法：
 *
 * | 方法      | RFC 引用  | 语义                   | 幂等性 | 安全性 |
 * |-----------|-----------|------------------------|--------|--------|
 * | GET       | §9.3.1    | 检索资源表示           | 是     | 是     |
 * | POST      | §9.3.3    | 提交数据进行处理       | 否     | 否     |
 * | PUT       | §9.3.4    | 替换或创建资源         | 是     | 否     |
 * | DELETE    | §9.3.5    | 删除资源               | 是     | 否     |
 * | HEAD      | §9.3.2    | 获取 GET 响应的头部    | 是     | 是     |
 * | OPTIONS   | §9.3.7    | 获取服务器支持的方法   | 是     | 是     |
 * | TRACE     | §9.3.8    | 回显请求（调试用）     | 是     | 是     |
 * | CONNECT   | §9.3.6    | 建立隧道（用于代理）   | 否     | 否     |
 * | PATCH     | RFC 5789  | 部分更新资源           | 否     | 否     |
 *
 * @section cookie_attributes Cookie 属性说明
 * 根据 RFC 6265 §4.1，Cookie 支持以下属性：
 *
 * | 属性      | 说明                                                         |
 * |-----------|--------------------------------------------------------------|
 * | Domain    | 指定 Cookie 可用的域名                                       |
 * | Path      | 指定 Cookie 可用的路径前缀                                   |
 * | Expires   | 指定过期时间（绝对时间）                                     |
 * | Max-Age   | 指定有效期（相对秒数），优先级高于 Expires                   |
 * | Secure    | 仅通过 HTTPS 传输                                            |
 * | HttpOnly  | 禁止 JavaScript 访问，缓解 XSS 攻击                          |
 * | SameSite  | 跨站请求策略：Strict（禁止跨站）、Lax（允许顶级导航）、None  |
 *
 * @section cors_headers CORS 响应头
 * 根据 Fetch 标准，CORS 使用以下响应头：
 *
 * | 响应头                                | 说明                         |
 * |---------------------------------------|------------------------------|
 * | Access-Control-Allow-Origin           | 允许访问的源                 |
 * | Access-Control-Allow-Methods          | 允许的 HTTP 方法             |
 * | Access-Control-Allow-Headers          | 允许的请求头                 |
 * | Access-Control-Allow-Credentials      | 是否允许携带凭证（Cookie）   |
 * | Access-Control-Max-Age                | 预检请求结果缓存时间         |
 *
 * @section websocket_protocol WebSocket 协议细节
 * 根据 RFC 6455，WebSocket 协议规范：
 *
 * **握手升级**：
 * - 客户端发送 `Upgrade: websocket` 和 `Connection: Upgrade`
 * - 服务器返回 `101 Switching Protocols`
 * - 使用 `Sec-WebSocket-Key` 和 `Sec-WebSocket-Accept` 验证握手
 *
 * **帧结构**（RFC 6455 §5.2）：
 * | 字段         | 位数 | 说明                                           |
 * |--------------|------|------------------------------------------------|
 * | FIN          | 1    | 是否为消息的最后一帧                           |
 * | RSV1-3       | 3    | 保留位，用于扩展                               |
 * | Opcode       | 4    | 帧类型：Continuation(0)、Text(1)、Binary(2)、Close(8)、Ping(9)、Pong(10) |
 * | MASK         | 1    | 客户端到服务器的帧必须设置掩码                 |
 * | Payload len  | 7/7+16/7+64 | 负载长度                                   |
 * | Masking key  | 0/32 | 掩码密钥（仅客户端帧）                         |
 *
 * **关闭状态码**（RFC 6455 §7.4）：
 * | 状态码 | 名称                      | 说明                           |
 * |--------|---------------------------|--------------------------------|
 * | 1000   | Normal Closure            | 正常关闭                       |
 * | 1001   | Going Away                | 端点离开（如浏览器关闭）       |
 * | 1002   | Protocol Error            | 协议错误                       |
 * | 1003   | Unsupported Data          | 接收到不支持的数据类型         |
 * | 1008   | Policy Violation          | 违反策略                       |
 * | 1009   | Message Too Big           | 消息过大                       |
 * | 1011   | Internal Error            | 服务器内部错误                 |
 *
 * @section implementation_details 实现细节
 * | 特性              | 规范参数                                  |
 * |-------------------|-------------------------------------------|
 * | HTTP 版本         | HTTP/1.1（RFC 9112）与 HTTP/2（RFC 7540） |
 * | HTTP/2 帧层       | 9 种帧类型：DATA / HEADERS / SETTINGS / PING / GOAWAY / RST_STREAM / WINDOW_UPDATE / PRIORITY /
 * PUSH_PROMISE | | HPACK 头部压缩    | 静态表 61 项 + 动态表 + Huffman 编码（RFC 7541） | | 头部字段大小写    |
 * 不区分大小写，存储为原始大小写            | | 分块传输编码      | 流式增量 chunked 解析，支持 Trailing Headers | |
 * 内容协商          | Accept-Encoding 解析，gzip/deflate 响应压缩 | | 字节范围请求      | 单范围 206 与多范围
 * multipart/byteranges 206 | | CONNECT 隧道      | 双工 TCP 隧道中继，用于 HTTPS 代理       | | 路由匹配          |
 * 基于 Segment Radix Tree 的 O(k) 前缀匹配   | | 静态路径、路径参数（:id）、通配符（*）、正则表达式回退 | |
 * 中间件执行顺序    | 预过滤 → 核心过滤 → 路由处理 → 后过滤     | | 会话存储          | 可插拔 session_store（内存 /
 * Redis 后端） | | Cookie 解析       | 支持 Set-Cookie 和 Cookie 头              | | 会话标识符        | 支持
 * JESSIONID, SESSIONID, PHPSESSID 等常见名称 | | CSRF 防护         | Double-Submit Cookie 模式，仅验证状态变更方法 | |
 * SNI 多证书        | 基于 TLS SNI 扩展的 hostname → SSL_CTX 映射，支持通配符 | | Event Loop        | Linux epoll
 * 边缘触发，min-heap 定时器，单线程异步 I/O | | WebSocket 运行模式 | 线程模式（3线程/会话）和事件驱动模式（零线程）双模
 * | | WebSocket 心跳    | 周期性发送 Ping 帧，等待 Pong 响应        | | WebSocket 压缩    | permessage-deflate（RFC
 * 7692），支持窗口比特位协商与上下文接管 | | WebSocket 升级    | 通用 Upgrade 分发器，支持自定义协议处理器 |
 *
 * @note 本模块的 HTTP 解析器严格遵循 RFC 9112 语法规则，支持分块传输编码（chunked）
 *       和 Content-Length 两种方式确定消息体长度。WebSocket 实现完整支持 RFC 6455
 *       定义的控制帧（Ping/Pong/Close）和分片消息，并支持 RFC 7692 定义的
 *       permessage-deflate 压缩扩展。HTTP/2 协议层提供纯 C++14 同步帧编解码，
 *       传输层由 event_loop 驱动。
 *
 * @warning 生产环境中应始终通过 HTTPS 使用 Secure 属性的 Cookie，
 *          并在敏感路由上启用 CSRF 防护。WebSocket 连接应考虑使用 WSS（WebSocket Secure）。
 *
 * @see https://www.rfc-editor.org/rfc/rfc9110.html
 * @see https://www.rfc-editor.org/rfc/rfc9112.html
 * @see https://www.rfc-editor.org/rfc/rfc7540.html
 * @see https://www.rfc-editor.org/rfc/rfc7541.html
 * @see https://www.rfc-editor.org/rfc/rfc6265.html
 * @see https://www.rfc-editor.org/rfc/rfc6455.html
 * @see https://www.rfc-editor.org/rfc/rfc7692.html
 * @see https://www.rfc-editor.org/rfc/rfc6066.html
 * @see https://fetch.spec.whatwg.org/
 */

/**
 * @enum http_status
 * @brief HTTP状态码枚举
 *
 * 定义了标准的HTTP状态码，按响应类别分组：
 * - 1xx：信息性状态码
 * - 2xx：成功状态码
 * - 3xx：重定向状态码
 * - 4xx：客户端错误状态码
 * - 5xx：服务器错误状态码
 */
enum class http_status : uint16_t {
    /**
     * @brief 100 Continue
     * 服务器收到请求头，客户端可以继续发送请求体
     */
    S1_CONTINUE = 100,

    /**
     * @brief 101 Switching Protocols
     * 同意切换协议
     */
    S1_SWITCHING_PROTOCOLS = 101,

    /**
     * @brief 200 OK
     * 请求成功
     */
    S2_OK = 200,

    /**
     * @brief 201 Created
     * 请求成功并创建了新资源
     */
    S2_CREATED = 201,

    /**
     * @brief 202 Accepted
     * 请求已接受但尚未处理
     */
    S2_ACCEPTED = 202,

    /**
     * @brief 204 No Content
     * 请求成功但无内容返回
     */
    S2_NO_CONTENT = 204,

    /**
     * @brief 206 Partial Content
     * 部分请求成功（范围请求）
     */
    S2_PARTIAL_CONTENT = 206,

    /**
     * @brief 301 Moved Permanently
     * 资源已永久迁移到新URL
     */
    S3_MOVED_PERMANENT = 301,

    /**
     * @brief 302 Found
     * 资源临时迁移到新URL
     */
    S3_FOUND = 302,

    /**
     * @brief 303 See Other
     * 重定向到另一个URI获取响应
     */
    S3_SEE_OTHER = 303,

    /**
     * @brief 304 Not Modified
     * 资源未修改，可使用本地缓存
     */
    S3_NOT_MODIFIED = 304,

    /**
     * @brief 307 Temporary Redirect
     * 临时重定向
     */
    S3_TEMPORARY_REDIRECT = 307,

    /**
     * @brief 308 Permanent Redirect
     * 永久重定向
     */
    S3_PERMANENT_REDIRECT = 308,

    /**
     * @brief 400 Bad Request
     * 请求格式错误
     */
    S4_BAD_REQUEST = 400,

    /**
     * @brief 401 Unauthorized
     * 需要身份验证
     */
    S4_UNAUTHORIZED = 401,

    /**
     * @brief 403 Forbidden
     * 服务器拒绝请求
     */
    S4_FORBIDDEN = 403,

    /**
     * @brief 404 Not Found
     * 请求的资源不存在
     */
    S4_NOT_FOUND = 404,

    /**
     * @brief 405 Method Not Allowed
     * 请求方法不允许
     */
    S4_METHOD_NOT_ALLOWED = 405,
    /**
     * @brief 406 Not Acceptable
     * 无法生成匹配Accept头的内容
     */
    S4_NOT_ACCEPTABLE = 406,

    /**
     * @brief 407 Proxy Authentication Required
     * 需要通过代理认证
     */
    S4_PROXY_AUTH_REQUIRED = 407,

    /**
     * @brief 408 Request Timeout
     * 客户端请求超时
     */
    S4_REQUEST_TIMEOUT = 408,

    /**
     * @brief 409 Conflict
     * 请求与资源当前状态冲突
     */
    S4_CONFLICT = 409,

    /**
     * @brief 410 Gone
     * 资源已永久删除
     */
    S4_GONE = 410,
    /**
     * @brief 411 Length Required
     * 请求未指定Content-Length
     */
    S4_LENGTH_REQUIRED = 411,

    /**
     * @brief 412 Precondition Failed
     * 请求先决条件未满足
     */
    S4_PRECONDITION_FAILED = 412,

    /**
     * @brief 415 Unsupported Media Type
     * 不支持的媒体格式
     */
    S4_UNSUPPORTED_MEDIA_TYPE = 415,

    /**
     * @brief 416 Range Not Satisfiable
     * 无法满足Range请求
     */
    S4_RANGE_NOT_SATISFIABLE = 416,
    /**
     * @brief 417 Expectation Failed
     * 无法满足Expect请求头
     */
    S4_EXPECTATION_FAILED = 417,

    /**
     * @brief 422 Unprocessable Content
     * 请求格式正确但语义有误
     */
    S4_UNPROCESSABLE_CONTENT = 422,

    /**
     * @brief 426 Upgrade Required
     * 需要升级协议
     */
    S4_UPGRADE_REQUIRED = 426,

    /**
     * @brief 413 Payload Too Large
     * 请求体过大
     */
    S4_PAYLOAD_TOO_LARGE = 413,

    /**
     * @brief 414 URI Too Long
     * 请求URL过长
     */
    S4_URI_TOO_LONG = 414,

    /**
     * @brief 429 Too Many Requests
     * 请求次数过多
     */
    S4_TOO_MANY_REQUESTS = 429,
    /**
     * @brief 431 Request Header Fields Too Large
     * 请求头字段过大
     */
    S4_HEADER_FIELDS_TOO_LARGE = 431,

    /**
     * @brief 451 Unavailable For Legal Reasons
     * 因法律原因不可用
     */
    S4_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

    /**
     * @brief 500 Internal Server Error
     * 服务器内部错误
     */
    S5_INTERNAL_SERVER_ERROR = 500,

    /**
     * @brief 502 Bad Gateway
     * 上游服务器响应无效
     */
    S5_BAD_GATEWAY = 502,

    /**
     * @brief 503 Service Unavailable
     * 服务暂时不可用
     */
    S5_SERVICE_UNAVAILABLE = 503,

    /**
     * @brief 504 Gateway Timeout
     * 上游服务器响应超时
     */
    S5_GATEWAY_TIMEOUT = 504,

    /**
     * @brief 501 Not Implemented
     * 服务器不支持该请求功能
     */
    S5_NOT_IMPLEMENTED = 501,

    /**
     * @brief 505 HTTP Version Not Supported
     * 不支持的HTTP版本
     */
    S5_HTTP_VERSION_NOT_SUPPORTED = 505
};

NEFORCE_API string http_status_message(http_status status);

NEFORCE_API http_status http_status_from_code(uint16_t code) noexcept;


/**
 * @struct http_content
 * @brief HTTP内容类型定义
 *
 * 定义了标准的HTTP Content-Type值，并提供类型判断方法。
 */
struct NEFORCE_API http_content : istringify<http_content> {
private:
    string content_{"UNKNOWN"}; ///< 内容类型字符串

public:
    http_content() = default;
    http_content(const http_content&) = default;
    http_content& operator=(const http_content&) = default;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    http_content(http_content&& other) noexcept :
    content_(_NEFORCE move(other.content_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    http_content& operator=(http_content&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }
        content_ = _NEFORCE move(other.content_);
        return *this;
    }

    /**
     * @brief 字符串构造函数
     * @param content 内容类型字符串
     */
    explicit http_content(string content) :
    content_(move(content)) {}

    /**
     * @brief 字符串赋值运算符
     * @param content 内容类型字符串
     * @return 自身引用
     */
    http_content& operator=(const string& content) {
        content_ = content;
        return *this;
    }

    ~http_content() = default;

    static const http_content& HTML_TEXT();  ///< text/html
    static const http_content& XML_TEXT();   ///< text/xml
    static const http_content& CSS_TEXT();   ///< text/css
    static const http_content& PLAIN_TEXT(); ///< text/plain
    static const http_content& JSON_APP();   ///< application/json
    static const http_content& FORM_APP();   ///< application/x-www-form-urlencoded
    static const http_content& JPEG_IMG();   ///< image/jpeg
    static const http_content& PNG_IMG();    ///< image/png
    static const http_content& BMP_IMG();    ///< image/bmp
    static const http_content& WEBP_IMG();   ///< image/webp
    static const http_content& HTML_MSG();   ///< message/html

    // TODO: application/problem+json — RFC 9457 Problem Details MIME type for standardized API error responses
    // TODO: application/octet-stream — generic binary data MIME type for file downloads
    // TODO: multipart/form-data — MIME type constant for form upload content negotiation

    NEFORCE_NODISCARD bool is_html_text() const { return content_ == HTML_TEXT().content_; }
    NEFORCE_NODISCARD bool is_xml_text() const { return content_ == XML_TEXT().content_; }
    NEFORCE_NODISCARD bool is_css_text() const { return content_ == CSS_TEXT().content_; }
    NEFORCE_NODISCARD bool is_plain_text() const { return content_ == PLAIN_TEXT().content_; }
    NEFORCE_NODISCARD bool is_json_app() const { return content_ == JSON_APP().content_; }
    NEFORCE_NODISCARD bool is_form_app() const { return content_ == FORM_APP().content_; }
    NEFORCE_NODISCARD bool is_jpeg_img() const { return content_ == JPEG_IMG().content_; }
    NEFORCE_NODISCARD bool is_png_img() const { return content_ == PNG_IMG().content_; }
    NEFORCE_NODISCARD bool is_bmp_img() const { return content_ == BMP_IMG().content_; }
    NEFORCE_NODISCARD bool is_webp_img() const { return content_ == WEBP_IMG().content_; }
    NEFORCE_NODISCARD bool is_html_msg() const { return content_ == HTML_MSG().content_; }

    NEFORCE_NODISCARD static bool is_html_text(const string_view view) { return view == HTML_TEXT().content_; }
    NEFORCE_NODISCARD static bool is_xml_text(const string_view view) { return view == XML_TEXT().content_; }
    NEFORCE_NODISCARD static bool is_css_text(const string_view view) { return view == CSS_TEXT().content_; }
    NEFORCE_NODISCARD static bool is_plain_text(const string_view view) { return view == PLAIN_TEXT().content_; }
    NEFORCE_NODISCARD static bool is_json_app(const string_view view) { return view == JSON_APP().content_; }
    NEFORCE_NODISCARD static bool is_form_app(const string_view view) { return view == FORM_APP().content_; }
    NEFORCE_NODISCARD static bool is_jpeg_img(const string_view view) { return view == JPEG_IMG().content_; }
    NEFORCE_NODISCARD static bool is_png_img(const string_view view) { return view == PNG_IMG().content_; }
    NEFORCE_NODISCARD static bool is_bmp_img(const string_view view) { return view == BMP_IMG().content_; }
    NEFORCE_NODISCARD static bool is_webp_img(const string_view view) { return view == WEBP_IMG().content_; }
    NEFORCE_NODISCARD static bool is_html_msg(const string_view view) { return view == HTML_MSG().content_; }

    /**
     * @brief 获取左值内容
     * @return 内容类型字符串
     */
    NEFORCE_NODISCARD const string& content() const& noexcept { return content_; }

    /**
     * @brief 获取右值内容
     * @return 内容类型字符串
     */
    NEFORCE_NODISCARD string content() && noexcept { return _NEFORCE move(content_); }

    /**
     * @brief 转换为字符串
     * @return 内容类型字符串
     */
    NEFORCE_NODISCARD string to_string() const { return content_; }
};


#ifdef DELETE
#    undef DELETE
#endif

/**
 * @struct http_method
 * @brief HTTP方法定义
 *
 * 定义了标准的HTTP请求方法，支持方法组合操作。
 */
struct NEFORCE_API http_method : istringify<http_method> {
private:
    string method_{"UNKNOWN"}; ///< HTTP方法字符串

public:
    http_method() = default;
    http_method(const http_method&) = default;
    http_method& operator=(const http_method&) = default;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    http_method(http_method&& other) noexcept :
    method_(_NEFORCE move(other.method_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    http_method& operator=(http_method&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        method_ = _NEFORCE move(other.method_);
        return *this;
    }

    /**
     * @brief 左值字符串构造函数
     * @param method HTTP方法字符串
     */
    explicit http_method(const string& method) :
    method_(method) {}

    /**
     * @brief 左值字符串赋值运算符
     * @param method HTTP方法字符串
     * @return 自身引用
     */
    http_method& operator=(const string& method) {
        method_ = method;
        return *this;
    }

    /**
     * @brief 右值字符串构造函数
     * @param method HTTP方法字符串
     */
    explicit http_method(string&& method) :
    method_(_NEFORCE move(method)) {}

    /**
     * @brief 右值字符串赋值运算符
     * @param method HTTP方法字符串
     * @return 自身引用
     */
    http_method& operator=(string&& method) {
        method_ = _NEFORCE move(method);
        return *this;
    }

    ~http_method() = default;

    static const http_method& GET();     ///< GET方法
    static const http_method& POST();    ///< POST方法
    static const http_method& HEAD();    ///< HEAD方法
    static const http_method& PUT();     ///< PUT方法
    static const http_method& DELETE();  ///< DELETE方法
    static const http_method& OPTIONS(); ///< OPTIONS方法
    static const http_method& TRACE();   ///< TRACE方法
    static const http_method& CONNECT(); ///< CONNECT方法
    static const http_method& PATCH();   ///< PATCH方法
    static const http_method& DEFAULT(); ///< 默认方法

    /**
     * @brief 获取左值方法
     * @return 方法字符串引用
     */
    NEFORCE_NODISCARD const string& method() const& noexcept { return method_; }

    /**
     * @brief 获取右值方法
     * @return 方法字符串
     */
    NEFORCE_NODISCARD string method() && noexcept { return _NEFORCE move(method_); }

    /**
     * @brief 方法组合操作符
     * @param rhs 右侧方法
     * @return 组合后的方法（使用逗号分隔）
     *
     * 用于表示允许多种方法的场景，如"GET, POST"
     */
    NEFORCE_NODISCARD http_method operator&(const http_method& rhs) const& {
        return http_method(method_ + ", " + rhs.method_);
    }

    NEFORCE_NODISCARD http_method operator&(http_method&& rhs) const& {
        return http_method(method_ + ", " + _NEFORCE move(rhs.method_));
    }

    NEFORCE_NODISCARD http_method operator&(const http_method& rhs) && {
        return http_method(_NEFORCE move(method_) + ", " + rhs.method_);
    }

    NEFORCE_NODISCARD http_method operator&(http_method&& rhs) && {
        return http_method(_NEFORCE move(method_) + ", " + _NEFORCE move(rhs.method_));
    }

    NEFORCE_NODISCARD bool is_get() const { return method_ == GET().method_; }
    NEFORCE_NODISCARD bool is_post() const { return method_ == POST().method_; }
    NEFORCE_NODISCARD bool is_head() const { return method_ == HEAD().method_; }
    NEFORCE_NODISCARD bool is_put() const { return method_ == PUT().method_; }
    NEFORCE_NODISCARD bool is_delete() const { return method_ == DELETE().method_; }
    NEFORCE_NODISCARD bool is_options() const { return method_ == OPTIONS().method_; }
    NEFORCE_NODISCARD bool is_trace() const { return method_ == TRACE().method_; }
    NEFORCE_NODISCARD bool is_connect() const { return method_ == CONNECT().method_; }
    NEFORCE_NODISCARD bool is_patch() const { return method_ == PATCH().method_; }

    /**
     * @brief 转换为字符串
     * @return 方法字符串
     */
    NEFORCE_NODISCARD string to_string() const { return method_; }
};


/**
 * @struct http_cookie_name
 * @brief HTTP Cookie名称定义
 *
 * 定义了常见的Cookie名称常量，用于会话管理。
 */
struct NEFORCE_API http_cookie_name : istringify<http_cookie_name> {
private:
    string cookie_{"UNKNOWN"}; ///< Cookie名称字符串

public:
    http_cookie_name() = default;
    http_cookie_name(const http_cookie_name&) = default;
    http_cookie_name& operator=(const http_cookie_name&) = default;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    http_cookie_name(http_cookie_name&& other) noexcept :
    cookie_(_NEFORCE move(other.cookie_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    http_cookie_name& operator=(http_cookie_name&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        cookie_ = _NEFORCE move(other.cookie_);
        return *this;
    }

    /**
     * @brief 字符串构造函数
     * @param cookie Cookie名称
     */
    explicit http_cookie_name(string cookie) :
    cookie_(move(cookie)) {}

    /**
     * @brief 字符串赋值运算符
     * @param cookie Cookie名称
     * @return 自身引用
     */
    http_cookie_name& operator=(const string& cookie) {
        cookie_ = cookie;
        return *this;
    }

    ~http_cookie_name() = default;

    static const http_cookie_name& JSESSIONID();   ///< Java/JSP会话ID
    static const http_cookie_name& SESSIONID();    ///< 通用会话ID
    static const http_cookie_name& PHPSESSID();    ///< PHP会话ID
    static const http_cookie_name& ASPSESSIONID(); ///< ASP会话ID

    /**
     * @brief 获取左值Cookie名称
     * @return Cookie名称引用
     */
    NEFORCE_NODISCARD const string& cookie_name() const& noexcept { return cookie_; }

    /**
     * @brief 获取右值Cookie名称
     * @return Cookie名称字符串
     */
    NEFORCE_NODISCARD string cookie_name() && noexcept { return _NEFORCE move(cookie_); }

    /**
     * @brief 转换为字符串
     * @return Cookie名称字符串
     */
    NEFORCE_NODISCARD string to_string() const { return cookie_; }
};


struct NEFORCE_API http_key {
    static const string& Access_Control_Allow_Credentials();
    static const string& Access_Control_Allow_Headers();
    static const string& Access_Control_Allow_Methods();
    static const string& Access_Control_Allow_Origin();
    static const string& Access_Control_Max_Age();
    static const string& Connection();
    static const string& Content_Length();
    static const string& Content_Type();
    static const string& Lax();
    static const string& Strict();
    static const string& X_Forwarded_Proto();

    static const string& Strict_Transport_Security();
    static const string& X_Frame_Options();
    static const string& X_Content_Type_Options();
    static const string& Content_Security_Policy();
    static const string& X_XSS_Protection();
    static const string& Referrer_Policy();
    static const string& Permissions_Policy();
};

/** @} */ // Http

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CONSTANTS_HPP__
