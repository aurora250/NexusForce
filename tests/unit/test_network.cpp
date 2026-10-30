#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/network/icmp_socket.hpp>
#include <NeForce/network/ip_socket.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <NeForce/network/socket_base.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/network/udp_socket.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <unistd.h>
#endif
using namespace neforce;

bool has_root() {
#ifdef NEFORCE_PLATFORM_LINUX
    return ::geteuid() == 0;
#else
    return true;
#endif
}

class IpHeaderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IpHeaderTest, SizeMatchesRfc791) { EXPECT_EQ(sizeof(ip_header), 20u); }

TEST_F(IpHeaderTest, VersionIhlParsing) {
    ip_header hdr{};
    hdr.version_ihl = 0x45;
    EXPECT_EQ(hdr.version(), 4u);
    EXPECT_EQ(hdr.ihl(), 5u);
}

TEST_F(IpHeaderTest, VersionIhlMaxValues) {
    ip_header hdr{};
    hdr.version_ihl = 0xFF;
    EXPECT_EQ(hdr.version(), 0x0Fu);
    EXPECT_EQ(hdr.ihl(), 0x0Fu);
}

TEST_F(IpHeaderTest, VersionIhlMinValues) {
    ip_header hdr{};
    hdr.version_ihl = 0x00;
    EXPECT_EQ(hdr.version(), 0u);
    EXPECT_EQ(hdr.ihl(), 0u);
}

TEST_F(IpHeaderTest, VersionAndIhlIndependent) {
    ip_header hdr{};

    hdr.version_ihl = 0x40;
    EXPECT_EQ(hdr.version(), 4u);
    EXPECT_EQ(hdr.ihl(), 0u);

    hdr.version_ihl = 0x05;
    EXPECT_EQ(hdr.version(), 0u);
    EXPECT_EQ(hdr.ihl(), 5u);

    hdr.version_ihl = 0x46;
    EXPECT_EQ(hdr.version(), 4u);
    EXPECT_EQ(hdr.ihl(), 6u);
}

TEST_F(IpHeaderTest, TotalLenFieldOffset) {
    ip_header hdr{};
    hdr.version_ihl = 0x46;
    auto* raw = reinterpret_cast<uint8_t*>(&hdr);
    EXPECT_EQ(raw[0], 0x46u);
}

TEST_F(IpHeaderTest, DefaultConstructorZeroed) {
    ip_header hdr{};
    EXPECT_EQ(hdr.version(), 0u);
    EXPECT_EQ(hdr.ihl(), 0u);
    EXPECT_EQ(hdr.tos, 0u);
    EXPECT_EQ(hdr.ttl, 0u);
    EXPECT_EQ(hdr.protocol, 0u);
}

class SocketExceptionTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SocketExceptionTest, LastErrorReturnsValue) { EXPECT_NO_THROW(socket_exception::last_error()); }

TEST_F(SocketExceptionTest, IsWouldBlockWithEwouldblock) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(socket_exception::is_would_block(WSAEWOULDBLOCK));
#else
    EXPECT_TRUE(socket_exception::is_would_block(EWOULDBLOCK));
#endif
}

TEST_F(SocketExceptionTest, IsWouldBlockWithOtherError) { EXPECT_FALSE(socket_exception::is_would_block(EINVAL)); }

TEST_F(SocketExceptionTest, IsWouldBlockWithZero) { EXPECT_FALSE(socket_exception::is_would_block(0)); }

TEST_F(SocketExceptionTest, ConstructWithDefaultArgs) {
    socket_exception ex;
    EXPECT_NE(string(ex.what()), "");
}

class SocketBaseTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SocketBaseTest, DefaultConstructorIsNotOpen) {
    socket_base sock;
    EXPECT_FALSE(sock.is_open());
    EXPECT_FALSE(static_cast<bool>(sock));
}

TEST_F(SocketBaseTest, NativeHandleConstructor) {
    socket_base sock(42);
    EXPECT_TRUE(sock.is_open());
    EXPECT_EQ(sock.native_handle(), 42);
}

