#ifndef MSTL_NETWORK_DNS_CLIENT_HPP__
#define MSTL_NETWORK_DNS_CLIENT_HPP__
#include "MSTL/core/compound/optional.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/time/clocks.hpp"
#include "MSTL/core/async/future.hpp"
#include "MSTL/core/numeric/random.hpp"
#include "dns_message.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <netinet/in.h>
#else
#include <ws2def.h>
#endif
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(dns_exception, link_exception, "DNS Operate Failed");


class MSTL_API dns_client {
private:
    string dns_server_;
    uint16_t dns_port_;
    _MSTL_CHRONO milliseconds timeout_;
    bool use_tcp_;
    unordered_map<string, pair<dns_query_result, _MSTL_CHRONO steady_clock::time_point>> cache_;
    _MSTL_CHRONO seconds cache_ttl_{300};

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

    ::sockaddr_in create_server_address() const;
    static dns_record parse_resource_record(const vector<byte_t>& data, size_t &offset);
    static string parse_a_record(const vector<byte_t>& rdata);
    static string parse_aaaa_record(const vector<byte_t>& rdata);
    static string parse_mx_record(const vector<byte_t>& data, size_t offset, uint16_t rdlength);
    static string parse_txt_record(const vector<byte_t>& rdata);

    static void ensure_winsock_initialized();

public:
    explicit dns_client(
        string dns_server = "8.8.8.8",
        const uint16_t dns_port = 53,
        const _MSTL_CHRONO milliseconds timeout = _MSTL_CHRONO milliseconds(5000),
        const bool use_tcp = false)
    : dns_server_(_MSTL move(dns_server)), dns_port_(dns_port), timeout_(timeout), use_tcp_(use_tcp) {}

    void set_dns_server(const string& server, const uint16_t port = 53) {
        dns_server_ = server;
        dns_port_ = port;
    }

    void set_timeout(const _MSTL_CHRONO milliseconds timeout) { timeout_ = timeout; }
    void set_use_tcp(const bool use_tcp) { use_tcp_ = use_tcp; }
    void set_cache_ttl(const _MSTL_CHRONO seconds ttl) { cache_ttl_ = ttl; }

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

MSTL_ALWAYS_INLINE_INLINE unique_ptr<dns_client>
make_dns_client(string dns_server = "8.8.8.8") {
    return _MSTL make_unique<dns_client>(_MSTL move(dns_server));
}

MSTL_END_NAMESPACE__
#endif // MSTL_NETWORK_DNS_CLIENT_HPP__
