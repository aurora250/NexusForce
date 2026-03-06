#include <NeForce/core/async/async.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/dns/dns_client.hpp>
#include <NeForce/network/socket/tcp_socket.hpp>
#include <NeForce/network/socket/udp_socket.hpp>
NEFORCE_BEGIN_NAMESPACE__

static uint16_t generate_dns_client_id() {
    thread_local random_mt tls_random;
    thread_local bool seeded = false;
    if (!seeded) {
        tls_random.set_seed(static_cast<uint32_t>(
            steady_clock::now().since_epoch().count() ^
            this_thread::id().native_handle()));
        seeded = true;
    }
    return tls_random.next_int(1, 65535);
}

static byte_vector encode_domain_name(const string_view domain) {
    byte_vector encoded;
    size_t start = 0;
    size_t pos;

    while ((pos = domain.find('.', start)) != string::npos) {
        const auto len = pos - start;
        if (len > 63) {
            throw_exception(dns_exception("Label too long in domain name"));
        }
        encoded.push_back(static_cast<byte_t>(len));
        encoded.insert(encoded.end(), domain.begin() + start, domain.begin() + pos);
        start = pos + 1;
    }

    if (start < domain.length()) {
        const auto len = domain.length() - start;
        if (len > 63) {
            throw_exception(dns_exception("Label too long in domain name"));
        }
        encoded.push_back(static_cast<byte_t>(len));
        encoded.insert(encoded.end(), domain.begin() + start, domain.end());
    }

    encoded.push_back(0);
    return encoded;
}

static byte_vector build_dns_query(const string_view domain, DNS_RECORD type, DNS_QUERY qclass) {
    byte_vector query;

    dns_header header;
    header.id = ::htons(generate_dns_client_id());
    header.flags = ::htons(0x0100);
    header.qdcount = ::htons(1);

    query.resize(sizeof(dns_header));
    memory_copy(query.data(), &header, sizeof(dns_header));

    auto encoded_domain = encode_domain_name(domain);
    query.insert(query.end(), encoded_domain.begin(), encoded_domain.end());

    uint16_t qtype = ::htons(static_cast<uint16_t>(type));
    uint16_t qclass_val = ::htons(static_cast<uint16_t>(qclass));

    query.insert(query.end(), reinterpret_cast<byte_t*>(&qtype),
        reinterpret_cast<byte_t*>(&qtype) + sizeof(qtype));
    query.insert(query.end(), reinterpret_cast<byte_t*>(&qclass_val),
        reinterpret_cast<byte_t*>(&qclass_val) + sizeof(qclass_val));

    return query;
}

static string decode_domain_name(const byte_vector& data, size_t& offset) {
    string name;
    bool jumped = false;
    size_t original_offset = offset;
    int jumps = 0;

    while (offset < data.size()) {
        const byte_t len = data[offset];

        if ((len & 0xC0) == 0xC0) {
            constexpr int MAX_JUMPS = 5;
            if (offset + 1 >= data.size()) {
                throw_exception(dns_exception("Invalid pointer in domain name"));
            }

            if (!jumped) {
                original_offset = offset + 2;
            }

            const uint16_t pointer = ((len & 0x3F) << 8) | data[offset + 1];
            offset = pointer;
            jumped = true;

            if (++jumps > MAX_JUMPS) {
                throw_exception(dns_exception("Too many jumps in domain name"));
            }
            continue;
        }

        if (len == 0) {
            offset++;
            break;
        }
        if (len > 63) {
            throw_exception(dns_exception("Invalid label length"));
        }
        if (offset + 1 + len > data.size()) {
            throw_exception(dns_exception("Domain name exceeds buffer"));
        }

        if (!name.empty()) {
            name += '.';
        }
        name.append(reinterpret_cast<const char*>(&data[offset + 1]), len);
        offset += 1 + len;
    }

    if (jumped) {
        offset = original_offset;
    }

    return name;
}