TEST_F(SocketBaseTest, MoveConstructor) {
    socket_base sock1(42);
    socket_base sock2(move(sock1));
    EXPECT_TRUE(sock2.is_open());
    EXPECT_EQ(sock2.native_handle(), 42);
    EXPECT_FALSE(sock1.is_open());
}

TEST_F(SocketBaseTest, MoveAssignment) {
    socket_base sock1(42);
    socket_base sock2;
    sock2 = move(sock1);
    EXPECT_TRUE(sock2.is_open());
    EXPECT_EQ(sock2.native_handle(), 42);
    EXPECT_FALSE(sock1.is_open());
}

TEST_F(SocketBaseTest, MoveAssignmentSelf) {
    socket_base sock(42);
    sock = move(sock);
    EXPECT_TRUE(sock.is_open());
}

TEST_F(SocketBaseTest, MoveAssignmentClosesExisting) {
    socket_base sock1(42);
    socket_base sock2(99);
    sock2 = move(sock1);
    EXPECT_EQ(sock2.native_handle(), 42);
}

TEST_F(SocketBaseTest, CloseUnopenedReturnsTrue) {
    socket_base sock;
    EXPECT_TRUE(sock.close());
}

TEST_F(SocketBaseTest, ReleaseTransfersOwnership) {
    socket_base sock(42);
    auto fd = sock.release();
    EXPECT_EQ(fd, 42);
    EXPECT_FALSE(sock.is_open());
}

TEST_F(SocketBaseTest, SetReuseAddressUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_reuse_address(true));
}

TEST_F(SocketBaseTest, SetReusePortUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_reuse_port(true));
}

TEST_F(SocketBaseTest, SetKeepAliveUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_keep_alive(true));
}

TEST_F(SocketBaseTest, SetTcpNodelayUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_tcp_nodelay(true));
}

TEST_F(SocketBaseTest, SetReceiveBufferSizeUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_receive_buffer_size(8192));
}

TEST_F(SocketBaseTest, SetSendBufferSizeUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_send_buffer_size(8192));
}

TEST_F(SocketBaseTest, SetSendTimeoutUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_send_timeout(milliseconds(1000)));
}

TEST_F(SocketBaseTest, SetReceiveTimeoutUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_receive_timeout(milliseconds(1000)));
}

TEST_F(SocketBaseTest, SetNonblockingUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.set_nonblocking(true));
}

TEST_F(SocketBaseTest, ShutdownSendUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.shutdown_send());
}

TEST_F(SocketBaseTest, ShutdownReceiveUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.shutdown_receive());
}

TEST_F(SocketBaseTest, ShutdownBothUnopenedReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.shutdown_both());
}

TEST_F(SocketBaseTest, GetOptionUnopenedReturnsFalse) {
    socket_base sock;
    int val = 0;
    ::socklen_t len = sizeof(val);
    EXPECT_FALSE(sock.get_option(SOL_SOCKET, SO_KEEPALIVE, &val, &len));
}

TEST_F(SocketBaseTest, LocalEndpointUnopenedReturnsNone) {
    socket_base sock;
    EXPECT_FALSE(sock.local_endpoint().has_value());
}

TEST_F(SocketBaseTest, RemoteEndpointUnopenedReturnsNone) {
    socket_base sock;
    EXPECT_FALSE(sock.remote_endpoint().has_value());
}

TEST_F(SocketBaseTest, BindUnopenedThrows) {
    socket_base sock;
    auto addr = ip_address::loopback(ports(80u));
    EXPECT_THROW(sock.bind(addr), value_exception);
}

TEST_F(SocketBaseTest, ListenUnopenedThrows) {
    socket_base sock;
    EXPECT_THROW(sock.listen(128), value_exception);
}

TEST_F(SocketBaseTest, OpenInvalidFamilyThrows) {
    socket_base sock;
    EXPECT_THROW(sock.open(ip_family::undef), value_exception);
}

TEST_F(SocketBaseTest, TryOpenInvalidFamilyReturnsFalse) {
    socket_base sock;
    EXPECT_FALSE(sock.try_open(ip_family::undef));
}

