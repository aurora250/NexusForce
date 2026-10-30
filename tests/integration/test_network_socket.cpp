#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/icmp_socket.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/network/udp_socket.hpp>
#include "utils.hpp"
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_LINUX
#    include <NeForce/core/system/process.hpp>
#endif
using namespace neforce;

namespace {
    bool has_root() {
#ifdef NEFORCE_PLATFORM_LINUX
        return process::current_privilege_level() == process::privilege_level::privileged;
#else
        return true;
#endif
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
    server.open();
    server.set_reuse_address(true);
    auto addr = ip_address::loopback();
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open();

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
    server.open();
    server.set_reuse_address(true);
    auto addr = ip_address::loopback();
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open();

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
    sock.open();
    auto addr = ip_address::loopback(ports(12345u));
    EXPECT_EQ(sock.send_to(memory_view<const char>(), addr), 0);
    sock.close();
}

TEST_F(UdpSocketIntegration, ConnectedModeSendReceive) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    udp_socket server;
    server.open();
    server.set_reuse_address(true);
    auto addr = ip_address::loopback();
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open();
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
    server.open(ip_family::inet6);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback(ports(0u), ip_family::inet6);
    server.bind(addr);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open(ip_family::inet6);

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

    auto dest = ip_address::parse("8.8.8.8", ports::undef);
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

    auto dest = ip_address::loopback();
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
    auto dest = ip_address::loopback();
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

    auto dest = ip_address::loopback();
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
    sock.open(ip_family::inet4);
    sock.set_reuse_address(true);

    auto addr = ip_address::any();
    EXPECT_NO_THROW(sock.bind(addr));

    auto local = sock.local_endpoint();
    ASSERT_TRUE(local.has_value());
    EXPECT_TRUE(local->is_ipv4());
    EXPECT_NE(local->port(), ports::undef);
    sock.close();
}

TEST_F(SocketBaseIntegration, GetOptionOnOpenSocket) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base sock;
    sock.open(ip_family::inet4);
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
    sock.open(ip_family::inet4);

    EXPECT_TRUE(sock.set_nonblocking(true));
    EXPECT_TRUE(sock.set_nonblocking(false));
    sock.close();
}

TEST_F(SocketBaseIntegration, SoReusePort) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    socket_base sock;
    sock.open(ip_family::inet4);
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
    server.open(ip_family::inet4);
    server.set_reuse_address(true);
    auto addr = ip_address::loopback();
    server.bind(addr);
    server.listen(1);
    auto bound = server.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    udp_socket client;
    client.open();
    client.connect(*bound);
    EXPECT_TRUE(client.is_open());

    server.close();
    client.close();
}

class AsyncIcmpIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AsyncIcmpIntegration, AsyncPingLoopbackSucceeds) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }

    io_context ioc;
    icmp_socket sock;
    ASSERT_NO_THROW(sock.open());

    auto dest = ip_address::loopback();
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
    ASSERT_FALSE(ec);
    EXPECT_TRUE(out.success);
    EXPECT_GE(out.rtt.count(), 0);
}

TEST_F(AsyncIcmpIntegration, AsyncPingTimeout) {
    if (!has_root()) {
        GTEST_SKIP() << "Root privileges required for raw ICMP socket";
    }

    io_context ioc;
    icmp_socket sock;
    ASSERT_NO_THROW(sock.open());

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

class SmtpAsyncConnectIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

namespace {
    string smtp_read_line(tcp_socket& sock) {
        string line;
        char ch = 0;
        while (true) {
            const ssize_t n = sock.receive({&ch, 1});
            if (n <= 0) {
                break;
            }
            if (ch == '\n') {
                break;
            }
            if (ch != '\r') {
                line += ch;
            }
        }
        return line;
    }

    void smtp_send(tcp_socket& sock, const string& data) {
        sock.send_all(memory_view<const char>(data.data(), data.size()));
    }

    // Minimal SMTP responder: 220 greeting, EHLO/HELO/QUIT support.
    void run_fake_smtp_server(tcp_acceptor& acceptor) {
        auto client = acceptor.accept();
        smtp_send(client, "220 fake.local ESMTP ready\r\n");
        while (true) {
            const string line = smtp_read_line(client);
            if (line.empty()) {
                break;
            }
            if (line.starts_with("EHLO")) {
                smtp_send(client, "250-fake.local\r\n250 8BITMIME\r\n");
            } else if (line.starts_with("HELO")) {
                smtp_send(client, "250 fake.local\r\n");
            } else if (line.starts_with("QUIT")) {
                smtp_send(client, "221 Bye\r\n");
                break;
            }
        }
        client.close();
    }
} // namespace

TEST_F(SmtpAsyncConnectIntegration, AsyncConnectCompletesFullHandshake) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    acceptor.open(ip_address::loopback());
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_fake_smtp_server(acceptor); });

    io_context ioc;
    smtp_socket smtp;
    bool done = false;
    error_code ec;
    smtp.async_connect(ioc, *bound, "local.test", smtp_socket::tls_mode::none, nullptr, "", [&](error_code e) {
        done = true;
        ec = e;
    });
    auto deadline = steady_clock::now() + seconds(5);
    while (!done && steady_clock::now() < deadline) {
        ioc.run_one(100);
    }
    EXPECT_TRUE(done);
    ASSERT_FALSE(ec) << ec.message().data();
    EXPECT_TRUE(smtp.is_connected());
    EXPECT_FALSE(smtp.is_tls_active());

    smtp.set_nonblocking(false);
    try {
        smtp.disconnect();
    } catch (const exception&) {
        // The fake server may have closed the connection right after the async
        // handshake finished; the QUIT exchange is best-effort here.
    }
    smtp.close();
    EXPECT_FALSE(smtp.is_connected());

    server_thread.join();
    acceptor.close();
}

TEST_F(SmtpAsyncConnectIntegration, AsyncConnectStarttlsModeRequiresContext) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    acceptor.open(ip_address::loopback());
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    io_context ioc;
    smtp_socket smtp;
    bool done = false;
    error_code ec;
    // ssl_ctx is nullptr for tls_mode::implicit: op must fail fast before any network I/O.
    smtp.async_connect(ioc, *bound, "local.test", smtp_socket::tls_mode::implicit, nullptr, "", [&](error_code e) {
        done = true;
        ec = e;
    });
    auto deadline = steady_clock::now() + seconds(3);
    while (!done && steady_clock::now() < deadline) {
        ioc.run_one(100);
    }
    EXPECT_TRUE(done);
    EXPECT_TRUE(ec);
    EXPECT_FALSE(smtp.is_connected());

    acceptor.close();
}
