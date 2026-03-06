#include <NeForce/db/pgsql/pgsql_connect.hpp>
#ifdef NEFORCE_SUPPORT_POSTGRESQL
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/db/pgsql/pgsql_prepared_statement.hpp>
#include <NeForce/db/pgsql/pgsql_result.hpp>
NEFORCE_BEGIN_NAMESPACE__

void pgsql_connect::clear_error() noexcept {
    last_error_.clear();
    last_errno_ = 0;
}

void pgsql_connect::set_error(string error, const uint32_t errno_val) {
    last_error_ = _NEFORCE move(error);
    last_errno_ = errno_val;
}

void pgsql_connect::update_error() {
    if (conn_) {
        last_error_ = PQerrorMessage(conn_);
        last_errno_ = 1;
    }
}

string pgsql_connect::build_conn_string(
    const string& user, const string& password,
    const string& dbname, const string& host,
    const uint32_t port) const {
    string result;

    if (!host.empty()) {
        result += "host=" + host + " ";
    }
    if (port > 0) {
        result += "port=" + _NEFORCE to_string(port) + " ";
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

bool pgsql_connect::connect_to(
    const string& user, const string& password,
    const string& dbname, const string& ip,
    const uint32_t port, const string& character_set) {
    clear_error();
    close();

    string conn_str = build_conn_string(user, password, dbname, ip, port);

    if (!character_set.empty()) {
        conn_str += "client_encoding=" + character_set;
    }

    conn_ = ::PQconnectdb(conn_str.data());

    if (!conn_ || ::PQstatus(conn_) != ::CONNECTION_OK) {
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

bool pgsql_connect::connect_to(const db_config& config) {
    const bool ok = connect_to(
        config.username, config.password, config.database,
        config.host, config.port, config.charset);
    if (ok) {
        config_ = config;
    }
    return ok;
}

bool pgsql_connect::set_character_set(const string& encoding) const {
    if (!conn_) return false;
    ::PGresult* res = ::PQexec(
        conn_, ("SET client_encoding TO " + encoding).data());
    if (!res) return false;
    const ::ExecStatusType status = ::PQresultStatus(res);
    ::PQclear(res);
    return status == ::PGRES_COMMAND_OK;
}

string_view pgsql_connect::get_character_set() const {
    if (!conn_) return {};
    ::PGresult* res = ::PQexec(conn_, "SHOW client_encoding");
    if (!res) return {};
    if (::PQresultStatus(res) != ::PGRES_TUPLES_OK) {
        ::PQclear(res);
        return {};
    }
    char* encoding = ::PQgetvalue(res, 0, 0);
    const string_view ret = encoding ? string_view(encoding) : string_view{};
    ::PQclear(res);
    return ret;
}

bool pgsql_connect::update(const string& sql) const {
    if (!conn_) return false;
    ::PGresult* res = ::PQexec(conn_, sql.data());
    if (!res) return false;
    const ::ExecStatusType status = ::PQresultStatus(res);
    ::PQclear(res);
    return status == ::PGRES_COMMAND_OK;
}

bool pgsql_connect::connected() const {
    return conn_ != nullptr &&
        ::PQstatus(conn_) == ::CONNECTION_OK;
}

void pgsql_connect::close() {
    if (conn_) {
        ::PQfinish(conn_);
        conn_ = nullptr;
    }
    clear_error();
}

bool pgsql_connect::reset_connect(const db_config& config) {
    close();
    return connect_to(config);
}

unique_ptr<idb_tb_result> pgsql_connect::query(const string& sql) const {
    if (!conn_) return nullptr;
    ::PGresult* res = ::PQexec(conn_, sql.data());
    if (!res) return nullptr;
    if (::PQresultStatus(res) != ::PGRES_TUPLES_OK) {
        ::PQclear(res);
        return nullptr;
    }
    return make_unique<pgsql_tb_result>(res, true);
}

unique_ptr<idb_prepared_statement> pgsql_connect::prepare_statement(const string& sql) const {
    if (!conn_) return nullptr;
    const auto stmt = new pgsql_prepared_statement(conn_, sql);
    if (!stmt->param_count() && sql.find('$') != string::npos) {
        delete stmt;
        return nullptr;
    }
    return unique_ptr<idb_prepared_statement>(stmt);
}

idb_connect* pgsql_factory::create_connect() {
    const auto conn = new pgsql_connect();
    if (!conn->connect_to(config_)) {
        delete conn;
        return nullptr;
    }
    return conn;
}

idb_result* pgsql_factory::create_result(void* native_result) {
    if (!native_result) return nullptr;
    const auto res = static_cast<::PGresult*>(native_result);
    return new pgsql_tb_result(res, false);
}

NEFORCE_END_NAMESPACE__
#endif
