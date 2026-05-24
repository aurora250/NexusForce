#include <NeForce/network/http/http_router.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    optional<regex> compile_pattern(const string& pattern, vector<string>& param_names) {
        if (pattern.contains('(') || pattern.contains('[')) {
            try {
                return regex("^" + pattern + "$");
            } catch (...) {
                return none;
            }
        }

        if (pattern.contains(':') && pattern.contains('*')) {
            return none;
        }

        string regex_pattern = "^";
        size_t pos = 0;

        while (pos < pattern.length()) {
            if (pattern[pos] == ':') {
                size_t end = pos + 1;
                while (end < pattern.length() &&
                       (is_alpha(pattern[end]) || is_digit(pattern[end]) || pattern[end] == '_')) {
                    ++end;
                }

                string param_name = pattern.substr(pos + 1, end - pos - 1);
                param_names.push_back(param_name);
                regex_pattern += "([^/]+)";
                pos = end;
            } else if (pattern[pos] == '*') {
                regex_pattern += "(.*)";
                ++pos;
            } else if (pattern[pos] == '/' || pattern[pos] == '.' || pattern[pos] == '?' || pattern[pos] == '+' ||
                       pattern[pos] == '^' || pattern[pos] == '$' || pattern[pos] == '|' || pattern[pos] == '\\') {
                regex_pattern += '\\';
                regex_pattern += pattern[pos];
                ++pos;
            } else {
                regex_pattern += pattern[pos];
                ++pos;
            }
        }

        regex_pattern += "$";

        try {
            return regex(move(regex_pattern));
        } catch (...) {
            return none;
        }
    }

    vector<string> split_methods(const string& method_str) {
        vector<string> result;
        size_t start = 0;
        size_t pos = method_str.find(',');

        while (pos != string::npos) {
            auto method = method_str.substr(start, pos - start);
            method.trim();
            if (!method.empty()) {
                result.push_back(_NEFORCE move(method));
            }
            start = pos + 1;
            pos = method_str.find(',', start);
        }
        result.push_back(method_str.tail(start));

        return result;
    }
} // namespace


http_router::http_router() { setup_default_handlers(); }

void http_router::get(const string& path, http_handler_t handler) {
    route(http_method::GET(), path, _NEFORCE move(handler));
}

void http_router::post(const string& path, http_handler_t handler) {
    route(http_method::POST(), path, _NEFORCE move(handler));
}

void http_router::put(const string& path, http_handler_t handler) {
    route(http_method::PUT(), path, _NEFORCE move(handler));
}

void http_router::del(const string& path, http_handler_t handler) {
    route(http_method::DELETE(), path, _NEFORCE move(handler));
}

void http_router::head(const string& path, http_handler_t handler) {
    route(http_method::HEAD(), path, _NEFORCE move(handler));
}

void http_router::options(const string& path, http_handler_t handler) {
    route(http_method::OPTIONS(), path, _NEFORCE move(handler));
}

void http_router::trace(const string& path, http_handler_t handler) {
    route(http_method::TRACE(), path, _NEFORCE move(handler));
}

void http_router::connect(const string& path, http_handler_t handler) {
    route(http_method::CONNECT(), path, _NEFORCE move(handler));
}

void http_router::patch(const string& path, http_handler_t handler) {
    route(http_method::PATCH(), path, _NEFORCE move(handler));
}

void http_router::get_post(const string& path, http_handler_t handler) {
    route(http_method::GET(), path, handler);
    route(http_method::POST(), path, _NEFORCE move(handler));
}

void http_router::post_delete(const string& path, http_handler_t handler) {
    route(http_method::POST(), path, handler);
    route(http_method::DELETE(), path, _NEFORCE move(handler));
}

void http_router::all(const string& path, http_handler_t handler) {
    route(http_method::GET(), path, handler);
    route(http_method::POST(), path, handler);
    route(http_method::PUT(), path, handler);
    route(http_method::DELETE(), path, handler);
    route(http_method::HEAD(), path, handler);
    route(http_method::OPTIONS(), path, handler);
    route(http_method::TRACE(), path, handler);
    route(http_method::CONNECT(), path, handler);
    route(http_method::PATCH(), path, _NEFORCE move(handler));
}

void http_router::route(const http_method& method, const string& path, const http_handler_t& handler) {
    if (path.empty() || !handler) {
        return;
    }

    const string& method_str = method.method();
    vector<string> methods;

    if (method_str.contains(',')) {
        methods = split_methods(method_str);
    } else {
        methods.push_back(method_str);
    }

    for (auto& m: methods) {
        m.trim();
        if (m.empty()) {
            continue;
        }

        route_entry entry;
        entry.pattern = path;
        entry.handler = handler;

        auto regex_opt = compile_pattern(path, entry.param_names);
        if (regex_opt) {
            entry.regex_pattern = _NEFORCE move(*regex_opt);
            entry.is_regex = true;
        }

        routes_[m].push_back(_NEFORCE move(entry));
    }
}

