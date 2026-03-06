#include <NeForce/db/database_pool.hpp>
#include <NeForce/core/system/console.hpp>
#ifdef NEFORCE_SUPPORT_DB
#include <NeForce/db/mysql/mysql_connect.hpp>
#include <NeForce/db/pgsql/pgsql_connect.hpp>
#include <NeForce/db/redis/redis_connect.hpp>
#include <NeForce/db/sqlite/sqlite_connect.hpp>
#endif
NEFORCE_BEGIN_NAMESPACE__

void database_pool::produce_connect_task() {
    while (true) {
        if (!running_) break;
        _NEFORCE smart_lock<_NEFORCE mutex> lock(queue_mtx_);
        while (!connect_queue_.empty()) {
            cv_.wait(lock);
            if (!running_) break;
        }
        if (!running_) break;
        if (connect_queue_.size() < max_size_) {
            auto* p = factory_->create_connect();
            if (p != nullptr) {
                p->refresh_alive();
                connect_queue_.push(p);
            }
        }
        cv_.notify_all();
    }
}

void database_pool::scanner_connect_task() {
    while (true) {
        if (!running_) break;
        _NEFORCE this_thread::sleep_for(seconds(max_idle_time_));
        if (!running_) break;
        _NEFORCE lock<_NEFORCE mutex> lock(queue_mtx_);

        while (connect_queue_.size() > init_size_) {
            const idb_connect* ptr = connect_queue_.front();
            if (ptr->get_alive() >= max_idle_time_ * 1000) {
                connect_queue_.pop();
                delete ptr;
            } else {
                break;
            }
        }
    }
}

database_pool::database_pool(
    const DB_TYPE type, const db_config& config,
    const size_t init_size, const size_t max_size,
    const size_t max_idle_time, const size_t connect_timeout)
: config_(config), init_size_(init_size),
max_size_(max_size), max_idle_time_(max_idle_time),
connect_timeout_(connect_timeout), running_(true) {
    switch(type) {
#ifdef NEFORCE_SUPPORT_MYSQL
        case DB_TYPE::MYSQL: {
            factory_ = make_unique<mysql_factory>(config);
            break;
        }
#endif
#ifdef NEFORCE_SUPPORT_SQLITE3
        case DB_TYPE::SQLITE3: {
            factory_ = make_unique<sqlite_factory>(config);
            break;
        }
#endif
#ifdef NEFORCE_SUPPORT_HIREDIS
        case DB_TYPE::REDIS: {
            factory_ = make_unique<redis_factory>(config);
            break;
        }
#endif
#ifdef NEFORCE_SUPPORT_POSTGRESQL
        case DB_TYPE::POSTGRESQL: {
            factory_ = make_unique<pgsql_factory>(config);
            break;
        }
#endif
        default: {
            throw_exception(value_exception("Useless Database Type"));
            break;
        }
    }

    size_t record = 0;
    size_t threshhold = 0;
    for (size_t i = 0; i < init_size_; i++) {
        auto* p = factory_->create_connect();
        if (p != nullptr) {
            p->refresh_alive();
            connect_queue_.push(p);
        }
        else {
            if (record == i) {
                ++threshhold;
                if (threshhold == 5) {
                    printcln(color::red(), "Retry connecting database in the same loop failed");
                    stop();
                }
            }
            record = i;
            --i;
        }
    }
    produce_ = _NEFORCE thread([this] { produce_connect_task(); });
    scanner_ = _NEFORCE thread([this] { scanner_connect_task(); });
}

void database_pool::stop() {
    running_ = false;
    cv_.notify_all();

    if (produce_.joinable()) {
        produce_.join();
    }
    if (scanner_.joinable()) {
        scanner_.join();
    }

    while (!connect_queue_.empty()) {
        delete connect_queue_.front();
        connect_queue_.pop();
    }
}

NEFORCE_END_NAMESPACE__
