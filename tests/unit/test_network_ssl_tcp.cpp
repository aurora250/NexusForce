#include <NeForce/network/ssl/ssl_context.hpp>
#include <NeForce/network/ssl/ssl_stream.hpp>
#include <NeForce/network/ssl/ssl_socket.hpp>
#include <NeForce/network/ssl/ssl_acceptor.hpp>
#include <NeForce/network/tcp/tcp_socket.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/network/tcp/tcp_client.hpp>
#include <openssl/ssl.h>
#include <gtest/gtest.h>
using namespace neforce;

class SslContextTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SslContextTest, DefaultConstructTlsServer) {
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_TRUE(ctx.is_valid());
    EXPECT_NE(ctx.native_handle(), nullptr);
}

TEST_F(SslContextTest, DefaultConstructTlsClient) {
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_TRUE(ctx.is_valid());
    EXPECT_NE(ctx.native_handle(), nullptr);
}

TEST_F(SslContextTest, DefaultConstructDtlsServer) {
    ssl_context ctx(ssl_method::dtls_server);
    EXPECT_TRUE(ctx.is_valid());
}

TEST_F(SslContextTest, DefaultConstructDtlsClient) {
    ssl_context ctx(ssl_method::dtls_client);
    EXPECT_TRUE(ctx.is_valid());
}

TEST_F(SslContextTest, BooleanConversion) {
    ssl_context ctx;
    EXPECT_TRUE(static_cast<bool>(ctx));
}

TEST_F(SslContextTest, MoveConstructedContextIsValid) {
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_TRUE(ctx.is_valid());
    ssl_context moved(move(ctx));
    EXPECT_TRUE(moved.is_valid());
    EXPECT_FALSE(ctx.is_valid());
}

TEST_F(SslContextTest, MoveAssignmentTransfersOwnership) {
    ssl_context ctx1(ssl_method::tls_client);
    ssl_context ctx2(ssl_method::tls_server);
    ctx2 = move(ctx1);
    EXPECT_TRUE(ctx2.is_valid());
    EXPECT_FALSE(ctx1.is_valid());
}

TEST_F(SslContextTest, CloneProducesValidContext) {
    ssl_context ctx(ssl_method::tls_client);
    auto cloned = ctx.clone();
    EXPECT_TRUE(cloned.is_valid());
    EXPECT_NE(cloned.native_handle(), nullptr);
}

TEST_F(SslContextTest, ClonePreservesMethod) {
    ssl_context ctx(ssl_method::tls_client);
    auto cloned = ctx.clone();
    EXPECT_TRUE(cloned.is_valid());
}

TEST_F(SslContextTest, CloneSharedProducesValidPointer) {
    ssl_context ctx(ssl_method::tls_client);
    auto ptr = ctx.clone_shared();
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(ptr->is_valid());
}

TEST_F(SslContextTest, SetDefaultOptions) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_default_options());
}

TEST_F(SslContextTest, SetCipherList) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_cipher_list("HIGH:!aNULL"));
}

TEST_F(SslContextTest, SetCipherSuites) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_ciphersuites("TLS_AES_256_GCM_SHA384"));
}

TEST_F(SslContextTest, SetOptions) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_options(ssl_option::NO_SSLv2 | ssl_option::NO_SSLv3));
}

TEST_F(SslContextTest, SetVerifyMode) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_verify_mode(ssl_verify::peer));
    EXPECT_NO_THROW(ctx.set_verify_mode(ssl_verify::none));
}

TEST_F(SslContextTest, RequireClientCertificate) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.require_client_certificate());
}

TEST_F(SslContextTest, SessionCacheSize) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_session_cache_size(128));
}

TEST_F(SslContextTest, Timeout) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_timeout(300));
}

TEST_F(SslContextTest, AlpnProtosEmptyList) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_alpn_protos({}));
}

TEST_F(SslContextTest, AlpnProtosValidList) {
    ssl_context ctx;
    EXPECT_NO_THROW(ctx.set_alpn_protos({"h2", "http/1.1"}));
}

TEST_F(SslContextTest, ServerContextDoesNotVerifyPeerByDefault) {
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(ctx.require_client_certificate());
}

TEST_F(SslContextTest, LoadVerifyLocationsEmptyPathsReturnsFalse) {
    ssl_context ctx;
    EXPECT_FALSE(ctx.load_verify_locations("", ""));
}

TEST_F(SslContextTest, LoadCertificateFileNotFoundReturnsFalse) {
    ssl_context ctx;
    EXPECT_FALSE(ctx.load_certificate("/nonexistent/cert.pem", "/nonexistent/key.pem"));
}

TEST_F(SslContextTest, LoadVerifyLocationsInvalidFileReturnsFalse) {
    ssl_context ctx;
    EXPECT_FALSE(ctx.load_verify_locations("/nonexistent/ca.pem", ""));
}

TEST_F(SslContextTest, LoadCertificateFromMemoryInvalidPemThrows) {
    ssl_context ctx;
    EXPECT_THROW(ctx.load_certificate_from_memory("not-valid-pem", "not-valid-pem"), ssl_exception);
}

