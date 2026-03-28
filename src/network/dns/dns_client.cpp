#include <NeForce/core/async/async.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/dns/dns_client.hpp>
#include <NeForce/network/socket/tcp_socket.hpp>
#include <NeForce/network/socket/udp_socket.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#include <arpa/inet.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    uint16_t generate_dns_client_id() {
        thread_local random_mt tls_random;
        thread_local bool seeded = false;
        if (!seeded) {
            const auto seed =
                steady_clock::now().since_epoch().count() ^
                this_thread::id().native_handle();
            tls_random.set_seed(seed);
            seeded = true;
        }
        return tls_random.next_int(1, 65535);
    }

    byte_vector encode_domain_name(string_view domain) {
        while (!domain.empty() && domain.back() == '.') {
            domain.remove_suffix(1);
        }

        if (domain.empty() || domain.length() > 253) {
            NEFORCE_THROW_EXCEPTION(dns_exception("Invalid domain name length"));
        }

        byte_vector encoded;
        encoded.reserve(domain.length() + 2);

        size_t start = 0;
        size_t pos;

        while ((pos = domain.find('.', start)) != string::npos) {
            const auto len = pos - start;
            if (len == 0 || len > 63) {
                NEFORCE_THROW_EXCEPTION(dns_exception("Invalid label length in domain name"));
            }
            encoded.push_back(static_cast<byte_t>(len));
            encoded.insert(encoded.end(), domain.begin() + start, domain.begin() + pos);
            start = pos + 1;
        }

        if (start < domain.length()) {
            const auto len = domain.length() - start;
            if (len == 0 || len > 63) {
                NEFORCE_THROW_EXCEPTION(dns_exception("Invalid label length in domain name"));
            }
            encoded.push_back(static_cast<byte_t>(len));
            encoded.insert(encoded.end(), domain.begin() + start, domain.end());
        }

        encoded.push_back(0);
        return encoded;
    }

    byte_vector build_dns_query(const string_view domain, const dns_record::raw type, dns_query qclass) {
        byte_vector query;
        query.reserve(sizeof(dns_header) + domain.length() + 6);

        dns_header header;
        header.id = endian::host_to_network<uint16_t>(generate_dns_client_id());
        header.flags = endian::host_to_network<uint16_t>(0x0100);
        header.qdcount = endian::host_to_network<uint16_t>(1);

        query.resize(sizeof(dns_header));
        memory_copy(query.data(), &header, sizeof(dns_header));

        auto encoded_domain = encode_domain_name(domain);
        query.insert(query.end(), encoded_domain.begin(), encoded_domain.end());

        const uint16_t qtype = endian::host_to_network(static_cast<uint16_t>(type));
        const uint16_t qclass_val = endian::host_to_network(static_cast<uint16_t>(qclass));

        query.insert(
            query.end(),
            reinterpret_cast<const byte_t*>(&qtype),
            reinterpret_cast<const byte_t*>(&qtype) + sizeof(qtype));
        query.insert(
            query.end(),
            reinterpret_cast<const byte_t*>(&qclass_val),
            reinterpret_cast<const byte_t*>(&qclass_val) + sizeof(qclass_val));

        return query;
    }

    string decode_domain_name(const byte_vector& data, size_t& offset) {
        constexpr int max_jumps = 5;
        constexpr size_t max_name_length = 253;

        string name;
        name.reserve(64);

        bool jumped = false;
        size_t original_offset = offset;
        int jumps = 0;

        while (offset < data.size()) {
            const byte_t len = data[offset];

            if ((len & 0xC0) == 0xC0) {
                if (offset + 1 >= data.size()) {
                    NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid pointer in domain name"));
                }

                if (!jumped) {
                    original_offset = offset + 2;
                }

                const uint16_t pointer = ((len & 0x3F) << 8) | data[offset + 1];
                if (pointer >= data.size()) {
                    NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Pointer exceeds buffer"));
                }

                offset = pointer;
                jumped = true;

                if (++jumps > max_jumps) {
                    NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Too many jumps in domain name"));
                }
                continue;
            }

            if (len == 0) {
                offset++;
                break;
            }

            if (len > 63) {
                NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid label length"));
            }
            if (offset + 1 + len > data.size()) {
                NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Domain name exceeds buffer"));
            }

            if (!name.empty()) {
                name += '.';
            }

            if (name.length() + len + 1 > max_name_length) {
                NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Domain name too long"));
            }

            name.append(reinterpret_cast<const char*>(&data[offset + 1]), len);
            offset += 1 + len;
        }

        if (jumped) {
            offset = original_offset;
        }

        return name;
    }

    string parse_a_record(const byte_vector& rdata) {
        if (rdata.size() != 4) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid A record length"));
        }

        char ip[INET_ADDRSTRLEN];
        if (::inet_ntop(AF_INET, rdata.data(), ip, INET_ADDRSTRLEN) == nullptr) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Failed to parse A record"));
        }

        return {ip};
    }

    string parse_aaaa_record(const byte_vector& rdata) {
        if (rdata.size() != 16) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid AAAA record length"));
        }

        char ip[INET6_ADDRSTRLEN];
        if (::inet_ntop(AF_INET6, rdata.data(), ip, INET6_ADDRSTRLEN) == nullptr) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Failed to parse AAAA record"));
        }

        return {ip};
    }

    string parse_mx_record(const byte_vector& data, size_t offset, const uint16_t rdlength) {
        if (rdlength < 2) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid MX record length"));
        }
        if (offset + 2 > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("MX record exceeds buffer"));
        }

        const auto preference = endian::network_to_host<uint16_t>(*reinterpret_cast<const uint16_t*>(&data[offset]));
        offset += 2;

        const string exchange = decode_domain_name(data, offset);
        return _NEFORCE to_string(preference) + " " + exchange;
    }

    string parse_txt_record(const byte_vector& rdata) {
        string result;
        size_t offset = 0;

        while (offset < rdata.size()) {
            const byte_t len = rdata[offset++];

            if (offset + len > rdata.size()) {
                NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid TXT record"));
            }

            result.append(reinterpret_cast<const char*>(&rdata[offset]), len);
            offset += len;
        }

        return result;
    }

    dns_record parse_resource_record(const byte_vector& data, size_t& offset) {
        dns_record record;
        record.name = decode_domain_name(data, offset);

        if (offset + 10 > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Incomplete resource record"));
        }

        record.type = static_cast<dns_record::raw>(
            endian::network_to_host(*reinterpret_cast<const uint16_t*>(&data[offset])));
        offset += 2;

        record.class_type = static_cast<dns_query>(
            endian::network_to_host(*reinterpret_cast<const uint16_t*>(&data[offset])));
        offset += 2;

        record.ttl = endian::network_to_host(*reinterpret_cast<const uint32_t*>(&data[offset]));
        offset += 4;

        const auto rdlength = endian::network_to_host(*reinterpret_cast<const uint16_t*>(&data[offset]));
        offset += 2;

        if (offset + rdlength > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("RDATA exceeds buffer"));
        }

        byte_vector rdata(data.begin() + offset, data.begin() + offset + rdlength);
        size_t rdata_offset = offset;

        try {
            switch (record.type) {
                case dns_record::A: {
                    record.data = parse_a_record(rdata);
                    break;
                }
                case dns_record::AAAA: {
                    record.data = parse_aaaa_record(rdata);
                    break;
                }
                case dns_record::CNAME:
                case dns_record::NS:
                case dns_record::PTR: {
                    record.data = decode_domain_name(data, rdata_offset);
                    break;
                }
                case dns_record::MX: {
                    record.data = parse_mx_record(data, rdata_offset, rdlength);
                    break;
                }
                case dns_record::TXT: {
                    record.data = parse_txt_record(rdata);
                    break;
                }
                default: {
                    record.data = "";
                    for (const byte_t byte : rdata) {
                        record.data += format("{:02x}", byte);
                    }
                    break;
                }
            }
        } catch (...) {
            offset += rdlength;
            throw;
        }

        offset += rdlength;
        return record;
    }

    dns_query_result parse_dns_response(const byte_vector& response) {
        if (response.size() < sizeof(dns_header)) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Response too short"));
        }

        dns_query_result result;
        dns_header header;
        memory_copy(&header, response.data(), sizeof(dns_header));

        header.id = endian::network_to_host<uint16_t>(header.id);
        header.flags = endian::network_to_host<uint16_t>(header.flags);
        header.qdcount = endian::network_to_host<uint16_t>(header.qdcount);
        header.ancount = endian::network_to_host<uint16_t>(header.ancount);
        header.nscount = endian::network_to_host<uint16_t>(header.nscount);
        header.arcount = endian::network_to_host<uint16_t>(header.arcount);

        result.response_code = static_cast<dns_response>(header.flags & 0x000F);
        result.truncated = (header.flags & 0x0200) != 0;
        result.recursive_available = (header.flags & 0x0080) != 0;

        size_t offset = sizeof(dns_header);

        for (uint16_t i = 0; i < header.qdcount; ++i) {
            decode_domain_name(response, offset);
            if (offset + 4 > response.size()) {
                NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Question section exceeds buffer"));
            }
            offset += 4;
        }

        result.answers.reserve(header.ancount);
        for (uint16_t i = 0; i < header.ancount; ++i) {
            result.answers.push_back(parse_resource_record(response, offset));
        }

        result.authorities.reserve(header.nscount);
        for (uint16_t i = 0; i < header.nscount; ++i) {
            result.authorities.push_back(parse_resource_record(response, offset));
        }

        result.additional.reserve(header.arcount);
        for (uint16_t i = 0; i < header.arcount; ++i) {
            result.additional.push_back(parse_resource_record(response, offset));
        }

        return result;
    }

    string create_cache_key(const string_view domain, const dns_record::raw type, dns_query qclass) {
        return domain + "_"_s + to_string(static_cast<int>(type)) + "_" + to_string(static_cast<int>(qclass));
    }
}


