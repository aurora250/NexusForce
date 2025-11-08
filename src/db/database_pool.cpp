#include <MSTL/db/database_pool.hpp>
#ifdef MSTL_SUPPORT_DB__
#include <MSTL/db/mysql.hpp>
#include <MSTL/db/sqlite.hpp>
#include <MSTL/db/redis.hpp>
MSTL_BEGIN_NAMESPACE__

void database_pool::produce_connect_task() {
    while (true) {
        if (!running_) break;
        std::unique_lock<std::mutex> lock(queue_mtx_);
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
        std::this_thread::sleep_for(std::chrono::seconds(max_idle_time_));
        if (!running_) break;
        std::unique_lock<std::mutex> lock(queue_mtx_);

        while (connect_queue_.size() > init_size_) {
            const idb_connect* ptr = connect_queue_.front();
            if (ptr->get_alive() >= max_idle_time_ * 1000) {
                connect_queue_.pop();
                delete ptr;
            }
            else
                break;
        }
    }
}

database_pool::database_pool(const DB_TYPE type, const db_connect_config& config,
        const size_t init_size, const size_t max_size,
        const size_t max_idle_time, const size_t connect_timeout) :
    config_(config), init_size_(init_size), max_size_(max_size), max_idle_time_(max_idle_time),
    connect_timeout_(connect_timeout), running_(true) {
    switch(type) {
#ifdef MSTL_SUPPORT_MYSQL__
        case DB_TYPE::MYSQL:
            factory_ = make_unique<db_mysql_factory>(config);
        break;
#endif
#ifdef MSTL_SUPPORT_SQLITE3__
        case DB_TYPE::SQLITE3:
            factory_ = make_unique<db_sqlite_factory>(config);
        break;
#endif
#ifdef MSTL_SUPPORT_REDIS__
        case DB_TYPE::REDIS:
            factory_ = make_unique<db_redis_factory>(config);
        break;
#endif
        default:
            // never run:
            Exception(DatabaseError("Useless Database Type"));
            break;
    }

    for (size_t i = 0; i < init_size_; i++) {
        auto* p = factory_->create_connect();
        if (p != nullptr) {
            p->refresh_alive();
            connect_queue_.push(p);
        }
        else {
            --i;
        }
    }
    produce_ = std::thread([this] { produce_connect_task(); });
    scanner_ = std::thread([this] { scanner_connect_task(); });
}

database_pool::~database_pool() {
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

_MSTL shared_ptr<idb_connect> database_pool::get_connect() {
    std::unique_lock<std::mutex> lock(queue_mtx_);

    while (connect_queue_.empty()) {
        if (cv_.wait_for(lock,
            std::chrono::milliseconds(connect_timeout_)) == std::cv_status::timeout) {
            if (connect_queue_.empty()) {
                if (connect_queue_.size() < max_size_) {
                    auto* new_conn = factory_->create_connect();
                    if (new_conn != nullptr) {
                        new_conn->refresh_alive();
                        connect_queue_.push(new_conn);
                        continue;
                    }
                }
                return nullptr;
            }
            }
    }

    idb_connect* raw_conn = connect_queue_.front();
    connect_queue_.pop();

    if (!raw_conn->is_valid()) {
        try {
            if (!raw_conn->reset_connect(config_)) {
                delete raw_conn;
                raw_conn = factory_->create_connect();
                if (raw_conn == nullptr) {
                    cv_.notify_all();
                    return nullptr;
                }
            }
        }
        catch (...) {
            delete raw_conn;
            cv_.notify_all();
            return nullptr;
        }
    }

    auto conn_ptr = _MSTL shared_ptr<idb_connect>(raw_conn,
        [this](idb_connect* p) {
            std::unique_lock<std::mutex> lock1(queue_mtx_);
            if (p->is_valid()) {
                p->refresh_alive();
                connect_queue_.push(p);
            }
            else {
                delete p;
            }
            cv_.notify_all();
        }
    );

    cv_.notify_all();
    return conn_ptr;
}

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_DB__
