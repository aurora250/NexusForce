#include <NeForce/core/encoding/percent.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/util/network_exception.hpp>
#include <NeForce/network/util/url.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    cbyte_view as_bytes(const string_view text) noexcept {
        return cbyte_view{reinterpret_cast<const byte_t*>(text.data()), text.size()};
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
    target.scheme.lowercase();
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

    if (colon_pos != string::npos) {
        if (colon_pos < host_port.size() - 1) {
            const auto port_str = host_port.substr(colon_pos + 1);
            target.port = ports::parse(port_str);

            if (!target.port) {
                bool is_valid_number = true;
                uint16_t port_num = 0;

                for (const char c: port_str) {
                    if (c < '0' || c > '9') {
                        is_valid_number = false;
                        break;
                    }
                    const uint16_t new_val = port_num * 10 + (c - '0');
                    if (new_val < port_num) {
                        is_valid_number = false;
                        break;
                    }
                    port_num = new_val;
                }

                if (is_valid_number && port_num > 0) {
                    target.port = ports(port_num);
                } else {
                    NEFORCE_THROW_EXCEPTION(network_exception("URL invalid port"));
                }
            }
        } else {
            NEFORCE_THROW_EXCEPTION(network_exception("URL malformed: empty port"));
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
        const ports default_port = ports::parse(scheme.view());
        if (default_port && port == default_port) {
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
    return percent::encode(as_bytes(str), encode_slash);
}

optional<string> url::decode(const string_view str) {
    const optional<byte_vector> decoded = percent::try_decode(str);
    if (!decoded) {
        return none;
    }
    if (decoded->empty()) {
        return string{};
    }
    return string{reinterpret_cast<const char*>(decoded->data()), decoded->size()};
}

string url::encode_form(const string_view str) { return percent::encode_form(as_bytes(str)); }

string url::decode_tolerant(const string_view str) { return percent::decode_tolerant(str); }

void url::parse_query(const string_view query, flat_unordered_map<string, string>& params) {
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

string url::build_query(const flat_unordered_map<string, string>& params) {
    if (params.empty()) {
        return "";
    }

    string result;
    bool first = true;
    for (const auto& param: params) {
        if (!first) {
            result += '&';
        }
        result += url::encode_form(param.first.view()) + '=' + url::encode_form(param.second.view());
        first = false;
    }
    return result;
}

NEFORCE_END_NAMESPACE__
