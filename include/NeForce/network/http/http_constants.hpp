#ifndef NEFORCE_NETWORK_HTTP_CONSTANTS_HPP__
#define NEFORCE_NETWORK_HTTP_CONSTANTS_HPP__

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

/**
 * @defgroup Http HTTP
 * @brief HTTP协议及操作
 * @{
 */

/**
 * @enum HTTP_STATUS
 * @brief HTTP状态码枚举
 *
 * 定义了标准的HTTP状态码，按响应类别分组：
 * - 1xx：信息性状态码
 * - 2xx：成功状态码
 * - 3xx：重定向状态码
 * - 4xx：客户端错误状态码
 * - 5xx：服务器错误状态码
 */
enum class HTTP_STATUS : uint16_t {
    /**
     * @brief 100 Continue
     * 服务器收到请求头，客户端可以继续发送请求体
     */
    S1_CONTINUE = 100,

    /**
     * @brief 101 Switching Protocols
     * 同意切换协议
     */
    S1_SWITCH_PROTOCOL = 101,

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
     * @brief 304 Not Modified
     * 资源未修改，可使用本地缓存
     */
    S3_NO_MODIFIED = 304,

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
    S4_NOT_FOUNT = 404,

    /**
     * @brief 405 Method Not Allowed
     * 请求方法不允许
     */
    S4_METHOD_NOT_ALLOWED = 405,

    /**
     * @brief 408 Request Timeout
     * 客户端请求超时
     */
    S4_REQUEST_TIMEOUT = 408,

    /**
     * @brief 413 Payload Too Large
     * 请求体过大
     */
    S4_PAYLOAD_LARGE = 413,

    /**
     * @brief 414 URI Too Long
     * 请求URL过长
     */
    S4_URL_LONG = 414,

    /**
     * @brief 429 Too Many Requests
     * 请求次数过多
     */
    S4_MANY_REQUESTS = 429,

    /**
     * @brief 500 Internal Server Error
     * 服务器内部错误
     */
    S5_INTERNAL_ERROR = 500,

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
     * @brief 505 HTTP Version Not Supported
     * 不支持的HTTP版本
     */
    S5_HTTP_VERSION_NOT_SUPPORT = 505
};

enum class WEBSOCKET_STATUS : uint16_t {
    NORMAL_CLOSURE = 1000,
    GOING_AWAY = 1001,
    PROTOCOL_ERROR = 1002,
    UNSUPPORTED_DATA = 1003,
    RESERVED = 1004,
    NO_STATUS_RCVD = 1005,
    ABNORMAL_CLOSURE = 1006,
    INVALID_FRAME_PAYLOAD_DATA = 1007,
    POLICY_VIOLATION = 1008,
    MESSAGE_TOO_BIG = 1009,
    MANDATORY_EXT = 1010,
    INTERNAL_ERROR = 1011,
    SERVICE_RESTART = 1012,
    TRY_AGAIN_LATER = 1013,
    BAD_GATEWAY = 1014,
    TLS_HANDSHAKE = 1015
};


/**
 * @struct HTTP_CONTENT
 * @brief HTTP内容类型定义
 *
 * 定义了标准的HTTP Content-Type值，并提供类型判断方法。
 */