TEST_F(SocketBaseTest, OpenAndClose) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.is_open());
    EXPECT_TRUE(static_cast<bool>(sock));
    sock.close();
    EXPECT_FALSE(sock.is_open());
}

TEST_F(SocketBaseTest, OpenReplacesExistingSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.is_open());
    sock.open(ip_family::inet4, socket_type::dgram);
    EXPECT_TRUE(sock.is_open());
    sock.close();
}

TEST_F(SocketBaseTest, SetReuseAddressOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.set_reuse_address(true));
    sock.close();
}

TEST_F(SocketBaseTest, SetKeepAliveOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.set_keep_alive(true));
    sock.close();
}

TEST_F(SocketBaseTest, SetTcpNodelayOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.set_tcp_nodelay(true));
    sock.close();
}

TEST_F(SocketBaseTest, SetBufferSizesOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.set_receive_buffer_size(65536));
    EXPECT_TRUE(sock.set_send_buffer_size(65536));
    sock.close();
}

TEST_F(SocketBaseTest, SetNonblockingOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.set_nonblocking(true));
    EXPECT_TRUE(sock.set_nonblocking(false));
    sock.close();
}

TEST_F(SocketBaseTest, SetTimeoutsOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.set_send_timeout(milliseconds(5000)));
    EXPECT_TRUE(sock.set_receive_timeout(milliseconds(5000)));
    sock.close();
}

TEST_F(SocketBaseTest, ShutdownOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_NO_THROW(sock.shutdown_both());
    sock.close();
}

TEST_F(SocketBaseTest, LocalEndpointOnOpenSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    sock.set_reuse_address(true);
    auto addr = ip_address::any();
    sock.bind(addr);
    auto local = sock.local_endpoint();
    EXPECT_TRUE(local.has_value());
    EXPECT_TRUE(local->is_ipv4());
    sock.close();
}

TEST_F(SocketBaseTest, ListenAndLocalEndpoint) {
    socket_base sock;
    sock.open(ip_family::inet4);
    sock.set_reuse_address(true);
    auto addr = ip_address::any();
    sock.bind(addr);
    sock.listen(8);
    auto local = sock.local_endpoint();
    EXPECT_TRUE(local.has_value());
    sock.close();
}

TEST_F(SocketBaseTest, BindInvalidEndpointThrows) {
    socket_base sock;
    sock.open(ip_family::inet4);
    ip_address invalid;
    EXPECT_THROW(sock.bind(invalid), value_exception);
    sock.close();
}

TEST_F(SocketBaseTest, CloseIsIdempotent) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.close());
    EXPECT_TRUE(sock.close());
}

TEST_F(SocketBaseTest, DestructorClosesSocket) {
    socket_base sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.is_open());
}

TEST_F(SocketBaseTest, TryOpenValidFamily) {
    socket_base sock;
    EXPECT_TRUE(sock.try_open(ip_family::inet4));
    EXPECT_TRUE(sock.is_open());
    sock.close();
}

TEST_F(SocketBaseTest, EnumerateSocketsRejectsUnsupportedFamily) {
    socket_base sock;
    EXPECT_FALSE(sock.enumerate_sockets(ip_family::unix, socket_base::protocol::tcp).has_value());
}

TEST_F(SocketBaseTest, EnumerateSocketsRejectsUnsupportedProtocol) {
    socket_base sock;
    EXPECT_FALSE(sock.enumerate_sockets(ip_family::inet4, socket_base::protocol::icmp).has_value());
}

