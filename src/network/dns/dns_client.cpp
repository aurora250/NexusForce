#include <NeForce/core/async/async.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/core/string/string_util.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/dns/dns_client.hpp>
#include <NeForce/network/tcp/tcp_socket.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <arpa/inet.h>
#    include <poll.h>
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    uint16_t generate_dns_client_id() {
        thread_local random_mt tls_random;
        thread_local bool seeded = false;
        if (!seeded) {
            const auto seed = steady_clock::now().since_epoch().count() ^ this_thread::id().native_handle();
            tls_random.set_seed(seed);
            seeded = true;
        }
        return tls_random.next_int(1, 65535);
    }

    template <typename T>
    T read_network_be(const byte_vector& data, size_t offset) {
        T value;
        memory_copy(&value, &data[offset], sizeof(T));
        return endian::network_to_host(value);
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
        size_t pos = 0;

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

    byte_vector build_dns_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                                const bool rd, const bool edns_enable, const bool dnssec_ok,
                                const uint16_t edns_payload) {
        byte_vector query;
        query.reserve(sizeof(dns_header) + domain.length() + 6 + (edns_enable ? 11 : 0));

        dns_header header;
        header.id = endian::host_to_network<uint16_t>(generate_dns_client_id());
        uint16_t flags = 0x0000; // QR=0, OPCODE=0(QUERY)
        if (rd) {
            flags |= 0x0100; // RD=1
        }
        header.flags = endian::host_to_network<uint16_t>(flags);
        header.qdcount = endian::host_to_network<uint16_t>(1);
        header.arcount = endian::host_to_network<uint16_t>(edns_enable ? 1 : 0);

        query.resize(sizeof(dns_header));
        memory_copy(query.data(), &header, sizeof(dns_header));

        auto encoded_domain = encode_domain_name(domain);
        query.insert(query.end(), encoded_domain.begin(), encoded_domain.end());

        const uint16_t qtype = endian::host_to_network(static_cast<uint16_t>(type));
        const uint16_t qclass_val = endian::host_to_network(static_cast<uint16_t>(qclass));

        query.insert(query.end(), reinterpret_cast<const byte_t*>(&qtype),
                     reinterpret_cast<const byte_t*>(&qtype) + sizeof(qtype));
        query.insert(query.end(), reinterpret_cast<const byte_t*>(&qclass_val),
                     reinterpret_cast<const byte_t*>(&qclass_val) + sizeof(qclass_val));

        if (edns_enable) {
            // OPT pseudo-RR: NAME = root (0x00)
            query.push_back(0x00);
            // TYPE = OPT (41)
            constexpr auto opt_type = endian::host_to_network<uint16_t>(edns::OPT_TYPE);
            query.insert(query.end(), reinterpret_cast<const byte_t*>(&opt_type),
                         reinterpret_cast<const byte_t*>(&opt_type) + sizeof(opt_type));
            // CLASS = UDP payload size
            const auto payload = endian::host_to_network<uint16_t>(edns_payload);
            query.insert(query.end(), reinterpret_cast<const byte_t*>(&payload),
                         reinterpret_cast<const byte_t*>(&payload) + sizeof(payload));
            // TTL = extended RCODE(0) | version(0) << 16 | DO bit
            const auto opt_ttl = endian::host_to_network<uint32_t>(dnssec_ok ? edns::DO_BIT : 0);
            query.insert(query.end(), reinterpret_cast<const byte_t*>(&opt_ttl),
                         reinterpret_cast<const byte_t*>(&opt_ttl) + sizeof(opt_ttl));
            // RDLENGTH = 0
            constexpr uint16_t rdlength = 0;
            query.insert(query.end(), reinterpret_cast<const byte_t*>(&rdlength),
                         reinterpret_cast<const byte_t*>(&rdlength) + sizeof(rdlength));
        }

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

        const auto preference = read_network_be<uint16_t>(data, offset);
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

    dns_srv_record parse_srv_record(const byte_vector& data, size_t offset, const uint16_t rdlength) {
        if (rdlength < 6) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Invalid SRV record length"));
        }
        if (offset + 6 > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("SRV record exceeds buffer"));
        }

        dns_srv_record srv;
        srv.priority = read_network_be<uint16_t>(data, offset);
        offset += 2;
        srv.weight = read_network_be<uint16_t>(data, offset);
        offset += 2;
        srv.port = read_network_be<uint16_t>(data, offset);
        offset += 2;
        srv.target = decode_domain_name(data, offset);
        return srv;
    }

    dns_soa_record parse_soa_record(const byte_vector& data, size_t offset, const uint16_t /* rdlength */) {
        dns_soa_record soa;
        soa.mname = decode_domain_name(data, offset);
        soa.rname = decode_domain_name(data, offset);

        if (offset + 20 > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("SOA record exceeds buffer"));
        }

        soa.serial = read_network_be<uint32_t>(data, offset);
        offset += 4;
        soa.refresh = read_network_be<uint32_t>(data, offset);
        offset += 4;
        soa.retry = read_network_be<uint32_t>(data, offset);
        offset += 4;
        soa.expire = read_network_be<uint32_t>(data, offset);
        offset += 4;
        soa.minimum = read_network_be<uint32_t>(data, offset);
        return soa;
    }

    void parse_opt_record(const byte_vector& data, size_t& offset, dns_query_result& result) {
        // NAME must be root (0x00) for OPT record
        if (data[offset] != 0x00) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("OPT record name must be root"));
        }
        offset++;

        if (offset + 10 > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Incomplete OPT record"));
        }

        const auto opt_type = read_network_be<uint16_t>(data, offset);
        if (opt_type != edns::OPT_TYPE) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Expected OPT record type 41"));
        }
        offset += 2;

        result.udp_payload_size = read_network_be<uint16_t>(data, offset);
        offset += 2;

        const auto opt_ttl = read_network_be<uint32_t>(data, offset);
        offset += 4;

        result.extended_rcode = (opt_ttl >> edns::EXT_RCODE_SHIFT) & 0xFF;
        result.edns_version = (opt_ttl >> edns::VERSION_SHIFT) & 0xFF;
        result.dnssec_ok = (opt_ttl & edns::DO_BIT) != 0;

        const auto rdlength = read_network_be<uint16_t>(data, offset);
        offset += 2;

        if (offset + rdlength > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("OPT RDATA exceeds buffer"));
        }

        offset += rdlength;
    }

    dns_record parse_resource_record(const byte_vector& data, size_t& offset) {
        dns_record record;
        record.name = decode_domain_name(data, offset);

        if (offset + 10 > data.size()) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Incomplete resource record"));
        }

        record.type = static_cast<dns_record::raw>(read_network_be<uint16_t>(data, offset));
        offset += 2;

        record.class_type = static_cast<dns_class>(read_network_be<uint16_t>(data, offset));
        offset += 2;

        record.ttl = read_network_be<uint32_t>(data, offset);
        offset += 4;

        const auto rdlength = read_network_be<uint16_t>(data, offset);
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
                case dns_record::SRV: {
                    auto srv = parse_srv_record(data, rdata_offset, rdlength);
                    record.data = _NEFORCE to_string(srv.priority) + " " + to_string(srv.weight) + " " +
                                  to_string(srv.port) + " " + srv.target;
                    break;
                }
                case dns_record::SOA: {
                    auto soa = parse_soa_record(data, rdata_offset, rdlength);
                    record.data = soa.mname + " " + soa.rname + " " + to_string(soa.serial) + " " +
                                  to_string(soa.refresh) + " " + to_string(soa.retry) + " " + to_string(soa.expire) +
                                  " " + to_string(soa.minimum);
                    break;
                }
                default: {
                    record.data = "";
                    for (const byte_t byte: rdata) {
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

    dns_query_result parse_dns_response(const byte_vector& response, const uint16_t expected_id) {
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

        if ((header.flags & 0x8000) == 0) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("QR bit not set in DNS response"));
        }

        if (expected_id != 0 && header.id != expected_id) {
            NEFORCE_THROW_EXCEPTION(dns_exception::parse_error(
                    format("Response ID mismatch: expected {}, got {}", expected_id, header.id)));
        }

        result.response_code = static_cast<dns_response>(header.flags & 0x000F);
        result.authoritative = (header.flags & 0x0400) != 0;
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

        for (uint16_t i = 0; i < header.arcount; ++i) {
            const size_t name_start = offset;
            decode_domain_name(response, offset);

            if (offset + 2 > response.size()) {
                NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("Additional section exceeds buffer"));
            }

            const uint16_t rtype = endian::network_to_host(*reinterpret_cast<const uint16_t*>(&response[offset]));

            if (rtype == edns::OPT_TYPE) {
                offset = name_start;
                parse_opt_record(response, offset, result);
            } else {
                offset = name_start;
                result.additional.push_back(parse_resource_record(response, offset));
            }
        }

        return result;
    }

    string create_cache_key(const string_view domain, const dns_record::raw type, const dns_class qclass) {
        return domain + "_"_s + to_string(static_cast<int>(type)) + "_" + to_string(static_cast<int>(qclass));
    }
} // namespace


