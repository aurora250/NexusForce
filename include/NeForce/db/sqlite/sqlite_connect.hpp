#ifndef NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
#define NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
#ifdef NEFORCE_SUPPORT_SQLITE3
#include "NeForce/db/db_interface.hpp"
#include <sqlite3.h>
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API sqlite_connect final : idb_tb_connect {
private:
    mutable ::sqlite3* db = nullptr;
    clock_type alive_time_ = 0;
    mutable string_view last_error_;

public:
    sqlite_connect() noexcept { ::sqlite3_open(nullptr, &db); }
    ~sqlite_connect() noexcept override { this->close(); }

    bool connect_to(const _NEFORCE string&, const _NEFORCE string&,
        const _NEFORCE string& dbname, const _NEFORCE string&,
        uint32_t, const _NEFORCE string&) override;

    bool connect_to(const db_config& config) override;
    NEFORCE_NODISCARD bool reset_connect(const db_config& config) override;

    NEFORCE_NODISCARD bool set_character_set(const _NEFORCE string& encoding) const override;

    NEFORCE_NODISCARD string_view get_character_set() const override;
    NEFORCE_NODISCARD string_view get_error() const override;
    NEFORCE_NODISCARD uint32_t get_errno() const override { return db ? ::sqlite3_errcode(db) : 0; }

    NEFORCE_NODISCARD bool update(const string& sql) const override;
    NEFORCE_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const override;
    NEFORCE_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    NEFORCE_NODISCARD bool connected() const override { return db != nullptr; }
    NEFORCE_NODISCARD bool is_valid() const override;

    void close() noexcept override { if (db) ::sqlite3_close(db); }
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    NEFORCE_NODISCARD clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }

private:
    bool connect_to_file(const string& file_path);
};

class NEFORCE_API sqlite_factory final : public idb_factory {
public:
    explicit sqlite_factory(db_config config)
    : idb_factory(_NEFORCE move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