byte_vector dns_client::send_udp_query(const byte_vector& query) const {
    thread_local udp_socket tls_udp_socket;

    if (!tls_udp_socket.is_open()) {
        tls_udp_socket.open();
        if (!tls_udp_socket.is_open()) {
            throw_exception(dns_exception("Failed to create UDP socket"));
        }
    }

    if (!tls_udp_socket.set_receive_timeout(config_.timeout)) {
        throw_exception(dns_exception("Failed to set socket timeout"));
    }

    const ssize_t sent = tls_udp_socket.send_to(
        memory_view<const char>{
            reinterpret_cast<const char*>(query.data()),
            query.size()
        },
        *ip_address::parse(config_.server, config_.port));

    if (sent < 0 || static_cast<size_t>(sent) != query.size()) {
        throw_exception(dns_exception("Failed to send UDP query"));
    }

    byte_vector buffer(512);
    const auto received = tls_udp_socket.receive_from(
        memory_view<char>{reinterpret_cast<char*>(buffer.data()), buffer.size()});
    if (received.first < 0) {
        throw_exception(dns_exception("UDP query timeout or receive error"));
    }
    buffer.resize(received.first);
    return buffer;
}

byte_vector dns_client::send_tcp_query(const byte_vector& query) const {
    thread_local struct tcp_socket_state {
        tcp_socket socket;
        string server;
        int port = 0;
    } tls_tcp_state;

    auto connect = [&] {
        tls_tcp_state.socket.close();
        tls_tcp_state.socket.open();
        if (!tls_tcp_state.socket.is_open()) {
            throw_exception(dns_exception("Failed to create TCP socket"));
        }
        if (!tls_tcp_state.socket.set_receive_timeout(config_.timeout) ||
            !tls_tcp_state.socket.set_send_timeout(config_.timeout)) {
            tls_tcp_state.socket.close();
            throw_exception(dns_exception("Failed to set socket timeout"));
        }
        tls_tcp_state.socket.connect(*ip_address::parse(config_.server, config_.port));
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
        const uint16_t length = ::htons(static_cast<uint16_t>(query.size()));
        if (tls_tcp_state.socket.send(memory_view<const char>{reinterpret_cast<const char*>(&length), 2}) != 2) {
            return false;
        }
        if (tls_tcp_state.socket.send(memory_view<const char>{
                reinterpret_cast<const char*>(query.data()), query.size()
            }) != static_cast<ssize_t>(query.size())) {
            return false;
            }
        return true;
    };

    if (!send_request()) {
        try {
            connect();
        } catch (...) {
            throw_exception(dns_exception("Failed to reconnect TCP socket"));
        }
        if (!send_request()) {
            throw_exception(dns_exception("Failed to send query data after reconnect"));
        }
    }

    uint16_t res_len;
    if (tls_tcp_state.socket.receive(memory_view<char>{reinterpret_cast<char*>(&res_len), 2}) != 2) {
        throw_exception(dns_exception("Failed to receive response length"));
    }
    res_len = ::ntohs(res_len);

    byte_vector buffer(res_len);
    size_t total = 0;
    while (total < res_len) {
        const ssize_t received = tls_tcp_state.socket.receive(memory_view<char>{
            reinterpret_cast<char*>(buffer.data() + total),
            res_len - total
        });
        if (received <= 0) {
            throw_exception(dns_exception("Failed to receive complete response"));
        }
        total += received;
    }

    return buffer;
}

static string parse_a_record(const byte_vector& rdata) {
    if (rdata.size() != 4) {
        throw_exception(dns_exception("Invalid A record length"));
    }
    char ip[INET_ADDRSTRLEN];
    if (::inet_ntop(AF_INET, rdata.data(), ip, INET_ADDRSTRLEN) == nullptr) {
        throw_exception(dns_exception("Failed to parse A record"));
    }
    return {ip};
}

static string parse_aaaa_record(const byte_vector& rdata) {
    if (rdata.size() != 16) {
        throw_exception(dns_exception("Invalid AAAA record length"));
    }
    char ip[INET6_ADDRSTRLEN];
    if (::inet_ntop(AF_INET6, rdata.data(), ip, INET6_ADDRSTRLEN) == nullptr) {
        throw_exception(dns_exception("Failed to parse AAAA record"));
    }
    return {ip};
}