void dns_client::start_io() {
    if (io_running_) {
        return;
    }

    shared_socket_.open();
    if (!shared_socket_.is_open()) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to create shared UDP socket"));
    }

    try {
        wake_pipe_ = pipe(false);
    } catch (...) {
        shared_socket_.close();
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to create wake pipe"));
    }

    if (!io_pool_.running()) {
        io_pool_.start(1);
    }

    io_running_ = true;
    io_pool_.submit_task([this] { io_receive_loop(); });
}

void dns_client::stop_io() {
    bool expected = true;
    if (!io_running_.compare_exchange_strong(expected, false)) {
        return;
    }

    if (wake_pipe_.is_valid()) {
        constexpr char byte = 1;
        (void) wake_pipe_.write(&byte, 1);
    }

    io_pool_.stop();

    {
        lock<mutex> lock(pending_mutex_);
        for (auto& [id, entry]: pending_queries_) {
            try {
                NEFORCE_THROW_EXCEPTION(dns_exception::network_error("DNS client shutting down"));
            } catch (...) {
                entry.promise.set_exception(current_exception());
            }
        }
        pending_queries_.clear();
    }

    shared_socket_.close();
    wake_pipe_.close();
}

void dns_client::io_receive_loop() {
    const auto sock_fd = shared_socket_.native_handle();
    const auto wake_fd = wake_pipe_.native_read_handle();

    pollfd pfds[2];
    pfds[0].fd = sock_fd;
    pfds[0].events = POLLIN;
    pfds[1].fd = wake_fd;
    pfds[1].events = POLLIN;

    while (io_running_) {
        pfds[0].revents = 0;
        pfds[1].revents = 0;

        const int ret = ::poll(pfds, 2, 1000);

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        if ((pfds[1].revents & POLLIN) != 0) {
            break;
        }

        if ((pfds[0].revents & POLLIN) != 0) {
            byte_vector buffer(4096);
            const auto received = shared_socket_.receive_from(
                    memory_view<char>{reinterpret_cast<char*>(buffer.data()), buffer.size()});

            if (received.first > 0) {
                buffer.resize(static_cast<size_t>(received.first));
                if (buffer.size() >= sizeof(uint16_t)) {
                    const uint16_t response_id =
                            endian::network_to_host(*reinterpret_cast<const uint16_t*>(buffer.data()));

                    pending_entry entry;
                    bool resolved = false;
                    {
                        lock<mutex> lock(pending_mutex_);
                        auto it = pending_queries_.find(response_id);
                        if (it != pending_queries_.end()) {
                            entry = move(it->second);
                            pending_queries_.erase(it);
                            resolved = true;
                        }
                    }

                    if (resolved) {
                        try {
                            auto result = parse_dns_response(buffer, response_id);
                            entry.promise.set_value(move(result));
                        } catch (...) {
                            entry.promise.set_exception(current_exception());
                        }
                    }
                }
            }
        }

        // Time out stale queries
        {
            lock<mutex> lock(pending_mutex_);
            auto now = steady_clock::now();
            for (auto it = pending_queries_.begin(); it != pending_queries_.end();) {
                if (now - it->second.created_at > config_.timeout) {
                    try {
                        NEFORCE_THROW_EXCEPTION(dns_exception::timeout());
                    } catch (...) {
                        it->second.promise.set_exception(current_exception());
                    }
                    it = pending_queries_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

void dns_client::send_query(const byte_vector& query) {
    const auto endpoint = ip_address::parse(config_.server, config_.port);
    if (!endpoint) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Invalid DNS server address"));
    }

    lock<mutex> lock(send_mutex_);
    const ssize_t sent = shared_socket_.send_to(
            memory_view<const char>{reinterpret_cast<const char*>(query.data()), query.size()}, *endpoint);

    if (sent < 0 || static_cast<size_t>(sent) != query.size()) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to send UDP query"));
    }
}

byte_vector dns_client::send_tcp_query(const byte_vector& query) {
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

        static_cast<ip_socket&>(tls_tcp_state.socket).connect(*endpoint);
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
        if (tls_tcp_state.socket.send(memory_view<const char>{reinterpret_cast<const char*>(&length), 2}) != 2) {
            return false;
        }

        if (tls_tcp_state.socket.send(memory_view<const char>{reinterpret_cast<const char*>(query.data()),
                                                              query.size()}) != query.size()) {
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

    uint16_t res_len = 0;
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
        const ssize_t received = tls_tcp_state.socket.receive(
                memory_view<char>{reinterpret_cast<char*>(buffer.data() + total), res_len - total});

        if (received <= 0) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to receive complete response"));
        }

        total += received;
    }

    return buffer;
}

dns_client::dns_client(config cfg, const bool use_tcp) :
config_(move(cfg)),
use_tcp_(use_tcp) {
    if (config_.server.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("DNS server address cannot be empty"));
    }
    if (config_.timeout <= milliseconds(0)) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Timeout must be positive"));
    }
}

