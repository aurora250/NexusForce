/**
 * @example health_check.cpp
 * @brief 健康检查 + 限流 + 日志 + 安全头 生产特性综合示例
 *
 * 演示生产环境常用 Filter：
 * - health_check_filter: 注册多个健康检查（DB、Redis、磁盘模拟）
 * - token_bucket_filter: IP 级别限流（10 req/s，突发 20）
 * - logging_filter: 请求/响应日志记录
 * - security_headers_filter: 自动添加 HSTS、CSP、X-Frame-Options
 *
 * 使用方式：
 *   curl http://localhost:8080/healthz            (健康检查)
 *   curl http://localhost:8080/healthz?details=1  (详细健康检查)
 *   curl http://localhost:8080/api/data           (正常请求)
 *   # 快速连续请求测试限流 (429 Too Many Requests)
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/health_check.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_security.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/rate_limiter.hpp>
#include <NeForce/core/system/sysinfo.hpp>
using namespace neforce;
using namespace neforce::http;

// 模拟外部服务健康状态
namespace {
    atomic<bool> db_healthy{true};
    atomic<bool> redis_healthy{true};
} // namespace

/**
 * @brief 检查磁盘空间
 */
bool check_disk_space() {
    const auto disk = sysinfo::get_disk_info();
    const auto free_mb = disk.free_bytes / (1024 * 1024);
    return free_mb > 100;
}

int main() {
    io_context context;

    http_server server(ports(8080u), context);
    auto& router = server.router();

    // =========================================================================
    // 过滤器链（顺序重要）
    // =========================================================================

    // 1. 安全头 — 最先执行，为所有响应添加安全头
    auto security = make_unique<security_headers_filter>();
    router.use(move(security));

    // 2. 限流 — IP 级别令牌桶
    auto rate_limit = make_unique<token_bucket_filter>(10.0, 20.0);
    rate_limit->per_route = false; // 全局限制（所有路由共享配额）
    router.use(move(rate_limit));

    // 3. 日志 — 记录所有请求和响应
    auto logging = make_unique<logging_filter>();
    logging->log_headers = true;
    logging->log_body = true;
    logging->max_body_log_size = 512_B; // 只记录前 512 字节
    router.use(move(logging));

    // 4. 健康检查 — 最后注册，匹配 /healthz 直接响应
    auto health = make_unique<health_check_filter>();
    health->path = "/healthz";
    health->show_details = true;

    // 注册健康检查项
    health->add_check("database", []() -> bool { return db_healthy.load(); });
    health->add_check("redis", []() -> bool { return redis_healthy.load(); });
    health->add_check("disk_space", check_disk_space);
    health->add_check("memory", []() -> bool {
        // 简单内存压力检查（无 OOM 即为健康）
        return true;
    });

    router.use(move(health));

    // =========================================================================
    // 路由
    // =========================================================================

    router.get("/", [](http_request& req, http_response& res) {
        res.body = R"(<h1>NexusForce Production Server</h1>
<p>Features: Health Check, Rate Limiting, Logging, Security Headers</p>
<ul>
  <li><a href="/healthz">GET /healthz</a> — Health check</li>
  <li><a href="/healthz?details=1">GET /healthz?details=1</a> — Detailed health</li>
  <li>GET /api/data — Protected API (rate limited: 10 req/s)</li>
  <li>POST /api/simulate/error — Simulate backend failure</li>
  <li>POST /api/simulate/recover — Recover backend</li>
</ul>)";
        res.set_content_type(http_content::HTML_TEXT().with_charset("utf-8"));
    });

    router.get("/api/data", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("status").value("ok");
        jb.key("data").begin_array().value("item1").value("item2").value("item3").end_array();
        jb.key("rate_limit_header").value(req.header("X-RateLimit-Remaining"));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 模拟 DB 故障
    router.post("/api/simulate/error", [](http_request& req, http_response& res) {
        db_healthy.store(false);
        json_builder jb;
        jb.begin_object();
        jb.key("simulated").value("database failure");
        jb.key("action").value("Now check GET /healthz to see unhealthy status");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // 恢复
    router.post("/api/simulate/recover", [](http_request& req, http_response& res) {
        db_healthy.store(true);
        redis_healthy.store(true);
        json_builder jb;
        jb.begin_object();
        jb.key("simulated").value("recovered all backends");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // =========================================================================
    // 启动
    // =========================================================================

    if (server.start()) {
        printfln("Production Server started on http://localhost:{}", static_cast<uint16_t>(server.port()));
        println("");
        println("=== Available Endpoints ===");
        printfln("  Health Check:  curl http://localhost:{}/healthz", static_cast<uint16_t>(server.port()));
        printfln("  Detailed HC:   curl http://localhost:{}/healthz?details=1", static_cast<uint16_t>(server.port()));
        printfln("  Data API:      curl http://localhost:{}/api/data", static_cast<uint16_t>(server.port()));
        printfln("  Simulate Fail: curl -X POST http://localhost:{}/api/simulate/error",
                 static_cast<uint16_t>(server.port()));
        println("");
        println("=== Rate Limiting Test ===");
        println("  for i in $(seq 1 25); do curl -s -o /dev/null -w '%{http_code}\n'");
        printfln("    http://localhost:{}/api/data; done", static_cast<uint16_t>(server.port()));
        println("  (Expect some 429 responses after exceeding 10 req/s)");

        while (true) {
            this_thread::sleep_for(10_ms);
        }
        server.stop();
        println("Server stopped");
    } else {
        println("Failed to start server");
        return 1;
    }

    return 0;
}
