#ifndef MSTL_DB_REDIS_HPP__
#define MSTL_DB_REDIS_HPP__
#ifdef MSTL_SUPPORT_DB__
#include "interface.hpp"
#ifdef MSTL_SUPPORT_REDIS__
#include <hiredis.h>
MSTL_BEGIN_NAMESPACE__

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

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_REDIS__
#endif // MSTL_SUPPORT_DB__
#endif // MSTL_DB_REDIS_HPP__
