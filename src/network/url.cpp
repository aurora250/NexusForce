#include <NeForce/core/utility/hexadecimal.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/url.hpp>
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
}


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
    target.scheme = str.substr(0, scheme_end);
    size_t pos = scheme_end + 3;
    if (pos >= len) {
        NEFORCE_THROW_EXCEPTION(network_exception("URL scheme invalid"));
    }

    size_t at_pos = str.find('@', pos);
    if (at_pos != string::npos && at_pos < str.find('/', pos)) {
        at_pos = pos;
    }

    const size_t path_pos = str.find('/', pos);
    const size_t query_pos = str.find('?', pos);
    const size_t fragment_pos = str.find('#', pos);

    size_t host_end = len;
    if (path_pos != string::npos) host_end = min(host_end, path_pos);
    if (query_pos != string::npos) host_end = min(host_end, query_pos);
    if (fragment_pos != string::npos) host_end = min(host_end, fragment_pos);

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
            target.host = host_port.substr(0, colon_pos);
        } else {
            target.host = host_port;
        }
    }

    if (colon_pos != string::npos && colon_pos < host_port.size() - 1) {
        string_view port_str = host_port.substr(colon_pos + 1);
        try {
            target.port = uinteger16::parse(port_str);
        } catch (...) {
            NEFORCE_THROW_EXCEPTION(network_exception("URL invalid port"));
        }
        if (target.port == 0) {
            NEFORCE_THROW_EXCEPTION(network_exception("URL invalid port"));
        }
    } else {
        target.port = default_port(target.scheme.view());
    }

    if (target.host.empty()) {
        NEFORCE_THROW_EXCEPTION(network_exception("URL invalid host"));
    }

    pos = host_end;

    size_t path_end = len;
    if (query_pos != string::npos) path_end = min(path_end, query_pos);
    if (fragment_pos != string::npos) path_end = min(path_end, fragment_pos);

    if (pos < path_end && str[pos] == '/') {
        target.path = str.substr(pos, path_end - pos);
    } else {
        target.path = "/";
    }

    if (query_pos != string::npos && query_pos < len - 1) {
        size_t query_end = fragment_pos != string::npos ? fragment_pos : len;
        target.query = str.substr(query_pos + 1, query_end - query_pos - 1);
    }

    if (fragment_pos != string::npos && fragment_pos < len - 1) {
        target.fragment = str.substr(fragment_pos + 1);
    }

    return target;
}

string url::to_string() const {
    string ret = scheme + "://" + host;

    bool show_port = (port != 0);
    if (show_port) {
        const uint16_t dport = default_port(scheme.view());
        if (dport != 0 && port == dport) {
            show_port = false;
        }
    }

    if (show_port) {
        ret += ":" + _NEFORCE to_string(port);
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

string url::encode(const string_view str, const bool encode_slash) noexcept {
    string result;
    result.reserve(str.size() * 3);

    for (const char c : str) {
        if (should_encode(c, encode_slash)) {
            result += '%';
            const auto uc = static_cast<unsigned char>(c);
            const char hex[] = "0123456789ABCDEF";
            result += hex[uc >> 4];
            result += hex[uc & 0x0F];
        } else {
            result += c;
        }
    }

    return result;
}

optional<string> url::decode(const string_view str) noexcept {
    string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            if (i + 2 >= str.size()) {
                return none;
            }

            try {
                const int high = hexadecimal::digit_value(str[i + 1]);
                const int low = hexadecimal::digit_value(str[i + 2]);
                result += static_cast<char>((high << 4) | low);
                i += 2;
            } catch (...) {
                return none;
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

uint16_t url::default_port(const string_view scheme) noexcept {
    if (scheme == "http" || scheme == "ws") return 80;
    if (scheme == "https" || scheme == "wss") return 443;
    if (scheme == "ftp") return 21;
    if (scheme == "ssh") return 22;
    if (scheme == "telnet") return 23;
    if (scheme == "smtp") return 25;
    if (scheme == "dns") return 53;
    if (scheme == "pop3") return 110;
    if (scheme == "imap") return 143;
    return 0;
}

string_view url::default_scheme(const uint16_t port, const bool is_ws) noexcept {
    switch (port) {
        case 80: {
            return is_ws ? "ws" : "http";
        }
        case 443: {
            return is_ws ? "wss" : "https";
        }
        case 21: {
            return "ftp";
        }
        case 22: {
            return "ssh";
        }
        case 23: {
            return "telnet";
        }
        case 25: {
            return "smtp";
        }
        case 53: {
            return "dns";
        }
        case 110: {
            return "pop3";
        }
        case 143: {
            return "imap";
        }
        default: {
            return "";
        }
    }
}

NEFORCE_END_NAMESPACE__
