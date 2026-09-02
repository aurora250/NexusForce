#include <NeForce/core/async/async.hpp>
#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/numeric/random.hpp>
#include <NeForce/core/string/format.hpp>
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

    // Generate a 0x20 randomized-case pattern of the query name.
    // Each ASCII letter is independently kept or flipped, so a forged
    // response cannot guess the expected case without seeing the query.
    string randomize_case_pattern(const string_view domain, const bool enable) {
        if (!enable) {
            return string(domain);
        }
        string pattern(domain);
        for (size_t i = 0; i < pattern.size(); ++i) {
            char& c = pattern[i];
            if (c >= 'a' && c <= 'z') {
                if (secret::next_int<uint32_t>(2) != 0) {
                    c = static_cast<char>(c - 'a' + 'A');
                }
            } else if (c >= 'A' && c <= 'Z') {
                if (secret::next_int<uint32_t>(2) != 0) {
                    c = static_cast<char>(c - 'A' + 'a');
                }
            }
        }
        return pattern;
    }

    // Source port rotation thresholds: rotate the shared UDP socket after
    // enough queries or enough wall time, shrinking the DNS poisoning window.
    constexpr uint32_t g_socket_rotate_queries = 128;
    constexpr seconds g_socket_rotate_interval{60};

    template <typename T>
    T read_network_be(const byte_vector& data, const size_t offset) {
        T value;
        _NEFORCE memory_copy(&value, &data[offset], sizeof(T));
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

        dns_header header{};
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
        return to_string(preference) + " " + exchange;
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
                    const auto srv = parse_srv_record(data, rdata_offset, rdlength);
                    record.data = to_string(srv.priority) + " " + to_string(srv.weight) + " " + to_string(srv.port) +
                                  " " + srv.target;
                    break;
                }
                case dns_record::SOA: {
                    const auto soa = parse_soa_record(data, rdata_offset, rdlength);
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

    dns_query_result parse_dns_response(const byte_vector& response, const uint16_t expected_id,
                                        const string_view expected_question) {
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
            if (i == 0 && !expected_question.empty()) {
                // 0x20 validation: the response must echo
                // the exact randomized-case QNAME we sent (case-sensitive compare).
                const string decoded = decode_domain_name(response, offset);
                if (decoded.view() != expected_question) {
                    NEFORCE_THROW_EXCEPTION(dns_exception::parse_error("0x20 question case mismatch in DNS response"));
                }
            } else {
                decode_domain_name(response, offset);
            }
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

void dns_client::dns_query_op::start() {
    if (cancel_slot != nullptr && cancel_slot->is_cancelled()) {
        handler(make_operation_aborted(), dns_query_result{});
        return;
    }

    start_time_ = steady_clock::now();
    timeout_ = client->config_.timeout;
    max_udp_retries_ = client->max_udp_retries_;
    const auto budget = static_cast<uint64_t>(timeout_.count()) * (static_cast<uint64_t>(max_udp_retries_) + 1);
    deadline_ = start_time_ + milliseconds(static_cast<int64_t>(budget));

    expected_question_ = randomize_case_pattern(domain.view(), client->case_randomize_);
    query_data_ = build_query(domain.view(), type, qclass, client->recursion_desired_, true, client->dnssec_ok_,
                              client->edns_udp_payload_, expected_question_.view());
    query_id_ = endian::network_to_host<uint16_t>(*reinterpret_cast<const uint16_t*>(query_data_.data()));

    {
        client->pending_mutex_.lock();
        auto& entry_ref = client->pending_queries_[query_id_];
        entry_ref.op = weak_from_this();
        entry_ref.query_data = query_data_;
        entry_ref.created_at = steady_clock::now();
        client->pending_mutex_.unlock();
    }

    try {
        client->maybe_rotate_socket();
        client->send_query(query_data_);
    } catch (...) {
        client->unregister_query(query_id_);
        handler(make_error_code(errc::io_error), dns_query_result{});
        return;
    }
    client->ensure_io_started();

    auto self = shared_from_this();
    const auto timeout_ms = static_cast<uint64_t>(timeout_.count());
    timer_id_ = client->ctx_->schedule_timer(timeout_ms, [self]() { self->on_timeout(); });

    if (cancel_slot != nullptr) {
        auto op_weak = weak_from_this();
        cancel_slot->assign([op_weak]() mutable {
            if (const auto op = op_weak.lock()) {
                op->do_cancel();
            }
        });
    }
}

void dns_client::dns_query_op::retry_udp() {
    expected_question_ = randomize_case_pattern(domain.view(), client->case_randomize_);
    query_data_ = build_query(domain.view(), type, qclass, client->recursion_desired_, true, client->dnssec_ok_,
                              client->edns_udp_payload_, expected_question_.view());
    query_id_ = endian::network_to_host<uint16_t>(*reinterpret_cast<const uint16_t*>(query_data_.data()));

    {
        client->pending_mutex_.lock();
        auto& entry_ref = client->pending_queries_[query_id_];
        entry_ref.op = weak_from_this();
        entry_ref.query_data = query_data_;
        entry_ref.created_at = steady_clock::now();
        client->pending_mutex_.unlock();
    }

    try {
        client->send_query(query_data_);
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // Send failure: keep waiting; the pending timeout path settles the op.
    }
}

void dns_client::dns_query_op::on_response(const error_code /*ec*/, dns_query_result result) {
    bool expected = false;
    if (!fired_.compare_exchange_strong(expected, true)) {
        return;
    }

    client->ctx_->cancel_timer(timer_id_);
    client->unregister_query(query_id_);

    if (result.truncated) {
        start_tcp_fallback();
        return;
    }

    const auto end_time = steady_clock::now();
    result.query_time = time_cast<milliseconds>(end_time - start_time_);

    const auto cache_key = create_cache_key(domain.view(), type, qclass);
    client->update_cache(cache_key, result);

    handler(error_code{}, move(result));
}

void dns_client::dns_query_op::on_timeout() {
    if (fired_.load()) {
        return;
    }

    const auto now = steady_clock::now();
    if (udp_retries_ < max_udp_retries_ && now < deadline_) {
        ++udp_retries_;
        client->unregister_query(query_id_);
        retry_udp();

        auto self = shared_from_this();
        const auto remaining_ms = time_cast<milliseconds>(deadline_ - now).count();
        const auto next_timeout = max<int64_t>(1, min<int64_t>(timeout_.count(), remaining_ms));
        timer_id_ = client->ctx_->schedule_timer(static_cast<uint64_t>(next_timeout), [self]() { self->on_timeout(); });
        return;
    }

    bool expected = false;
    if (!fired_.compare_exchange_strong(expected, true)) {
        return;
    }
    client->unregister_query(query_id_);
    handler(make_error_code(errc::timed_out), dns_query_result{});
}

void dns_client::dns_query_op::do_cancel() {
    bool expected = false;
    if (!fired_.compare_exchange_strong(expected, true)) {
        return;
    }
    client->ctx_->cancel_timer(timer_id_);
    client->unregister_query(query_id_);
    handler(make_operation_aborted(), dns_query_result{});
}

void dns_client::dns_query_op::start_tcp_fallback() {
    // Entry points:
    // (1) TC-flagged UDP reply from on_response(), where fired_ is already set;
    // (2) forced-TCP mode from async_query(), where no query has been built yet and fired_ is still clear.
    if (query_data_.empty()) {
        start_time_ = steady_clock::now();
        timeout_ = client->config_.timeout;
        max_udp_retries_ = client->max_udp_retries_;
        deadline_ = start_time_ + milliseconds(timeout_.count());

        expected_question_ = randomize_case_pattern(domain.view(), client->case_randomize_);
        query_data_ = build_query(domain.view(), type, qclass, client->recursion_desired_, true, client->dnssec_ok_,
                                  client->edns_udp_payload_, expected_question_.view());
        query_id_ = endian::network_to_host<uint16_t>(*reinterpret_cast<const uint16_t*>(query_data_.data()));

        if (tcp_only_) {
            client->pending_mutex_.lock();
            auto& entry_ref = client->pending_queries_[query_id_];
            entry_ref.op = weak_from_this();
            entry_ref.query_data = query_data_;
            entry_ref.created_at = steady_clock::now();
            client->pending_mutex_.unlock();
        }
    }

    const auto now = steady_clock::now();
    if (now >= deadline_) {
        finish_tcp(make_error_code(errc::timed_out), dns_query_result{});
        return;
    }

    const auto endpoint = ip_address::parse(client->config_.server, client->config_.port);
    if (!endpoint) {
        finish_tcp(make_error_code(errc::invalid_argument), dns_query_result{});
        return;
    }

    tcp_socket_ = make_unique<tcp_socket>();
    tcp_socket_->open();
    if (!tcp_socket_->is_open()) {
        finish_tcp(make_error_code(errc::io_error), dns_query_result{});
        return;
    }

    const auto remaining_ms = time_cast<milliseconds>(deadline_ - now).count();
    auto self = shared_from_this();
    tcp_timer_id_ = client->ctx_->schedule_timer(static_cast<uint64_t>(max<int64_t>(1, remaining_ms)),
                                                 [self]() { self->on_tcp_timeout(); });
    tcp_socket_->async_connect(*client->ctx_, *endpoint, [self](error_code ec) { self->on_tcp_connected(ec); });
}

void dns_client::dns_query_op::on_tcp_connected(const error_code ec) {
    if (ec) {
        finish_tcp(ec, dns_query_result{});
        return;
    }

    tcp_request_buf_.resize(query_data_.size() + 2);
    const auto len = endian::host_to_network<uint16_t>(static_cast<uint16_t>(query_data_.size()));
    memory_copy(tcp_request_buf_.data(), &len, 2);
    memory_copy(tcp_request_buf_.data() + 2, query_data_.data(), query_data_.size());

    auto self = shared_from_this();
    tcp_socket_->async_write(
            *client->ctx_,
            memory_view<const char>{reinterpret_cast<const char*>(tcp_request_buf_.data()), tcp_request_buf_.size()},
            [self](error_code ec, size_t bytes) { self->on_tcp_written(ec, bytes); });
}

void dns_client::dns_query_op::on_tcp_written(const error_code ec, const size_t /*bytes*/) {
    if (ec) {
        finish_tcp(ec, dns_query_result{});
        return;
    }

    auto self = shared_from_this();
    tcp_socket_->async_read(*client->ctx_,
                            memory_view<char>{reinterpret_cast<char*>(tcp_len_buf_), sizeof(tcp_len_buf_)},
                            [self](error_code ec, size_t bytes) { self->on_tcp_len_read(ec, bytes); });
}

void dns_client::dns_query_op::on_tcp_len_read(const error_code ec, const size_t bytes) {
    if (ec) {
        finish_tcp(ec, dns_query_result{});
        return;
    }
    if (bytes != sizeof(tcp_len_buf_)) {
        finish_tcp(make_error_code(errc::protocol_error), dns_query_result{});
        return;
    }

    const uint16_t len = endian::network_to_host(*reinterpret_cast<const uint16_t*>(tcp_len_buf_));
    if (len == 0) {
        finish_tcp(make_error_code(errc::protocol_error), dns_query_result{});
        return;
    }
    tcp_response_len_ = len;
    tcp_response_buf_.resize(len);
    tcp_response_received_ = 0;

    auto self = shared_from_this();
    tcp_socket_->async_read(
            *client->ctx_,
            memory_view<char>{reinterpret_cast<char*>(tcp_response_buf_.data()), tcp_response_buf_.size()},
            [self](error_code ec, size_t n) { self->on_tcp_body_read(ec, n); });
}

void dns_client::dns_query_op::on_tcp_body_read(const error_code ec, const size_t bytes) {
    if (ec) {
        finish_tcp(ec, dns_query_result{});
        return;
    }

    tcp_response_received_ += bytes;
    if (tcp_response_received_ < tcp_response_buf_.size()) {
        auto self = shared_from_this();
        tcp_socket_->async_read(
                *client->ctx_,
                memory_view<char>{reinterpret_cast<char*>(tcp_response_buf_.data() + tcp_response_received_),
                                  tcp_response_buf_.size() - tcp_response_received_},
                [self](error_code ec, size_t n) { self->on_tcp_body_read(ec, n); });
        return;
    }

    try {
        auto result = parse_response(tcp_response_buf_, query_id_, expected_question_.view());
        finish_tcp(error_code{}, move(result));
    } catch (...) {
        finish_tcp(make_error_code(errc::protocol_error), dns_query_result{});
    }
}

void dns_client::dns_query_op::on_tcp_timeout() { finish_tcp(make_error_code(errc::timed_out), dns_query_result{}); }

void dns_client::dns_query_op::finish_tcp(const error_code ec, dns_query_result result) {
    if (tcp_finished_) {
        return;
    }
    tcp_finished_ = true;

    client->ctx_->cancel_timer(tcp_timer_id_);
    client->unregister_query(query_id_);
    if (tcp_socket_ && tcp_socket_->is_open()) {
        tcp_socket_->close();
    }

    if (!ec) {
        const auto end_time = steady_clock::now();
        result.query_time = time_cast<milliseconds>(end_time - start_time_);
        const auto cache_key = create_cache_key(domain.view(), type, qclass);
        client->update_cache(cache_key, result);
    }
    handler(ec, move(result));
}

void dns_client::start_io() {
    if (io_running_) {
        return;
    }

    shared_socket_.open();
    if (!shared_socket_.is_open()) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to create shared UDP socket"));
    }

    shared_socket_.bind(ip_address::any());
    shared_socket_.set_nonblocking(true);

    io_running_ = true;
    if (ctx_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(dns_exception("dns_client: io_context is required for async I/O"));
    }
    register_shared_receive();
    socket_created_at_ = steady_clock::now();
    query_count_ = 0;
}

void dns_client::stop_io() {
    bool expected = true;
    if (!io_running_.compare_exchange_strong(expected, false)) {
        return;
    }

    if (shared_socket_.is_open()) {
        ctx_->remove_fd(static_cast<int>(shared_socket_.native_handle()));
    }

    {
        vector<shared_ptr<dns_query_op>> tcp_ops;
        pending_mutex_.lock();
        for (auto& pending_query: pending_queries_) {
            if (auto const op = pending_query.second.op.lock()) {
                if (op->tcp_only_) {
                    tcp_ops.push_back(op);
                    continue;
                }
                bool fired_expected = false;
                if (op->fired_.compare_exchange_strong(fired_expected, true)) {
                    op->handler(make_operation_aborted(), dns_query_result{});
                }
            }
        }
        pending_queries_.clear();
        pending_mutex_.unlock();

        for (auto const& op: tcp_ops) {
            op->on_tcp_timeout();
        }
    }

    shared_socket_.close();
}

void dns_client::register_shared_receive() {
    auto* self = this;
    ctx_->add_fd(static_cast<int>(shared_socket_.native_handle()), epoll_in,
                 [self](int /*fd*/, uint32_t /*events*/, const error_code ec) { self->on_udp_readable(ec); });
}

void dns_client::on_udp_readable(const error_code ec) {
    if (ec || !io_running_) {
        return;
    }
    process_udp_receive();
}

void dns_client::process_udp_receive() {
    while (true) {
        byte_vector buffer(65535);
        ssize_t recv_size = 0;
        try {
            const auto received = shared_socket_.receive_from(
                    memory_view<char>{reinterpret_cast<char*>(buffer.data()), buffer.size()});
            recv_size = received.first;
        } catch (...) {
            break;
        }
        if (recv_size <= 0) {
            break;
        }
        buffer.resize(static_cast<size_t>(recv_size));
        if (buffer.size() < sizeof(uint16_t)) {
            continue;
        }
        const uint16_t response_id = endian::network_to_host(*reinterpret_cast<const uint16_t*>(buffer.data()));
        dispatch_to_op(response_id, move(buffer));
    }
}

void dns_client::dispatch_to_op(const uint16_t txid, byte_vector response) {
    pending_mutex_.lock();
    const auto it = pending_queries_.find(txid);
    if (it == pending_queries_.end()) {
        pending_mutex_.unlock();
        return;
    }

    const auto shared_op = it->second.op.lock();
    if (shared_op != nullptr && shared_op->tcp_only_) {
        pending_mutex_.unlock();
        return;
    }

    const weak_ptr<dns_query_op> weak_op = move(it->second.op);
    pending_queries_.erase(it);
    pending_mutex_.unlock();

    const auto op = weak_op.lock();
    if (!op) {
        return;
    }

    try {
        auto result = parse_dns_response(response, op->query_id_, op->expected_question_.view());
        op->on_response(error_code{}, move(result));
    } catch (...) {
        bool expected = false;
        if (op->fired_.compare_exchange_strong(expected, true)) {
            ctx_->cancel_timer(op->timer_id_);
            op->handler(make_error_code(errc::protocol_error), dns_query_result{});
        }
    }
}

void dns_client::unregister_query(const uint16_t txid) {
    lock<mutex> lock(pending_mutex_);
    pending_queries_.erase(txid);
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

void dns_client::maybe_rotate_socket() {
    ++query_count_;
    const auto elapsed = time_cast<seconds>(steady_clock::now() - socket_created_at_);
    if (query_count_ < g_socket_rotate_queries && elapsed < g_socket_rotate_interval) {
        return;
    }
    {
        lock<mutex> lock(pending_mutex_);
        if (!pending_queries_.empty()) {
            return;
        }
    }
    rotate_socket();
}

void dns_client::rotate_socket() {
    lock<mutex> lock(send_mutex_);
    if (shared_socket_.is_open()) {
        ctx_->remove_fd(static_cast<int>(shared_socket_.native_handle()));
        shared_socket_.close();
    }

    shared_socket_.open();
    if (!shared_socket_.is_open()) {
        NEFORCE_THROW_EXCEPTION(dns_exception::network_error("Failed to recreate shared UDP socket"));
    }
    shared_socket_.bind(ip_address::any());
    shared_socket_.set_nonblocking(true);
    register_shared_receive();

    socket_created_at_ = steady_clock::now();
    query_count_ = 0;
}

dns_client::dns_client(config cfg, io_context& ctx, const bool use_tcp) :
config_(move(cfg)),
use_tcp_(use_tcp),
ctx_(&ctx) {
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

void dns_client::async_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                             function<void(error_code, dns_query_result)> handler) {
    if (domain.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Domain name cannot be empty"));
    }
    ensure_io_started();

    const auto cache_key = create_cache_key(domain, type, qclass);
    auto cached = check_cache(cache_key);
    if (cached) {
        auto h = move(handler);
        auto res = move(*cached);
        ctx_->post([h = move(h), res = move(res)]() mutable { h(error_code{}, move(res)); });
        return;
    }

    const auto op = make_shared<dns_query_op>();
    op->client = this;
    op->domain = domain;
    op->type = type;
    op->qclass = qclass;
    op->handler = move(handler);
    op->tcp_only_ = use_tcp_;

    if (use_tcp_) {
        op->start_tcp_fallback();
    } else {
        op->start();
    }
}

void dns_client::async_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                             cancellation_slot& slot, function<void(error_code, dns_query_result)> handler) {
    if (domain.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Domain name cannot be empty"));
    }
    ensure_io_started();

    const auto cache_key = create_cache_key(domain, type, qclass);
    auto cached = check_cache(cache_key);
    if (cached) {
        if (slot.is_cancelled()) {
            handler(make_operation_aborted(), dns_query_result{});
            return;
        }
        auto h = move(handler);
        auto res = move(*cached);
        ctx_->post([h = move(h), res = move(res)]() mutable { h(error_code{}, move(res)); });
        return;
    }

    const auto op = make_shared<dns_query_op>();
    op->client = this;
    op->domain = domain;
    op->type = type;
    op->qclass = qclass;
    op->handler = move(handler);
    op->cancel_slot = &slot;
    op->tcp_only_ = use_tcp_;

    if (use_tcp_) {
        op->start_tcp_fallback();
    } else {
        op->start();
    }
}

auto dns_client::async_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                             use_future_t /*unused*/) {
    async_result<use_future_t, void(error_code, dns_query_result)> result(use_future);
    async_query(domain, type, qclass, result.get_handler());
    auto fut = result.get();
    return fut;
}

