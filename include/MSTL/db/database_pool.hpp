#ifndef MSTL_DATABASE_POOL_HPP__
#define MSTL_DATABASE_POOL_HPP__
#ifdef MSTL_SUPPORT_DB__
#include "MSTL/core/undef_cmacro.hpp"
#include "MSTL/core/queue.hpp"
#include "interface.hpp"
#include <mutex>
#include <thread>
#include <condition_variable>
MSTL_BEGIN_NAMESPACE__

class MSTL_API database_pool {
private:
    db_connect_config config_;
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

    friend database_pool& get_instance_database_pool();

    void produce_connect_task();
    void scanner_connect_task();

public:
    database_pool(DB_TYPE type, const db_connect_config& config,
        size_t init_size = 50, size_t max_size = 1024,
        size_t max_idle_time = 30, size_t connect_timeout = 100);

    ~database_pool();

    database_pool(const database_pool&) = delete;
    database_pool& operator =(const database_pool&) = delete;
    database_pool(database_pool&&) = delete;
    database_pool& operator =(database_pool&&) = delete;

    _MSTL shared_ptr<idb_connect> get_connect();
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DATABASE_POOL_HPP__
