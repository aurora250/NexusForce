#ifndef MSTL_REDIS_CONNECT_HPP__
#define MSTL_REDIS_CONNECT_HPP__
#ifdef MSTL_SUPPORT_REDIS__
#include "MSTL/db/db_interface.hpp"
#include "redis_config.hpp"
#include "MSTL/core/undef_cmacro.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API redis_connect final : idb_kv_connect {
private:
    _MSTL_REDIS redisContext* context_ = nullptr;
    clock_type alive_time_ = 0;
    mutable string last_error_{};

    _MSTL_REDIS redisReply* execute_command(string_view command, const vector<string_view>& args = {}) const;
    bool authenticate(const string& password) const;
    bool select_database(const string& db_index) const;
    bool connect_to_host(const string& host, uint16_t port, const string& password, const string& dbname);

public:
    redis_connect() = default;
    ~redis_connect() override { close(); }

    bool connect_to(const string&, const string& password,
        const string& dbname, const string& host,
        const uint32_t port, const string&) override {
        return connect_to_host(host, port, password, dbname);
    }

    bool connect_to(const db_config& config) override {
        return connect_to_host(
            config.host,
            config.port,
            config.password,
            config.database
            );
    }

    bool reset_connect(const db_config& config) override;

    MSTL_DEPRECATE_FOR("Redis not support setting character sets")
    bool set_character_set(const string&) const noexcept override { return false; }

    MSTL_DEPRECATE_FOR("Redis not support setting character sets")
    string_view get_character_set() const noexcept override { return ""; }

    string_view get_error() const noexcept override;
    uint32_t get_errno() const noexcept override { return context_ ? context_->err : 0; }

    bool update(const string& sql) const noexcept override;
    unique_ptr<idb_kv_result> query(const string& sql) const override;

    bool connected() const noexcept override { return context_ != nullptr && !context_->err; }
    bool is_valid() const noexcept override;

    void close() noexcept override;
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }

    bool set(const string& key, const string& value) override;
    bool setex(const string& key, const string& value, int seconds) override;
    unique_ptr<idb_kv_result> get(const string& key) override;
    bool del(const string& key) override;
    bool exists(const string& key) override;
    bool expire(const string& key, int seconds) override;

    bool hset(const string& key, const string& field, const string& value) override;
    unique_ptr<idb_kv_result> hget(const string& key, const string& field) override;
    unique_ptr<idb_kv_result> hgetall(const string& key) override;

    bool lpush(const string& key, const string& value) override;
    bool rpush(const string& key, const string& value) override;
    unique_ptr<idb_kv_result> lrange(const string& key, int start, int stop) override;

    bool sadd(const string& key, const string& member) override;
    unique_ptr<idb_kv_result> smembers(const string& key) override;
};

class MSTL_API redis_factory final : public idb_factory {
public:
    explicit redis_factory(db_config config)
    : idb_factory(_MSTL move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_REDIS_CONNECT_HPP__
