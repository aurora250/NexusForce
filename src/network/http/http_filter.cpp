#include <MSTL/network/http/http_filter.hpp>
#include <MSTL/core/system/console.hpp>
#include <MSTL/core/file/file.hpp>
MSTL_BEGIN_NAMESPACE__

void http_filter_chain::clean_filter() noexcept {
    for (const auto* filter : filters_) {
        delete filter;
    }
    filters_.clear();
}

bool http_filter_chain::execute_pre_filters(http_request& request, http_response& response) {
    for (auto* filter : filters_) {
        if (!filter->pre_filter(request, response)) {
            return false;
        }
    }
    return true;
}

void http_filter_chain::execute_post_filters(http_request& request, http_response& response) {
    for (auto it = filters_.rbegin(); it != filters_.rend(); ++it) {
        (*it)->post_filter(request, response);
    }
}

void http_filter_chain::execute_filters(http_request& request, http_response& response) {
    for (auto* filter : filters_) {
        filter->do_filter(request, response);
    }
}

bool cors_filter::pre_filter(http_request& request, http_response& response) {
    response.set_allow_origin(allowed_origins_);
    response.set_allow_credentials(allow_credentials_);
    response.set_allow_method(allowed_methods_);
    response.set_allow_headers(allowed_headers_);
    response.set_max_age(max_age_);

    if (request.method().is_options()) {
        response.set_status(HTTP_STATUS::S2_NO_CONTENT);
        response.set_status_msg("No Content");
        return false;
    }

    return true;
}

bool logging_filter::pre_filter(http_request& request, http_response& response) {
    println("[", datetime::now(), "] Request: ", request.method(), " ", request.path());
    return true;
}

void logging_filter::post_filter(http_request& request, http_response& response) {
    using UT = underlying_type_t<HTTP_STATUS>;
    println("[", datetime::now(), "] Response: ",
        static_cast<UT>(response.status()), " ",
        response.status_msg()
        );
}

static_file_filter::static_file_filter(string root_path)
: root_path_(_MSTL move(root_path)) {
    mime_types_[".css"] = HTTP_CONTENT::CSS_TEXT;
    mime_types_[".jpg"] = HTTP_CONTENT::JPEG_IMG;
    mime_types_[".jpeg"] = HTTP_CONTENT::JPEG_IMG;
    mime_types_[".png"] = HTTP_CONTENT::PNG_IMG;
    mime_types_[".html"] = HTTP_CONTENT::HTML_TEXT;
    mime_types_[".json"] = HTTP_CONTENT::JSON_APP;
    mime_types_[".txt"] = HTTP_CONTENT::PLAIN_TEXT;
}

bool static_file_filter::pre_filter(http_request& request, http_response& response) {
    const string& path = request.path();

    for (const auto& elem : mime_types_) {
        const auto& ext = elem.first;
        const auto& type = elem.second;
        if (path.ends_with(ext.view())) {
            try {
                const _MSTL path file_path(root_path_ + path);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
                    response.set_body(file(file_path).read_binary());
                } else {
                    response.set_body(file(file_path).read());
                }

                response.set_ok();
                response.set_status_msg("OK");
                response.set_content_type(type);
                return false;
            } catch (...) {
                response.set_not_found();
                response.set_body("File not found");
                return false;
            }
        }
    }
    return true;
}

MSTL_END_NAMESPACE__
