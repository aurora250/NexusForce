#include <NeForce/core/numeric/random.hpp>
#include <NeForce/network/http/csrf_filter.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    bool is_state_changing(const http_method& method) {
        return method.is_post() || method.is_put() || method.is_patch() || method.is_delete();
    }

    string generate_token() {
        string token;
        token.reserve(64);
        for (int i = 0; i < 64; ++i) {
            token += format("{:x}", secret::next_int<uint32_t>(16));
        }
        return token;
    }
} // namespace


bool csrf_filter::pre_filter(http_request& request, http_response& response) {
    if (!enabled) {
        return true;
    }

    // Use session-based storage when available (prevents subdomain cookie injection),
    // fall back to double-submit cookie pattern when no session exists.
    const string session_key = "_csrf_token";
    bool has_session = (request.session != nullptr);

    string csrf_token;
    if (has_session && !request.session->get(session_key).empty()) {
        csrf_token = string(request.session->get(session_key));
    } else {
        // Check cookie for existing token (double-submit fallback)
        const auto cookie_val = request.cookie(cookie_name);
        if (!cookie_val.empty()) {
            csrf_token = string(cookie_val);
        } else {
            csrf_token = generate_token();
        }
        if (has_session) {
            request.session->set(session_key, csrf_token);
        }
    }

    {
        http_cookie csrf_cookie;
        csrf_cookie.name = cookie_name;
        csrf_cookie.value = csrf_token;
        csrf_cookie.http_only = false;
        csrf_cookie.same_site = http_key::Strict();
        csrf_cookie.max_age = token_max_age;
        csrf_cookie.path = "/";
        response.cookies.emplace_back(move(csrf_cookie));
    }

    if (!is_state_changing(request.method)) {
        return true;
    }

    string request_token;
    const auto header_val = request.header(header_name);
    if (!header_val.empty()) {
        request_token = string(header_val);
    } else {
        const auto it = request.form_data.find(form_field_name);
        if (it != request.form_data.end()) {
            request_token = it->second;
        }
    }

    if (request_token.empty() || request_token != csrf_token) {
        response.status = http_status::S4_FORBIDDEN;
        response.status_message = "Forbidden";
        response.set_content_type(http_content::PLAIN_TEXT());
        response.body = "CSRF token validation failed";
        return false;
    }

    return true;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
