#include <NeForce/core/async/async_stream.hpp>
#include <NeForce/core/async/latch.hpp>
#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/network/ssl/ssl_acceptor.hpp>
#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <NeForce/network/tcp/tcp_server.hpp>
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

    bool openssl_available() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return process::execute_shell("openssl version > NUL 2>&1").exit_code == 0;
#else
        return process::execute_shell("openssl version > /dev/null 2>&1").exit_code == 0;
#endif
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    const char* SERVER_CERT = "D:/OpenSSL/neforce_test_server.crt";
    const char* SERVER_KEY = "D:/OpenSSL/neforce_test_server.key";
#else
    const char* SERVER_CERT = "/tmp/neforce_test_server.crt";
    const char* SERVER_KEY = "/tmp/neforce_test_server.key";
#endif

    bool generate_self_signed_cert() {
        if (!openssl_available()) {
            return false;
        }

#ifdef NEFORCE_PLATFORM_WINDOWS
        const char* redirect = "2>NUL";
#else
        const char* redirect = "2>/dev/null";
#endif
        string cmd = "openssl req -x509 -newkey rsa:2048 -keyout ";
        cmd += SERVER_KEY;
        cmd += " -out ";
        cmd += SERVER_CERT;
        cmd += " -days 1 -nodes -subj ";
        cmd += "/CN=localhost";
        cmd += " -addext subjectAltName=DNS:localhost ";
        cmd += redirect;
        return process::execute_shell(cmd.data()).exit_code == 0;
    }

    void cleanup_certs() {
        filesystem::remove(path{SERVER_CERT});
        filesystem::remove(path{SERVER_KEY});
    }

    bool ssl_prereqs() {
        if (!network_available()) {
            return false;
        }
        return generate_self_signed_cert();
    }
} // namespace


class SslEchoIntegration : public ::testing::Test {
protected:
    void SetUp() override {
        if (!ssl_prereqs()) {
            GTEST_SKIP() << "SSL prerequisites not met";
        }
    }

    void TearDown() override { cleanup_certs(); }
    io_context ctx_;

    void run_ssl_echo_server(tcp_acceptor& acceptor, const ssl_context& ctx, latch* ready = nullptr) {
        try {
            auto tcp_client = acceptor.accept();
            ssl_socket ssl_client(move(tcp_client));
            ssl_client.init_server_ssl(ctx);
            if (ready != nullptr) {
                ready->count_down();
            }

            char buf[65536];
            ssize_t received = ssl_client.receive({buf, sizeof(buf)});
            if (received > 0) {
                ssl_client.send_all({buf, static_cast<size_t>(received)});
            }
            ssl_client.close();
        } catch (const ssl_exception& e) {
            if (ready != nullptr) {
                ready->count_down();
            }
            eprintln(e.what());
        }
    }
};

TEST_F(SslEchoIntegration, SslEchoRoundtrip) {
    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    thread server_thread([&]() { run_ssl_echo_server(acceptor, server_ctx); });

    ssl_context client_ctx(ssl_method::TLS_CLIENT);
    client_ctx.set_verify_mode(SSL_VERIFY_NONE);

    ssl_socket client;
    client.open();
    ASSERT_TRUE(client.connect(*bound, milliseconds(5000)));
    client.init_client_ssl(client_ctx, "localhost");

    const char* msg = "SSL Echo Test!";
    ssize_t sent = client.send({msg, 14});
    EXPECT_EQ(sent, 14);

    char buf[128];
    ssize_t received = client.receive({buf, sizeof(buf)});
    EXPECT_EQ(received, 14);
    EXPECT_EQ(string_view(buf, 14), "SSL Echo Test!");

    client.close();
    server_thread.join();
    acceptor.close();
}