struct NEFORCE_API HTTP_CONTENT : istringify<HTTP_CONTENT> {
private:
    string content_{"UNKNOWN"};  ///< 内容类型字符串

public:
    HTTP_CONTENT() = default;
    HTTP_CONTENT(const HTTP_CONTENT&) = default;
    HTTP_CONTENT& operator =(const HTTP_CONTENT&) = default;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    HTTP_CONTENT(HTTP_CONTENT&& other) noexcept
    : content_(_NEFORCE move(other.content_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    HTTP_CONTENT& operator =(HTTP_CONTENT&& other) noexcept {
        if (addressof(other) == this) return *this;
        content_ = _NEFORCE move(other.content_);
        return *this;
    }

    /**
     * @brief 字符串构造函数
     * @param content 内容类型字符串
     */
    explicit HTTP_CONTENT(const string& content)
    : content_(content) {}

    /**
     * @brief 字符串赋值运算符
     * @param content 内容类型字符串
     * @return 自身引用
     */
    HTTP_CONTENT& operator =(const string& content) {
        content_ = content;
        return *this;
    }

    ~HTTP_CONTENT() = default;

    static const HTTP_CONTENT HTML_TEXT;   ///< text/html
    static const HTTP_CONTENT XML_TEXT;    ///< text/xml
    static const HTTP_CONTENT CSS_TEXT;    ///< text/css
    static const HTTP_CONTENT PLAIN_TEXT;  ///< text/plain
    static const HTTP_CONTENT JSON_APP;    ///< application/json
    static const HTTP_CONTENT FORM_APP;    ///< application/x-www-form-urlencoded
    static const HTTP_CONTENT JPEG_IMG;    ///< image/jpeg
    static const HTTP_CONTENT PNG_IMG;     ///< image/png
    static const HTTP_CONTENT BMP_IMG;     ///< image/bmp
    static const HTTP_CONTENT WEBP_IMG;    ///< image/webp
    static const HTTP_CONTENT HTML_MSG;    ///< message/html

    NEFORCE_NODISCARD bool is_html_text() const noexcept { return content_ == HTML_TEXT.content_; }
    NEFORCE_NODISCARD bool is_xml_text() const noexcept { return content_ == XML_TEXT.content_; }
    NEFORCE_NODISCARD bool is_css_text() const noexcept { return content_ == CSS_TEXT.content_; }
    NEFORCE_NODISCARD bool is_plain_text() const noexcept { return content_ == PLAIN_TEXT.content_; }
    NEFORCE_NODISCARD bool is_json_app() const noexcept { return content_ == JSON_APP.content_; }
    NEFORCE_NODISCARD bool is_form_app() const noexcept { return content_ == FORM_APP.content_; }
    NEFORCE_NODISCARD bool is_jpeg_img() const noexcept { return content_ == JPEG_IMG.content_; }
    NEFORCE_NODISCARD bool is_png_img() const noexcept { return content_ == PNG_IMG.content_; }
    NEFORCE_NODISCARD bool is_bmp_img() const noexcept { return content_ == BMP_IMG.content_; }
    NEFORCE_NODISCARD bool is_webp_img() const noexcept { return content_ == WEBP_IMG.content_; }
    NEFORCE_NODISCARD bool is_html_msg() const noexcept { return content_ == HTML_MSG.content_; }

    NEFORCE_NODISCARD static bool is_html_text(const string_view view) noexcept { return view == HTML_TEXT.content_; }
    NEFORCE_NODISCARD static bool is_xml_text(const string_view view) noexcept { return view == XML_TEXT.content_; }
    NEFORCE_NODISCARD static bool is_css_text(const string_view view) noexcept { return view == CSS_TEXT.content_; }
    NEFORCE_NODISCARD static bool is_plain_text(const string_view view) noexcept { return view == PLAIN_TEXT.content_; }
    NEFORCE_NODISCARD static bool is_json_app(const string_view view) noexcept { return view == JSON_APP.content_; }
    NEFORCE_NODISCARD static bool is_form_app(const string_view view) noexcept { return view == FORM_APP.content_; }
    NEFORCE_NODISCARD static bool is_jpeg_img(const string_view view) noexcept { return view == JPEG_IMG.content_; }
    NEFORCE_NODISCARD static bool is_png_img(const string_view view) noexcept { return view == PNG_IMG.content_; }
    NEFORCE_NODISCARD static bool is_bmp_img(const string_view view) noexcept { return view == BMP_IMG.content_; }
    NEFORCE_NODISCARD static bool is_webp_img(const string_view view) noexcept { return view == WEBP_IMG.content_; }
    NEFORCE_NODISCARD static bool is_html_msg(const string_view view) noexcept { return view == HTML_MSG.content_; }

    /**
     * @brief 获取左值内容
     * @return 内容类型字符串
     */
    NEFORCE_NODISCARD const string& content() const & noexcept {
        return content_;
    }

    /**
     * @brief 获取右值内容
     * @return 内容类型字符串
     */
    NEFORCE_NODISCARD string content() && noexcept {
        return _NEFORCE move(content_);
    }

    /**
     * @brief 转换为字符串
     * @return 内容类型字符串
     */
    NEFORCE_NODISCARD string to_string() const {
        return content_;
    }
};


#ifdef DELETE
#undef DELETE
#endif

/**
 * @struct HTTP_METHOD
 * @brief HTTP方法定义
 *
 * 定义了标准的HTTP请求方法，支持方法组合操作。
 */
struct NEFORCE_API HTTP_METHOD : istringify<HTTP_METHOD> {
private:
    string method_{"UNKNOWN"};  ///< HTTP方法字符串

public:
    HTTP_METHOD() = default;
    HTTP_METHOD(const HTTP_METHOD&) = default;
    HTTP_METHOD& operator =(const HTTP_METHOD&) = default;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    HTTP_METHOD(HTTP_METHOD&& other) noexcept
    : method_(_NEFORCE move(other.method_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    HTTP_METHOD& operator =(HTTP_METHOD&& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        method_ = _NEFORCE move(other.method_);
        return *this;
    }

    /**
     * @brief 左值字符串构造函数
     * @param method HTTP方法字符串
     */
    explicit HTTP_METHOD(const string& method)
    : method_(method) {}

    /**
     * @brief 左值字符串赋值运算符
     * @param method HTTP方法字符串
     * @return 自身引用
     */
    HTTP_METHOD& operator =(const string& method) {
        method_ = method;
        return *this;
    }

    /**
     * @brief 右值字符串构造函数
     * @param method HTTP方法字符串
     */
    explicit HTTP_METHOD(string&& method)
    : method_(_NEFORCE move(method)) {}

    /**
     * @brief 右值字符串赋值运算符
     * @param method HTTP方法字符串
     * @return 自身引用
     */
    HTTP_METHOD& operator =(string&& method) {
        method_ = _NEFORCE move(method);
        return *this;
    }

    ~HTTP_METHOD() = default;

    static const HTTP_METHOD GET;       ///< GET方法
    static const HTTP_METHOD POST;      ///< POST方法
    static const HTTP_METHOD HEAD;      ///< HEAD方法
    static const HTTP_METHOD PUT;       ///< PUT方法
    static const HTTP_METHOD DELETE;    ///< DELETE方法
    static const HTTP_METHOD OPTIONS;   ///< OPTIONS方法
    static const HTTP_METHOD TRACE;     ///< TRACE方法
    static const HTTP_METHOD CONNECT;   ///< CONNECT方法
    static const HTTP_METHOD DEFAULT;   ///< 默认方法

    /**
     * @brief 获取左值方法
     * @return 方法字符串引用
     */
    NEFORCE_NODISCARD const string& method() const & noexcept {
        return method_;
    }

    /**
     * @brief 获取右值方法
     * @return 方法字符串
     */
    NEFORCE_NODISCARD string method() && noexcept {
        return _NEFORCE move(method_);
    }

    /**
     * @brief 方法组合操作符
     * @param rhs 右侧方法
     * @return 组合后的方法（使用逗号分隔）
     *
     * 用于表示允许多种方法的场景，如"GET, POST"
     */
    NEFORCE_NODISCARD HTTP_METHOD operator &(const HTTP_METHOD& rhs) const & {
        return HTTP_METHOD(method_ + ", " + rhs.method_);
    }

    NEFORCE_NODISCARD HTTP_METHOD operator &(HTTP_METHOD&& rhs) const & {
        return HTTP_METHOD(method_ + ", " + _NEFORCE move(rhs.method_));
    }

    NEFORCE_NODISCARD HTTP_METHOD operator &(const HTTP_METHOD& rhs) && {
        return HTTP_METHOD(_NEFORCE move(method_) + ", " + rhs.method_);
    }

    NEFORCE_NODISCARD HTTP_METHOD operator &(HTTP_METHOD&& rhs) && {
        return HTTP_METHOD(_NEFORCE move(method_) + ", " + _NEFORCE move(rhs.method_));
    }

    NEFORCE_NODISCARD bool is_get() const noexcept { return method_ == GET.method_; }
    NEFORCE_NODISCARD bool is_post() const noexcept { return method_ == POST.method_; }
    NEFORCE_NODISCARD bool is_head() const noexcept { return method_ == HEAD.method_; }
    NEFORCE_NODISCARD bool is_put() const noexcept { return method_ == PUT.method_; }
    NEFORCE_NODISCARD bool is_delete() const noexcept { return method_ == DELETE.method_; }
    NEFORCE_NODISCARD bool is_options() const noexcept { return method_ == OPTIONS.method_; }
    NEFORCE_NODISCARD bool is_trace() const noexcept { return method_ == TRACE.method_; }
    NEFORCE_NODISCARD bool is_connect() const noexcept { return method_ == CONNECT.method_; }

    /**
     * @brief 转换为字符串
     * @return 方法字符串
     */
    NEFORCE_NODISCARD string to_string() const {
        return method_;
    }
};


/**
 * @struct HTTP_COOKIE_NAME
 * @brief HTTP Cookie名称定义
 *
 * 定义了常见的Cookie名称常量，用于会话管理。
 */
struct NEFORCE_API HTTP_COOKIE_NAME : istringify<HTTP_COOKIE_NAME> {
private:
    string cookie_{"UNKNOWN"};  ///< Cookie名称字符串

public:
    HTTP_COOKIE_NAME() = default;
    HTTP_COOKIE_NAME(const HTTP_COOKIE_NAME&) = default;
    HTTP_COOKIE_NAME& operator =(const HTTP_COOKIE_NAME&) = default;

    /**
     * @brief 移动构造函数
     * @param other 源对象
     */
    HTTP_COOKIE_NAME(HTTP_COOKIE_NAME&& other) noexcept
    : cookie_(_NEFORCE move(other.cookie_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源对象
     * @return 自身引用
     */
    HTTP_COOKIE_NAME& operator =(HTTP_COOKIE_NAME&& other) noexcept {
        if (_NEFORCE addressof(other) == this) return *this;
        cookie_ = _NEFORCE move(other.cookie_);
        return *this;
    }

    /**
     * @brief 字符串构造函数
     * @param cookie Cookie名称
     */
    explicit HTTP_COOKIE_NAME(const string& cookie)
    : cookie_(cookie) {}

    /**
     * @brief 字符串赋值运算符
     * @param cookie Cookie名称
     * @return 自身引用
     */
    HTTP_COOKIE_NAME& operator =(const string& cookie) {
        cookie_ = cookie;
        return *this;
    }

    ~HTTP_COOKIE_NAME() = default;

    static const HTTP_COOKIE_NAME JSESSIONID;    ///< Java/JSP会话ID
    static const HTTP_COOKIE_NAME SESSIONID;     ///< 通用会话ID
    static const HTTP_COOKIE_NAME PHPSESSID;     ///< PHP会话ID
    static const HTTP_COOKIE_NAME ASPSESSIONID;  ///< ASP会话ID

    /**
     * @brief 获取左值Cookie名称
     * @return Cookie名称引用
     */
    NEFORCE_NODISCARD const string& cookie_name() const & noexcept {
        return cookie_;
    }

    /**
     * @brief 获取右值Cookie名称
     * @return Cookie名称字符串
     */
    NEFORCE_NODISCARD string cookie_name() && noexcept {
        return _NEFORCE move(cookie_);
    }

    /**
     * @brief 转换为字符串
     * @return Cookie名称字符串
     */
    NEFORCE_NODISCARD string to_string() const {
        return cookie_;
    }
};

/** @} */ // Http

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_CONSTANTS_HPP__
