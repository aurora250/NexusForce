#ifndef NEFORCE_CORE_CONTAINER_LRU_CACHE_HPP__
#define NEFORCE_CORE_CONTAINER_LRU_CACHE_HPP__

/**
 * @file lru_cache.hpp
 * @brief LRU缓存实现
 *
 * 此文件提供了LRU（最近最少使用）缓存的实现，支持O(1)时间复杂度的插入、
 * 访问和删除操作。同时支持缓存项过期时间管理和条件删除。
 */

#include "NeForce/core/container/list.hpp"
#include "NeForce/core/container/unordered_map.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Cache 缓存
 * @brief 缓存容器实现
 * @{
 */

/**
 * @class lru_cache
 * @brief LRU缓存类模板
 * @tparam Key 键类型
 * @tparam Value 值类型
 *
 * 实现LRU（最近最少使用）缓存淘汰策略的容器。
 * 当缓存达到容量上限时，会淘汰最长时间未被访问的条目。
 *
 * 特性：
 * - O(1)时间复杂度的插入和访问
 * - 支持缓存项过期时间
 * - 支持条件删除
 *
 * @note 此类不是线程安全的，多线程环境下需要外部同步
 */
template <typename Key, typename Value>
class lru_cache {
public:
    using key_type        = Key;      ///< 键类型
    using value_type      = Value;    ///< 值类型
    using size_type       = size_t;   ///< 大小类型
    using clock           = steady_clock;      ///< 时钟类型
    using time_point      = clock::time_point; ///< 时间点类型
    using duration        = clock::duration;   ///< 持续时间类型

private:
    using list_type = list<pair<Key, Value>>;           ///< 底层列表类型
    using list_iterator = typename list_type::iterator; ///< 列表迭代器类型

    size_type capacity_;                                 ///< 缓存容量
    list_type list_;                                     ///< 双向链表，头部为最近使用，尾部为最久未使用
    unordered_map<Key, list_iterator> map_;              ///< 键到链表节点的映射
    unordered_map<Key, time_point> access_times_;        ///< 键到最后访问时间的映射
    
public:
    /**
     * @brief 构造函数
     * @param capacity 缓存容量
     * @throws value_exception 当capacity为0时抛出
     *
     * 创建一个指定容量的LRU缓存。
     */
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

    /**
     * @brief 插入或更新缓存项
     * @param key 键
     * @param value 值
     *
     * 如果键已存在，更新其值并将其移动到最近使用位置；
     * 如果键不存在且缓存已满，淘汰最久未使用的项后再插入新项；
     * 无论插入还是更新，都会更新该键的最后访问时间。
     */
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

    /**
     * @brief 获取缓存项
     * @param key 键
     * @return 包含值的optional，如果键不存在则返回none
     *
     * 访问缓存项会将其移动到最近使用位置，并更新最后访问时间。
     */
    NEFORCE_NODISCARD optional<Value> get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return none;
        }
        list_.splice(list_.begin(), list_, it->second);
        access_times_[key] = clock::now();
        return optional<Value>{it->second->second};
    }

    /**
     * @brief 查看缓存项（不更新访问状态）
     * @param key 键
     * @return 包含值的optional，如果键不存在则返回none
     *
     * 与get不同，此方法不会移动缓存项到最近使用位置，也不会更新访问时间。
     * 适用于仅需要查看而不改变缓存状态的场景。
     */
    NEFORCE_NODISCARD optional<Value> peek(const Key& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return none;
        }
        return optional<Value>{it->second->second};
    }

    /**
     * @brief 删除缓存项
     * @param key 键
     * @return 如果键存在并成功删除返回true，否则返回false
     */
    bool erase(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        list_.erase(it->second);
        map_.erase(it);
        return true;
    }

    /**
     * @brief 删除过期的缓存项
     * @param max_age 最大存活时间
     *
     * 遍历所有缓存项，删除最后访问时间超过max_age的项。
     * 适用于需要自动清理过期数据的场景。
     */
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

    /**
     * @brief 清空所有缓存项
     */
    void clear() {
        list_.clear();
        map_.clear();
    }

    /**
     * @brief 获取当前缓存大小
     * @return 缓存中的元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept {
        return list_.size();
    }

    /**
     * @brief 获取缓存容量
     * @return 缓存的最大容量
     */
    NEFORCE_NODISCARD size_type capacity() const noexcept {
        return capacity_;
    }

    /**
     * @brief 检查缓存是否包含指定键
     * @param key 键
     * @return 如果存在返回true，否则返回false
     */
    NEFORCE_NODISCARD bool contains(const Key& key) const noexcept {
        return map_.find(key) != map_.end();
    }

    /**
     * @brief 按条件删除缓存项
     * @tparam Predicate 谓词类型
     * @param pred 判断函数，返回true时删除对应项
     *
     * 遍历所有缓存项，删除满足谓词条件的项。
     */
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

/** @} */ // Cache

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_LRU_CACHE_HPP__
