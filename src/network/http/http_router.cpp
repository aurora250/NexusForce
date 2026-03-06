#include <NeForce/network/http/http_router.hpp>
NEFORCE_BEGIN_NAMESPACE__

http_router::http_router() {
    setup_default_handlers();
}

void http_router::get(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::GET.method()][path] = _NEFORCE move(handler);
}

void http_router::post(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::POST.method()][path] = _NEFORCE move(handler);
}

void http_router::put(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::PUT.method()][path] = _NEFORCE move(handler);
}

void http_router::del(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::DELETE.method()][path] = _NEFORCE move(handler);
}

void http_router::head(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::HEAD.method()][path] = _NEFORCE move(handler);
}

void http_router::options(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::OPTIONS.method()][path] = _NEFORCE move(handler);
}

void http_router::trace(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::TRACE.method()][path] = _NEFORCE move(handler);
}

void http_router::connect(const string& path, handler_func handler) {
    routes_[HTTP_METHOD::CONNECT.method()][path] = _NEFORCE move(handler);
}

void http_router::get_post(const string& path, handler_func handler) {
    get(path, handler);
    post(path, _NEFORCE move(handler));
}

void http_router::post_delete(const string& path, handler_func handler) {
    post(path, handler);
    del(path, _NEFORCE move(handler));
}

void http_router::all(const string& path, handler_func handler) {
    get(path, handler);
    post(path, handler);
    put(path, handler);
    del(path, handler);
    head(path, handler);
    options(path, handler);
    trace(path, handler);
    connect(path, _NEFORCE move(handler));
}

void http_router::route(const HTTP_METHOD& method, const string& path, handler_func handler) {
    string method_str = method.method();
    if (method_str.find(',') != string::npos) {
        vector<string> methods = split_methods(method_str);
        for (auto& m : methods) {
            routes_[m.trim()][path] = handler;
        }
    } else {
        routes_[method_str][path] = handler;
    }
}

http_router::handler_func* http_router::find_handler(const HTTP_METHOD& method, const string& path) {
    auto method_it = routes_.find(method.method());
    if (method_it == routes_.end()) return nullptr;

    auto path_it = method_it->second.find(path);
    if (path_it == method_it->second.end()) return nullptr;

    return &path_it->second;
}

vector<string> http_router::split_methods(const string& method_str) {
    vector<string> result;
    size_t start = 0;
    size_t pos = method_str.find(',');

    while (pos != string::npos) {
        result.push_back(method_str.substr(start, pos - start));
        start = pos + 1;
        pos = method_str.find(',', start);
    }
    result.push_back(method_str.substr(start));

    return result;
}

void http_router::setup_default_handlers() {
    not_found_handler_ = [](http_request& request, http_response& response) {
        response.status = HTTP_STATUS::S4_NOT_FOUNT;
        response.status_message = "Not Found";
        response.body = "404 - Resource not found: " + request.path;
    };

    method_not_allowed_handler_ = [](http_request& request, http_response& response) {
        response.status = HTTP_STATUS::S4_METHOD_NOT_ALLOWED;
        response.status_message = "Method Not Allowed";
        response.body = "405 - Method not allowed: " + request.method.to_string();
    };
}

http_response http_router::handle_request(http_request& request) {
    http_response response;

    if (!middleware_chain_.execute_pre_filters(request, response)) {
        return response;
    }

    middleware_chain_.execute_filters(request, response);

    auto* handler = find_handler(request.method, request.path);
    if (handler) {
        (*handler)(request, response);
    } else {
        bool path_exists = false;
        for (const auto& elm : routes_) {
            const auto& paths = elm.second;
            if (paths.find(request.path) != paths.end()) {
                path_exists = true;
                break;
            }
        }

        if (path_exists) {
            method_not_allowed_handler_(request, response);
        } else {
            not_found_handler_(request, response);
        }
    }

    middleware_chain_.execute_post_filters(request, response);

    return response;
}

NEFORCE_END_NAMESPACE__