void dns_client::async_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                             detached_t /*unused*/) {
    async_query(domain, type, qclass,
                function<void(error_code, dns_query_result)>([](error_code, dns_query_result) {}));
}

#ifdef NEFORCE_STANDARD_20
awaitable<error_code, dns_query_result> dns_client::async_query(const string_view domain, const dns_record::raw type,
                                                                const dns_class qclass, use_awaitable_t /*unused*/) {
    async_result<use_awaitable_t, void(error_code, dns_query_result)> result(use_awaitable);
    async_query(domain, type, qclass, result.get_handler());
    return result.get();
}
#endif

dns_query_result dns_client::query(const string_view domain, const dns_record::raw type, const dns_class qclass) {
    if (domain.empty()) {
        NEFORCE_THROW_EXCEPTION(dns_exception("Domain name cannot be empty"));
    }
    ensure_io_started();

    auto fut = async_query(domain, type, qclass, use_future);

    const auto total_budget =
            static_cast<uint64_t>(config_.timeout.count()) * (static_cast<uint64_t>(max_udp_retries_) + 2);
    const auto deadline = steady_clock::now() + milliseconds(static_cast<int64_t>(total_budget));
    while (fut.wait_for(milliseconds(0)) != future_status::ready) {
        const auto now = steady_clock::now();
        if (now >= deadline) {
            NEFORCE_THROW_EXCEPTION(dns_exception::network_error("DNS query timed out"));
        }
        const auto remaining = time_cast<milliseconds>(deadline - now);
        const int poll_ms = max(0, min(static_cast<int>(remaining.count()), 100));
        ctx_->run_one(poll_ms);
    }

    return fut.get();
}

