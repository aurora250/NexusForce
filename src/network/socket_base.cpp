#include <NeForce/core/memory/endian.hpp>
#include <NeForce/core/string/format.hpp>
#include <NeForce/network/socket_base.hpp>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <iphlpapi.h>
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <arpa/inet.h>
#    include <cerrno>
#    include <fcntl.h>
#    include <netinet/tcp.h>
#    include <unistd.h>
#endif
#ifdef NEFORCE_PLATFORM_LINUX
#    include <linux/inet_diag.h>
#    include <linux/netlink.h>
#    include <linux/sock_diag.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_PLATFORM_WINDOWS
namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct winsock_guard {
        winsock_guard() {
            ::WSADATA wsa_data;
            if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
                NEFORCE_THROW_EXCEPTION(system_exception("WSAStartup failed"));
            }
        }
        ~winsock_guard() { ::WSACleanup(); }
        winsock_guard(const winsock_guard&) = delete;
        winsock_guard& operator=(const winsock_guard&) = delete;
    };

    void ensure_winsock() { static winsock_guard guard{}; }

    string endpoint_text(const uint32_t address, const uint32_t port) {
        char text[INET_ADDRSTRLEN] = {};
        if (::inet_ntop(AF_INET, &address, text, sizeof(text)) == nullptr) {
            return {};
        }
        return format("{}:{}", text, endian::network_to_host(static_cast<uint16_t>(port)));
    }

    string endpoint_text(const uint8_t* address, const uint32_t port) {
        char text[INET6_ADDRSTRLEN] = {};
        if (::inet_ntop(AF_INET6, address, text, sizeof(text)) == nullptr) {
            return {};
        }
        return format("[{}]:{}", text, endian::network_to_host(static_cast<uint16_t>(port)));
    }
} // namespace
#endif

#ifndef NEFORCE_STANDARD_17
constexpr socket_base::native_handle_type socket_base::invalid_handle;
#endif

bool socket_exception::is_would_block(int error) noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
    return error == WSAEWOULDBLOCK;
#else
    return error == EWOULDBLOCK;
#endif
}

socket_base::socket_base() {
#ifdef NEFORCE_PLATFORM_WINDOWS
    ensure_winsock();
#endif
}

socket_base& socket_base::operator=(socket_base&& other) noexcept {
    if (addressof(other) == this) {
        return *this;
    }
    close();
    fd_ = exchange(other.fd_, invalid_handle);
    nonblocking_ = exchange(other.nonblocking_, false);
    return *this;
}

void socket_base::open(const ip_family f, const type t, const protocol p) {
    if (f != ip_family::inet4 && f != ip_family::inet6) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid address family for socket"));
    }

    close();

    fd_ = ::socket(static_cast<int>(f), static_cast<int>(t), static_cast<int>(p));
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to create socket"));
    }
}

bool socket_base::close() noexcept {
    if (!is_open()) {
        return true;
    }

    bool success = false;
#ifdef NEFORCE_PLATFORM_WINDOWS
    success = (::closesocket(fd_) == 0);
#else
    success = (::close(fd_) == 0);
#endif

    fd_ = invalid_handle;
    nonblocking_ = false;
    return success;
}

bool socket_base::try_open(const ip_family f, const type t, const protocol p) noexcept {
    if (f != ip_family::inet4 && f != ip_family::inet6) {
        return false;
    }

    close();
    fd_ = ::socket(static_cast<int>(f), static_cast<int>(t), static_cast<int>(p));
    return is_open();
}

bool socket_base::set_nonblocking(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::u_long mode = enable ? 1 : 0;
    if (::ioctlsocket(fd_, FIONBIO, &mode) != 0) {
        return false;
    }
#else
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (::fcntl(fd_, F_SETFL, flags) == -1) {
        return false;
    }
#endif
    nonblocking_ = enable;
    return true;
}

bool socket_base::is_nonblocking() const noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef NEFORCE_PLATFORM_WINDOWS
    return nonblocking_;
#else
    const int flags = ::fcntl(fd_, F_GETFL, 0);
    return flags != -1 && (flags & O_NONBLOCK) != 0;
#endif
}

bool socket_base::shutdown_send() noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::shutdown(fd_, SD_SEND) == 0;
#else
    return ::shutdown(fd_, SHUT_WR) == 0;
#endif
}

