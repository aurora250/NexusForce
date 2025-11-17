#include <MSTL/core/utility/console.hpp>
#include <MSTL/web/filter.hpp>
MSTL_BEGIN_NAMESPACE__

void filter_chain::clean_filter() noexcept {
    for (const auto* filter : filters_) {
        delete filter;
    }
    filters_.clear();
}

bool filter_chain::execute_pre_filters(http_request& request, http_response& response) {
    for (auto* filter : filters_) {
        if (!filter->pre_filter(request, response)) {
            return false;
        }
    }
    return true;
}

void filter_chain::execute_post_filters(http_request& request, http_response& response) {
    for (auto it = filters_.rbegin(); it != filters_.rend(); ++it) {
        (*it)->post_filter(request, response);
    }
}

void filter_chain::execute_filters(http_request& request, http_response& response) {
    for (auto* filter : filters_) {
        filter->do_filter(request, response);
    }
}

void cors_filter::do_filter(http_request& request, http_response& response) {
    response.set_allow_origin(allowed_origins_);
    response.set_allow_method(allowed_methods_);
    response.set_allow_headers(allowed_headers_);
    response.set_allow_credentials(true);

    if (request.method().is_options()) {
        response.set_ok();
        response.set_status_msg("OK");
        response.set_body("");
    }
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

MSTL_END_NAMESPACE__