dns_client::~dns_client() {
    try {
        stop_io();
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

void dns_client::ensure_io_started() {
    if (!io_running_) {
        start_io();
    }
}

dns_query_result dns_client::query(const string_view domain, const dns_record::raw type, const dns_class qclass) {
    ensure_io_started();
    if (domain.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Domain name cannot be empty"));
    }

    const auto cache_key = create_cache_key(domain, type, qclass);
    auto cached = check_cache(cache_key);
    if (cached) {
        return *cached;
    }

    const auto start_time = steady_clock::now();

    const auto query_data =
            build_dns_query(domain, type, qclass, recursion_desired_, true, dnssec_ok_, edns_udp_payload_);
    const uint16_t query_id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(query_data.data()));

    if (use_tcp_) {
        auto response = send_tcp_query(query_data);
        auto result = parse_dns_response(response, query_id);
        const auto end_time = steady_clock::now();
        result.query_time = time_cast<milliseconds>(end_time - start_time);
        update_cache(cache_key, result);
        return result;
    }

    promise<dns_query_result> prom;
    auto fut = prom.get_future();

    {
        lock<mutex> lock(pending_mutex_);
        pending_queries_.emplace(query_id, pending_entry{move(prom), query_data, steady_clock::now()});
    }

    send_query(query_data);

    try {
        auto result = fut.get();
        const auto end_time = steady_clock::now();
        result.query_time = time_cast<milliseconds>(end_time - start_time);

        if (!result.truncated) {
            update_cache(cache_key, result);
            return result;
        }

        auto tcp_response = send_tcp_query(query_data);
        auto tcp_result = parse_dns_response(tcp_response, query_id);
        const auto tcp_end_time = steady_clock::now();
        tcp_result.query_time = time_cast<milliseconds>(tcp_end_time - start_time);
        update_cache(cache_key, tcp_result);
        return tcp_result;

    } catch (...) {
        {
            lock<mutex> lock(pending_mutex_);
            pending_queries_.erase(query_id);
        }
        throw;
    }
}

future<dns_query_result> dns_client::query_async(const string& domain, dns_record::raw type, dns_class qclass) {
    ensure_io_started();

    if (domain.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Domain name cannot be empty"));
    }

    const auto cache_key = create_cache_key(domain.view(), type, qclass);
    auto cached = check_cache(cache_key);
    if (cached) {
        promise<dns_query_result> prom;
        prom.set_value(*cached);
        return prom.get_future();
    }

    const auto query_data =
            build_dns_query(domain.view(), type, qclass, recursion_desired_, true, dnssec_ok_, edns_udp_payload_);
    const uint16_t query_id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(query_data.data()));

    promise<dns_query_result> prom;
    auto fut = prom.get_future();

    {
        lock<mutex> lock(pending_mutex_);
        pending_queries_.emplace(query_id, pending_entry{move(prom), query_data, steady_clock::now()});
    }

    send_query(query_data);

    return fut;
}