bool socket_base::shutdown_receive() noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::shutdown(fd_, SD_RECEIVE) == 0;
#else
    return ::shutdown(fd_, SHUT_RD) == 0;
#endif
}

bool socket_base::shutdown_both() noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    return ::shutdown(fd_, SD_BOTH) == 0;
#else
    return ::shutdown(fd_, SHUT_RDWR) == 0;
#endif
}

bool socket_base::set_option(const int level, const int optname, const void* value, const ::socklen_t len) noexcept {
    return ::setsockopt(fd_, level, optname, static_cast<const char*>(value), len) == 0;
}

bool socket_base::get_option(const int level, const int optname, void* optval, ::socklen_t* optlen) const noexcept {
    if (!is_open()) {
        return false;
    }
    return ::getsockopt(fd_, level, optname, static_cast<char*>(optval), optlen) == 0;
}

bool socket_base::set_reuse_address(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

bool socket_base::get_reuse_address() const noexcept {
    if (!is_open()) {
        return false;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_REUSEADDR, &value, &optlen)) {
        return false;
    }
    return value != 0;
}

bool socket_base::set_reuse_port(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef SO_REUSEPORT
    const int value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value));
#else
    ignore = enable;
    return false;
#endif
}

bool socket_base::get_reuse_port() const noexcept {
    if (!is_open()) {
        return false;
    }
#ifdef SO_REUSEPORT
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_REUSEPORT, &value, &optlen)) {
        return false;
    }
    return value != 0;
#else
    return false;
#endif
}

bool socket_base::set_keep_alive(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
    const ::socklen_t value = enable ? 1 : 0;
    return set_option(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
}

bool socket_base::get_keep_alive() const noexcept {
    if (!is_open()) {
        return false;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_KEEPALIVE, &value, &optlen)) {
        return false;
    }
    return value != 0;
}

