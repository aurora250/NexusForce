#ifndef NEFORCE_NETWORK_HTTP_FILTER_HPP__
#define NEFORCE_NETWORK_HTTP_FILTER_HPP__
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/http/http_server_message.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API http_filter {
public:
    virtual ~http_filter() = default;
    virtual void do_filter(http_request& request, http_response& response) = 0;
    virtual bool pre_filter(http_request& request, http_response& response) { return true; }
    virtual void post_filter(http_request& request, http_response& response) {}
    NEFORCE_NODISCARD virtual string name() const { return "http_filter"; }
};


class NEFORCE_API http_filter_chain {
private:
    vector<unique_ptr<http_filter>> filters_;
    bool owns_filters_ = true;

public:
    http_filter_chain() = default;

    explicit http_filter_chain(const bool owns_filters) :
    owns_filters_(owns_filters) {}

    http_filter_chain(const http_filter_chain&) = delete;
    http_filter_chain& operator=(const http_filter_chain&) = delete;

    http_filter_chain(http_filter_chain&&) noexcept = default;
    http_filter_chain& operator=(http_filter_chain&&) noexcept = default;

    void add_filter(unique_ptr<http_filter> filter);
    void add_filter_ref(http_filter* filter);

    void clear() noexcept;

    NEFORCE_NODISCARD size_t size() const noexcept { return filters_.size(); }
    NEFORCE_NODISCARD bool empty() const noexcept { return filters_.empty(); }

    bool execute_pre_filters(http_request& request, http_response& response);
    void execute_post_filters(http_request& request, http_response& response);
    void execute_filters(http_request& request, http_response& response);
};


#ifdef DELETE
#    undef DELETE
#endif

class NEFORCE_API cors_filter final : public http_filter {
public:
    string allowed_origins;
    HTTP_METHOD allowed_methods{HTTP_METHOD::DEFAULT()};
    string allowed_headers{"Content-Type, Cookie, Accept, X-Requested-With"};
    bool allow_credentials = true;
    size_t max_age = 86400;

    cors_filter() = default;

    explicit cors_filter(string origins) :
    allowed_origins(_NEFORCE move(origins)) {}

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "cors_filter"; }
};

class NEFORCE_API logging_filter final : public http_filter {
public:
    bool log_headers = false;
    bool log_body = false;
    size_t max_body_log_size = 1024;

    bool pre_filter(http_request& request, http_response& response) override;
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "logging_filter"; }
};

class NEFORCE_API static_file_filter final : public http_filter {
public:
    string root_path_;
    unordered_map<string, HTTP_CONTENT> mime_types_;
    bool enable_cache_ = true;
    size_t max_file_size_ = 10 * 1024 * 1024;


    explicit static_file_filter(string root_path);

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "static_file_filter"; }

    NEFORCE_NODISCARD static bool is_safe_path(const string& path);

    NEFORCE_NODISCARD optional<HTTP_CONTENT> get_mime_type(const string& path) const;
    void add_mime_type(const string& extension, HTTP_CONTENT content_type);
};

class NEFORCE_API rate_limit_filter final : public http_filter {
private:
    struct rate_limit_info {
        size_t count = 0;
        datetime last_reset{datetime::now()};
    };
    unordered_map<string, rate_limit_info> client_requests_;
    mutex mutex_;

public:
    size_t max_requests = 100;
    seconds window_seconds{60};

public:
    explicit rate_limit_filter(const size_t max_requests = 100, const seconds window = seconds(60)) :
    max_requests(max_requests),
    window_seconds(window) {}

    void set_max_requests(const size_t max_request) { max_requests = max_request; }

    void set_window(const seconds window) { window_seconds = window; }

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    string name() const override { return "rate_limit_filter"; }

    void cleanup_old_entries();
};

class NEFORCE_API authentication_filter final : public http_filter {
private:
    vector<string> excluded_paths_;
    function<bool(const http_request&)> auth_validator_;

    bool is_path_excluded(const string& path) const;

public:
    authentication_filter() = default;

    explicit authentication_filter(function<bool(const http_request&)> validator) :
    auth_validator_(_NEFORCE move(validator)) {}

    void set_auth_validator(function<bool(const http_request&)> validator) {
        auth_validator_ = _NEFORCE move(validator);
    }

    void add_excluded_path(string path) { excluded_paths_.push_back(_NEFORCE move(path)); }

    void clear_excluded_paths() { excluded_paths_.clear(); }

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "authentication_filter"; }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_FILTER_HPP__
