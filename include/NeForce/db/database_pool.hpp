#ifndef NEFORCE_DATABASE_DATABASE_POOL_HPP__
#define NEFORCE_DATABASE_DATABASE_POOL_HPP__
#include "NeForce/core/container/queue.hpp"
#include "NeForce/core/memory/shared_ptr.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API database_pool {
private:
    db_config config_;
    size_t init_size_;
    size_t max_size_;
    seconds max_idle_time_;
    milliseconds connect_timeout_;

    unique_ptr<idb_factory> factory_ = nullptr;
    queue<idb_connect*> connect_queue_;
    mutex queue_mtx_;
    condition_variable cv_;
    atomic<bool> running_{false};

    thread produce_;
    thread scanner_;

    void produce_connect_task();
    void scanner_connect_task();

    template <typename T>
    shared_ptr<T> get_connect_impl();

public:
    database_pool(
        db_type type, const db_config& config,
        size_t init_size = 50, size_t max_size = 1024,
        seconds max_idle_time = seconds{30},
        milliseconds connect_timeout = milliseconds{100});

    ~database_pool() { stop(); }

    database_pool(const database_pool&) = delete;
    database_pool& operator =(const database_pool&) = delete;
    database_pool(database_pool&&) = delete;
    database_pool& operator =(database_pool&&) = delete;

    void stop();

    shared_ptr<idb_connect> get_connect() {
        return get_connect_impl<idb_connect>();
    }
    shared_ptr<idb_tb_connect> get_tb_connect() {
        return get_connect_impl<idb_tb_connect>();
    }
    shared_ptr<idb_kv_connect> get_kv_connect() {
        return get_connect_impl<idb_kv_connect>();
    }
};


template <typename T>
shared_ptr<T> database_pool::get_connect_impl() {
    smart_lock<mutex> lk1(queue_mtx_);

    while (connect_queue_.empty() && running_) {
        if (cv_.wait_for(lk1, milliseconds(connect_timeout_)) == cv_status::timeout) {
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
            if (!raw_conn->reconnect(config_)) {
                delete raw_conn;
                raw_conn = factory_->create_connect();
                if (raw_conn == nullptr) {
                    cv_.notify_all();
                    return nullptr;
                }
            }
        } catch (...) {
            delete raw_conn;
            cv_.notify_all();
            return nullptr;
        }
    }

    shared_ptr<T> conn_ptr {
        dynamic_cast<T*>(raw_conn),
        [this](T* p) {
            lock<mutex> lk2(queue_mtx_);
            if (p->is_valid()) {
                p->refresh_alive();
                connect_queue_.push(p);
            }
            else {
                delete p;
            }
            cv_.notify_all();
        }
    };

    cv_.notify_all();
    return conn_ptr;
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DATABASE_POOL_HPP__
