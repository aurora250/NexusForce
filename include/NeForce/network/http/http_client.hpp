#ifndef NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__

/**
 * @file http_client.hpp
 * @brief HTTP客户端实现
 *
 * 此文件提供了完整的HTTP客户端实现，支持HTTP/HTTPS请求、重定向、
 * Cookie管理、代理、文件下载等功能。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/file/path.hpp"
#include "NeForce/core/utility/byte_size.hpp"
#include "NeForce/network/http/http_client_message.hpp"
#include "NeForce/network/tcp/tcp_client.hpp"
#include "NeForce/network/util/url.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct http_retry_policy
 * @brief 重试策略
 * @warning 客户端无法判断请求是否幂等，非幂等请求开启重试可能产生重复副作用
 */
struct http_retry_policy {
    uint16_t max_attempts = 1;            ///< 总尝试次数，1 表示不重试
    milliseconds initial_backoff{200};    ///< 首次退避时长
    milliseconds max_backoff{5000};       ///< 单次退避时长上限
    uint16_t backoff_multiplier = 2;      ///< 退避倍数，每次失败后乘该值
    bool honor_retry_after = true;        ///< 是否优先采用响应的 `Retry-After` 头（秒）作为退避时长
    bool retry_on_transport_error = true; ///< 传输层失败时是否重试
    bool retry_on_status_code = true;     ///< 可重试状态码是否重试
};

/**
 * @class http_stream_handle
 * @brief 流式请求控制句柄
 * @note 重复使用同一句柄发起下一次流式请求前应先 `reset()`
 */
class NEFORCE_API http_stream_handle {
public:
    http_stream_handle() = default;
    ~http_stream_handle() = default;

    http_stream_handle(const http_stream_handle&) = delete;
    http_stream_handle& operator=(const http_stream_handle&) = delete;

    /**
     * @brief 取消进行中的流式请求
     * @warning 只中断本机阻塞读取，不会通知对端；被中止的连接不再复用
     */
    void abort() noexcept;

    /**
     * @brief 查询是否已请求取消
     * @return 已置位取消标记返回true
     */
    NEFORCE_NODISCARD bool is_aborted() const noexcept { return aborted_.load(); }

    /**
     * @brief 清除取消标记，供复用句柄发起下一次请求
     */
    void reset() noexcept {
        aborted_.store(false);
        handle_.store(socket_base::invalid_handle);
    }

private:
    friend class http_client;

    void attach(const socket_base::native_handle_type handle) noexcept { handle_.store(handle); }
    void detach() noexcept { handle_.store(socket_base::invalid_handle); }

    atomic<bool> aborted_{false};                                                 ///< 是否已请求取消
    atomic<socket_base::native_handle_type> handle_{socket_base::invalid_handle}; ///< 当前绑定的 socket 句柄
};

/**
 * @struct http_stream_observer
 * @brief 流式响应观察者
 */
struct http_stream_observer {
    function<void(const http_client_response&)> on_headers; ///< 响应头到达回调
    function<bool(string_view data)> on_data;               ///< 正文片段回调，返回 false 立即中止