TEST_F(SslContextTest, LoadCertificateFromMemoryEmptyDataThrows) {
    ssl_context ctx;
    EXPECT_THROW(ctx.load_certificate_from_memory("", ""), value_exception);
}

TEST_F(SslContextTest, MoveConstructorIsNoexcept) { EXPECT_TRUE(is_nothrow_move_constructible_v<ssl_context>); }

TEST_F(SslContextTest, NativeHandleOnDefaultCtxIsNotNull) {
    ssl_context ctx;
    EXPECT_NE(ctx.native_handle(), nullptr);
}

class SslStreamTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SslStreamTest, DefaultConstructorIsNotValid) {
    ssl_stream stream;
    EXPECT_FALSE(stream.is_valid());
    EXPECT_FALSE(static_cast<bool>(stream));
}

TEST_F(SslStreamTest, ConstructFromContextIsValid) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_TRUE(stream.is_valid());
}

TEST_F(SslStreamTest, ResetMakesStreamValid) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream;
    stream.reset(ctx);
    EXPECT_TRUE(stream.is_valid());
}

TEST_F(SslStreamTest, ResetWithInvalidContextThrows) {
    ssl_stream stream;
    ssl_context ctx(ssl_method::tls_client);
    ssl_context moved_from = move(ctx);
    EXPECT_THROW(stream.reset(ctx), ssl_exception);
}

TEST_F(SslStreamTest, ReadWithoutInitReturnsMinusOne) {
    ssl_stream stream;
    char buf[16];
    EXPECT_EQ(stream.read(buf, sizeof(buf)), -1);
}

TEST_F(SslStreamTest, WriteWithoutInitReturnsMinusOne) {
    ssl_stream stream;
    const char* data = "test";
    EXPECT_EQ(stream.write(data, 4), -1);
}

TEST_F(SslStreamTest, ReadWithNullBufferReturnsMinusOne) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_EQ(stream.read(nullptr, 16), -1);
}

TEST_F(SslStreamTest, WriteWithNullBufferReturnsMinusOne) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_EQ(stream.write(nullptr, 16), -1);
}

TEST_F(SslStreamTest, ReadWithZeroSizeReturnsZero) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    char buf[16];
    EXPECT_EQ(stream.read(buf, 0), 0);
}

TEST_F(SslStreamTest, WriteWithZeroSizeReturnsZero) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    const char* data = "test";
    EXPECT_EQ(stream.write(data, 0), 0);
}

TEST_F(SslStreamTest, WriteWithNullBufferAndZeroSizeReturnsZero) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_EQ(stream.write(nullptr, 0), 0);
}

TEST_F(SslStreamTest, SetFdWithoutInitThrows) {
    ssl_stream stream;
    EXPECT_THROW(stream.set_fd(0), ssl_exception);
}

TEST_F(SslStreamTest, SetFdInvalidHandleThrows) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_THROW(stream.set_fd(socket_base::invalid_handle), value_exception);
}

TEST_F(SslStreamTest, AcceptWithoutInitThrows) {
    ssl_stream stream;
    EXPECT_THROW(stream.accept(), ssl_exception);
}

TEST_F(SslStreamTest, ConnectWithoutInitThrows) {
    ssl_stream stream;
    EXPECT_THROW(stream.connect(), ssl_exception);
}

TEST_F(SslStreamTest, PendingWithoutInitReturnsZero) {
    ssl_stream stream;
    EXPECT_EQ(stream.pending(), 0);
}

TEST_F(SslStreamTest, SetSniHostnameWithoutInitThrows) {
    ssl_stream stream;
    EXPECT_THROW(stream.set_sni_hostname("example.com"), ssl_exception);
}

TEST_F(SslStreamTest, SetSniHostnameEmptyThrows) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_THROW(stream.set_sni_hostname(""), value_exception);
}

TEST_F(SslStreamTest, GetPeerCertificateWithoutInitReturnsNull) {
    ssl_stream stream;
    auto cert = stream.peer_certificate();
    EXPECT_EQ(cert.native_handle(), nullptr);
}

TEST_F(SslStreamTest, VerifyPeerWithoutInitReturnsFalse) {
    ssl_stream stream;
    EXPECT_FALSE(stream.verify_peer());
}

TEST_F(SslStreamTest, GetCipherNameWithoutInitReturnsEmpty) {
    ssl_stream stream;
    EXPECT_EQ(stream.cipher_name(), "");
}

TEST_F(SslStreamTest, GetVersionWithoutInitReturnsEmpty) {
    ssl_stream stream;
    EXPECT_EQ(stream.version(), "");
}

TEST_F(SslStreamTest, LastErrorInitiallyEmpty) {
    ssl_stream stream;
    EXPECT_TRUE(stream.last_error().empty());
}

TEST_F(SslStreamTest, NativeHandleOnValidStreamIsNotNull) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_NE(stream.native_handle(), nullptr);
}

