#include <MSTL/core/utility/packages.hpp>
#include <MSTL/core/async/async.hpp>
#include <MSTL/core/string/vsprintf.hpp>
#include <MSTL/network/dns/dns_client.hpp>
#include <MSTL/network/socket.hpp>
#ifdef MSTL_PLATFORM_LINUX__
#include <arpa/inet.h>
#endif
MSTL_BEGIN_NAMESPACE__

vector<byte_t> dns_client::build_dns_query(const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    vector<byte_t> query;

    dns_header header;
    header.id = ::htons(generate_query_id());
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

vector<byte_t> dns_client::encode_domain_name(const string& domain) {
    vector<byte_t> encoded;
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

string dns_client::decode_domain_name(const vector<byte_t>& data, size_t& offset) {
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

::sockaddr_in dns_client::create_server_address() const {
    ::sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = ::htons(dns_port_);
    if (::inet_pton(AF_INET, dns_server_.c_str(), &addr.sin_addr) <= 0) {
        throw_exception(dns_exception("Invalid DNS server address"));
    }
    return addr;
}

vector<byte_t> dns_client::send_udp_query(const vector<byte_t>& query) const {
    ensure_winsock_initialized();

    const socket udp_sock(SOCKET_DOMAIN::IPV4, SOCKET_TYPE::DATAGRAM, SOCKET_PROTOCOL::UDP);

    if (!udp_sock.is_valid()) {
        throw_exception(dns_exception("Failed to create UDP socket"));
    }
    if (!udp_sock.set_receive_timeout(timeout_)) {
        throw_exception(dns_exception("Failed to set socket timeout"));
    }

    const ssize_t sent = udp_sock.send_to(query.data(), query.size(), create_server_address());
    if (sent < 0 || static_cast<size_t>(sent) != query.size()) {
        throw_exception(dns_exception("Failed to send UDP query"));
    }

    vector<byte_t> buffer(512);
    const ssize_t received = udp_sock.receive_from(buffer.data(), buffer.size());
    if (received < 0) {
        throw_exception(dns_exception("UDP query timeout or receive error"));
    }
    buffer.resize(received);
    return buffer;
}

vector<byte_t> dns_client::send_tcp_query(const vector<byte_t>& query) const {
    ensure_winsock_initialized();

    const socket tcp_sock(SOCKET_DOMAIN::IPV4, SOCKET_TYPE::STREAM, SOCKET_PROTOCOL::TCP);

    if (!tcp_sock.is_valid()) {
        throw_exception(dns_exception("Failed to create TCP socket"));
    }
    if (!tcp_sock.set_receive_timeout(timeout_) || !tcp_sock.set_send_timeout(timeout_)) {
        throw_exception(dns_exception("Failed to set socket timeout"));
    }
    if (!tcp_sock.connect(create_server_address())) {
        throw_exception(dns_exception("Failed to connect to DNS server"));
    }

    const uint16_t length = ::htons(static_cast<uint16_t>(query.size()));
    if (tcp_sock.send(&length, 2) != 2) {
        throw_exception(dns_exception("Failed to send query length"));
    }
    if (tcp_sock.send(query.data(), query.size()) != static_cast<ssize_t>(query.size())) {
        throw_exception(dns_exception("Failed to send query data"));
    }

    uint16_t res_len;
    if (tcp_sock.receive(&res_len, 2) != 2) {
        throw_exception(dns_exception("Failed to receive response length"));
    }
    res_len = ::ntohs(res_len);

    vector<byte_t> buffer(res_len);
    size_t total = 0;
    while (total < res_len) {
        const ssize_t received = tcp_sock.receive(buffer.data() + total, res_len - total);
        if (received <= 0) {
            throw_exception(dns_exception("Failed to receive complete response"));
        }
        total += received;
    }

    return buffer;
}

string dns_client::parse_a_record(const vector<byte_t>& rdata) {
    if (rdata.size() != 4) {
        throw_exception(dns_exception("Invalid A record length"));
    }
    char ip[INET_ADDRSTRLEN];
    if (::inet_ntop(AF_INET, rdata.data(), ip, INET_ADDRSTRLEN) == nullptr) {
        throw_exception(dns_exception("Failed to parse A record"));
    }
    return {ip};
}

string dns_client::parse_aaaa_record(const vector<byte_t>& rdata) {
    if (rdata.size() != 16) {
        throw_exception(dns_exception("Invalid AAAA record length"));
    }
    char ip[INET6_ADDRSTRLEN];
    if (::inet_ntop(AF_INET6, rdata.data(), ip, INET6_ADDRSTRLEN) == nullptr) {
        throw_exception(dns_exception("Failed to parse AAAA record"));
    }
    return {ip};
}

string dns_client::parse_mx_record(const vector<byte_t>& data, size_t offset, const uint16_t rdlength) {
    if (rdlength < 2) {
        throw_exception(dns_exception("Invalid MX record length"));
    }
    const uint16_t preference = ::ntohs(*reinterpret_cast<const uint16_t*>(&data[offset]));
    offset += 2;
    const string exchange = decode_domain_name(data, offset);
    return to_string(preference) + " " + exchange;
}

string dns_client::parse_txt_record(const vector<byte_t>& rdata) {
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

void dns_client::ensure_winsock_initialized() {
#ifdef MSTL_PLATFORM_WINDOWS__
    static bool initialized = []() -> bool {
        ::WSADATA wsa_data;
        int result = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            return false;
        }
        std::atexit([]() {
            ::WSACleanup();
        });
        return true;
    }();

    if (!initialized) {
        throw_exception(dns_exception("Failed to initialize Winsock"));
    }
#endif
}

dns_record dns_client::parse_resource_record(const vector<byte_t>& data, size_t& offset) {
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

    vector<byte_t> rdata(data.begin() + offset, data.begin() + offset + rdlength);
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
                char hex[4];
                _MSTL snprintf(hex, sizeof(hex), "%02x", byte);
                record.data += hex;
            }
            break;
        }
    }

    offset += rdlength;
    return record;
}

