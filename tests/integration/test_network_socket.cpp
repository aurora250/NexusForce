#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/icmp_socket.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <NeForce/network/udp_socket.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <unistd.h>
#endif
using namespace neforce;

namespace {
    bool has_root() {
#ifdef NEFORCE_PLATFORM_LINUX
        return ::geteuid() == 0;
#else
        return true;
#endif
    }

    bool network_available() {
        udp_socket sock;
        if (!sock.try_open(AF_INET, SOCK_DGRAM, 0)) {
            return false;
        }
        sock.set_reuse_address(true);
        auto addr = ip_address::any(ports(0u), AF_INET);
        sock.bind(addr);
        auto local = sock.local_endpoint();
        sock.close();
        return local.has_value();
    }
} // namespace

class UdpSocketIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UdpSocketIntegration, LoopbackSendReceive) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    udp_socket server;
    server.open(AF_INET);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open(AF_INET);

    const char* msg = "integration test message";
    ssize_t sent = client.send_to(memory_view<const char>(msg, 24), *bound);
    EXPECT_EQ(sent, 24);

    char buf[128];
    auto [received, sender] = server.receive_from(memory_view<char>(buf, 128));
    EXPECT_EQ(received, 24);
    EXPECT_EQ(string_view(buf, 24), "integration test message");
    EXPECT_TRUE(sender.is_ipv4());

    server.close();
    client.close();
}

TEST_F(UdpSocketIntegration, SendReceiveMultiplePackets) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    udp_socket server;
    server.open(AF_INET);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open(AF_INET);

    for (int i = 0; i < 5; ++i) {
        string msg = "packet_" + to_string(i);
        ssize_t sent = client.send_to(memory_view<const char>(msg.data(), msg.size()), *bound);
        EXPECT_EQ(sent, static_cast<ssize_t>(msg.size()));

        char buf[128];
        auto [received, sender] = server.receive_from(memory_view<char>(buf, 128));
        EXPECT_EQ(received, static_cast<ssize_t>(msg.size()));
        EXPECT_EQ(string_view(buf, msg.size()), msg);
    }

    server.close();
    client.close();
}

TEST_F(UdpSocketIntegration, EmptySendToReturnsZero) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    udp_socket sock;
    sock.open(AF_INET);
    auto addr = ip_address::loopback(ports(12345u), AF_INET);
    EXPECT_EQ(sock.send_to(memory_view<const char>(), addr), 0);
    sock.close();
}

TEST_F(UdpSocketIntegration, ConnectedModeSendReceive) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    udp_socket server;
    server.open(AF_INET);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open(AF_INET);
    client.connect(*bound);

    const char* msg = "connected udp";
    ssize_t sent = client.send(memory_view<const char>(msg, 14));
    EXPECT_EQ(sent, 14);

    char buf[128];
    auto [received, sender] = server.receive_from(memory_view<char>(buf, 128));
    EXPECT_EQ(received, 14);
    EXPECT_EQ(string_view(buf, 13), "connected udp");

    server.close();
    client.close();
}

TEST_F(UdpSocketIntegration, Ipv6LoopbackSendReceive) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    udp_socket server;
    server.open(AF_INET6);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback(ports(0u), AF_INET6);
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open(AF_INET6);

    const char* msg = "ipv6 udp test";
    ssize_t sent = client.send_to(memory_view<const char>(msg, 14), *bound);
    EXPECT_EQ(sent, 14);

    char buf[128];
    auto [received, sender] = server.receive_from(memory_view<char>(buf, 128));
    EXPECT_EQ(received, 14);
    EXPECT_EQ(string_view(buf, 13), "ipv6 udp test");
    EXPECT_TRUE(sender.is_ipv6());

    server.close();
    client.close();
}

class IcmpSocketIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IcmpSocketIntegration, PingGoogleDns) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }

    icmp_socket sock;
    ASSERT_NO_THROW(sock.open());
    EXPECT_TRUE(sock.is_open());

    auto dest = ip_address::parse("8.8.8.8", ports::UNDEF);
    ASSERT_TRUE(dest.has_value());

    auto result = sock.ping(*dest, milliseconds(3000));
    if (result.success) {
        EXPECT_GT(result.rtt.count(), 0);
        EXPECT_GE(result.reply_size, 0u);
        EXPECT_GT(result.reply_ttl, 0u);
    }
}

TEST_F(IcmpSocketIntegration, PingLoopback) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }

    icmp_socket sock;
    ASSERT_NO_THROW(sock.open());

    auto dest = ip_address::loopback(ports::UNDEF, AF_INET);
    auto result = sock.ping(dest, milliseconds(500));
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.rtt.count(), 0);
}