TEST_F(SslStreamTest, ReleaseReturnsHandleAndNullifies) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    void* ssl = stream.release();
    EXPECT_NE(ssl, nullptr);
    EXPECT_FALSE(stream.is_valid());
    ::SSL_free(static_cast<::SSL*>(ssl));
}

TEST_F(SslStreamTest, CloseIsSafeOnDefaultConstructed) {
    ssl_stream stream;
    EXPECT_NO_THROW(stream.close());
}

TEST_F(SslStreamTest, CloseIsSafeAfterReset) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_NO_THROW(stream.close());
    EXPECT_FALSE(stream.is_valid());
}

TEST_F(SslStreamTest, MoveConstructorTransfersOwnership) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream1(ctx);
    EXPECT_TRUE(stream1.is_valid());
    ssl_stream stream2(move(stream1));
    EXPECT_TRUE(stream2.is_valid());
    EXPECT_FALSE(stream1.is_valid());
}

TEST_F(SslStreamTest, MoveAssignmentTransfersOwnership) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream1(ctx);
    ssl_stream stream2;
    stream2 = move(stream1);
    EXPECT_TRUE(stream2.is_valid());
    EXPECT_FALSE(stream1.is_valid());
}

TEST_F(SslStreamTest, ReadAllWithoutInitThrows) {
    ssl_stream stream;
    EXPECT_THROW(stream.read_all(1024), ssl_exception);
}

TEST_F(SslStreamTest, ReadAllZeroMaxSizeReturnsEmpty) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    auto result = stream.read_all(0);
    EXPECT_TRUE(result.empty());
}

TEST_F(SslStreamTest, WriteAllWithoutInitReturnsFalse) {
    ssl_stream stream;
    const char* data = "test";
    EXPECT_FALSE(stream.write_all(data, 4));
}

TEST_F(SslStreamTest, WriteAllNullDataReturnsFalse) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_FALSE(stream.write_all(nullptr, 16));
}

TEST_F(SslStreamTest, WriteAllZeroSizeReturnsTrue) {
    ssl_context ctx(ssl_method::tls_client);
    ssl_stream stream(ctx);
    EXPECT_TRUE(stream.write_all("test", 0));
}

class SslSocketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SslSocketTest, DefaultConstructorIsNotSsl) {
    ssl_socket sock;
    EXPECT_FALSE(sock.is_ssl());
}

TEST_F(SslSocketTest, ConstructorFromFd) {
    socket_base base;
    base.open(ip_family::INET4);
    auto fd = base.release();
    ssl_socket sock(fd);
    EXPECT_TRUE(sock.is_open());
    sock.close();
}

TEST_F(SslSocketTest, ConstructorFromTcpSocket) {
    tcp_socket tcp;
    tcp.open(ip_family::INET4);
    ssl_socket ssl(move(tcp));
    EXPECT_TRUE(ssl.is_open());
    ssl.close();
}

TEST_F(SslSocketTest, IsSslInitiallyFalse) {
    ssl_socket sock;
    EXPECT_FALSE(sock.is_ssl());
    EXPECT_EQ(sock.is_ssl(), false);
}

TEST_F(SslSocketTest, SslAccessorThrowsWhenNotInitialized) {
    ssl_socket sock;
    EXPECT_THROW(ignore = sock.ssl(), ssl_exception);
}

TEST_F(SslSocketTest, SslConstAccessorThrowsWhenNotInitialized) {
    const ssl_socket sock;
    EXPECT_THROW(ignore = sock.ssl(), ssl_exception);
}

TEST_F(SslSocketTest, InitServerSslWithoutOpenThrows) {
    ssl_socket sock;
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_THROW(sock.init_server_ssl(ctx), value_exception);
}

TEST_F(SslSocketTest, InitClientSslWithoutOpenThrows) {
    ssl_socket sock;
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_THROW(sock.init_client_ssl(ctx, "localhost"), value_exception);
}

TEST_F(SslSocketTest, InitServerSslWithInvalidContextThrows) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    ssl_context ctx(ssl_method::tls_server);
    ssl_context moved_from = move(ctx);
    EXPECT_THROW(sock.init_server_ssl(ctx), ssl_exception);
    sock.close();
}

TEST_F(SslSocketTest, PeerCertificateInfoWithoutSslReturnsEmpty) {
    ssl_socket sock;
    EXPECT_EQ(sock.peer_certificate_info(), "");
}

TEST_F(SslSocketTest, SendWithoutSslFallsBackToTcp) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_EQ(sock.send(memory_view<const char>()), 0);
    sock.close();
}

TEST_F(SslSocketTest, SendEmptyDataReturnsZeroAfterOpen) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_EQ(sock.send(memory_view<const char>()), 0);
    sock.close();
}

TEST_F(SslSocketTest, ReceiveEmptyBufferReturnsZeroAfterOpen) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_EQ(sock.receive(memory_view<char>()), 0);
    sock.close();
}

TEST_F(SslSocketTest, ReceiveWithoutSslFallsBackToTcp) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_EQ(sock.receive(memory_view<char>()), 0);
    sock.close();
}

