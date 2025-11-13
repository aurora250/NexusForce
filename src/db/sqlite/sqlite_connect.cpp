#include <MSTL/db/sqlite/sqlite_connect.hpp>
#ifdef MSTL_SUPPORT_SQLITE3__
#include <MSTL/db/sqlite/sqlite_prepared_statement.hpp>
#include <MSTL/db/sqlite/sqlite_result.hpp>
MSTL_BEGIN_NAMESPACE__

bool sqlite_connect::connect_to(const _MSTL string&, const _MSTL string&,
        const _MSTL string& dbname, const _MSTL string&,
        uint32_t, const _MSTL string&) {
    _MSTL_SQLITE sqlite3_open(dbname.c_str(), &db);
    return connect_to_file(dbname);
}

bool sqlite_connect::connect_to(const db_config& config) {
    _MSTL_SQLITE sqlite3_open(config.database.c_str(), &db);
    return connect_to_file(config.database);
}

bool sqlite_connect::set_character_set(const _MSTL string& encoding) const {
    const string sql = "PRAGMA encoding = '" + encoding + "';";
    return update(sql);
}

_MSTL string_view sqlite_connect::get_character_set() const {
    _MSTL_SQLITE sqlite3_stmt* stmt = nullptr;
    if (_MSTL_SQLITE sqlite3_prepare_v2(db, "PRAGMA encoding;", -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    string_view encoding;
    if (_MSTL_SQLITE sqlite3_step(stmt) == SQLITE_ROW) {
        encoding = reinterpret_cast<const char*>(_MSTL_SQLITE sqlite3_column_text(stmt, 0));
    }
    _MSTL_SQLITE sqlite3_finalize(stmt);
    return encoding;
}

_MSTL string_view sqlite_connect::get_error() const {
    if (db) last_error_ = _MSTL_SQLITE sqlite3_errmsg(db);
    return last_error_;
}

bool sqlite_connect::update(const string& sql) const {
    if (!connected()) return false;

    char* error_msg = nullptr;
    if (_MSTL_SQLITE sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_msg) != SQLITE_OK) {
        if (error_msg) {
            last_error_ = error_msg;
            _MSTL_SQLITE sqlite3_free(error_msg);
        }
        return false;
    }
    return true;
}

unique_ptr<idb_tb_result> sqlite_connect::query(const string& sql) const {
    if (!connected()) return {};

    _MSTL_SQLITE sqlite3_stmt* stmt = nullptr;
    if (_MSTL_SQLITE sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    return make_unique<sqlite_result>(stmt);
}

unique_ptr<idb_prepared_statement> sqlite_connect::prepare_statement(const string& sql) const {
    return make_unique<sqlite_prepared_statement>(db, sql);
}

bool sqlite_connect::is_valid() const {
    if (!connected()) return false;
    _MSTL_SQLITE sqlite3_stmt* stmt = nullptr;
    if (_MSTL_SQLITE sqlite3_prepare_v2(db, "SELECT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        _MSTL_SQLITE sqlite3_finalize(stmt);
        return true;
    }
    return false;
}

bool sqlite_connect::reset_connect(const db_config& config) {
    if (connected()) {
        _MSTL_SQLITE sqlite3_close(db);
        return connect_to(config);
    }
    return false;
}

bool sqlite_connect::connect_to_file(const string& file_path) {
    if (connected()) {
        close();
    }

    if (_MSTL_SQLITE sqlite3_open(file_path.c_str(), &db) != SQLITE_OK) {
        last_error_ = _MSTL_SQLITE sqlite3_errmsg(db);
        close();
        return false;
    }
    return true;
}

idb_connect* sqlite_factory::create_connect() {
    auto conn = new sqlite_connect();
    if (!conn->connect_to(config_)) {
        return nullptr;
    }
    return conn;
}

idb_result* sqlite_factory::create_result(void* native_result) {
    return new sqlite_result(static_cast<_MSTL_SQLITE sqlite3_stmt*>(native_result));
}

MSTL_END_NAMESPACE__
#endif // MSTL_SUPPORT_SQLITE3__
