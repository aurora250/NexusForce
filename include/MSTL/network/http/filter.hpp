#ifndef FILTER_HPP
#define FILTER_HPP
#include "http_message.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API filter {
public:
    virtual ~filter() = default;
    virtual void do_filter(http_request& request, http_response& response) = 0;
    virtual bool pre_filter(http_request& request, http_response& response) { return true; }
    virtual void post_filter(http_request& request, http_response& response) {}
};


class MSTL_API filter_chain {
private:
    vector<filter*> filters_;

public:
    void add_filter(filter* filter) { filters_.push_back(filter); }
    void clean_filter() noexcept;

    bool execute_pre_filters(http_request& request, http_response& response);
    void execute_post_filters(http_request& request, http_response& response);
    void execute_filters(http_request& request, http_response& response);
};


#ifdef DELETE
#undef DELETE
#endif

class MSTL_API cors_filter final : public filter {
private:
    string allowed_origins_ = "*";
    HTTP_METHOD allowed_methods_ = HTTP_METHOD::DEFAULT;
    string allowed_headers_ = "Content-Type, Cookie, Accept, X-Requested-With";

public:
    void set_allowed_origins(string origins) { allowed_origins_ = _MSTL move(origins); }
    void set_allowed_methods(HTTP_METHOD methods) { allowed_methods_ = _MSTL move(methods); }
    void set_allowed_headers(string headers) { allowed_headers_ = _MSTL move(headers); }

    void do_filter(http_request& request, http_response& response) override;
};

class MSTL_API logging_filter final : public filter {
public:
    bool pre_filter(http_request& request, http_response& response) override;
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

MSTL_END_NAMESPACE__
#endif //FILTER_HPP