    /**
     * @brief 检查观察者是否具备正文回调
     * @return 已设置 `on_data` 返回true
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return static_cast<bool>(on_data); }
};

/**
 * @class http_client
 * @brief HTTP客户端类
 *
 * 提供完整的HTTP客户端功能，支持同步和异步请求。
 *
 * 主要功能：
 * - HTTP/HTTPS请求
 * - 自动重定向处理
 * - Cookie持久化
 * - 请求/响应超时控制
 * - 代理支持
 * - 文件下载
 * - 异步请求
 * - SSL/TLS支持
 * - 进度回调
 * - 分块传输处理
 *
 * 使用示例：
 * @code
 * // 创建HTTP客户端
 * http_client client;
 *
 * // GET请求
 * auto response = client.get("https://api.example.com/users");
 * if (response.is_success()) {
 *     println(response.body);
 * }
 *
 * // POST JSON请求
 * string json = R"({"name": "John", "age": 30})";
 * auto post_resp = client.post_json("https://api.example.com/users", json);
 *
 * // POST表单请求
 * flat_unordered_map<string, string> form = {{"username", "john"}, {"password", "123"}};
 * auto form_resp = client.post_form("https://example.com/login", form);
 *
 * // 设置自定义请求头
 * flat_unordered_map<string, string> headers = {{"Authorization", "Bearer token123"}};
 * auto auth_resp = client.get("https://api.example.com/profile", headers);
 *
 * // 下载文件
 * client.download_file("https://example.com/file.zip", "/path/to/save/file.zip");
 *
 * // 设置代理
 * client.set_proxy("proxy.example.com", 8080);
 *
 * // 设置超时
 * client.set_timeout(milliseconds(5000));
 *
 * // 异步请求
 * auto future = client.request_async(req);
 * auto async_resp = future.get();
 * @endcode
 */
class NEFORCE_API http_client {
public:
    /**
     * @struct config
     * @brief HTTP客户端配置
     */
    struct config {
        milliseconds connect_timeout{5000};                 ///< 连接超时
        milliseconds send_timeout{5000};                    ///< 发送超时
        milliseconds receive_timeout{5000};                 ///< 接收超时
        uint16_t max_redirects = 5;                         ///< 最大重定向次数
        bool follow_redirects = true;                       ///< 是否跟随重定向
        bool keep_alive = true;                             ///< 是否复用连接
        milliseconds keep_alive_idle_timeout{60000};        ///< 空闲连接复用上限，超过后重连；0 表示不限制
        milliseconds stream_idle_timeout{300000};           ///< 流式响应两段正文之间的最长空闲，0 表示沿用接收超时
        bool verify_ssl = true;                             ///< 是否验证SSL证书
        byte_size max_response_size{10_MB};                 ///< 最大响应大小
        byte_size buffer_size{8_KB};                        ///< 缓冲区大小
        flat_unordered_map<string, string> default_headers; ///< 默认请求头
        string user_agent{"NexusForce HTTP Client/1.0"};    ///< User-Agent
        string proxy_host;                                  ///< 代理主机
        ports proxy_port{ports::undef};                     ///< 代理端口
        http_retry_policy retry;                            ///< 重试策略
        // TODO: Connection pool — max_connections_per_host, idle_timeout, max_idle_connections
        // TODO: Circuit breaker — failure_threshold, recovery_timeout, half_open_max_requests
    };

    using progress_callback_t = function<void(size_t, size_t)>; ///< 进度回调类型
    using error_callback_t = function<void(const exception&)>;  ///< 错误回调类型
    using time_point = steady_clock::time_point;                ///< 时间点类型

    using client_type = ssl_client; ///< 底层客户端类型

private:
    client_type client_;                                    ///< TCP/SSL客户端
    io_context* ctx_{nullptr};                              ///< 异步 I/O 执行上下文
    config config_;                                         ///< 客户端配置
    flat_unordered_map<string, http_cookie> cookie_jar_;    ///< Cookie存储
    flat_unordered_map<string, string> persistent_headers_; ///< 持久化请求头
    mutable mutex mutex_;                                   ///< 保护共享数据

    progress_callback_t progress_callback_; ///< 进度回调
    error_callback_t error_callback_;       ///< 错误回调
    time_point last_activity_;              ///< 上一次响应结束时刻

private:
    string build_request_str(const http_client_request& req, const url& req_url) const;
    bool send_request(string_view request_str, time_point& send_start);

    optional<http_client_response> read_response(time_point& receive_start, const string& request_host,
                                                 const string& request_path, bool expect_body);

    void update_cookies(const vector<http_cookie>& resp_cookies, const url& request_url);
    string build_cookie_header(const url& request_url) const;

    bool prepare_connection(const http_client_request& request);

    http_client_response do_request_once(http_client_request request, int redirect_count, bool& transport_failed);
    http_client_response do_request(http_client_request request, int redirect_count = 0);

    http_client_response stream_once(const http_client_request& request, int redirect_count,
                                     const http_stream_observer& observer, http_stream_handle* handle,
                                     bool& transport_failed);
    http_client_response do_request_stream(const http_client_request& request, const http_stream_observer& observer,
                                           http_stream_handle* handle, int redirect_count);

    milliseconds retry_backoff(uint16_t attempt, const http_client_response& response) const;

    bool ensure_connected(const string& host, ports port, bool use_ssl = false);

public:
    explicit http_client(io_context& ioc) :
    http_client(ioc, config()) {}

    /**
     * @brief 构造函数
     * @param ioc 异步 I/O 执行上下文
     * @param cfg 客户端配置
     */
    explicit http_client(io_context& ioc, config cfg);

    explicit http_client(io_context& ioc, ssl_context ctx) :
    http_client(ioc, move(ctx), config()) {}

