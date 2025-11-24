#ifndef MSTL_NETWORK_HTTP_ROUTER_HPP__
#define MSTL_NETWORK_HTTP_ROUTER_HPP__
#include "MSTL/core/functional/function.hpp"
#include "filter.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API router {
public:
    using handler_func = function<void(http_request&, http_response&)>;

private:
    unordered_map<string, unordered_map<string, handler_func>> routes_;
    http_filter_chain middleware_chain_;

    handler_func not_found_handler_;
    handler_func method_not_allowed_handler_;

    handler_func* find_handler(const HTTP_METHOD& method, const string& path);
    void setup_default_handlers();
    static vector<string> split_methods(const string& method_str);

public:
    router();
    ~router() = default;

    void get(const string& path, handler_func handler);
    void post(const string& path, handler_func handler);
    void put(const string& path, handler_func handler);
    void del(const string& path, handler_func handler);
    void head(const string& path, handler_func handler);
    void options(const string& path, handler_func handler);
    void trace(const string& path, handler_func handler);
    void connect(const string& path, handler_func handler);
    void get_post(const string& path, handler_func handler);
    void post_delete(const string& path, handler_func handler);
    void all(const string& path, handler_func handler);

    void route(const HTTP_METHOD& method, const string& path, handler_func handler);

    void use(http_filter* middleware) { middleware_chain_.add_filter(middleware); }

    void set_not_found_handler(handler_func handler) { not_found_handler_ = _MSTL move(handler); }
    void set_method_not_allowed_handler(handler_func handler) { method_not_allowed_handler_ = _MSTL move(handler); }

    http_response handle_request(http_request& request);
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_HTTP_ROUTER_HPP__
