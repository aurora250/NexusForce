#ifndef MSTL_NETWORK_HTTP_CONSTANTS_HPP__
#define MSTL_NETWORK_HTTP_CONSTANTS_HPP__
#include "MSTL/core/interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(http_exception, link_exception, "Http Actions Failed");


enum class HTTP_STATUS : uint32_t {
    // 1xx Message Codes

    // Server received the request header and can continue to send the request body
    S1_CONTINUE = 100,
    // Consent to switch protocols
    S1_SWITCH_PROTOCOL = 101,

    // 2xx Success Codes

    // Request succeed
    S2_OK = 200,
    // Request succeed and created new resources
    S2_CREATED = 201,
    // Request succeed but no content is returned
    S2_NO_CONTENT = 204,
    // Partial request succeed
    S2_PARTIAL_CONTENT = 206,

    // 3xx Redirect Codes

    // Resources are permanently migrated to the new URL
    S3_MOVED_PERMANENT = 301,
    // Resources are temporarily migrated to the new URL
    S3_FOUND = 302,
    // Resources are unmodified and use the local cache to save
    S3_NO_MODIFIED = 304,
    // Temporary redirect
    S3_TEMPORARY_REDIRECT = 307,
    // Permanent redirect
    S3_PERMANENT_REDIRECT = 308,

    // 4xx Client Error Codes

    // Request is in the wrong format
    S4_BAD_REQUEST = 400,
    // Identity verification is required
    S4_UNAUTHORIZED = 401,
    // Server rejects request
    S4_FORBIDDEN = 403,
    // The requested resource does not exist
    S4_NOT_FOUNT = 404,
    // The request method is not allowed or not existed
    S4_METHOD_NOT_ALLOWED = 405,
    // Client request timeout
    S4_REQUEST_TIMEOUT = 408,
    // The request is too large
    S4_PAYLOAD_LARGE = 413,
    // The URL of the request is too long
    S4_URL_LONG = 414,
    // Excessive number of requests
    S4_MANY_REQUESTS = 429,

    // 5xx Server Error Codes

    // Internal server error
    S5_INTERNAL_ERROR = 500,
    // Invalid upstream server response
    S5_BAD_GATEWAY = 502,
    // The service is temporarily unavailable
    S5_SERVICE_UNAVAILABLE = 503,
    // The upstream server responds too slowly
    S5_GATEWAY_TIMEOUT = 504,
    // The HTTP version of the request is not supported
    S5_HTTP_VERSION_NOT_SUPPORT = 505
};


MSTL_INLINE17 constexpr string_view HTTP_CRLF = "\r\n";
MSTL_INLINE17 constexpr string_view HTTP_CRLF2 = "\r\n\r\n";


struct MSTL_API HTTP_CONTENT : istringify<HTTP_CONTENT> {
private:
    string content_{"UNKNOWN"};

public:
    HTTP_CONTENT() = default;
    HTTP_CONTENT(const HTTP_CONTENT&) = default;
    HTTP_CONTENT& operator=(const HTTP_CONTENT&) = default;

    HTTP_CONTENT(HTTP_CONTENT&& content) noexcept : content_(_MSTL move(content.content_)) {}

    HTTP_CONTENT& operator =(HTTP_CONTENT&& content) noexcept {
        if (addressof(content) == this) return *this;
        content_ = _MSTL move(content.content_);
        return *this;
    }

    explicit HTTP_CONTENT(const string& content) : content_(content) {}

    HTTP_CONTENT& operator =(const string& content) {
        content_ = content;
        return *this;
    }

    ~HTTP_CONTENT() = default;


    static const HTTP_CONTENT HTML_TEXT;
    static const HTTP_CONTENT XML_TEXT;
    static const HTTP_CONTENT CSS_TEXT;
    static const HTTP_CONTENT PLAIN_TEXT;
    static const HTTP_CONTENT JSON_APP;
    static const HTTP_CONTENT FORM_APP;
    static const HTTP_CONTENT JPEG_IMG;
    static const HTTP_CONTENT PNG_IMG;
    static const HTTP_CONTENT BMP_IMG;
    static const HTTP_CONTENT WEBP_IMG;
    static const HTTP_CONTENT HTML_MSG;

