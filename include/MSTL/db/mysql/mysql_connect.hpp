#ifndef MSTL_DB_MYSQL_CONNECT_HPP__
#define MSTL_DB_MYSQL_CONNECT_HPP__
#ifdef MSTL_SUPPORT_MYSQL__
#include "mysql_result.hpp"
#include "mysql_prepared_statement.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API db_mysql_connect final : idb_tb_connect {
private:
    _MSTL_MYSQL MYSQL* mysql_ = nullptr;
    clock_type alive_time_ = 0;

public:
    db_mysql_connect() noexcept { mysql_ = _MSTL_MYSQL mysql_init(nullptr); }
    ~db_mysql_connect() noexcept override { this->close(); }

    MSTL_NODISCARD bool connect_to(
        const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        uint32_t port, const _MSTL string& character_set) noexcept override;

    MSTL_NODISCARD bool connect_to(const db_config& config) noexcept override {
        return connect_to(
            config.username,
            config.password,
            config.database,
            config.host,
            config.port,
            config.charset
        );
    }
    MSTL_NODISCARD bool reset_connect(const db_config& config) override;

    MSTL_NODISCARD bool set_character_set(const _MSTL string& encoding) const noexcept override {
        return connected() && !_MSTL_MYSQL mysql_set_character_set(mysql_, encoding.data());
    }
    MSTL_NODISCARD bool set_options(const _MSTL_MYSQL mysql_option option, const _MSTL string& str) const noexcept {
        return connected() && !_MSTL_MYSQL mysql_options(mysql_, option, str.data());
    }

    MSTL_NODISCARD string_view get_character_set() const noexcept override {
        return _MSTL_MYSQL mysql_character_set_name(mysql_);
    }
    MSTL_NODISCARD string_view get_error() const noexcept override {
        return _MSTL_MYSQL mysql_error(mysql_);
    }
    MSTL_NODISCARD uint32_t get_errno() const noexcept override {
        return _MSTL_MYSQL mysql_errno(mysql_);
    }

    MSTL_NODISCARD bool update(const _MSTL string& sql) const noexcept override {
        return !_MSTL_MYSQL mysql_query(mysql_, sql.c_str());
    }
    MSTL_NODISCARD unique_ptr<idb_tb_result> query(const _MSTL string& sql) const noexcept override;

    MSTL_NODISCARD unique_ptr<mysql_prepared_statement> prepare_statement(const string& sql) const {
        return _MSTL make_unique<mysql_prepared_statement>(mysql_, sql);
    }


    MSTL_NODISCARD bool connected() const noexcept override { return mysql_ != nullptr; }
    MSTL_NODISCARD bool is_valid() const noexcept override { return mysql_ping(mysql_) == 0; }

    void close() noexcept override { if (connected()) mysql_close(mysql_); }
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    MSTL_NODISCARD clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }
};

class MSTL_API db_mysql_factory final : public idb_factory {
public:
    explicit db_mysql_factory(db_config config)
    : idb_factory(_MSTL move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override {
        return new db_mysql_result(static_cast<_MSTL_MYSQL MYSQL_RES*>(native_result));
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
#endif // MSTL_DB_MYSQL_CONNECT_HPP__
