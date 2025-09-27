#ifndef MSTL_DATABASE_POOL_HPP__
#define MSTL_DATABASE_POOL_HPP__
#include "MSTL/core/basiclib.hpp"
#ifdef MSTL_SUPPORT_DB__
#ifdef MSTL_SUPPORT_MYSQL__
#include <mysql.h>
#endif
#ifdef MSTL_SUPPORT_SQLITE3__
#include <sqlite3.h>
#endif
#ifdef MSTL_SUPPORT_REDIS__
#include <hiredis.h>
#endif
#include "MSTL/core/undef_cmacro.hpp"
#include <mutex>
#include <thread>
#include <condition_variable>
#include "MSTL/core/queue.hpp"
#include "MSTL/core/list.hpp"
#include "MSTL/core/datetime.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(DatabaseError, LinkError, "Database Operations Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(DatabaseTypeCastError, TypeCastError, "Database Type Cast Failed.")

enum class DB_TYPE {
#ifdef MSTL_SUPPORT_MYSQL__
    MYSQL = 1,
#endif
#ifdef MSTL_SUPPORT_SQLITE3__
    SQLITE3,
#endif
#ifdef MSTL_SUPPORT_REDIS__
    REDIS
#endif
};

struct MSTL_API db_connect_config {
    string username{};
    string password{};
    string database{};
    string host = "127.0.0.1";
    uint16_t port = 0;
    string charset{};

#ifdef MSTL_SUPPORT_MYSQL__
    static db_connect_config for_mysql(const string& db);
#endif

#ifdef MSTL_SUPPORT_SQLITE3__
    static db_connect_config for_sqlite(const string& file);
#endif

#ifdef MSTL_SUPPORT_REDIS__
    static db_connect_config for_redis(const string& db);
#endif
};


struct MSTL_API idb_result {
    using size_type         = size_t;
    using difference_type   = ptrdiff_t;

    virtual ~idb_result() = default;
    virtual bool empty() const = 0;
    virtual size_type row_count() const = 0;
    virtual size_type column_count() const = 0;

    virtual const list<string_view>& column_names() const = 0;

    virtual bool next() = 0;

    virtual string_view at(size_type) const = 0;
    virtual bool at_bool(size_type) const = 0;
    virtual int8_t at_int8(size_type) const = 0;
    virtual int16_t at_int16(size_type) const = 0;
    virtual int32_t at_int32(size_type) const = 0;
    virtual int64_t at_int64(size_type) const = 0;
    virtual float32_t at_float32(size_type) const = 0;
    virtual float64_t at_float64(size_type) const = 0;
    virtual decimal_t at_decimal(size_type) const = 0;
    virtual vector<char> at_blob(size_type) const = 0;
    virtual string at_set(size_type) const = 0;
    virtual uint64_t at_bit(size_type) const = 0;
    virtual date at_date(size_type) const = 0;
    virtual time at_time(size_type) const = 0;
    virtual datetime at_datetime(size_type) const = 0;
    virtual timestamp at_timestamp(size_type) const = 0;
    virtual string at_string(size_type) const = 0;
    virtual string_view at_enum(size_type) const = 0;
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
    virtual unique_ptr<idb_result> query(const string& sql) const = 0;
    virtual bool connected() const = 0;
    virtual bool is_valid() const = 0;
    virtual void close() = 0;
    virtual void refresh_alive() = 0;
    virtual clock_type get_alive() const = 0;
    virtual bool reset_connect(const db_connect_config& config) = 0;
};

class MSTL_API idb_factory {
protected:
    db_connect_config config_;

public:
    explicit idb_factory(const db_connect_config& config);

    virtual ~idb_factory() = default;
    virtual idb_connect* create_connect() = 0;
    virtual idb_result* create_result(void* native_result) = 0;
};


#ifdef MSTL_SUPPORT_MYSQL__

