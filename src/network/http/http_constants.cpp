#include <NeForce/network/http/http_constants.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

string http_status_message(const http_status status) {
    switch (status) {
        case http_status::continue_:
            return "Continue";
        case http_status::switching_protocols:
            return "Switching Protocols";
        case http_status::ok:
            return "OK";
        case http_status::created:
            return "Created";
        case http_status::accepted:
            return "Accepted";
        case http_status::no_content:
            return "No Content";
        case http_status::partial_content:
            return "Partial Content";
        case http_status::moved_permanent:
            return "Moved Permanently";
        case http_status::found:
            return "Found";
        case http_status::see_other:
            return "See Other";
        case http_status::not_modified:
            return "Not Modified";
        case http_status::temporary_redirect:
            return "Temporary Redirect";
        case http_status::permanent_redirect:
            return "Permanent Redirect";
        case http_status::bad_request:
            return "Bad Request";
        case http_status::unauthorized:
            return "Unauthorized";
        case http_status::forbidden:
            return "Forbidden";
        case http_status::not_found:
            return "Not Found";
        case http_status::method_not_allowed:
            return "Method Not Allowed";
        case http_status::not_acceptable:
            return "Not Acceptable";
        case http_status::proxy_auth_required:
            return "Proxy Authentication Required";
        case http_status::request_timeout:
            return "Request Timeout";
        case http_status::conflict:
            return "Conflict";
        case http_status::gone:
            return "Gone";
        case http_status::length_required:
            return "Length Required";
        case http_status::precondition_failed:
            return "Precondition Failed";
        case http_status::payload_too_large:
            return "Payload Too Large";
        case http_status::uri_too_long:
            return "URI Too Long";
        case http_status::unsupported_media_type:
            return "Unsupported Media Type";
        case http_status::range_not_satisfiable:
            return "Range Not Satisfiable";
        case http_status::expectation_failed:
            return "Expectation Failed";
        case http_status::unprocessable_content:
            return "Unprocessable Content";
        case http_status::upgrade_required:
            return "Upgrade Required";
        case http_status::too_many_requests:
            return "Too Many Requests";
        case http_status::header_fields_too_large:
            return "Request Header Fields Too Large";
        case http_status::unavailable_for_legal_reasons:
            return "Unavailable For Legal Reasons";
        case http_status::internal_server_error:
            return "Internal Server Error";
        case http_status::not_implemented:
            return "Not Implemented";
        case http_status::bad_gateway:
            return "Bad Gateway";
        case http_status::service_unavailable:
            return "Service Unavailable";
        case http_status::gateway_timeout:
            return "Gateway Timeout";
        case http_status::http_version_not_supported:
            return "HTTP Version Not Supported";
        default:
            return "Unknown";
    }
}

http_status http_status_from_code(const uint16_t code) noexcept {
    switch (code) {
        case 100:
            return http_status::continue_;
        case 101:
            return http_status::switching_protocols;
        case 200:
            return http_status::ok;
        case 201:
            return http_status::created;
        case 202:
            return http_status::accepted;
        case 204:
            return http_status::no_content;
        case 206:
            return http_status::partial_content;
        case 301:
            return http_status::moved_permanent;
        case 302:
            return http_status::found;
        case 303:
            return http_status::see_other;
        case 304:
            return http_status::not_modified;
        case 307:
            return http_status::temporary_redirect;
        case 308:
            return http_status::permanent_redirect;
        case 400:
            return http_status::bad_request;
        case 401:
            return http_status::unauthorized;
        case 403:
            return http_status::forbidden;
        case 404:
            return http_status::not_found;
        case 405:
            return http_status::method_not_allowed;
        case 406:
            return http_status::not_acceptable;
        case 407:
            return http_status::proxy_auth_required;
        case 408:
            return http_status::request_timeout;
        case 409:
            return http_status::conflict;
        case 410:
            return http_status::gone;
        case 411:
            return http_status::length_required;
        case 412:
            return http_status::precondition_failed;
        case 413:
            return http_status::payload_too_large;
        case 414:
            return http_status::uri_too_long;
        case 415:
            return http_status::unsupported_media_type;
        case 416:
            return http_status::range_not_satisfiable;
        case 417:
            return http_status::expectation_failed;
        case 422:
            return http_status::unprocessable_content;
        case 426:
            return http_status::upgrade_required;
        case 429:
            return http_status::too_many_requests;
        case 431:
            return http_status::header_fields_too_large;
        case 451:
            return http_status::unavailable_for_legal_reasons;
        case 500:
            return http_status::internal_server_error;
        case 501:
            return http_status::not_implemented;
        case 502:
            return http_status::bad_gateway;
        case 503:
            return http_status::service_unavailable;
        case 504:
            return http_status::gateway_timeout;
        case 505:
            return http_status::http_version_not_supported;
        default: {
            if (code >= 100 && code < 200) {
                return http_status::continue_;
            }
            if (code >= 200 && code < 300) {
                return http_status::ok;
            }
            if (code >= 300 && code < 400) {
                return http_status::found;
            }
            if (code >= 400 && code < 500) {
                return http_status::bad_request;
            }
            if (code >= 500 && code < 600) {
                return http_status::internal_server_error;
            }
            return http_status::internal_server_error;
        }
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

const string& http_key::Strict_Transport_Security() {
    static string key{"Strict-Transport-Security"};
    return key;
}

const string& http_key::X_Frame_Options() {
    static string key{"X-Frame-Options"};
    return key;
}

const string& http_key::X_Content_Type_Options() {
    static string key{"X-Content-Type-Options"};
    return key;
}

const string& http_key::Content_Security_Policy() {
    static string key{"Content-Security-Policy"};
    return key;
}

const string& http_key::X_XSS_Protection() {
    static string key{"X-XSS-Protection"};
    return key;
}

const string& http_key::Referrer_Policy() {
    static string key{"Referrer-Policy"};
    return key;
}

const string& http_key::Permissions_Policy() {
    static string key{"Permissions-Policy"};
    return key;
}

NEFORCE_END_HTTP__

string http_error_category::message(const int32_t ev) const {
    return http::http_status_message(static_cast<http::http_status>(ev));
}

const error_category& http_category() noexcept {
    static http_error_category instance;
    return instance;
}

NEFORCE_END_NAMESPACE__
