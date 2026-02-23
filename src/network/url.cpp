#include <MSTL/core/utility/packages.hpp>
#include <MSTL/network/url.hpp>
MSTL_BEGIN_NAMESPACE__

url url::parse(const string_view str) {
    url target{};
    const size_t len = str.size();

    const auto scheme_end = str.find("://");
    if (scheme_end == string::npos) {
        throw_exception(network_exception("Invalid URL: missing scheme"));
    }
    target.scheme = str.substr(0, scheme_end);
    size_t pos = scheme_end + 3;

    const size_t path_pos = str.find('/', pos);
    const string_view host_port = str.substr(pos, path_pos == string::npos ? len - pos : path_pos - pos);

    const size_t colon = host_port.find(':');
    if (colon != string::npos) {
        target.host = host_port.substr(0, colon);
        const string_view port_str = host_port.substr(colon + 1);
        target.port = _MSTL uinteger16::parse(port_str).value();
    } else {
        target.host = host_port;
        target.port = (target.scheme == "https") ? 443 : 80;
    }

    pos = (path_pos == string::npos) ? len : path_pos;

    if (pos < len) {
        const size_t qpos = str.find('?', pos);
        if (qpos == string::npos) {
            target.path = str.substr(pos);
        } else {
            target.path = str.substr(pos, qpos - pos);
            target.query = str.substr(qpos + 1);
        }
    } else {
        target.path = "/";
    }

    return target;
}

string url::to_string() const {
    string ret = scheme + "://" + host;
    if (port != 0 && ((scheme == "http" && port != 80) || (scheme == "https" && port != 443))) {
        ret += ":" + _MSTL to_string(port);
    }
    ret += path.empty() ? "/" : path;
    if (!query.empty()) ret += "?" + query;
    return ret;
}

MSTL_END_NAMESPACE__
