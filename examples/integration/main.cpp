/**
 * @brief 集成示例——任务管理 REST API 服务器入口
 *
 * 演示各核心组件的协同工作：
 *
 * 网络：http_server、过滤器链（CORS→限流→日志→CSRF→压缩→健康检查→认证）、WebSocket
 * 数据库：database_pool、scope_transaction、repository、sql_builder
 * 反射：NFRS 标记宏 → 构建时代码生成 → json_serializer 自动序列化
 * 日志：层级化 Logger、控制台彩色输出、文件轮转、异步模式
 * 工具：uuid、sysinfo、bytesize 字面量、scope_exit、datetime
 */

#include "task.hpp"
#include "task_store.hpp"
#include "ws_broadcaster.hpp"

#include <NeForce/core/serialize/json_serializer.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <NeForce/core/utility/scope.hpp>
#include <NeForce/core/utility/uuid.hpp>
#include <NeForce/db/scope_transaction.hpp>
#include <NeForce/logging/file_sink.hpp>
#include <NeForce/logging/logger.hpp>
#include <NeForce/network/http/csrf_filter.hpp>
#include <NeForce/network/http/health_check.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/rate_limiter.hpp>
#include <NeForce/network/http/websocket.hpp>

using namespace neforce;
using namespace neforce::http;

namespace {
    TaskStore g_store;
    WsBroadcaster g_ws;
    atomic<bool> g_db_healthy{true};
} // namespace

// =========================================================================
// 路由注册
// =========================================================================

