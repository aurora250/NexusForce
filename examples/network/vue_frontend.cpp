/**
 * @example vue_frontend.cpp
 * @brief Vue.js 前端 + NexusForce HTTP API 后端示例
 *
 * 演示如何使用 http_server 作为 Vue.js SPA 的生产服务器：
 * - 静态文件服务（Vue build 输出 dist/ 目录）
 * - SPA history 模式回退（Vue Router createWebHistory）
 * - RESTful API 路由（用户 CRUD、登录、仪表盘数据）
 * - CORS 跨域支持
 * - 请求日志记录
 * - 会话管理（Cookie-based Session）
 * - 压缩（gzip/deflate）
 * - API 限流保护
 * - 安全响应头
 *
 * ====== Vue.js 项目准备 ======
 *
 * 1. 创建 Vue 项目：
 *    npm create vue@latest vue-app
 *    cd vue-app
 *    npm install
 *
 * 2. 安装 Vue Router（history 模式）：
 *    npm install vue-router@4
 *
 * 3. 配置 Vite 代理（开发时可选）：
 *    // vite.config.js
 *    export default {
 *      server: {
 *        proxy: {
 *          '/api': 'http://localhost:8080'
 *        }
 *      }
 *    }
 *
 * 4. 构建生产版本：
 *    npm run build
 *    （生成 dist/ 目录）
 *
 * 5. 将 dist/ 放到本程序同级目录下，或使用命令行参数指定路径。
 *
 * ====== 使用方式 ======
 *
 * 生产模式（服务 dist/ 静态文件）：
 *   ./NexusForceVueFrontendExample
 *   ./NexusForceVueFrontendExample ./dist 8080
 *
 * 开发模式（反向代理到 Vite dev server :5173）：
 *   ./NexusForceVueFrontendExample --dev
 *   ./NexusForceVueFrontendExample --dev ./vue-app/dist 8080
 *
 * 访问：
 *   浏览器打开 http://localhost:8080
 *
 * API 端点：
 *   GET    /api/health              健康检查
 *   POST   /api/login               用户登录
 *   POST   /api/logout              用户登出
 *   GET    /api/session              获取当前会话
 *   GET    /api/users                用户列表（分页）
 *   GET    /api/users/:id            单个用户
 *   POST   /api/users                创建用户
 *   PUT    /api/users/:id            更新用户
 *   DELETE /api/users/:id            删除用户
 *   GET    /api/dashboard/stats      仪表盘统计数据
 *   GET    /api/notifications        通知列表
 */

#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/network/http/http_compress.hpp>
#include <NeForce/network/http/http_filter.hpp>
#include <NeForce/network/http/http_security.hpp>
#include <NeForce/network/http/http_server.hpp>
#include <NeForce/network/http/rate_limiter.hpp>

using namespace neforce;
using namespace neforce::http;

// ---- 模拟数据库 ----

struct User {
    int id;
    string name;
    string email;
    string role;
    string created_at;
};

class MockDatabase {
public:
    MockDatabase() {
        users_ = {
                {1, "Alice Wang", "alice@example.com", "admin", "2025-01-15T08:00:00Z"},
                {2, "Bob Li", "bob@example.com", "editor", "2025-02-20T10:30:00Z"},
                {3, "Charlie Zhang", "charlie@example.com", "viewer", "2025-03-10T14:00:00Z"},
                {4, "Diana Chen", "diana@example.com", "editor", "2025-04-05T09:15:00Z"},
                {5, "Ethan Liu", "ethan@example.com", "viewer", "2025-05-12T16:45:00Z"},
        };
        next_id_ = 6;
    }

    vector<User> get_users(int page, int page_size) const {
        vector<User> result;
        const int start = (page - 1) * page_size;
        if (start >= static_cast<int>(users_.size())) {
            return result;
        }
        const int end = min(start + page_size, static_cast<int>(users_.size()));
        for (int i = start; i < end; ++i) {
            result.push_back(users_[i]);
        }
        return result;
    }

    const User* get_user(int id) const {
        for (const auto& u: users_) {
            if (u.id == id) {
                return &u;
            }
        }
        return nullptr;
    }

    User create_user(const string& name, const string& email, const string& role) {
        User u{next_id_++, name, email, role, datetime::now().to_string()};
        users_.push_back(u);
        return u;
    }

