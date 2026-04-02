#ifndef NEFORCE_DATABASE_DATABASE_POOL_HPP__
#define NEFORCE_DATABASE_DATABASE_POOL_HPP__
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API database_pool {
public:
    struct pool_config {
        size_t init_size = 5;
        size_t min_size = 5;
        size_t max_size = 64;
        seconds max_idle_time{60};
        milliseconds acquire_timeout{3000};
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

    db_config config_;
    pool_config pool_cfg_;
    unique_ptr<idb_factory> factory_;

    queue<connection_entry> idle_queue_;
    mutable mutex queue_mtx_;
    condition_variable cv_;

    atomic<size_t> total_count_{0};
    atomic<bool> running_{false};

    thread replenish_thread_;
    thread scanner_thread_;

    idb_connect* try_create_connect() noexcept;

    void return_connect(idb_connect* conn) noexcept;

    void replenish_task();
    void scanner_task();

    template <typename T> shared_ptr<T> acquire_impl();

public:
    database_pool(db_type type, const db_config& config) :
    database_pool(type, config, pool_config()) {}

    database_pool(db_type type, const db_config& config, const pool_config& pool_config);

    ~database_pool();

    database_pool(const database_pool&) = delete;
    database_pool& operator=(const database_pool&) = delete;
    database_pool(database_pool&&) = delete;
    database_pool& operator=(database_pool&&) = delete;

    shared_ptr<idb_connect> get_connect();
    shared_ptr<idb_tb_connect> get_tb_connect();
    shared_ptr<idb_kv_connect> get_kv_connect();

    size_t idle_count() const noexcept;
    size_t total_count() const noexcept;
    bool is_running() const noexcept { return running_.load(memory_order_acquire); }

    void stop();
};


template <typename T> shared_ptr<T> database_pool::acquire_impl() {
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

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DATABASE_POOL_HPP__
