#include <NeForce/network/http/reverse_proxy.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

bool reverse_proxy_filter::should_proxy(const string& path) const {
    if (path_prefix_.empty()) {
        return true;
    }
    return path.starts_with(path_prefix_);
}

proxy_backend reverse_proxy_filter::select_backend(const http_request& request) {
    lock<mutex> lk(mutex_);

    if (selector_) {
        return selector_(request);
    }

    if (backends_.empty()) {
        return {};
    }

    size_t idx = rr_counter_++ % backends_.size();
    return backends_[idx];
}

http_client_response reverse_proxy_filter::forward(const proxy_backend& backend, const http_request& request) {
    http_client::config cfg;
    cfg.connect_timeout = connect_timeout;
    cfg.send_timeout = send_timeout;
    cfg.receive_timeout = receive_timeout;
    cfg.follow_redirects = follow_redirects;

    http_client client(cfg);

    http_client_request creq;
    creq.method = request.method;
    creq.host = backend.host;
    creq.port = backend.port;
    creq.path = request.path;
    if (!request.query.empty()) {
        creq.path += "?" + request.query;
    }

    creq.headers = request.headers;

    if (!passthrough_host_header) {
        creq.headers["Host"] = backend.host;
    }

    auto client_ip = request.client_ip();
    if (!client_ip.empty()) {
        auto existing_fwd = creq.headers.find("X-Forwarded-For");
        if (existing_fwd != creq.headers.end()) {
            creq.headers["X-Forwarded-For"] = existing_fwd->second + ", " + client_ip;
        } else {
            creq.headers["X-Forwarded-For"] = client_ip;
        }
    }
    creq.headers["X-Forwarded-Proto"] = backend.scheme;
    creq.headers["X-Forwarded-Host"] = request.header("Host");

    if (header_rewrite_) {
        lock<mutex> lk(mutex_);
        header_rewrite_(creq.headers);
    }

    creq.body = request.body;

    return client.request(move(creq));
}

void reverse_proxy_filter::copy_response(const http_client_response& from, http_response& to) {
    to.status = from.status;
    to.status_message = from.status_message;
    to.body = from.body;

    for (const auto& hdr: from.headers) {
        bool skip = false;
        for (const auto& s: skip_resp_headers_) {
            if (hdr.first.lowercase() == s.lowercase()) {
                skip = true;
                break;
            }
        }
        if (!skip && !hdr.second.empty()) {
            to.headers[hdr.first] = hdr.second[0];
        }
    }
}

bool reverse_proxy_filter::pre_filter(http_request& request, http_response& response) {
    if (!should_proxy(request.path)) {
        return true;
    }

    auto backend = select_backend(request);
    if (backend.host.empty()) {
        response.status = http_status::S5_BAD_GATEWAY;
        response.status_message = "Bad Gateway";
        response.body = "No backend available";
        response.set_content_type(http_content::PLAIN_TEXT());
        return false;
    }

    try {
        auto backend_resp = forward(backend, request);
        copy_response(backend_resp, response);
    } catch (const exception& e) {
        response.status = http_status::S5_BAD_GATEWAY;
        response.status_message = "Bad Gateway";
        response.body = string("Proxy error: ") + e.what();
        response.set_content_type(http_content::PLAIN_TEXT());
    }

    return false;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