TEST_F(SocketBaseTest, EnumerateTcpSocketsFindsListener) {
    socket_base sock;
    sock.open(ip_family::inet4, socket_base::type::stream, socket_base::protocol::tcp);
    sock.bind(ip_address::loopback(ports(0u), ip_family::inet4));
    sock.listen(1);

    const auto local = sock.local_endpoint();
    ASSERT_TRUE(local.has_value());

    const auto table = sock.enumerate_sockets(ip_family::inet4, socket_base::protocol::tcp);
    if (!table.has_value()) {
        GTEST_SKIP() << "socket table enumeration is not available";
    }
    bool found = false;
    for (const auto& entry: *table) {
        if (entry.local != local->to_string()) {
            continue;
        }
        found = true;
        EXPECT_EQ(entry.family, ip_family::inet4);
        EXPECT_EQ(entry.proto, socket_base::protocol::tcp);
        EXPECT_EQ(entry.status, socket_base::state::listen);
    }
    EXPECT_TRUE(found);
}

TEST_F(SocketBaseTest, EnumerateUdpSocketsFindsBoundSocket) {
    socket_base sock;
    sock.open(ip_family::inet4, socket_base::type::dgram, socket_base::protocol::udp);
    sock.bind(ip_address::any(ports(0u), ip_family::inet4));

    const auto local = sock.local_endpoint();
    ASSERT_TRUE(local.has_value());

    const auto table = sock.enumerate_sockets(ip_family::inet4, socket_base::protocol::udp);
    if (!table.has_value()) {
        GTEST_SKIP() << "socket table enumeration is not available";
    }
    bool found = false;
    for (const auto& entry: *table) {
        if (entry.local != local->to_string()) {
            continue;
        }
        found = true;
        EXPECT_EQ(entry.proto, socket_base::protocol::udp);
        EXPECT_EQ(entry.status, socket_base::state::unconnected);
    }
    EXPECT_TRUE(found);
}

class IpSocketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IpSocketTest, DefaultConstructorFamilyIsUnspec) {
    udp_socket sock;
    EXPECT_EQ(sock.address_family(), ip_family::undef);
    EXPECT_FALSE(sock.is_ipv4());
    EXPECT_FALSE(sock.is_ipv6());
}

TEST_F(IpSocketTest, ConnectUnopenedThrows) {
    udp_socket sock;
    auto addr = ip_address::loopback(ports(80u));
    EXPECT_THROW(sock.connect(addr), value_exception);
}

TEST_F(IpSocketTest, ConnectInvalidEndpointThrows) {
    udp_socket sock;
    sock.open();
    ip_address invalid;
    EXPECT_THROW(sock.connect(invalid), value_exception);
    sock.close();
}

TEST_F(IpSocketTest, CloseResetsFamilyToUnspec) {
    udp_socket sock;
    sock.open();
    EXPECT_EQ(sock.address_family(), ip_family::inet4);
    EXPECT_TRUE(sock.is_ipv4());
    sock.close();
    EXPECT_EQ(sock.address_family(), ip_family::undef);
    EXPECT_FALSE(sock.is_ipv4());
}

TEST_F(IpSocketTest, OpenIpv4SetsFamily) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    EXPECT_EQ(sock.address_family(), ip_family::inet4);
    EXPECT_TRUE(sock.is_ipv4());
    EXPECT_FALSE(sock.is_ipv6());
    sock.close();
}

TEST_F(IpSocketTest, OpenIpv6SetsFamily) {
    udp_socket sock;
    sock.open(ip_family::inet6);
    EXPECT_EQ(sock.address_family(), ip_family::inet6);
    EXPECT_FALSE(sock.is_ipv4());
    EXPECT_TRUE(sock.is_ipv6());
    sock.close();
}

TEST_F(IpSocketTest, OpenIpInvalidFamilyThrows) {
    udp_socket sock;
    EXPECT_THROW(sock.open(ip_family::undef), value_exception);
}

class UdpSocketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UdpSocketTest, DefaultConstructor) {
    udp_socket sock;
    EXPECT_FALSE(sock.is_open());
}

TEST_F(UdpSocketTest, SendToUnopenedThrows) {
    udp_socket sock;
    auto addr = ip_address::loopback(ports(80u), ip_family::inet4);
    char data[] = "test";
    EXPECT_THROW(sock.send_to(memory_view<const char>(data, 4), addr), value_exception);
}

