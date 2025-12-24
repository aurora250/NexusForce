#ifndef MSTL_NETWORK_DNS_CLIENT_HPP__
#define MSTL_NETWORK_DNS_CLIENT_HPP__
#include "../../core/utility/optional.hpp"
#include "MSTL/core/async/future.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/numeric/random.hpp"
#include "MSTL/core/time/clocks.hpp"
#include "dns_message.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <netinet/in.h>
#else
#include <ws2def.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(dns_exception, exception, "DNS Operate Failed");


class MSTL_API dns_client {
private:
    string dns_server_;
    uint16_t dns_port_;
    milliseconds timeout_;
    bool use_tcp_;
    unordered_map<string, pair<dns_query_result, steady_clock::time_point>> cache_;
    seconds cache_ttl_{300};

private:
    static vector<byte_t> build_dns_query(const string& domain, DNS_RECORD type, DNS_QUERY qclass);
    static vector<byte_t> encode_domain_name(const string& domain);
    static string decode_domain_name(const vector<byte_t>& data, size_t& offset);

    vector<byte_t> send_udp_query(const vector<byte_t>& query)const;
    vector<byte_t> send_tcp_query(const vector<byte_t>& query)const;

    static dns_query_result parse_dns_response(const vector<byte_t>& response);
    static uint16_t generate_query_id() { return random_mt::next_int(1, 65535); }

    optional<dns_query_result> check_cache(const string& key);
    void update_cache(const string& key, const dns_query_result& result);
    static string create_cache_key(const string& domain, DNS_RECORD type, DNS_QUERY qclass);

    static dns_record parse_resource_record(const vector<byte_t>& data, size_t &offset);
    static string parse_a_record(const vector<byte_t>& rdata);
    static string parse_aaaa_record(const vector<byte_t>& rdata);
    static string parse_mx_record(const vector<byte_t>& data, size_t offset, uint16_t rdlength);
    static string parse_txt_record(const vector<byte_t>& rdata);

public:
    explicit dns_client(
        string dns_server = "8.8.8.8",
        uint16_t dns_port = 53,
        milliseconds timeout = milliseconds(5000),
        bool use_tcp = false);

    void set_dns_server(const string& server, uint16_t port = 53);

    void set_timeout(const milliseconds timeout) { timeout_ = timeout; }
    void set_use_tcp(const bool use_tcp) { use_tcp_ = use_tcp; }
    void set_cache_ttl(const seconds ttl) { cache_ttl_ = ttl; }

    void clear_cache() { cache_.clear(); }

    dns_query_result query(
        const string& domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    _MSTL future<dns_query_result> query_async(
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

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_DNS_CLIENT_HPP__