struct MSTL_API db_mysql_result final : idb_result {
private:
    ::MYSQL_RES* result = nullptr;
    size_type rows = 0;
    size_type columns = 0;
    ::MYSQL_ROW cursor = nullptr;
    list<string_view>* column_name_ = new list<string_view>;
    list<enum_field_types>* column_types_ = new list<enum_field_types>;

public:
    db_mysql_result() noexcept = default;

    explicit db_mysql_result(::MYSQL_RES* result) noexcept;

    ~db_mysql_result() override;

    MSTL_NODISCARD bool empty() const noexcept override;

    MSTL_NODISCARD size_type row_count() const noexcept override;
    MSTL_NODISCARD size_type column_count() const noexcept override;

    MSTL_NODISCARD const list<string_view>& column_names() const noexcept override;
    MSTL_NODISCARD decltype(auto) column_types() const noexcept ;

    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD _MSTL string_view at(size_type n) const noexcept override;

    MSTL_NODISCARD bool at_bool(size_type n) const override;
    MSTL_NODISCARD int8_t at_int8(size_type n) const override;
    MSTL_NODISCARD int16_t at_int16(size_type n) const override;
    MSTL_NODISCARD int32_t at_int32(size_type n) const override;
    MSTL_NODISCARD int64_t at_int64(size_type n) const override;
    MSTL_NODISCARD float32_t at_float32(size_type n) const override;
    MSTL_NODISCARD float64_t at_float64(size_type n) const override;
    MSTL_NODISCARD decimal_t at_decimal(size_type n) const override;
    MSTL_NODISCARD _MSTL vector<char> at_blob(size_type n) const override;
    MSTL_NODISCARD _MSTL string at_set(size_type n) const override;
    MSTL_NODISCARD uint64_t at_bit(size_type n) const override;
    MSTL_NODISCARD _MSTL date at_date(size_type n) const override;
    MSTL_NODISCARD _MSTL time at_time(size_type n) const override;
    MSTL_NODISCARD _MSTL datetime at_datetime(size_type n) const override;
    MSTL_NODISCARD _MSTL timestamp at_timestamp(size_type n) const override;
    MSTL_NODISCARD string at_string(size_type n) const noexcept override;
    MSTL_NODISCARD string_view at_enum(size_type n) const noexcept override;
};


// database connection is based on MySql connection.
struct MSTL_API db_mysql_connect final : idb_connect {
private:
    ::MYSQL* mysql = nullptr;
    clock_type alive_time_ = 0;

public:
    db_mysql_connect() noexcept;
    ~db_mysql_connect() noexcept override;

    MSTL_NODISCARD bool connect_to(
        const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        uint32_t port, const _MSTL string& character_set) noexcept override;

    MSTL_NODISCARD bool connect_to(const db_connect_config& config) noexcept override;
    MSTL_NODISCARD bool reset_connect(const db_connect_config& config) override;

    MSTL_NODISCARD bool set_character_set(const _MSTL string& encoding) const noexcept override;
    MSTL_NODISCARD bool set_options(mysql_option option, const _MSTL string& str) const noexcept;

    MSTL_NODISCARD string_view get_character_set() const noexcept override;
    MSTL_NODISCARD string_view get_error() const noexcept override;
    MSTL_NODISCARD uint32_t get_errno() const noexcept override;

    MSTL_NODISCARD bool update(const _MSTL string& sql) const noexcept override;
    MSTL_NODISCARD unique_ptr<idb_result> query(const _MSTL string& sql) const noexcept override;

    MSTL_NODISCARD bool connected() const noexcept override;
    MSTL_NODISCARD bool is_valid() const noexcept override;

    void close() noexcept override;
    void refresh_alive() noexcept override;
    MSTL_NODISCARD clock_type get_alive() const noexcept override;
};