future<dns_query_result> dns_client::query_async(const string& domain, const dns_record::raw type,
                                                 const dns_class qclass) {
    return async_query(domain.view(), type, qclass, use_future);
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
            const auto parts = record.data.split(" ");
            if (parts.size() >= 4) {
                srv.priority = to_uint16(parts[0].view());
                srv.weight = to_uint16(parts[1].view());
                srv.port = to_uint16(parts[2].view());
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
            const auto parts = record.data.split(" ");
            if (parts.size() >= 7) {
                soa.mname = parts[0];
                soa.rname = parts[1];
                soa.serial = to_uint32(parts[2].view());
                soa.refresh = to_uint32(parts[3].view());
                soa.retry = to_uint32(parts[4].view());
                soa.expire = to_uint32(parts[5].view());
                soa.minimum = to_uint32(parts[6].view());
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

    ensure_io_started();

    vector<future<dns_query_result>> futures;
    futures.reserve(domains.size());

    for (const auto& domain: domains) {
        futures.push_back(query_async(domain, type));
    }

    const auto total_budget =
            static_cast<uint64_t>(config_.timeout.count()) * (static_cast<uint64_t>(max_udp_retries_) + 2);
    const auto deadline = steady_clock::now() + milliseconds(total_budget);

    vector<dns_query_result> results;
    results.reserve(futures.size());

    for (auto& fut: futures) {
        while (fut.wait_for(milliseconds(0)) != future_status::ready) {
            if (steady_clock::now() >= deadline) {
                break;
            }
            const auto remaining = time_cast<milliseconds>(deadline - steady_clock::now());
            const int poll_ms = min(static_cast<int>(remaining.count()), 100);
            ctx_->run_one(poll_ms);
        }
        try {
            results.push_back(fut.get());
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
            const auto age = time_cast<seconds>(now - it->second.created_at);
            if (age < it->second.ttl) {
                return optional<dns_query_result>{it->second.result};
            }
        } else {
            return none;
        }
    }

    lock<shared_mutex> write_lock(cache_mutex_);
    const auto it = cache_.find(key);
    if (it != cache_.end()) {
        const auto now = steady_clock::now();
        const auto age = time_cast<seconds>(now - it->second.created_at);
        if (age >= it->second.ttl) {
            cache_.erase(it);
        } else {
            return optional<dns_query_result>{it->second.result};
        }
    }

    return none;
}

void dns_client::update_cache(const string& key, const dns_query_result& result) {
    lock<shared_mutex> write_lock(cache_mutex_);
    cache_[key] = cache_entry{result, steady_clock::now(), effective_cache_ttl(result, cache_ttl_)};
}

seconds dns_client::effective_cache_ttl(const dns_query_result& result, const seconds cap) {
    if (!result.is_success()) {
        return edns::NEGATIVE_CACHE_TTL;
    }

    uint32_t min_record_ttl = numeric_traits<uint32_t>::max();
    bool found = false;
    for (const auto& record: result.answers) {
        min_record_ttl = min(record.ttl, min_record_ttl);
        found = true;
    }
    if (!found) {
        return seconds(max<int64_t>(0, cap.count()));
    }

    const int64_t effective = min<int64_t>(static_cast<int64_t>(min_record_ttl), cap.count());
    return seconds(max<int64_t>(0, effective));
}

byte_vector dns_client::build_query(const string_view domain, const dns_record::raw type, const dns_class qclass,
                                    const bool rd, const bool edns_enable, const bool dnssec_ok,
                                    const uint16_t edns_payload, const string_view case_pattern) {
    const string_view qname = case_pattern.empty() ? domain : case_pattern;
    return build_dns_query(qname, type, qclass, rd, edns_enable, dnssec_ok, edns_payload);
}

dns_query_result dns_client::parse_response(const byte_vector& response, const uint16_t expected_id,
                                            const string_view expected_question) {
    return parse_dns_response(response, expected_id, expected_question);
}

NEFORCE_END_NAMESPACE__
