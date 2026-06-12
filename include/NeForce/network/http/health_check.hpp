#ifndef NEFORCE_NETWORK_HTTP_HEALTH_CHECK_HPP__
#define NEFORCE_NETWORK_HTTP_HEALTH_CHECK_HPP__

/**
 * @file health_check.hpp
 * @brief 健康检查端点过滤器
 */

#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class health_check_filter
 * @brief 健康检查端点过滤器
 *
 * 拦截预配置路径的 GET 请求，返回 JSON 格式的健康状态。
 * 支持自定义健康检查回调（如数据库连接、Redis可达性等）。
 *
 * 响应格式：
 * {"status":"ok","uptime":12345,"checks":{"db":"ok","redis":"ok"}}
 * 非健康状态返回 503。
 */
class NEFORCE_API health_check_filter final : public http_filter {
public:
    string path{"/healthz"}; ///< 健康检查路径
    bool show_details{true}; ///< 是否展示详细检查结果
    bool enabled{true};      ///< 是否启用

    using check_callback = function<bool()>;

    health_check_filter() = default;

    /**
     * @brief 注册一个健康检查组件的回调
     * @param name 组件名（如 "db", "redis"）
     * @param check 返回true表示健康
     */
    void add_check(const string& name, check_callback check) { checks_[name] = move(check); }

    void remove_check(const string& name) { checks_.erase(name); }

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& /*request*/, http_response& /*response*/) override {}
    NEFORCE_NODISCARD string name() const override { return "health_check_filter"; }

private:
    unordered_map<string, check_callback> checks_;
    int64_t start_time_ms_{0}; ///< 在首次请求时惰性设置

    pair<string, bool> build_health_json();
};

// TODO: Metrics collection — request count, latency percentiles (p50/p95/p99), error rate, active connections gauge
// TODO: Prometheus exposition format — /metrics endpoint with Prometheus text format for scraping
// TODO: OpenTelemetry tracing — distributed trace context propagation (W3C TraceContext / B3), span export to OTLP/Jaeger/Zipkin
// TODO: Admin endpoints — /actuator/loggers (dynamic log level), /actuator/threaddump, /actuator/env

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HEALTH_CHECK_HPP__