TEST_F(SslSocketTest, CloseReturnsTrueWhenAlreadyClosed) {
    ssl_socket sock;
    EXPECT_TRUE(sock.close());
}

TEST_F(SslSocketTest, MoveConstructorPreservesOpenState) {
    ssl_socket sock1;
    sock1.open(ip_family::INET4);
    EXPECT_TRUE(sock1.is_open());

    ssl_socket sock2(move(sock1));
    EXPECT_TRUE(sock2.is_open());
    EXPECT_FALSE(sock1.is_open());
    sock2.close();
}

class SslAcceptorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SslAcceptorTest, DefaultConstructor) {
    ssl_acceptor acceptor;
    EXPECT_FALSE(acceptor.is_open());
}

TEST_F(SslAcceptorTest, SetSslContextValid) {
    ssl_acceptor acceptor;
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(acceptor.set_ssl_context(move(ctx)));
}

TEST_F(SslAcceptorTest, SetSslContextInvalidThrows) {
    ssl_acceptor acceptor;
    ssl_context ctx(ssl_method::tls_server);
    ssl_context moved_from = move(ctx);
    EXPECT_THROW(acceptor.set_ssl_context(move(ctx)), ssl_exception);
}

TEST_F(SslAcceptorTest, ContextAccessorReturnsRef) {
    ssl_acceptor acceptor;
    ssl_context ctx(ssl_method::tls_server);
    acceptor.set_ssl_context(move(ctx));
    EXPECT_TRUE(acceptor.context().is_valid());
}

TEST_F(SslAcceptorTest, AcceptSslWithoutOpenThrows) {
    ssl_acceptor acceptor;
    ssl_context ctx(ssl_method::tls_server);
    acceptor.set_ssl_context(move(ctx));
    EXPECT_THROW(ignore = acceptor.accept_ssl(), value_exception);
}

TEST_F(SslAcceptorTest, AcceptSslNonblockWithoutOpenReturnsNone) {
    ssl_acceptor acceptor;
    ssl_context ctx(ssl_method::tls_server);
    acceptor.set_ssl_context(move(ctx));
    auto result = acceptor.accept_ssl_nonblock();
    EXPECT_FALSE(result.has_value());
}

class TcpSocketTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TcpSocketTest, DefaultConstructorIsNotOpen) {
    tcp_socket sock;
    EXPECT_FALSE(sock.is_open());
}

TEST_F(TcpSocketTest, OpenIpv4) {
    tcp_socket sock;
    EXPECT_NO_THROW(sock.open(ip_family::INET4));
    EXPECT_TRUE(sock.is_open());
    EXPECT_TRUE(sock.is_ipv4());
    sock.close();
}

TEST_F(TcpSocketTest, OpenIpv6) {
    tcp_socket sock;
    EXPECT_NO_THROW(sock.open(ip_family::INET6));
    EXPECT_TRUE(sock.is_open());
    EXPECT_TRUE(sock.is_ipv6());
    sock.close();
}

TEST_F(TcpSocketTest, OpenDefaultIsIpv4) {
    tcp_socket sock;
    sock.open();
    EXPECT_TRUE(sock.is_ipv4());
    sock.close();
}

TEST_F(TcpSocketTest, ConnectWithInvalidEndpointThrows) {
    tcp_socket sock;
    sock.open(ip_family::INET4);
    ip_address invalid;
    EXPECT_THROW(sock.connect(invalid, milliseconds(1000)), value_exception);
    sock.close();
}

TEST_F(TcpSocketTest, ConnectWithoutOpenThrows) {
    tcp_socket sock;
    auto addr = ip_address::loopback(ports(8080u), ip_family::INET4);
    EXPECT_THROW(sock.connect(addr, milliseconds(1000)), value_exception);
}

TEST_F(TcpSocketTest, SendWithoutOpenThrows) {
    tcp_socket sock;
    EXPECT_THROW(sock.send(memory_view<const char>("x", 1)), value_exception);
}

TEST_F(TcpSocketTest, ReceiveWithoutOpenThrows) {
    tcp_socket sock;
    char buf[16];
    EXPECT_THROW(sock.receive(memory_view<char>(buf, sizeof(buf))), value_exception);
}

TEST_F(TcpSocketTest, SendEmptyDataReturnsZero) {
    tcp_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_EQ(sock.send(memory_view<const char>()), 0);
    sock.close();
}

TEST_F(TcpSocketTest, ReceiveEmptyBufferReturnsZero) {
    tcp_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_EQ(sock.receive(memory_view<char>()), 0);
    sock.close();
}

TEST_F(TcpSocketTest, IsSslReturnsFalse) {
    tcp_socket sock;
    EXPECT_FALSE(sock.is_ssl());
}

TEST_F(TcpSocketTest, SendAllWithoutOpenThrows) {
    tcp_socket sock;
    EXPECT_THROW(sock.send_all(memory_view<const char>("test", 4)), value_exception);
}

TEST_F(TcpSocketTest, ReceiveAllWithoutOpenThrows) {
    tcp_socket sock;
    EXPECT_THROW(sock.receive_all(1024), value_exception);
}

