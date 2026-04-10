#include <NeForce/network/http/http_constants.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

#ifdef DELETE
#    undef DELETE
#endif

string http_status_message(const http_status status) {
    switch (status) {
        case http_status::S1_CONTINUE:
            return "Continue";
        case http_status::S1_SWITCH_PROTOCOL:
            return "Switching Protocols";
        case http_status::S2_OK:
            return "OK";
        case http_status::S2_CREATED:
            return "Created";
        case http_status::S2_NO_CONTENT:
            return "No Content";
        case http_status::S2_PARTIAL_CONTENT:
            return "Partial Content";
        case http_status::S3_MOVED_PERMANENT:
            return "Moved Permanently";
        case http_status::S3_FOUND:
            return "Found";
        case http_status::S3_NO_MODIFIED:
            return "Not Modified";
        case http_status::S3_TEMPORARY_REDIRECT:
            return "Temporary Redirect";
        case http_status::S3_PERMANENT_REDIRECT:
            return "Permanent Redirect";
        case http_status::S4_BAD_REQUEST:
            return "Bad Request";
        case http_status::S4_UNAUTHORIZED:
            return "Unauthorized";
        case http_status::S4_FORBIDDEN:
            return "Forbidden";
        case http_status::S4_NOT_FOUNT:
            return "Not Found";
        case http_status::S4_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case http_status::S4_REQUEST_TIMEOUT:
            return "Request Timeout";
        case http_status::S4_PAYLOAD_LARGE:
            return "Payload Too Large";
        case http_status::S4_URL_LONG:
            return "URI Too Long";
        case http_status::S4_MANY_REQUESTS:
            return "Too Many Requests";
        case http_status::S5_INTERNAL_ERROR:
            return "Internal Server Error";
        case http_status::S5_BAD_GATEWAY:
            return "Bad Gateway";
        case http_status::S5_SERVICE_UNAVAILABLE:
            return "Service Unavailable";
        case http_status::S5_GATEWAY_TIMEOUT:
            return "Gateway Timeout";
        case http_status::S5_HTTP_VERSION_NOT_SUPPORT:
            return "HTTP Version Not Supported";
        default:
            return "Unknown";
    }
}

http_status http_status_from_code(const uint16_t code) noexcept {
    switch (code) {
        case 100:
            return http_status::S1_CONTINUE;
        case 101:
            return http_status::S1_SWITCH_PROTOCOL;
        case 200:
            return http_status::S2_OK;
        case 201:
            return http_status::S2_CREATED;
        case 204:
            return http_status::S2_NO_CONTENT;
        case 206:
            return http_status::S2_PARTIAL_CONTENT;
        case 301:
            return http_status::S3_MOVED_PERMANENT;
        case 302:
            return http_status::S3_FOUND;
        case 304:
            return http_status::S3_NO_MODIFIED;
        case 307:
            return http_status::S3_TEMPORARY_REDIRECT;
        case 308:
            return http_status::S3_PERMANENT_REDIRECT;
        case 400:
            return http_status::S4_BAD_REQUEST;
        case 401:
            return http_status::S4_UNAUTHORIZED;
        case 403:
            return http_status::S4_FORBIDDEN;
        case 404:
            return http_status::S4_NOT_FOUNT;
        case 405:
            return http_status::S4_METHOD_NOT_ALLOWED;
        case 408:
            return http_status::S4_REQUEST_TIMEOUT;
        case 413:
            return http_status::S4_PAYLOAD_LARGE;
        case 414:
            return http_status::S4_URL_LONG;
        case 429:
            return http_status::S4_MANY_REQUESTS;
        case 500:
            return http_status::S5_INTERNAL_ERROR;
        case 502:
            return http_status::S5_BAD_GATEWAY;
        case 503:
            return http_status::S5_SERVICE_UNAVAILABLE;
        case 504:
            return http_status::S5_GATEWAY_TIMEOUT;
        case 505:
            return http_status::S5_HTTP_VERSION_NOT_SUPPORT;
        default:
            return http_status::S5_INTERNAL_ERROR;
    }
}

