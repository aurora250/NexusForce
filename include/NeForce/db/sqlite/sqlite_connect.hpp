#ifndef NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
#define NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <sqlite3.h>
#    include "NeForce/db/db_interface.hpp"
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API sqlite_connect final : idb_tb_connect {
private:
    ::sqlite3* link_ = nullptr;
    mutable string last_error_;

    bool connect_to_file(const string& file_path);

public:
    sqlite_connect() noexcept { ::sqlite3_open(nullptr, &link_); }
    ~sqlite_connect() noexcept override { this->close(); }

    bool connect(const db_config& config) override;
    NEFORCE_NODISCARD bool reconnect(const db_config& config) override;
    void close() noexcept override;

    NEFORCE_NODISCARD bool set_character_set(const string& encoding) const override;

    NEFORCE_NODISCARD string_view get_character_set() const override;
    NEFORCE_NODISCARD string_view get_error() const override;
    NEFORCE_NODISCARD uint32_t get_errno() const override { return link_ ? ::sqlite3_errcode(link_) : 0; }

    NEFORCE_NODISCARD bool update(const string& sql) const override;
    NEFORCE_NODISCARD unique_ptr<idb_tb_result> query(const string& sql) const override;
    NEFORCE_NODISCARD unique_ptr<idb_prepared_statement> prepare_statement(const string& sql) const override;

    NEFORCE_NODISCARD bool connected() const override { return link_ != nullptr; }
    NEFORCE_NODISCARD bool is_valid() const override;
};

class NEFORCE_API sqlite_factory final : public idb_factory {
public:
    explicit sqlite_factory(db_config config) :
    idb_factory(move(config)) {}

    idb_connect* create_connect() override;
    idb_result* create_result(void* native_result) override;
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_SQLITE_CONNECT_HPP__
