#include <NeForce/db/sqlite/sqlite_connect.hpp>
#ifdef NEFORCE_SUPPORT_SQLITE3
#    include <NeForce/db/sqlite/sqlite_prepared_statement.hpp>
#    include <NeForce/db/sqlite/sqlite_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool sqlite_connect::connect(const db_config& config) {
    last_error_.clear();
    last_errno_ = 0;
    if (connected()) {
        close();
    }
    if (::sqlite3_open(config.database.data(), &link_) != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(link_);
        last_errno_ = ::sqlite3_errcode(link_);
        close();
        return false;
    }
    return true;
}

bool sqlite_connect::reconnect(const db_config& config) {
    close();
    return connect(config);
}

#    ifdef NEFORCE_SUPPORT_SQLCIPHER

bool sqlite_connect::connect(const db_config& config, const string& encryption_key, const key_type type) {
    last_error_.clear();
    last_errno_ = 0;
    if (connected()) {
        close();
    }
    if (::sqlite3_open(config.database.data(), &link_) != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(link_);
        last_errno_ = ::sqlite3_errcode(link_);
        close();
        return false;
    }
    if (!encryption_key.empty()) {
        if (type == key_type::RAW) {
            ::sqlite3_exec(link_, "PRAGMA cipher_kdf_iter = 1;", nullptr, nullptr, nullptr);
        }
        const int rc = ::sqlite3_key_v2(link_, "main", encryption_key.data(), static_cast<int>(encryption_key.size()));
        if (rc != SQLITE_OK) {
            last_error_ = ::sqlite3_errmsg(link_);
            last_errno_ = ::sqlite3_errcode(link_);
            close();
            return false;
        }
    }
    return true;
}

bool sqlite_connect::reconnect(const db_config& config, const string& encryption_key, const key_type type) {
    close();
    return connect(config, encryption_key, type);
}

bool sqlite_connect::rekey(const string& new_key, const key_type type) {
    if (link_ == nullptr) {
        return false;
    }
    if (type == key_type::RAW) {
        ::sqlite3_exec(link_, "PRAGMA cipher_kdf_iter = 1;", nullptr, nullptr, nullptr);
    }
    const int rc = ::sqlite3_rekey_v2(link_, "main", new_key.data(), static_cast<int>(new_key.size()));
    if (rc != SQLITE_OK) {
        last_error_ = ::sqlite3_errmsg(link_);
        last_errno_ = ::sqlite3_errcode(link_);
        return false;
    }
    return true;
}

#    endif

void sqlite_connect::close() noexcept {
    if (link_ != nullptr) {
        ::sqlite3_close(link_);
        link_ = nullptr;
    }
}

bool sqlite_connect::set_character_set(const string& encoding) {
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

string_view sqlite_connect::get_error() const { return last_error_.view(); }

bool sqlite_connect::update(const string& sql) const {
    if (!connected()) {
        return false;
    }

    char* error_msg = nullptr;
    if (::sqlite3_exec(link_, sql.data(), nullptr, nullptr, &error_msg) != SQLITE_OK) {
        last_errno_ = ::sqlite3_errcode(link_);
        if (error_msg != nullptr) {
            last_error_ = error_msg;
            ::sqlite3_free(error_msg);
        }
        return false;
    }
    return true;
}

unique_ptr<idb_tb_result> sqlite_connect::query(const string& sql) const {
    if (!connected()) {
        return {};
    }

    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(link_, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }
    return make_unique<sqlite_result>(stmt);
}

unique_ptr<idb_prepared_statement> sqlite_connect::prepare_statement(const string& sql) const {
    return make_unique<sqlite_prepared_statement>(link_, sql);
}

bool sqlite_connect::table_exists(const string& table) const {
    auto result = query(table_exists_query(table));
    return result != nullptr && result->next();
}

bool sqlite_connect::is_valid() const {
    if (!connected()) {
        return false;
    }
    ::sqlite3_stmt* stmt = nullptr;
    if (::sqlite3_prepare_v2(link_, "SELECT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        ::sqlite3_finalize(stmt);
        return true;
    }
    last_error_ = ::sqlite3_errmsg(link_);
    last_errno_ = ::sqlite3_errcode(link_);
    return false;
}

size_t sqlite_connect::batch_insert(const string& table, const vector<string>& columns,
                                    const vector<vector<string>>& rows) {
    if (rows.empty() || columns.empty()) {
        return 0;
    }

    begin();
    auto stmt = prepare_statement("INSERT INTO " + table + " (" + string::join(columns, ", ") + ") VALUES (" +
                                  string::join(vector<string>(columns.size(), "?"), ", ") + ")");

    if (stmt == nullptr) {
        rollback();
        return 0;
    }

    size_t inserted = 0;
    for (const auto& row: rows) {
        for (size_t c = 0; c < columns.size(); ++c) {
            stmt->bind_param(static_cast<uint32_t>(c + 1), row[c]);
        }
        if (stmt->execute()) {
            ++inserted;
        }
    }
    commit();
    return inserted;
}

idb_connect* sqlite_factory::create_connect() {
    auto* conn = new sqlite_connect();
    if (!conn->connect(config_
#    ifdef NEFORCE_SUPPORT_SQLCIPHER
                       ,
                       encryption_key_, key_type_
#    endif
                       )) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* sqlite_factory::create_result(void* native_result) {
    return new sqlite_result(static_cast<::sqlite3_stmt*>(native_result));
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_SQLITE3
