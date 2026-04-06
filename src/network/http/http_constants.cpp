#include <NeForce/network/http/http_constants.hpp>
NEFORCE_BEGIN_NAMESPACE__

#ifdef DELETE
#    undef DELETE
#endif

const HTTP_CONTENT& HTTP_CONTENT::HTML_TEXT() {
    static HTTP_CONTENT content{"text/html"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::XML_TEXT() {
    static HTTP_CONTENT content{"text/xml"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::CSS_TEXT() {
    static HTTP_CONTENT content{"text/css"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::PLAIN_TEXT() {
    static HTTP_CONTENT content{"text/plain"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::JSON_APP() {
    static HTTP_CONTENT content{"application/json"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::FORM_APP() {
    static HTTP_CONTENT content{"application/x-www-form-urlencoded"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::JPEG_IMG() {
    static HTTP_CONTENT content{"image/jpeg"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::PNG_IMG() {
    static HTTP_CONTENT content{"image/png"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::BMP_IMG() {
    static HTTP_CONTENT content{"image/bmp"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::WEBP_IMG() {
    static HTTP_CONTENT content{"image/webp"};
    return content;
}

const HTTP_CONTENT& HTTP_CONTENT::HTML_MSG() {
    static HTTP_CONTENT content{"message/http"};
    return content;
}


const HTTP_METHOD& HTTP_METHOD::GET() {
    static HTTP_METHOD mth{"GET"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::POST() {
    static HTTP_METHOD mth{"POST"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::HEAD() {
    static HTTP_METHOD mth{"HEAD"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::PUT() {
    static HTTP_METHOD mth{"PUT"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::DELETE() {
    static HTTP_METHOD mth{"DELETE"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::OPTIONS() {
    static HTTP_METHOD mth{"OPTIONS"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::TRACE() {
    static HTTP_METHOD mth{"TRACE"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::CONNECT() {
    static HTTP_METHOD mth{"CONNECT"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::PATCH() {
    static HTTP_METHOD mth{"PATCH"};
    return mth;
}

const HTTP_METHOD& HTTP_METHOD::DEFAULT() {
    static HTTP_METHOD mth = GET() & POST() & DELETE() & PUT() & OPTIONS();
    return mth;
}


const HTTP_COOKIE_NAME& HTTP_COOKIE_NAME::JSESSIONID() {
    static HTTP_COOKIE_NAME name{"JSESSIONID"};
    return name;
}

const HTTP_COOKIE_NAME& HTTP_COOKIE_NAME::SESSIONID() {
    static HTTP_COOKIE_NAME name{"SESSIONID"};
    return name;
}

const HTTP_COOKIE_NAME& HTTP_COOKIE_NAME::PHPSESSID() {
    static HTTP_COOKIE_NAME name{"PHPSESSID"};
    return name;
}

const HTTP_COOKIE_NAME& HTTP_COOKIE_NAME::ASPSESSIONID() {
    static HTTP_COOKIE_NAME name{"ASP.NET_SessionId"};
    return name;
}


const string& HTTP_KEY::Access_Control_Allow_Credentials() {
    static string key{"Access-Control-Allow-Credentials"};
    return key;
}

const string& HTTP_KEY::Access_Control_Allow_Headers() {
    static string key{"Access-Control-Allow-Headers"};
    return key;
}

const string& HTTP_KEY::Access_Control_Allow_Methods() {
    static string key{"Access-Control-Allow-Methods"};
    return key;
}

const string& HTTP_KEY::Access_Control_Allow_Origin() {
    static string key{"Access-Control-Allow-Origin"};
    return key;
}

const string& HTTP_KEY::Access_Control_Max_Age() {
    static string key{"Access-Control-Max-Age"};
    return key;
}

const string& HTTP_KEY::Connection() {
    static string key{"Connection"};
    return key;
}

const string& HTTP_KEY::Content_Length() {
    static string key{"Content-Length"};
    return key;
}

const string& HTTP_KEY::Content_Type() {
    static string key{"Content-Type"};
    return key;
}

const string& HTTP_KEY::Lax() {
    static string key{"Lax"};
    return key;
}

const string& HTTP_KEY::Strict() {
    static string key{"Strict"};
    return key;
}

const string& HTTP_KEY::X_Forwarded_Proto() {
    static string key{"X-Forwarded-Proto"};
    return key;
}

NEFORCE_END_NAMESPACE__