    bool update_user(int id, const string& name, const string& email, const string& role) {
        for (auto& u: users_) {
            if (u.id == id) {
                if (!name.empty()) {
                    u.name = name;
                }
                if (!email.empty()) {
                    u.email = email;
                }
                if (!role.empty()) {
                    u.role = role;
                }
                return true;
            }
        }
        return false;
    }

    bool delete_user(int id) {
        for (auto it = users_.begin(); it != users_.end(); ++it) {
            if (it->id == id) {
                users_.erase(it);
                return true;
            }
        }
        return false;
    }

    size_t total_users() const { return users_.size(); }

private:
    vector<User> users_;
    int next_id_ = 1;
};

// ---- JSON 辅助函数 ----

// 将单个用户序列化到当前 json_builder 上下文中（作为对象）
static void write_user_json(json_builder& jb, const User& user) {
    jb.begin_object();
    jb.key("id").value(static_cast<double>(user.id));
    jb.key("name").value(user.name);
    jb.key("email").value(user.email);
    jb.key("role").value(user.role);
    jb.key("created_at").value(user.created_at);
    jb.end_object();
}

static string user_to_json(const User& user) {
    json_builder jb;
    write_user_json(jb, user);
    return jb.build()->to_string();
}

// ---- API 路由注册 ----

