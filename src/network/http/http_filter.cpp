#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/memory/shared_ptr.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/async_filter.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_range.hpp>
#include <NeForce/network/util/url.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

void http_filter_chain::add_filter(unique_ptr<http_filter> filter) {
    if (filter) {
        filters_.push_back({move(filter), true});
    }
}

void http_filter_chain::add_filter_ref(http_filter* filter) {
    if (filter != nullptr) {
        filters_.push_back({unique_ptr<http_filter>(filter), false});
    }
}

void http_filter_chain::clear() noexcept {
    for (auto& entry: filters_) {
        if (!entry.owned) {
            static_cast<void>(entry.filter.release());
        }
    }
    filters_.clear();
}

bool http_filter_chain::execute_pre_filters(http_request& request, http_response& response) {
    for (const auto& entry: filters_) {
        if (!entry.filter) {
            continue;
        }

        try {
            if (!entry.filter->pre_filter(request, response)) {
                return false;
            }
        } catch (const exception& e) {
            println("[ERROR] Pre-filter '", entry.filter->name(), "' failed: ", e.what());
            return false;
        } catch (...) {
            println("[ERROR] Pre-filter '", entry.filter->name(), "' failed with unknown error");
            return false;
        }
    }
    return true;
}

void http_filter_chain::execute_post_filters(http_request& request, http_response& response) {
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto it = filters_.rbegin(); it != filters_.rend(); ++it) {
        if (!it->filter) {
            continue;
        }

        try {
            it->filter->post_filter(request, response);
        } catch (const exception& e) {
            println("[ERROR] Post-filter '", it->filter->name(), "' failed: ", e.what());
        } catch (...) {
            println("[ERROR] Post-filter '", it->filter->name(), "' failed with unknown error");
        }
    }
}

void http_filter_chain::execute_filters(http_request& request, http_response& response) {
    for (const auto& entry: filters_) {
        if (!entry.filter) {
            continue;
        }

        try {
            entry.filter->do_filter(request, response);
        } catch (const exception& e) {
            println("[ERROR] Filter '", entry.filter->name(), "' failed: ", e.what());
        } catch (...) {
            println("[ERROR] Filter '", entry.filter->name(), "' failed with unknown error");
        }
    }
}

void http_filter_chain::execute_pre_filters_async(http_request& request, http_response& response, http_context& ctx,
                                                  function<void(bool)> next) {
    auto* filters = &filters_;

    struct chain_state {
        const vector<filter_entry>* filters;
        size_t index = 0;
        http_request* request_raw = nullptr;
        http_response* response_raw = nullptr;
        http_context* ctx_raw = nullptr;
        shared_ptr<http_request> request_own;
        shared_ptr<http_response> response_own;
        shared_ptr<http_context> ctx_own;
        function<void(bool)> next;
        bool migrated = false;

        http_request& req() { return migrated ? *request_own : *request_raw; }
        http_response& res() { return migrated ? *response_own : *response_raw; }
        http_context& c() { return migrated ? *ctx_own : *ctx_raw; }

        void ensure_ownership() {
            if (migrated) {
                return;
            }
            request_own = make_shared<http_request>(*request_raw);
            response_own = make_shared<http_response>(*response_raw);
            ctx_own = make_shared<http_context>(*ctx_raw);
            migrated = true;
        }

        void run_next() {
            if (index >= filters->size()) {
                next(true);
                return;
            }
            const auto& entry = (*filters)[index++];
            if (!entry.filter) {
                run_next();
                return;
            }
            auto* af = dynamic_cast<async_filter*>(entry.filter.get());
            if (af != nullptr) {
                ensure_ownership();
                try {
                    af->pre_filter_async(req(), res(), c(), [this](bool ok) {
                        if (ok) {
                            run_next();
                        } else {
                            next(false);
                        }
                    });
                } catch (...) {
                    next(false);
                }
            } else {
                try {
                    if (!entry.filter->pre_filter(req(), res())) {
                        next(false);
                        return;
                    }
                } catch (...) {
                    next(false);
                    return;
                }
                run_next();
            }
        }
    };

    auto state = make_shared<chain_state>();
    state->filters = filters;
    state->request_raw = &request;
    state->response_raw = &response;
    state->ctx_raw = &ctx;
    state->next = move(next);
    state->run_next();
}

