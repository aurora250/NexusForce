#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/network/socket/icmp_socket.hpp>
#include <NeForce/network/socket/ip_socket.hpp>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <cerrno>
#endif
NEFORCE_BEGIN_NAMESPACE__

namespace {
    uint16_t calculate_checksum(const void* data, size_t len) noexcept {
        uint32_t sum = 0;
        const auto* ptr = static_cast<const uint16_t*>(data);
        while (len > 1) {
            sum += *ptr++;
            len -= 2;
        }
        if (len != 0U) {
            sum += *reinterpret_cast<const uint8_t*>(ptr);
        }
        while (sum >> 16 != 0U) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return static_cast<uint16_t>(~sum);
    }
} // namespace


void icmp_socket::send_echo_request(const ip_address& dest, const uint16_t id, const uint16_t seq, const uint8_t ttl,
                                    const void* data, const size_t data_len) {

    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("ICMP socket not opened"));
    }

    const size_t packet_len = sizeof(icmp_header) + data_len;
    vector<char> packet(packet_len);
    auto* hdr = reinterpret_cast<icmp_header*>(packet.data());
    hdr->type = ICMP_ECHO_REQUEST;
    hdr->code = 0;
    hdr->id = endian::host_to_network<uint16_t>(id);
    hdr->sequence = endian::host_to_network<uint16_t>(seq);
    if (data != nullptr && data_len != 0U) {
        memory_copy(packet.data() + sizeof(icmp_header), data, data_len);
    }
    hdr->checksum = 0;
    hdr->checksum = calculate_checksum(packet.data(), packet_len);

#ifdef NEFORCE_PLATFORM_LINUX
    if (::setsockopt(fd_, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Set IP_TTL failed."));
    }
#endif
#ifdef NEFORCE_PLATFORM_WINDOWS
    if (::setsockopt(fd_, IPPROTO_IP, IP_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl)) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Set IP_TTL failed."));
    }
#endif

    const ssize_t sent = ::sendto(fd_, packet.data(), static_cast<int>(packet_len), 0, dest.data(), dest.size());
    if (sent < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("sendto failed"));
    }
}

bool icmp_socket::receive_reply(const milliseconds timeout, const uint16_t expected_id, const uint16_t expected_seq,
                                ip_address& sender, icmp_header& out_header, vector<char>& out_data,
                                uint8_t& recv_ttl) {

    const auto start = steady_clock::now();
    auto remaining = timeout;

    bool old_blocking = true;
    if (!set_nonblocking(true)) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Set nonblocking mode failed"));
    }

    char recv_buffer[65536];
    ::sockaddr_storage peer_addr{};
    ::socklen_t peer_len = sizeof(peer_addr);

    bool received = false;

    while (remaining.count() > 0 && !received) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(fd_, &read_fds);

        timeval tv{};
        tv.tv_sec = static_cast<long>(remaining.count() / 1000);
        tv.tv_usec = static_cast<long>((remaining.count() % 1000) * 1000);

#ifdef NEFORCE_PLATFORM_WINDOWS
        int sel_ret = ::select(0, &read_fds, nullptr, nullptr, &tv);
#else
        int sel_ret = ::select(static_cast<int>(fd_ + 1), &read_fds, nullptr, nullptr, &tv);
#endif

        if (sel_ret < 0) {
            const int err = socket_exception::last_error();
            if (err == EINTR) {
                auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            set_nonblocking(old_blocking);
            NEFORCE_THROW_EXCEPTION(socket_exception("select failed"));
        }

        if (sel_ret == 0) {
            // timeout
            break;
        }

        peer_len = sizeof(peer_addr);
        const ssize_t recv_len = ::recvfrom(fd_, static_cast<char*>(recv_buffer), sizeof(recv_buffer), 0,
                                            reinterpret_cast<::sockaddr*>(&peer_addr), &peer_len);
        if (recv_len < 0) {
            const int err = socket_exception::last_error();
            if (socket_exception::is_would_block(err)) {
                auto elapsed = steady_clock::now() - start;
                remaining = timeout - time_cast<milliseconds>(elapsed);
                continue;
            }
            set_nonblocking(old_blocking);
            NEFORCE_THROW_EXCEPTION(socket_exception("recvfrom failed"));
        }

        if (peer_addr.ss_family == AF_INET) {
            sender = ip_address(*reinterpret_cast<::sockaddr_in*>(&peer_addr));
        } else {
            continue; // ignore non-IPv4 pack
        }

        size_t ip_header_len = 0;
        const uint8_t* icmp_start = nullptr;
        size_t icmp_len = 0;

        if (static_cast<size_t>(recv_len) < sizeof(ip_header)) {
            continue;
        }
        const auto* ip = reinterpret_cast<const ip_header*>(recv_buffer);
        if (ip->version != 4) {
            continue;
        }
        ip_header_len = static_cast<size_t>(ip->ihl * 4);
        if (ip_header_len < 20 || ip_header_len > static_cast<size_t>(recv_len)) {
            continue;
        }
        icmp_start = reinterpret_cast<const uint8_t*>(recv_buffer) + ip_header_len;
        icmp_len = recv_len - ip_header_len;
        recv_ttl = ip->ttl;

        if (icmp_len < sizeof(icmp_header)) {
            continue;
        }

        icmp_header outer_hdr{};
        memory_copy(&outer_hdr, icmp_start, sizeof(icmp_header));
        outer_hdr.id = endian::network_to_host<uint16_t>(outer_hdr.id);
        outer_hdr.sequence = endian::network_to_host<uint16_t>(outer_hdr.sequence);
        outer_hdr.checksum = endian::network_to_host<uint16_t>(outer_hdr.checksum);

        if (outer_hdr.type == ICMP_ECHO_REPLY) {
            if (outer_hdr.id == expected_id && outer_hdr.sequence == expected_seq) {
                memory_copy(&out_header, &outer_hdr, sizeof(icmp_header));
                out_data.assign(icmp_start + sizeof(icmp_header), icmp_start + icmp_len);
                received = true;
                break;
            }
        } else if (outer_hdr.type == ICMP_TIME_EXCEEDED) {
            constexpr size_t min_orig_len = sizeof(ip_header) + sizeof(icmp_header);
            if (icmp_len < sizeof(icmp_header) + min_orig_len) {
                continue; // ignore
            }

            const auto* orig_ip = reinterpret_cast<const ip_header*>(icmp_start + sizeof(icmp_header));
            const auto orig_ip_header_len = static_cast<size_t>(orig_ip->ihl * 4);
            if (orig_ip_header_len < sizeof(ip_header) || orig_ip_header_len > icmp_len - sizeof(icmp_header)) {
                continue;
            }

            const auto* orig_icmp = reinterpret_cast<const icmp_header*>(reinterpret_cast<const uint8_t*>(orig_ip) +
                                                                         orig_ip_header_len);

            if (reinterpret_cast<const uint8_t*>(orig_icmp) + sizeof(icmp_header) > icmp_start + icmp_len) {
                continue;
            }

            const auto orig_id = endian::network_to_host<uint16_t>(orig_icmp->id);
            const auto orig_seq = endian::network_to_host<uint16_t>(orig_icmp->sequence);

            if (orig_id == expected_id && orig_seq == expected_seq) {
                memory_copy(&out_header, &outer_hdr, sizeof(icmp_header));
                out_data.assign(icmp_start, icmp_start + icmp_len);
                received = true;
                break;
            }
        }

        auto elapsed = steady_clock::now() - start;
        remaining = timeout - time_cast<milliseconds>(elapsed);
    }

    set_nonblocking(old_blocking);
    return received;
}