byte_vector dns_client::send_udp_query(const byte_vector& query) const {
    thread_local struct udp_socket_state {
        udp_socket socket;
        milliseconds timeout{0};
        string server;
        ports port;
    } tls_udp_state;

    const bool config_changed = tls_udp_state.server != config_.server || tls_udp_state.port != config_.port;

    if (!tls_udp_state.socket.is_open() || config_changed) {
        tls_udp_state.socket.close();
        tls_udp_state.socket.open();
        if (!tls_udp_state.socket.is_open()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to create UDP socket"));
        }
        tls_udp_state.server = config_.server;
        tls_udp_state.port = config_.port;
        tls_udp_state.timeout = milliseconds(0);
    }

    if (tls_udp_state.timeout != config_.timeout) {
        if (!tls_udp_state.socket.set_receive_timeout(config_.timeout)) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to set socket timeout"));
        }
        tls_udp_state.timeout = config_.timeout;
    }

    const auto endpoint = ip_address::parse(config_.server, config_.port);
    if (!endpoint) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Invalid DNS server address"));
    }

    const ssize_t sent = tls_udp_state.socket.send_to(
        memory_view<const char>{
            reinterpret_cast<const char*>(query.data()),
            query.size()
        },
        *endpoint);

    if (sent < 0 || static_cast<size_t>(sent) != query.size()) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to send UDP query"));
    }

    byte_vector buffer(512);
    const auto received = tls_udp_state.socket.receive_from(
        memory_view<char>{reinterpret_cast<char*>(buffer.data()), buffer.size()});

    if (received.first < 0) {
        NEFORCE_THROW_EXCEPTION(dns_exception::timeout());
    }
    if (received.first == 0) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Empty response received"));
    }

    buffer.resize(received.first);
    return buffer;
}