void http_filter_chain::execute_post_filters_async(http_request& request, http_response& response, http_context& ctx,
                                                   function<void()> next) {
    auto* filters = &filters_;

    struct chain_state {
        const vector<filter_entry>* filters;
        size_t index = 0;
        http_request* request_raw = nullptr;
        http_response* response_raw = nullptr;
        http_context* ctx_raw = nullptr;
        shared_ptr<http_request> request_own;
        shared_ptr<http_response> response_own;
        shared_ptr<http_context> ctx_own;
        function<void()> next;
        bool migrated = false;

        http_request& req() { return migrated ? *request_own : *request_raw; }
        http_response& res() { return migrated ? *response_own : *response_raw; }
        http_context& c() { return migrated ? *ctx_own : *ctx_raw; }

        void ensure_ownership() {
            if (migrated) {
                return;
            }
            request_own = make_shared<http_request>(*request_raw);
            response_own = make_shared<http_response>(*response_raw);
            ctx_own = make_shared<http_context>(*ctx_raw);
            migrated = true;
        }

        void run_next() {
            if (index >= filters->size()) {
                next();
                return;
            }
            const auto& entry = (*filters)[index++];
            if (!entry.filter) {
                run_next();
                return;
            }
            auto* af = dynamic_cast<async_filter*>(entry.filter.get());
            if (af != nullptr) {
                ensure_ownership();
                try {
                    af->post_filter_async(req(), res(), c(), [this](bool) { run_next(); });
                } catch (...) {
                    run_next();
                }
            } else {
                try {
                    entry.filter->post_filter(req(), res());
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                } catch (...) {
                    // ignore
                }
                run_next();
            }
        }
    };

    auto state = make_shared<chain_state>();
    state->filters = filters;
    state->request_raw = &request;
    state->response_raw = &response;
    state->ctx_raw = &ctx;
    state->next = move(next);
    state->run_next();
}

bool cors_filter::pre_filter(http_request& request, http_response& response) {
    if (allowed_origins.empty()) {
        return true;
    }

    const auto origin = request.header("Origin");
    if (origin.empty()) {
        return true;
    }

    if (allowed_origins != "*" && allowed_origins != origin) {
        return true;
    }

    response.headers[http_key::Access_Control_Allow_Origin()] = allowed_origins;
    // 反射非通配符 origin 时需添加 Vary: Origin（W3C Fetch 规范）
    if (allowed_origins != "*") {
        response.headers["Vary"] = "Origin";
    }
    if (allow_credentials) {
        response.headers[http_key::Access_Control_Allow_Credentials()] = "true";
    }
    response.headers[http_key::Access_Control_Allow_Methods()] = allowed_methods.to_string();
    response.headers[http_key::Access_Control_Allow_Headers()] = allowed_headers;
    response.headers[http_key::Access_Control_Max_Age()] = to_string(max_age.count());

    if (request.method.is_options()) {
        response.status = http_status::S2_NO_CONTENT;
        response.status_message = "No Content";
        return false;
    }

    return true;
}

bool logging_filter::pre_filter(http_request& request, http_response& response) {
    string log_msg =
            "[" + datetime::now().to_string() + "] Request: " + request.method.to_string() + " " + request.path;

    if (!request.query.empty()) {
        log_msg += "?" + request.query;
    }

    if (log_headers && !request.headers.empty()) {
        log_msg += "\n  Headers:";
        for (const auto& pair: request.headers) {
            const string key = pair.first;
            const string value = pair.second;
            log_msg += "\n" + move(key) + ": " + move(value);
        }
    }

    if (log_body && !request.body.empty()) {
        const size_t body_size = request.body.size();
        const size_t max_size = max_body_log_size.bytes();
        const size_t log_size = (body_size > max_size) ? max_size : body_size;
        log_msg += "\nBody (" + to_string(body_size) + " bytes): " + request.body.view(0, log_size);
        if (body_size > max_size) {
            log_msg += "...";
        }
    }

    println(log_msg);

    return true;
}

