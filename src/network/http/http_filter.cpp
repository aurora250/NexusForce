// #include <NeForce/core/algorithm/remove.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_filter.hpp>
NEFORCE_BEGIN_NAMESPACE__

void http_filter_chain::add_filter(unique_ptr<http_filter> filter) {
    if (filter) {
        filters_.emplace_back(_NEFORCE move(filter));
    }
}

void http_filter_chain::add_filter_ref(http_filter* filter) {
    if (filter) {
        filters_.push_back(unique_ptr<http_filter>(filter));
        owns_filters_ = false;
    }
}

// bool http_filter_chain::remove_filter(const string& filter_name) {
//     auto it = remove_if(filters_.begin(), filters_.end(),
//         [&filter_name](const unique_ptr<http_filter>& f) {
//             return f && f->name() == filter_name;
//         });
//
//     if (it != filters_.end()) {
//         filters_.erase(it, filters_.end());
//         return true;
//     }
//     return false;
// }

void http_filter_chain::clear() noexcept {
    if (owns_filters_) {
        filters_.clear();
    } else {
        for (auto& filter : filters_) {
            filter.release();
        }
        filters_.clear();
    }
}

bool http_filter_chain::execute_pre_filters(http_request& request, http_response& response) {
    for (const auto& filter : filters_) {
        if (!filter) {
            continue;
        }

        try {
            if (!filter->pre_filter(request, response)) {
                return false;
            }
        } catch (const exception& e) {
            println("[ERROR] Pre-filter '", filter->name(), "' failed: ", e.what());
            return false;
        } catch (...) {
            println("[ERROR] Pre-filter '", filter->name(), "' failed with unknown error");
            return false;
        }
    }
    return true;
}

void http_filter_chain::execute_post_filters(http_request& request, http_response& response) {
    for (auto it = filters_.rbegin(); it != filters_.rend(); ++it) {
        if (!*it) {
            continue;
        }

        try {
            (*it)->post_filter(request, response);
        } catch (const exception& e) {
            println("[ERROR] Post-filter '", (*it)->name(), "' failed: ", e.what());
        } catch (...) {
            println("[ERROR] Post-filter '", (*it)->name(), "' failed with unknown error");
        }
    }
}

void http_filter_chain::execute_filters(http_request& request, http_response& response) {
    for (const auto& filter : filters_) {
        if (!filter) {
            continue;
        }

        try {
            filter->do_filter(request, response);
        } catch (const exception& e) {
            println("[ERROR] Filter '", filter->name(), "' failed: ", e.what());
        } catch (...) {
            println("[ERROR] Filter '", filter->name(), "' failed with unknown error");
        }
    }
}

bool cors_filter::pre_filter(http_request& request, http_response& response) {
    if (allowed_origins.empty()) {
        return true;
    }

    response.headers[HTTP_KEY::Access_Control_Allow_Origin] = allowed_origins;
    response.headers[HTTP_KEY::Access_Control_Allow_Credentials] = to_string(allow_credentials);
    response.headers[HTTP_KEY::Access_Control_Allow_Methods] = allowed_methods.to_string();
    response.headers[HTTP_KEY::Access_Control_Allow_Headers] = allowed_headers;
    response.headers[HTTP_KEY::Access_Control_Max_Age] = to_string(max_age);

    if (request.method.is_options()) {
        response.status = HTTP_STATUS::S2_NO_CONTENT;
        response.status_message = "No Content";
        return false;
    }

    return true;
}

bool logging_filter::pre_filter(http_request& request, http_response& response) {
    string log_msg =
        "[" + datetime::now().to_string() + "] Request: " +
        request.method.to_string() + " " + request.path;

    if (!request.query.empty()) {
        log_msg += "?" + request.query;
    }

    if (log_headers && !request.headers.empty()) {
        log_msg += "\n  Headers:";
        for (const auto& pair : request.headers) {
            const string key = pair.first;
            const string value = pair.second;
            log_msg += "\n" + move(key) + ": " + move(value);
        }
    }

    if (log_body && !request.body.empty()) {
        const size_t body_size = request.body.size();
        const size_t log_size = (body_size > max_body_log_size) ? max_body_log_size : body_size;
        log_msg += "\nBody (" + to_string(body_size) + " bytes): " + request.body.view(0, log_size);
        if (body_size > max_body_log_size) {
            log_msg += "...";
        }
    }

    println(log_msg);

    return true;
}

void logging_filter::post_filter(http_request& request, http_response& response) {
    using UT = underlying_type_t<HTTP_STATUS>;

    string log_msg =
        "[" + datetime::now().to_string() + "] Response: " +
        to_string(static_cast<UT>(response.status));

    if (!response.status_message.empty()) {
        log_msg += " " + response.status_message;
    }

    if (log_headers && !response.headers.empty()) {
        log_msg += "\nHeaders:";
        for (const auto& pair : response.headers) {
            const string key = pair.first;
            const string value = pair.second;
            log_msg += "\n" + key + ": " + value;
        }
    }

    if (log_body && !response.body.empty()) {
        const size_t body_size = response.body.size();
        const size_t log_size = (body_size > max_body_log_size) ? max_body_log_size : body_size;
        log_msg += "\nBody (" + to_string(body_size) + " bytes): " + response.body.view(0, log_size);
        if (body_size > max_body_log_size) {
            log_msg += "...";
        }
    }

    println(log_msg);
}

