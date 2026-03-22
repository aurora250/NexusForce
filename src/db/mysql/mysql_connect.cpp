#include <NeForce/db/mysql/mysql_connect.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
#include <NeForce/db/mysql/mysql_prepared_statement.hpp>
#include <NeForce/db/mysql/mysql_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool mysql_connect::connect(const db_config& config) noexcept {
    const ::MYSQL* p = ::mysql_real_connect(
        link_, config.host.data(), config.username.data(),
        config.password.data(), config.database.data(), config.port,
        nullptr, 0);
    if (p == nullptr) return false;
    return set_character_set(config.charset);
}

bool mysql_connect::reconnect(const db_config& config) {
    if (connected()) {
        ::mysql_close(link_);
        link_ = ::mysql_init(nullptr);
        return connect(config);
    }
    return false;
}

void mysql_connect::close() noexcept {
    if (connected()) {
        ::mysql_close(link_);
    }
}

bool mysql_connect::set_character_set(const string& encoding) const noexcept {
    return connected() && !::mysql_set_character_set(link_, encoding.data());
}

bool mysql_connect::set_options(const ::mysql_option option, const string& str) const noexcept {
    return connected() && !::mysql_options(link_, option, str.data());
}

string_view mysql_connect::get_character_set() const noexcept {
    return ::mysql_character_set_name(link_);
}

string_view mysql_connect::get_error() const noexcept {
    return ::mysql_error(link_);
}

uint32_t mysql_connect::get_errno() const noexcept {
    return ::mysql_errno(link_);
}

bool mysql_connect::update(const string& sql) const noexcept {
    return !::mysql_query(link_, sql.data());
}

unique_ptr<idb_tb_result> mysql_connect::query(const string& sql) const noexcept {
    if (::mysql_query(link_, sql.data())) return {};
    return make_unique<mysql_result>(::mysql_store_result(link_));
}

unique_ptr<idb_prepared_statement> mysql_connect::prepare_statement(const string& sql) const {
    return make_unique<mysql_prepared_statement>(link_, sql.view());
}

idb_connect* mysql_factory::create_connect() {
    const auto conn = new mysql_connect();
    if (!conn->connect(config_)) {
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
