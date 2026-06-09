#ifndef NEFORCE_NETWORK_HTTP_SESSION_STORE_HPP__
#define NEFORCE_NETWORK_HTTP_SESSION_STORE_HPP__

/**
 * @file session_store.hpp
 * @brief 可插拔会话存储抽象层
 */

#include "NeForce/core/async/shared_mutex.hpp"
#include "NeForce/core/utility/optional.hpp"
#include "NeForce/network/http/http_session.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

/**
 * @addtogroup HTTP HTTP
 * @{
 */

/**
 * @class session_store
 * @brief 会话存储抽象接口
 *
 * 所有会话存储后端必须实现此接口。
 */
class session_store {
public:
    virtual ~session_store() = default;

    virtual optional<http_session> load(const string& id) = 0;
    virtual void save(const http_session& session) = 0;
    virtual void remove(const string& id) = 0;
    NEFORCE_NODISCARD virtual bool exists(const string& id) const = 0;
    virtual void cleanup() = 0;
    NEFORCE_NODISCARD virtual size_t count() const = 0;
};

/**
 * @class memory_session_store
 * @brief 基于内存的会话存储实现
 */
class memory_session_store final : public session_store {
private:
    mutable shared_mutex mutex_;
    unordered_map<string, http_session> sessions_;

public:
    memory_session_store() = default;

    optional<http_session> load(const string& id) override {
        shared_lock<shared_mutex> lk(mutex_);
        auto it = sessions_.find(id);
        if (it != sessions_.end() && it->second.is_valid()) {
            return optional<http_session>{it->second};
        }
        return none;
    }

    void save(const http_session& session) override {
        lock<shared_mutex> lk(mutex_);
        sessions_[session.id] = session;
    }

    void remove(const string& id) override {
        lock<shared_mutex> lk(mutex_);
        sessions_.erase(id);
    }

    bool exists(const string& id) const override {
        shared_lock<shared_mutex> lk(mutex_);
        auto it = sessions_.find(id);
        return it != sessions_.end() && it->second.is_valid();
    }

    void cleanup() override {
        lock<shared_mutex> lk(mutex_);
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (!it->second.is_valid() || it->second.expired()) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }

    size_t count() const override {
        shared_lock<shared_mutex> lk(mutex_);
        return sessions_.size();
    }
};

/** @} */ // HTTP

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_NETWORK_HTTP_SESSION_STORE_HPP__