static_file_filter::static_file_filter(string root_path)
: root_path_(_NEFORCE move(root_path)) {
    if (!root_path_.empty() && !root_path_.ends_with("/")) {
        root_path_ += "/";
    }

    mime_types_[".css"] = HTTP_CONTENT::CSS_TEXT;
    mime_types_[".jpg"] = HTTP_CONTENT::JPEG_IMG;
    mime_types_[".jpeg"] = HTTP_CONTENT::JPEG_IMG;
    mime_types_[".png"] = HTTP_CONTENT::PNG_IMG;
    mime_types_[".bmp"] = HTTP_CONTENT::BMP_IMG;
    mime_types_[".webp"] = HTTP_CONTENT::WEBP_IMG;
    mime_types_[".html"] = HTTP_CONTENT::HTML_TEXT;
    mime_types_[".htm"] = HTTP_CONTENT::HTML_TEXT;
    mime_types_[".json"] = HTTP_CONTENT::JSON_APP;
    mime_types_[".txt"] = HTTP_CONTENT::PLAIN_TEXT;
    mime_types_[".xml"] = HTTP_CONTENT::XML_TEXT;
}

optional<HTTP_CONTENT> static_file_filter::get_mime_type(const string& path) const {
    for (const auto& mime : mime_types_) {
        const auto& ext = mime.first;
        auto type = mime.second;
        if (path.ends_with(ext.view())) {
            return optional<HTTP_CONTENT>{move(type)};
        }
    }
    return none;
}

bool static_file_filter::is_safe_path(const string& path) {
    if (path.find("..") != string::npos) {
        return false;
    }
    if (path.find("//") != string::npos) {
        return false;
    }
    return true;
}

bool static_file_filter::pre_filter(http_request& request, http_response& response) {
    if (!request.method.is_get() && !request.method.is_head()) {
        return true;
    }

    const string& path = request.path;

    if (!is_safe_path(path)) {
        response.status = HTTP_STATUS::S4_FORBIDDEN;
        response.status_message = "Forbidden";
        response.set_content_type(HTTP_CONTENT::PLAIN_TEXT);
        response.body = "Access denied";
        return false;
    }

    const auto mime_type = get_mime_type(path);
    if (!mime_type) {
        return true;
    }

    try {
        const _NEFORCE path file_path(root_path_ + path);

        if (!file_path.exists()) {
            return true;
        }

        const size_t file_size = file::size(file_path);
        if (file_size > max_file_size_) {
            response.status = HTTP_STATUS::S4_PAYLOAD_LARGE;
            response.status_message = "Payload Too Large";
            response.set_content_type(HTTP_CONTENT::PLAIN_TEXT);
            response.body = "File too large";
            return false;
        }

        const bool is_binary = mime_type->is_jpeg_img() ||
                               mime_type->is_png_img() ||
                               mime_type->is_bmp_img() ||
                               mime_type->is_webp_img();

        if (is_binary) {
            response.body = file(file_path).read_binary();
        } else {
            response.body = file(file_path).read();
        }

        response.status = HTTP_STATUS::S2_OK;
        response.status_message = "OK";
        response.set_content_type(*mime_type);

        if (enable_cache_) {
            response.set_header("Cache-Control", "public, max-age=3600");
        }

        return false;
    } catch (const exception& e) {
        println("[ERROR] Static file filter failed: ", e.what());
        return true;
    }
}

void static_file_filter::add_mime_type(const string& extension, HTTP_CONTENT content_type) {
    if (!extension.empty()) {
        mime_types_[extension] = _NEFORCE move(content_type);
    }
}

bool rate_limit_filter::pre_filter(http_request& request, http_response& response) {
    const auto client_ip = request.client_ip();
    if (client_ip.empty()) {
        return true;
    }

    lock<mutex> lk(mutex_);

    const auto now = datetime::now();
    auto& info = client_requests_[client_ip];

    const auto elapsed = now - info.last_reset;
    if (elapsed >= window_seconds.count()) {
        info.count = 0;
        info.last_reset = now;
    }

    info.count++;

    if (info.count > max_requests) {
        response.status = HTTP_STATUS::S4_MANY_REQUESTS;
        response.status_message = "Too Many Requests";
        response.set_content_type(HTTP_CONTENT::PLAIN_TEXT);
        response.body = "Rate limit exceeded";
        response.set_header("Retry-After", to_string(window_seconds.count()));
        return false;
    }

    response.set_header("X-RateLimit-Limit", to_string(max_requests));
    response.set_header("X-RateLimit-Remaining", to_string(max_requests - info.count));
    response.set_header("X-RateLimit-Reset", to_string(timestamp(info.last_reset).value() + window_seconds.count()));

    return true;
}

void rate_limit_filter::cleanup_old_entries() {
    lock<mutex> lk(mutex_);

    const auto now = datetime::now();
    for (auto it = client_requests_.begin(); it != client_requests_.end();) {
        const auto elapsed = now - it->second.last_reset;
        if (elapsed >= window_seconds.count() * 2) {
            it = client_requests_.erase(it);
        } else {
            ++it;
        }
    }
}

bool authentication_filter::is_path_excluded(const string& path) const {
    for (const auto& excluded : excluded_paths_) {
        if (path == excluded || path.starts_with(excluded)) {
            return true;
        }
    }
    return false;
}

bool authentication_filter::pre_filter(http_request& request, http_response& response) {
    if (is_path_excluded(request.path)) {
        return true;
    }
    if (!auth_validator_) {
        return true;
    }

    if (!auth_validator_(request)) {
        response.status = HTTP_STATUS::S4_UNAUTHORIZED;
        response.status_message = "Unauthorized";
        response.set_content_type(HTTP_CONTENT::PLAIN_TEXT);
        response.body = "Authentication required";
        response.set_header("WWW-Authenticate", "Bearer");
        return false;
    }

    return true;
}

NEFORCE_END_NAMESPACE__
