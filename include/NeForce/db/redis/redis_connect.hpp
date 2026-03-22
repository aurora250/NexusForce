#ifndef NEFORCE_DATABASE_REDIS_CONNECT_HPP__
#define NEFORCE_DATABASE_REDIS_CONNECT_HPP__
#ifdef NEFORCE_SUPPORT_HIREDIS
#include "NeForce/db/db_interface.hpp"
#include <hiredis/hiredis.h>
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API redis_connect final : idb_kv_connect {
private:
    ::redisContext* link_ = nullptr;
    mutable string last_error_{};

    ::redisReply* execute_command(string_view command, const vector<string_view>& args = {}) const;
    bool authenticate(const string& password) const;
    bool select_database(const string& db_index) const;

public:
    redis_connect() = default;
    ~redis_connect() override { close(); }

    bool connect(const db_config& config) override;
    bool reconnect(const db_config& config) override;
    void close() noexcept override;

    NEFORCE_DEPRECATED_FOR("Redis not support setting character sets")
    bool set_character_set(const string&) const noexcept override { return false; }

    NEFORCE_DEPRECATED_FOR("Redis not support setting character sets")
    string_view get_character_set() const noexcept override { return ""; }

    string_view get_error() const noexcept override;
    uint32_t get_errno() const noexcept override { return link_ ? link_->err : 0; }

    bool update(const string& sql) const noexcept override;
    unique_ptr<idb_kv_result> query(const string& sql) const override;

    bool connected() const noexcept override { return link_ != nullptr && !link_->err; }
    bool is_valid() const noexcept override;

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

class NEFORCE_API redis_factory final : public idb_factory {
public:
    explicit redis_factory(db_config config)
    : idb_factory(_NEFORCE move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_REDIS_CONNECT_HPP__
