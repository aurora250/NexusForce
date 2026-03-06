#ifndef NEFORCE_NETWORK_HTTP_FILTER_HPP__
#define NEFORCE_NETWORK_HTTP_FILTER_HPP__
#include "NeForce/network/http/http_server_message.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API http_filter {
public:
    virtual ~http_filter() = default;
    virtual void do_filter(http_request& request, http_response& response) = 0;
    virtual bool pre_filter(http_request& request, http_response& response) { return true; }
    virtual void post_filter(http_request& request, http_response& response) {}
};


class NEFORCE_API http_filter_chain {
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

class NEFORCE_API cors_filter final : public http_filter {
private:
    string allowed_origins_;
    HTTP_METHOD allowed_methods_;
    string allowed_headers_;
    bool allow_credentials_;
    size_t max_age_;

public:
    cors_filter() = default;

    explicit cors_filter(string origins,
        HTTP_METHOD methods = HTTP_METHOD::DEFAULT,
        string headers = "Content-Type, Cookie, Accept, X-Requested-With",
        const bool credentials = true,
        const size_t max_age = 86400) :
    allowed_origins_(_NEFORCE move(origins)), allowed_methods_(_NEFORCE move(methods)),
    allowed_headers_(_NEFORCE move(headers)), allow_credentials_(credentials),
    max_age_(max_age) {}

    void set_allowed_origins(string origins) { allowed_origins_ = _NEFORCE move(origins); }
    void set_allowed_methods(HTTP_METHOD methods) { allowed_methods_ = _NEFORCE move(methods); }
    void set_allowed_headers(string headers) { allowed_headers_ = _NEFORCE move(headers); }
    void set_allowed_credentials(const bool credentials) { allow_credentials_ = credentials; }
    void set_max_age(const size_t max_age) { max_age_ = max_age; }

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

class NEFORCE_API logging_filter final : public http_filter {
public:
    bool pre_filter(http_request& request, http_response& response) override;
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

class NEFORCE_API static_file_filter final : public http_filter {
private:
    string root_path_;
    unordered_map<string, HTTP_CONTENT> mime_types_;

public:
    explicit static_file_filter(string root_path);

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_FILTER_HPP__
