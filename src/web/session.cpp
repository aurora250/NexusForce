#include <MSTL/web/session.hpp>
MSTL_BEGIN_NAMESPACE__
#ifdef DELETE
#undef DELETE
#endif

const string HTTP_CONTENT::HTML_TEXT{"text/html"};
const string HTTP_CONTENT::XML_TEXT{"text/xml"};
const string HTTP_CONTENT::CSS_TEXT{"text/css"};
const string HTTP_CONTENT::PLAIN_TEXT{"text/plain"};
const string HTTP_CONTENT::JSON_APP{"application/json"};
const string HTTP_CONTENT::FORM_APP{"application/x-www-form-urlencoded"};
const string HTTP_CONTENT::JPEG_IMG{"image/jpeg"};
const string HTTP_CONTENT::PNG_IMG{"image/png"};
const string HTTP_CONTENT::BMP_IMG{"image/bmp"};
const string HTTP_CONTENT::WEBP_IMG{"image/webp"};
const string HTTP_CONTENT::HTML_MSG{"message/http"};

const HTTP_METHOD HTTP_METHOD::GET{"GET"};
const HTTP_METHOD HTTP_METHOD::POST{"POST"};
const HTTP_METHOD HTTP_METHOD::HEAD{"HEAD"};
const HTTP_METHOD HTTP_METHOD::PUT{"PUT"};
const HTTP_METHOD HTTP_METHOD::DELETE{"DELETE"};
const HTTP_METHOD HTTP_METHOD::OPTIONS{"OPTIONS"};
const HTTP_METHOD HTTP_METHOD::TRACE{"TRACE"};
const HTTP_METHOD HTTP_METHOD::CONNECT{"CONNECT"};

MSTL_END_NAMESPACE__
