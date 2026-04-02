#ifndef NEFORCE_DATABASE_DB_INTERFACE_HPP__
#define NEFORCE_DATABASE_DB_INTERFACE_HPP__
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/time/clocks.hpp"
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/db/db_config.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API idb_result {
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    virtual ~idb_result() = default;
    virtual bool empty() const = 0;
    virtual bool next() = 0;
};

struct NEFORCE_API idb_tb_result : idb_result {
    ~idb_tb_result() override = default;

    virtual size_type row_count() const = 0;
    virtual size_type column_count() const = 0;

    virtual const vector<string_view>& column_names() const = 0;

    virtual string_view get(size_type n) const = 0;

    virtual bool get_bool(size_type n) const = 0;
    virtual int16_t get_int16(size_type n) const = 0;
    virtual int32_t get_int32(size_type n) const = 0;
    virtual int64_t get_int64(size_type n) const = 0;
    virtual float32_t get_float32(size_type n) const = 0;
    virtual float64_t get_float64(size_type n) const = 0;
    virtual decimal_t get_decimal(size_type n) const = 0;

    virtual vector<char> get_blob(size_type n) const = 0;
    virtual uint64_t get_bit(size_type n) const = 0;

    virtual date get_date(size_type n) const = 0;
    virtual time get_time(size_type n) const = 0;
    virtual datetime get_datetime(size_type n) const = 0;
    virtual timestamp get_timestamp(size_type n) const = 0;
};

struct NEFORCE_API idb_kv_result : idb_result {
    ~idb_kv_result() override = default;

    virtual string_view key() const = 0;
    virtual string_view value() const = 0;

    virtual bool value_bool() const = 0;
    virtual int64_t value_int64() const = 0;
    virtual double value_double() const = 0;
    virtual vector<string> value_array() const = 0;
    virtual const vector<pair<string, string>>& value_hash() const = 0;
};


struct NEFORCE_API idb_prepared_result : idb_tb_result {
    ~idb_prepared_result() override = default;
};

struct NEFORCE_API idb_prepared_statement {
    virtual ~idb_prepared_statement() = default;

    virtual uint32_t param_count() const noexcept = 0;

    virtual bool bind_param(uint32_t index, const string& value) = 0;
    virtual bool bind_param(uint32_t index, string_view value) = 0;
    virtual bool bind_param(uint32_t index, const char* value) = 0;
    virtual bool bind_param(uint32_t index, int32_t value) = 0;
    virtual bool bind_param(uint32_t index, int64_t value) = 0;
    virtual bool bind_param(uint32_t index, float64_t value) = 0;
    virtual bool bind_param(uint32_t index, const void* data, size_t length) = 0;

    virtual bool execute() = 0;
    virtual unique_ptr<idb_prepared_result> execute_query() = 0;
    virtual string_view get_error() const noexcept = 0;
    virtual uint32_t get_errno() const noexcept = 0;
};


struct NEFORCE_API idb_connect {
public:
    using clock_type = milliseconds;

private:
    clock_type alive_time_{0};

public:
    virtual ~idb_connect() = default;

    virtual bool connect(const db_config& config) = 0;
    virtual bool reconnect(const db_config& config) = 0;
    virtual void close() = 0;

    virtual bool set_character_set(const string& encoding) const = 0;

    virtual string_view get_character_set() const = 0;
    virtual string_view get_error() const = 0;
    virtual uint32_t get_errno() const = 0;

    virtual bool update(const string& sql) const = 0;

    virtual bool connected() const = 0;
    virtual bool is_valid() const = 0;

    void refresh_alive() noexcept { alive_time_ = time_cast<milliseconds>(steady_clock::now().since_epoch()); }
    NEFORCE_NODISCARD clock_type get_alive() const noexcept {
        return time_cast<milliseconds>(steady_clock::now().since_epoch()) - alive_time_;
    }
};

struct NEFORCE_API idb_tb_connect : idb_connect {
    ~idb_tb_connect() override = default;

    virtual unique_ptr<idb_tb_result> query(const string& sql) const = 0;
    virtual unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const = 0;
};

struct NEFORCE_API idb_kv_connect : idb_connect {
    ~idb_kv_connect() override = default;

    virtual unique_ptr<idb_kv_result> query(const string& sql) const = 0;

    virtual bool set(const string& key, const string& value) = 0;
    virtual bool setex(const string& key, const string& value, int seconds) = 0;
    virtual unique_ptr<idb_kv_result> get(const string& key) = 0;
    virtual bool del(const string& key) = 0;
    virtual bool exists(const string& key) = 0;
    virtual bool expire(const string& key, int seconds) = 0;

    virtual bool hset(const string& key, const string& field, const string& value) = 0;
    virtual unique_ptr<idb_kv_result> hget(const string& key, const string& field) = 0;
    virtual unique_ptr<idb_kv_result> hgetall(const string& key) = 0;

    virtual bool lpush(const string& key, const string& value) = 0;
    virtual bool rpush(const string& key, const string& value) = 0;
    virtual unique_ptr<idb_kv_result> lrange(const string& key, int start, int stop) = 0;

    virtual bool sadd(const string& key, const string& member) = 0;
    virtual unique_ptr<idb_kv_result> smembers(const string& key) = 0;
};


class NEFORCE_API idb_factory {
protected:
    db_config config_;

public:
    explicit idb_factory(db_config config) :
    config_(move(config)) {}
    virtual ~idb_factory() = default;

    virtual idb_connect* create_connect() = 0;
    virtual idb_result* create_result(void* native_result) = 0;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DB_INTERFACE_HPP__
