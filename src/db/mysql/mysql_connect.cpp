#include <NeForce/db/mysql/mysql_connect.hpp>
#ifdef NEFORCE_SUPPORT_MYSQL
#    include <NeForce/db/mysql/mysql_prepared_statement.hpp>
#    include <NeForce/db/mysql/mysql_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

bool mysql_connect::connect(const db_config& config) {
    last_error_.clear();
    last_errno_ = 0;
    const ::MYSQL* p = ::mysql_real_connect(link_, config.host.data(), config.username.data(), config.password.data(),
                                            config.database.data(), static_cast<int>(config.port.value()), nullptr, 0);
    if (p == nullptr) {
        last_error_ = ::mysql_error(link_);
        last_errno_ = ::mysql_errno(link_);
        return false;
    }
    ignore = set_character_set(config.charset);
    return true;
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
        link_ = nullptr;
    }
}

bool mysql_connect::set_character_set(const string& encoding) noexcept {
    return connected() && ::mysql_set_character_set(link_, encoding.data()) == 0;
}

bool mysql_connect::set_options(const ::mysql_option option, const string& str) const noexcept {
    return connected() && ::mysql_options(link_, option, str.data()) == 0;
}

string_view mysql_connect::get_character_set() const noexcept { return ::mysql_character_set_name(link_); }

string_view mysql_connect::get_error() const noexcept { return last_error_.view(); }

uint32_t mysql_connect::get_errno() const noexcept { return last_errno_; }

bool mysql_connect::update(const string& sql) const {
    if (::mysql_query(link_, sql.data()) != 0) {
        last_error_ = ::mysql_error(link_);
        last_errno_ = ::mysql_errno(link_);
        return false;
    }
    return true;
}

unique_ptr<idb_tb_result> mysql_connect::query(const string& sql) const {
    if (::mysql_query(link_, sql.data()) != 0) {
        last_error_ = ::mysql_error(link_);
        last_errno_ = ::mysql_errno(link_);
        return {};
    }
    return make_unique<mysql_result>(::mysql_store_result(link_));
}

unique_ptr<idb_prepared_statement> mysql_connect::prepare_statement(const string& sql) const {
    return make_unique<mysql_prepared_statement>(link_, sql.view());
}

size_t mysql_connect::batch_insert(const string& table, const vector<string>& columns,
                                   const vector<vector<string>>& rows) {
    if (rows.empty() || columns.empty()) {
        return 0;
    }

    string sql;
    sql.reserve(table.size() + columns.size() * 32 + rows.size() * columns.size() * 32);
    sql += "INSERT INTO " + table + " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) {
            sql += ", ";
        }
        sql += columns[i];
    }
    sql += ") VALUES ";

    for (size_t r = 0; r < rows.size(); ++r) {
        if (r > 0) {
            sql += ", ";
        }
        sql += "(";
        for (size_t c = 0; c < columns.size(); ++c) {
            if (c > 0) {
                sql += ", ";
            }
            sql += "'" + rows[r][c] + "'";
        }
        sql += ")";
    }

    return update(sql) ? rows.size() : 0;
}

idb_connect* mysql_factory::create_connect() {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto* conn = new mysql_connect();
    if (!conn->connect(config_)) {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* mysql_factory::create_result(void* native_result) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new mysql_result(static_cast<::MYSQL_RES*>(native_result));
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_SUPPORT_MYSQL