byte_vector dns_client::send_tcp_query(const byte_vector& query) const {
    thread_local struct tcp_socket_state {
        tcp_socket socket;
        string server;
        ports port;
    } tls_tcp_state;

    auto connect = [&] {
        tls_tcp_state.socket.close();
        tls_tcp_state.socket.open();

        if (!tls_tcp_state.socket.is_open()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to create TCP socket"));
        }
        if (!tls_tcp_state.socket.set_receive_timeout(config_.timeout) ||
            !tls_tcp_state.socket.set_send_timeout(config_.timeout)) {
            tls_tcp_state.socket.close();
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to set socket timeout"));
        }

        const auto endpoint = ip_address::parse(config_.server, config_.port);
        if (!endpoint) {
            tls_tcp_state.socket.close();
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Invalid DNS server address"));
        }

        tls_tcp_state.socket.connect(*endpoint);
        tls_tcp_state.server = config_.server;
        tls_tcp_state.port = config_.port;
    };

    if (tls_tcp_state.socket.is_open()) {
        if (tls_tcp_state.server != config_.server || tls_tcp_state.port != config_.port) {
            connect();
        }
    } else {
        connect();
    }

    auto send_request = [&]() -> bool {
        if (query.size() > 65535) {
            return false;
        }

        const auto length = endian::host_to_network<uint16_t>(query.size());
        if (tls_tcp_state.socket.send(memory_view<const char>{
            reinterpret_cast<const char*>(&length), 2
        }) != 2) {
            return false;
        }

        if (tls_tcp_state.socket.send(memory_view<const char>{
                reinterpret_cast<const char*>(query.data()), query.size()
        }) != query.size()) {
            return false;
        }

        return true;
    };

    if (!send_request()) {
        try {
            connect();
        } catch (...) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to reconnect TCP socket"));
        }
        if (!send_request()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to send query data after reconnect"));
        }
    }

    uint16_t res_len;
    if (tls_tcp_state.socket.receive(memory_view<char>{reinterpret_cast<char*>(&res_len), 2}) != 2) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to receive response length"));
    }

    res_len = endian::network_to_host<uint16_t>(res_len);
    if (res_len == 0 || res_len > 65535) {
        NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid response length"));
    }

    byte_vector buffer(res_len);
    size_t total = 0;

    while (total < res_len) {
        const ssize_t received = tls_tcp_state.socket.receive(memory_view<char>{
            reinterpret_cast<char*>(buffer.data() + total),
            res_len - total
        });

        if (received <= 0) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to receive complete response"));
        }

        total += received;
    }

    return buffer;
}