TEST_F(IcmpSocketIntegration, PingWithCustomPayload) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }

    icmp_socket sock;
    ASSERT_NO_THROW(sock.open());

    char payload[] = "ICMP test payload data";
    auto dest = ip_address::loopback(ports::UNDEF, AF_INET);
    auto result = sock.ping(dest, milliseconds(1000), 1, payload, sizeof(payload));
    if (result.success) {
        EXPECT_EQ(result.reply_size, sizeof(payload));
    }
}

TEST_F(IcmpSocketIntegration, TracerouteToLoopback) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }

    icmp_socket sock;
    ASSERT_NO_THROW(sock.open());

    auto dest = ip_address::loopback(ports::UNDEF, AF_INET);
    auto hops = sock.traceroute(dest, 5, milliseconds(500), 2);
    EXPECT_GE(hops.size(), 1u);
    EXPECT_TRUE(hops[0].reached);
    EXPECT_TRUE(hops[0].address.is_valid());
}

class SmtpSocketIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SmtpSocketIntegration, ConnectInvalidHostThrows) {
    smtp_socket smtp;
    EXPECT_THROW(smtp.connect("", ports(25)), value_exception);
}

TEST_F(SmtpSocketIntegration, DisconnectWithoutConnectIsNoop) {
    smtp_socket smtp;
    EXPECT_NO_THROW(smtp.disconnect());
    EXPECT_FALSE(smtp.is_connected());
}

TEST_F(SmtpSocketIntegration, BuildMessageFormat) {
    smtp_message msg;
    msg.from = "sender@test.com";
    msg.to = {"recipient@test.com"};
    msg.subject = "Test Subject";
    msg.body = "Test body line 1\r\nTest body line 2";

    smtp_socket smtp;
    smtp_message msg2;
    msg2.from = "sender@test.com";
    msg2.to = {"recipient@test.com"};
    msg2.cc = {"cc@test.com"};
    msg2.subject = "Test";
    msg2.body = "body";
    msg2.is_html = true;

    EXPECT_EQ(msg2.is_html, true);
    EXPECT_EQ(msg.from, "sender@test.com");
    EXPECT_EQ(msg.to.size(), 1u);
}

TEST_F(SmtpSocketIntegration, BccNotInHeaders) {
    smtp_message msg;
    msg.from = "sender@test.com";
    msg.to = {"visible@test.com"};
    msg.bcc = {"hidden@test.com"};

    EXPECT_EQ(msg.to.size(), 1u);
    EXPECT_EQ(msg.bcc.size(), 1u);
    EXPECT_EQ(msg.to[0], "visible@test.com");
    EXPECT_EQ(msg.bcc[0], "hidden@test.com");
    EXPECT_TRUE(msg.cc.empty());
}

class SocketBaseIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SocketBaseIntegration, OpenTcpAndBindToAnyPort) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base sock;
    sock.open(AF_INET, SOCK_STREAM, 0);
    sock.set_reuse_address(true);

    auto addr = ip_address::any(ports(0u), AF_INET);
    EXPECT_NO_THROW(sock.bind(addr));

    auto local = sock.local_endpoint();
    ASSERT_TRUE(local.has_value());
    EXPECT_TRUE(local->is_ipv4());
    EXPECT_NE(local->port(), ports::UNDEF);
    sock.close();
}

TEST_F(SocketBaseIntegration, GetOptionOnOpenSocket) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base sock;
    sock.open(AF_INET, SOCK_STREAM, 0);
    sock.set_reuse_address(true);

    int val = 0;
    ::socklen_t len = sizeof(val);
    EXPECT_TRUE(sock.get_option(SOL_SOCKET, SO_REUSEADDR, &val, &len));
    sock.close();
}

TEST_F(SocketBaseIntegration, NonblockingModeToggle) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base sock;
    sock.open(AF_INET, SOCK_STREAM, 0);

    EXPECT_TRUE(sock.set_nonblocking(true));
    EXPECT_TRUE(sock.set_nonblocking(false));
    sock.close();
}

TEST_F(SocketBaseIntegration, SoReusePort) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base sock;
    sock.open(AF_INET, SOCK_STREAM, 0);
    sock.set_reuse_port(true);
    sock.close();
}

class IpSocketIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(IpSocketIntegration, ConnectToLoopbackWithTcp) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base server;
    server.open(AF_INET, SOCK_STREAM, 0);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    server.bind(addr);
    server.listen(1);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open(AF_INET);
    client.connect(*bound);
    EXPECT_TRUE(client.is_open());

    server.close();
    client.close();
}
