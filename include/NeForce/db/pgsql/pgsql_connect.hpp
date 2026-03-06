#ifndef NEFORCE_DATABASE_PGSQL_CONNECT_HPP__
#define NEFORCE_DATABASE_PGSQL_CONNECT_HPP__
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#include "NeForce/db/db_interface.hpp"
#include <libpq-fe.h>
NEFORCE_BEGIN_NAMESPACE__

class NEFORCE_API pgsql_connect final : public idb_tb_connect {
private:
    db_config config_{};
    string last_error_{};
    string charset_ = "utf8";
    clock_type last_alive_ = 0;
    uint32_t last_errno_ = 0;
    ::PGconn* conn_ = nullptr;

    void clear_error() noexcept;
    void set_error(string error, uint32_t errno_val = 0);
    void update_error();
    string build_conn_string(
        const string& user, const string& password,
        const string& dbname, const string& host,
        uint32_t port) const;

public:
    pgsql_connect() : last_alive_(std::clock()) {}
    ~pgsql_connect() override { close(); }

    pgsql_connect(const pgsql_connect&) = delete;
    pgsql_connect& operator =(const pgsql_connect&) = delete;

    NEFORCE_NODISCARD bool connect_to(
        const string& user, const string& password,
        const string& dbname, const string& ip,
        uint32_t port, const string& character_set) override;
    NEFORCE_NODISCARD bool connect_to(const db_config& config) override;

    bool set_character_set(const string& encoding) const override;
    NEFORCE_NODISCARD string_view get_character_set() const override;
    NEFORCE_NODISCARD string_view get_error() const override { return last_error_.view(); }
    NEFORCE_NODISCARD uint32_t get_errno() const override { return last_errno_; }

    bool update(const string& sql) const override;

    NEFORCE_NODISCARD bool connected() const override;
    NEFORCE_NODISCARD bool is_valid() const override { return connected(); }
    void close() override;

    void refresh_alive() override { last_alive_ = std::clock(); }
    NEFORCE_NODISCARD clock_type get_alive() const override { return last_alive_; }

    bool reset_connect(const db_config& config) override;

    unique_ptr<idb_tb_result> query(const string& sql) const override;
    unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;
};


class NEFORCE_API pgsql_factory final : public idb_factory {
public:
    explicit pgsql_factory(db_config config) : idb_factory(_NEFORCE move(config)) {}
    ~pgsql_factory() override = default;

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_PGSQL_CONNECT_HPP__