TEST_F(SslEchoIntegration, SslMultipleMessages) {
    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    thread server_thread([&]() {
        try {
            auto tcp_client = acceptor.accept();
            ssl_socket ssl_client(move(tcp_client));
            ssl_client.init_server_ssl(server_ctx);

            for (int i = 0; i < 5; ++i) {
                char buf[256];
                ssize_t received = ssl_client.receive({buf, sizeof(buf)});
                if (received > 0) {
                    ssl_client.send_all({buf, static_cast<size_t>(received)});
                }
            }
            ssl_client.close();
        } catch (const ssl_exception&) {
        } catch (...) {
        }
    });

    ssl_context client_ctx(ssl_method::TLS_CLIENT);
    client_ctx.set_verify_mode(SSL_VERIFY_NONE);

    ssl_socket client;
    client.open();
    ASSERT_TRUE(client.connect(*bound, milliseconds(5000)));
    client.init_client_ssl(client_ctx, "localhost");

    for (int i = 0; i < 5; ++i) {
        string msg = "ssl_msg_" + to_string(i);
        ssize_t sent = client.send({msg.data(), msg.size()});
        EXPECT_EQ(sent, static_cast<ssize_t>(msg.size()));

        char buf[256];
        ssize_t received = client.receive({buf, sizeof(buf)});
        EXPECT_EQ(received, static_cast<ssize_t>(msg.size()));
        EXPECT_EQ(string_view(buf, msg.size()), msg);
    }

    client.close();
    server_thread.join();
    acceptor.close();
}

TEST_F(SslEchoIntegration, SslPeerCertificateInfo) {
    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    latch ssl_ready(1);

    thread server_thread([&]() { run_ssl_echo_server(acceptor, server_ctx, &ssl_ready); });

    ssl_context client_ctx(ssl_method::TLS_CLIENT);
    client_ctx.set_verify_mode(SSL_VERIFY_NONE);

    ssl_socket client;
    client.open();
    ASSERT_TRUE(client.connect(*bound, milliseconds(5000)));
    client.init_client_ssl(client_ctx, "localhost");
    ssl_ready.wait();

    string cert_info = client.peer_certificate_info();
    EXPECT_FALSE(cert_info.empty());

    bool has_subject = cert_info.find("Subject:") != string::npos || cert_info.find("subject=") != string::npos ||
                       cert_info.find("CN") != string::npos;
    EXPECT_TRUE(has_subject) << "Certificate info: " << cert_info.data();

    bool has_issuer = cert_info.find("Issuer:") != string::npos || cert_info.find("issuer=") != string::npos ||
                      cert_info.find("CN") != string::npos;
    EXPECT_TRUE(has_issuer) << "Certificate info: " << cert_info.data();

    client.close();
    server_thread.join();
    acceptor.close();
}

TEST_F(SslEchoIntegration, SslCipherAndProtocol) {
    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    latch ssl_ready(1);

    thread server_thread([&]() { run_ssl_echo_server(acceptor, server_ctx, &ssl_ready); });

    ssl_context client_ctx(ssl_method::TLS_CLIENT);
    client_ctx.set_verify_mode(SSL_VERIFY_NONE);

    ssl_socket client;
    client.open();
    ASSERT_TRUE(client.connect(*bound, milliseconds(5000)));
    client.init_client_ssl(client_ctx, "localhost");
    ssl_ready.wait();

    string cipher = client.ssl().get_cipher_name();
    EXPECT_FALSE(cipher.empty()) << "Cipher name should not be empty";

    string version = client.ssl().get_version();
    EXPECT_FALSE(version.empty()) << "Protocol version should not be empty";

    client.close();
    server_thread.join();
    acceptor.close();
}

class SslAcceptorIntegration : public ::testing::Test {
protected:
    void SetUp() override {
        if (!ssl_prereqs()) {
            GTEST_SKIP() << "SSL prerequisites not met";
        }
    }

    void TearDown() override { cleanup_certs(); }
    io_context ctx_;
};

