#ifndef NEFORCE_NETWORK_HTTP_REVERSE_PROXY_HPP__
#define NEFORCE_NETWORK_HTTP_REVERSE_PROXY_HPP__
/**
 * @file reverse_proxy.hpp
 * @brief HTTP 反向代理过滤器
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/unordered_map.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/network/http/http_client.hpp"
#include "NeForce/network/http/http_filter.hpp"
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
class NEFORCE_API reverse_proxy_filter final : public http_filter {
public:
    /// 请求头重写回调
    using header_rewrite_cb = function<void(unordered_map<string, string>&)>;
    /// 后端选择回调
    using backend_selector_cb = function<proxy_backend(const http_request&)>;

    /// @brief 连接超时（毫秒）
    milliseconds connect_timeout{5000};
    /// @brief 发送超时（毫秒）
    milliseconds send_timeout{5000};
    /// @brief 接收超时（毫秒）
    milliseconds receive_timeout{30000};
    /// @brief 是否跟随重定向
    bool follow_redirects{false};
    /// @brief 是否保持原始 Host 头（不过滤）
    bool passthrough_host_header{false};

    reverse_proxy_filter() = default;

    /**
     * @brief 构造函数（单后端）
     */
    reverse_proxy_filter(string host, ports port, string scheme = "http") {
        add_backend({move(host), port, move(scheme)});
    }

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

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "reverse_proxy_filter"; }

private:
    vector<proxy_backend> backends_;
    string path_prefix_;
    backend_selector_cb selector_;
    header_rewrite_cb header_rewrite_;
    vector<string> skip_resp_headers_;
    mutable mutex mutex_;
    size_t rr_counter_{0}; ///< 轮询计数器

    proxy_backend select_backend(const http_request& request);
    bool should_proxy(const string& path) const;
    http_client_response forward(const proxy_backend& backend, const http_request& request);
    void copy_response(const http_client_response& from, http_response& to);

    // TODO: WebSocket proxy — tunnel WebSocket connections through to backend with bidirectional frame forwarding
    // TODO: Response body transformation — modify/rewrite proxied response bodies (e.g., URL rewriting for CDN paths)
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_REVERSE_PROXY_HPP__
