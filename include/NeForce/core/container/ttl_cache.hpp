#ifndef NEFORCE_CORE_CONTAINER_TTL_CACHE_HPP__
#define NEFORCE_CORE_CONTAINER_TTL_CACHE_HPP__
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/lru_cache.hpp"
#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename Key, typename Value>
class ttl_cache {
public:
    using clock           = steady_clock;
    using time_point      = clock::time_point;
    using duration        = clock::duration;
    using size_type       = size_t;

    enum class refresh_policy : uint8_t {
        never,              // 不刷新
        on_access,          // 访问时刷新
        sliding_window      // 滑动窗口，每次访问延长TTL
    };

private:
    struct entry {
        Value value;
        time_point expiry;
    };

    lru_cache<Key, entry> cache_;
    duration default_ttl_;

    atomic<bool> running_{false};
    refresh_policy refresh_policy_{refresh_policy::never};
    duration cleanup_interval_{seconds(1)};
    thread cleanup_thread_;

public:
    explicit ttl_cache(size_type capacity, duration default_ttl = seconds(60))
    : cache_(capacity), default_ttl_(default_ttl) {}

    ~ttl_cache() {
        disable_cleanup();
    }

    void enable_cleanup(duration interval = seconds(1)) {
        if (running_) return;

        cleanup_interval_ = interval;
        running_ = true;
        cleanup_thread_ = thread([this] {
            while (running_) {
                this_thread::sleep_for(cleanup_interval_);
                cleanup();
            }
        });
    }

    void disable_cleanup() {
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
    }

    void set_refresh_policy(refresh_policy policy) {
        refresh_policy_ = policy;
    }

    void put(const Key& key, const Value& value) {
        time_point expiry = clock::now() + default_ttl_;
        cache_.put(key, entry{value, expiry});
    }

    void put(const Key& key, const Value& value, duration ttl) {
        time_point expiry = clock::now() + ttl;
        cache_.put(key, entry{value, expiry});
    }

    optional<Value> get(const Key& key) {
        auto opt_entry = cache_.get(key);
        if (!opt_entry) {
            return none;
        }

        const entry& e = *opt_entry;
        if (e.expiry < clock::now()) {
            cache_.erase(key);
            return none;
        }

        if (refresh_policy_ == refresh_policy::on_access ||
            refresh_policy_ == refresh_policy::sliding_window) {
            entry updated_entry = e;
            updated_entry.expiry = clock::now() + default_ttl_;
            cache_.put(key, updated_entry);
        }

        return optional<Value>{e.value};
    }

    bool contains(const Key& key) {
        auto opt_entry = cache_.peek(key);
        if (!opt_entry) {
            return false;
        }
        const entry& e = *opt_entry;
        if (e.expiry < clock::now()) {
            cache_.erase(key);
            return false;
        }
        return true;
    }

    bool erase(const Key& key) {
        return cache_.erase(key);
    }

    void clear() {
        cache_.clear();
    }

    size_type size() const noexcept {
        return cache_.size();
    }

    size_type capacity() const noexcept {
        return cache_.capacity();
    }

    void cleanup() {
        auto now = clock::now();
        cache_.remove_if([now](const auto& pair) {
            return pair.second.expiry < now;
        });
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_TTL_CACHE_HPP__
