#ifndef NEFORCE_NETWORK_HTTP_HTTP_SERVER_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_SERVER_HPP__

/**
 * @file http_server.hpp
 * @brief HTTP/HTTPS服务器实现
 *
 * 此文件提供了完整的HTTP/HTTPS服务器实现，支持路由、会话管理、
 * WebSocket升级、SSL/TLS加密等功能。
 *
 * 主要功能：
 * - HTTP/HTTPS服务器
 * - 路由管理（RESTful风格）
 * - 会话管理
 * - WebSocket支持
 * - SSL/TLS加密
 * - 请求转发
 * - 自动会话清理
 * - 大请求处理
 */

#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/event_loop.hpp"
#include "NeForce/network/http/http_router.hpp"
#include "NeForce/network/http/session_store.hpp"
#include "NeForce/network/http/websocket.hpp"
#include "NeForce/network/ssl/sni_manager.hpp"
#include "NeForce/network/tcp/tcp_server.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class http_server
 * @brief HTTP/HTTPS服务器类
 *
 * 提供完整的HTTP/HTTPS服务器功能，包括请求解析、路由分发、
 * 会话管理、WebSocket升级等。
 *
 * 使用示例：
 * @code
 * // HTTP服务器
 * http_server server(ports{8080});
 *
 * // 设置路由
 * server.router().get("/", [](http_request& req, http_response& res) {
 *     res.body = "<h1>Hello World</h1>";
 *     res.set_content_type(http_content::HTML_TEXT());
 * });
 *
 * server.router().get("/api/users", [](http_request& req, http_response& res) {
 *     res.body = R"({"users": [{"id": 1, "name": "John"}]})";
 *     res.set_content_type(http_content::JSON_APP());
 * });
 *
 * // WebSocket路由
 * server.websocket().route("/ws", [](websocket_server::session_ptr session) {
 *     session->set_message_handler([](const string& msg, websocket_opcode opcode) {
 *         session->send("Echo: " + msg);
 *     });
 * });
 *
 * // 启动服务器
 * server.start();
 * @endcode
 */
class NEFORCE_API http_server {
public:
    using socket_type = tcp_socket; ///< Socket类型

    /**
     * @struct session_manager
     * @brief 会话管理器
     *
     * 管理所有HTTP会话，包括创建、查找、删除和过期清理。
     */
    struct NEFORCE_API session_manager {
        unordered_map<string, http_session> sessions_; ///< 会话存储
        mutable mutex mutex_;                          ///< 会话互斥锁
        condition_variable cv_;                        ///< 条件变量
        atomic<bool> cleanup_running_;                 ///< 清理线程运行标志
        thread cleanup_thread_;                        ///< 清理线程
        seconds cleanup_interval_{300};                ///< 清理间隔
        size_t max_sessions_{10000};                   ///< 最大会话数

        session_manager();
        ~session_manager();

        session_manager(const session_manager&) = delete;
        session_manager& operator=(const session_manager&) = delete;

        session_manager(session_manager&&) noexcept = delete;
        session_manager& operator=(session_manager&&) noexcept = delete;

        http_session* get_session(const string& session_id, bool create = true);
        void remove_session(const string& session_id) noexcept;
        void cleanup_expired_sessions();
        bool session_exists(const string& session_id) const noexcept;

        size_t session_count() const noexcept;
        void set_cleanup_interval(seconds interval) noexcept;
        void set_max_sessions(size_t max) noexcept;
    };

private:
    unique_ptr<tcp_server_base> server_;       ///< TCP/SSL服务器
    vector<shared_ptr<event_loop>> h2c_loops_; ///< h2c升级连接的事件循环
    vector<thread> h2c_threads_;               ///< h2c升级连接的线程
    mutable mutex h2c_mutex_;                  ///< 保护h2c列表
    http_router router_;                       ///< HTTP路由器
    websocket_server ws_server_;               ///< WebSocket服务器
    session_manager session_manager_;          ///< 会话管理器
    sni_manager sni_;                          ///< SNI证书管理器

    http_cookie_name cookie_name_{http_cookie_name::JSESSIONID()}; ///< 会话Cookie名称
    unique_ptr<http::session_store> session_store_;                ///< 可插拔会话存储后端

    mutable mutex conn_mutex_;
    unordered_map<string, size_t> conn_per_ip_; ///< 每个IP的连接计数

    static http_request parse_request(tcp_socket* client_socket, session_manager& manager, const http_cookie_name& name,
                                      byte_size max_header_size, byte_size max_body_size, size_t max_header_count,
                                      milliseconds body_read_timeout);

    static http_session* get_or_create_session(http_request& request, bool create, session_manager& manager,
                                               const http_cookie_name& name);

public:
    byte_size max_server_header_size{16_KB}; ///< 最大请求头大小
    byte_size max_server_body_size{100_MB};  ///< 最大请求体大小
    bool enable_websocket{true};             ///< 是否启用WebSocket
    bool enable_connect{true};               ///< 是否启用CONNECT隧道

    size_t max_connections_per_ip{0};      ///< 每IP最大连接数（0=不限制）
    size_t max_header_count{100};          ///< 最大请求头数量
    size_t max_h2c_upgrades{32};           ///< 最大h2c升级连接数
    milliseconds body_read_timeout{30000}; ///< 请求体读取超时（毫秒）

private:
    unordered_map<string, function<bool(http_request&, tcp_socket*)>> upgrade_handlers_;

    void handle_client(unique_ptr<tcp_socket> client_socket);
    bool try_upgrade(unique_ptr<tcp_socket>& client_socket, http_request& request);
    static void handle_connect(const unique_ptr<tcp_socket>& client_socket, http_request& request);
    void handle_request_with_forward(tcp_socket& client_socket, http_request& request, http_session* sess);

public:
    /**
     * @brief 构造HTTP服务器
     * @param port 监听端口
     * @param worker_count 工作线程数（默认最大）
     */
    explicit http_server(ports port, size_t worker_count = thread_pool::max_thread_threshhold());

