#ifndef NEFORCE_NETWORK_HTTP_SESSION_STORE_REDIS_HPP__
#define NEFORCE_NETWORK_HTTP_SESSION_STORE_REDIS_HPP__

/**
 * @file session_store_redis.hpp
 * @brief Redis会话存储后端
 */

#include "NeForce/network/http/session_store.hpp"
#ifdef NEFORCE_SUPPORT_HIREDIS
#    include "NeForce/db/redis/redis_connect.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class redis_session_store
 * @brief Redis后端会话存储
 *
 * 每个session以 "session:<id>" 为键存储在Redis中，
 * 使用 SETEX 设置 TTL = session.max_age 秒。
 * 序列化格式为简单的 key=value&key=value 字符串。
 */
class redis_session_store final : public session_store {
public:
    explicit redis_session_store(redis_connect& conn) :
    conn_(&conn) {}

    optional<http_session> load(const string& id) override;
    void save(const http_session& session) override;
    void remove(const string& id) override { conn_->del(prefix_ + id); }
    NEFORCE_NODISCARD bool exists(const string& id) const override { return conn_->exists(prefix_ + id); }
    void cleanup() override {}
    NEFORCE_NODISCARD size_t count() const override { return 0; }

private:
    redis_connect* conn_;
    string prefix_{"session:"};

    // TODO: Redis Cluster/Sentinel support — handle MOVED/ASK redirection and automatic failover for HA deployments
    // TODO: Improved serialization — use JSON or MessagePack instead of key=value& pairs to handle special characters
    // TODO: Session count implementation — use SCAN/KEYS with prefix to return accurate session count
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_NETWORK_HTTP_SESSION_STORE_REDIS_HPP__
