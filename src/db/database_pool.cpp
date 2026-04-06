#include <NeForce/core/system/console.hpp>
#include <NeForce/db/database_pool.hpp>
#ifdef NEFORCE_SUPPORT_DB
#    include <NeForce/db/mysql/mysql_connect.hpp>
#    include <NeForce/db/pgsql/pgsql_connect.hpp>
#    include <NeForce/db/redis/redis_connect.hpp>
#    include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif
NEFORCE_BEGIN_NAMESPACE__

database_pool::database_pool(const db_type type, const db_config& config, const pool_config& pool_config) :
config_(config),
pool_cfg_(pool_config) {
    if (pool_cfg_.min_size == 0) {
        pool_cfg_.min_size = 1;
    }
    if (pool_cfg_.max_size < pool_cfg_.min_size) {
        pool_cfg_.max_size = pool_cfg_.min_size;
    }
    if (pool_cfg_.init_size < pool_cfg_.min_size) {
        pool_cfg_.init_size = pool_cfg_.min_size;
    }
    if (pool_cfg_.init_size > pool_cfg_.max_size) {
        pool_cfg_.init_size = pool_cfg_.max_size;
    }

    switch (type) {
#ifdef NEFORCE_SUPPORT_MYSQL
        case db_type::MYSQL:
            factory_ = make_unique<mysql_factory>(config);
            break;
#endif
#ifdef NEFORCE_SUPPORT_SQLITE3
        case db_type::SQLITE3:
            factory_ = make_unique<sqlite_factory>(config);
            break;
#endif
#ifdef NEFORCE_SUPPORT_HIREDIS
        case db_type::REDIS:
            factory_ = make_unique<redis_factory>(config);
            break;
#endif
#ifdef NEFORCE_SUPPORT_POSTGRESQL
        case db_type::POSTGRESQL:
            factory_ = make_unique<pgsql_factory>(config);
            break;
#endif
        default:
            NEFORCE_THROW_EXCEPTION(value_exception("Unsupported database type"));
            unreachable();
    }

    size_t created = 0;
    size_t consecutive_failures = 0;
    constexpr size_t max_consecutive_failures = 5;

    while (created < pool_cfg_.init_size) {
        idb_connect* conn = try_create_connect();
        if (conn != nullptr) {
            idle_queue_.emplace(conn);
            total_count_.fetch_add(1, memory_order_relaxed);
            ++created;
            consecutive_failures = 0;
        } else {
            ++consecutive_failures;
            printcfln(color::yellow(), "[database_pool] Connection attempt failed ({}/{})", consecutive_failures,
                      max_consecutive_failures);

            if (consecutive_failures >= max_consecutive_failures) {
                printcfln(color::red(),
                          "[database_pool] Unable to establish initial connections, "
                          "created {}/{}",
                          created, pool_cfg_.init_size);
                if (created == 0) {
                    NEFORCE_THROW_EXCEPTION(database_exception("Failed to create any initial database connection"));
                }
                break;
            }
        }
    }

    running_.store(true, memory_order_release);
    replenish_thread_ = thread([this] { replenish_task(); });
    scanner_thread_ = thread([this] { scanner_task(); });
}

database_pool::~database_pool() { stop(); }

shared_ptr<idb_connect> database_pool::get_connect() { return acquire_impl<idb_connect>(); }

shared_ptr<idb_tb_connect> database_pool::get_tb_connect() { return acquire_impl<idb_tb_connect>(); }

shared_ptr<idb_kv_connect> database_pool::get_kv_connect() { return acquire_impl<idb_kv_connect>(); }

size_t database_pool::idle_count() const noexcept {
    unique_lock<mutex> lk(queue_mtx_);
    return idle_queue_.size();
}

size_t database_pool::total_count() const noexcept { return total_count_.load(memory_order_acquire); }

void database_pool::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false, memory_order_acq_rel, memory_order_acquire)) {
        return;
    }

    cv_.notify_all();

    if (replenish_thread_.joinable()) {
        replenish_thread_.join();
    }
    if (scanner_thread_.joinable()) {
        scanner_thread_.join();
    }

    unique_lock<mutex> lk(queue_mtx_);
    while (!idle_queue_.empty()) {
        delete idle_queue_.front().conn;
        idle_queue_.pop();
    }
    total_count_.store(0, memory_order_release);
}

idb_connect* database_pool::try_create_connect() noexcept {
    try {
        auto* conn = factory_->create_connect();
        if (conn != nullptr) {
            return conn;
        }
        delete conn;
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

void database_pool::return_connect(idb_connect* conn) {
    if (conn == nullptr) {
        return;
    }

    if (!conn->is_valid()) {
        delete conn;
        total_count_.fetch_sub(1, memory_order_relaxed);
        cv_.notify_one();
        return;
    }

    unique_lock<mutex> lk(queue_mtx_);

    if (idle_queue_.size() >= pool_cfg_.max_size) {
        lk.unlock_quiet();
        delete conn;
        total_count_.fetch_sub(1, memory_order_relaxed);
        return;
    }

    idle_queue_.emplace(conn);
    lk.unlock_quiet();

    cv_.notify_one();
}

void database_pool::replenish_task() {
    while (running_.load(memory_order_acquire)) {
        unique_lock<mutex> lk(queue_mtx_);

        cv_.wait(lk,
                 [this] { return !running_.load(memory_order_relaxed) || idle_queue_.size() < pool_cfg_.min_size; });

        if (!running_.load(memory_order_relaxed)) {
            break;
        }

        const size_t cur_total = total_count_.load(memory_order_relaxed);
        const size_t cur_idle = idle_queue_.size();
        const size_t need = pool_cfg_.min_size - cur_idle;
        const size_t can_add = cur_total < pool_cfg_.max_size ? pool_cfg_.max_size - cur_total : 0;
        const size_t to_create = min(need, can_add);

        lk.unlock_quiet();

        for (size_t i = 0; i < to_create; ++i) {
            if (!running_.load(memory_order_relaxed)) {
                return;
            }

            idb_connect* conn = try_create_connect();
            if (conn == nullptr) {
                break;
            }

            total_count_.fetch_add(1, memory_order_relaxed);

            unique_lock<mutex> lk2(queue_mtx_);
            idle_queue_.emplace(conn);
            lk2.unlock_quiet();

            cv_.notify_one();
        }
    }
}

void database_pool::scanner_task() {
    while (running_.load(memory_order_acquire)) {
        {
            unique_lock<mutex> lk(queue_mtx_);
            cv_.wait_for(lk, pool_cfg_.max_idle_time / 2, [this] { return !running_.load(memory_order_relaxed); });
        }

        if (!running_.load(memory_order_relaxed)) {
            break;
        }

        unique_lock<mutex> lk(queue_mtx_);

        size_t removed = 0;
        while (idle_queue_.size() > pool_cfg_.min_size) {
            const connection_entry& front = idle_queue_.front();

            const bool timed_out = front.idle_duration() >= time_cast<milliseconds>(pool_cfg_.max_idle_time);
            if (!timed_out) {
                break;
            }

            delete front.conn;
            idle_queue_.pop();
            total_count_.fetch_sub(1, memory_order_relaxed);
            ++removed;
        }

        lk.unlock_quiet();

        if (removed > 0) {
            printcfln(color::gray(), "[database_pool] Reclaimed {} idle connections, total={}", removed,
                      total_count_.load(memory_order_relaxed));
        }
    }
}

NEFORCE_END_NAMESPACE__
