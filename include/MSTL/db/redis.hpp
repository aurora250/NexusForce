#ifndef MSTL_DB_REDIS_HPP__
#define MSTL_DB_REDIS_HPP__
#ifdef MSTL_SUPPORT_REDIS__
#include "db_interface.hpp"
#include <hiredis.h>
namespace redis {
    using ::redisReply;
    using ::redisContext;
    using ::freeReplyObject;
    using ::redisCommand;
    using ::redisCommandArgv;
    using ::redisConnect;
    using ::redisFree;
}
MSTL_BEGIN_NAMESPACE__

struct MSTL_API db_redis_result final : idb_kv_result {
private:
    redis::redisReply* reply_ = nullptr;
    size_type cursor_ = 0;
    size_type rows_ = 0;
    unique_ptr<vector<string>> column_names_ = make_unique<vector<string>>();
    unique_ptr<vector<pair<string, string>>> kv_pairs_ = make_unique<vector<pair<string, string>>>();
    bool is_array_ = false;
    size_type kv_cursor_ = 0;

    static string format_redis_reply_element(redis::redisReply* element);
    void process_reply();
    string at_string() const;

public:
    db_redis_result() noexcept = default;

    explicit db_redis_result(redis::redisReply* reply) noexcept
    : reply_(reply) {
        process_reply();
    }

    ~db_redis_result() override {
        if (reply_) redis::freeReplyObject(reply_);
    }

    MSTL_NODISCARD bool empty() const noexcept override { return !reply_ || (rows_ == 0 && kv_pairs_->empty()); }
    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD string_view key() const noexcept override;
    MSTL_NODISCARD string_view value() const noexcept override;

    MSTL_NODISCARD bool value_bool() const override;
    MSTL_NODISCARD int64_t value_int64() const override;
    MSTL_NODISCARD double value_double() const override;
    MSTL_NODISCARD vector<string> value_array() const override;
    MSTL_NODISCARD const vector<pair<string, string>>& value_hash() const override { return *kv_pairs_; }

    MSTL_NODISCARD int type() const noexcept { return reply_ ? reply_->type : -1; }
    MSTL_NODISCARD bool is_nil() const noexcept { return reply_ && reply_->type == REDIS_REPLY_NIL; }
};

struct MSTL_API db_redis_connect final : idb_kv_connect {
private:
    redis::redisContext* context_ = nullptr;
    clock_type alive_time_ = 0;
    mutable string last_error_{};

    redis::redisReply* execute_command(string_view command, const vector<string_view>& args = {}) const;
    bool authenticate(const string& password) const;
    bool select_database(const string& db_index) const;
    bool connect_to_host(const string& host, uint16_t port, const string& password, const string& dbname);

public:
    db_redis_connect() = default;
    ~db_redis_connect() override { close(); }

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

class MSTL_API db_redis_factory final : public idb_factory {
public:
    explicit db_redis_factory(db_config config)
    : idb_factory(_MSTL move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override {
        return new db_redis_result(static_cast<redis::redisReply*>(native_result));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_REDIS__
#endif // MSTL_DB_REDIS_HPP__
