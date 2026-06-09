#include <NeForce/db/pgsql/pgsql_connect.hpp>
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#    include <NeForce/core/utility/packages.hpp>
#    include <NeForce/db/pgsql/pgsql_prepared_statement.hpp>
#    include <NeForce/db/pgsql/pgsql_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

namespace {
    string build_conn_string(const db_config& config) {
        string result;

        if (!config.host.empty()) {
            result += "host=" + config.host + " ";
        }
        if (config.port) {
            result += "port=" + to_string(config.port.value()) + " ";
        }
        if (!config.database.empty()) {
            result += "dbname=" + config.database + " ";
        }
        if (!config.username.empty()) {
            result += "user=" + config.username + " ";
        }
        if (!config.password.empty()) {
            result += "password=" + config.password + " ";
        }
        if (!config.charset.empty()) {
            result += "client_encoding=" + config.charset;
        }
        return result;
    }
} // namespace

bool pgsql_connect::connect(const db_config& config) {
    last_error_.clear();
    last_errno_ = 0;
    close();

    string conn_str = build_conn_string(config);
    link_ = ::PQconnectdb(conn_str.data());

    if (link_ == nullptr || ::PQstatus(link_) != ::CONNECTION_OK) {
        string last_error = ::PQerrorMessage(link_);
        close();
        last_error_ = move(last_error);
        last_errno_ = 1;
        return false;
    }

    refresh_alive();
    return true;
}

bool pgsql_connect::reconnect(const db_config& config) {
    close();
    return connect(config);
}

void pgsql_connect::close() {
    if (link_ != nullptr) {
        ::PQfinish(link_);
        link_ = nullptr;
    }
}

bool pgsql_connect::set_character_set(const string& encoding) {
    if (link_ == nullptr) {
        return false;
    }

    ::PGresult* res = ::PQexec(link_, ("SET client_encoding TO " + encoding).data());
    if (res == nullptr) {
        return false;
    }

    const ::ExecStatusType status = ::PQresultStatus(res);
    ::PQclear(res);
    return status == ::PGRES_COMMAND_OK;
}

string_view pgsql_connect::get_character_set() const {
    if (link_ == nullptr) {
        return {};
    }

    ::PGresult* res = ::PQexec(link_, "SHOW client_encoding");
    if (res == nullptr) {
        return {};
    }

    if (::PQresultStatus(res) != ::PGRES_TUPLES_OK) {
        ::PQclear(res);
        return {};
    }

    const char* encoding = ::PQgetvalue(res, 0, 0);
    const string_view ret = encoding != nullptr ? string_view(encoding) : ""_sv;
    ::PQclear(res);
    return ret;
}

bool pgsql_connect::update(const string& sql) const {
    if (link_ == nullptr) {
        return false;
    }

    ::PGresult* res = ::PQexec(link_, sql.data());
    if (res == nullptr) {
        return false;
    }

    const ::ExecStatusType status = ::PQresultStatus(res);
    ::PQclear(res);
    return status == ::PGRES_COMMAND_OK;
}

bool pgsql_connect::connected() const { return link_ != nullptr && ::PQstatus(link_) == ::CONNECTION_OK; }

unique_ptr<idb_tb_result> pgsql_connect::query(const string& sql) const {
    if (link_ == nullptr) {
        return nullptr;
    }

    ::PGresult* res = ::PQexec(link_, sql.data());
    if (res == nullptr) {
        return nullptr;
    }

    if (::PQresultStatus(res) != ::PGRES_TUPLES_OK) {
        ::PQclear(res);
        return nullptr;
    }
    return make_unique<pgsql_tb_result>(res, true);
}

unique_ptr<idb_prepared_statement> pgsql_connect::prepare_statement(const string& sql) const {
    if (link_ == nullptr) {
        return nullptr;
    }

    auto* const stmt = new pgsql_prepared_statement(link_, sql);

    if (stmt->param_count() == 0U && sql.contains('$')) {
        delete stmt;
        return nullptr;
    }
    return {stmt};
}

size_t pgsql_connect::batch_insert(const string& table, const vector<string>& columns,
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

    size_t param_idx = 1;
    for (size_t r = 0; r < rows.size(); ++r) {
        if (r > 0) {
            sql += ", ";
        }
        sql += "(";
        for (size_t c = 0; c < columns.size(); ++c) {
            if (c > 0) {
                sql += ", ";
            }
            sql += "$" + to_string(param_idx++);
        }
        sql += ")";
    }

    auto stmt = prepare_statement(sql);
    if (stmt == nullptr) {
        return 0;
    }

    param_idx = 1;
    for (const auto& row: rows) {
        for (const auto& val: row) {
            stmt->bind_param(param_idx++, val);
        }
    }

    return stmt->execute() ? rows.size() : 0;
}

bool pgsql_connect::table_exists(const string& table) const {
    auto result = query("SELECT 1 FROM information_schema.tables WHERE table_name = '" + table + "'");
    return result != nullptr && result->next();
}

idb_connect* pgsql_factory::create_connect() {
    auto* const conn = new pgsql_connect();
    if (!conn->connect(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* pgsql_factory::create_result(void* native_result) {
    if (native_result == nullptr) {
        return nullptr;
    }
    auto* const res = static_cast<::PGresult*>(native_result);
    return new pgsql_tb_result(res, false);
}

NEFORCE_END_NAMESPACE__
#endif