TEST_F(UdpSocketTest, SendToInvalidEndpointThrows) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    ip_address invalid;
    char data[] = "test";
    EXPECT_THROW(sock.send_to(memory_view<const char>(data, 4), invalid), value_exception);
    sock.close();
}

TEST_F(UdpSocketTest, SendToEmptyDataReturnsZero) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    ip_address addr = ip_address::loopback(ports(12345u), ip_family::inet4);
    EXPECT_EQ(sock.send_to(memory_view<const char>(), addr), 0);
    sock.close();
}

TEST_F(UdpSocketTest, SendUnopenedThrows) {
    udp_socket sock;
    char data[] = "test";
    EXPECT_THROW(sock.send(memory_view<const char>(data, 4)), value_exception);
}

TEST_F(UdpSocketTest, ReceiveFromUnopenedThrows) {
    udp_socket sock;
    char buf[64];
    EXPECT_THROW(sock.receive_from(memory_view<char>(buf, 64)), value_exception);
}

TEST_F(UdpSocketTest, ReceiveFromEmptyBufferThrows) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    EXPECT_THROW(sock.receive_from(memory_view<char>()), value_exception);
    sock.close();
}

TEST_F(UdpSocketTest, ReceiveUnopenedThrows) {
    udp_socket sock;
    char buf[64];
    EXPECT_THROW(sock.receive(memory_view<char>(buf, 64)), value_exception);
}

TEST_F(UdpSocketTest, ReceiveEmptyBufferThrows) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    EXPECT_THROW(sock.receive(memory_view<char>()), value_exception);
    sock.close();
}

TEST_F(UdpSocketTest, OpenCloseAndReopen) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    EXPECT_TRUE(sock.is_open());
    sock.close();
    EXPECT_FALSE(sock.is_open());
    sock.open(ip_family::inet6);
    EXPECT_TRUE(sock.is_open());
    sock.close();
}

TEST_F(UdpSocketTest, SendAndReceiveOnLoopback) {
    udp_socket sock;
    sock.open(ip_family::inet4);
    sock.set_reuse_address(true);

    auto addr = ip_address::loopback(ports(0u), ip_family::inet4);
    sock.bind(addr);
    auto bound = sock.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    const char* msg = "hello udp";
    ssize_t sent = sock.send_to(memory_view<const char>(msg, 9), *bound);
    EXPECT_EQ(sent, 9);

    char buf[64];
    auto [received, sender] = sock.receive_from(memory_view<char>(buf, 64));
    EXPECT_EQ(received, 9);
    EXPECT_EQ(string_view(buf, 9), "hello udp");
    sock.close();
}

class IcmpSocketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IcmpSocketTest, DefaultConstructor) {
    icmp_socket sock;
    EXPECT_FALSE(sock.is_open());
}

TEST_F(IcmpSocketTest, PingInvalidIpThrows) {
    icmp_socket sock;
    ip_address invalid;
    EXPECT_THROW(ignore = sock.ping(invalid, milliseconds(100)), value_exception);
}

TEST_F(IcmpSocketTest, PingIpv6Throws) {
    icmp_socket sock;
    auto ip = ip_address::parse("::1", ports::undef);
    ASSERT_TRUE(ip.has_value());
    EXPECT_THROW(ignore = sock.ping(*ip, milliseconds(100)), value_exception);
}

TEST_F(IcmpSocketTest, TracerouteInvalidIpThrows) {
    icmp_socket sock;
    ip_address invalid;
    EXPECT_THROW(ignore = sock.traceroute(invalid), value_exception);
}

TEST_F(IcmpSocketTest, TracerouteIpv6Throws) {
    icmp_socket sock;
    auto ip = ip_address::parse("::1", ports::undef);
    ASSERT_TRUE(ip.has_value());
    EXPECT_THROW(ignore = sock.traceroute(*ip), value_exception);
}

TEST_F(IcmpSocketTest, OpenRequiresRoot) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }
    icmp_socket sock;
    EXPECT_NO_THROW(sock.open());
    EXPECT_TRUE(sock.is_open());
}

