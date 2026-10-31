#ifndef NEFORCE_NETWORK_HTTP_REVERSE_PROXY_HPP__
#define NEFORCE_NETWORK_HTTP_REVERSE_PROXY_HPP__
/**
 * @file reverse_proxy.hpp
 * @brief HTTP 反向代理过滤器
 */

#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/flat_unordered_map.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/network/http/async_filter.hpp"
#include "NeForce/network/http/http_client.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct proxy_backend
 * @brief 反向代理后端服务器配置
 */
struct proxy_backend {
    string host;
    ports port;
    string scheme{"http"}; ///< "http" 或 "https"
    size_t weight{1};      ///< 权重（用于加权负载均衡）
    bool enabled{true};    ///< 是否启用

    NEFORCE_NODISCARD string to_url(const string& path) const {
        return scheme + "://" + host + ":" + to_string(port.value()) + path;
    }
};

/**
 * @brief 反向代理连接池统计
 */
struct reverse_proxy_stats {
    uint64_t requests{0};            ///< 代理请求数
    uint64_t backend_connections{0}; ///< 新建后端连接数
    uint64_t pool_waits{0};          ///< 连接池等待次数
    uint64_t queue_rejections{0};    ///< 排队上限拒绝次数
    uint64_t direct_requests{0};     ///< 同进程 direct-backend 请求数
    uint32_t pool_wait_p99_us{0};    ///< 连接池等待 p99（微秒）
    uint64_t async_requests{0};      ///< 异步远程代理请求数
    uint64_t async_connections{0};   ///< 异步远程代理新建后端连接数
    uint64_t async_errors{0};        ///< 异步远程代理传输错误数
};

/**
 * @class reverse_proxy_filter
 * @brief HTTP反向代理过滤器
 *
 * 将匹配的请求转发到后端服务器，并将响应返回给客户端。
 * 支持请求头重写、响应头过滤、超时控制。
 *
 * 使用示例：
 * @code
 * reverse_proxy_filter proxy;
 * proxy.add_backend({"api-backend", ports(8080)});
 * proxy.set_path_prefix("/api");  // 仅代理 /api/ * 路径
 * router.use(make_unique<reverse_proxy_filter>(move(proxy)));
 * @endcode
 */
class NEFORCE_API reverse_proxy_filter final : public async_filter {
public:
    /// 请求头重写回调
    using header_rewrite_cb = function<void(flat_unordered_map<string, string>&)>;
    /// 后端选择回调
    using backend_selector_cb = function<proxy_backend(const http_request&)>;
    /// 同进程后端处理回调，返回前必须填充 response
    using direct_backend_handler = function<void(http_request&, http_response&)>;

    milliseconds connect_timeout{5000};     ///< 连接超时
    milliseconds send_timeout{5000};        ///< 发送超时
    milliseconds receive_timeout{30000};    ///< 接收超时
    bool follow_redirects{false};           ///< 是否跟随重定向
    bool passthrough_host_header{false};    ///< 是否保持原始 Host 头（不过滤）
    size_t max_idle_connections{32};        ///< 每个后端保留的最大空闲连接数
    size_t max_connections_per_backend{0};  ///< 每个后端最大并发连接数（0=不限制）
    size_t max_queue_per_backend{0};        ///< 每个后端超过并发上限时的最大排队数（0=不限制）
    size_t max_h2c_upgrades{32};            ///< 最大 h2c 升级连接数
    bool verify_backend_ssl{true};          ///< 是否校验 HTTPS 后端证书
    bool raw_response_passthrough{false};   ///< 是否直接透传后端原始响应
    bool enable_thread_local_clients{true}; ///< 前端 worker 线程本地复用后端连接（仅 max_connections_per_backend=0 时）
    bool enable_async_backend{true};        ///< 是否为 async_filter 提供事件驱动后端
    size_t async_max_idle_connections{32};  ///< 异步后端连接池空闲连接上限
    size_t async_max_response_size{64UL * 1024UL * 1024UL}; ///< 异步后端响应最大字节数
    direct_backend_handler direct_backend;                  ///< 同进程直接后端，为空时走远程代理

    /**
     * @brief 构造函数
     * @param ctx 异步 I/O 执行上下文
     */
    explicit reverse_proxy_filter(io_context& ctx) :
    ctx_(&ctx) {}

    /**
     * @brief 构造函数（单后端）
     * @param ctx 异步 I/O 执行上下文
     */
    reverse_proxy_filter(io_context& ctx, string host, ports port, string scheme = "http") :
    ctx_(&ctx) {
        add_backend({move(host), port, move(scheme)});
    }