void icmp_socket::open(const int family) {
    if (family != AF_INET) {
        NEFORCE_THROW_EXCEPTION(value_exception("ICMP socket support IPv4 only"));
    }

    close();

    fd_ = ::socket(family, SOCK_RAW, IPPROTO_ICMP);
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("create raw ICMP socket failed (you may need root)"));
    }

#ifdef NEFORCE_PLATFORM_LINUX
    constexpr int on = 1;
    ::setsockopt(fd_, SOL_IP, IP_RECVTTL, &on, sizeof(on));
#endif
}

icmp_socket::ping_result icmp_socket::ping(const ip_address& dest, const milliseconds timeout, const uint16_t sequence,
                                           const void* data, const size_t data_len) {

    if (!dest.is_valid() || !dest.is_ipv4()) {
        NEFORCE_THROW_EXCEPTION(value_exception("ping target must be valid IPv4 address"));
    }

    const uint16_t id = process::current_id();
    send_echo_request(dest, id, sequence, 64, data, data_len);

    ip_address sender;
    icmp_header reply_hdr{};
    vector<char> reply_data;
    uint8_t reply_ttl = 0;
    const auto start_time = steady_clock::now();

    const bool got_reply = receive_reply(timeout, id, sequence, sender, reply_hdr, reply_data, reply_ttl);

    const auto end_time = steady_clock::now();
    const auto rtt = time_cast<milliseconds>(end_time - start_time);

    ping_result result{};
    result.destination = dest;
    result.rtt = rtt;
    result.success = got_reply && (reply_hdr.type == ICMP_ECHO_REPLY);
    result.reply_size = reply_data.size();
    result.reply_ttl = reply_ttl;
    return result;
}

vector<icmp_socket::traceroute_hop> icmp_socket::traceroute(const ip_address& dest, const int max_hops,
                                                            const milliseconds probe_timeout,
                                                            const int probes_per_hop) {

    if (!dest.is_valid() || !dest.is_ipv4()) {
        NEFORCE_THROW_EXCEPTION(value_exception("traceroute target must be valid IPv4 address"));
    }

    vector<traceroute_hop> hops;
    const uint16_t id = process::current_id();

    for (int ttl = 1; ttl <= max_hops; ++ttl) {
        traceroute_hop hop;
        hop.reached = false;
        hop.address = ip_address();

        for (int probe = 0; probe < probes_per_hop; ++probe) {
            const auto seq = static_cast<uint16_t>((ttl << 8) | probe);
            send_echo_request(dest, id, seq, static_cast<uint8_t>(ttl), nullptr, 0);

            ip_address sender;
            icmp_header reply_hdr{};
            vector<char> reply_data;
            uint8_t reply_ttl = 0;
            auto start_time = steady_clock::now();

            const bool got = receive_reply(probe_timeout, id, seq, sender, reply_hdr, reply_data, reply_ttl);

            const auto end_time = steady_clock::now();
            const auto rtt = time_cast<milliseconds>(end_time - start_time);

            if (got) {
                if (!hop.address.is_valid()) {
                    hop.address = sender;
                }
                hop.rtt[probe] = rtt;

                if (reply_hdr.type == ICMP_ECHO_REPLY) {
                    hop.reached = true;
                }
            } else {
                hop.rtt[probe] = milliseconds(-1);
            }
            this_thread::sleep_for(milliseconds(50));
        }

        hops.push_back(hop);

        if (hop.reached) {
            break;
        }
    }

    return hops;
}

NEFORCE_END_NAMESPACE__