const http_content& http_content::HTML_TEXT() {
    static http_content content{"text/html"};
    return content;
}

const http_content& http_content::XML_TEXT() {
    static http_content content{"text/xml"};
    return content;
}

const http_content& http_content::CSS_TEXT() {
    static http_content content{"text/css"};
    return content;
}

const http_content& http_content::PLAIN_TEXT() {
    static http_content content{"text/plain"};
    return content;
}

const http_content& http_content::JSON_APP() {
    static http_content content{"application/json"};
    return content;
}

const http_content& http_content::FORM_APP() {
    static http_content content{"application/x-www-form-urlencoded"};
    return content;
}

const http_content& http_content::JPEG_IMG() {
    static http_content content{"image/jpeg"};
    return content;
}

const http_content& http_content::PNG_IMG() {
    static http_content content{"image/png"};
    return content;
}

const http_content& http_content::BMP_IMG() {
    static http_content content{"image/bmp"};
    return content;
}

const http_content& http_content::WEBP_IMG() {
    static http_content content{"image/webp"};
    return content;
}

const http_content& http_content::HTML_MSG() {
    static http_content content{"message/http"};
    return content;
}


const http_method& http_method::GET() {
    static http_method mth{"GET"};
    return mth;
}

const http_method& http_method::POST() {
    static http_method mth{"POST"};
    return mth;
}

const http_method& http_method::HEAD() {
    static http_method mth{"HEAD"};
    return mth;
}

const http_method& http_method::PUT() {
    static http_method mth{"PUT"};
    return mth;
}

const http_method& http_method::DELETE() {
    static http_method mth{"DELETE"};
    return mth;
}

const http_method& http_method::OPTIONS() {
    static http_method mth{"OPTIONS"};
    return mth;
}

const http_method& http_method::TRACE() {
    static http_method mth{"TRACE"};
    return mth;
}

const http_method& http_method::CONNECT() {
    static http_method mth{"CONNECT"};
    return mth;
}

const http_method& http_method::PATCH() {
    static http_method mth{"PATCH"};
    return mth;
}

const http_method& http_method::DEFAULT() {
    static http_method mth = GET() & POST() & DELETE() & PUT() & OPTIONS();
    return mth;
}


const http_cookie_name& http_cookie_name::JSESSIONID() {
    static http_cookie_name name{"JSESSIONID"};
    return name;
}

const http_cookie_name& http_cookie_name::SESSIONID() {
    static http_cookie_name name{"SESSIONID"};
    return name;
}

const http_cookie_name& http_cookie_name::PHPSESSID() {
    static http_cookie_name name{"PHPSESSID"};
    return name;
}

const http_cookie_name& http_cookie_name::ASPSESSIONID() {
    static http_cookie_name name{"ASP.NET_SessionId"};
    return name;
}


const string& http_key::Access_Control_Allow_Credentials() {
    static string key{"Access-Control-Allow-Credentials"};
    return key;
}

const string& http_key::Access_Control_Allow_Headers() {
    static string key{"Access-Control-Allow-Headers"};
    return key;
}

const string& http_key::Access_Control_Allow_Methods() {
    static string key{"Access-Control-Allow-Methods"};
    return key;
}

const string& http_key::Access_Control_Allow_Origin() {
    static string key{"Access-Control-Allow-Origin"};
    return key;
}

const string& http_key::Access_Control_Max_Age() {
    static string key{"Access-Control-Max-Age"};
    return key;
}

const string& http_key::Connection() {
    static string key{"Connection"};
    return key;
}

const string& http_key::Content_Length() {
    static string key{"Content-Length"};
    return key;
}

const string& http_key::Content_Type() {
    static string key{"Content-Type"};
    return key;
}

const string& http_key::Lax() {
    static string key{"Lax"};
    return key;
}

const string& http_key::Strict() {
    static string key{"Strict"};
    return key;
}

const string& http_key::X_Forwarded_Proto() {
    static string key{"X-Forwarded-Proto"};
    return key;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