bool socket_base::set_tcp_nodelay(const bool enable) noexcept {
    if (!is_open()) {
        return false;
    }
    const ::socklen_t value = enable ? 1 : 0;
    return set_option(IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
}

bool socket_base::get_tcp_nodelay() const noexcept {
    if (!is_open()) {
        return false;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(IPPROTO_TCP, TCP_NODELAY, &value, &optlen)) {
        return false;
    }
    return value != 0;
}

bool socket_base::set_receive_buffer_size(const int size) noexcept {
    if (!is_open()) {
        return false;
    }
    return set_option(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

int socket_base::get_receive_buffer_size() const noexcept {
    if (!is_open()) {
        return -1;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_RCVBUF, &value, &optlen)) {
        return -1;
    }
    return value;
}

bool socket_base::set_send_buffer_size(const int size) noexcept {
    if (!is_open()) {
        return false;
    }
    return set_option(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

int socket_base::get_send_buffer_size() const noexcept {
    if (!is_open()) {
        return -1;
    }
    int value = 0;
    ::socklen_t optlen = sizeof(value);
    if (!get_option(SOL_SOCKET, SO_SNDBUF, &value, &optlen)) {
        return -1;
    }
    return value;
}

optional<milliseconds> socket_base::get_send_timeout() const noexcept {
    if (!is_open()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD ms = 0;
    int optlen = sizeof(ms);
    if (!get_option(SOL_SOCKET, SO_SNDTIMEO, &ms, &optlen)) {
        return none;
    }
    return milliseconds(ms);
#else
    ::timeval tv = {};
    ::socklen_t optlen = sizeof(tv);
    if (!get_option(SOL_SOCKET, SO_SNDTIMEO, &tv, &optlen)) {
        return none;
    }
    return milliseconds(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

bool socket_base::set_send_timeout(const milliseconds timeout) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto ms = static_cast<::DWORD>(timeout.count());
    return set_option(SOL_SOCKET, SO_SNDTIMEO, &ms, sizeof(ms));
#else
    ::timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    return set_option(SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

bool socket_base::set_receive_timeout(const milliseconds timeout) noexcept {
    if (!is_open()) {
        return false;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto ms = static_cast<::DWORD>(timeout.count());
    return set_option(SOL_SOCKET, SO_RCVTIMEO, &ms, sizeof(ms));
#else
    ::timeval tv;
    tv.tv_sec = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    return set_option(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

optional<milliseconds> socket_base::get_receive_timeout() const noexcept {
    if (!is_open()) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    ::DWORD ms = 0;
    int optlen = sizeof(ms);
    if (!get_option(SOL_SOCKET, SO_RCVTIMEO, &ms, &optlen)) {
        return none;
    }
    return milliseconds(ms);
#else
    ::timeval tv = {};
    ::socklen_t optlen = sizeof(tv);
    if (!get_option(SOL_SOCKET, SO_RCVTIMEO, &tv, &optlen)) {
        return none;
    }
    return milliseconds(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

optional<ip_address> socket_base::local_endpoint() const {
    if (!is_open()) {
        return none;
    }

    ::sockaddr_storage storage{};
    ::socklen_t len = sizeof(storage);

    if (::getsockname(fd_, reinterpret_cast<::sockaddr*>(&storage), &len) != 0) {
        return none;
    }

    if (storage.ss_family == AF_INET) {
        return ip_address(*reinterpret_cast<::sockaddr_in*>(&storage));
    }
    if (storage.ss_family == AF_INET6) {
        return ip_address(*reinterpret_cast<::sockaddr_in6*>(&storage));
    }

    return none;
}

optional<ip_address> socket_base::remote_endpoint() const {
    if (!is_open()) {
        return none;
    }

    ::sockaddr_storage storage{};
    ::socklen_t len = sizeof(storage);

    if (::getpeername(fd_, reinterpret_cast<::sockaddr*>(&storage), &len) != 0) {
        return none;
    }

    if (storage.ss_family == AF_INET) {
        return ip_address(*reinterpret_cast<::sockaddr_in*>(&storage));
    }
    if (storage.ss_family == AF_INET6) {
        return ip_address(*reinterpret_cast<::sockaddr_in6*>(&storage));
    }

    return none;
}

void socket_base::bind(const ip_address& endpoint) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (!endpoint.is_valid()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Invalid endpoint for UDP bind"));
    }

    if (::bind(fd_, endpoint.data(), endpoint.size()) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("Failed to bind UDP socket to endpoint"));
    }
}

void socket_base::listen(const int backlog) {
    if (!is_open()) {
        NEFORCE_THROW_EXCEPTION(value_exception("Socket is not open"));
    }

    if (::listen(fd_, backlog) < 0) {
        NEFORCE_THROW_EXCEPTION(socket_exception("PORT: listen failed"));
    }
}

socket_base::state socket_base::to_state(const uint8_t raw, const bool udp) noexcept {
    if (udp) {
        // Linux UDP: 7 = TCP_CLOSE (UNCONN), 1 = TCP_ESTABLISHED
        // Windows UDP is non-state
#ifdef NEFORCE_PLATFORM_WINDOWS
        return raw == 0 ? state::unconnected : state::established;
#else
        if (raw == 7) {
            return state::unconnected;
        }
        if (raw == 1) {
            return state::established;
        }
        return state::unknown;
#endif
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    // Windows MIB_TCP_STATE
    switch (raw) {
        case MIB_TCP_STATE_CLOSED:
            return state::closed;
        case MIB_TCP_STATE_LISTEN:
            return state::listen;
        case MIB_TCP_STATE_SYN_SENT:
            return state::syn_sent;
        case MIB_TCP_STATE_SYN_RCVD:
            return state::syn_recv;
        case MIB_TCP_STATE_ESTAB:
            return state::established;
        case MIB_TCP_STATE_FIN_WAIT1:
            return state::fin_wait1;
        case MIB_TCP_STATE_FIN_WAIT2:
            return state::fin_wait2;
        case MIB_TCP_STATE_CLOSE_WAIT:
            return state::close_wait;
        case MIB_TCP_STATE_CLOSING:
            return state::closing;
        case MIB_TCP_STATE_LAST_ACK:
            return state::last_ack;
        case MIB_TCP_STATE_TIME_WAIT:
            return state::time_wait;
        case MIB_TCP_STATE_DELETE_TCB:
            return state::delete_tcb;
        default:
            return state::unknown;
    }
#else
    // Linux inet_diag state (TCP_ESTABLISHED=1 ... TCP_NEW_SYN_RECV=12)
    if (raw >= 1 && raw <= 12) {
        return static_cast<state>(raw);
    }
    return state::unknown;
#endif
}

string_view socket_base::state_name(const state st) noexcept {
    switch (st) {
        case state::established:
            return "ESTABLISHED";
        case state::syn_sent:
            return "SYN_SENT";
        case state::syn_recv:
            return "SYN_RECV";
        case state::fin_wait1:
            return "FIN_WAIT1";
        case state::fin_wait2:
            return "FIN_WAIT2";
        case state::time_wait:
            return "TIME_WAIT";
        case state::closed:
            return "CLOSE";
        case state::close_wait:
            return "CLOSE_WAIT";
        case state::last_ack:
            return "LAST_ACK";
        case state::listen:
            return "LISTEN";
        case state::closing:
            return "CLOSING";
        case state::new_syn_recv:
            return "NEW_SYN_RECV";
        case state::delete_tcb:
            return "DELETE_TCB";
        case state::unconnected:
            return "UNCONN";
        default:
            return "UNKNOWN";
    }
}

string_view socket_base::state_name(const uint8_t raw, const bool udp) noexcept {
    return state_name(to_state(raw, udp));
}

optional<vector<socket_base::diag_entry>> socket_base::enumerate_sockets(const ip_family family, const protocol proto) {
    if (family != ip_family::inet4 && family != ip_family::inet6) {
        return none;
    }
    if (proto != protocol::tcp && proto != protocol::udp) {
        return none;
    }

#ifdef NEFORCE_PLATFORM_LINUX
    const bool udp = proto == protocol::udp;
    struct request_message {
        ::nlmsghdr header;
        ::inet_diag_req_v2 request;
    } message{};
    message.header.nlmsg_len = NLMSG_LENGTH(sizeof(::inet_diag_req_v2));
    message.header.nlmsg_type = SOCK_DIAG_BY_FAMILY;
    message.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    message.header.nlmsg_seq = 1;
    message.request.sdiag_family = static_cast<uint8_t>(family == ip_family::inet4 ? AF_INET : AF_INET6);
    message.request.sdiag_protocol = static_cast<uint8_t>(udp ? IPPROTO_UDP : IPPROTO_TCP);
    message.request.idiag_states = 0xFFFFFFFFU;

    const int netlink_fd = ::socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_SOCK_DIAG);
    if (netlink_fd < 0) {
        return none;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    struct netlink_closer {
        int fd;
        ~netlink_closer() { ::close(fd); }
    } closer{netlink_fd};

    ::sockaddr_nl kernel{};
    kernel.nl_family = AF_NETLINK;
    if (::sendto(netlink_fd, &message, message.header.nlmsg_len, 0, reinterpret_cast<const ::sockaddr*>(&kernel),
                 sizeof(kernel)) < 0) {
        return none;
    }

    vector<diag_entry> entries;
    alignas(::nlmsghdr) char response[16384] = {};
    for (;;) {
        const ssize_t bytes = ::recv(netlink_fd, response, sizeof(response), 0);
        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            return none;
        }
        if (bytes == 0) {
            return none;
        }
        auto remaining = static_cast<unsigned int>(bytes);
        for (auto* header = reinterpret_cast<struct ::nlmsghdr*>(response); NLMSG_OK(header, remaining);
             header = NLMSG_NEXT(header, remaining)) {
            if (header->nlmsg_type == NLMSG_DONE) {
                return entries;
            }
            if (header->nlmsg_type == NLMSG_ERROR) {
                return none;
            }
            if (header->nlmsg_type != SOCK_DIAG_BY_FAMILY) {
                continue;
            }
            const auto* socket_info = static_cast<const struct ::inet_diag_msg*>(NLMSG_DATA(header));
            diag_entry entry;
            entry.family = family;
            entry.proto = proto;
            entry.status = to_state(socket_info->idiag_state, udp);
            entry.uid = socket_info->idiag_uid;
            entry.inode = socket_info->idiag_inode;
            entry.cookie = (static_cast<uint64_t>(socket_info->id.idiag_cookie[1]) << 32) |
                           static_cast<uint64_t>(socket_info->id.idiag_cookie[0]);
            entry.receive_queue = socket_info->idiag_rqueue;
            entry.send_queue = socket_info->idiag_wqueue;

            char text[INET6_ADDRSTRLEN] = {};
            const uint16_t local_port = endian::network_to_host(socket_info->id.idiag_sport);
            const uint16_t remote_port = endian::network_to_host(socket_info->id.idiag_dport);
            if (family == ip_family::inet4) {
                ::inet_ntop(AF_INET, socket_info->id.idiag_src, text, sizeof(text));
                entry.local = format("{}:{}", text, local_port);
                ::inet_ntop(AF_INET, socket_info->id.idiag_dst, text, sizeof(text));
                entry.remote = format("{}:{}", text, remote_port);
            } else {
                ::inet_ntop(AF_INET6, socket_info->id.idiag_src, text, sizeof(text));
                entry.local = format("[{}]:{}", text, local_port);
                ::inet_ntop(AF_INET6, socket_info->id.idiag_dst, text, sizeof(text));
                entry.remote = format("[{}]:{}", text, remote_port);
            }
            entries.push_back(move(entry));
        }
    }
#elif defined(NEFORCE_PLATFORM_WINDOWS)
    const ::ULONG address_family = family == ip_family::inet4 ? AF_INET : AF_INET6;
    vector<diag_entry> entries;

    if (proto == protocol::tcp) {
        ::ULONG size = 0;
        const ::TCP_TABLE_CLASS table_class = TCP_TABLE_OWNER_PID_ALL;
        if (::GetExtendedTcpTable(nullptr, &size, FALSE, address_family, table_class, 0) != ERROR_INSUFFICIENT_BUFFER ||
            size == 0) {
            return none;
        }
        vector<::BYTE> buffer(size);
        if (::GetExtendedTcpTable(buffer.data(), &size, FALSE, address_family, table_class, 0) != NO_ERROR) {
            return none;
        }
        if (address_family == AF_INET) {
            const auto* table = reinterpret_cast<const ::MIB_TCPTABLE_OWNER_PID*>(buffer.data());
            for (::DWORD i = 0; i < table->dwNumEntries; ++i) {
                const auto& row = table->table[i];
                diag_entry entry;
                entry.family = family;
                entry.proto = proto;
                entry.status = to_state(static_cast<uint8_t>(row.dwState));
                entry.pid = row.dwOwningPid;
                entry.local = endpoint_text(row.dwLocalAddr, row.dwLocalPort);
                entry.remote = endpoint_text(row.dwRemoteAddr, row.dwRemotePort);
                entries.push_back(move(entry));
            }
        } else {
            const auto* table = reinterpret_cast<const ::MIB_TCP6TABLE_OWNER_PID*>(buffer.data());
            for (::DWORD i = 0; i < table->dwNumEntries; ++i) {
                const auto& row = table->table[i];
                diag_entry entry;
                entry.family = family;
                entry.proto = proto;
                entry.status = to_state(static_cast<uint8_t>(row.dwState));
                entry.pid = row.dwOwningPid;
                entry.local = endpoint_text(row.ucLocalAddr, row.dwLocalPort);
                entry.remote = endpoint_text(row.ucRemoteAddr, row.dwRemotePort);
                entries.push_back(move(entry));
            }
        }
    } else {
        ::ULONG size = 0;
        const ::UDP_TABLE_CLASS table_class = UDP_TABLE_OWNER_PID;
        if (::GetExtendedUdpTable(nullptr, &size, FALSE, address_family, table_class, 0) != ERROR_INSUFFICIENT_BUFFER ||
            size == 0) {
            return none;
        }
        vector<::BYTE> buffer(size);
        if (::GetExtendedUdpTable(buffer.data(), &size, FALSE, address_family, table_class, 0) != NO_ERROR) {
            return none;
        }
        if (address_family == AF_INET) {
            const auto* table = reinterpret_cast<const ::MIB_UDPTABLE_OWNER_PID*>(buffer.data());
            for (::DWORD i = 0; i < table->dwNumEntries; ++i) {
                const auto& row = table->table[i];
                diag_entry entry;
                entry.family = family;
                entry.proto = proto;
                entry.status = state::unconnected;
                entry.pid = row.dwOwningPid;
                entry.local = endpoint_text(row.dwLocalAddr, row.dwLocalPort);
                entries.push_back(move(entry));
            }
        } else {
            const auto* table = reinterpret_cast<const ::MIB_UDP6TABLE_OWNER_PID*>(buffer.data());
            for (::DWORD i = 0; i < table->dwNumEntries; ++i) {
                const auto& row = table->table[i];
                diag_entry entry;
                entry.family = family;
                entry.proto = proto;
                entry.status = state::unconnected;
                entry.pid = row.dwOwningPid;
                entry.local = endpoint_text(row.ucLocalAddr, row.dwLocalPort);
                entries.push_back(move(entry));
            }
        }
    }
    return entries;
#else
    return none;
#endif
}

NEFORCE_END_NAMESPACE__
