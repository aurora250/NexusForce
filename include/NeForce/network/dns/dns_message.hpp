#ifndef NEFORCE_NETWORK_DNS_MESSAGE_HPP
#define NEFORCE_NETWORK_DNS_MESSAGE_HPP
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
#include "NeForce/core/time/duration.hpp"
NEFORCE_BEGIN_NAMESPACE__

enum class dns_opcode : uint8_t {
    QUERY = 0,  // 标准查询
    IQUERY = 1, // 反向查询
    STATUS = 2, // 服务器状态请求
    NOTIFY = 4, // 区域通知
    UPDATE = 5  // 区域更新
};

enum class dns_query : uint16_t {
    INTERNET = 1,
    CHAOS = 3,
    HESIOD = 4,
    ANY = 255
};

enum class dns_response : uint8_t {
    NON_ERROR = 0,
    FORMAT_ERROR = 1,
    SERVER_FAILURE = 2,
    NAME_ERROR = 3,
    NOT_IMPLEMENTED = 4,
    REFUSED = 5
};


class dns_exception final : public network_exception {
public:
    enum class code {
        TIMEOUT,
        NETWORK_ERROR,
        PARSE_ERROR,
        SERVER_FAILURE,
        TRUNCATED,
        NO_RECORD
    };

    explicit dns_exception(const string& what) :
    network_exception(what.data()) {}

    dns_exception(const string& what, const code code) :
    network_exception(what.data(), static_type, static_cast<int>(code)) {}

    static dns_exception timeout() { return dns_exception("DNS query timeout", code::TIMEOUT); }

    static dns_exception network_error(const string& detail, const code code = code::NETWORK_ERROR) {
        return dns_exception("Network error: " + detail, code);
    }

    static dns_exception parse_error(const string& detail) {
        return dns_exception("Parse error: " + detail, code::PARSE_ERROR);
    }

    static constexpr auto static_type = "dns_exception";
};


struct dns_record {
    enum raw : uint16_t {
        A = 1,     // IPv4地址
        NS = 2,    // 名称服务器
        CNAME = 5, // 规范名称
        SOA = 6,   // 授权开始
        PTR = 12,  // 指针记录
        MX = 15,   // 邮件交换
        TXT = 16,  // 文本记录
        AAAA = 28, // IPv6地址
        SRV = 33   // 服务记录
    };

    string name;
    string data;
    uint32_t ttl;
    raw type{raw::A};
    dns_query class_type;

    dns_record() = default;

    dns_record(string n, const raw t, const dns_query c, const uint32_t ttl_val, string d) noexcept :
    name(_NEFORCE move(n)),
    data(_NEFORCE move(d)),
    ttl(ttl_val),
    type(t),
    class_type(c) {}
};


struct dns_query_result {
    vector<dns_record> answers;
    vector<dns_record> authorities;
    vector<dns_record> additional;
    milliseconds query_time;
    dns_response response_code{dns_response::NON_ERROR};
    bool truncated;
    bool recursive_available;

    NEFORCE_NODISCARD bool is_success() const noexcept { return response_code == dns_response::NON_ERROR; }
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
