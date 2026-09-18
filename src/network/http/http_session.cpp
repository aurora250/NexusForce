#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/network/http/http_session.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

http_cookie http_cookie::parse(const string_view header) { return parse(header, {}, {}); }

http_cookie http_cookie::parse(const string_view header, string default_domain, string default_path) {
    http_cookie c{};
    if (header.empty()) {
        return c;
    }

    size_t start = 0;
    bool first = true;

    while (start < header.size()) {
        const size_t end = header.find(';', start);
        const size_t pair_end = (end == string_view::npos) ? header.size() : end;
        const auto pair = header.view(start, pair_end - start).trim();
        const size_t eq_pos = pair.find('=');

        if (first) {
            first = false;
            if (eq_pos != string_view::npos) {
                c.name = http_cookie_name(string(pair.head(eq_pos).trim()));
                c.value = string(pair.tail(eq_pos + 1).trim());
            }
            c.domain = default_domain.empty() ? "" : move(default_domain);
            c.path = default_path.empty() ? "/" : move(default_path);
        } else {
            const string_view attr_name = (eq_pos != string_view::npos) ? pair.view(0, eq_pos).trim() : pair;
            const string_view attr_val = (eq_pos != string_view::npos) ? pair.view(eq_pos + 1).trim() : string_view{};

            auto attr_lower = string(attr_name);
            attr_lower.lowercase();

            if (attr_lower == "httponly") {
                c.http_only = true;
            } else if (attr_lower == "secure") {
                c.secure = true;
            } else if (attr_lower == "path") {
                c.path = string(attr_val);
            } else if (attr_lower == "domain") {
                c.domain = string(attr_val);
            } else if (attr_lower == "samesite") {
                c.same_site = string(attr_val);
            } else if (attr_lower == "max-age") {
                try {
                    c.max_age = seconds{integer32::parse(attr_val).value()};
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (const exception& e) {
                    NEFORCE_REPORT_EXCEPTION(e);
                }
            } else if (attr_lower == "expires") {
                try {
                    c.expires = datetime::parse_RFC1123(attr_val);
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (const exception& e) {
                    NEFORCE_REPORT_EXCEPTION(e);
                }
            }
        }

        if (end == string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return c;
}

NEFORCE_NODISCARD string http_cookie::to_string() const {
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

    if (max_age >= 0_s) {
        result += "; Max-Age=" + _NEFORCE to_string(max_age.count());
    }

    if (expires > datetime::epoch()) {
        result += "; Expires=" + expires.to_RFC1123();
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

bool http_cookie::is_valid() const noexcept {
    if (name.cookie_name().empty()) {
        return false;
    }

    if (max_age == 0_s) {
        return false;
    }

    if (max_age > 0_s && expires > datetime::epoch() && expires < datetime::now()) {
        return false;
    }

    if (max_age <= 0_s && expires > datetime::epoch() && expires < datetime::now()) {
        return false;
    }

    return true;
}

bool http_cookie::is_expired() const noexcept {
    if (max_age == 0_s) {
        return true;
    }
    if (max_age > 0_s && expires > datetime::epoch()) {
        return expires < datetime::now();
    }
    if (expires <= datetime::epoch()) {
        return false;
    }
    return expires < datetime::now();
}

void http_cookie::set_expires_from_now(const seconds sec) {
    if (sec <= 0_s) {
        expires = datetime::epoch();
        return;
    }
    expires = datetime::now() + sec.count();
}

string& http_session::operator[](const string& key) {
    lock<mutex> lk(*mtx_);
    touch_locked();
    return data[key];
}

string_view http_session::get(const string& key) const {
    lock<mutex> lk(*mtx_);
    const auto it = data.find(key);
    if (it != data.end()) {
        return it->second.view();
    }
    return "";
}

void http_session::set(const string& key, string value) {
    lock<mutex> lk(*mtx_);
    touch_locked();
    data[key] = move(value);
}

bool http_session::remove(const string& key) {
    lock<mutex> lk(*mtx_);
    touch_locked();
    return data.erase(key) > 0;
}

void http_session::clear() {
    lock<mutex> lk(*mtx_);
    touch_locked();
    data.clear();
}

void http_session::invalidate() noexcept {
    lock<mutex> lk(*mtx_);
    invalidated = true;
    data.clear();
}

void http_session::regenerate_id() {
    lock<mutex> lk(*mtx_);
    string new_id;
    new_id.reserve(32);
    for (int i = 0; i < 32; ++i) {
        new_id += format("{:x}", secret::next_int<uint32_t>(16));
    }
    id = move(new_id);
    touch_locked();
    is_new = true;
}

void http_session::touch() noexcept {
    lock<mutex> lk(*mtx_);
    touch_locked();
}

bool http_session::contains(const string& key) const noexcept {
    lock<mutex> lk(*mtx_);
    return data.find(key) != data.end();
}

bool http_session::is_valid() const noexcept {
    lock<mutex> lk(*mtx_);
    return is_valid_locked();
}

bool http_session::expired(seconds max_inactive) const noexcept {
    lock<mutex> lk(*mtx_);
    return expired_locked(max_inactive);
}

seconds http_session::age() const noexcept {
    lock<mutex> lk(*mtx_);
    return seconds{datetime::now() - create_time};
}

seconds http_session::idle_time() const noexcept {
    lock<mutex> lk(*mtx_);
    return seconds{datetime::now() - last_access};
}

string http_session::session_id() const {
    lock<mutex> lk(*mtx_);
    return id;
}

bool http_session::is_new_session() const noexcept {
    lock<mutex> lk(*mtx_);
    return is_new;
}

flat_unordered_map<string, string> http_session::data_snapshot() const {
    lock<mutex> lk(*mtx_);
    return data;
}

void http_session::set_max_age(const seconds age) noexcept {
    lock<mutex> lk(*mtx_);
    max_age = age;
}

void http_session::set_last_access(const datetime time) noexcept {
    lock<mutex> lk(*mtx_);
    last_access = time;
}

http_session http_session::clone() const {
    lock<mutex> lk(*mtx_);
    http_session copy;
    copy.id = id;
    copy.data = data;
    copy.last_access = last_access;
    copy.create_time = create_time;
    copy.max_age = max_age;
    copy.is_new = is_new;
    copy.invalidated = invalidated;
    return copy;
}

string http_session::to_string() const {
    string result;
    result.reserve(256);

    result += "Session ID: [" + id + "]\n";
    result += "Created: " + create_time.to_string() + "\n";
    result += "Last Access: " + last_access.to_string() + "\n";
    const seconds age{datetime::now() - last_access};
    result += "Age: " + _NEFORCE to_string(age.count()) + "s\n";
    const seconds idle{datetime::now() - last_access};
    result += "Idle: " + _NEFORCE to_string(idle.count()) + "s\n";
    result += "Max Age: " + _NEFORCE to_string(max_age.count()) + "s\n";
    result += "Is New: " + _NEFORCE to_string(is_new) + "\n";
    result += "Valid: " + _NEFORCE to_string(is_valid_locked()) + "\n";
    result += "Data Count: " + _NEFORCE to_string(data.size()) + "\n";

    if (!data.empty()) {
        result += "Data:\n";
        for (const auto& pair: data) {
            const string key = pair.first;
            const string value = pair.second;
            result += "  " + move(key) + " = " + move(value) + "\n";
        }
    }

    return result;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
