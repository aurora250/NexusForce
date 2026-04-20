#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/util/url.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    bool should_encode(const char c, const bool encode_slash) noexcept {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            return false;
        }
        if (c == '-' || c == '_' || c == '.' || c == '~') {
            return false;
        }
        if (!encode_slash && c == '/') {
            return false;
        }
        return true;
    }
} // namespace


bool url::is_valid() const noexcept {
    if (scheme.empty() || host.empty()) {
        return false;
    }
    if (path.empty()) {
        return false;
    }
    if (!path.starts_with('/')) {
        return false;
    }
    return true;
}

url url::parse(const string_view str) {
    if (str.empty()) {
        NEFORCE_THROW_EXCEPTION(network_exception("URL is empty"));
    }

    url target{};
    const size_t len = str.size();

    const auto scheme_end = str.find("://");
    if (scheme_end == string::npos) {
        NEFORCE_THROW_EXCEPTION(network_exception("URL missing scheme"));
    }
    target.scheme = str.head(scheme_end);
    size_t pos = scheme_end + 3;
    if (pos >= len) {
        NEFORCE_THROW_EXCEPTION(network_exception("URL scheme invalid"));
    }

    const size_t path_pos = str.find('/', pos);
    const size_t query_pos = str.find('?', pos);
    const size_t fragment_pos = str.find('#', pos);

    size_t host_end = len;
    if (path_pos != string::npos) {
        host_end = min(host_end, path_pos);
    }
    if (query_pos != string::npos) {
        host_end = min(host_end, query_pos);
    }
    if (fragment_pos != string::npos) {
        host_end = min(host_end, fragment_pos);
    }

    const string_view host_port = str.substr(pos, host_end - pos);
    size_t colon_pos = string::npos;

    if (host_port.starts_with('[')) {
        const size_t bracket_end = host_port.find(']');
        if (bracket_end == string::npos) {
            NEFORCE_THROW_EXCEPTION(network_exception("URL malformed: unclosed IPv6 address"));
        }
        target.host = host_port.substr(1, bracket_end - 1);
        if (bracket_end + 1 < host_port.size() && host_port[bracket_end + 1] == ':') {
            colon_pos = bracket_end + 1;
        }
    } else {
        colon_pos = host_port.find_last_of(':');
        if (colon_pos != string::npos) {
            target.host = host_port.head(colon_pos);
        } else {
            target.host = host_port;
        }
    }

    if (colon_pos != string::npos && colon_pos < host_port.size() - 1) {
        target.port = ports::parse(host_port.substr(colon_pos + 1));
        if (!target.port) {
            NEFORCE_THROW_EXCEPTION(network_exception("URL invalid port"));
        }
    } else {
        target.port = ports::parse(target.scheme.view());
    }

    if (target.host.empty()) {
        NEFORCE_THROW_EXCEPTION(network_exception("URL invalid host"));
    }

    pos = host_end;

    size_t path_end = len;
    if (query_pos != string::npos) {
        path_end = min(path_end, query_pos);
    }
    if (fragment_pos != string::npos) {
        path_end = min(path_end, fragment_pos);
    }

    if (pos < path_end && str[pos] == '/') {
        target.path = str.substr(pos, path_end - pos);
    } else {
        target.path = "/";
    }

    if (query_pos != string::npos && query_pos < len - 1) {
        const size_t query_end = fragment_pos != string::npos ? fragment_pos : len;
        target.query = str.substr(query_pos + 1, query_end - query_pos - 1);
    }

    if (fragment_pos != string::npos && fragment_pos < len - 1) {
        target.fragment = str.tail(fragment_pos + 1);
    }

    return target;
}

string url::to_string() const {
    string ret = scheme + "://" + host;

    bool show_port = static_cast<bool>(port);
    if (show_port) {
        const ports dport = ports::parse(scheme.view());
        if (!dport && port == dport) {
            show_port = false;
        }
    }

    if (show_port) {
        ret += ":" + _NEFORCE to_string(static_cast<uint16_t>(port));
    }
    ret += path.empty() ? "/" : path;

    if (!query.empty()) {
        ret += "?" + query;
    }
    if (!fragment.empty()) {
        ret += "#" + fragment;
    }

    return ret;
}

string url::encode(const string_view str, const bool encode_slash) {
    string result;
    result.reserve(str.size() * 3);

    for (const char c: str) {
        if (should_encode(c, encode_slash)) {
            result += '%';
            const auto uc = static_cast<unsigned char>(c);
            constexpr string_view hex{"0123456789ABCDEF"};
            result += hex[uc >> 4];
            result += hex[uc & 0x0F];
        } else {
            result += c;
        }
    }

    return result;
}

optional<string> url::decode(const string_view str) {
    string result;
    result.reserve(str.size());

    size_t i = 0;
    while (i < str.size()) {
        const char c = str[i];

        if (c == '%') {
            if (i + 2 >= str.size()) {
                return none;
            }

            const auto xpair = hexadecimal::xdigit_value(str[i + 1], str[i + 2]);
            if (!xpair.first) {
                return none;
            }

            result += static_cast<char>(xpair.second);
            i += 3;
        } else if (c == '+') {
            result += ' ';
            ++i;
        } else {
            result += c;
            ++i;
        }
    }

    return result;
}

string url::encode_form(const string_view str) {
    string result;
    result.reserve(str.size() * 3);
    for (const char c: str) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += '+';
        } else {
            result += '%';
            const auto uc = static_cast<unsigned char>(c);
            constexpr string_view hex{"0123456789ABCDEF"};
            result += hex[uc >> 4];
            result += hex[uc & 0x0F];
        }
    }
    return result;
}

string url::decode_tolerant(const string_view str) {
    auto result = url::decode(str);
    if (result) {
        return move(*result);
    }

    string fallback;
    fallback.reserve(str.size());

    size_t i = 0;
    while (i < str.size()) {
        const char c = str[i];

        if (c == '%' && i + 2 < str.size()) {
            auto decoded = url::decode(str.substr(i, 3));
            if (decoded && decoded->size() == 1) {
                fallback += (*decoded)[0];
                i += 3;
                continue;
            }
        }

        if (c == '+') {
            fallback += ' ';
        } else {
            fallback += c;
        }
        ++i;
    }

    return fallback;
}

void url::parse_query(const string_view query, unordered_map<string, string>& params) {
    if (query.empty()) {
        return;
    }

    size_t start = 0;
    while (start < query.size()) {
        const size_t end = query.find('&', start);
        const size_t pair_end = (end == string_view::npos) ? query.size() : end;
        const auto pair = query.view(start, pair_end - start);
        const size_t eq_pos = pair.find('=');

        if (eq_pos != string_view::npos) {
            params[url::decode_tolerant(pair.view(0, eq_pos))] = url::decode_tolerant(pair.view(eq_pos + 1));
        }

        if (end == string_view::npos) {
            break;
        }
        start = end + 1;
    }
}

string url::build_query(const unordered_map<string, string>& params) {
    if (params.empty()) {
        return "";
    }

    string result;
    bool first = true;
    for (const auto& [key, value]: params) {
        if (!first) {
            result += '&';
        }
        result += url::encode_form(key.view()) + '=' + url::encode_form(value.view());
        first = false;
    }
    return result;
}

NEFORCE_END_NAMESPACE__
