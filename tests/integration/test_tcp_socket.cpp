#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
#include <NeForce/network/tcp/tcp_socket.hpp>
#include <NeForce/network/udp_socket.hpp>
#include <gtest/gtest.h>
using namespace neforce;

namespace {
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


class TcpEchoIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    void run_echo_server(tcp_acceptor& acceptor, int iterations = 1) {
        for (int i = 0; i < iterations; ++i) {
            auto client = acceptor.accept();
            char buf[65536];
            ssize_t received = client.receive({buf, sizeof(buf)});
            if (received > 0) {
                client.send_all({buf, static_cast<size_t>(received)});
            }
            client.close();
        }
    }
};

TEST_F(TcpEchoIntegration, BasicEcho) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_echo_server(acceptor); });

    tcp_socket client;
    client.open(AF_INET);
    ASSERT_TRUE(client.connect(*bound, milliseconds(3000)));

    const char* msg = "Hello TCP Echo!";
    ssize_t sent = client.send({msg, 14});
    EXPECT_EQ(sent, 14);

    char buf[128];
    ssize_t received = client.receive({buf, sizeof(buf)});
    EXPECT_EQ(received, 14);
    EXPECT_EQ(string_view(buf, 14), "Hello TCP Echo");

    client.close();
    server_thread.join();
    acceptor.close();
}

TEST_F(TcpEchoIntegration, SendAllReceiveAll) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() {
        auto client = acceptor.accept();
        auto data = client.receive_all(65536);
        if (!data.empty()) {
            client.send_all({data.data(), data.size()});
        }
        client.close();
    });

    tcp_socket client;
    client.open(AF_INET);
    ASSERT_TRUE(client.connect(*bound, milliseconds(3000)));

    vector<char> payload(65536);
    for (size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<char>(i % 256);
    }
    client.send_all({payload.data(), payload.size()});

    auto response = client.receive_all(payload.size());
    EXPECT_EQ(response.size(), payload.size());
    EXPECT_EQ(memcmp(response.data(), payload.data(), payload.size()), 0);

    client.close();
    server_thread.join();
    acceptor.close();
}

TEST_F(TcpEchoIntegration, ConnectToRefusedPort) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_socket client;
    client.open(AF_INET);

    auto addr = ip_address::loopback(ports(19999u), AF_INET);
    bool connected = false;
    try {
        connected = client.connect(addr, milliseconds(500));
    } catch (const socket_exception& /* ignore */) {
        // ignore
    }
    EXPECT_FALSE(connected);
    client.close();
}

TEST_F(TcpEchoIntegration, ConnectToUnreachableHost) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_socket client;
    client.open(AF_INET);

    auto addr = ip_address::parse("192.0.2.1", ports(12345u));
    ASSERT_TRUE(addr.has_value());

    bool connected = false;
    try {
        connected = client.connect(*addr, milliseconds(1000));
    } catch (const socket_exception& /* ignore */) {
        // ignore
    }
    EXPECT_FALSE(connected);
    client.close();
}

class TcpAcceptorIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TcpAcceptorIntegration, OpenAndBind) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    EXPECT_NO_THROW(acceptor.open(addr));

    auto local = acceptor.local_endpoint();
    ASSERT_TRUE(local.has_value());
    EXPECT_TRUE(local->is_ipv4());
    EXPECT_NE(local->port(), ports::UNDEF);

    acceptor.close();
}

TEST_F(TcpAcceptorIntegration, AcceptNonblockEmpty) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    acceptor.open(addr);
    acceptor.set_nonblocking(true);

    auto client = acceptor.accept_nonblock();
    EXPECT_FALSE(client.has_value());

    acceptor.close();
}