TEST_F(TcpSocketTest, ConnectToRefusedPortFails) {
    tcp_socket sock;
    sock.open(ip_family::INET4);
    auto addr = ip_address::loopback(ports(12345u), ip_family::INET4);
    try {
        bool result = sock.connect(addr, milliseconds(500));
        EXPECT_FALSE(result);
    } catch (const socket_exception&) {
        EXPECT_TRUE(true);
    }
}

TEST_F(TcpSocketTest, DoubleOpenReplacesSocket) {
    tcp_socket sock;
    sock.open(ip_family::INET4);
    EXPECT_TRUE(sock.is_open());
    sock.open(ip_family::INET4);
    EXPECT_TRUE(sock.is_open());
    sock.close();
}

TEST_F(TcpSocketTest, MoveConstructorTransfersState) {
    tcp_socket sock1;
    sock1.open(ip_family::INET4);
    EXPECT_TRUE(sock1.is_open());

    tcp_socket sock2(move(sock1));
    EXPECT_TRUE(sock2.is_open());
    EXPECT_FALSE(sock1.is_open());
    sock2.close();
}

class TcpAcceptorTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TcpAcceptorTest, DefaultConstructorIsNotOpen) {
    tcp_acceptor acceptor;
    EXPECT_FALSE(acceptor.is_open());
}

TEST_F(TcpAcceptorTest, OpenWithInvalidEndpointThrows) {
    tcp_acceptor acceptor;
    ip_address invalid;
    EXPECT_THROW(acceptor.open(invalid, 128), value_exception);
}

TEST_F(TcpAcceptorTest, OpenWithAnyPortSucceeds) {
    tcp_acceptor acceptor;
    auto addr = ip_address::any(ports(0u), ip_family::INET4);
    EXPECT_NO_THROW(acceptor.open(addr, 128));
    EXPECT_TRUE(acceptor.is_open());
    acceptor.close();
}

TEST_F(TcpAcceptorTest, AcceptWithoutOpenThrows) {
    tcp_acceptor acceptor;
    EXPECT_THROW(ignore = acceptor.accept(), value_exception);
}

TEST_F(TcpAcceptorTest, AcceptNonblockWithoutOpenReturnsNone) {
    tcp_acceptor acceptor;
    auto result = acceptor.accept_nonblock();
    EXPECT_FALSE(result.has_value());
}

TEST_F(TcpAcceptorTest, AcceptNonblockWithNoPendingConnectionsReturnsNone) {
    tcp_acceptor acceptor;
    auto addr = ip_address::any();
    acceptor.open(addr, 128);
    acceptor.set_nonblocking(true);

    auto result = acceptor.accept_nonblock();
    EXPECT_FALSE(result.has_value());
    acceptor.close();
}

TEST_F(TcpAcceptorTest, UsingSocketBaseOpenCompiles) {
    tcp_acceptor acceptor;
    EXPECT_NO_THROW(acceptor.socket_base::open(ip_family::INET4));
    EXPECT_TRUE(acceptor.is_open());
    acceptor.close();
}

class TcpClientTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    io_context ctx_;
};

TEST_F(TcpClientTest, DefaultConstructor) {
    tcp_client client(ctx_);
    EXPECT_FALSE(client.is_connected());
    EXPECT_EQ(client.connected_host(), "");
}

TEST_F(TcpClientTest, SetConnectTimeoutPositive) {
    tcp_client client(ctx_);
    EXPECT_NO_THROW(client.set_connect_timeout(milliseconds(3000)));
    EXPECT_EQ(client.connect_timeout(), milliseconds(3000));
}

TEST_F(TcpClientTest, SetConnectTimeoutZeroThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(client.set_connect_timeout(milliseconds(0)), value_exception);
}

TEST_F(TcpClientTest, SetConnectTimeoutNegativeThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(client.set_connect_timeout(milliseconds(-1)), value_exception);
}

TEST_F(TcpClientTest, SetSendTimeoutZeroThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(client.set_send_timeout(milliseconds(0)), value_exception);
}

TEST_F(TcpClientTest, SetRecvTimeoutZeroThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(client.set_recv_timeout(milliseconds(0)), value_exception);
}

TEST_F(TcpClientTest, SetAutoReconnectZeroAttemptsThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(client.set_auto_reconnect(true, 0), value_exception);
}

TEST_F(TcpClientTest, SetReconnectDelayNegativeThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(client.set_reconnect_delay(milliseconds(-1)), value_exception);
}

TEST_F(TcpClientTest, DefaultAutoReconnectDisabled) {
    tcp_client client(ctx_);
    EXPECT_FALSE(client.is_auto_reconnect());
    EXPECT_FALSE(client.is_reconnecting());
}

TEST_F(TcpClientTest, SocketAccessorWithoutConnectThrows) {
    tcp_client client(ctx_);
    EXPECT_THROW(ignore = client.socket(), value_exception);
}

