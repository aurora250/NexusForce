#ifndef MSTL_DB_MYSQL_HPP__
#define MSTL_DB_MYSQL_HPP__
#ifdef MSTL_SUPPORT_DB__
#include "interface.hpp"
#ifdef MSTL_SUPPORT_MYSQL__
#ifdef CR_OUT_OF_MEMORY
#undef CR_OUT_OF_MEMORY
#endif
#include <mysql.h>
MSTL_BEGIN_NAMESPACE__

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

    MSTL_NODISCARD bool empty() const noexcept override { return result == nullptr; }

    MSTL_NODISCARD size_type row_count() const noexcept override;
    MSTL_NODISCARD size_type column_count() const noexcept override;

    MSTL_NODISCARD const list<string_view>& column_names() const noexcept override;
    MSTL_NODISCARD decltype(auto) column_types() const noexcept;

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
    explicit db_mysql_factory(const db_connect_config& config) : idb_factory(config) {};

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_SUPPORT_DB__
#endif // MSTL_DB_MYSQL_HPP__
