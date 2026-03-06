#include <NeForce/network/http/session.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/string/to_string.hpp>
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_NODISCARD string cookie::to_string() const {
    string result;
    result += name.cookie_name() + "=" + value;

    if (!domain.empty()) result += "; Domain=" + domain;
    if (!path.empty()) result += "; Path=" + path;

    if (max_age >= 0) {
        result += "; Max-Age=" + _NEFORCE to_string(max_age);
    }
    if (expires > datetime::epoch()) {
        result += "; Expires=" + expires.to_string_GMT();
    }

    if (secure) result += "; Secure";
    if (http_only) result += "; HttpOnly";
    if (!same_site.empty()) result += "; SameSite=" + same_site;

    return result;
}

NEFORCE_NODISCARD bool session::expired(int max_inactive) const noexcept {
    if (max_inactive <= 0) max_inactive = max_age;
    const datetime now = datetime::now();
    const int64_t diff = now - last_access;
    return diff > max_inactive;
}

NEFORCE_NODISCARD string session::to_string() const {
    string result;
    result += "Session ID: ["_s + _NEFORCE to_string(id) + "]" + "Data: ";
    result += _NEFORCE to_string(data);
    return result;
}

NEFORCE_END_NAMESPACE__