static string parse_mx_record(const byte_vector& data, size_t offset, const uint16_t rdlength) {
    if (rdlength < 2) {
        throw_exception(dns_exception("Invalid MX record length"));
    }
    const uint16_t preference = ::ntohs(*reinterpret_cast<const uint16_t*>(&data[offset]));
    offset += 2;
    const string exchange = decode_domain_name(data, offset);
    return to_string(preference) + " " + exchange;
}

static string parse_txt_record(const byte_vector& rdata) {
    string result;
    size_t offset = 0;

    while (offset < rdata.size()) {
        const byte_t len = rdata[offset++];
        if (offset + len > rdata.size()) {
            throw_exception(dns_exception("Invalid TXT record"));
        }

        result.append(reinterpret_cast<const char*>(&rdata[offset]), len);
        offset += len;
    }

    return result;
}

static dns_record parse_resource_record(const byte_vector& data, size_t& offset) {
    dns_record record;
    record.name = decode_domain_name(data, offset);

    if (offset + 10 > data.size()) {
        throw_exception(dns_exception("Incomplete resource record"));
    }

    record.type = static_cast<DNS_RECORD>(::ntohs(*reinterpret_cast<const uint16_t*>(&data[offset])));
    offset += 2;
    record.class_type = static_cast<DNS_QUERY>(::ntohs(*reinterpret_cast<const uint16_t*>(&data[offset])));
    offset += 2;
    record.ttl = ::ntohl(*reinterpret_cast<const uint32_t*>(&data[offset]));
    offset += 4;
    const uint16_t rdlength = ::ntohs(*reinterpret_cast<const uint16_t*>(&data[offset]));
    offset += 2;

    if (offset + rdlength > data.size()) {
        throw_exception(dns_exception("RDATA exceeds buffer"));
    }

    byte_vector rdata(data.begin() + offset, data.begin() + offset + rdlength);
    size_t rdata_offset = offset;

    switch (record.type) {
        case DNS_RECORD::A: {
            record.data = parse_a_record(rdata);
            break;
        } case DNS_RECORD::AAAA: {
            record.data = parse_aaaa_record(rdata);
            break;
        } case DNS_RECORD::CNAME: case DNS_RECORD::NS: case DNS_RECORD::PTR: {
            record.data = decode_domain_name(data, rdata_offset);
            break;
        } case DNS_RECORD::MX: {
            record.data = parse_mx_record(data, rdata_offset, rdlength);
            break;
        } case DNS_RECORD::TXT: {
            record.data = parse_txt_record(rdata);
            break;
        } default: {
            record.data = "";
            for (const byte_t byte : rdata) {
                record.data += format("{:02x}", byte);
            }
            break;
        }
    }

    offset += rdlength;
    return record;
}

static dns_query_result parse_dns_response(const byte_vector& response) {
    if (response.size() < sizeof(dns_header)) {
        throw_exception(dns_exception("Response too short"));
    }

    dns_query_result result;
    dns_header header;
    memory_copy(&header, response.data(), sizeof(dns_header));

    header.id = ::ntohs(header.id);
    header.flags = ::ntohs(header.flags);
    header.qdcount = ::ntohs(header.qdcount);
    header.ancount = ::ntohs(header.ancount);
    header.nscount = ::ntohs(header.nscount);
    header.arcount = ::ntohs(header.arcount);

    result.response_code = static_cast<DNS_RESPONSE>(header.flags & 0x000F);
    result.truncated = (header.flags & 0x0200) != 0;
    result.recursive_available = (header.flags & 0x0080) != 0;

    size_t offset = sizeof(dns_header);
    for (uint16_t i = 0; i < header.qdcount; ++i) {
        decode_domain_name(response, offset);
        offset += 4;
    }

    for (uint16_t i = 0; i < header.ancount; ++i) {
        result.answers.push_back(parse_resource_record(response, offset));
    }
    for (uint16_t i = 0; i < header.nscount; ++i) {
        result.authorities.push_back(parse_resource_record(response, offset));
    }
    for (uint16_t i = 0; i < header.arcount; ++i) {
        result.additional.push_back(parse_resource_record(response, offset));
    }

    return result;
}