    /**
     * @brief 构造函数（带SSL上下文）
     * @param ioc 异步 I/O 执行上下文
     * @param ctx SSL上下文
     * @param cfg 客户端配置
     */
    explicit http_client(io_context& ioc, ssl_context ctx, config cfg);

    ~http_client() = default;

    http_client(const http_client&) = delete;
    http_client& operator=(const http_client&) = delete;

    http_client(http_client&&) noexcept = delete;
    http_client& operator=(http_client&&) noexcept = delete;

    // TODO: Declarative HTTP client — interface-based client generation (like @FeignClient / HttpExchange), define API as abstract methods with annotations

    /**
     * @brief 设置客户端配置
     * @param cfg 新配置
     */
    void set_config(config cfg) {
        lock<mutex> lk(mutex_);
        config_ = move(cfg);
    }

    /**
     * @brief 获取客户端配置
     * @return 配置常量引用
     */
    const config& get_config() const noexcept { return config_; }

    /**
     * @brief 获取底层客户端
     * @return 客户端引用
     */
    const client_type& get_client() const noexcept { return client_; }
    client_type& get_client() noexcept { return client_; }

    /**
     * @brief 设置默认请求头
     * @param key 头名称
     * @param value 头值
     */
    void set_default_header(const string& key, string value) {
        lock<mutex> lk(mutex_);
        persistent_headers_[key] = move(value);
    }

    /**
     * @brief 移除默认请求头
     * @param key 头名称
     */
    void remove_default_header(const string& key) {
        lock<mutex> lk(mutex_);
        persistent_headers_.erase(key);
    }

    /**
     * @brief 设置最大重定向次数
     * @param max 最大次数
     */
    void set_max_redirects(uint16_t max) { config_.max_redirects = max; }

    /**
     * @brief 设置是否跟随重定向
     * @param follow 是否跟随
     */
    void set_follow_redirects(bool follow) { config_.follow_redirects = follow; }

    /**
     * @brief 设置统一超时时间
     * @param timeout 超时时间
     */
    void set_timeout(milliseconds timeout) {
        config_.connect_timeout = timeout;
        config_.send_timeout = timeout;
        config_.receive_timeout = timeout;
    }

    /**
     * @brief 设置代理
     * @param host 代理主机
     * @param port 代理端口
     */
    void set_proxy(string host, const ports port) {
        config_.proxy_host = move(host);
        config_.proxy_port = port;
    }

    /**
     * @brief 清除代理设置
     */
    void clear_proxy() {
        config_.proxy_host.clear();
        config_.proxy_port = ports::undef;
    }

    /**
     * @brief 设置进度回调
     * @param callback 回调函数
     */
    void set_progress_callback(progress_callback_t callback) { progress_callback_ = move(callback); }

    /**
     * @brief 设置错误回调
     * @param callback 回调函数
     */
    void set_error_callback(error_callback_t callback) { error_callback_ = move(callback); }

    /**
     * @brief 设置SSL上下文
     * @param ctx SSL上下文
     */
    void set_ssl_context(ssl_context ctx);

    /**
     * @brief 设置是否验证SSL证书
     * @param verify 是否验证
     */
    void set_verify_ssl(bool verify);

    /**
     * @brief 清空Cookie存储
     */
    void clear_cookies() {
        lock<mutex> lk(mutex_);
        cookie_jar_.clear();
    }

    /**
     * @brief 设置Cookie
     * @param c Cookie对象
     * @param domain 域名
     * @param path 路径
     */
    void set_cookie(const http_cookie& c, const string& domain, const string& path = "/");

    /**
     * @brief 获取所有Cookie
     * @return Cookie映射
     */
    NEFORCE_NODISCARD flat_unordered_map<string, http_cookie> get_cookies() const {
        lock<mutex> lk(mutex_);
        return cookie_jar_;
    }

