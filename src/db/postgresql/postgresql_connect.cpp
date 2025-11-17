#include <MSTL/db/postgresql/postgresql_connect.hpp>
#ifdef MSTL_SUPPORT_POSTGRESQL__
#include <MSTL/db/postgresql/postgresql_result.hpp>
#include <MSTL/db/postgresql/postgresql_prepared_statement.hpp>
#include <MSTL/core/packages.hpp>
MSTL_BEGIN_NAMESPACE__

void postgresql_connect::clear_error() noexcept {
    last_error_.clear();
    last_errno_ = 0;
}

void postgresql_connect::set_error(string error, const uint32_t errno_val) {
    last_error_ = _MSTL move(error);
    last_errno_ = errno_val;
}

void postgresql_connect::update_error() {
    if (conn_) {
        last_error_ = PQerrorMessage(conn_);
        last_errno_ = 1;
    }
}

string postgresql_connect::build_conn_string(
    const string& user, const string& password,
    const string& dbname, const string& host,
    const uint32_t port) const {
    string result;

    if (!host.empty()) {
        result += "host=" + host + " ";
    }
    if (port > 0) {
        result += "port=" + _MSTL to_string(port) + " ";
    }
    if (!dbname.empty()) {
        result += "dbname=" + dbname + " ";
    }
    if (!user.empty()) {
        result += "user=" + user + " ";
    }
    if (!password.empty()) {
        result += "password=" + password + " ";
    }
    return result;
}

bool postgresql_connect::connect_to(
    const string& user, const string& password,
    const string& dbname, const string& ip,
    const uint32_t port, const string& character_set) {
    clear_error();
    close();

    string conn_str = build_conn_string(user, password, dbname, ip, port);

    if (!character_set.empty()) {
        conn_str += "client_encoding=" + character_set;
    }

    conn_ = _MSTL_POSTGRESQL PQconnectdb(conn_str.c_str());

    if (!conn_ || _MSTL_POSTGRESQL PQstatus(conn_) != _MSTL_POSTGRESQL CONNECTION_OK) {
        update_error();
        close();
        return false;
    }

    charset_ = character_set;
    refresh_alive();

    config_.username = user;
    config_.password = password;
    config_.database = dbname;
    config_.host = ip;
    config_.port = static_cast<uint16_t>(port);
    config_.charset = character_set;

    return true;
}

bool postgresql_connect::connect_to(const db_config& config) {
    const bool ok = connect_to(
        config.username, config.password, config.database,
        config.host, config.port, config.charset);
    if (ok) {
        config_ = config;
    }
    return ok;
}

bool postgresql_connect::set_character_set(const string& encoding) const {
    if (!conn_) return false;
    _MSTL_POSTGRESQL PGresult* res = _MSTL_POSTGRESQL PQexec(
        conn_, ("SET client_encoding TO " + encoding).c_str());
    if (!res) return false;
    const _MSTL_POSTGRESQL ExecStatusType status = _MSTL_POSTGRESQL PQresultStatus(res);
    _MSTL_POSTGRESQL PQclear(res);
    return status == _MSTL_POSTGRESQL PGRES_COMMAND_OK;
}

string_view postgresql_connect::get_character_set() const {
    if (!conn_) return {};
    _MSTL_POSTGRESQL PGresult* res = _MSTL_POSTGRESQL PQexec(conn_, "SHOW client_encoding");
    if (!res) return {};
    if (_MSTL_POSTGRESQL PQresultStatus(res) != _MSTL_POSTGRESQL PGRES_TUPLES_OK) {
        _MSTL_POSTGRESQL PQclear(res);
        return {};
    }
    char* encoding = _MSTL_POSTGRESQL PQgetvalue(res, 0, 0);
    const string_view ret = encoding ? string_view(encoding) : string_view{};
    _MSTL_POSTGRESQL PQclear(res);
    return ret;
}

bool postgresql_connect::update(const string& sql) const {
    if (!conn_) return false;
    _MSTL_POSTGRESQL PGresult* res = _MSTL_POSTGRESQL PQexec(conn_, sql.c_str());
    if (!res) return false;
    const _MSTL_POSTGRESQL ExecStatusType status = _MSTL_POSTGRESQL PQresultStatus(res);
    _MSTL_POSTGRESQL PQclear(res);
    return status == _MSTL_POSTGRESQL PGRES_COMMAND_OK;
}

bool postgresql_connect::connected() const {
    return conn_ != nullptr &&
        _MSTL_POSTGRESQL PQstatus(conn_) == _MSTL_POSTGRESQL CONNECTION_OK;
}

void postgresql_connect::close() {
    if (conn_) {
        _MSTL_POSTGRESQL PQfinish(conn_);
        conn_ = nullptr;
    }
    clear_error();
}

bool postgresql_connect::reset_connect(const db_config& config) {
    close();
    return connect_to(config);
}

unique_ptr<idb_tb_result> postgresql_connect::query(const string& sql) const {
    if (!conn_) return nullptr;
    _MSTL_POSTGRESQL PGresult* res = _MSTL_POSTGRESQL PQexec(conn_, sql.c_str());
    if (!res) return nullptr;
    if (_MSTL_POSTGRESQL PQresultStatus(res) != _MSTL_POSTGRESQL PGRES_TUPLES_OK) {
        _MSTL_POSTGRESQL PQclear(res);
        return nullptr;
    }
    return make_unique<postgresql_tb_result>(res, true);
}

unique_ptr<idb_prepared_statement> postgresql_connect::prepare_statement(const string& sql) const {
    if (!conn_) return nullptr;
    const auto stmt = new postgresql_prepared_statement(conn_, sql);
    if (!stmt->param_count() && sql.find('$') != string::npos) {
        delete stmt;
        return nullptr;
    }
    return unique_ptr<idb_prepared_statement>(stmt);
}

idb_connect* postgresql_factory::create_connect() {
    const auto conn = new postgresql_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* postgresql_factory::create_result(void* native_result) {
    if (!native_result) return nullptr;
    _MSTL_POSTGRESQL PGresult* res = static_cast<_MSTL_POSTGRESQL PGresult*>(native_result);
    return new postgresql_tb_result(res, false);
}

MSTL_END_NAMESPACE__
#endif
