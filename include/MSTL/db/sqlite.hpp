#ifndef MSTL_DB_SQLITE_HPP__
#define MSTL_DB_SQLITE_HPP__
#ifdef MSTL_SUPPORT_DB__
#include "interface.hpp"
#ifdef MSTL_SUPPORT_SQLITE3__
#include <sqlite3.h>
#endif
MSTL_BEGIN_NAMESPACE__

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

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DB_SQLITE_HPP__