    /**
     * @brief 析构过滤器并关闭空闲异步后端连接
     */
    ~reverse_proxy_filter() noexcept override;

    /**
     * @brief 添加后端服务器
     */
    void add_backend(proxy_backend backend) {
        lock<mutex> lk(mutex_);
        backends_.push_back(move(backend));
    }

    /**
     * @brief 设置路径前缀匹配（仅代理匹配路径）
     */
    void set_path_prefix(string prefix) {
        lock<mutex> lk(mutex_);
        path_prefix_ = move(prefix);
    }

    /**
     * @brief 设置自定义后端选择器
     */
    void set_backend_selector(backend_selector_cb selector) {
        lock<mutex> lk(mutex_);
        selector_ = move(selector);
    }

    /**
     * @brief 设置请求头重写回调
     */
    void set_header_rewrite(header_rewrite_cb cb) {
        lock<mutex> lk(mutex_);
        header_rewrite_ = move(cb);
    }

    /**
     * @brief 添加不透传的响应头
     */
    void add_skip_response_header(string header) {
        lock<mutex> lk(mutex_);
        skip_resp_headers_.push_back(move(header));
    }

    /**
     * @brief 关闭并清空所有后端空闲连接
     */
    void close_idle_connections();

    /**
     * @brief 获取连接池统计
     * @return 请求数、新建后端连接数、等待次数与排队拒绝次数
     */
    NEFORCE_NODISCARD reverse_proxy_stats stats() const;

    bool pre_filter(http_request& request, http_response& response) override;

    /**
     * @brief 异步预处理
     * @param request HTTP请求
     * @param response HTTP响应
     * @param ctx 请求上下文
     * @param next 完成回调
     */
    void pre_filter_async(http_request& request, http_response& response, http_context& ctx,
                          next_callback next) override;

    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "reverse_proxy_filter"; }

private:
    io_context* ctx_{nullptr};
    vector<proxy_backend> backends_;
    string path_prefix_;
    backend_selector_cb selector_;
    header_rewrite_cb header_rewrite_;
    vector<string> skip_resp_headers_;
    mutable mutex mutex_;
    size_t rr_counter_{0}; ///< 轮询计数器

    struct backend_state {
        size_t active{0};  ///< 已借出的连接数
        size_t waiting{0}; ///< 等待连接的请求数
    };

    flat_unordered_map<string, vector<unique_ptr<http_client>>> idle_clients_; ///< 按后端复用的连接池
    flat_unordered_map<string, backend_state> backend_states_;                 ///< 后端并发/排队状态
    mutable mutex pool_mutex_;                                                 ///< 连接池互斥锁
    condition_variable pool_cv_;                                               ///< 连接池等待条件变量
    atomic<uint64_t> request_count_{0};
    atomic<uint64_t> backend_connection_count_{0};
    atomic<uint64_t> pool_wait_count_{0};
    atomic<uint64_t> queue_rejection_count_{0};
    atomic<uint64_t> direct_request_count_{0};
    vector<uint32_t> pool_wait_us_;

    struct reverse_proxy_async_state;

    mutable mutex async_mutex_;                         ///< 保护异步状态初始化
    shared_ptr<reverse_proxy_async_state> async_state_; ///< 异步后端连接池

    proxy_backend select_backend(const http_request& request);
    NEFORCE_NODISCARD shared_ptr<reverse_proxy_async_state> ensure_async_state();
    NEFORCE_NODISCARD string build_async_request(const proxy_backend& backend, const http_request& request);
    bool should_proxy(const string& path) const;
    http_client_response forward(const proxy_backend& backend, const http_request& request);
    http_client_raw_response forward_raw(const proxy_backend& backend, const http_request& request);
    http_client_request build_backend_request(const proxy_backend& backend, const http_request& request);
    void copy_response(http_client_response&& from, http_response& to);
    void filter_raw_response_headers(string& raw);

    NEFORCE_NODISCARD const string& backend_key(const proxy_backend& backend) const;
    NEFORCE_NODISCARD http_client::config backend_client_config() const;
    NEFORCE_NODISCARD unique_ptr<http_client> acquire_backend_client(const proxy_backend& backend);
    void release_backend_client(const proxy_backend& backend, unique_ptr<http_client> client);

    // TODO: WebSocket proxy — tunnel WebSocket connections through to backend with bidirectional frame forwarding
    // TODO: Response body transformation — modify/rewrite proxied response bodies (e.g., URL rewriting for CDN paths)
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_REVERSE_PROXY_HPP__