dns_query_result dns_client::parse_dns_response(const vector<byte_t>& response) {
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

dns_client::dns_client(string dns_server, const uint16_t dns_port,
    const milliseconds timeout, const bool use_tcp)
: dns_server_(_MSTL move(dns_server)), dns_port_(dns_port), timeout_(timeout), use_tcp_(use_tcp) {}

void dns_client::set_dns_server(const string& server, const uint16_t port) {
    dns_server_ = server;
    dns_port_ = port;
}

dns_query_result dns_client::query(
    const string& domain, const DNS_RECORD type, const DNS_QUERY qclass) {
    const auto start_time = steady_clock::now();

    const auto cache_key = create_cache_key(domain, type, qclass);
    auto cached = check_cache(cache_key);
    if (cached) {
        return *cached;
    }

    const auto query_data = build_dns_query(domain, type, qclass);
    vector<byte_t> response;
    if (use_tcp_) {
        response = send_tcp_query(query_data);
    } else {
        response = send_udp_query(query_data);
    }

    auto result = parse_dns_response(response);
    const auto end_time = steady_clock::now();
    result.query_time = duration_cast<milliseconds>(end_time - start_time);

    update_cache(cache_key, result);

    return result;
}

future<dns_query_result> dns_client::query_async(
    const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    return _MSTL async(launch::async, [this, domain, type, qclass] {
        return query(domain, type, qclass);
    });
}

vector<string> dns_client::resolve_a(const string& domain) {
    const auto result = query(domain, DNS_RECORD::A);
    vector<string> ips;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::A) {
            ips.push_back(record.data);
        }
    }

    return ips;
}

vector<string> dns_client::resolve_aaaa(const string& domain) {
    const auto result = query(domain, DNS_RECORD::AAAA);
    vector<string> ips;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::AAAA) {
            ips.push_back(record.data);
        }
    }

    return ips;
}

vector<string> dns_client::resolve_cname(const string& domain) {
    const auto result = query(domain, DNS_RECORD::CNAME);
    vector<string> cnames;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::CNAME) {
            cnames.push_back(record.data);
        }
    }

    return cnames;
}

vector<string> dns_client::resolve_mx(const string& domain) {
    const auto result = query(domain, DNS_RECORD::MX);
    vector<string> mx_records;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::MX) {
            mx_records.push_back(record.data);
        }
    }

    return mx_records;
}

vector<string> dns_client::resolve_txt(const string& domain) {
    const auto result = query(domain, DNS_RECORD::TXT);
    vector<string> txt_records;

    for (const auto& record : result.answers) {
        if (record.type == DNS_RECORD::TXT) {
            txt_records.push_back(record.data);
        }
    }
    return txt_records;
}

string dns_client::reverse_query(const string& ip) {
    string reverse_domain;

    if (ip.find(':') != string::npos) {
        throw_exception(dns_exception("IPv6 reverse query not fully implemented"));
    } else {
        vector<string> parts;
        size_t start = 0;
        size_t pos;

        while ((pos = ip.find('.', start)) != string::npos) {
            parts.push_back(ip.substr(start, pos - start));
            start = pos + 1;
        }
        if (start < ip.length()) {
            parts.push_back(ip.substr(start));
        }

        if (parts.size() != 4) {
            throw_exception(dns_exception("Invalid IPv4 address"));
        }
        reverse_domain = parts[3] + "." + parts[2] + "." + parts[1] + "." + parts[0] + ".in-addr.arpa";
    }

    const auto result = query(reverse_domain, DNS_RECORD::PTR);
    if (!result.answers.empty() && result.answers[0].type == DNS_RECORD::PTR) {
        return result.answers[0].data;
    }
    return "";
}

vector<dns_query_result> dns_client::batch_query(
    const vector<string>& domains, const DNS_RECORD type) {
    vector<_MSTL future<dns_query_result>> futures;

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

string dns_client::create_cache_key(
    const string& domain, DNS_RECORD type, DNS_QUERY qclass) {
    return domain + "_" + to_string(static_cast<int>(type)) + "_" + to_string(static_cast<int>(qclass));
}

optional<dns_query_result> dns_client::check_cache(const string& key) {
    const auto it = cache_.find(key);
    if (it != cache_.end()) {
        const auto now = steady_clock::now();
        const auto cache_age = duration_cast<seconds>(now - it->second.second);

        if (cache_age < cache_ttl_) {
            return dns_query_result(it->second.first);
        }
        cache_.erase(it);
    }
    return nullopt;
}

void dns_client::update_cache(const string& key, const dns_query_result& result) {
    if (result.is_success()) {
        cache_[key] = {result, steady_clock::now()};
    }
}

MSTL_END_NAMESPACE__