http_router::route_entry* http_router::find_handler(const http_method& method, const string& path,
                                                    http_request& request) {
    auto method_it = routes_.find(method.method());
    if (method_it == routes_.end()) {
        return nullptr;
    }

    string search_path = path;
    if (!strict_routing && search_path.length() > 1 && search_path.ends_with("/")) {
        search_path = search_path.head(search_path.length() - 1);
    }

    for (auto& entry: method_it->second) {
        if (entry.is_regex) {
            match_result matches = entry.regex_pattern->search(search_path);
            if (matches.matched()) {
                for (size_t i = 0; i < entry.param_names.size() && i + 1 < matches.size(); ++i) {
                    request.set_parameter(entry.param_names[i], matches[i + 1]);
                }
                for (size_t i = entry.param_names.size(); i + 1 < matches.size(); ++i) {
                    request.set_parameter(to_string(i + 1), matches[i + 1]);
                }
                return &entry;
            }
        } else {
            string pattern = entry.pattern;
            if (!strict_routing && pattern.length() > 1 && pattern.ends_with("/")) {
                pattern = pattern.head(pattern.length() - 1);
            }

            bool match = case_sensitive ? search_path == pattern : search_path.lowercase() == pattern.lowercase();

            if (match) {
                return &entry;
            }
        }
    }

    return nullptr;
}

void http_router::setup_default_handlers() {
    not_found_handler_ = [](http_request& request, http_response& response) {
        response.status = http_status::S4_NOT_FOUND;
        response.status_message = "Not Found";
        response.set_content_type(http_content::HTML_TEXT());
        response.body = "<!DOCTYPE html>"
                        "<html><head><title>404 Not Found</title></head>"
                        "<body><h1>404 - Not Found</h1>"
                        "<p>The requested resource was not found: " +
                        request.path +
                        "</p>"
                        "</body></html>";
    };

    method_not_allowed_handler_ = [](http_request& request, http_response& response) {
        response.status = http_status::S4_METHOD_NOT_ALLOWED;
        response.status_message = "Method Not Allowed";
        response.set_content_type(http_content::HTML_TEXT());
        response.body = "<!DOCTYPE html>"
                        "<html><head><title>405 Method Not Allowed</title></head>"
                        "<body><h1>405 - Method Not Allowed</h1>"
                        "<p>The method " +
                        request.method.to_string() +
                        " is not allowed for this resource.</p>"
                        "</body></html>";
    };

    exception_handler_ = [](http_request& request, http_response& response, const exception& e) {
        response.status = http_status::S5_INTERNAL_SERVER_ERROR;
        response.status_message = "Internal Server Error";
        response.set_content_type(http_content::HTML_TEXT());
        response.body = "<!DOCTYPE html>"
                        "<html><head><title>500 Internal Server Error</title></head>"
                        "<body><h1>500 - Internal Server Error</h1>"
                        "<p>An error occurred while processing your request.</p>"
                        "<p>Error: " +
                        string(e.what()) +
                        "</p>"
                        "</body></html>";
    };
}

http_response http_router::handle_request(http_request& request) {
    http_response response;

    try {
        if (!middleware_chain_.execute_pre_filters(request, response)) {
            return response;
        }

        middleware_chain_.execute_filters(request, response);

        const auto* route_entry = find_handler(request.method, request.path, request);

        if (route_entry != nullptr) {
            try {
                route_entry->handler(request, response);
            } catch (const exception& e) {
                if (exception_handler_) {
                    exception_handler_(request, response, e);
                } else {
                    throw;
                }
            }
        } else {
            bool path_exists = false;
            for (const auto& route: routes_) {
                const auto& entries = route.second;
                for (const auto& entry: entries) {
                    if (entry.is_regex) {
                        if (entry.regex_pattern->match(request.path)) {
                            path_exists = true;
                            break;
                        }
                    } else {
                        if (entry.pattern == request.path) {
                            path_exists = true;
                            break;
                        }
                    }
                }
                if (path_exists) {
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
    } catch (const exception& e) {
        if (exception_handler_) {
            exception_handler_(request, response, e);
        } else {
            response.status = http_status::S5_INTERNAL_SERVER_ERROR;
            response.status_message = "Internal Server Error";
            response.set_content_type(http_content::PLAIN_TEXT());
            response.body = "Internal Server Error: "_s + e.what();
        }
    }

    return response;
}

size_t http_router::route_count() const noexcept {
    size_t count = 0;
    for (const auto& route: routes_) {
        const auto& entries = route.second;
        count += entries.size();
    }
    return count;
}

bool http_router::has_route(const http_method& method, const string& path) const {
    const auto method_it = routes_.find(method.method());
    if (method_it == routes_.end()) {
        return false;
    }
    // NOLINTNEXTLINE(readability-use-anyofallof)
    for (const auto& entry: method_it->second) {
        if (entry.pattern == path) {
            return true;
        }
    }
    return false;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
