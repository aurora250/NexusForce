#ifndef MSTL_DNS_CLIENT_HPP__
#define MSTL_DNS_CLIENT_HPP__
#include <netinet/in.h>
#include <arpa/inet.h>
#include <future>
#include "MSTL/core/string.hpp"
#include "MSTL/core/vector.hpp"
#include "MSTL/core/optional.hpp"
#include "MSTL/core/unordered_map.hpp"
#include "socket.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(DNSError, LinkError, "DNS Operate Failed");


enum class DNS_RECORD : uint16_t {
    A = 1,       // IPv4地址
    NS = 2,      // 名称服务器
    CNAME = 5,   // 规范名称
    SOA = 6,     // 授权开始
    PTR = 12,    // 指针记录
    MX = 15,     // 邮件交换
    TXT = 16,    // 文本记录
    AAAA = 28,   // IPv6地址
    SRV = 33     // 服务记录
};

enum class DNS_QUERY : uint16_t {
    INTERNET = 1,
    CHAOS = 3,
    HESIOD = 4,
    ANY = 255
};

enum class DNS_RESPONSE : uint8_t {
    NO_ERROR = 0,
    FORMAT_ERROR = 1,
    SERVER_FAILURE = 2,
    NAME_ERROR = 3,
    NOT_IMPLEMENTED = 4,
    REFUSED = 5
};


struct dns_record {
    string name;
    DNS_RECORD type;
    DNS_QUERY class_type;
    uint32_t ttl;
    string data;

    dns_record() = default;
    dns_record(const string& n,
        const DNS_RECORD t, const DNS_QUERY c,
        const uint32_t ttl_val, const string& d)
    : name(n), type(t), class_type(c), ttl(ttl_val), data(d) {}
};

struct dns_query_result {
    vector<dns_record> answers;
    vector<dns_record> authorities;
    vector<dns_record> additional;
    DNS_RESPONSE response_code;
    bool truncated;
    bool recursive_available;
    chrono::milliseconds query_time;

    bool is_success() const {
        return response_code == DNS_RESPONSE::NO_ERROR;
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


class dns_client {
private:
    string dns_server_;
    uint16_t dns_port_;
    chrono::milliseconds timeout_;
    bool use_tcp_;
    unordered_map<string, pair<dns_query_result, chrono::steady_clock::time_point>> cache_;
    chrono::seconds cache_ttl_{300};

private:
    vector<uint8_t> build_dns_query(const string& domain, DNS_RECORD type, DNS_QUERY qclass)const;
    vector<uint8_t> encode_domain_name(const string& domain)const;
    string decode_domain_name(const vector<uint8_t>& data, size_t& offset)const;

    vector<uint8_t> send_udp_query(const vector<uint8_t>& query)const;
    vector<uint8_t> send_tcp_query(const vector<uint8_t>& query)const;

    dns_query_result parse_dns_response(const vector<uint8_t>& response)const;
    uint16_t generate_query_id() const { return random_mt::next_int(1, 65535); }

    optional<dns_query_result> check_cache(const string& key);
    void update_cache(const string& key, const dns_query_result& result);
    static string create_cache_key(const string& domain, DNS_RECORD type, DNS_QUERY qclass);

    ::sockaddr_in create_server_address() const;
    dns_record parse_resource_record(const vector<uint8_t>& data, size_t &offset)const;
    string parse_a_record(const vector<uint8_t>& rdata) const;
    string parse_aaaa_record(const vector<uint8_t>& rdata)const;
    string parse_mx_record(const vector<uint8_t>& data, size_t offset, uint16_t rdlength)const;
    string parse_txt_record(const vector<uint8_t>& rdata)const;

public:
    explicit dns_client(
        const string& dns_server = "8.8.8.8",
        const uint16_t dns_port = 53,
        const chrono::milliseconds timeout = chrono::milliseconds(5000),
        const bool use_tcp = false)
    : dns_server_(dns_server), dns_port_(dns_port), timeout_(timeout), use_tcp_(use_tcp) {}

    void set_dns_server(const string& server, const uint16_t port = 53) {
        dns_server_ = server;
        dns_port_ = port;
    }

    void set_timeout(const chrono::milliseconds timeout) { timeout_ = timeout; }
    void set_use_tcp(const bool use_tcp) { use_tcp_ = use_tcp; }
    void set_cache_ttl(const chrono::seconds ttl) { cache_ttl_ = ttl; }

    void clear_cache() { cache_.clear(); }

    dns_query_result query(
        const string& domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    std::future<dns_query_result> query_async(
        const string& domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    vector<string> resolve_a(const string& domain);
    vector<string> resolve_aaaa(const string& domain);
    vector<string> resolve_cname(const string& domain);
    vector<string> resolve_mx(const string& domain);
    vector<string> resolve_txt(const string& domain);
    string reverse_query(const string& ip);

    vector<dns_query_result> batch_query(const vector<string>& domains, DNS_RECORD type = DNS_RECORD::A);
};

inline unique_ptr<dns_client> create_dns_client(const string& dns_server = "8.8.8.8") {
    return make_unique<dns_client>(dns_server);
}

MSTL_END_NAMESPACE__
#endif // MSTL_DNS_CLIENT_HPP__
