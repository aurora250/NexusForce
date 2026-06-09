/**
 * @example reverse_proxy.cpp
 * @brief 反向代理 + 负载均衡示例
 *
 * 演示反向代理和负载均衡功能：
 * - 启动 2 个后端 HTTP 服务器（port 8081、8082）
 * - 持久 http_client 连接池：每后端复用 TCP 连接，无 SSL 握手开销
 * - 自定义 Round-Robin 负载均衡
 * - /api/ * 请求轮询转发到后端
 *
 * 使用方式：
 *   curl http://localhost:8080/api/info     → 轮询转发到 8081 或 8082
 *   curl http://localhost:8080/frontend     → 由主服务器直接处理
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_client.hpp>
#include <NeForce/network/http/http_server.hpp>
using namespace neforce;
using namespace neforce::http;

// ---- 后端服务器 ----

unique_ptr<http_server> start_backend(const string& name, uint16_t port) {
    auto server = make_unique<http_server>(ports(port));

    server->router().get("/api/info", [name, port](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("backend").value(name);
        jb.key("port").value(static_cast<double>(port));
        jb.key("timestamp").value(static_cast<double>(::time(nullptr)));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    server->router().get("/api/health", [](http_request& req, http_response& res) {
        res.body = R"({"status":"ok"})";
        res.set_content_type(http_content::JSON_APP());
    });

    server->start();
    return server;
}

// ---- 持久连接池 ----

class proxy_pool {
public:
    void add_backend(const string& host, uint16_t port) {
        backends_.push_back({host, port, make_unique<http_client>()});
    }

    http_client_response proxy(const string& path) {
        lock<mutex> lk(mtx_);
        rr_counter_ = (rr_counter_ + 1) % backends_.size();
        auto& b = backends_[rr_counter_];

        http_client_request creq;
        creq.method = http_method::GET();
        creq.host = b.host;
        creq.port = ports(b.port);
        creq.path = path;
        creq.headers["Connection"] = "keep-alive";

        return b.client->request(move(creq));
    }

private:
    struct backend_entry {
        string host;
        uint16_t port;
        unique_ptr<http_client> client;
    };
    vector<backend_entry> backends_;
    size_t rr_counter_ = 0;
    mutex mtx_;
};

int main() {
    // 启动 2 个后端
    println("Starting backend servers...");
    auto backend1 = start_backend("backend-1", 8081);
    auto backend2 = start_backend("backend-2", 8082);
    println("  backend-1: http://localhost:8081");
    println("  backend-2: http://localhost:8082");

    // 持久 http_client 连接池（纯 HTTP，无 SSL 握手）
    proxy_pool pool;
    pool.add_backend("127.0.0.1", 8081);
    pool.add_backend("127.0.0.1", 8082);
    println("Connection pool: 2 persistent HTTP clients");

    // 代理服务器
    http_server proxy_server(ports(8080u));
    auto& router = proxy_server.router();

    router.get("/api/info", [&pool](http_request& req, http_response& res) {
        auto backend_resp = pool.proxy(req.path);
        res.status = backend_resp.status;
        res.body = backend_resp.body;
        for (auto& h: backend_resp.headers) {
            if (!h.second.empty()) {
                res.headers[h.first] = h.second[0];
            }
        }
        res.set_content_type(http_content::JSON_APP());
    });

    router.get("/api/health", [&pool](http_request& req, http_response& res) {
        auto backend_resp = pool.proxy(req.path);
        res.status = backend_resp.status;
        res.body = backend_resp.body;
        res.set_content_type(http_content::JSON_APP());
    });

    router.get("/", [](http_request& req, http_response& res) {
        res.body = R"(<h1>NexusForce Reverse Proxy + Load Balancer</h1>
<ul>
  <li>GET <a href="/api/info">/api/info</a> → Backend (Round-Robin)</li>
  <li>GET <a href="/api/health">/api/health</a> → Backend Health</li>
  <li>GET <a href="/frontend">/frontend</a> → Direct (not proxied)</li>
</ul>
<p>Refresh /api/info to see backend rotation</p>)";
        res.set_content_type(http_content::HTML_TEXT());
    });

    router.get("/frontend", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("source").value("frontend (direct)");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    if (proxy_server.start()) {
        printfln("Reverse Proxy started on http://localhost:{}", static_cast<uint16_t>(proxy_server.port()));
        println("");
        println("  curl http://localhost:8080/              (info page)");
        println("  curl http://localhost:8080/api/info      (→ backend Round-Robin)");
        println("  curl http://localhost:8080/api/health    (→ backend health)");
        println("  curl http://localhost:8080/frontend      (direct)");

        while (true) {
            this_thread::sleep_for(10_ms);
        }

        proxy_server.stop();
        backend1->stop();
        backend2->stop();
        println("All servers stopped");
    } else {
        println("Failed to start proxy server");
        return 1;
    }

    return 0;
}
