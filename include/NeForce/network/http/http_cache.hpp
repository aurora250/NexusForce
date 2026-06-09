#ifndef NEFORCE_NETWORK_HTTP_HTTP_CACHE_HPP__
#define NEFORCE_NETWORK_HTTP_HTTP_CACHE_HPP__

/**
 * @file http_cache.hpp
 * @brief HTTP 响应缓存与缓存过滤器
 */

#include "NeForce/core/async/shared_mutex.hpp"
#include "NeForce/network/http/http_filter.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @struct cached_response
 * @brief 缓存的HTTP响应
 */
struct cached_response {
    http_status status{http_status::S2_OK};
    string status_message;
    unordered_map<string, string> headers;
    string body;
    string etag;
    datetime cached_at;
    seconds max_age{60};
};

/**
 * @class response_cache
 * @brief HTTP响应缓存
 *
 * 基于URL的响应缓存，支持ETag和Cache-Control。
 * 线程安全，支持LRU淘汰。
 */
class NEFORCE_API response_cache {
public:
    seconds default_max_age{60};   ///< 默认缓存时间
    size_t max_entries{10000};     ///< 最大缓存条目数
    byte_size max_body_size{1_MB}; ///< 最大缓存响应体大小

    response_cache() = default;

    /**
     * @brief 获取缓存的响应
     * @param key 缓存键（通常为 method+url）
     * @return 缓存的响应，不存在返回none
     */
    optional<cached_response> get(const string& key);

    /**
     * @brief 存入缓存
     * @param key 缓存键
     * @param response 响应对象
     */
    void put(const string& key, const http_response& response, seconds max_age = -1_s);

    /**
     * @brief 删除缓存条目
     */
    void remove(const string& key);

    /**
     * @brief 清理过期条目
     */
    void cleanup();

    /**
     * @brief 清空所有缓存
     */
    void clear();

    /**
     * @brief 缓存条目数
     */
    size_t size() const {
        shared_lock<shared_mutex> lk(mutex_);
        return entries_.size();
    }

    /**
     * @brief 生成ETag值
     */
    static string generate_etag(string_view body);

    /**
     * @brief 构建缓存键
     */
    static string build_key(const http_method& method, string_view url);

private:
    struct entry {
        cached_response response;
        uint64_t last_access_ms;
    };
    mutable shared_mutex mutex_;
    unordered_map<string, entry> entries_;

    void evict_lru();
};

/**
 * @class cache_filter
 * @brief HTTP缓存过滤器
 *
 * 自动缓存GET请求的响应，处理If-None-Match和If-Modified-Since。
 */
class NEFORCE_API cache_filter final : public http_filter {
public:
    bool enabled{true};
    bool cache_only_get{true};

    cache_filter() = default;
    explicit cache_filter(response_cache* cache) :
    cache_(cache) {}

    void set_cache(response_cache* cache) { cache_ = cache; }

    bool pre_filter(http_request& request, http_response& response) override;
    void post_filter(http_request& request, http_response& response) override;
    void do_filter(http_request& request, http_response& response) override {}
    NEFORCE_NODISCARD string name() const override { return "cache_filter"; }

private:
    response_cache* cache_{nullptr};
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_HTTP_CACHE_HPP__