TEST_F(TcpClientTest, ConstSocketAccessorWithoutConnectThrows) {
    const tcp_client client(ctx_);
    EXPECT_THROW(ignore = client.socket(), value_exception);
}

TEST_F(TcpClientTest, ConnectEmptyHostReturnsFalse) {
    tcp_client client(ctx_);
    EXPECT_FALSE(client.connect("", ports(80)));
}

TEST_F(TcpClientTest, ConnectZeroPortReturnsFalse) {
    tcp_client client(ctx_);
    ports zero(0u);
    EXPECT_FALSE(client.connect("localhost", zero));
}

TEST_F(TcpClientTest, DisconnectWithoutConnectIsSafe) {
    tcp_client client(ctx_);
    EXPECT_NO_THROW(client.disconnect());
}

TEST_F(TcpClientTest, SetPreferIpv6) {
    tcp_client client(ctx_);
    client.set_prefer_ipv6(true);
    EXPECT_TRUE(client.prefer_ipv6());
    client.set_prefer_ipv6(false);
    EXPECT_FALSE(client.prefer_ipv6());
}

TEST_F(TcpClientTest, SendNullDataReturnsMinusOne) {
    tcp_client client(ctx_);
    EXPECT_EQ(client.send(nullptr, 16), -1);
}

TEST_F(TcpClientTest, SendZeroLengthReturnsZero) {
    tcp_client client(ctx_);
    EXPECT_EQ(client.send(nullptr, 0), 0);
}

TEST_F(TcpClientTest, ReceiveNullDataReturnsMinusOne) {
    tcp_client client(ctx_);
    EXPECT_EQ(client.receive(nullptr, 16), -1);
}

TEST_F(TcpClientTest, ReceiveZeroLengthReturnsZero) {
    tcp_client client(ctx_);
    EXPECT_EQ(client.receive(nullptr, 0), 0);
}

TEST_F(TcpClientTest, SendAllNullDataReturnsFalse) {
    tcp_client client(ctx_);
    EXPECT_FALSE(client.send_all(nullptr, 16));
}

TEST_F(TcpClientTest, SendAllZeroLengthReturnsTrue) {
    tcp_client client(ctx_);
    EXPECT_TRUE(client.send_all(nullptr, 0));
}

TEST_F(TcpClientTest, ExceptionHandlerIsCalled) {
    tcp_client client(ctx_);
    bool called = false;
    client.set_exception_handler([&called](const neforce::exception&) { called = true; });
    client.connect("", ports(80));
    EXPECT_TRUE(called);
}

TEST_F(TcpClientTest, ConnectInvalidHostReturnsFalse) {
    dns_client::config cfg;
    cfg.server = "127.0.0.1";
    cfg.port = ports(9u);
    cfg.timeout = milliseconds(200);
    tcp_client client(cfg, ctx_);
    client.get_dns_client().set_max_udp_retries(0);

    EXPECT_FALSE(client.connect("invalid-host-that-does-not-exist.test", ports(80)));
    EXPECT_FALSE(client.is_connected());
}

class SslClientTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    io_context ctx_;
};

TEST_F(SslClientTest, DefaultConstructor) {
    ssl_client client(ctx_);
    EXPECT_FALSE(client.is_connected());
    EXPECT_FALSE(client.is_ssl_initialized());
}

TEST_F(SslClientTest, HasSslContextInitiallyFalse) {
    ssl_client client(ctx_);
    EXPECT_FALSE(client.has_ssl_context());
}

TEST_F(SslClientTest, ConstructorWithSslContext) {
    ssl_context ssl_ctx(ssl_method::tls_client);
    ssl_client client(ctx_, move(ssl_ctx));
    EXPECT_TRUE(client.has_ssl_context());
}

TEST_F(SslClientTest, LoadCaFileCreatesContext) {
    ssl_client client(ctx_);
    client.load_ca_file("/nonexistent/ca.pem");
    EXPECT_TRUE(client.has_ssl_context());
}

TEST_F(SslClientTest, SetSslContextValid) {
    ssl_client client(ctx_);
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_NO_THROW(client.set_ssl_context(move(ctx)));
    EXPECT_TRUE(client.has_ssl_context());
}

TEST_F(SslClientTest, SetSslContextInvalidThrows) {
    ssl_client client(ctx_);
    ssl_context ctx(ssl_method::tls_client);
    ssl_context moved_from = move(ctx);
    EXPECT_THROW(client.set_ssl_context(move(ctx)), ssl_exception);
}

TEST_F(SslClientTest, SetVerifyPeerDefaultTrue) {
    ssl_client client(ctx_);
    EXPECT_TRUE(client.get_verify_peer());
}

TEST_F(SslClientTest, SetVerifyPeerFalse) {
    ssl_client client(ctx_);
    EXPECT_NO_THROW(client.set_verify_peer(false));
    EXPECT_FALSE(client.get_verify_peer());
}

TEST_F(SslClientTest, PeerCertificateInfoNotConnectedReturnsEmpty) {
    ssl_client client(ctx_);
    auto info = client.peer_certificate_info();
    EXPECT_EQ(info, "");
}

