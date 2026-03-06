#include <NeForce/db/mysql/mysql_connect.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
#include <NeForce/db/mysql/mysql_prepared_statement.hpp>
#include <NeForce/db/mysql/mysql_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool mysql_connect::connect_to(
        const _NEFORCE string& user, const _NEFORCE string& password,
        const _NEFORCE string& dbname, const _NEFORCE string& ip,
        const uint32_t port, const _NEFORCE string& character_set) noexcept {
    const ::MYSQL* p = ::mysql_real_connect(mysql_, ip.data(), user.data(),
        password.data(), dbname.data(), port, nullptr, 0);
    if (p == nullptr) return false;
    return this->set_character_set(character_set);
}

unique_ptr<idb_tb_result> mysql_connect::query(const _NEFORCE string& sql) const noexcept {
    if (::mysql_query(mysql_, sql.data())) return {};
    return make_unique<mysql_result>(::mysql_store_result(mysql_));
}

unique_ptr<idb_prepared_statement> mysql_connect::prepare_statement(const string& sql) const {
    return _NEFORCE make_unique<mysql_prepared_statement>(mysql_, sql);
}

bool mysql_connect::reset_connect(const db_config& config) {
    if (connected()) {
        ::mysql_close(mysql_);
        mysql_ = ::mysql_init(nullptr);
        return connect_to(config);
    }
    return false;
}

idb_connect* mysql_factory::create_connect() {
    const auto conn = new mysql_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* mysql_factory::create_result(void* native_result) {
    return new mysql_result(static_cast<::MYSQL_RES*>(native_result));
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