    /**
     * @brief 构造HTTPS服务器
     * @param port 监听端口
     * @param ctx SSL上下文（必须已加载证书）
     * @param worker_count 工作线程数（默认最大）
     */
    http_server(ports port, ssl_context ctx, size_t worker_count = thread_pool::max_thread_threshhold());

    ~http_server();

    http_server(const http_server&) = delete;
    http_server& operator=(const http_server&) = delete;

    http_server(http_server&&) noexcept = delete;
    http_server& operator=(http_server&&) noexcept = delete;

    /**
     * @brief 加载SSL证书
     * @param cert_file 证书文件路径
     * @param key_file 私钥文件路径
     * @return 加载成功返回true
     */
    bool load_certificate(const string& cert_file, const string& key_file);

    /**
     * @brief 添加SNI多域名证书
     * @param hostname 域名（如 "example.com"）
     * @param ctx 该域名的SSL上下文
     *
     * 允许同一端口为不同域名提供不同证书。
     * 在TLS握手时根据客户端SNI自动选择对应证书。
     */
    void add_sni_host(const string& hostname, ssl_context ctx) { sni_.add_host(hostname, move(ctx)); }

    /**
     * @brief 获取SNI管理器
     * @return SNI管理器引用
     */
    NEFORCE_NODISCARD sni_manager& sni() noexcept { return sni_; }

    /**
     * @brief 获取SNI管理器常量引用
     * @return SNI管理器常量引用
     */
    NEFORCE_NODISCARD const sni_manager& sni() const noexcept { return sni_; }

    /**
     * @brief 获取路由器引用
     * @return 路由器引用
     */
    NEFORCE_NODISCARD http_router& router() noexcept { return router_; }

    /**
     * @brief 获取路由器常量引用
     * @return 路由器常量引用
     */
    NEFORCE_NODISCARD const http_router& router() const noexcept { return router_; }

    /**
     * @brief 获取WebSocket服务器引用
     * @return WebSocket服务器引用
     */
    NEFORCE_NODISCARD websocket_server& websocket() noexcept { return ws_server_; }

    /**
     * @brief 获取WebSocket服务器常量引用
     * @return WebSocket服务器常量引用
     */
    NEFORCE_NODISCARD const websocket_server& websocket() const noexcept { return ws_server_; }

    /**
     * @brief 设置会话Cookie名称
     * @param name Cookie名称
     */
    void set_cookie_name(http_cookie_name name) noexcept { cookie_name_ = move(name); }

    /**
     * @brief 设置自定义会话存储后端
     * @param store 会话存储实现
     *
     * 可传入 redis_session_store 等实现以实现分布式会话。
     * 不设置则使用默认内存存储（session_manager内置）。
     */
    void set_session_store(unique_ptr<session_store> store) noexcept { session_store_ = move(store); }

    /**
     * @brief 获取会话存储后端
     * @return 存储后端指针，未设置返回nullptr
     */
    NEFORCE_NODISCARD http::session_store* session_store() noexcept { return session_store_.get(); }

    /**
     * @brief 获取会话Cookie名称
     * @return Cookie名称
     */
    NEFORCE_NODISCARD const http_cookie_name& cookie_name() const noexcept { return cookie_name_; }

    /**
     * @brief 设置会话清理间隔
     * @param interval 间隔时间
     */
    void set_session_cleanup_interval(seconds interval) noexcept { session_manager_.set_cleanup_interval(interval); }

    /**
     * @brief 设置最大会话数
     * @param max 最大数量
     */
    void set_max_sessions(size_t max) noexcept { session_manager_.set_max_sessions(max); }

    /**
     * @brief 注册协议升级处理器
     * @param protocol 协议名（如 "websocket", "h2c"）
     * @param handler 处理器函数，接收请求和socket，返回true表示已处理
     *
     * 服务器收到 Upgrade 请求时，按 Upgrade 头部值查找并调用对应处理器。
     * WebSocket 升级处理器已默认注册（当 enable_websocket 为 true 时）。
     */
    void set_upgrade_handler(string protocol, function<bool(http_request&, tcp_socket*)> handler) {
        upgrade_handlers_[move(protocol)] = move(handler);
    }

    /**
     * @brief 获取监听端口
     * @return 端口号
     */
    NEFORCE_NODISCARD ports port() const noexcept { return server_->port(); }

    /**
     * @brief 检查服务器是否运行中
     * @return 运行中返回true
     */
    NEFORCE_NODISCARD bool is_running() const noexcept { return server_->is_running(); }

    /**
     * @brief 获取会话（从请求中）
     * @param request HTTP请求
     * @param create 不存在时是否创建
     * @return 会话指针
     */
    NEFORCE_NODISCARD http_session* get_session(http_request& request, bool create = false);

    /**
     * @brief 启动服务器
     * @param backlog 连接队列大小
     * @return 启动成功返回true
     */
    bool start(int backlog = SOMAXCONN) { return server_->start(backlog); }

    /**
     * @brief 停止服务器
     */
    void stop() noexcept { server_->stop(); }

    /**
     * @brief 获取底层服务器指针
     * @return 服务器指针
     */
    NEFORCE_NODISCARD tcp_server_base* server() noexcept { return server_.get(); }

    /**
     * @brief 获取底层服务器常量指针
     * @return 服务器常量指针
     */
    NEFORCE_NODISCARD const tcp_server_base* server() const noexcept { return server_.get(); }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_SERVER_HPP__