void setup_routes(http_router& router) {
    // ---- 首页 ----
    router.get("/", [](http_request& req, http_response& res) {
        res.body = R"(<h1>NexusForce Task Server</h1>
<h2>REST API</h2><ul>
<li>GET /api/tasks — 列出全部任务</li>
<li>GET /api/tasks/:id — 查询单个任务</li>
<li>POST /api/tasks — 创建任务（需 Bearer Token）</li>
<li>POST /api/tasks/:id — 更新任务（需 Bearer Token）</li>
<li>DELETE /api/tasks/:id — 删除任务（需 Bearer Token）</li>
<li>GET /api/tasks/search?status=pending — 按状态筛选</li>
<li>GET /api/tasks/stats — 统计</li>
</ul>
<h2>系统</h2><ul>
<li>GET /api/sysinfo — 系统信息</li>
<li>GET /healthz — 健康检查</li>
</ul>
<h2>实时</h2><ul>
<li>WebSocket ws://localhost:8080/ws/tasks</li>
</ul>
<p><small>限流: 20 req/s | 认证: Bearer nexusforce-demo-token</small></p>)";
        res.set_content_type(http_content::HTML_TEXT().with_charset("utf-8"));
    });

    // ---- 列出全部任务 ----
    router.get("/api/tasks", [](http_request& req, http_response& res) {
        auto tasks = g_store.find_all();
        json_builder jb;
        jb.begin_object();
        jb.key("tasks").begin_array();
        for (auto& t: tasks) {
            jb.value(serialize::json_serializer::to_string(reflect::meta_any(t)));
        }
        jb.end_array();
        jb.key("count").value(static_cast<double>(tasks.size()));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 按 ID 查询 ----
    router.get("/api/tasks/:id", [](http_request& req, http_response& res) {
        auto task = g_store.find_by_id(string(req.parameter("id")));
        if (task) {
            res.body = serialize::json_serializer::to_string(reflect::meta_any(*task));
        } else {
            res.status = http_status::S4_NOT_FOUND;
            res.body = R"({"error":"Task not found"})";
        }
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 创建任务（需要认证） ----
    router.post("/api/tasks", [](http_request& req, http_response& res) {
        auto conn = g_store.get_connection();
        if (!conn) {
            res.status = http_status::S5_INTERNAL_SERVER_ERROR;
            res.body = R"({"error":"Database unavailable"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        string_view title;
        string_view status_val = "pending";
        const char* p = req.body.data();
        const char* end = p + req.body.size();
        while (p < end) {
            string_view line(p, end - p);
            size_t nl = line.find('\n');
            if (nl != string::npos) {
                line = line.view(0, nl);
            }
            if (line.starts_with("title=")) {
                title = line.view(6);
            }
            if (line.starts_with("status=")) {
                status_val = line.view(7);
            }
            p += line.size() + 1;
        }

        if (title.empty()) {
            res.status = http_status::S4_BAD_REQUEST;
            res.body = R"({"error":"title is required"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        scope_transaction tx{*conn};
        Task t;
        t.id = uuid::v4().to_string();
        t.title = string(title);
        t.status = string(status_val);
        t.created_at = datetime::now().to_string();
        t.updated_at = t.created_at;

        g_store.insert(*conn, t);
        tx.commit();

        g_ws.broadcast("task_created", t);
        NEFORCE_LOGGER_LOGF_INFO("app.task", "created: id={}", t.id);

        res.status = http_status::S2_CREATED;
        res.body = serialize::json_serializer::to_string(reflect::meta_any(t));
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 更新任务（需要认证） ----
    router.post("/api/tasks/:id", [](http_request& req, http_response& res) {
        auto task = g_store.find_by_id(string(req.parameter("id")));
        if (!task) {
            res.status = http_status::S4_NOT_FOUND;
            res.body = R"({"error":"Task not found"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        auto conn = g_store.get_connection();
        if (!conn) {
            res.status = http_status::S5_INTERNAL_SERVER_ERROR;
            res.body = R"({"error":"Database unavailable"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        const char* p = req.body.data();
        const char* end = p + req.body.size();
        while (p < end) {
            string_view line(p, end - p);
            size_t nl = line.find('\n');
            if (nl != string::npos) {
                line = line.view(0, nl);
            }
            if (line.starts_with("title=")) {
                task->title = string(line.view(6));
            }
            if (line.starts_with("status=")) {
                task->status = string(line.view(7));
            }
            p += line.size() + 1;
        }
        task->updated_at = datetime::now().to_string();

        scope_transaction tx{*conn};
        g_store.update(*conn, *task);
        tx.commit();

        g_ws.broadcast("task_updated", *task);
        NEFORCE_LOGGER_LOGF_INFO("app.task", "updated: id={}", task->id);

        res.body = serialize::json_serializer::to_string(reflect::meta_any(*task));
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 删除任务（需要认证） ----
    router.del("/api/tasks/:id", [](http_request& req, http_response& res) {
        auto task = g_store.find_by_id(string(req.parameter("id")));
        if (!task) {
            res.status = http_status::S4_NOT_FOUND;
            res.body = R"({"error":"Task not found"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        auto conn = g_store.get_connection();
        if (!conn) {
            res.status = http_status::S5_INTERNAL_SERVER_ERROR;
            res.body = R"({"error":"Database unavailable"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        scope_transaction tx{*conn};
        g_store.remove(*conn, *task);
        tx.commit();

        g_ws.broadcast("task_deleted", *task);
        NEFORCE_LOGGER_LOGF_INFO("app.task", "deleted: id={}", task->id);

        json_builder jb;
        jb.begin_object();
        jb.key("deleted").value(true);
        jb.key("id").value(task->id);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 按状态搜索（正则路由） ----
    router.get("/api/tasks/search", [](http_request& req, http_response& res) {
        string_view query = req.query.view();
        string filter;
        if (query.starts_with("status=")) {
            filter = string(query.view(7));
        }

        auto all = g_store.find_all();
        json_builder jb;
        jb.begin_object();
        jb.key("tasks").begin_array();
        for (auto& t: all) {
            if (filter.empty() || t.status == filter) {
                jb.value(serialize::json_serializer::to_string(reflect::meta_any(t)));
            }
        }
        jb.end_array();
        jb.key("filter").value(filter);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 统计 ----
    router.get("/api/tasks/stats", [](http_request& req, http_response& res) {
        res.body = g_store.stats();
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 系统信息 ----
    router.get("/api/sysinfo", [](http_request& req, http_response& res) {
        auto& si = sysinfo::instance();
        auto disk = sysinfo::get_disk_info();
        auto& mem = si.get_memory_info();
        auto& cpu = si.get_CPU_info();
        auto& os = si.get_os_version_info();

        json_builder jb;
        jb.begin_object();
        jb.key("os").value(os.product_name);
        jb.key("os_version").value(os.version());
        jb.key("cpu").value(cpu.brand);
        jb.key("cpu_cores").value(static_cast<double>(cpu.cores));
        jb.key("cpu_logical").value(static_cast<double>(cpu.logical_processors));
        jb.key("cpu_usage_pct").value(sysinfo::cpu_usage());
        jb.key("memory_mb").begin_object();
        jb.key("total").value(static_cast<double>(mem.total_physical / 10 * 1024 * 1024));
        jb.key("available").value(static_cast<double>(mem.available_physical / 10 * 1024 * 1024));
        jb.end_object();
        jb.key("disk_mb").begin_object();
        jb.key("total").value(static_cast<double>(disk.total_bytes / 10 * 1024 * 1024));
        jb.key("free").value(static_cast<double>(disk.free_bytes / 10 * 1024 * 1024));
        jb.end_object();
        jb.key("uptime_sec").value(static_cast<double>(sysinfo::uptime_seconds()));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ---- 404 / 405 ----
    router.set_not_found_handler([](http_request& req, http_response& res) {
        res.status = http_status::S4_NOT_FOUND;
        json_builder jb;
        jb.begin_object();
        jb.key("error").value("Not Found");
        jb.key("path").value(req.path);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    router.set_method_not_allowed_handler([](http_request& req, http_response& res) {
        res.status = http_status::S4_METHOD_NOT_ALLOWED;
        json_builder jb;
        jb.begin_object();
        jb.key("error").value("Method Not Allowed");
        jb.key("method").value(req.method.to_string());
        jb.key("path").value(req.path);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });
}

// =========================================================================
// 过滤器链
// =========================================================================

void setup_filters(http_router& router) {
    router.use(make_unique<cors_filter>("*"));
    router.use(make_unique<token_bucket_filter>(20.0, 40.0));

    auto log_filter = make_unique<logging_filter>();
    log_filter->max_body_log_size = 256_B;
    router.use(move(log_filter));

    auto csrf = make_unique<csrf_filter>();
    csrf->cookie_name = "XSRF-TOKEN";
    router.use(move(csrf));

    auto compress = make_unique<compress_filter>();
    compress->min_size = 512_B;
    router.use(move(compress));

    auto health = make_unique<health_check_filter>();
    health->path = "/healthz";
    health->show_details = true;
    health->add_check("database", [] { return g_db_healthy.load(); });
    router.use(move(health));

    auto auth = make_unique<authentication_filter>();
    auth->set_auth_validator([](const http_request& req) -> bool {
        const string h = req.header("Authorization");
        return h.starts_with("Bearer ") && h.view(7) == "nexusforce-demo-token";
    });
    auth->add_included_path("/api/tasks");
    router.use(move(auth));
}

// =========================================================================
// 日志
// =========================================================================

void setup_logging() {
    auto root = logger_registry::instance().root_logger();
    root->set_level(log_level::INFO);

    auto console_sink = make_shared<neforce::console_sink>();
    console_sink->set_formatter(make_unique<log_formatter>("[{time}] [{level}] {message}"));
    root->add_sink(console_sink);

    auto file_sink = make_shared<neforce::file_sink>(path("logs/task_server.log"), 10 * 1024 * 1024, true, 3);
    file_sink->set_formatter(make_unique<log_formatter>("[{time}] [{level}] {file}:{line} - {message}"));
    root->add_sink(file_sink);

    root->enable_async(nullptr, 4096, overflow_policy::overrun_oldest);

    auto task_log = logger_registry::instance().get_logger("app.task");
    task_log->add_context("component", "task_server");
    NEFORCE_LOGGER_LOG_INFO("app.task", "日志系统就绪");
}

// =========================================================================
// 入口
// =========================================================================

int main() {
    setup_logging();

    if (!g_store.initialize()) {
        eprintln("数据库初始化失败");
        return 1;
    }

    io_context ioc;
    http_server server(ports(8080u), ioc, 4);
    auto& router = server.router();

    setup_filters(router);
    setup_routes(router);

    // WebSocket
    server.websocket().route("/ws/tasks", [](websocket_server::session_ptr s) {
        NEFORCE_LOG_INFO("WebSocket 客户端已连接");
        g_ws.add_session(s);
        s->set_message_handler([](const string& msg, websocket_opcode) { NEFORCE_LOGF_DEBUG("WS recv: {}", msg); });
        s->set_close_handler([](websocket_status st, const string&) {
            NEFORCE_LOGF_INFO("WS closed: code={}", static_cast<uint16_t>(st));
        });
        s->start();
        s->send(R"({"event":"connected","message":"Welcome"})");
    });
    server.websocket().set_io_context(ioc);

    if (!server.start()) {
        eprintln("服务器启动失败");
        return 1;
    }

    thread ws_loop_thread([&ioc] { ioc.run(); });

    auto cleanup = scope_exit([&ioc] {
        ioc.stop();
        g_store.shutdown();
        NEFORCE_LOG_INFO("Task Server 已停止");
    });

    thread io_thread([&ioc] { ioc.run(); });

    printcln(color::green(), "╔══════════════════════════════════════════════╗");
    printcln(color::green(), "║   NexusForce Task Server 已启动               ║");
    printcln(color::green(), "╚══════════════════════════════════════════════╝");
    printfln("\n  REST API:  http://localhost:{}", static_cast<uint16_t>(server.port()));
    printfln("  WebSocket: ws://localhost:{}/ws/tasks", static_cast<uint16_t>(server.port()));
    printfln("  健康检查:  http://localhost:{}/healthz\n", static_cast<uint16_t>(server.port()));

    printcln(color::yellow(), "=== 测试命令 ===\n");
    printfln("  curl http://localhost:{}/api/tasks", static_cast<uint16_t>(server.port()));
    printfln("  curl -X POST http://localhost:{}/api/tasks \\", static_cast<uint16_t>(server.port()));
    println("    -H \"Authorization: Bearer nexusforce-demo-token\" \\");
    println("    -d \"title=Buy groceries\"");
    printfln("  curl http://localhost:{}/api/sysinfo", static_cast<uint16_t>(server.port()));
    printfln("  curl http://localhost:{}/healthz?details=1", static_cast<uint16_t>(server.port()));
    printfln("  wscat -c ws://localhost:{}/ws/tasks\n", static_cast<uint16_t>(server.port()));

    while (true) {
        this_thread::sleep_for(100_ms);
    }
}
