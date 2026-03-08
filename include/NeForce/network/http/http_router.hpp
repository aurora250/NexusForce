#ifndef NEFORCE_NETWORK_HTTP_ROUTER_HPP__
#define NEFORCE_NETWORK_HTTP_ROUTER_HPP__
#include "NeForce/core/string/regex.hpp"
#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API http_router {
public:
    using http_handler_t = function<void(http_request&, http_response&)>;
    using exception_handler_t = function<void(http_request&, http_response&, const exception&)>;

private:
    struct route_entry {
        string pattern;
        http_handler_t handler;
        optional<regex> regex_pattern;
        vector<string> param_names;
        bool is_regex = false;
    };

    unordered_map<string, vector<route_entry>> routes_;
    http_filter_chain middleware_chain_;

    http_handler_t not_found_handler_;
    http_handler_t method_not_allowed_handler_;
    exception_handler_t exception_handler_;

public:
    bool case_sensitive = true;
    bool strict_routing = false;

private:
    route_entry* find_handler(const HTTP_METHOD& method, const string& path, http_request& request);
    void setup_default_handlers();

public:
    http_router();
    ~http_router() = default;

    http_router(const http_router&) = delete;
    http_router& operator=(const http_router&) = delete;
    http_router(http_router&&) noexcept = default;
    http_router& operator=(http_router&&) noexcept = default;

    void get(const string& path, http_handler_t handler);
    void post(const string& path, http_handler_t handler);
    void put(const string& path, http_handler_t handler);
    void del(const string& path, http_handler_t handler);
    void head(const string& path, http_handler_t handler);
    void options(const string& path, http_handler_t handler);
    void trace(const string& path, http_handler_t handler);
    void connect(const string& path, http_handler_t handler);
    void patch(const string& path, http_handler_t handler);

    void get_post(const string& path, http_handler_t handler);
    void post_delete(const string& path, http_handler_t handler);
    void all(const string& path, http_handler_t handler);

    void route(const HTTP_METHOD& method, const string& path, http_handler_t handler);

    void use(unique_ptr<http_filter> middleware) {
        middleware_chain_.add_filter(_NEFORCE move(middleware));
    }

    http_filter_chain& middleware_chain() noexcept {
        return middleware_chain_;
    }

    void set_not_found_handler(http_handler_t handler) {
        not_found_handler_ = _NEFORCE move(handler);
    }

    void set_method_not_allowed_handler(http_handler_t handler) {
        method_not_allowed_handler_ = _NEFORCE move(handler);
    }

    void set_exception_handler(exception_handler_t handler) {
        exception_handler_ = _NEFORCE move(handler);
    }

    http_response handle_request(http_request& request);

    NEFORCE_NODISCARD bool has_route(const HTTP_METHOD& method, const string& path) const;
    NEFORCE_NODISCARD size_t route_count() const noexcept;

    void clear_routes() noexcept {
        routes_.clear();
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_ROUTER_HPP__
