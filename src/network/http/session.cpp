#include <NeForce/network/http/session.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/string/to_string.hpp>
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_NODISCARD string cookie::to_string() const {
    if (name.cookie_name().empty()) {
        return "";
    }

    string result;
    result.reserve(256);

    result += name.cookie_name() + "=" + value;

    if (!domain.empty()) {
        result += "; Domain=" + domain;
    }

    if (!path.empty()) {
        result += "; Path=" + path;
    }

    if (max_age >= 0) {
        result += "; Max-Age=" + _NEFORCE to_string(max_age);
    }

    if (expires > datetime::epoch()) {
        result += "; Expires=" + expires.to_string_GMT();
    }

    if (secure) {
        result += "; Secure";
    }

    if (http_only) {
        result += "; HttpOnly";
    }

    if (!same_site.empty()) {
        result += "; SameSite=" + same_site;
    }

    return result;
}

bool cookie::is_valid() const noexcept {
    if (name.cookie_name().empty()) {
        return false;
    }

    if (expires > datetime::epoch() && expires < datetime::now()) {
        return false;
    }

    return true;
}

bool cookie::is_expired() const noexcept {
    if (expires <= datetime::epoch()) {
        return false;
    }
    return expires < datetime::now();
}

void cookie::set_expires_from_now(const int64_t seconds) {
    if (seconds <= 0) {
        expires = datetime::epoch();
        return;
    }
    expires = datetime::now() + seconds;
}

string& session::operator [](const string& key) {
    touch();
    return data[key];
}

string_view session::get(const string& key) const {
    const auto it = data.find(key);
    if (it != data.end()) {
        return it->second.view();
    }
    return "";
}

void session::set(const string& key, string value) {
    touch();
    data[key] = _NEFORCE move(value);
}

bool session::remove(const string& key) {
    touch();
    return data.erase(key) > 0;
}

void session::clear() {
    touch();
    data.clear();
}

void session::invalidate() noexcept {
    invalidated = true;
    data.clear();
}

void session::touch() noexcept {
    last_access = datetime::now();
    is_new = false;
}

bool session::contains(const string& key) const noexcept {
    return data.find(key) != data.end();
}

bool session::is_valid() const noexcept {
    if (invalidated) {
        return false;
    }

    if (id.empty()) {
        return false;
    }

    return !expired();
}

bool session::expired(int max_inactive) const noexcept {
    if (max_inactive <= 0) {
        max_inactive = max_age;
    }

    if (max_inactive <= 0) {
        return false;
    }

    const int64_t idle = idle_time();
    return idle > max_inactive;
}

string session::to_string() const {
    string result;
    result.reserve(256);

    result += "Session ID: [" + id + "]\n";
    result += "Created: " + create_time.to_string() + "\n";
    result += "Last Access: " + last_access.to_string() + "\n";
    result += "Age: " + _NEFORCE to_string(age()) + "s\n";
    result += "Idle: " + _NEFORCE to_string(idle_time()) + "s\n";
    result += "Max Age: " + _NEFORCE to_string(max_age) + "s\n";
    result += "Is New: " + _NEFORCE to_string(is_new) + "\n";
    result += "Valid: " + _NEFORCE to_string(is_valid()) + "\n";
    result += "Data Count: " + _NEFORCE to_string(data.size()) + "\n";

    if (!data.empty()) {
        result += "Data:\n";
        for (const auto& [key, value] : data) {
            result += "  " + key + " = " + value + "\n";
        }
    }

    return result;
}

NEFORCE_END_NAMESPACE__
