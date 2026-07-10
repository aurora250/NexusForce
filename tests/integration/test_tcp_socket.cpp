#include <NeForce/core/async/async_result.hpp>
#include <NeForce/core/async/async_stream.hpp>
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
        if (!sock.try_open(socket_base::family::INET4, socket_base::type::DGRAM)) {
            return false;
        }
        sock.set_reuse_address(true);
        auto addr = ip_address::any();
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
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_echo_server(acceptor); });

    tcp_socket client;
    client.open();
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
    auto addr = ip_address::loopback();
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
    client.open();
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
    client.open();

    auto addr = ip_address::loopback(ports(19999u));
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
    client.open();

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
    auto addr = ip_address::loopback();
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
    auto addr = ip_address::loopback();
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
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    acceptor.set_nonblocking(true);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    tcp_socket client;
    client.open();
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

    io_context ctx_;

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
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_echo_server(acceptor); });

    io_context ctx;
    tcp_client client(ctx);
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
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() { run_echo_server(acceptor); });

    tcp_client client(ctx_);
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
    tcp_client client(ctx_);

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
    tcp_client client(ctx_);
    EXPECT_FALSE(client.is_auto_reconnect());
    EXPECT_FALSE(client.is_reconnecting());
    EXPECT_EQ(client.reconnect_attempts(), 3);
}

TEST_F(TcpClientIntegration, ExceptionHandler) {
    tcp_client client(ctx_);
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
    tcp_client client(ctx_);
    EXPECT_FALSE(client.connect("", ports(25)));
    EXPECT_FALSE(client.connect("invalid..host", ports(80)));
}

TEST_F(TcpClientIntegration, SocketAccessThrowsWhenNotConnected) {
    tcp_client client(ctx_);
    EXPECT_THROW(ignore = client.socket(), value_exception);
}

class TcpServerIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    io_context ctx_;
};

TEST_F(TcpServerIntegration, StartStopLifecycle) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_server server(ports(0), ctx_, 2);
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
        auto addr = ip_address::loopback();
        tmp.open(addr);
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    atomic<int> handled{0};
    tcp_server server(test_port, ctx_, 2);

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

    tcp_client client(ctx_);
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
        auto addr = ip_address::loopback();
        tmp.open(addr);
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    atomic<int> handled{0};
    tcp_server server(test_port, ctx_, 4);

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
            tcp_client client(ctx_);
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
    tcp_server server(ports(0), ctx_, 2);
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
    EXPECT_NO_THROW(sock.open());
    EXPECT_TRUE(sock.is_open());
    sock.close();
    EXPECT_FALSE(sock.is_open());
}

class AsyncSocketIntegration : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    io_context ctx_;
};

TEST_F(AsyncSocketIntegration, ConnectWriteReadUseFuture) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        tmp.open(ip_address::loopback());
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    tcp_server server(test_port, ctx_, 2);
    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        char buf[256];
        ssize_t n = sock->receive({buf, sizeof(buf)});
        if (n > 0) {
            sock->send_all({buf, static_cast<size_t>(n)});
        }
        sock->close();
    });
    ASSERT_TRUE(server.start());

    tcp_socket sock;
    sock.open();

    auto endpoint = ip_address::parse("127.0.0.1", test_port);
    ASSERT_TRUE(endpoint.has_value());

    {
        auto fut = sock.async_connect(ctx_, *endpoint, use_future);
        EXPECT_NO_THROW(fut.get());
    }
    EXPECT_TRUE(sock.is_open());

    const string msg = "hello_async_test";
    {
        auto fut = sock.async_send(ctx_, {msg.data(), msg.size()}, use_future);
        EXPECT_NO_THROW({
            size_t written = fut.get();
            EXPECT_EQ(written, msg.size());
        });
    }

    char buf[256] = {};
    {
        auto fut = sock.async_receive(ctx_, {buf, sizeof(buf)}, use_future);
        EXPECT_NO_THROW({
            size_t read_n = fut.get();
            EXPECT_EQ(read_n, msg.size());
            EXPECT_EQ(string_view(buf, read_n), msg.view());
        });
    }

    sock.close();
    server.stop();
}

TEST_F(AsyncSocketIntegration, ConnectWriteReadLambda) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        tmp.open(ip_address::loopback());
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    tcp_server server(test_port, ctx_, 2);
    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        char buf[256];
        ssize_t n = sock->receive({buf, sizeof(buf)});
        if (n > 0) {
            sock->send_all({buf, static_cast<size_t>(n)});
        }
        sock->close();
    });
    ASSERT_TRUE(server.start());

    tcp_socket sock;
    sock.open();

    auto endpoint = ip_address::parse("127.0.0.1", test_port);
    ASSERT_TRUE(endpoint.has_value());

    bool connect_done = false;
    error_code connect_ec;
    sock.async_connect(ctx_, *endpoint, [&](error_code ec) {
        connect_ec = ec;
        connect_done = true;
    });
    while (!connect_done) {
        ctx_.poll_one();
    }
    EXPECT_FALSE(connect_ec) << connect_ec.message().data();

    bool write_done = false;
    error_code write_ec;
    size_t written = 0;
    const string msg = "lambda_test_msg";
    sock.async_send(ctx_, {msg.data(), msg.size()}, [&](error_code ec, size_t n) {
        write_ec = ec;
        written = n;
        write_done = true;
    });
    while (!write_done) {
        ctx_.poll_one();
    }
    EXPECT_FALSE(write_ec);
    EXPECT_EQ(written, msg.size());

    bool read_done = false;
    char buf[256] = {};
    error_code read_ec;
    size_t read_n = 0;
    sock.async_receive(ctx_, {buf, sizeof(buf)}, [&](error_code ec, size_t n) {
        read_ec = ec;
        read_n = n;
        read_done = true;
    });
    while (!read_done) {
        ctx_.poll_one();
    }
    EXPECT_FALSE(read_ec);
    EXPECT_EQ(read_n, msg.size());
    EXPECT_EQ(string_view(buf, read_n), msg.view());

    sock.close();
    server.stop();
}

