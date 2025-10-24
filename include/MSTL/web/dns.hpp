#ifndef MSTL_DNS_CLIENT_HPP__
#define MSTL_DNS_CLIENT_HPP__
#include "MSTL/core/environment.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <netinet/in.h>
#include <future>
#include <chrono>
#include "socket.hpp"
#include "MSTL/core/string.hpp"
#include "MSTL/core/vector.hpp"
#include "MSTL/core/optional.hpp"
MSTL_BEGIN_NAMESPACE__

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


struct DNS_record {
    string name;
    DNS_RECORD type;
    DNS_QUERY class_type;
    uint32_t ttl;
    string data;
    
    DNS_record() = default;
    DNS_record(const string& n, DNS_RECORD t, DNS_QUERY c, uint32_t ttl_val, const string& d)
        : name(n), type(t), class_type(c), ttl(ttl_val), data(d) {}
};

struct DNS_query_result {
    vector<DNS_record> answers;
    vector<DNS_record> authorities;
    vector<DNS_record> additional;
    DNS_RESPONSE response_code;
    bool truncated;
    bool recursive_available;
    std::chrono::milliseconds query_time;
    
    bool is_success() const { return response_code == DNS_RESPONSE::NO_ERROR; }
};

MSTL_ERROR_BUILD_DERIVED_CLASS(DNSException, LinkError, "DNS Operate Failed");
MSTL_ERROR_BUILD_FINAL_CLASS(DnsTimeoutException, DNSException, "DNS Link Timeout");
MSTL_ERROR_BUILD_FINAL_CLASS(DnsParseException, DNSException, "DNS Parse Error");


class DNS_client {
private:
    string dns_server_;
    uint16_t dns_port_;
    std::chrono::milliseconds timeout_;
    bool use_tcp_;
    unordered_map<string, DNS_query_result> cache_;
    std::chrono::seconds cache_ttl_;

    struct DNS_header {
        uint16_t id;
        uint16_t flags;
        uint16_t qdcount;
        uint16_t ancount;
        uint16_t nscount;
        uint16_t arcount;
        
        DNS_header() : id(0), flags(0), qdcount(0), ancount(0), nscount(0), arcount(0) {}
    };

    vector<uint8_t> buildDnsQuery(const string& domain, DNS_RECORD type, DNS_QUERY qclass);

    vector<uint8_t> encodeDomainName(const string& domain);

    string decodeDomainName(const vector<uint8_t>& data, size_t& offset);

    vector<uint8_t> sendUdpQuery(const vector<uint8_t>& query);
    vector<uint8_t> sendTcpQuery(const vector<uint8_t>& query);

    DNS_query_result parseDnsResponse(const vector<uint8_t>& response);

    uint16_t generateQueryId();

    optional<DNS_query_result> checkCache(const string& key);

    void updateCache(const string& key, const DNS_query_result& result);

