#include <NeForce/network/http/http_security.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

void security_headers_filter::post_filter(http_request& /*request*/, http_response& response) {
    if (enable_hsts) {
        string hsts = "max-age=" + to_string(hsts_max_age.count());
        if (hsts_include_subdomains) {
            hsts += "; includeSubDomains";
        }
        if (hsts_preload) {
            hsts += "; preload";
        }
        response.set_header(http_key::Strict_Transport_Security(), move(hsts));
    }

    if (enable_frame_options) {
        response.set_header(http_key::X_Frame_Options(), frame_option_value);
    }

    if (enable_content_type_options) {
        response.set_header(http_key::X_Content_Type_Options(), "nosniff");
    }

    if (enable_csp && !csp_value.empty()) {
        response.set_header(http_key::Content_Security_Policy(), csp_value);
    }

    if (enable_xss_protection) {
        response.set_header(http_key::X_XSS_Protection(), xss_protection_value);
    }

    if (enable_referrer_policy) {
        response.set_header(http_key::Referrer_Policy(), referrer_policy_value);
    }

    if (enable_permissions_policy) {
        response.set_header(http_key::Permissions_Policy(), permissions_policy_value);
    }
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