class MSTL_API db_mysql_factory final : public idb_factory {
public:
    explicit db_mysql_factory(const db_connect_config& config);

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

#endif


#ifdef MSTL_SUPPORT_SQLITE3__

struct MSTL_API db_sqlite_result final : idb_result {
private:
    ::sqlite3_stmt* stmt = nullptr;
    size_type cursor_ = 0;
    size_type columns = 0;
    list<string_view>* column_names_ = new list<string_view>;
    list<int>* column_types_ = new list<int>;

public:
    db_sqlite_result() noexcept = default;

    explicit db_sqlite_result(::sqlite3_stmt* statement) noexcept;
    ~db_sqlite_result() override;

    // useless function
    MSTL_NODISCARD MSTL_DEPRECATE_FOR("use COUNT * instead of using this function")
    size_type row_count() const noexcept override;
    MSTL_NODISCARD size_type column_count() const noexcept override;

    MSTL_NODISCARD bool empty() const noexcept override;
    MSTL_NODISCARD const list<string_view>& column_names() const override;
    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD _MSTL string_view at(size_type n) const noexcept override;

    MSTL_NODISCARD bool at_bool(size_type n) const override;
    MSTL_NODISCARD int8_t at_int8(size_type n) const override;
    MSTL_NODISCARD int16_t at_int16(size_type n) const override;
    MSTL_NODISCARD int32_t at_int32(size_type n) const override;
    MSTL_NODISCARD int64_t at_int64(size_type n) const override;
    MSTL_NODISCARD float32_t at_float32(size_type n) const override;
    MSTL_NODISCARD float64_t at_float64(size_type n) const override;
    MSTL_NODISCARD decimal_t at_decimal(size_type n) const override;
    MSTL_NODISCARD _MSTL vector<char> at_blob(size_type n) const override;
    MSTL_NODISCARD _MSTL string at_set(size_type n) const override;
    MSTL_NODISCARD uint64_t at_bit(size_type n) const noexcept override;
    MSTL_NODISCARD _MSTL date at_date(size_type n) const noexcept override;
    MSTL_NODISCARD _MSTL time at_time(size_type n) const noexcept override;
    MSTL_NODISCARD _MSTL datetime at_datetime(size_type n) const override;
    MSTL_NODISCARD _MSTL timestamp at_timestamp(size_type n) const override;
    MSTL_NODISCARD _MSTL string at_string(size_type n) const override;
    MSTL_NODISCARD _MSTL string_view at_enum(size_type n) const override;
};

struct MSTL_API db_sqlite_connect final : idb_connect {
private:
    mutable ::sqlite3* db = nullptr;
    clock_type alive_time_ = 0;
    mutable string_view last_error_;

public:
    db_sqlite_connect() noexcept;
    ~db_sqlite_connect() noexcept override;

    bool connect_to(const _MSTL string&, const _MSTL string&,
        const _MSTL string& dbname, const _MSTL string&,
        uint32_t, const _MSTL string&) override;

    bool connect_to(const db_connect_config& config) override;
    MSTL_NODISCARD bool reset_connect(const db_connect_config& config) override;

    MSTL_NODISCARD bool set_character_set(const _MSTL string& encoding) const override;

    MSTL_NODISCARD string_view get_character_set() const override;
    MSTL_NODISCARD string_view get_error() const override;
    MSTL_NODISCARD uint32_t get_errno() const override;

    MSTL_NODISCARD bool update(const string& sql) const override;
    MSTL_NODISCARD unique_ptr<idb_result> query(const string& sql) const override;

    MSTL_NODISCARD bool connected() const override;
    MSTL_NODISCARD bool is_valid() const override;

    void close() noexcept override;
    void refresh_alive() noexcept override;
    MSTL_NODISCARD clock_type get_alive() const noexcept override;

private:
    bool connect_to_file(const string& file_path);
};

class MSTL_API db_sqlite_factory final : public idb_factory {
public:
    explicit db_sqlite_factory(const db_connect_config& config);

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};
#endif

#ifdef MSTL_SUPPORT_REDIS__

struct MSTL_API db_redis_result final : idb_result {
private:
    ::redisReply* reply_ = nullptr;
    size_type cursor_ = 0;
    size_type rows_ = 0;
    list<string_view> column_names_;
    bool is_array_ = false;

