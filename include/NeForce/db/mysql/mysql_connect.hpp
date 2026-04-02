#ifndef NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
#define NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <mysql/mysql.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API mysql_connect final : idb_tb_connect {
private:
    ::MYSQL* link_ = nullptr;

public:
    mysql_connect() noexcept { link_ = ::mysql_init(nullptr); }
    ~mysql_connect() noexcept override { this->close(); }

    NEFORCE_NODISCARD bool connect(const db_config& config) noexcept override;
    NEFORCE_NODISCARD bool reconnect(const db_config& config) override;
    void close() noexcept override;

    NEFORCE_NODISCARD bool set_character_set(const string& encoding) const noexcept override;
    NEFORCE_NODISCARD bool set_options(::mysql_option option, const string& str) const noexcept;

    NEFORCE_NODISCARD string_view get_character_set() const noexcept override;
    NEFORCE_NODISCARD string_view get_error() const noexcept override;
    NEFORCE_NODISCARD uint32_t get_errno() const noexcept override;

    NEFORCE_NODISCARD bool update(const string& sql) const noexcept override;
    NEFORCE_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const noexcept override;
    NEFORCE_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    NEFORCE_NODISCARD bool connected() const noexcept override { return link_ != nullptr; }
    NEFORCE_NODISCARD bool is_valid() const noexcept override { return ::mysql_ping(link_) == 0; }
};

class NEFORCE_API mysql_factory final : public idb_factory {
public:
    explicit mysql_factory(db_config config) :
    idb_factory(move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
#endif // NEFORCE_DATABASE_MYSQL_CONNECT_HPP__
