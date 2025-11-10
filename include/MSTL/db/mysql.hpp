#ifndef MSTL_DB_MYSQL_HPP__
#define MSTL_DB_MYSQL_HPP__
#ifdef MSTL_SUPPORT_MYSQL__
#include "interface.hpp"
#ifdef CR_OUT_OF_MEMORY
#undef CR_OUT_OF_MEMORY
#endif
#include <mysql.h>
namespace mysql {
    using ::MYSQL;
    using ::MYSQL_RES;
    using ::MYSQL_ROW;
    using ::MYSQL_ROWS;
    using ::MYSQL_FIELD;
    using ::enum_field_types;
    using ::mysql_option;
    using ::mysql_init;
    using ::mysql_real_connect;
    using ::mysql_error;
    using ::mysql_errno;
    using ::mysql_options;
    using ::mysql_set_character_set;
    using ::mysql_character_set_name;
    using ::mysql_num_rows;
    using ::mysql_num_fields;
    using ::mysql_fetch_row;
    using ::mysql_fetch_field;
    using ::mysql_free_result;
    using ::mysql_close;
    using ::mysql_query;
    using ::mysql_store_result;
    using ::MYSQL_TYPE_BOOL;
    using ::MYSQL_TYPE_SHORT;
    using ::MYSQL_TYPE_TINY;
    using ::MYSQL_TYPE_LONG;
    using ::MYSQL_TYPE_INT24;
    using ::MYSQL_TYPE_LONGLONG;
    using ::MYSQL_TYPE_FLOAT;
    using ::MYSQL_TYPE_DOUBLE;
    using ::MYSQL_TYPE_DECIMAL;
    using ::MYSQL_TYPE_NEWDECIMAL;
    using ::MYSQL_TYPE_BLOB;
    using ::MYSQL_TYPE_TINY_BLOB;
    using ::MYSQL_TYPE_MEDIUM_BLOB;
    using ::MYSQL_TYPE_LONG_BLOB;
    using ::MYSQL_TYPE_SET;
    using ::MYSQL_TYPE_BIT;
    using ::MYSQL_TYPE_DATE;
    using ::MYSQL_TYPE_DATETIME;
    using ::MYSQL_TYPE_TIMESTAMP;
}
MSTL_BEGIN_NAMESPACE__

struct MSTL_API db_mysql_result final : idb_tb_result {
private:
    mysql::MYSQL_RES* result_ = nullptr;
    size_type rows_ = 0;
    size_type columns_ = 0;
    mysql::MYSQL_ROW cursor_ = nullptr;
    list<string_view>* column_name_ = new list<string_view>;
    list<mysql::enum_field_types>* column_types_ = new list<mysql::enum_field_types>;

public:
    db_mysql_result() noexcept = default;

    explicit db_mysql_result(mysql::MYSQL_RES* result) noexcept;

    ~db_mysql_result() override;

    MSTL_NODISCARD bool empty() const noexcept override { return result_ == nullptr; }

    MSTL_NODISCARD size_type row_count() const noexcept override { return rows_; }
    MSTL_NODISCARD size_type column_count() const noexcept override { return columns_; }

    MSTL_NODISCARD const list<string_view>& column_names() const noexcept override { return *column_name_; }
    MSTL_NODISCARD decltype(auto) column_types() const noexcept {
        return const_cast<const list<mysql::enum_field_types>&>(*column_types_);
    }

    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD _MSTL string_view get(size_type n) const noexcept override;

    MSTL_NODISCARD bool get_bool(size_type n) const override;
    MSTL_NODISCARD int8_t get_int8(size_type n) const override;
    MSTL_NODISCARD int16_t get_int16(size_type n) const override;
    MSTL_NODISCARD int32_t get_int32(size_type n) const override;
    MSTL_NODISCARD int64_t get_int64(size_type n) const override;
    MSTL_NODISCARD float32_t get_float32(size_type n) const override;
    MSTL_NODISCARD float64_t get_float64(size_type n) const override;
    MSTL_NODISCARD decimal_t get_decimal(size_type n) const override;
    MSTL_NODISCARD _MSTL vector<char> get_blob(size_type n) const override;
    MSTL_NODISCARD _MSTL string get_set(size_type n) const override;
    MSTL_NODISCARD uint64_t get_bit(size_type n) const override;
    MSTL_NODISCARD _MSTL date get_date(size_type n) const override;
    MSTL_NODISCARD _MSTL time get_time(size_type n) const override;
    MSTL_NODISCARD _MSTL datetime get_datetime(size_type n) const override;
    MSTL_NODISCARD _MSTL timestamp get_timestamp(size_type n) const override;
    MSTL_NODISCARD string get_string(size_type n) const noexcept override { return string{get(n)}; }
    MSTL_NODISCARD string_view get_enum(size_type n) const noexcept override { return get(n); }
};


struct MSTL_API db_mysql_connect final : idb_tb_connect {
private:
    mysql::MYSQL* mysql_ = nullptr;
    clock_type alive_time_ = 0;

public:
    db_mysql_connect() noexcept { mysql_ = mysql::mysql_init(nullptr); }
    ~db_mysql_connect() noexcept override { this->close(); }

    MSTL_NODISCARD bool connect_to(
        const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        uint32_t port, const _MSTL string& character_set) noexcept override;

    MSTL_NODISCARD bool connect_to(const db_connect_config& config) noexcept override {
        return connect_to(
            config.username,
            config.password,
            config.database,
            config.host,
            config.port,
            config.charset
        );
    }
    MSTL_NODISCARD bool reset_connect(const db_connect_config& config) override;

    MSTL_NODISCARD bool set_character_set(const _MSTL string& encoding) const noexcept override {
        return connected() && !mysql::mysql_set_character_set(mysql_, encoding.data());
    }
    MSTL_NODISCARD bool set_options(const mysql::mysql_option option, const _MSTL string& str) const noexcept {
        return connected() && !mysql::mysql_options(mysql_, option, str.data());
    }

    MSTL_NODISCARD string_view get_character_set() const noexcept override {
        return mysql::mysql_character_set_name(mysql_);
    }
    MSTL_NODISCARD string_view get_error() const noexcept override {
        return mysql::mysql_error(mysql_);
    }
    MSTL_NODISCARD uint32_t get_errno() const noexcept override {
        return mysql::mysql_errno(mysql_);
    }

    MSTL_NODISCARD bool update(const _MSTL string& sql) const noexcept override {
        return !mysql::mysql_query(mysql_, sql.c_str());
    }
    MSTL_NODISCARD unique_ptr<idb_tb_result> query(const _MSTL string& sql) const noexcept override;

    MSTL_NODISCARD bool connected() const noexcept override { return mysql_ != nullptr; }
    MSTL_NODISCARD bool is_valid() const noexcept override { return mysql_ping(mysql_) == 0; }

    void close() noexcept override { if (connected()) mysql_close(mysql_); }
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    MSTL_NODISCARD clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }
};

class MSTL_API db_mysql_factory final : public idb_factory {
public:
    explicit db_mysql_factory(db_connect_config config)
    : idb_factory(_MSTL move(config)) {};

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override {
        return new db_mysql_result(static_cast<mysql::MYSQL_RES*>(native_result));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_DB_MYSQL_HPP__
