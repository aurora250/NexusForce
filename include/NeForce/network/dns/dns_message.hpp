#ifndef NEFORCE_NETWORK_DNS_MESSAGE_HPP
#define NEFORCE_NETWORK_DNS_MESSAGE_HPP
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/time/duration.hpp"
#include "NeForce/network/dns/dns_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

class dns_exception final : public network_exception {
public:
    enum class Code {
        TIMEOUT,
        NETWORK_ERROR,
        PARSE_ERROR,
        SERVER_FAILURE,
        TRUNCATED,
        NO_RECORD
    };

    explicit dns_exception(const string& what)
    : network_exception(what.data()) {}

    dns_exception(const string& what, const Code code)
    : network_exception(what.data(), static_type, static_cast<int>(code)) {}

    static dns_exception timeout() {
        return dns_exception("DNS query timeout", Code::TIMEOUT);
    }

    static dns_exception network_error(const string& detail, const Code code = Code::NETWORK_ERROR) {
        return dns_exception("Network error: " + detail, code);
    }

    static dns_exception parse_error(const string& detail) {
        return dns_exception("Parse error: " + detail, Code::PARSE_ERROR);
    }

    static constexpr auto static_type = "dns_exception";
};


struct dns_record {
    string name;
    string data;
    uint32_t ttl;
    DNS_RECORD type;
    DNS_QUERY class_type;

    dns_record() = default;

    dns_record(
        string n,
        const DNS_RECORD t,
        const DNS_QUERY c,
        const uint32_t ttl_val,
        string d) noexcept
    : name(_NEFORCE move(n)), data(_NEFORCE move(d)),
      ttl(ttl_val), type(t), class_type(c) {}
};


struct dns_query_result {
    vector<dns_record> answers;
    vector<dns_record> authorities;
    vector<dns_record> additional;
    milliseconds query_time;
    DNS_RESPONSE response_code;
    bool truncated;
    bool recursive_available;

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
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_DNS_MESSAGE_HPP