void logging_filter::post_filter(http_request& request, http_response& response) {
    using UT = underlying_type_t<http_status>;

    string log_msg = "[" + datetime::now().to_string() + "] Response: " + to_string(static_cast<UT>(response.status));

    if (!response.status_message.empty()) {
        log_msg += " " + response.status_message;
    }

    if (log_headers && !response.headers.empty()) {
        log_msg += "\nHeaders:";
        for (const auto& pair: response.headers) {
            const string key = pair.first;
            const string value = pair.second;
            log_msg += "\n" + key + ": " + value;
        }
    }

    if (log_body && !response.body.empty()) {
        const size_t body_size = response.body.size();
        const size_t max_size = max_body_log_size.bytes();
        const size_t log_size = (body_size > max_size) ? max_size : body_size;
        log_msg += "\nBody (" + to_string(body_size) + " bytes): " + response.body.view(0, log_size);
        if (body_size > max_size) {
            log_msg += "...";
        }
    }

    println(log_msg);
}

static_file_filter::static_file_filter(string root_path) :
root_path_(move(root_path)) {
    if (!root_path_.empty() && !root_path_.ends_with("/")) {
        root_path_ += "/";
    }

    mime_types_[".css"] = http_content::CSS_TEXT();
    mime_types_[".js"] = "application/javascript";
    mime_types_[".svg"] = "image/svg+xml";
    mime_types_[".ico"] = "image/x-icon";
    mime_types_[".jpg"] = http_content::JPEG_IMG();
    mime_types_[".jpeg"] = http_content::JPEG_IMG();
    mime_types_[".png"] = http_content::PNG_IMG();
    mime_types_[".bmp"] = http_content::BMP_IMG();
    mime_types_[".webp"] = http_content::WEBP_IMG();
    mime_types_[".gif"] = "image/gif";
    mime_types_[".html"] = http_content::HTML_TEXT();
    mime_types_[".htm"] = http_content::HTML_TEXT();
    mime_types_[".json"] = http_content::JSON_APP();
    mime_types_[".txt"] = http_content::PLAIN_TEXT();
    mime_types_[".xml"] = http_content::XML_TEXT();
    mime_types_[".woff2"] = "font/woff2";
    mime_types_[".ttf"] = "font/ttf";
    mime_types_[".pdf"] = "application/pdf";
    mime_types_[".mp4"] = "video/mp4";
}

optional<http_content> static_file_filter::get_mime_type(const string& path) const {
    for (const auto& mime: mime_types_) {
        const auto& ext = mime.first;
        auto type = mime.second;
        if (path.ends_with(ext.view())) {
            return optional<http_content>{move(type)};
        }
    }
    return none;
}

bool static_file_filter::is_safe_path(const string& path) {
    if (path.empty()) {
        return false;
    }
    if (path.contains("..")) {
        return false;
    }
    if (path.contains("//")) {
        return false;
    }
    if (path.contains('%')) {
        auto decoded = url::decode(path.view());
        if (decoded && (decoded->contains("..") || decoded->contains("//"))) {
            return false;
        }
    }
    return true;
}

