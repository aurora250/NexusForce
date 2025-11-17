#ifndef MSTL_DATABASE_POOL_HPP__
#define MSTL_DATABASE_POOL_HPP__
#include <condition_variable>
#include <mutex>
#include <thread>
#include "../core/container/queue.hpp"
#include "db_interface.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API database_pool {
private:
    db_config config_;
    size_t init_size_;
    size_t max_size_;
    size_t max_idle_time_;  // s
    size_t connect_timeout_;  // ms

    _MSTL unique_ptr<idb_factory> factory_ = nullptr;
    _MSTL queue<idb_connect*> connect_queue_;
    std::mutex queue_mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};

    std::thread produce_;
    std::thread scanner_;

    void produce_connect_task();
    void scanner_connect_task();

    template <typename T>
    _MSTL shared_ptr<T> get_connect_impl();

public:
    database_pool(DB_TYPE type, const db_config& config,
        size_t init_size = 50, size_t max_size = 1024,
        size_t max_idle_time = 30, size_t connect_timeout = 100);

    ~database_pool();

    database_pool(const database_pool&) = delete;
    database_pool& operator =(const database_pool&) = delete;
    database_pool(database_pool&&) = delete;
    database_pool& operator =(database_pool&&) = delete;

    _MSTL shared_ptr<idb_connect> get_connect() {
        return get_connect_impl<idb_connect>();
    }
    _MSTL shared_ptr<idb_tb_connect> get_tb_connect() {
        return get_connect_impl<idb_tb_connect>();
    }
    _MSTL shared_ptr<idb_kv_connect> get_kv_connect() {
        return get_connect_impl<idb_kv_connect>();
    }
};


template <typename T>
_MSTL shared_ptr<T> database_pool::get_connect_impl() {
    std::unique_lock<std::mutex> lock(queue_mtx_);

    while (connect_queue_.empty() && running_) {
        if (cv_.wait_for(lock, std::chrono::milliseconds(connect_timeout_)) == std::cv_status::timeout) {
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

    auto conn_ptr = _MSTL shared_ptr<T>(
        dynamic_cast<T*>(raw_conn),
        [this](T* p) {
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
#endif // MSTL_DATABASE_POOL_HPP__