TEST_F(TcpAcceptorIntegration, AcceptNonblockWithPending) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    acceptor.open(addr);
    acceptor.set_nonblocking(true);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    tcp_socket client;
    client.open(AF_INET);
    ASSERT_TRUE(client.connect(*bound, milliseconds(3000)));

    this_thread::sleep_for(milliseconds(50));

    auto accepted = acceptor.accept_nonblock();
    EXPECT_TRUE(accepted.has_value());
    if (accepted.has_value()) {
        EXPECT_TRUE(accepted->is_open());
        accepted->close();
    }

    client.close();
    acceptor.close();
}

class TcpClientIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

    void run_echo_server(tcp_acceptor& acceptor) {
        auto client = acceptor.accept();
        char buf[4096];
        ssize_t received = client.receive({buf, sizeof(buf)});
        if (received > 0) {
            client.send_all({buf, static_cast<size_t>(received)});
        }
        client.close();
    }
};

TEST_F(TcpClientIntegration, ConnectDisconnectLifecycle) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_echo_server(acceptor); });

    tcp_client client;
    EXPECT_FALSE(client.is_connected());

    bool connected = client.connect("127.0.0.1", bound->port());
    EXPECT_TRUE(connected);
    EXPECT_TRUE(client.is_connected());

    client.disconnect();
    EXPECT_FALSE(client.is_connected());

    server_thread.join();
    acceptor.close();
}

TEST_F(TcpClientIntegration, SendReceiveRoundtrip) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback(ports(0u), AF_INET);
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_echo_server(acceptor); });

    tcp_client client;
    ASSERT_TRUE(client.connect("127.0.0.1", bound->port()));

    const char* msg = "tcp_client roundtrip";
    ssize_t sent = client.send(msg, 20);
    EXPECT_EQ(sent, 20);

    char buf[128];
    ssize_t received = client.receive(buf, sizeof(buf));
    EXPECT_EQ(received, 20);
    EXPECT_EQ(string_view(buf, 20), "tcp_client roundtrip");

    client.disconnect();
    server_thread.join();
    acceptor.close();
}

TEST_F(TcpClientIntegration, TimeoutSettings) {
    tcp_client client;

    EXPECT_EQ(client.connect_timeout(), milliseconds(5000));
    EXPECT_EQ(client.send_timeout(), milliseconds(5000));
    EXPECT_EQ(client.recv_timeout(), milliseconds(5000));

    client.set_connect_timeout(milliseconds(10000));
    client.set_send_timeout(milliseconds(8000));
    client.set_recv_timeout(milliseconds(6000));

    EXPECT_EQ(client.connect_timeout(), milliseconds(10000));
    EXPECT_EQ(client.send_timeout(), milliseconds(8000));
    EXPECT_EQ(client.recv_timeout(), milliseconds(6000));

    EXPECT_THROW(client.set_connect_timeout(milliseconds(0)), value_exception);
    EXPECT_THROW(client.set_send_timeout(milliseconds(-1)), value_exception);
    EXPECT_THROW(client.set_recv_timeout(milliseconds(0)), value_exception);
}

TEST_F(TcpClientIntegration, AutoReconnectDisabled) {
    tcp_client client;
    EXPECT_FALSE(client.is_auto_reconnect());
    EXPECT_FALSE(client.is_reconnecting());
    EXPECT_EQ(client.reconnect_attempts(), 3);
}

TEST_F(TcpClientIntegration, ExceptionHandler) {
    tcp_client client;
    atomic<bool> called{false};

    client.set_exception_handler([&](const exception&) { called.store(true); });

    bool result = client.connect("", ports(25));
    EXPECT_FALSE(result);
    EXPECT_TRUE(called.load());

    called.store(false);
    result = client.connect("127.0.0.1", ports(0u));
    EXPECT_FALSE(result);
    EXPECT_TRUE(called.load());
}

TEST_F(TcpClientIntegration, ConnectInvalidHostThrows) {
    tcp_client client;
    EXPECT_FALSE(client.connect("", ports(25)));
    EXPECT_FALSE(client.connect("invalid..host", ports(80)));
}