TEST_F(SslAcceptorIntegration, AcceptSslCompletesHandshake) {
    ssl_context ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    ssl_acceptor acceptor;
    acceptor.set_ssl_context(ctx.clone());

    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    latch server_done(1);

    thread client_thread([&]() {
        ssl_context client_ctx(ssl_method::TLS_CLIENT);
        client_ctx.set_verify_mode(SSL_VERIFY_NONE);

        ssl_socket client;
        client.open();
        if (client.connect(*bound, milliseconds(5000))) {
            client.init_client_ssl(client_ctx, "localhost");
            client.send_all({"ssl_acceptor_test"});
            server_done.wait();
            client.close();
        }
    });

    auto ssl_client = acceptor.accept_ssl();
    EXPECT_TRUE(ssl_client.is_open());
    EXPECT_TRUE(ssl_client.is_ssl());

    char buf[128];
    ssize_t received = ssl_client.receive({buf, sizeof(buf)});
    EXPECT_EQ(received, 18);
    EXPECT_EQ(string_view(buf, 17), "ssl_acceptor_test");

    server_done.count_down();
    ssl_client.close();
    client_thread.join();
    acceptor.close();
}

TEST_F(SslAcceptorIntegration, AcceptSslNonblock) {
    ssl_context ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    ssl_acceptor acceptor;
    acceptor.set_ssl_context(ctx.clone());

    auto addr = ip_address::loopback();
    acceptor.open(addr);
    acceptor.set_nonblocking(true);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    latch server_done(1);

    thread client_thread([&]() {
        ssl_context client_ctx(ssl_method::TLS_CLIENT);
        client_ctx.set_verify_mode(SSL_VERIFY_NONE);

        ssl_socket client;
        client.open();
        if (client.connect(*bound, milliseconds(5000))) {
            client.init_client_ssl(client_ctx, "localhost");
            client.send_all({"nonblock_ssl"});
            server_done.wait();
            client.close();
        }
    });

    optional<ssl_socket> ssl_client;
    for (int i = 0; i < 50; ++i) {
        ssl_client = acceptor.accept_ssl_nonblock();
        if (ssl_client.has_value()) {
            break;
        }
        this_thread::sleep_for(milliseconds(100));
    }

    ASSERT_TRUE(ssl_client.has_value());
    EXPECT_TRUE(ssl_client->is_ssl());

    char buf[128];
    ssize_t received = ssl_client->receive({buf, sizeof(buf)});
    EXPECT_EQ(received, 13);
    EXPECT_EQ(string_view(buf, 12), "nonblock_ssl");

    server_done.count_down();
    ssl_client->close();
    client_thread.join();
    acceptor.close();
}

TEST_F(SslAcceptorIntegration, AcceptSslNonblockEmpty) {
    ssl_context ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    ssl_acceptor acceptor;
    acceptor.set_ssl_context(ctx.clone());

    auto addr = ip_address::loopback();
    acceptor.open(addr);
    acceptor.set_nonblocking(true);

    auto result = acceptor.accept_ssl_nonblock();
    EXPECT_FALSE(result.has_value());

    acceptor.close();
}

class SslClientIntegration : public ::testing::Test {
protected:
    void SetUp() override {
        if (!ssl_prereqs()) {
            GTEST_SKIP() << "SSL prerequisites not met";
        }
    }

    void TearDown() override { cleanup_certs(); }
    io_context ctx_;
};

TEST_F(SslClientIntegration, ConnectWithSslContext) {
    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    latch server_done(1);

    thread server_thread([&]() {
        try {
            auto tcp_client = acceptor.accept();
            ssl_socket ssl_client(move(tcp_client));
            ssl_client.init_server_ssl(server_ctx);

            char buf[256];
            ssize_t received = ssl_client.receive({buf, sizeof(buf)});
            if (received > 0) {
                ssl_client.send_all({buf, static_cast<size_t>(received)});
            }
            ssl_client.close();
        } catch (const ssl_exception& e) {
            eprintln(e.what());
        }
        server_done.count_down();
    });

    ssl_client client(ctx_);
    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    client.set_verify_peer(false);

    ASSERT_TRUE(client.connect("127.0.0.1", bound->port()));
    EXPECT_TRUE(client.is_connected());

    const char* msg = "ssl_client connect test";
    client.send_all(msg, 23);

    char buf[256];
    ssize_t received = client.receive(buf, sizeof(buf));
    EXPECT_EQ(received, 23);
    EXPECT_EQ(string_view(buf, 23), "ssl_client connect test");

    client.disconnect();
    server_done.wait();
    server_thread.join();
    acceptor.close();
}