dns_client::dns_client(config cfg, const bool use_tcp)
: config_(move(cfg)), use_tcp_(use_tcp) {
    if (config_.server.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("DNS server address cannot be empty"));
    }
    if (config_.timeout <= milliseconds(0)) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Timeout must be positive"));
    }
}

dns_query_result dns_client::query(const string_view domain, const dns_record::raw type, const dns_query qclass) {
    if (domain.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Domain name cannot be empty"));
    }

    const auto start_time = steady_clock::now();

    const auto cache_key = create_cache_key(domain, type, qclass);
    auto cached = check_cache(cache_key);
    if (cached) {
        return *cached;
    }

    const auto query_data = build_dns_query(domain, type, qclass);
    byte_vector response;

    if (use_tcp_) {
        response = send_tcp_query(query_data);
    } else {
        response = send_udp_query(query_data);
        auto result = parse_dns_response(response);

        if (result.truncated) {
            response = send_tcp_query(query_data);
        } else {
            const auto end_time = steady_clock::now();
            result.query_time = time_cast<milliseconds>(end_time - start_time);
            update_cache(cache_key, result);
            return result;
        }
    }

    auto result = parse_dns_response(response);
    const auto end_time = steady_clock::now();
    result.query_time = time_cast<milliseconds>(end_time - start_time);

    update_cache(cache_key, result);

    return result;
}

future<dns_query_result> dns_client::query_async(const string& domain, dns_record::raw type, dns_query qclass) {
    return async(launch::async, [this, domain, type, qclass] {
        return query(domain.view(), type, qclass);
    });
}

vector<string> dns_client::resolve_a(const string_view domain) {
    const auto result = query(domain, dns_record::A);
    vector<string> ips;
    ips.reserve(result.answers.size());

    for (const auto& record : result.answers) {
        if (record.type == dns_record::A) {
            ips.push_back(record.data);
        }
    }

    return ips;
}

vector<string> dns_client::resolve_aaaa(const string_view domain) {
    const auto result = query(domain, dns_record::AAAA);
    vector<string> ips;
    ips.reserve(result.answers.size());

    for (const auto& record : result.answers) {
        if (record.type == dns_record::AAAA) {
            ips.push_back(record.data);
        }
    }

    return ips;
}