static string create_cache_key(const string_view domain, DNS_RECORD type, DNS_QUERY qclass) {
    return domain + "_"_s + to_string(static_cast<int>(type)) + "_" + to_string(static_cast<int>(qclass));
}

dns_client::dns_client(config cfg, const bool use_tcp)
: config_(move(cfg)), use_tcp_(use_tcp) {}

dns_query_result dns_client::query(const string_view domain, const DNS_RECORD type, const DNS_QUERY qclass) {
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
        const auto result = parse_dns_response(response);
        if (result.truncated) {
            response = send_tcp_query(query_data);
        }
    }

    auto result = parse_dns_response(response);
    const auto end_time = steady_clock::now();
    result.query_time = time_cast<milliseconds>(end_time - start_time);

    update_cache(cache_key, result);

    return result;
}

future<dns_query_result> dns_client::query_async(const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    return async(launch::async, [this, domain, type, qclass] {
        return query(domain.view(), type, qclass);
    });
}

vector<string> dns_client::resolve_a(const string_view domain) {
    const auto result = query(domain, DNS_RECORD::A);
    vector<string> ips;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::A) {
            ips.push_back(record.data);
        }
    }

    return ips;
}

vector<string> dns_client::resolve_aaaa(const string_view domain) {
    const auto result = query(domain, DNS_RECORD::AAAA);
    vector<string> ips;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::AAAA) {
            ips.push_back(record.data);
        }
    }

    return ips;
}

vector<string> dns_client::resolve_cname(const string_view domain) {
    const auto result = query(domain, DNS_RECORD::CNAME);
    vector<string> cnames;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::CNAME) {
            cnames.push_back(record.data);
        }
    }

    return cnames;
}

vector<string> dns_client::resolve_mx(const string_view domain) {
    const auto result = query(domain, DNS_RECORD::MX);
    vector<string> mx_records;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::MX) {
            mx_records.push_back(record.data);
        }
    }

    return mx_records;
}

vector<string> dns_client::resolve_txt(const string_view domain) {
    const auto result = query(domain, DNS_RECORD::TXT);
    vector<string> txt_records;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::TXT) {
            txt_records.push_back(record.data);
        }
    }
    return txt_records;
}

string dns_client::reverse_query(const string_view ip) {
    string reverse_domain;

    if (ip.find(':') != string::npos) {
        throw_exception(dns_exception("IPv6 reverse query not fully implemented"));
    } else {
        vector<string_view> parts;
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
            throw_exception(dns_exception("Invalid IPv4 address"));
        }
        reverse_domain = parts[3] + "."_s + parts[2] + "." + parts[1] + "." + parts[0] + ".in-addr.arpa";
    }

    const auto result = query(reverse_domain.view(), DNS_RECORD::PTR);
    if (!result.answers.empty() && result.answers[0].type == DNS_RECORD::PTR) {
        return result.answers[0].data;
    }
    return "";
}

vector<dns_query_result> dns_client::batch_query(const vector<string>& domains, const DNS_RECORD type) {
    vector<future<dns_query_result>> futures;

    for (const auto& domain : domains) {
        futures.push_back(query_async(domain, type));
    }

    vector<dns_query_result> results;
    for (auto& future : futures) {
        try {
            results.push_back(future.get());
        } catch (...) {
            dns_query_result failed_result;
            failed_result.response_code = DNS_RESPONSE::SERVER_FAILURE;
            results.push_back(failed_result);
        }
    }

    return results;
}

optional<dns_query_result> dns_client::check_cache(const string& key) {
    const auto it = cache_.find(key);
    if (it != cache_.end()) {
        const auto now = steady_clock::now();
        const auto cache_age = time_cast<seconds>(now - it->second.second);

        if (cache_age < cache_ttl_) {
            return dns_query_result(it->second.first);
        }
        cache_.erase(it);
    }
    return none;
}

void dns_client::update_cache(const string& key, const dns_query_result& result) {
    if (result.is_success()) {
        cache_[key] = {result, steady_clock::now()};
    }
}

NEFORCE_END_NAMESPACE__