bool static_file_filter::pre_filter(http_request& request, http_response& response) {
    if (!request.method.is_get() && !request.method.is_head()) {
        return true;
    }

    const string& req_path = request.path;

    if (!is_safe_path(req_path)) {
        response.status = http_status::S4_FORBIDDEN;
        response.status_message = "Forbidden";
        response.set_content_type(http_content::PLAIN_TEXT());
        response.body = "Access denied";
        return false;
    }

    const auto mime_type = get_mime_type(req_path);
    if (!mime_type) {
        return true;
    }

    try {
        const path file_path(root_path_ + req_path);

        if (!file_path.exists()) {
            return true;
        }

        const auto file_size = filesystem::size(file_path);
        if (file_size > max_file_size_) {
            response.status = http_status::S4_PAYLOAD_TOO_LARGE;
            response.status_message = "Payload Too Large";
            response.set_content_type(http_content::PLAIN_TEXT());
            response.body = "File too large";
            return false;
        }

        const bool is_binary = mime_type->is_jpeg_img() || mime_type->is_png_img() || mime_type->is_bmp_img() ||
                               mime_type->is_webp_img();

        if (is_binary) {
            response.body = file(file_path).read_binary();
        } else {
            response.body = file(file_path).read();
        }

        response.set_content_type(*mime_type);

        if (enable_range_ && !response.body.empty()) {
            const auto range_header = request.header("Range");
            if (!range_header.empty()) {
                const string full_content = move(response.body);
                auto ranges = parse_ranges(range_header, full_content.size());

                if (ranges.empty()) {
                    response.status = http_status::S4_RANGE_NOT_SATISFIABLE;
                    response.status_message = "Range Not Satisfiable";
                    response.set_header("Content-Range", string("bytes */") + to_string(full_content.size()));
                    response.body.clear();
                    return false;
                }

                if (ranges.size() == 1) {
                    const auto& r = ranges[0];
                    const uint64_t length = r.end - r.start + 1;
                    response.body = full_content.substr(r.start, static_cast<size_t>(length));

                    response.status = http_status::S2_PARTIAL_CONTENT;
                    response.status_message = "Partial Content";
                    response.set_header("Content-Range", build_content_range(r, full_content.size()));
                } else {
                    string boundary = "NEXUSFORCE_";
                    for (int i = 0; i < 16; ++i) {
                        boundary += format("{:x}", secret::next_int<uint32_t>(16));
                    }
                    response.body = build_multipart_ranges(ranges, mime_type->to_string().view(), boundary.view(),
                                                           [&full_content](const byte_range& rng) -> string {
                                                               const uint64_t length = rng.end - rng.start + 1;
                                                               return full_content.substr(rng.start,
                                                                                          static_cast<size_t>(length));
                                                           });

                    response.status = http_status::S2_PARTIAL_CONTENT;
                    response.status_message = "Partial Content";
                    response.set_content_type("multipart/byteranges; boundary="_s + boundary);
                }
            } else {
                response.status = http_status::S2_OK;
                response.status_message = "OK";
            }
        } else {
            response.status = http_status::S2_OK;
            response.status_message = "OK";
        }

        if (enable_cache_) {
            response.set_header("Cache-Control", "public, max-age=3600");
        }

        return false;
    } catch (const exception& e) {
        println("[ERROR] Static file filter failed: ", e.what());
        return true;
    }
}

void static_file_filter::add_mime_type(const string& extension, http_content content_type) {
    if (!extension.empty()) {
        mime_types_[extension] = move(content_type);
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
        response.status = http_status::S4_TOO_MANY_REQUESTS;
        response.status_message = "Too Many Requests";
        response.set_content_type(http_content::PLAIN_TEXT());
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
    // NOLINTNEXTLINE(readability-use-anyofallof)
    for (const auto& excluded: excluded_paths_) {
        if (path == excluded || path.starts_with(excluded)) {
            return true;
        }
    }
    return false;
}

bool authentication_filter::is_path_protected(const string& path) const {
    if (included_paths_.empty()) {
        return !is_path_excluded(path);
    }
    for (const auto& included: included_paths_) {
        if (path == included || path.starts_with(included)) {
            return true;
        }
    }
    return false;
}

bool authentication_filter::pre_filter(http_request& request, http_response& response) {
    if (!is_path_protected(request.path)) {
        return true;
    }
    if (!auth_validator_) {
        return true;
    }

    if (!auth_validator_(request)) {
        response.status = http_status::S4_UNAUTHORIZED;
        response.status_message = "Unauthorized";
        response.set_content_type(http_content::PLAIN_TEXT());
        response.body = "Authentication required";
        response.set_header("WWW-Authenticate", "Bearer realm=\"Restricted Access\"");
        return false;
    }

    return true;
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
