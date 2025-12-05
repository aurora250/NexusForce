#ifndef MSTL_NETWORK_URL_HPP__
#define MSTL_NETWORK_URL_HPP__
#include "MSTL/core/interface/istringify.hpp"
MSTL_BEGIN_NAMESPACE__

struct url : istringify<url> {
    string scheme;
    string host;
    uint16_t port = 0;
    string path;
    string query;

    url() = default;

    explicit url(const string_view url_str) { parse(url_str); }
    explicit url(const string& url_str) : url(url_str.view()) {}
    explicit url(const char* url_str) : url(string_view(url_str)) {}

    void parse(string_view str);

    MSTL_NODISCARD string to_string() const;
};

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_URL_HPP__