TEST_F(SslClientIntegration, PeerVerificationDisabled) {
    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    thread server_thread([&]() {
        try {
            auto tcp_client = acceptor.accept();
            ssl_socket ssl_client(move(tcp_client));
            ssl_client.init_server_ssl(server_ctx);
            ssl_client.send_all({"verified"});
            ssl_client.close();
        } catch (const ssl_exception& e) {
            eprintln(e.what());
        }
    });

    ssl_client client(ctx_);
    EXPECT_TRUE(client.get_verify_peer());

    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    client.set_verify_peer(false);
    EXPECT_FALSE(client.get_verify_peer());

    ASSERT_TRUE(client.connect("127.0.0.1", bound->port()));

    char buf[128];
    ssize_t received = client.receive(buf, sizeof(buf));
    EXPECT_EQ(received, 9);
    EXPECT_EQ(string_view(buf, 8), "verified");

    client.disconnect();
    server_thread.join();
    acceptor.close();
}

TEST_F(SslClientIntegration, SniHostnameSetting) {
    ssl_client client(ctx_);
    EXPECT_TRUE(client.sni_hostname().empty());

    client.set_sni_hostname("example.com");
    EXPECT_EQ(client.sni_hostname(), "example.com");

    client.set_sni_hostname("test.local");
    EXPECT_EQ(client.sni_hostname(), "test.local");
}

TEST_F(SslClientIntegration, CertificateInfoAfterConnect) {
    ssl_context server_ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(server_ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    tcp_acceptor acceptor;
    auto addr = ip_address::loopback();
    acceptor.open(addr);
    auto bound = acceptor.local_endpoint();
    ASSERT_TRUE(bound.has_value());

    latch server_done(1);

    thread server_thread([&]() {
        auto tcp_client = acceptor.accept();
        ssl_socket ssl_client(move(tcp_client));
        ssl_client.init_server_ssl(server_ctx);
        ssl_client.send_all({"cert_info_test"});
        ssl_client.close();
        server_done.count_down();
    });

    ssl_client client(ctx_);
    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    client.set_verify_peer(false);

    ASSERT_TRUE(client.connect("127.0.0.1", bound->port()));

    string cipher = client.cipher_name();
    EXPECT_FALSE(cipher.empty());

    string protocol = client.protocol_version();
    EXPECT_FALSE(protocol.empty());

    string cert = client.peer_certificate_info();
    EXPECT_FALSE(cert.empty());

    bool has_cn = cert.find("CN") != string::npos || cert.find("localhost") != string::npos;
    EXPECT_TRUE(has_cn) << "Cert info: " << cert.data();

    server_done.wait();
    client.disconnect();
    server_thread.join();
    acceptor.close();
}

TEST_F(SslClientIntegration, HasSslContextCheck) {
    ssl_client client(ctx_);
    EXPECT_FALSE(client.has_ssl_context());
    EXPECT_FALSE(client.is_ssl_initialized());

    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    EXPECT_TRUE(client.has_ssl_context());
}

class SslServerIntegration : public ::testing::Test {
protected:
    void SetUp() override {
        if (!ssl_prereqs()) {
            GTEST_SKIP() << "SSL prerequisites not met";
        }
    }

    void TearDown() override { cleanup_certs(); }
    io_context ctx_;
};

TEST_F(SslServerIntegration, StartWithCertificate) {
    ssl_server server(ports(0), ctx_, 2);
    ASSERT_TRUE(server.load_certificate(SERVER_CERT, SERVER_KEY));

    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        sock->send_all({"ssl_server_ok"});
        sock->close();
    });

    bool started = server.start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(server.is_running());

    ssl_client client(ctx_);
    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    client.set_verify_peer(false);

    ASSERT_TRUE(client.connect("127.0.0.1", server.port()));
    EXPECT_TRUE(client.is_connected());

    this_thread::sleep_for(milliseconds(100));

    char buf[128];
    ssize_t received = client.receive(buf, sizeof(buf));
    EXPECT_EQ(received, 14);

    client.disconnect();
    server.stop();
}