static void register_api_routes(http_router& router, MockDatabase& db) {
    // ===== 健康检查 =====
    router.get("/api/health", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("status").value("ok");
        jb.key("service").value("NexusForce Vue Backend");
        jb.key("timestamp").value(datetime::now().to_string());
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 登录 =====
    router.post("/api/login", [](http_request& req, http_response& res) {
        if (!req.has_session()) {
            res.status = http_status::S5_INTERNAL_SERVER_ERROR;
            res.body = R"({"error":"Session support required"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        // 简单演示：接受任意用户名密码
        auto it = req.form_data.find("username");
        const string username = (it != req.form_data.end()) ? it->second : "demo";

        auto* sess = req.session;
        sess->set("user", username);
        sess->set("role", "admin");
        sess->set("login_time", datetime::now().to_string());
        sess->regenerate_id(); // 防止 Session Fixation

        json_builder jb;
        jb.begin_object();
        jb.key("status").value("ok");
        jb.key("message").value("Login successful");
        jb.key("user").value(username);
        jb.key("token").value("nexusforce-demo-token");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 登出 =====
    router.post("/api/logout", [](http_request& req, http_response& res) {
        if (req.has_session()) {
            req.session->clear();
        }
        json_builder jb;
        jb.begin_object();
        jb.key("status").value("ok");
        jb.key("message").value("Logged out");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 获取当前会话 =====
    router.get("/api/session", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();

        if (req.has_session()) {
            auto* sess = req.session;
            jb.key("authenticated").value(!sess->get("user").empty());
            jb.key("user").value(sess->get("user"));
            jb.key("role").value(sess->get("role"));
            jb.key("login_time").value(sess->get("login_time"));
        } else {
            jb.key("authenticated").value(false);
            jb.key("user").value("guest");
        }

        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 用户列表（分页）=====
    router.get("/api/users", [&db](http_request& req, http_response& res) {
        int page = 1;
        int page_size = 10;

        const string_view query = req.query.view();
        if (query.starts_with("page=")) {
            auto amp = query.find('&');
            page = max(1, to_int32(query.view(5, amp == string::npos ? query.size() : amp)));
        }
        if (query.contains("page_size=")) {
            auto pos = query.find("page_size=");
            auto end = query.find('&', pos);
            page_size = max(1, min(100, to_int32(query.view(pos + 10, end == string::npos ? query.size() : end))));
        }

        const auto users = db.get_users(page, page_size);

        json_builder jb;
        jb.begin_object();
        jb.key("data");
        jb.begin_array();
        for (const auto& u: users) {
            write_user_json(jb, u);
        }
        jb.end_array();
        jb.key("page").value(static_cast<double>(page));
        jb.key("page_size").value(static_cast<double>(page_size));
        jb.key("total").value(static_cast<double>(db.total_users()));
        jb.key("total_pages").value(static_cast<double>((db.total_users() + page_size - 1) / page_size));
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 单个用户 =====
    router.get("/api/users/:id", [&db](http_request& req, http_response& res) {
        const int id = to_int32(req.parameter("id"));
        const auto* user = db.get_user(id);

        if (user == nullptr) {
            res.status = http_status::S4_NOT_FOUND;
            res.body = R"({"error":"User not found"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        res.body = user_to_json(*user);
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 创建用户 =====
    router.post("/api/users", [&db](http_request& req, http_response& res) {
        auto name_it = req.form_data.find("name");
        auto email_it = req.form_data.find("email");
        auto role_it = req.form_data.find("role");

        const string name = (name_it != req.form_data.end()) ? name_it->second : "";
        const string email = (email_it != req.form_data.end()) ? email_it->second : "";
        const string role = (role_it != req.form_data.end()) ? role_it->second : "viewer";

        if (name.empty() || email.empty()) {
            res.status = http_status::S4_BAD_REQUEST;
            res.body = R"({"error":"Name and email are required"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        const auto user = db.create_user(name, email, role);
        res.status = http_status::S2_CREATED;
        res.status_message = "Created";
        res.body = user_to_json(user);
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 更新用户 =====
    router.put("/api/users/:id", [&db](http_request& req, http_response& res) {
        const int id = to_int32(req.parameter("id"));

        auto name_it = req.form_data.find("name");
        auto email_it = req.form_data.find("email");
        auto role_it = req.form_data.find("role");

        const string name = (name_it != req.form_data.end()) ? name_it->second : "";
        const string email = (email_it != req.form_data.end()) ? email_it->second : "";
        const string role = (role_it != req.form_data.end()) ? role_it->second : "";

        if (!db.update_user(id, name, email, role)) {
            res.status = http_status::S4_NOT_FOUND;
            res.body = R"({"error":"User not found"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        const auto* updated = db.get_user(id);
        res.body = updated ? user_to_json(*updated) : R"({"status":"ok"})";
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 删除用户 =====
    router.del("/api/users/:id", [&db](http_request& req, http_response& res) {
        const int id = to_int32(req.parameter("id"));

        if (!db.delete_user(id)) {
            res.status = http_status::S4_NOT_FOUND;
            res.body = R"({"error":"User not found"})";
            res.set_content_type(http_content::JSON_APP());
            return;
        }

        json_builder jb;
        jb.begin_object();
        jb.key("status").value("ok");
        jb.key("message").value("User deleted");
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 仪表盘统计数据 =====
    router.get("/api/dashboard/stats", [&db](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_object();
        jb.key("total_users").value(static_cast<double>(db.total_users()));
        jb.key("active_sessions").value(128.0);
        jb.key("requests_today").value(15420.0);
        jb.key("avg_response_ms").value(12.5);
        jb.key("cpu_percent").value(34.7);
        jb.key("memory_mb").value(256.8);
        jb.end_object();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });

    // ===== 通知列表 =====
    router.get("/api/notifications", [](http_request& req, http_response& res) {
        json_builder jb;
        jb.begin_array();

        jb.begin_object();
        jb.key("id").value(1.0);
        jb.key("type").value("info");
        jb.key("title").value("System Update");
        jb.key("message").value("NexusForce v2.0 is now available");
        jb.key("read").value(false);
        jb.key("created_at").value("2025-06-10T08:00:00Z");
        jb.end_object();

        jb.begin_object();
        jb.key("id").value(2.0);
        jb.key("type").value("warning");
        jb.key("title").value("Disk Space");
        jb.key("message").value("Server disk usage at 85%");
        jb.key("read").value(false);
        jb.key("created_at").value("2025-06-09T15:30:00Z");
        jb.end_object();

        jb.begin_object();
        jb.key("id").value(3.0);
        jb.key("type").value("success");
        jb.key("title").value("Backup Complete");
        jb.key("message").value("Daily backup completed successfully");
        jb.key("read").value(true);
        jb.key("created_at").value("2025-06-08T02:00:00Z");
        jb.end_object();

        jb.end_array();
        res.body = jb.build()->to_string();
        res.set_content_type(http_content::JSON_APP());
    });
}

// ---- 入口 ----

int main(int argc, char* argv[]) {
    // 解析命令行参数
    string static_dir = "./dist";
    uint16_t port = 8080;
    bool dev_mode = false;

    for (int i = 1; i < argc; ++i) {
        const string_view arg(argv[i]);
        if (arg == "--dev") {
            dev_mode = true;
        } else if (!arg.starts_with("-")) {
            if (static_dir == "./dist") {
                static_dir = argv[i];
            } else {
                port = static_cast<uint16_t>(to_int32(arg));
            }
        }
    }

    println("╔══════════════════════════════════════════════════╗");
    println("║   NexusForce HTTP Server — Vue.js SPA Backend   ║");
    println("╚══════════════════════════════════════════════════╝");
    println();

    // ---- 创建服务器 ----
    http_server server{ports(port)};

    auto& router = server.router();

    // =========================================================================
    // 中间件链（按顺序执行）
    // =========================================================================

    // 1. 请求日志
    auto logger = make_unique<logging_filter>();
    logger->log_headers = false;
    logger->log_body = false;
    router.use(move(logger));

    // 2. CORS — 允许前端跨域访问
    auto cors = make_unique<cors_filter>("*");
    cors->allowed_headers = "Content-Type, Authorization, X-Requested-With";
    cors->allow_credentials = true;
    router.use(move(cors));

#ifdef NEFORCE_SUPPORT_ZLIB
    // 3. HTTP 压缩 — 自动 gzip/deflate
    auto compress = make_unique<compress_filter>();
    compress->min_size = 1_KB;
    router.use(move(compress));
#endif

    // 4. 安全响应头
    auto security = make_unique<security_headers_filter>();
    security->csp_value = "default-src 'self'; script-src 'self' 'unsafe-inline'; "
                          "style-src 'self' 'unsafe-inline'; font-src 'self'; "
                          "img-src 'self' data:; connect-src 'self'";
    security->enable_hsts = false; // 开发环境关闭 HSTS
    router.use(move(security));

    // 5. API 限流 — 30 req/s, 突发 60
    auto rate_limit = make_unique<token_bucket_filter>(30.0, 60.0);
    router.use(move(rate_limit));

    // 6. 静态文件服务 + SPA 回退（作为最后一个中间件）
    auto static_files = make_unique<static_file_filter>(static_dir);
    static_files->set_spa_fallback("index.html"); // Vue Router history 模式
    static_files->add_spa_exclude_path("/api");   // API 路由不触发 SPA 回退
    static_files->set_enable_cache(true);
    router.use(move(static_files));

    // =========================================================================
    // API 路由
    // =========================================================================

    MockDatabase db;
    register_api_routes(router, db);

    // =========================================================================
    // 404 处理器 — 返回 JSON（仅当 API 路径未匹配时触发）
    // =========================================================================

    router.set_not_found_handler([](http_request& req, http_response& res) {
        res.status = http_status::S4_NOT_FOUND;
        json_builder jb;
        jb.begin_object();
        jb.key("error").value("Not Found");
        jb.key("path").value(req.path);
        jb.key("method").value(req.method.to_string());
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

    // =========================================================================
    // 启动服务器
    // =========================================================================

    printfln("Mode:       {}", dev_mode ? "Development (Vite proxy)" : "Production");
    printfln("Static dir: {}", static_dir);
    printfln("Port:       {}", port);

    if (server.start()) {
        printfln("\n✅  Server running at http://localhost:{}", port);
        println();
        println("📋  API Endpoints:");
        println("    GET    /api/health");
        println("    POST   /api/login");
        println("    POST   /api/logout");
        println("    GET    /api/session");
        println("    GET    /api/users?page=1&page_size=10");
        println("    GET    /api/users/:id");
        println("    POST   /api/users");
        println("    PUT    /api/users/:id");
        println("    DELETE /api/users/:id");
        println("    GET    /api/dashboard/stats");
        println("    GET    /api/notifications");
        println();
        println("🌐  SPA fallback: enabled (index.html)");
        println("    All non-/api/*, non-file routes → index.html");
        println();
        println("💡  Try:");
        printfln("    curl http://localhost:{}/api/health", port);
        printfln("    curl http://localhost:{}/api/users", port);
        printfln("    curl -X POST http://localhost:{}/api/login -d 'username=demo'", port);
        println();
        println("    Open http://localhost:{}/ in browser (after npm run build)", port);
        println();

        while (true) {
            this_thread::sleep_for(100_ms);
        }

        server.stop();
        println("Server stopped");
    } else {
        println("❌  Failed to start HTTP server");
        return 1;
    }

    return 0;
}