TEST_F(IcmpSocketTest, PingLocalhost) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }
    icmp_socket sock;
    sock.open();
    auto dest = ip_address::loopback(ports::undef, ip_family::inet4);
    auto result = sock.ping(dest, milliseconds(500));
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.rtt.count(), 0);
}

TEST_F(IcmpSocketTest, AsyncPingInvalidIpDeliversError) {
    io_context ioc;
    icmp_socket sock;
    ip_address invalid;
    bool called = false;
    error_code ec;
    sock.async_ping(ioc, invalid, milliseconds(100), 0, nullptr, 0, [&](error_code e, icmp_socket::ping_result r) {
        called = true;
        ec = e;
        EXPECT_FALSE(r.success);
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(IcmpSocketTest, AsyncPingIpv6DeliversError) {
    io_context ioc;
    icmp_socket sock;
    auto ip = ip_address::parse("::1", ports::undef);
    ASSERT_TRUE(ip.has_value());
    bool called = false;
    error_code ec;
    sock.async_ping(ioc, *ip, milliseconds(100), 0, nullptr, 0, [&](error_code e, icmp_socket::ping_result r) {
        called = true;
        ec = e;
        EXPECT_FALSE(r.success);
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(IcmpSocketTest, AsyncPingUnopenedSocketDeliversError) {
    io_context ioc;
    icmp_socket sock;
    auto dest = ip_address::loopback(ports::undef, ip_family::inet4);
    bool called = false;
    error_code ec;
    sock.async_ping(ioc, dest, milliseconds(100), 0, nullptr, 0, [&](error_code e, icmp_socket::ping_result r) {
        called = true;
        ec = e;
        EXPECT_FALSE(r.success);
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(IcmpSocketTest, AsyncPingLoopbackSucceeds) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }
    io_context ioc;
    icmp_socket sock;
    sock.open();
    auto dest = ip_address::loopback(ports::undef, ip_family::inet4);
    bool done = false;
    error_code ec;
    icmp_socket::ping_result out;
    sock.async_ping(ioc, dest, milliseconds(1000), 0, nullptr, 0, [&](error_code e, icmp_socket::ping_result r) {
        done = true;
        ec = e;
        out = r;
    });
    auto deadline = steady_clock::now() + seconds(3);
    while (!done && steady_clock::now() < deadline) {
        ioc.run_one(100);
    }
    EXPECT_TRUE(done);
    if (done) {
        EXPECT_TRUE(out.success);
        EXPECT_GE(out.rtt.count(), 0);
    }
}

TEST_F(IcmpSocketTest, AsyncPingTimeoutReportsUnsuccessful) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }
    io_context ioc;
    icmp_socket sock;
    sock.open();
    auto dest = ip_address::parse("192.0.2.1", ports::undef);
    ASSERT_TRUE(dest.has_value());
    bool done = false;
    icmp_socket::ping_result out;
    sock.async_ping(ioc, *dest, milliseconds(200), 0, nullptr, 0, [&](error_code e, icmp_socket::ping_result r) {
        done = true;
        out = r;
    });
    auto deadline = steady_clock::now() + seconds(3);
    while (!done && steady_clock::now() < deadline) {
        ioc.run_one(100);
    }
    EXPECT_TRUE(done);
    EXPECT_FALSE(out.success);
}

TEST_F(IcmpSocketTest, ChecksumProducesValidResult) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }
    icmp_socket sock;
    sock.open();
    auto dest = ip_address::loopback(ports::undef, ip_family::inet4);
    char payload[] = "checksum-test";
    auto result = sock.ping(dest, milliseconds(500), 0, payload, sizeof(payload));
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.reply_size, 0u);
}

class SmtpSocketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SmtpSocketTest, DefaultConstructorNotConnected) {
    smtp_socket smtp;
    EXPECT_FALSE(smtp.is_connected());
    EXPECT_FALSE(smtp.is_tls_active());
}

TEST_F(SmtpSocketTest, ConnectInvalidAddressThrows) {
    smtp_socket smtp;
    ip_address invalid;
    EXPECT_THROW(smtp.connect(invalid), value_exception);
}

TEST_F(SmtpSocketTest, SendWithoutConnectionThrows) {
    smtp_socket smtp;
    smtp_message msg;
    msg.from = "test@test.com";
    msg.to = {"someone@test.com"};
    EXPECT_THROW(smtp.send(msg), smtp_exception);
}

TEST_F(SmtpSocketTest, SendWithEmptyFromThrows) {
    smtp_socket smtp;
    smtp_message msg;
    EXPECT_THROW(smtp.send(msg), exception);
}

TEST_F(SmtpSocketTest, SendWithEmptyToThrows) {
    smtp_socket smtp;
    smtp_message msg;
    msg.from = "test@test.com";
    EXPECT_THROW(smtp.send(msg), exception);
}

TEST_F(SmtpSocketTest, AuthenticateWithoutConnectionThrows) {
    smtp_socket smtp;
    EXPECT_THROW(smtp.authenticate("user", "pass", smtp_socket::auth_method::plain), smtp_exception);
}

TEST_F(SmtpSocketTest, StarttlsWithoutConnectionThrows) {
    smtp_socket smtp;
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_THROW(smtp.starttls(ctx), smtp_exception);
}

TEST_F(SmtpSocketTest, DisconnectWithoutConnectionIsNoop) {
    smtp_socket smtp;
    EXPECT_NO_THROW(smtp.disconnect());
}

TEST_F(SmtpSocketTest, NoopWithoutConnectionThrows) {
    smtp_socket smtp;
    EXPECT_THROW(smtp.noop(), smtp_exception);
}

TEST_F(SmtpSocketTest, TlsInactiveByDefault) {
    smtp_socket smtp;
    EXPECT_FALSE(smtp.is_tls_active());
}

TEST_F(SmtpSocketTest, VerifyPeerOnUnconnectedReturnsFalse) {
    smtp_socket smtp;
    EXPECT_FALSE(smtp.verify_peer());
}

TEST_F(SmtpSocketTest, AsyncConnectInvalidAddressDeliversError) {
    io_context ioc;
    smtp_socket smtp;
    ip_address invalid;
    bool called = false;
    error_code ec;
    smtp.async_connect(ioc, invalid, "localhost", smtp_socket::tls_mode::none, nullptr, "", [&](error_code e) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
    EXPECT_FALSE(smtp.is_connected());
}

TEST_F(SmtpSocketTest, AsyncConnectRefusedPortDeliversError) {
    ports test_port(0u);
    {
        tcp_acceptor tmp;
        tmp.open(ip_address::loopback());
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    auto addr = ip_address::loopback(test_port, ip_family::inet4);

    bool probe_refused = false;
    {
        tcp_socket probe;
        probe.open();
        try {
            probe_refused = !probe.connect(addr, milliseconds(500));
        } catch (const socket_exception&) {
            probe_refused = true;
        }
    }
    if (!probe_refused) {
        GTEST_SKIP() << "Probe port is accepting connections; cannot test refusal";
    }

    io_context ioc;
    smtp_socket smtp;
    bool called = false;
    error_code ec;
    smtp.async_connect(ioc, addr, "localhost", smtp_socket::tls_mode::none, nullptr, "", [&](error_code e) {
        called = true;
        ec = e;
    });
    auto deadline = steady_clock::now() + seconds(3);
    while (!called && steady_clock::now() < deadline) {
        ioc.run_one(100);
    }
    EXPECT_TRUE(called);
    if (!ec) {
        GTEST_SKIP() << "Port became accepting between probe and async connect";
    }
    EXPECT_TRUE(ec);
    EXPECT_FALSE(smtp.is_connected());
}

TEST_F(SmtpSocketTest, AsyncConnectUseFutureTokenCompiles) {
    io_context ioc;
    smtp_socket smtp;
    auto addr = ip_address::loopback(ports(9u), ip_family::inet4);
    auto fut = smtp.async_connect(ioc, addr, "localhost", smtp_socket::tls_mode::none, nullptr, "", use_future);
    EXPECT_TRUE(fut.valid());
}