TEST_F(SslServerIntegration, StartWithoutCertFails) {
    ssl_server server(ports(0), ctx_, 2);
    server.set_client_handler([](unique_ptr<tcp_socket> sock) { sock->close(); });

    bool started = server.start();
    EXPECT_FALSE(started);
    EXPECT_FALSE(server.is_running());
}

TEST_F(SslServerIntegration, StopAndRestart) {
    ssl_server server(ports(0), ctx_, 2);
    ASSERT_TRUE(server.load_certificate(SERVER_CERT, SERVER_KEY));

    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        sock->send_all({"round1"});
        sock->close();
    });

    ASSERT_TRUE(server.start());
    EXPECT_TRUE(server.is_running());
    server.stop();
    EXPECT_FALSE(server.is_running());

    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        sock->send_all({"round2"});
        sock->close();
    });

    bool started = server.start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(server.is_running());

    ssl_client client(ctx_);
    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    client.set_verify_peer(false);

    ASSERT_TRUE(client.connect("127.0.0.1", server.port()));

    this_thread::sleep_for(milliseconds(100));

    char buf[128];
    ssize_t received = client.receive(buf, sizeof(buf));
    EXPECT_EQ(received, 7);
    EXPECT_EQ(string_view(buf, 6), "round2");

    client.disconnect();
    server.stop();
}

TEST_F(SslServerIntegration, SetSslContextOverride) {
    ssl_server server(ports(0), ctx_, 2);

    ssl_context ctx(ssl_method::TLS_SERVER);
    ASSERT_TRUE(ctx.load_certificate(SERVER_CERT, SERVER_KEY));

    server.set_ssl_context(move(ctx));
    EXPECT_TRUE(server.get_ssl_context().is_valid());

    server.set_client_handler([](unique_ptr<tcp_socket> sock) { sock->close(); });

    bool started = server.start();
    EXPECT_TRUE(started);
    server.stop();
}

TEST_F(SslServerIntegration, CancelAsyncReadWrite) {
    ssl_server server(ports(0), ctx_, 2);
    ASSERT_TRUE(server.load_certificate(SERVER_CERT, SERVER_KEY));

    server.set_client_handler([](unique_ptr<tcp_socket> sock) {
        this_thread::sleep_for(milliseconds(2000));
        sock->close();
    });
    ASSERT_TRUE(server.start());

    ssl_client client(ctx_);
    client.set_ssl_context(ssl_context(ssl_method::TLS_CLIENT));
    client.set_verify_peer(false);
    ASSERT_TRUE(client.connect("127.0.0.1", server.port()));

    stop_source stop_src;
    cancellation_slot slot(stop_src.get_token());

    bool handler_called = false;

    auto& ssl_sock = dynamic_cast<ssl_socket&>(client.socket());
    char buf[256];
    ssl_sock.ssl().async_read(ctx_, {buf, sizeof(buf)}, slot, [&](error_code, size_t) { handler_called = true; });

    ignore = stop_src.request_stop();

    auto deadline = steady_clock::now() + seconds(3);
    while (!handler_called && steady_clock::now() < deadline) {
        ctx_.poll_one();
    }

    EXPECT_TRUE(handler_called);

    client.disconnect();
    server.stop();
}
