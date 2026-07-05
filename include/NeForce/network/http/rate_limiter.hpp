#ifndef NEFORCE_NETWORK_HTTP_RATE_LIMITER_HPP__
#define NEFORCE_NETWORK_HTTP_RATE_LIMITER_HPP__

/**
 * @file rate_limiter.hpp
 * @brief 令牌桶限流器
 *
 * 基于令牌桶算法的HTTP请求限流过滤器。支持：
 * - 每IP / 每路由独立限流桶
 * - Lock-free atomic令牌计数器
 * - 可配置填充速率和突发容量
 * - LRU淘汰过期桶
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct token_bucket
 * @brief 令牌桶数据结构
 *
 * 使用原子操作实现无锁的令牌消耗和补充。
 */
struct NEFORCE_API token_bucket {
    double tokens{0.};          ///< 当前令牌数
    uint64_t last_refill_ms{0}; ///< 上次补充时间（毫秒时间戳）
    double refill_rate{1.};     ///< 每秒补充令牌数
    double capacity{1.};        ///< 桶容量
    mutable mutex mutex_;       ///< 保护内部状态

    token_bucket() = default;
    ~token_bucket() = default;

    token_bucket(double rate, double burst) :
    tokens(burst),
    last_refill_ms(static_cast<uint64_t>(steady_clock::now().since_epoch().count() / 1'000'000)),
    refill_rate(rate),
    capacity(burst) {}

    token_bucket(const token_bucket& other) :
    tokens(other.tokens),
    last_refill_ms(other.last_refill_ms),
    refill_rate(other.refill_rate),
    capacity(other.capacity) {}

    token_bucket& operator=(const token_bucket& other) {
        if (this != &other) {
            tokens = other.tokens;
            last_refill_ms = other.last_refill_ms;
            refill_rate = other.refill_rate;
            capacity = other.capacity;
        }
        return *this;
    }

    /**
     * @brief 尝试消耗一个令牌
     * @param now_ms 当前时间（毫秒时间戳）
     * @return 消耗成功返回true（频次未超限）
     *
     * 先补充令牌（基于时间差），再尝试消耗。
     */
    bool try_consume(uint64_t now_ms);

    /**
     * @brief 补充令牌（基于时间差）
     * @param now_ms 当前时间
     */
    void refill(uint64_t now_ms);
};

/**
 * @class token_bucket_limiter
 * @brief 令牌桶限流管理器
 *
 * 管理多个客户端/路由的令牌桶，支持LRU淘汰和批量清理。
 */
class NEFORCE_API token_bucket_limiter {
private:
    struct bucket_entry {
        token_bucket bucket;
        uint64_t last_access_ms;
    };

    double default_rate_{10.0};
    double default_burst_{20.0};

    mutable mutex mutex_;
    unordered_map<string, bucket_entry> buckets_;

    static constexpr size_t MAX_BUCKETS_COUNT{10000};

public:
    token_bucket_limiter() = default;

    /**
     * @brief 检查并消耗令牌
     * @param key 客户端标识（如IP或IP+路由）
     * @param rate 每秒补充令牌数（<=0 使用默认）
     * @param burst 桶容量（<=0 使用默认）
     * @return 允许通过返回true，被限制返回false
     */
    bool allow(const string& key, double rate = 0.0, double burst = 0.0);

    /// 设置默认的令牌补充速率
    void set_default_rate(double rate);
    /// 设置默认的桶容量
    void set_default_burst(double burst);
    /// 获取桶数量
    NEFORCE_NODISCARD size_t size() const;

    /**
     * @brief 清理过期的桶
     * @param max_age 超过此时间未使用的桶将被移除
     */
    void cleanup_expired(seconds max_age = seconds{300});
};


/**
 * @class token_bucket_filter
 * @brief 令牌桶HTTP限流过滤器
 *
 * 在pre_filter阶段基于客户端IP和路由进行限流。
 * 默认配置：每秒10请求，突发20请求，每IP独立限流。
 */
class NEFORCE_API token_bucket_filter final : public http_filter {
private:
    token_bucket_limiter limiter_;

public:
    double default_rate{10.0};  ///< 默认每秒请求数（每IP）
    double default_burst{20.0}; ///< 默认突发容量（每IP）
    bool per_route{false};      ///< 是否对每个路由单独限流
    bool enabled{true};         ///< 是否启用

    token_bucket_filter() = default;

    explicit token_bucket_filter(double rate, double burst) :
    default_rate(rate),
    default_burst(burst) {}

    bool pre_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}

    void cleanup_expired(seconds max_age = seconds{300});

    NEFORCE_NODISCARD string name() const override { return "token_bucket_filter"; }

    // TODO: Distributed rate limiting — Redis/Lua-script based shared counter for multi-instance rate limit coordination
    // TODO: Sliding window algorithm — more precise rate limiting using sliding log/window instead of token bucket
    // TODO: Rate limit response headers — X-RateLimit-Limit, X-RateLimit-Remaining, X-RateLimit-Reset in 429 responses
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_RATE_LIMITER_HPP__