    MSTL_NODISCARD bool is_html_text() const noexcept { return content_ == HTML_TEXT.content_; }
    MSTL_NODISCARD bool is_xml_text() const noexcept { return content_ == XML_TEXT.content_; }
    MSTL_NODISCARD bool is_css_text() const noexcept { return content_ == CSS_TEXT.content_; }
    MSTL_NODISCARD bool is_plain_text() const noexcept { return content_ == PLAIN_TEXT.content_; }
    MSTL_NODISCARD bool is_json_app() const noexcept { return content_ == JSON_APP.content_; }
    MSTL_NODISCARD bool is_form_app() const noexcept { return content_ == FORM_APP.content_; }
    MSTL_NODISCARD bool is_jpeg_img() const noexcept { return content_ == JPEG_IMG.content_; }
    MSTL_NODISCARD bool is_png_img() const noexcept { return content_ == PNG_IMG.content_; }
    MSTL_NODISCARD bool is_bmp_img() const noexcept { return content_ == BMP_IMG.content_; }
    MSTL_NODISCARD bool is_webp_img() const noexcept { return content_ == WEBP_IMG.content_; }
    MSTL_NODISCARD bool is_html_msg() const noexcept { return content_ == HTML_MSG.content_; }

    MSTL_NODISCARD static bool is_html_text(const string_view view) noexcept { return view == HTML_TEXT.content_; }
    MSTL_NODISCARD static bool is_xml_text(const string_view view) noexcept { return view == XML_TEXT.content_; }
    MSTL_NODISCARD static bool is_css_text(const string_view view) noexcept { return view == CSS_TEXT.content_; }
    MSTL_NODISCARD static bool is_plain_text(const string_view view) noexcept { return view == PLAIN_TEXT.content_; }
    MSTL_NODISCARD static bool is_json_app(const string_view view) noexcept { return view == JSON_APP.content_; }
    MSTL_NODISCARD static bool is_form_app(const string_view view) noexcept { return view == FORM_APP.content_; }
    MSTL_NODISCARD static bool is_jpeg_img(const string_view view) noexcept { return view == JPEG_IMG.content_; }
    MSTL_NODISCARD static bool is_png_img(const string_view view) noexcept { return view == PNG_IMG.content_; }
    MSTL_NODISCARD static bool is_bmp_img(const string_view view) noexcept { return view == BMP_IMG.content_; }
    MSTL_NODISCARD static bool is_webp_img(const string_view view) noexcept { return view == WEBP_IMG.content_; }
    MSTL_NODISCARD static bool is_html_msg(const string_view view) noexcept { return view == HTML_MSG.content_; }


    MSTL_NODISCARD string_view content() const & noexcept { return content_.view(); }
    MSTL_NODISCARD string content() && noexcept { return _MSTL move(content_); }
    MSTL_NODISCARD string to_string() const { return content_; }
};


#ifdef DELETE
#undef DELETE
#endif

struct MSTL_API HTTP_METHOD : istringify<HTTP_METHOD> {
private:
    string method_{"UNKNOWN"};

public:
    HTTP_METHOD() = default;
    HTTP_METHOD(const HTTP_METHOD&) = default;
    HTTP_METHOD& operator =(const HTTP_METHOD&) = default;

    HTTP_METHOD(HTTP_METHOD&& method) noexcept : method_(_MSTL move(method.method_)) {}
    HTTP_METHOD& operator =(HTTP_METHOD&& method) noexcept {
        if (_MSTL addressof(method) == this) return *this;
        method_ = _MSTL move(method.method_);
        return *this;
    }

    explicit HTTP_METHOD(const string& method)
    : method_(method) {}

    HTTP_METHOD& operator =(const string& method) {
        method_ = method;
        return *this;
    }

    explicit HTTP_METHOD(string&& method)
    : method_(_MSTL move(method)) {}

