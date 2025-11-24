#ifndef MSTL_NETWORK_HTTP_FILTER_HPP__
#define MSTL_NETWORK_HTTP_FILTER_HPP__
#include "http_message.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API http_filter {
public:
    virtual ~http_filter() = default;
    virtual void do_filter(http_request& request, http_response& response) = 0;
    virtual bool pre_filter(http_request& request, http_response& response) { return true; }
    virtual void post_filter(http_request& request, http_response& response) {}
};


class MSTL_API http_filter_chain {
private:
    vector<http_filter*> filters_;

public:
    void add_filter(http_filter* filter) { filters_.push_back(filter); }
    void clean_filter() noexcept;

    bool execute_pre_filters(http_request& request, http_response& response);
    void execute_post_filters(http_request& request, http_response& response);
    void execute_filters(http_request& request, http_response& response);
};


#ifdef DELETE
#undef DELETE
#endif

class MSTL_API cors_filter final : public http_filter {
private:
    string allowed_origins_ = "*";
    HTTP_METHOD allowed_methods_ = HTTP_METHOD::DEFAULT;
    string allowed_headers_ = "Content-Type, Cookie, Accept, X-Requested-With";
    bool allow_credentials_ = true;
    size_t max_age_ = 86400;

public:
    cors_filter() = default;

    explicit cors_filter(string origins,
        HTTP_METHOD methods = HTTP_METHOD::GET & HTTP_METHOD::POST & HTTP_METHOD::OPTIONS,
        string headers = "Content-Type, Cookie, Accept, X-Requested-With",
        const bool credentials = true,
        const size_t max_age = 86400) :
    allowed_origins_(_MSTL move(origins)), allowed_methods_(_MSTL move(methods)),
    allowed_headers_(_MSTL move(headers)), allow_credentials_(credentials),
    max_age_(max_age) {}

    void set_allowed_origins(string origins) { allowed_origins_ = _MSTL move(origins); }
    void set_allowed_methods(HTTP_METHOD methods) { allowed_methods_ = _MSTL move(methods); }
    void set_allowed_headers(string headers) { allowed_headers_ = _MSTL move(headers); }
    void set_allowed_credentials(const bool credentials) { allow_credentials_ = credentials; }
    void set_max_age(const size_t max_age) { max_age_ = max_age; }

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

class MSTL_API logging_filter final : public http_filter {
public:
    bool pre_filter(http_request& request, http_response& response) override;
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

class MSTL_API static_file_filter final : public http_filter {
private:
    string root_path_;
    unordered_map<string, HTTP_CONTENT> mime_types_;

public:
    explicit static_file_filter(string root_path);

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_FILTER_HPP__