vector<string> dns_client::resolve_cname(const string_view domain) {
    const auto result = query(domain, dns_record::CNAME);
    vector<string> cnames;
    cnames.reserve(result.answers.size());

    for (const auto& record : result.answers) {
        if (record.type == dns_record::CNAME) {
            cnames.push_back(record.data);
        }
    }

    return cnames;
}

vector<string> dns_client::resolve_mx(const string_view domain) {
    const auto result = query(domain, dns_record::MX);
    vector<string> mx_records;
    mx_records.reserve(result.answers.size());

    for (const auto& record : result.answers) {
        if (record.type == dns_record::MX) {
            mx_records.push_back(record.data);
        }
    }

    return mx_records;
}

vector<string> dns_client::resolve_txt(const string_view domain) {
    const auto result = query(domain, dns_record::TXT);
    vector<string> txt_records;
    txt_records.reserve(result.answers.size());

    for (const auto& record : result.answers) {
        if (record.type == dns_record::TXT) {
            txt_records.push_back(record.data);
        }
    }
    return txt_records;
}

string dns_client::reverse_query(const string_view ip) {
    if (ip.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("IP address cannot be empty"));
    }

    string reverse_domain;

    if (ip.find(':') != string::npos) {
        NEFORCE_THROW_EXCEPTION(dns_exception("IPv6 reverse query not fully implemented"));
    } else {
        vector<string_view> parts;
        parts.reserve(4);

        size_t start = 0;
        size_t pos;

        while ((pos = ip.find('.', start)) != string::npos) {
            parts.push_back(ip.view(start, pos - start));
            start = pos + 1;
        }

        if (start < ip.length()) {
            parts.push_back(ip.view(start));
        }

        if (parts.size() != 4) {
            NEFORCE_THROW_EXCEPTION(dns_exception("Invalid IPv4 address"));
        }

        reverse_domain = parts[3] + "."_s + parts[2] + "." + parts[1] + "." + parts[0] + ".in-addr.arpa";
    }

    const auto result = query(reverse_domain.view(), dns_record::PTR);

    if (!result.answers.empty() && result.answers[0].type == dns_record::PTR) {
        return result.answers[0].data;
    }
    return "";
}

vector<dns_query_result> dns_client::batch_query(const vector<string>& domains, const dns_record::raw type) {
    if (domains.empty()) {
        return {};
    }

    vector<future<dns_query_result>> futures;
    futures.reserve(domains.size());

    for (const auto& domain : domains) {
        futures.push_back(query_async(domain, type));
    }

    vector<dns_query_result> results;
    results.reserve(futures.size());

    for (auto& future : futures) {
        try {
            results.push_back(future.get());
        } catch (...) {
            dns_query_result failed_result;
            failed_result.response_code = dns_response::SERVER_FAILURE;
            results.push_back(failed_result);
        }
    }

    return results;
}

optional<dns_query_result> dns_client::check_cache(const string& key) {
    constexpr seconds negative_ttl{30};

    {
        shared_lock<shared_mutex> read_lock(cache_mutex_);
        const auto it = cache_.find(key);
        if (it != cache_.end()) {
            const auto now = steady_clock::now();
            const auto age = time_cast<seconds>(now - it->second.second);
            const auto& res = it->second.first;
            const auto max_ttl = res.is_success() ? cache_ttl_ : negative_ttl;
            if (age < max_ttl) {
                return optional<dns_query_result>{res};
            }
        } else {
            return none;
        }
    }

    lock<shared_mutex> write_lock(cache_mutex_);
    const auto it = cache_.find(key);
    if (it != cache_.end()) {
        const auto now = steady_clock::now();
        const auto age = time_cast<seconds>(now - it->second.second);
        const auto& res = it->second.first;
        const auto max_ttl = res.is_success() ? cache_ttl_ : negative_ttl;
        if (age >= max_ttl) {
            cache_.erase(it);
        } else {
            return optional<dns_query_result>{res};
        }
    }

    return none;
}

void dns_client::update_cache(const string& key, const dns_query_result& result) {
    if (result.is_success()) {
        lock<shared_mutex> write_lock(cache_mutex_);
        cache_[key] = {result, steady_clock::now()};
    }
}

NEFORCE_END_NAMESPACE__
