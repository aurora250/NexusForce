#ifndef MSTL_DB_INTERFACE_HPP__
#define MSTL_DB_INTERFACE_HPP__
#ifdef MSTL_SUPPORT_DB__
#include "db_config.hpp"
#include "MSTL/core/vector.hpp"
#include "MSTL/core/datetime.hpp"
#include "MSTL/core/list.hpp"
#include <ctime>
MSTL_BEGIN_NAMESPACE__

struct MSTL_API idb_result {
    using size_type         = size_t;
    using difference_type   = ptrdiff_t;

    virtual ~idb_result() = default;
    virtual bool empty() const = 0;
    virtual bool next() = 0;
};

struct MSTL_API idb_tb_result : idb_result {
    ~idb_tb_result() override = default;

    virtual size_type row_count() const = 0;
    virtual size_type column_count() const = 0;

    virtual const list<string_view>& column_names() const = 0;

    virtual string_view get(size_type) const = 0;
    virtual bool get_bool(size_type) const = 0;
    virtual int8_t get_int8(size_type) const = 0;
    virtual int16_t get_int16(size_type) const = 0;
    virtual int32_t get_int32(size_type) const = 0;
    virtual int64_t get_int64(size_type) const = 0;
    virtual float32_t get_float32(size_type) const = 0;
    virtual float64_t get_float64(size_type) const = 0;
    virtual decimal_t get_decimal(size_type) const = 0;
    virtual vector<char> get_blob(size_type) const = 0;
    virtual string get_set(size_type) const = 0;
    virtual uint64_t get_bit(size_type) const = 0;
    virtual date get_date(size_type) const = 0;
    virtual time get_time(size_type) const = 0;
    virtual datetime get_datetime(size_type) const = 0;
    virtual timestamp get_timestamp(size_type) const = 0;
    virtual string get_string(size_type) const = 0;
    virtual string_view get_enum(size_type) const = 0;
};

struct MSTL_API idb_kv_result : idb_result {
    ~idb_kv_result() override = default;

    virtual string_view key() const = 0;
    virtual string_view value() const = 0;

    virtual bool value_bool() const = 0;
    virtual int64_t value_int64() const = 0;
    virtual double value_double() const = 0;
    virtual vector<string> value_array() const = 0;
    virtual const vector<pair<string, string>>& value_hash() const = 0;
};


struct MSTL_API idb_connect {
    using clock_type = std::clock_t;

    virtual ~idb_connect() = default;

    virtual bool connect_to(const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        uint32_t port, const _MSTL string& character_set) = 0;
    virtual bool connect_to(const db_connect_config& config) = 0;

    virtual bool set_character_set(const _MSTL string& encoding) const = 0;
    virtual string_view get_character_set() const = 0;
    virtual string_view get_error() const = 0;
    virtual uint32_t get_errno() const = 0;

    virtual bool update(const _MSTL string& sql) const = 0;
    virtual bool connected() const = 0;
    virtual bool is_valid() const = 0;
    virtual void close() = 0;
    virtual void refresh_alive() = 0;
    virtual clock_type get_alive() const = 0;
    virtual bool reset_connect(const db_connect_config& config) = 0;
};

struct MSTL_API idb_tb_connect : idb_connect {
    ~idb_tb_connect() override = default;

    virtual unique_ptr<idb_tb_result> query(const string& sql) const = 0;
};

struct MSTL_API idb_kv_connect : idb_connect {
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


class MSTL_API idb_factory {
protected:
    db_connect_config config_;

public:
    explicit idb_factory(db_connect_config config) : config_(_MSTL move(config)) {}
    virtual ~idb_factory() = default;

    virtual idb_connect* create_connect() = 0;
    virtual idb_result* create_result(void* native_result) = 0;
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_DB__
#endif // MSTL_DB_INTERFACE_HPP__