    HTTP_METHOD& operator =(string&& method) {
        method_ = _MSTL move(method);
        return *this;
    }

    ~HTTP_METHOD() = default;

    static const HTTP_METHOD GET;
    static const HTTP_METHOD POST;
    static const HTTP_METHOD HEAD;
    static const HTTP_METHOD PUT;
    static const HTTP_METHOD DELETE;
    static const HTTP_METHOD OPTIONS;
    static const HTTP_METHOD TRACE;
    static const HTTP_METHOD CONNECT;
    static const HTTP_METHOD DEFAULT;


    MSTL_NODISCARD const string& method() const & noexcept { return method_; }
    MSTL_NODISCARD string method() && noexcept { return _MSTL move(method_); }

    MSTL_NODISCARD HTTP_METHOD operator &(const HTTP_METHOD& rh) const & {
        return HTTP_METHOD(method_ + ", " + rh.method_);
    }
    MSTL_NODISCARD HTTP_METHOD operator &(HTTP_METHOD&& rh) const & {
        return HTTP_METHOD(method_ + ", " + _MSTL move(rh.method_));
    }
    MSTL_NODISCARD HTTP_METHOD operator &(const HTTP_METHOD& rh) && {
        return HTTP_METHOD(_MSTL move(method_) + ", " + rh.method_);
    }
    MSTL_NODISCARD HTTP_METHOD operator &(HTTP_METHOD&& rh) && {
        return HTTP_METHOD(_MSTL move(method_) + ", " + _MSTL move(rh.method_));
    }

    MSTL_NODISCARD bool is_get() const noexcept { return method_ == GET.method_; }
    MSTL_NODISCARD bool is_post() const noexcept { return method_ == POST.method_; }
    MSTL_NODISCARD bool is_head() const noexcept { return method_ == HEAD.method_; }
    MSTL_NODISCARD bool is_put() const noexcept { return method_ == PUT.method_; }
    MSTL_NODISCARD bool is_delete() const noexcept { return method_ == DELETE.method_; }
    MSTL_NODISCARD bool is_options() const noexcept { return method_ == OPTIONS.method_; }
    MSTL_NODISCARD bool is_trace() const noexcept { return method_ == TRACE.method_; }
    MSTL_NODISCARD bool is_connect() const noexcept { return method_ == CONNECT.method_; }

    MSTL_NODISCARD string to_string() const { return method_; }
};


struct MSTL_API HTTP_COOKIE_NAME : istringify<HTTP_COOKIE_NAME> {
private:
    string cookie_{"UNKNOWN"};

public:
    HTTP_COOKIE_NAME() = default;
    HTTP_COOKIE_NAME(const HTTP_COOKIE_NAME&) = default;
    HTTP_COOKIE_NAME& operator =(const HTTP_COOKIE_NAME&) = default;

    HTTP_COOKIE_NAME(HTTP_COOKIE_NAME&& cookie) noexcept : cookie_(_MSTL move(cookie.cookie_)) {}
    HTTP_COOKIE_NAME& operator =(HTTP_COOKIE_NAME&& cookie) noexcept {
        if (_MSTL addressof(cookie) == this) return *this;
        cookie_ = _MSTL move(cookie.cookie_);
        return *this;
    }

    explicit HTTP_COOKIE_NAME(const string& cookie) : cookie_(cookie) {}
    HTTP_COOKIE_NAME& operator =(const string& cookie) {
        cookie_ = cookie;
        return *this;
    }

    ~HTTP_COOKIE_NAME() = default;

    static const HTTP_COOKIE_NAME JSESSIONID;
    static const HTTP_COOKIE_NAME SESSIONID;
    static const HTTP_COOKIE_NAME PHPSESSID;
    static const HTTP_COOKIE_NAME ASPSESSIONID;

    MSTL_NODISCARD const string& cookie_name() const & noexcept { return cookie_; }
    MSTL_NODISCARD string cookie_name() && noexcept { return _MSTL move(cookie_); }

    MSTL_NODISCARD string to_string() const { return cookie_; }
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_CONSTANTS_HPP__