    /**
     * @brief 发送GET请求
     * @param url 请求URL
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP GET请求到指定URL。
     * 支持HTTP和HTTPS协议。
     * 自动处理Cookie和重定向。
     */
    http_client_response get(const string& url, const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送POST请求
     * @param url 请求URL
     * @param body 请求正文
     * @param content_type Content-Type头值
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP POST请求到指定URL。
     * 默认Content-Type为application/x-www-form-urlencoded。
     * 支持HTTP和HTTPS协议。
     * 自动处理Cookie和重定向。
     */
    http_client_response post(const string& url, const string& body = "",
                              const string& content_type = "application/x-www-form-urlencoded",
                              const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送POST JSON请求
     * @param url_str 请求URL
     * @param json_body JSON格式的请求正文
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送JSON格式的POST请求。
     * Content-Type自动设置为application/json。
     * 适用于RESTful API调用。
     */
    http_client_response post_json(const string& url_str, const string& json_body,
                                   const flat_unordered_map<string, string>& headers);

    /**
     * @brief 发送POST表单请求
     * @param url_str 请求URL
     * @param form_data 表单数据键值对
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送application/x-www-form-urlencoded格式的POST请求。
     * 自动将表单数据编码为URL编码格式。
     * 适用于HTML表单提交场景。
     */
    http_client_response post_form(const string& url_str, const flat_unordered_map<string, string>& form_data,
                                   const flat_unordered_map<string, string>& headers);

    /**
     * @brief 发送PUT请求
     * @param url 请求URL
     * @param body 请求正文
     * @param content_type Content-Type头值
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP PUT请求到指定URL。
     * 用于更新资源，通常包含完整的资源表示。
     * 默认Content-Type为application/x-www-form-urlencoded。
     */
    http_client_response put(const string& url, const string& body = "",
                             const string& content_type = "application/x-www-form-urlencoded",
                             const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送DELETE请求
     * @param url 请求URL
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP DELETE请求到指定URL。
     * 用于删除指定资源。
     */
    http_client_response del(const string& url, const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送HEAD请求
     * @param url 请求URL
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP HEAD请求到指定URL。
     * 只获取响应头，不获取响应正文。
     * 适用于检查资源是否存在或获取元信息。
     */
    http_client_response head(const string& url, const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送OPTIONS请求
     * @param url 请求URL
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP OPTIONS请求到指定URL。
     * 用于获取服务器支持的HTTP方法列表。
     * 常用于CORS预检请求。
     */
    http_client_response options(const string& url, const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送PATCH请求
     * @param url 请求URL
     * @param body 请求正文
     * @param content_type Content-Type头值
     * @param headers 自定义请求头
     * @return HTTP响应对象
     *
     * 发送HTTP PATCH请求到指定URL。
     * 用于部分更新资源，只发送需要修改的字段。
     * 默认Content-Type为application/x-www-form-urlencoded。
     */
    http_client_response patch(const string& url, const string& body = "",
                               const string& content_type = "application/x-www-form-urlencoded",
                               const flat_unordered_map<string, string>& headers = {});

    /**
     * @brief 发送自定义HTTP请求
     * @param req 请求对象
     * @return 响应对象
     */
    http_client_response request(http_client_request req);

    /**
     * @brief 下载文件
     * @param url 文件URL
     * @param output 输出路径
     * @param is_binary 是否为二进制文件
     * @return 下载成功返回true
     */
    bool download_file(const string& url, path output, bool is_binary = true);

    /**
     * @brief 发送流式请求
     * @param req 请求对象
     * @param observer 响应头与正文片段回调
     * @param handle 可选的取消句柄，请求返回前一直绑定底层连接
     * @return 响应元信息，`body` 始终为空
     * @warning 被取消时返回值仍可能带有已解析的状态码与响应头，需要通过 `http_stream_handle::is_aborted` 区分"对端正常结束"与"本端中止"
     */
    http_client_response request_stream(const http_client_request& req, const http_stream_observer& observer,
                                        http_stream_handle* handle = nullptr);

    /**
     * @brief 异步流式请求
     * @param req 请求对象
     * @param observer 响应头与正文片段回调
     * @param handle 可选的取消句柄
     * @return future响应元信息
     * @warning `handle` 由调用方管理生命周期，future 完成前不得析构
     */
    future<http_client_response> request_stream_async(http_client_request req, const http_stream_observer& observer,
                                                      http_stream_handle* handle = nullptr);

    /**
     * @brief 异步HTTP请求
     * @param req 请求对象
     * @return future响应对象
     */
    future<http_client_response> request_async(http_client_request req);

#ifdef NEFORCE_STANDARD_20
    /**
     * @brief awaitable 异步HTTP请求
     * @param req 请求对象
     * @return 可协程等待的结果
     */
    awaitable<http_client_response> request_async(http_client_request req, use_awaitable_t /*unused*/);
#endif

    /**
     * @brief 关闭连接
     */
    void close();

    /**
     * @brief 检查是否已连接
     * @return 已连接返回true
     */
    NEFORCE_NODISCARD bool is_connected() const noexcept { return client_.is_connected(); }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CLIENT_HPP__