    static string format_redis_reply_element(::redisReply* element);

public:
    db_redis_result() noexcept = default;
    explicit db_redis_result(::redisReply* reply) noexcept;
    ~db_redis_result() override;

    MSTL_NODISCARD bool empty() const noexcept override;
    MSTL_NODISCARD size_type row_count() const noexcept override;
    MSTL_NODISCARD size_type column_count() const noexcept override;
    MSTL_NODISCARD const list<string_view>& column_names() const noexcept override;

    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD string_view at(size_type) const noexcept override;

    MSTL_NODISCARD bool at_bool(size_type) const override;
    MSTL_NODISCARD int8_t at_int8(size_type) const override;
    MSTL_NODISCARD int16_t at_int16(size_type) const override;
    MSTL_NODISCARD int32_t at_int32(size_type) const override;
    MSTL_NODISCARD int64_t at_int64(size_type) const override;
    MSTL_NODISCARD float32_t at_float32(size_type) const override;
    MSTL_NODISCARD float64_t at_float64(size_type) const override;
    MSTL_NODISCARD decimal_t at_decimal(size_type) const override;
    MSTL_NODISCARD _MSTL vector<char> at_blob(size_type) const override;
    MSTL_NODISCARD _MSTL string at_set(size_type) const override;
    MSTL_NODISCARD uint64_t at_bit(size_type) const override;
    MSTL_NODISCARD _MSTL date at_date(size_type) const override;
    MSTL_NODISCARD _MSTL time at_time(size_type) const override;
    MSTL_NODISCARD _MSTL datetime at_datetime(size_type) const override;
    MSTL_NODISCARD _MSTL timestamp at_timestamp(size_type) const override;
    MSTL_NODISCARD _MSTL string at_string(size_type) const override;
    MSTL_NODISCARD _MSTL string_view at_enum(size_type) const noexcept override;
};

struct MSTL_API db_redis_connect final : idb_connect {
private:
    ::redisContext* context_ = nullptr;
    clock_type alive_time_ = 0;
    mutable string last_error_{};

    bool authenticate(const string& password) const;
    bool select_database(const string& db_index) const;
    bool connect_to_host(const string& host, uint16_t port, const string& password, const string& dbname);

public:
    db_redis_connect() = default;
    ~db_redis_connect() override;

    bool connect_to(const string&, const string& password,
        const string& dbname, const string& host,
        uint32_t port, const string&) override;

    bool connect_to(const db_connect_config& config) override;
    bool reset_connect(const db_connect_config& config) override;

    MSTL_DEPRECATE_FOR("Redis not support setting character sets")
    bool set_character_set(const string&) const noexcept override;

    MSTL_DEPRECATE_FOR("Redis not support setting character sets")
    string_view get_character_set() const noexcept override;

    string_view get_error() const noexcept override;
    uint32_t get_errno() const noexcept override;

    bool update(const string& sql) const noexcept override;
    unique_ptr<idb_result> query(const string& sql) const override;

    bool connected() const noexcept override;
    bool is_valid() const noexcept override;

    void close() noexcept override;
    void refresh_alive() noexcept override;
    clock_type get_alive() const noexcept override;
};

class MSTL_API db_redis_factory final : public idb_factory {
public:
    explicit db_redis_factory(const db_connect_config& config);

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

#endif


class MSTL_API database_pool {
private:
    db_connect_config config_;
    size_t init_size_;
    size_t max_size_;
    size_t max_idle_time_;  // s
    size_t connect_timeout_;  // ms

    unique_ptr<idb_factory> factory_ = nullptr;
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
    database_pool& operator=(const database_pool&) = delete;
    database_pool(database_pool&&) = delete;
    database_pool& operator=(database_pool&&) = delete;

    _MSTL shared_ptr<idb_connect> get_connect();
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DATABASE_POOL_HPP__
