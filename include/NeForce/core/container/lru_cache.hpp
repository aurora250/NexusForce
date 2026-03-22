#ifndef NEFORCE_CORE_CONTAINER_LRU_CACHE_HPP__
#define NEFORCE_CORE_CONTAINER_LRU_CACHE_HPP__
#include "NeForce/core/container/list.hpp"
#include "NeForce/core/container/unordered_map.hpp"
NEFORCE_BEGIN_NAMESPACE__

template <typename Key, typename Value>
class lru_cache {
public:
    using key_type        = Key;
    using value_type      = Value;
    using size_type       = size_t;
    using clock           = steady_clock;
    using time_point      = clock::time_point;
    using duration        = clock::duration;

private:
    using list_type = list<pair<Key, Value>>;
    using list_iterator = typename list_type::iterator;

    size_type capacity_;
    list_type list_;
    unordered_map<Key, list_iterator> map_;
    unordered_map<Key, time_point> access_times_;
    
public:
    explicit lru_cache(size_type capacity)
    : capacity_(capacity) {
        if (capacity_ == 0) {
            NEFORCE_THROW_EXCEPTION(value_exception("lru_cache capacity must be positive"));
        }
    }

    lru_cache(const lru_cache&) = delete;
    lru_cache& operator=(const lru_cache&) = delete;
    lru_cache(lru_cache&&) = default;
    lru_cache& operator =(lru_cache&&) = default;

    void put(const Key& key, const Value& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            list_.splice(list_.begin(), list_, it->second);
        } else {
            if (list_.size() >= capacity_) {
                auto last = --list_.end();
                map_.erase(last->first);
                access_times_.erase(last->first);
                list_.pop_back();
            }
            list_.emplace_front(key, value);
            map_[key] = list_.begin();
        }
        access_times_[key] = clock::now();
    }

    optional<Value> get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return none;
        }
        list_.splice(list_.begin(), list_, it->second);
        access_times_[key] = clock::now();
        return optional<Value>{it->second->second};
    }

    optional<Value> peek(const Key& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return none;
        }
        return optional<Value>{it->second->second};
    }

    bool erase(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        list_.erase(it->second);
        map_.erase(it);
        return true;
    }

    void remove_expired(duration max_age) {
        auto now = clock::now();
        auto it = access_times_.begin();
        while (it != access_times_.end()) {
            if (now - it->second > max_age) {
                erase(it->first);
                it = access_times_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear() {
        list_.clear();
        map_.clear();
    }

    size_type size() const noexcept {
        return list_.size();
    }

    size_type capacity() const noexcept {
        return capacity_;
    }

    bool contains(const Key& key) const noexcept {
        return map_.find(key) != map_.end();
    }

    template <typename Predicate>
    void remove_if(Predicate pred) {
        for (auto it = list_.begin(); it != list_.end(); ) {
            if (pred(*it)) {
                map_.erase(it->first);
                it = list_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_LRU_CACHE_HPP__
