#ifndef MSTL_DNS_MESSAGE_HPP
#define MSTL_DNS_MESSAGE_HPP
#include "dns_constants.hpp"
#include "MSTL/core/vector.hpp"
#include "MSTL/core/chrono.hpp"
MSTL_BEGIN_NAMESPACE__

struct dns_record {
    string name;
    string data;
    uint32_t ttl;
    DNS_RECORD type;
    DNS_QUERY class_type;

    dns_record() = default;
    dns_record(string n,
        const DNS_RECORD t, const DNS_QUERY c,
        const uint32_t ttl_val, string d)
    : name(_MSTL move(n)), data(_MSTL move(d)), ttl(ttl_val), type(t), class_type(c) {}
};

struct dns_query_result {
    vector<dns_record> answers;
    vector<dns_record> authorities;
    vector<dns_record> additional;
    DNS_RESPONSE response_code;
    bool truncated;
    bool recursive_available;
    chrono::milliseconds query_time;

    bool is_success() const noexcept {
        return response_code == DNS_RESPONSE::NON_ERROR;
    }
};

struct dns_header {
    uint16_t id = 0;
    uint16_t flags = 0;
    uint16_t qdcount = 0;
    uint16_t ancount = 0;
    uint16_t nscount = 0;
    uint16_t arcount = 0;

    dns_header() = default;
};

MSTL_END_NAMESPACE__
#endif // MSTL_DNS_MESSAGE_HPP