vector<string> dns_client::resolve_a(const string_view domain) {
    string current_domain = domain;
    int cname_hops = 0;

    while (cname_hops < 5) {
        const auto result = query(current_domain.view(), dns_record::A);
        vector<string> ips;

        for (const auto& record: result.answers) {
            if (record.type == dns_record::A) {
                ips.push_back(record.data);
            }
        }

        if (!ips.empty()) {
            return ips;
        }

        bool found_cname = false;
        for (const auto& record: result.answers) {
            if (record.type == dns_record::CNAME) {
                current_domain = record.data;
                found_cname = true;
                break;
            }
        }

        if (!found_cname) {
            return ips;
        }
        ++cname_hops;
    }

    return {};
}

vector<string> dns_client::resolve_aaaa(const string_view domain) {
    string current_domain = domain;
    int cname_hops = 0;

    while (cname_hops < 5) {
        const auto result = query(current_domain.view(), dns_record::AAAA);
        vector<string> ips;

        for (const auto& record: result.answers) {
            if (record.type == dns_record::AAAA) {
                ips.push_back(record.data);
            }
        }

        if (!ips.empty()) {
            return ips;
        }

        bool found_cname = false;
        for (const auto& record: result.answers) {
            if (record.type == dns_record::CNAME) {
                current_domain = record.data;
                found_cname = true;
                break;
            }
        }

        if (!found_cname) {
            return ips;
        }
        ++cname_hops;
    }

    return {};
}

