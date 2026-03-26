#ifndef NEFORCE_NETWORK_DNS_CLIENT_HPP__
#define NEFORCE_NETWORK_DNS_CLIENT_HPP__
#include "NeForce/core/async/future.hpp"
#include "NeForce/core/async/shared_mutex.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/dns/dns_message.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API dns_client {
public:
    struct config {
        string server{"8.8.8.8"};
        int port{53};
        milliseconds timeout{5000};
    };

private:
    config config_{};
    unordered_map<string, pair<dns_query_result, steady_clock::time_point>> cache_;
    mutable shared_mutex cache_mutex_;
    seconds cache_ttl_{300};
    bool use_tcp_ = false;

private:
    byte_vector send_udp_query(const byte_vector& query) const;
    byte_vector send_tcp_query(const byte_vector& query) const;

    optional<dns_query_result> check_cache(const string& key);
    void update_cache(const string& key, const dns_query_result& result);

public:
    dns_client()
    : dns_client(config()) {}

    explicit dns_client(config cfg, bool use_tcp = false);

    void set_config(config cfg) { config_ = move(cfg); }
    void set_timeout(const milliseconds timeout) { config_.timeout = timeout; }
    void set_use_tcp(const bool use_tcp) { use_tcp_ = use_tcp; }
    void set_cache_ttl(const seconds ttl) { cache_ttl_ = ttl; }

    void clear_cache() { cache_.clear(); }

    dns_query_result query(
        string_view domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    future<dns_query_result> query_async(
        const string& domain,
        DNS_RECORD type = DNS_RECORD::A,
        DNS_QUERY qclass = DNS_QUERY::INTERNET
    );

    vector<string> resolve_a(string_view domain);
    vector<string> resolve_aaaa(string_view domain);
    vector<string> resolve_cname(string_view domain);
    vector<string> resolve_mx(string_view domain);
    vector<string> resolve_txt(string_view domain);
    string reverse_query(string_view ip);

    vector<dns_query_result> batch_query(const vector<string>& domains, DNS_RECORD type = DNS_RECORD::A);
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_DNS_CLIENT_HPP__
