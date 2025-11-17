#ifndef MSTL_POSTGRESQL_CONNECT_HPP__
#define MSTL_POSTGRESQL_CONNECT_HPP__
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include "../../core/config/undef_cmacro.hpp"
#include "MSTL/db/db_interface.hpp"
#include "postgresql_config.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API postgresql_connect final : public idb_tb_connect {
private:
    db_config config_{};
    string last_error_{};
    string charset_ = "utf8";
    clock_type last_alive_ = 0;
    uint32_t last_errno_ = 0;
    _MSTL_POSTGRESQL PGconn* conn_ = nullptr;

    void clear_error() noexcept;
    void set_error(string error, uint32_t errno_val = 0);
    void update_error();
    string build_conn_string(
        const string& user, const string& password,
        const string& dbname, const string& host,
        uint32_t port) const;

public:
    postgresql_connect() : last_alive_(std::clock()) {}
    ~postgresql_connect() override { close(); }

    postgresql_connect(const postgresql_connect&) = delete;
    postgresql_connect& operator=(const postgresql_connect&) = delete;

    MSTL_NODISCARD bool connect_to(
        const string& user, const string& password,
        const string& dbname, const string& ip,
        uint32_t port, const string& character_set) override;
    MSTL_NODISCARD bool connect_to(const db_config& config) override;

    bool set_character_set(const string& encoding) const override;
    MSTL_NODISCARD string_view get_character_set() const override;
    MSTL_NODISCARD string_view get_error() const override { return last_error_.view(); }
    MSTL_NODISCARD uint32_t get_errno() const override { return last_errno_; }

    bool update(const string& sql) const override;

    MSTL_NODISCARD bool connected() const override;
    MSTL_NODISCARD bool is_valid() const override { return connected(); }
    void close() override;

    void refresh_alive() override { last_alive_ = std::clock(); }
    MSTL_NODISCARD clock_type get_alive() const override { return last_alive_; }

    bool reset_connect(const db_config& config) override;

    unique_ptr<idb_tb_result> query(const string& sql) const override;
    unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;
};


class MSTL_API postgresql_factory final : public idb_factory {
public:
    explicit postgresql_factory(db_config config) : idb_factory(_MSTL move(config)) {}
    ~postgresql_factory() override = default;

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_POSTGRESQL_CONNECT_HPP__