vector<string> dns_client::resolve_cname(const string_view domain) {
    const auto result = query(domain, dns_record::CNAME);
    vector<string> cnames;
    cnames.reserve(result.answers.size());

    for (const auto& record: result.answers) {
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

    for (const auto& record: result.answers) {
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

    for (const auto& record: result.answers) {
        if (record.type == dns_record::TXT) {
            txt_records.push_back(record.data);
        }
    }
    return txt_records;
}

vector<dns_srv_record> dns_client::resolve_srv(const string_view domain) {
    const auto result = query(domain, dns_record::SRV);
    vector<dns_srv_record> srv_records;
    srv_records.reserve(result.answers.size());

    for (const auto& record: result.answers) {
        if (record.type == dns_record::SRV) {
            dns_srv_record srv;
            const auto parts = split(record.data.view(), " ");
            if (parts.size() >= 4) {
                srv.priority = to_uint16(parts[0]);
                srv.weight = to_uint16(parts[1]);
                srv.port = to_uint16(parts[2]);
                srv.target = parts[3];
                srv_records.push_back(srv);
            }
        }
    }
    return srv_records;
}

optional<dns_soa_record> dns_client::resolve_soa(const string_view domain) {
    const auto result = query(domain, dns_record::SOA);

    for (const auto& record: result.answers) {
        if (record.type == dns_record::SOA) {
            dns_soa_record soa;
            const auto parts = split(record.data.view(), " ");
            if (parts.size() >= 7) {
                soa.mname = parts[0];
                soa.rname = parts[1];
                soa.serial = to_uint32(parts[2]);
                soa.refresh = to_uint32(parts[3]);
                soa.retry = to_uint32(parts[4]);
                soa.expire = to_uint32(parts[5]);
                soa.minimum = to_uint32(parts[6]);
                return optional<dns_soa_record>{soa};
            }
        }
    }
    return none;
}

string dns_client::reverse_query(const string_view ip) {
    if (ip.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("IP address cannot be empty"));
    }

    if (ip.contains(':')) {
        NEFORCE_THROW_EXCEPTION(dns_exception("IPv6 reverse query not fully implemented"));
    }

    vector<string_view> parts;
    parts.reserve(4);

    size_t start = 0;
    size_t pos = 0;

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

    const string reverse_domain = parts[3] + "."_s + parts[2] + "." + parts[1] + "." + parts[0] + ".in-addr.arpa";

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

    for (const auto& domain: domains) {
        futures.push_back(query_async(domain, type));
    }

    vector<dns_query_result> results;
    results.reserve(futures.size());

    for (auto& future: futures) {
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
    {
        shared_lock<shared_mutex> read_lock(cache_mutex_);
        const auto it = cache_.find(key);
        if (it != cache_.end()) {
            const auto now = steady_clock::now();
            const auto age = time_cast<seconds>(now - it->second.second);
            const auto& res = it->second.first;
            const auto max_ttl = res.is_success() ? cache_ttl_ : edns::NEGATIVE_CACHE_TTL;
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
        const auto max_ttl = res.is_success() ? cache_ttl_ : edns::NEGATIVE_CACHE_TTL;
        if (age >= max_ttl) {
            cache_.erase(it);
        } else {
            return optional<dns_query_result>{res};
        }
    }

    return none;
}

void dns_client::update_cache(const string& key, const dns_query_result& result) {
    lock<shared_mutex> write_lock(cache_mutex_);
    cache_[key] = {result, steady_clock::now()};
}

byte_vector dns_client::build_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                                    const bool rd, const bool edns_enable, const bool dnssec_ok,
                                    const uint16_t edns_payload) {
    return build_dns_query(domain, type, qclass, rd, edns_enable, dnssec_ok, edns_payload);
}

dns_query_result dns_client::parse_response(const byte_vector& response, const uint16_t expected_id) {
    return parse_dns_response(response, expected_id);
}

NEFORCE_END_NAMESPACE__
