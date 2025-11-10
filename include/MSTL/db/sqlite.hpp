#ifndef MSTL_DB_SQLITE_HPP__
#define MSTL_DB_SQLITE_HPP__
#ifdef MSTL_SUPPORT_SQLITE3__
#include "interface.hpp"
#include <sqlite3.h>
namespace sqlite {
    using ::sqlite3;
    using ::sqlite3_stmt;
    using ::sqlite3_column_count;
    using ::sqlite3_column_name;
    using ::sqlite3_column_type;
    using ::sqlite3_step;
    using ::sqlite3_finalize;
    using ::sqlite3_column_text;
    using ::sqlite3_column_int;
    using ::sqlite3_column_int64;
    using ::sqlite3_column_double;
    using ::sqlite3_open;
    using ::sqlite3_prepare_v2;
    using ::sqlite3_errmsg;
    using ::sqlite3_errcode;
    using ::sqlite3_exec;
    using ::sqlite3_free;
    using ::sqlite3_close;
}
MSTL_BEGIN_NAMESPACE__

struct MSTL_API db_sqlite_result final : idb_tb_result {
private:
    sqlite::sqlite3_stmt* stmt_ = nullptr;
    size_type cursor_ = 0;
    size_type columns_ = 0;
    list<string_view>* column_names_ = new list<string_view>;
    list<int>* column_types_ = new list<int>;

public:
    db_sqlite_result() noexcept = default;

    explicit db_sqlite_result(sqlite::sqlite3_stmt* statement) noexcept;
    ~db_sqlite_result() override;

    MSTL_NODISCARD MSTL_DEPRECATE_FOR("use COUNT * instead of using this function")
    size_type row_count() const noexcept override { return 0; }
    MSTL_NODISCARD size_type column_count() const noexcept override { return columns_; }

    MSTL_NODISCARD bool empty() const noexcept override { return stmt_ == nullptr; }
    MSTL_NODISCARD const list<string_view>& column_names() const noexcept override { return *column_names_; }
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
    MSTL_NODISCARD _MSTL string get_set(size_type n) const override { return get_string(n); }
    MSTL_NODISCARD uint64_t get_bit(size_type n) const noexcept override;
    MSTL_NODISCARD _MSTL date get_date(size_type n) const noexcept override { return get_datetime(n).dates(); }
    MSTL_NODISCARD _MSTL time get_time(size_type n) const noexcept override { return get_datetime(n).times(); }
    MSTL_NODISCARD _MSTL datetime get_datetime(size_type n) const override;
    MSTL_NODISCARD _MSTL timestamp get_timestamp(size_type n) const override;
    MSTL_NODISCARD _MSTL string get_string(size_type n) const override { return string{get(n)}; }
    MSTL_NODISCARD _MSTL string_view get_enum(size_type n) const override { return get(n); }
};

struct MSTL_API db_sqlite_connect final : idb_tb_connect {
private:
    mutable sqlite::sqlite3* db = nullptr;
    clock_type alive_time_ = 0;
    mutable string_view last_error_;

public:
    db_sqlite_connect() noexcept { sqlite::sqlite3_open(nullptr, &db); }
    ~db_sqlite_connect() noexcept override { this->close(); }

    bool connect_to(const _MSTL string&, const _MSTL string&,
        const _MSTL string& dbname, const _MSTL string&,
        uint32_t, const _MSTL string&) override;

    bool connect_to(const db_connect_config& config) override;
    MSTL_NODISCARD bool reset_connect(const db_connect_config& config) override;

    MSTL_NODISCARD bool set_character_set(const _MSTL string& encoding) const override;

    MSTL_NODISCARD string_view get_character_set() const override;
    MSTL_NODISCARD string_view get_error() const override;
    MSTL_NODISCARD uint32_t get_errno() const override { return db ? sqlite::sqlite3_errcode(db) : 0; }

    MSTL_NODISCARD bool update(const string& sql) const override;
    MSTL_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const override;

    MSTL_NODISCARD bool connected() const override { return db != nullptr; }
    MSTL_NODISCARD bool is_valid() const override;

    void close() noexcept override { if (db) sqlite::sqlite3_close(db); }
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    MSTL_NODISCARD clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }

private:
    bool connect_to_file(const string& file_path);
};

class MSTL_API db_sqlite_factory final : public idb_factory {
public:
    explicit db_sqlite_factory(db_connect_config config)
    : idb_factory(_MSTL move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override {
        return new db_sqlite_result(static_cast<sqlite::sqlite3_stmt*>(native_result));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_SQLITE3__
#endif // MSTL_DB_SQLITE_HPP__