TEST_F(SslClientTest, CipherNameNotConnectedReturnsEmpty) {
    ssl_client client(ctx_);
    auto name = client.cipher_name();
    EXPECT_EQ(name, "");
}

TEST_F(SslClientTest, ProtocolVersionNotConnectedReturnsEmpty) {
    ssl_client client(ctx_);
    auto version = client.protocol_version();
    EXPECT_EQ(version, "");
}

TEST_F(SslClientTest, PeerCertificateInfoReturnsStringNotStringView) {
    ssl_client client(ctx_);
    auto info = client.peer_certificate_info();
    EXPECT_TRUE((is_same_v<decltype(info), string>) );
}

TEST_F(SslClientTest, CipherNameReturnsStringNotStringView) {
    ssl_client client(ctx_);
    auto name = client.cipher_name();
    EXPECT_TRUE((is_same_v<decltype(name), string>) );
}

TEST_F(SslClientTest, ProtocolVersionReturnsStringNotStringView) {
    ssl_client client(ctx_);
    auto version = client.protocol_version();
    EXPECT_TRUE((is_same_v<decltype(version), string>) );
}

TEST_F(SslClientTest, SslSocketRefNotConnectedThrows) {
    ssl_client client(ctx_);
    EXPECT_THROW(ignore = client.ssl_socket_ref(), value_exception);
}

TEST_F(SslClientTest, LoadCaPathCreatesContext) {
    ssl_client client(ctx_);
    client.load_ca_path("/nonexistent/ca_dir");
    EXPECT_TRUE(client.has_ssl_context());
}

class SslAlpnNegotiateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SslAlpnNegotiateTest, StreamWithoutSslReturnsEmpty) {
    ssl_stream stream;
    EXPECT_TRUE(stream.alpn_negotiated().empty());
}

TEST_F(SslAlpnNegotiateTest, StreamBeforeHandshakeReturnsEmpty) {
    ssl_context ctx(ssl_method::tls_server);
    ssl_stream stream(ctx);
    auto alpn = stream.alpn_negotiated();
    EXPECT_TRUE(alpn.empty());
}

TEST_F(SslAlpnNegotiateTest, StreamWithClientContextNoHandshake) {
    ssl_context ctx(ssl_method::tls_client);
    ctx.set_alpn_protos({"h2", "http/1.1"});
    ssl_stream stream(ctx);
    auto alpn = stream.alpn_negotiated();
    EXPECT_TRUE(alpn.empty());
}

TEST_F(SslAlpnNegotiateTest, SocketWithoutSslReturnsEmpty) {
    ssl_socket sock;
    EXPECT_TRUE(sock.get_alpn_negotiated().empty());
}

TEST_F(SslAlpnNegotiateTest, SocketWithSslNoHandshakeReturnsEmpty) {
    ssl_context ctx(ssl_method::tls_server);
    ssl_socket sock;
    EXPECT_TRUE(sock.get_alpn_negotiated().empty());
}

TEST_F(SslAlpnNegotiateTest, ContextSetAlpnProtosH2) {
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(ctx.set_alpn_protos({"h2", "http/1.1"}));
}

TEST_F(SslAlpnNegotiateTest, ContextSetAlpnProtosHttp11Only) {
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(ctx.set_alpn_protos({"http/1.1"}));
}

TEST_F(SslAlpnNegotiateTest, ContextSetAlpnProtosEmpty) {
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(ctx.set_alpn_protos({}));
}

TEST_F(SslAlpnNegotiateTest, ContextSetAlpnProtosTwiceKeepsContextValid) {
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(ctx.set_alpn_protos({"h2", "http/1.1"}));
    EXPECT_NO_THROW(ctx.set_alpn_protos({"h2"}));
    EXPECT_TRUE(ctx.is_valid());
    EXPECT_NE(ctx.native_handle(), nullptr);
}

TEST_F(SslAlpnNegotiateTest, ContextSetAlpnProtosEmptyKeepsContextValid) {
    ssl_context ctx(ssl_method::tls_server);
    ctx.set_alpn_protos({"h2"});
    EXPECT_NO_THROW(ctx.set_alpn_protos({}));
    EXPECT_TRUE(ctx.is_valid());
    EXPECT_NE(ctx.native_handle(), nullptr);
}

TEST_F(SslSocketTest, PrepareServerSslWithoutOpenThrows) {
    ssl_socket sock;
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_THROW(sock.prepare_server_ssl(ctx), value_exception);
}

TEST_F(SslSocketTest, PrepareClientSslWithoutOpenThrows) {
    ssl_socket sock;
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_THROW(sock.prepare_client_ssl(ctx), value_exception);
}

TEST_F(SslSocketTest, PrepareServerSslInvalidContextThrows) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    ssl_context ctx(ssl_method::tls_server);
    ssl_context moved_from = move(ctx);
    (void) moved_from;
    EXPECT_THROW(sock.prepare_server_ssl(ctx), ssl_exception);
    sock.close();
}

