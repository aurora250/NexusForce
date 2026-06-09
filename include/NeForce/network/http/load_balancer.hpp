#ifndef NEFORCE_NETWORK_HTTP_LOAD_BALANCER_HPP__
#define NEFORCE_NETWORK_HTTP_LOAD_BALANCER_HPP__

/**
 * @file load_balancer.hpp
 * @brief HTTP 负载均衡管理器
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/utility/packages.hpp"
#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @enum lb_strategy
 * @brief 负载均衡策略
 */
enum class lb_strategy : uint8_t {
    ROUND_ROBIN,       ///< 轮询
    LEAST_CONNECTIONS, ///< 最少连接
    WEIGHTED,          ///< 加权轮询
    RANDOM             ///< 随机
};

/**
 * @struct lb_backend
 * @brief 负载均衡后端节点
 */
struct lb_backend {
    string host;
    ports port;
    string scheme{"http"};
    size_t weight{1};
    atomic<size_t> active_connections{0};
    atomic<bool> healthy{true};
    uint64_t last_health_check_ms{0};
    size_t consecutive_failures{0};

    lb_backend() = default;
    lb_backend(string h, ports p, string s = "http", size_t w = 1) :
    host(move(h)),
    port(p),
    scheme(move(s)),
    weight(w) {}
};

/**
 * @class load_balancer
 * @brief HTTP负载均衡管理器
 *
 * 管理多个后端节点，按策略分发请求。
 * 支持主动健康检查和故障转移。
 *
 * 使用示例：
 * @code
 * load_balancer lb;
 * lb.add_backend({"web1", ports(8080)});
 * lb.add_backend({"web2", ports(8080)});
 * lb.set_strategy(lb_strategy::LEAST_CONNECTIONS);
 *
 * auto* backend = lb.select_backend();
 * if (backend) {
 *     // 转发请求到 backend
 *     lb.release_backend(backend);  // 请求完成后调用
 * }
 * @endcode
 */
class NEFORCE_API load_balancer {
public:
    using health_check_cb = function<bool(const lb_backend&)>;

private:
    vector<lb_backend> backends_;
    lb_strategy strategy_{lb_strategy::ROUND_ROBIN};
    mutable mutex mutex_;
    atomic<size_t> rr_counter_{0};

    size_t max_failures_{3};                    ///< 连续失败次数阈值（标记不健康）
    milliseconds health_check_interval_{10000}; ///< 健康检查间隔
    milliseconds health_check_timeout_{3000};   ///< 健康检查超时

    health_check_cb health_checker_; ///< 自定义健康检查

public:
    load_balancer() = default;

    void add_backend(lb_backend backend) {
        lock<mutex> lk(mutex_);
        backends_.push_back(move(backend));
    }

    void remove_backend(const string& host, ports port) {
        lock<mutex> lk(mutex_);
        for (auto it = backends_.begin(); it != backends_.end(); ++it) {
            if (it->host == host && it->port == port) {
                backends_.erase(it);
                return;
            }
        }
    }

    void set_strategy(lb_strategy s) {
        lock<mutex> lk(mutex_);
        strategy_ = s;
    }

    void set_health_check(health_check_cb cb) {
        lock<mutex> lk(mutex_);
        health_checker_ = move(cb);
    }

    /// @brief 设置容忍的最大连续失败次数
    void set_max_failures(size_t max) { max_failures_ = max; }
    /// @brief 设置健康检查间隔
    void set_health_check_interval(milliseconds ms) { health_check_interval_ = ms; }
    /// @brief 设置健康检查超时
    void set_health_check_timeout(milliseconds ms) { health_check_timeout_ = ms; }

    /**
     * @brief 选择后端节点
     * @return 选中的后端，无可选用节点返回nullptr
     */
    lb_backend* select_backend();

    /**
     * @brief 释放后端节点（请求完成）
     * @param backend select_backend返回的节点指针
     */
    static void release_backend(lb_backend* backend);

    /**
     * @brief 标记后端请求失败
     */
    void mark_failure(lb_backend* backend);

    /**
     * @brief 运行健康检查（将所有标记为不健康但已恢复的节点恢复）
     */
    void run_health_checks();

    /**
     * @brief 获取后端主机列表快照
     */
    vector<string> backend_hosts() const {
        lock<mutex> lk(mutex_);
        vector<string> hosts;
        for (const auto& b: backends_) {
            hosts.push_back(b.host + ":" + to_string(b.port.value()));
        }
        return hosts;
    }

    size_t backend_count() const noexcept { return backends_.size(); }
    lb_strategy strategy() const noexcept { return strategy_; }

    /**
     * @brief 获取健康节点数
     */
    size_t healthy_count() const;

private:
    lb_backend* select_round_robin();
    lb_backend* select_least_connections();
    lb_backend* select_weighted();
    lb_backend* select_random();
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_LOAD_BALANCER_HPP__
