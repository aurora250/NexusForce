#ifndef NEFORCE_NETWORK_SSL_SSL_SESSION_CACHE_HPP__
#define NEFORCE_NETWORK_SSL_SSL_SESSION_CACHE_HPP__

/**
 * @file ssl_session_cache.hpp
 * @brief TLS 会话票据缓存
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/flat_unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/network/util/ports.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class ssl_session_cache
 * @brief 跨连接复用 TLS 会话
 *
 * 保存客户端握手得到的 SSL_SESSION，使后续连接可以携带 session ticket
 * 进行缩写握手（TLS 1.2 session resumption / TLS 1.3 PSK resumption）。
 *
 * TLS 1.3 的 session ticket 在服务端默认按单次使用处理，多个并发连接不能共享同一张票据。
 */
struct ssl_session_cache_stats {
    uint64_t hits{0};    ///< 成功取出的票据数
    uint64_t misses{0};  ///< 未命中次数
    uint64_t stored{0};  ///< 写入的票据数
    uint64_t evicted{0}; ///< 因达到端点上限而淘汰的票据数
};

/**
 * @brief TLS 会话票据缓存统计
 */
class NEFORCE_API ssl_session_cache {
public:
    ssl_session_cache() = default;
    ~ssl_session_cache();

    ssl_session_cache(const ssl_session_cache&) = delete;
    ssl_session_cache& operator=(const ssl_session_cache&) = delete;
    ssl_session_cache(ssl_session_cache&&) = delete;
    ssl_session_cache& operator=(ssl_session_cache&&) = delete;

    /**
     * @brief 取出并移除一张会话的独立引用
     * @param host 主机名
     * @param port 端口
     * @return SSL_SESSION 指针，未命中返回nullptr；调用方需 free_session
     */
    NEFORCE_NODISCARD void* acquire(const string& host, ports port);

    /**
     * @brief 保存会话（接管引用）
     * @param host 主机名
     * @param port 端口
     * @param session SSL_SESSION 指针
     */
    void store(const string& host, ports port, void* session);

    /**
     * @brief 获取缓存统计
     * @return 当前累计命中、未命中、写入与淘汰计数
     */
    NEFORCE_NODISCARD ssl_session_cache_stats stats() const noexcept;

    /**
     * @brief 清空所有缓存会话
     */
    void clear();

private:
    static constexpr size_t max_sessions_per_endpoint = 32;

    mutable mutex mutex_;
    flat_unordered_map<string, vector<void*>> sessions_;
    mutable atomic<uint64_t> hits_{0};
    mutable atomic<uint64_t> misses_{0};
    mutable atomic<uint64_t> stored_{0};
    mutable atomic<uint64_t> evicted_{0};
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_SSL_SSL_SESSION_CACHE_HPP__