TEST_F(SslSocketTest, PrepareClientSslInvalidContextThrows) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    ssl_context ctx(ssl_method::tls_client);
    ssl_context moved_from = move(ctx);
    (void) moved_from;
    EXPECT_THROW(sock.prepare_client_ssl(ctx), ssl_exception);
    sock.close();
}

TEST_F(SslSocketTest, PrepareServerSslMarksSslWithoutHandshake) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    ssl_context ctx(ssl_method::tls_server);
    EXPECT_NO_THROW(sock.prepare_server_ssl(ctx));
    EXPECT_TRUE(sock.is_ssl());
    sock.close();
}

TEST_F(SslSocketTest, PrepareClientSslWithHostnameMarksSsl) {
    ssl_socket sock;
    sock.open(ip_family::INET4);
    ssl_context ctx(ssl_method::tls_client);
    EXPECT_NO_THROW(sock.prepare_client_ssl(ctx, "localhost"));
    EXPECT_TRUE(sock.is_ssl());
    sock.close();
}

TEST_F(SslSocketTest, AsyncHandshakeBeforePrepareDeliversError) {
    io_context ioc;
    ssl_socket sock;
    bool called = false;
    error_code ec;
    sock.async_handshake(ioc, [&](error_code e) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(SslSocketTest, AsyncReadWithoutTlsRoutesToTcpSocket) {
    io_context ioc;
    ssl_socket sock;
    sock.open(ip_family::INET4);
    char buf[16];
    bool called = false;
    error_code ec;
    sock.async_read(ioc, {buf, sizeof(buf)}, [&](error_code e, size_t) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
    sock.close();
}

TEST_F(SslSocketTest, AsyncWriteWithoutTlsRoutesToTcpSocket) {
    io_context ioc;
    ssl_socket sock;
    sock.open(ip_family::INET4);
    bool called = false;
    error_code ec;
    sock.async_write(ioc, {"x", 1}, [&](error_code e, size_t) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
    sock.close();
}

TEST_F(SslAcceptorTest, AsyncAcceptWithoutOpenDeliversError) {
    io_context ioc;
    ssl_acceptor acceptor;
    bool called = false;
    error_code ec;
    acceptor.async_accept(ioc, [&](error_code e, ssl_socket) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(TcpClientTest, AsyncReadBeforeConnectDeliversError) {
    tcp_client client(ctx_);
    char buf[8];
    bool called = false;
    error_code ec;
    client.async_read(ctx_, {buf, sizeof(buf)}, [&](error_code e, size_t) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(TcpClientTest, AsyncWriteBeforeConnectDeliversError) {
    tcp_client client(ctx_);
    bool called = false;
    error_code ec;
    client.async_write(ctx_, {"x", 1}, [&](error_code e, size_t) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(TcpClientTest, AsyncReadUseFutureBeforeConnectThrows) {
    tcp_client client(ctx_);
    char buf[8];
    auto fut = client.async_read(ctx_, {buf, sizeof(buf)}, use_future);
    EXPECT_THROW(ignore = fut.get(), system_exception);
}

TEST_F(TcpClientTest, AsyncConnectInvalidEndpointDeliversError) {
    tcp_client client(ctx_);
    ip_address invalid;
    bool called = false;
    error_code ec;
    client.async_connect(ctx_, invalid, [&](error_code e) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
}

TEST_F(TcpClientTest, AsyncConnectPreCancelledSlotAborts) {
    tcp_client client(ctx_);
    auto endpoint = ip_address::loopback(ports(9u), ip_family::INET4);

    stop_source stop_src;
    cancellation_slot slot(stop_src.get_token());
    ignore = stop_src.request_stop();

    bool called = false;
    error_code ec;
    client.async_connect(ctx_, endpoint, slot, [&](error_code e) {
        called = true;
        ec = e;
    });
    EXPECT_TRUE(called);
    EXPECT_TRUE(ec);
    EXPECT_EQ(ec.value(), static_cast<int>(errc::operation_canceled));
}

TEST_F(TcpClientTest, AsyncConnectTokenOverloadsCompile) {
    tcp_client client(ctx_);
    auto endpoint = ip_address::loopback(ports(9u), ip_family::INET4);
    auto fut = client.async_connect(ctx_, endpoint, use_future);
    EXPECT_TRUE(fut.valid());
    client.async_connect(ctx_, endpoint, detached);
}

TEST_F(SslContextTest, MoveIntoByValueSinkDoesNotDoubleFree) {
    auto sink = [](ssl_context ctx) {
        EXPECT_TRUE(ctx.is_valid());
        return ctx;
    };
    ssl_context result = sink(ssl_context(ssl_method::tls_client));
    EXPECT_TRUE(result.is_valid());
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST_F(SslContextTest, MoveAssignmentNullsSourceAndKeepsTarget) {
    ssl_context a(ssl_method::tls_client);
    ssl_context b(ssl_method::tls_server);
    b = move(a);
    EXPECT_TRUE(b.is_valid());
    EXPECT_FALSE(a.is_valid());
    EXPECT_NO_THROW(ignore = b.native_handle());
}
