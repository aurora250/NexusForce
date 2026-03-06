#ifndef NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
#define NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
#ifdef NEFORCE_SUPPORT_MYSQL
#include "NeForce/db/db_interface.hpp"
#ifdef CR_OUT_OF_MEMORY
#undef CR_OUT_OF_MEMORY
#endif
#include <mysql/mysql.h>
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API mysql_connect final : idb_tb_connect {
private:
    ::MYSQL* mysql_ = nullptr;
    clock_type alive_time_ = 0;

public:
    mysql_connect() noexcept { mysql_ = ::mysql_init(nullptr); }
    ~mysql_connect() noexcept override { this->close(); }

    NEFORCE_NODISCARD bool connect_to(
        const _NEFORCE string& user, const _NEFORCE string& password,
        const _NEFORCE string& dbname, const _NEFORCE string& ip,
        uint32_t port, const _NEFORCE string& character_set) noexcept override;

    NEFORCE_NODISCARD bool connect_to(const db_config& config) noexcept override {
        return connect_to(
            config.username,
            config.password,
            config.database,
            config.host,
            config.port,
            config.charset
        );
    }
    NEFORCE_NODISCARD bool reset_connect(const db_config& config) override;

    NEFORCE_NODISCARD bool set_character_set(const _NEFORCE string& encoding) const noexcept override {
        return connected() && !::mysql_set_character_set(mysql_, encoding.data());
    }
    NEFORCE_NODISCARD bool set_options(const ::mysql_option option, const _NEFORCE string& str) const noexcept {
        return connected() && !::mysql_options(mysql_, option, str.data());
    }

    NEFORCE_NODISCARD string_view get_character_set() const noexcept override {
        return ::mysql_character_set_name(mysql_);
    }
    NEFORCE_NODISCARD string_view get_error() const noexcept override {
        return ::mysql_error(mysql_);
    }
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override {
        return ::mysql_errno(mysql_);
    }

    NEFORCE_NODISCARD bool update(const _NEFORCE string& sql) const noexcept override {
        return !::mysql_query(mysql_, sql.data());
    }
    NEFORCE_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const noexcept override;
    NEFORCE_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    NEFORCE_NODISCARD bool connected() const noexcept override { return mysql_ != nullptr; }
    NEFORCE_NODISCARD bool is_valid() const noexcept override { return mysql_ping(mysql_) == 0; }

    void close() noexcept override { if (connected()) mysql_close(mysql_); }
    void refresh_alive() noexcept override { alive_time_ = std::clock(); }
    NEFORCE_NODISCARD clock_type get_alive() const noexcept override { return std::clock() - alive_time_; }
};

class NEFORCE_API mysql_factory final : public idb_factory {
public:
    explicit mysql_factory(db_config config)
    : idb_factory(_NEFORCE move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
