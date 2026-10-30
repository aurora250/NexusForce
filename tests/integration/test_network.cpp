#include <NeForce/core/async/io_context.hpp>
#include <NeForce/core/time/clocks.hpp>
#include <NeForce/network/http/http_client.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/util/arp.hpp>
#include <NeForce/network/icmp_socket.hpp>
#include <NeForce/network/smtp_socket.hpp>
#include <NeForce/network/tcp/tcp_acceptor.hpp>
#include <NeForce/core/file/file.hpp>
#include <NeForce/core/system/console.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::http;

class HttpClientLocalTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    io_context server_ctx_;
    io_context client_ctx_;
};

TEST_F(HttpClientLocalTest, LocalHttpServerSyncAndAsyncRequests) {
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

    http_server server(test_port, server_ctx_, 1);
    server.router().get("/ping", [](http_request&, http_response& res) {
        res.status = http_status::ok;
        res.status_message = "OK";
        res.body = "pong";
    });
    if (!server.start()) {
        GTEST_SKIP() << "http_server failed to bind local port";
    }

    http_client client(client_ctx_);

    auto response = client.get("http://127.0.0.1:" + to_string(test_port.value()) + "/ping");
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.body, "pong");

    client.close();
    this_thread::sleep_for(milliseconds(200));

    http_client_request req;
    req.method = http_method::GET();
    req.host = "127.0.0.1";
    req.scheme = "http";
    req.port = test_port;
    req.path = "/ping";
    auto fut = client.request_async(req);
    auto deadline = steady_clock::now() + seconds(5);
    while (fut.wait_for(milliseconds(0)) != future_status::ready && steady_clock::now() < deadline) {
        client_ctx_.run_one(100);
    }
    ASSERT_EQ(fut.wait_for(milliseconds(0)), future_status::ready);
    auto async_resp = fut.get();
    EXPECT_TRUE(async_resp.is_success());
    EXPECT_EQ(async_resp.body, "pong");

    client.close();
    server.stop();
}

#ifdef NEFORCE_STANDARD_20
TEST_F(HttpClientLocalTest, LocalHttpServerAwaitableRequest) {
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

    http_server server(test_port, server_ctx_, 1);
    server.router().get("/aw", [](http_request&, http_response& res) {
        res.status = http_status::ok;
        res.status_message = "OK";
        res.body = "aw-body";
    });
    if (!server.start()) {
        GTEST_SKIP() << "http_server failed to bind local port";
    }

    http_client client(client_ctx_);
    http_client_request req;
    req.method = http_method::GET();
    req.host = "127.0.0.1";
    req.scheme = "http";
    req.port = test_port;
    req.path = "/aw";

    auto awaiter = client.request_async(req, use_awaitable);
    auto deadline = steady_clock::now() + seconds(5);
    while (!awaiter.await_ready() && steady_clock::now() < deadline) {
        client_ctx_.run_one(100);
    }
    ASSERT_TRUE(awaiter.await_ready());
    auto resp = awaiter.await_resume();
    EXPECT_TRUE(resp.is_success());
    EXPECT_EQ(resp.body, "aw-body");

    client.close();
    server.stop();
}
#endif
