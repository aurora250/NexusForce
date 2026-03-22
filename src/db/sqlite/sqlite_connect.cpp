#include <NeForce/db/sqlite/sqlite_connect.hpp>
#ifdef NEFORCE_SUPPORT_SQLITE3
#include <NeForce/db/sqlite/sqlite_prepared_statement.hpp>
#include <NeForce/db/sqlite/sqlite_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool sqlite_connect::connect(const db_config& config) {
    if (connected()) {
        close();
    }
    if (::sqlite3_open(config.database.data(), &link_) != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(link_);
        close();
        return false;
    }
    return true;
}

bool sqlite_connect::reconnect(const db_config& config) {
    if (connected()) {
        ::sqlite3_close(link_);
        return connect(config);
    }
    return false;
}

void sqlite_connect::close() noexcept {
    if (link_) {
        ::sqlite3_close(link_);
    }
}

bool sqlite_connect::set_character_set(const string& encoding) const {
    const string sql = "PRAGMA encoding = '" + encoding + "';";
    return update(sql);
}

string_view sqlite_connect::get_character_set() const {
    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(link_, "PRAGMA encoding;", -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    string_view encoding;
    if (::sqlite3_step(stmt) == SQLITE_ROW) {
        encoding = reinterpret_cast<const char*>(::sqlite3_column_text(stmt, 0));
    }
    ::sqlite3_finalize(stmt);
    return encoding;
}

string_view sqlite_connect::get_error() const {
    if (link_) last_error_ = ::sqlite3_errmsg(link_);
    return last_error_.view();
}

bool sqlite_connect::update(const string& sql) const {
    if (!connected()) return false;

    char* error_msg = nullptr;
    if (::sqlite3_exec(link_, sql.data(), nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (error_msg) {
            last_error_ = error_msg;
            ::sqlite3_free(error_msg);
        }
        return false;
    }
    return true;
}

unique_ptr<idb_tb_result> sqlite_connect::query(const string& sql) const {
    if (!connected()) return {};

    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(link_, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    return make_unique<sqlite_result>(stmt);
}

unique_ptr<idb_prepared_statement> sqlite_connect::prepare_statement(const string& sql) const {
    return make_unique<sqlite_prepared_statement>(link_, sql);
}

bool sqlite_connect::is_valid() const {
    if (!connected()) return false;
    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(link_, "SELECT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        ::sqlite3_finalize(stmt);
        return true;
    }
    return false;
}

idb_connect* sqlite_factory::create_connect() {
    auto conn = new sqlite_connect();
    if (!conn->connect(config_)) {
        return nullptr;
    }
    return conn;
}

idb_result* sqlite_factory::create_result(void* native_result) {
    return new sqlite_result(static_cast<::sqlite3_stmt*>(native_result));
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_SQLITE3
