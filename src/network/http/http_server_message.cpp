#include <NeForce/network/http/http_server_message.hpp>
#include <NeForce/network/util/url.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    void parse_cookies(const string_view cookie_header, http_request& request) {
        if (cookie_header.empty()) {
            return;
        }

        size_t start = 0;
        while (start < cookie_header.length()) {
            const size_t end = cookie_header.find(';', start);
            const size_t pair_end = (end == string::npos) ? cookie_header.length() : end;

            const auto pair = cookie_header.view(start, pair_end - start).trim();
            const size_t eq_pos = pair.find('=');

            if (eq_pos != string::npos) {
                request.set_cookie(pair.head(eq_pos).trim(), pair.tail(eq_pos + 1).trim());
            }

            if (end == string::npos) {
                break;
            }
            start = end + 1;
        }
    }
} // namespace


string_view http_request::client_ip() const noexcept {
    const auto xff = header("X-Forwarded-For");
    if (!xff.empty()) {
        const auto comma = xff.find(',');
        return comma != string::npos ? xff.view(0, comma) : xff;
    }
    const auto xri = header("X-Real-IP");
    if (!xri.empty()) {
        return xri;
    }
    return "";
}

void http_request::clear() {
    method = http_method::GET();
    path = "/";
    version = "HTTP/1.1";
    query.clear();
    body.clear();
    headers.clear();
    cookies.clear();
    parameters.clear();
    session = nullptr;
}

http_request http_request::parse(const string_view str) {
    http_request request;
    size_t pos = 0;
    string_view line;

    if (getline(str, pos, line)) {
        line = line.trim();
        const size_t pos1 = line.find(' ');
        if (pos1 == string::npos) {
            NEFORCE_THROW_EXCEPTION(http_exception("Invalid request line"));
        }
        request.method = line.view(0, pos1);

        const size_t pos2 = line.find(' ', pos1 + 1);
        if (pos2 == string::npos) {
            NEFORCE_THROW_EXCEPTION(http_exception("Invalid request line"));
        }
        request.path = line.view(pos1 + 1, pos2 - pos1 - 1);
        request.version = line.view(pos2 + 1);
    }

    const size_t query_pos = request.path.find('?');
    if (query_pos != string::npos) {
        request.query = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);
    }

    while (getline(str, pos, line)) {
        line = line.trim();
        if (line.empty()) {
            break;
        }

        const size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            string key = line.view(0, colon_pos).trim();
            string value = line.view(colon_pos + 1).trim();
            request.set_header(key, _NEFORCE move(value));
        }
    }

    const size_t body_start = str.find("\r\n\r\n");
    if (body_start != string_view::npos && body_start + 4 < str.size()) {
        request.body = str.substr(body_start + 4);
    }

    const auto cookie_str = request.header("Cookie");
    if (!cookie_str.empty()) {
        parse_cookies(cookie_str, request);
    }

    return request;
}

string http_request::to_string() const {
    string result;
    result.reserve(512 + body.size());

    string full_path = path;
    if (!query.empty()) {
        full_path += "?" + query;
    }
    result += method.to_string() + " " + full_path + " " + version + "\r\n";

    for (const auto& [key, value]: headers) {
        result += key + ": " + value + "\r\n";
    }

    if (!cookies.empty()) {
        result += "Cookie: ";
        bool first = true;
        for (const auto& [name, value]: cookies) {
            if (!first) {
                result += "; ";
            }
            result += name + "=" + value;
            first = false;
        }
        result += "\r\n";
    }

    if (!body.empty() && !has_header(http_key::Content_Length())) {
        result += http_key::Content_Length() + ": " + _NEFORCE to_string(body.size()) + "\r\n";
    }

    result += "\r\n";
    result += body;
    return result;
}

string http_response::to_string() const {
    string result;
    result.reserve(1024 + body.size());

    if (!redirect_url.empty()) {
        result += version + " 302 Found\r\n";
        result += "Location: " + redirect_url + "\r\n";
    } else {
        result += version + " " + _NEFORCE to_string(static_cast<uint16_t>(status)) + " " + status_message + "\r\n";
    }

    for (const auto& cookie: cookies) {
        result += "Set-Cookie: " + cookie.to_string() + "\r\n";
    }

    if (redirect_url.empty() && !has_header(http_key::Content_Length())) {
        result += http_key::Content_Length() + ": " + _NEFORCE to_string(body.size()) + "\r\n";
    }

    for (const auto& [key, value]: headers) {
        result += key + ": " + value + "\r\n";
    }
    result += "\r\n";

    if (redirect_url.empty()) {
        result += body;
    }
    return result;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
