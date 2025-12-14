#include <MSTL/network/url.hpp>
#include <MSTL/network/http/http_constants.hpp>
#include <MSTL/core/utility/packages.hpp>
MSTL_BEGIN_NAMESPACE__

void url::parse(const string_view str) {
    scheme.clear(); host.clear(); port = 0; path.clear(); query.clear();
    const size_t len = str.size();

    const auto scheme_end = str.find("://");
    if (scheme_end == string::npos) {
        throw_exception(http_exception("Invalid URL: missing scheme"));
    }
    scheme = str.substr(0, scheme_end);
    size_t pos = scheme_end + 3;

    const size_t path_pos = str.find('/', pos);
    const string_view host_port = str.substr(pos, path_pos == string::npos ? len - pos : path_pos - pos);

    const size_t colon = host_port.find(':');
    if (colon != string::npos) {
        host = host_port.substr(0, colon);
        const string_view port_str = host_port.substr(colon + 1);
        port = _MSTL uinteger16::parse(port_str).value();
    } else {
        host = host_port;
        port = (scheme == "https") ? 443 : 80;
    }

    pos = (path_pos == string::npos) ? len : path_pos;

    if (pos < len) {
        const size_t qpos = str.find('?', pos);
        if (qpos == string::npos) {
            path = str.substr(pos);
        } else {
            path = str.substr(pos, qpos - pos);
            query = str.substr(qpos + 1);
        }
    } else {
        path = "/";
    }
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