TEST_F(AsyncSocketIntegration, CancelAsyncConnect) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    tcp_socket sock;
    sock.open();

    auto endpoint = ip_address::parse("192.0.2.1", ports(12345u));
    ASSERT_TRUE(endpoint.has_value());

    stop_source stop_src;
    cancellation_slot slot(stop_src.get_token());

    bool handler_called = false;

    sock.async_connect(ctx_, *endpoint, slot, [&](error_code) { handler_called = true; });

    ignore = stop_src.request_stop();

    auto deadline = steady_clock::now() + seconds(3);
    while (!handler_called && steady_clock::now() < deadline) {
        ctx_.poll_one();
    }

    EXPECT_TRUE(handler_called);

    sock.close();
}

TEST_F(AsyncSocketIntegration, CancelAsyncRead) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        tmp.open(ip_address::loopback());
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    tcp_server server(test_port, ctx_, 2);
    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        this_thread::sleep_for(milliseconds(1000));
        sock->close();
    });
    ASSERT_TRUE(server.start());

    tcp_socket sock;
    sock.open();

    auto endpoint = ip_address::parse("127.0.0.1", test_port);
    ASSERT_TRUE(endpoint.has_value());

    ASSERT_TRUE(sock.connect(*endpoint, milliseconds(3000)));

    stop_source stop_src;
    cancellation_slot slot(stop_src.get_token());

    bool handler_called = false;

    char buf[256];
    sock.async_receive(ctx_, {buf, sizeof(buf)}, slot, [&](error_code, size_t) { handler_called = true; });

    ignore = stop_src.request_stop();

    auto deadline = steady_clock::now() + seconds(3);
    while (!handler_called && steady_clock::now() < deadline) {
        ctx_.poll_one();
    }

    EXPECT_TRUE(handler_called);

    sock.close();
    server.stop();
}

TEST_F(AsyncSocketIntegration, AsyncStreamPolymorphicDispatch) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        tmp.open(ip_address::loopback());
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    tcp_server server(test_port, ctx_, 2);
    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        char buf[256];
        ssize_t n = sock->receive({buf, sizeof(buf)});
        if (n > 0) {
            sock->send_all({buf, static_cast<size_t>(n)});
        }
    });
    ASSERT_TRUE(server.start());

    tcp_socket sock;
    sock.open();
    auto endpoint = ip_address::parse("127.0.0.1", test_port);
    ASSERT_TRUE(endpoint.has_value());
    ASSERT_TRUE(sock.connect(*endpoint, milliseconds(3000)));

    async_stream& stream = sock;
    const char msg[] = "polymorphic_test";
    char buf[256] = {};

    auto fut = stream.async_write(ctx_, {msg, sizeof(msg)}, use_future);
    ctx_.run_one(500);
    size_t written = fut.get();
    EXPECT_EQ(written, sizeof(msg));

    auto fut2 = stream.async_read(ctx_, {buf, sizeof(buf)}, use_future);
    ctx_.run_one(500);
    size_t n = fut2.get();
    EXPECT_EQ(n, sizeof(msg));

    sock.close();
    server.stop();
}

TEST_F(AsyncSocketIntegration, ForwardingAliasSendReceive) {
    if (!network_available()) {
        GTEST_SKIP() << "No network connectivity";
    }

    auto test_port = ports(0u);
    {
        tcp_acceptor tmp;
        tmp.open(ip_address::loopback());
        auto bound = tmp.local_endpoint();
        if (bound.has_value()) {
            test_port = bound->port();
        }
        tmp.close();
    }

    tcp_server server(test_port, ctx_, 2);
    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        char buf[256];
        ssize_t n = sock->receive({buf, sizeof(buf)});
        if (n > 0) {
            sock->send_all({buf, static_cast<size_t>(n)});
        }
    });
    ASSERT_TRUE(server.start());

    tcp_socket sock;
    sock.open();
    auto endpoint = ip_address::parse("127.0.0.1", test_port);
    ASSERT_TRUE(endpoint.has_value());
    ASSERT_TRUE(sock.connect(*endpoint, milliseconds(3000)));

    const char msg[] = "alias_forwards_correctly";
    char buf[256] = {};

    auto fut = sock.async_send(ctx_, {msg, sizeof(msg)}, use_future);
    ctx_.run_one(500);
    size_t written = fut.get();
    EXPECT_EQ(written, sizeof(msg));

    auto fut2 = sock.async_receive(ctx_, {buf, sizeof(buf)}, use_future);
    ctx_.run_one(500);
    size_t n = fut2.get();
    EXPECT_EQ(n, sizeof(msg));

    sock.close();
    server.stop();
}