    static string createCacheKey(const string& domain, DNS_RECORD type, DNS_QUERY qclass);

public:
    explicit DNS_client(
        const string& dns_server = "8.8.8.8",
        uint16_t dns_port = 53,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000),
        bool use_tcp = false)
    : dns_server_(dns_server), dns_port_(dns_port), timeout_(timeout),
    use_tcp_(use_tcp), cache_ttl_(std::chrono::seconds(300)) {}

    void setDnsServer(const string& server, uint16_t port = 53) {
        dns_server_ = server;
        dns_port_ = port;
    }

    void setTimeout(std::chrono::milliseconds timeout) {
        timeout_ = timeout;
    }

    void setUseTcp(bool use_tcp) {
        use_tcp_ = use_tcp;
    }

    void setCacheTtl(std::chrono::seconds ttl) {
        cache_ttl_ = ttl;
    }

    void clearCache() {
        cache_.clear();
    }

    DNS_query_result query(
        const string& domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    std::future<DNS_query_result> queryAsync(
        const string& domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    vector<string> resolveA(const string& domain);
    vector<string> resolveAAAA(const string& domain);
    vector<string> resolveCNAME(const string& domain);
    vector<string> resolveMX(const string& domain);
    vector<string> resolveTXT(const string& domain);
    string reverseQuery(const string& ip);

    vector<DNS_query_result> batchQuery(const vector<string>& domains, DNS_RECORD type = DNS_RECORD::A);
};


inline vector<uint8_t> DNS_client::buildDnsQuery(
    const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    vector<uint8_t> query;

    DNS_header header;
    header.id = htons(generateQueryId());
    header.flags = htons(0x0100);  // 标准查询，递归
    header.qdcount = htons(1);     // 一个问题
    
    // 写入头部
    query.resize(sizeof(DNS_header));
    memory_copy(query.data(), &header, sizeof(DNS_header));
    
    // 编码域名
    auto encoded_domain = encodeDomainName(domain);
    query.insert(query.end(), encoded_domain.begin(), encoded_domain.end());
    
    // 查询类型和类别
    uint16_t qtype = htons(static_cast<uint16_t>(type));
    uint16_t qclass_val = htons(static_cast<uint16_t>(qclass));
    
    query.insert(query.end(), reinterpret_cast<uint8_t*>(&qtype), 
                 reinterpret_cast<uint8_t*>(&qtype) + sizeof(qtype));
    query.insert(query.end(), reinterpret_cast<uint8_t*>(&qclass_val), 
                 reinterpret_cast<uint8_t*>(&qclass_val) + sizeof(qclass_val));
    
    return query;
}

inline vector<uint8_t> DNS_client::encodeDomainName(const string& domain) {
    vector<uint8_t> encoded;
    string::size_type start = 0;
    string::size_type pos = 0;
    
    while ((pos = domain.find('.', start)) != string::npos) {
        auto len = pos - start;
        if (len > 63) {
            throw DNSException("Label too long in domain name");
        }
        encoded.push_back(static_cast<uint8_t>(len));
        encoded.insert(encoded.end(), domain.begin() + start, domain.begin() + pos);
        start = pos + 1;
    }
    
    // 处理最后一个标签
    if (start < domain.length()) {
        const auto len = domain.length() - start;
        if (len > 63) {
            throw DNSException("Label too long in domain name");
        }
        encoded.push_back(static_cast<uint8_t>(len));
        encoded.insert(encoded.end(), domain.begin() + start, domain.end());
    }
    
    encoded.push_back(0);  // 结束符
    return encoded;
}

inline DNS_query_result DNS_client::query(
    const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    auto start_time = std::chrono::steady_clock::now();
    
    // 检查缓存
    auto cache_key = createCacheKey(domain, type, qclass);
    auto cached = checkCache(cache_key);
    if (cached) {
        return *cached;
    }

        auto query_data = buildDnsQuery(domain, type, qclass);
        
        // 发送查询
        vector<uint8_t> response;
        if (use_tcp_) {
            response = sendTcpQuery(query_data);
        } else {
            response = sendUdpQuery(query_data);
        }
        
        // 解析响应
        auto result = parseDnsResponse(response);
        
        // 计算查询时间
        auto end_time = std::chrono::steady_clock::now();
        result.query_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        // 更新缓存
        updateCache(cache_key, result);
        
        return result;
}

inline std::future<DNS_query_result> DNS_client::queryAsync(
    const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    return std::async(std::launch::async, [this, domain, type, qclass] {
        return query(domain, type, qclass);
    });
}

inline vector<string> DNS_client::resolveA(const string& domain) {
    auto result = query(domain, DNS_RECORD::A);
    vector<string> ips;
    
    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::A) {
            ips.push_back(record.data);
        }
    }
    
    return ips;
}

inline uint16_t DNS_client::generateQueryId() {
    return random_mt::next_int(1, 65535);
}

inline string DNS_client::createCacheKey(
    const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    return domain + "_" + to_string(static_cast<int>(type)) + "_" + to_string(static_cast<int>(qclass));
}

inline unique_ptr<DNS_client> createDnsClient(const string& dns_server = "8.8.8.8") {
    return make_unique<DNS_client>(dns_server);
}

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DNS_CLIENT_HPP__
