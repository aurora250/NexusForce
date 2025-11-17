#include <MSTL/database/mysql/mysql_connect.hpp>
#ifdef MSTL_SUPPORT_MYSQL__
#include <MSTL/database/mysql/mysql_prepared_statement.hpp>
#include <MSTL/database/mysql/mysql_result.hpp>
MSTL_BEGIN_NAMESPACE__

bool mysql_connect::connect_to(
        const _MSTL string& user, const _MSTL string& password,
        const _MSTL string& dbname, const _MSTL string& ip,
        const uint32_t port, const _MSTL string& character_set) noexcept {
    const _MSTL_MYSQL MYSQL* p = _MSTL_MYSQL mysql_real_connect(mysql_, ip.c_str(), user.c_str(),
        password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p == nullptr) return false;
    return this->set_character_set(character_set);
}

unique_ptr<idb_tb_result> mysql_connect::query(const _MSTL string& sql) const noexcept {
    if (_MSTL_MYSQL mysql_query(mysql_, sql.c_str())) return {};
    return make_unique<mysql_result>(_MSTL_MYSQL mysql_store_result(mysql_));
}

unique_ptr<idb_prepared_statement> mysql_connect::prepare_statement(const string& sql) const {
    return _MSTL make_unique<mysql_prepared_statement>(mysql_, sql);
}

bool mysql_connect::reset_connect(const db_config& config) {
    if (connected()) {
        _MSTL_MYSQL mysql_close(mysql_);
        mysql_ = _MSTL_MYSQL mysql_init(nullptr);
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
    return new mysql_result(static_cast<_MSTL_MYSQL MYSQL_RES*>(native_result));
}

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_MYSQL__
