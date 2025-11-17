#ifndef MSTL_SQLITE_CONNECT_HPP__
#define MSTL_SQLITE_CONNECT_HPP__
#ifdef MSTL_SUPPORT_SQLITE3__
#include "MSTL/database/db_interface.hpp"
#include "MSTL/database/sqlite/sqlite_config.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API sqlite_connect final : idb_tb_connect {
private:
    mutable _MSTL_SQLITE sqlite3* db = nullptr;
    clock_type alive_time_ = 0;
    mutable string_view last_error_;

public:
    sqlite_connect() noexcept { _MSTL_SQLITE sqlite3_open(nullptr, &db); }
    ~sqlite_connect() noexcept override { this->close(); }

    bool connect_to(const _MSTL string&, const _MSTL string&,
        const _MSTL string& dbname, const _MSTL string&,
        uint32_t, const _MSTL string&) override;

    bool connect_to(const db_config& config) override;
    MSTL_NODISCARD bool reset_connect(const db_config& config) override;

    MSTL_NODISCARD bool set_character_set(const _MSTL string& encoding) const override;

    MSTL_NODISCARD string_view get_character_set() const override;
    MSTL_NODISCARD string_view get_error() const override;
    MSTL_NODISCARD uint32_t get_errno() const override { return db ? _MSTL_SQLITE sqlite3_errcode(db) : 0; }

    MSTL_NODISCARD bool update(const string& sql) const override;
    MSTL_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const override;
    MSTL_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    MSTL_NODISCARD bool connected() const override { return db != nullptr; }
    MSTL_NODISCARD bool is_valid() const override;

    void close() noexcept override { if (db) _MSTL_SQLITE sqlite3_close(db); }
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    MSTL_NODISCARD clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }

private:
    bool connect_to_file(const string& file_path);
};

class MSTL_API sqlite_factory final : public idb_factory {
public:
    explicit sqlite_factory(db_config config)
    : idb_factory(_MSTL move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_SQLITE_CONNECT_HPP__
