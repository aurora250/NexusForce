#ifndef NEFORCE_DATABASE_DATABASE_POOL_HPP__
#define NEFORCE_DATABASE_DATABASE_POOL_HPP__

/**
 * @file database_pool.hpp
 * @brief 数据库连接池
 *
 * 此文件提供了数据库连接池的实现，用于管理数据库连接的创建、复用和回收。
 * 支持连接池大小动态调整、空闲连接回收、连接健康检查等功能。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Database 数据库
 * @brief 数据库相关功能
 * @{
 */

/**
 * @class database_pool
 * @brief 数据库连接池类
 *
 * 管理数据库连接的生命周期，提供连接的获取和归还功能。
 * 内部维护一个空闲连接队列，支持多线程安全访问。
 *
 * 主要特性：
 * - 连接复用，减少连接创建开销
 * - 最小/最大连接数配置
 * - 空闲连接超时回收
 * - 连接健康检查
 * - 自动补充连接
 * - 获取连接超时控制
 * - 支持关系型数据库和键值存储
 */
class NEFORCE_API database_pool {
public:
    /**
     * @struct pool_config
     * @brief 连接池配置
     */
    struct pool_config {
        size_t init_size = 5;               ///< 初始连接数
        size_t min_size = 5;                ///< 最小连接数
        size_t max_size = 64;               ///< 最大连接数
        seconds max_idle_time{60};          ///< 连接最大空闲时间
        milliseconds acquire_timeout{3000}; ///< 获取连接超时时间
    };

private:
    struct connection_entry {
        idb_connect* conn = nullptr;
        milliseconds idle_at{0};

        connection_entry() noexcept = default;

        explicit connection_entry(idb_connect* c) :
        conn(c),
        idle_at(current_ms()) {}

        milliseconds idle_duration() const noexcept { return current_ms() - idle_at; }

        static milliseconds current_ms() noexcept { return time_cast<milliseconds>(steady_clock::now().since_epoch()); }
    };

    db_config config_;                ///< 数据库连接配置
    pool_config pool_cfg_;            ///< 连接池配置
    unique_ptr<idb_factory> factory_; ///< 数据库连接工厂

    queue<connection_entry> idle_queue_; ///< 空闲连接队列
    mutable mutex queue_mtx_;            ///< 保护空闲队列的互斥锁
    condition_variable cv_;              ///< 条件变量（用于等待连接）

    atomic<size_t> total_count_{0}; ///< 总连接数（包括使用中和空闲）
    atomic<bool> running_{false};   ///< 连接池运行标志

    thread replenish_thread_; ///< 连接补充线程
    thread scanner_thread_;   ///< 空闲连接回收线程

    idb_connect* try_create_connect() noexcept;

    void return_connect(idb_connect* conn);

    void replenish_task();
    void scanner_task();

    template <typename T>
    shared_ptr<T> acquire_impl();

public:
    /**
     * @brief 构造函数
     * @param type 数据库类型
     * @param config 数据库连接配置
     */
    database_pool(db_type type, const db_config& config) :
    database_pool(type, config, pool_config()) {}

    /**
     * @brief 构造函数
     * @param type 数据库类型
     * @param config 数据库连接配置
     * @param pool_config 连接池配置
     * @throws database_exception 初始化连接失败时抛出
     */
    database_pool(db_type type, const db_config& config, const pool_config& pool_config);

    /**
     * @brief 析构函数
     *
     * 停止所有后台线程，释放所有连接资源。
     */
    ~database_pool();

    database_pool(const database_pool&) = delete;
    database_pool& operator=(const database_pool&) = delete;
    database_pool(database_pool&&) = delete;
    database_pool& operator=(database_pool&&) = delete;

    /**
     * @brief 获取通用数据库连接
     * @return 共享指针管理的连接对象
     *
     * 返回基类idb_connect指针，适用于不需要特定类型操作的场景。
     */
    shared_ptr<idb_connect> get_connect();

    /**
     * @brief 获取关系型数据库连接
     * @return 共享指针管理的连接对象
     *
     * 返回idb_tb_connect指针，支持SQL查询操作。
     */
    shared_ptr<idb_tb_connect> get_tb_connect();

    /**
     * @brief 获取键值存储连接
     * @return 共享指针管理的连接对象
     *
     * 返回idb_kv_connect指针，支持键值操作。
     */
    shared_ptr<idb_kv_connect> get_kv_connect();

    /**
     * @brief 获取空闲连接数
     * @return 当前空闲队列中的连接数量
     */
    size_t idle_count() const noexcept;

    /**
     * @brief 获取总连接数
     * @return 当前总连接数（包括使用中和空闲）
     */
    size_t total_count() const noexcept;

    /**
     * @brief 检查连接池是否正在运行
     * @return 运行中返回true
     */
    bool is_running() const noexcept { return running_.load(memory_order_acquire); }

    /**
     * @brief 停止连接池
     *
     * 停止所有后台线程，等待当前使用中的连接归还后释放资源。
     */
    void stop();
};

/// @cond
template <typename T>
shared_ptr<T> database_pool::acquire_impl() {
    unique_lock<mutex> lk(queue_mtx_);

    const bool got = cv_.wait_for(lk, pool_cfg_.acquire_timeout,
                                  [this] { return !idle_queue_.empty() || !running_.load(memory_order_relaxed); });

    if (!running_.load(memory_order_relaxed)) {
        return nullptr;
    }

    if (!got || idle_queue_.empty()) {
        const size_t cur = total_count_.load(memory_order_relaxed);
        if (cur >= pool_cfg_.max_size) {
            return nullptr;
        }
        idb_connect* raw = try_create_connect();
        if (raw == nullptr) {
            return nullptr;
        }

        total_count_.fetch_add(1, memory_order_relaxed);
        T* typed = dynamic_cast<T*>(raw);
        if (typed == nullptr) {
            delete raw;
            total_count_.fetch_sub(1, memory_order_relaxed);
            return nullptr;
        }
        return shared_ptr<T>(typed, [this](T* p) { this->return_connect(p); });
    }

    const connection_entry entry = idle_queue_.front();
    idle_queue_.pop();
    lk.unlock_quiet();

    idb_connect* raw = entry.conn;

    if (!raw->is_valid()) {
        if (!raw->reconnect(config_)) {
            delete raw;
            total_count_.fetch_sub(1, memory_order_relaxed);
            cv_.notify_one();
            return nullptr;
        }
    }

    T* typed = dynamic_cast<T*>(raw);
    if (typed == nullptr) {
        this->return_connect(raw);
        return nullptr;
    }

    return shared_ptr<T>(typed, [this](T* p) { this->return_connect(p); });
}
/// @endcond

/** @} */ // Database

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DATABASE_POOL_HPP__
