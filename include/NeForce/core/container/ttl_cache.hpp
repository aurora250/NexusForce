#ifndef NEFORCE_CORE_CONTAINER_TTL_CACHE_HPP__
#define NEFORCE_CORE_CONTAINER_TTL_CACHE_HPP__

/**
 * @file ttl_cache.hpp
 * @brief TTL缓存实现
 *
 * 此文件提供了TTL（生存时间）缓存的实现，基于LRU缓存扩展而来。
 * 支持缓存项的自动过期、过期清理线程、多种刷新策略等功能。
 * 适用于需要自动过期功能的缓存场景。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/lru_cache.hpp"
#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Cache 缓存
 * @brief 缓存容器实现
 * @{
 */

/**
 * @class ttl_cache
 * @brief TTL缓存类模板
 * @tparam Key 键类型
 * @tparam Value 值类型
 *
 * 实现带有生存时间（TTL）的缓存容器，基于LRU缓存实现。
 * 每个缓存项都有独立的过期时间，过期后自动失效。
 * 支持可选的自动清理线程，定时清理过期项。
 *
 * 特性：
 * - 自动过期：每个缓存项有独立的TTL
 * - 过期清理：支持后台线程自动清理过期项
 * - 刷新策略：支持多种过期时间刷新策略
 * - 继承LRU特性：满容量时淘汰最久未使用的项
 *
 * @note 此类不是线程安全的，多线程环境下需要外部同步
 */
template <typename Key, typename Value>
class ttl_cache {
public:
    using clock = steady_clock;           ///< 时钟类型
    using time_point = clock::time_point; ///< 时间点类型
    using duration = clock::duration;     ///< 持续时间类型
    using size_type = size_t;             ///< 大小类型

    /**
     * @enum refresh_policy
     * @brief 过期时间刷新策略
     */
    enum class refresh_policy : uint8_t {
        never,         ///< 不刷新，保持原始过期时间
        on_access,     ///< 访问时刷新，每次访问都重置过期时间
        sliding_window ///< 滑动窗口，每次访问延长TTL
    };

private:
    /**
     * @struct entry
     * @brief 缓存项内部结构
     *
     * 存储缓存的值及其过期时间。
     */
    struct entry {
        Value value;       ///< 缓存的值
        time_point expiry; ///< 过期时间点
    };

    lru_cache<Key, entry> cache_; ///< 底层LRU缓存
    duration default_ttl_;        ///< 默认生存时间

    atomic<bool> running_{false};                          ///< 清理线程运行标志
    refresh_policy refresh_policy_{refresh_policy::never}; ///< 刷新策略
    duration cleanup_interval_{seconds(1)};                ///< 清理间隔
    thread cleanup_thread_;                                ///< 后台清理线程

public:
    /**
     * @brief 构造函数
     * @param capacity 缓存容量
     * @param default_ttl 默认生存时间，默认为60秒
     *
     * 创建一个指定容量和默认TTL的缓存。
     */
    explicit ttl_cache(size_type capacity, duration default_ttl = seconds(60)) :
    cache_(capacity),
    default_ttl_(default_ttl) {}

    /**
     * @brief 析构函数
     *
     * 自动停止并等待清理线程结束。
     */
    ~ttl_cache() { disable_cleanup(); }

    /**
     * @brief 启用后台清理线程
     * @param interval 清理间隔，默认为1秒
     *
     * 启动后台线程定期清理过期的缓存项。
     * 如果清理线程已运行，此操作无效。
     */
    void enable_cleanup(duration interval = seconds(1)) {
        if (running_) {
            return;
        }

        cleanup_interval_ = interval;
        running_ = true;
        cleanup_thread_ = thread([this] {
            while (running_) {
                this_thread::sleep_for(cleanup_interval_);
                cleanup();
            }
        });
    }

    /**
     * @brief 禁用后台清理线程
     *
     * 停止后台清理线程并等待其结束。
     * 不会清空已有缓存项，只是停止自动清理。
     */
    void disable_cleanup() {
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
    }

    /**
     * @brief 设置刷新策略
     * @param policy 新的刷新策略
     *
     * 决定访问缓存项时如何影响其过期时间。
     */
    void set_refresh_policy(refresh_policy policy) { refresh_policy_ = policy; }

    /**
     * @brief 插入缓存项（使用默认TTL）
     * @param key 键
     * @param value 值
     *
     * 使用默认TTL插入或更新缓存项。
     */
    void put(const Key& key, const Value& value) {
        time_point expiry = clock::now() + default_ttl_;
        cache_.put(key, entry{value, expiry});
    }

    /**
     * @brief 插入缓存项（指定TTL）
     * @param key 键
     * @param value 值
     * @param ttl 生存时间
     *
     * 使用指定的TTL插入或更新缓存项。
     */
    void put(const Key& key, const Value& value, duration ttl) {
        time_point expiry = clock::now() + ttl;
        cache_.put(key, entry{value, expiry});
    }

    /**
     * @brief 获取缓存项
     * @param key 键
     * @return 包含值的optional，如果键不存在或已过期返回none
     *
     * 根据刷新策略决定是否更新过期时间。
     */
    NEFORCE_NODISCARD optional<Value> get(const Key& key) {
        auto opt_entry = cache_.get(key);
        if (!opt_entry) {
            return none;
        }

        const entry& e = *opt_entry;
        if (e.expiry < clock::now()) {
            cache_.erase(key);
            return none;
        }

        if (refresh_policy_ == refresh_policy::on_access || refresh_policy_ == refresh_policy::sliding_window) {
            entry updated_entry = e;
            updated_entry.expiry = clock::now() + default_ttl_;
            cache_.put(key, updated_entry);
        }

        return optional<Value>{e.value};
    }

    /**
     * @brief 检查缓存是否包含指定键
     * @param key 键
     * @return 如果键存在且未过期返回true，否则返回false
     *
     * 此方法会检查过期时间，如果键已过期则自动删除。
     */
    NEFORCE_NODISCARD bool contains(const Key& key) {
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

    /**
     * @brief 删除缓存项
     * @param key 键
     * @return 如果键存在并成功删除返回true，否则返回false
     */
    bool erase(const Key& key) { return cache_.erase(key); }

    /**
     * @brief 清空所有缓存项
     */
    void clear() { cache_.clear(); }

    /**
     * @brief 获取当前缓存大小
     * @return 缓存中的元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return cache_.size(); }

    /**
     * @brief 获取缓存容量
     * @return 缓存的最大容量
     */
    NEFORCE_NODISCARD size_type capacity() const noexcept { return cache_.capacity(); }

    /**
     * @brief 手动清理过期项
     *
     * 遍历所有缓存项，删除已过期的项。
     * 此方法在启用后台清理线程时会自动定期调用。
     */
    void cleanup() {
        auto now = clock::now();
        cache_.remove_if([now](const auto& pair) { return pair.second.expiry < now; });
    }
};

/** @} */ // Cache

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_TTL_CACHE_HPP__