TEST_F(TcpClientIntegration, SocketAccessThrowsWhenNotConnected) {
    tcp_client client;
    EXPECT_THROW(ignore = client.socket(), value_exception);
}

class TcpServerIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TcpServerIntegration, StartStopLifecycle) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_server server(ports(0u), 2);
    EXPECT_FALSE(server.is_running());

    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        sock->send_all({"pong"});
        sock->close();
    });

    bool started = server.start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(server.is_running());

    server.stop();
    EXPECT_FALSE(server.is_running());
}

TEST_F(TcpServerIntegration, ClientHandlerCalled) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        auto addr = ip_address::loopback(ports(0u), AF_INET);
        tmp.open(addr);
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    atomic<int> handled{0};
    tcp_server server(test_port, 2);

    server.set_client_handler([&](unique_ptr<tcp_socket> sock) {
        handled.fetch_add(1);
        char buf[128];
        ssize_t received = sock->receive({buf, sizeof(buf)});
        if (received > 0) {
            sock->send_all({buf, static_cast<size_t>(received)});
        }
        sock->close();
    });

    ASSERT_TRUE(server.start());
    this_thread::sleep_for(milliseconds(50));

    tcp_client client;
    ASSERT_TRUE(client.connect("127.0.0.1", test_port));

    const char* msg = "server_test";
    client.send(msg, 11);
    char buf[128];
    ssize_t received = client.receive(buf, sizeof(buf));
    EXPECT_EQ(received, 11);
    EXPECT_EQ(string_view(buf, 11), "server_test");

    client.disconnect();

    this_thread::sleep_for(milliseconds(100));
    EXPECT_GE(handled.load(), 1);

    server.stop();
}

TEST_F(TcpServerIntegration, MultipleClients) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        auto addr = ip_address::loopback(ports(0u), AF_INET);
        tmp.open(addr);
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    atomic<int> handled{0};
    tcp_server server(test_port, 4);

    server.set_client_handler([&](unique_ptr<tcp_socket> sock) {
        handled.fetch_add(1);
        char buf[128];
        ssize_t received = sock->receive({buf, sizeof(buf)});
        if (received > 0) {
            sock->send_all({buf, static_cast<size_t>(received)});
        }
        sock->close();
    });

    ASSERT_TRUE(server.start());
    this_thread::sleep_for(milliseconds(50));

    const int num_clients = 5;
    vector<thread> threads;
    atomic<int> successes{0};

    for (int i = 0; i < num_clients; ++i) {
        threads.emplace_back([&, i]() {
            tcp_client client;
            if (client.connect("127.0.0.1", test_port)) {
                string msg = "client_" + to_string(i);
                client.send(msg.data(), msg.size());
                char buf[128];
                ssize_t received = client.receive(buf, sizeof(buf));
                if (received == static_cast<ssize_t>(msg.size()) && string_view(buf, msg.size()) == msg) {
                    successes.fetch_add(1);
                }
                client.disconnect();
            }
        });
    }

    for (auto& t: threads) {
        t.join();
    }

    this_thread::sleep_for(milliseconds(100));
    EXPECT_EQ(handled.load(), num_clients);
    EXPECT_EQ(successes.load(), num_clients);

    server.stop();
}

TEST_F(TcpServerIntegration, StartWithoutHandlerFails) {
    tcp_server server(ports(0u), 2);
    bool started = server.start();
    EXPECT_FALSE(started);
}

class TcpSocketStandalone : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TcpSocketStandalone, IsSslReturnsFalse) {
    tcp_socket sock;
    EXPECT_FALSE(sock.is_ssl());
}

TEST_F(TcpSocketStandalone, OpenAndClose) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_socket sock;
    EXPECT_NO_THROW(sock.open(AF_INET));
    EXPECT_TRUE(sock.is_open());
    sock.close();
    EXPECT_FALSE(sock.is_open());
}
