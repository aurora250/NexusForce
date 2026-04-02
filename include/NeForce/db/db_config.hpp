#ifndef NEFORCE_DATABASE_DB_CONFIG_HPP__
#define NEFORCE_DATABASE_DB_CONFIG_HPP__
#include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_ERROR_BUILD_FINAL_CLASS(database_typecast_exception, database_exception, "Database Type Cast Failed.")
NEFORCE_ERROR_BUILD_FINAL_CLASS(database_prepared_stmt_exception, database_exception,
                                "Database Prepared Statement Operations Error.")


enum class db_type : uint8_t {
#ifdef NEFORCE_SUPPORT_MYSQL
    MYSQL = 1,
#endif
#ifdef NEFORCE_SUPPORT_SQLITE3
    SQLITE3,
#endif
#ifdef NEFORCE_SUPPORT_HIREDIS
    REDIS,
#endif
#ifdef NEFORCE_SUPPORT_POSTGRESQL
    POSTGRESQL
#endif
};

struct NEFORCE_API db_config {
    string username{};
    string password{};
    string database{};
    string host = "127.0.0.1";
    string charset{};
    uint16_t port = 0;

#ifdef NEFORCE_SUPPORT_POSTGRESQL
    static db_config for_postgresql(const string& db = "postgres");
#endif

#ifdef NEFORCE_SUPPORT_MYSQL
    static db_config for_mysql(const string& db);
#endif

#ifdef NEFORCE_SUPPORT_SQLITE3
    static db_config for_sqlite(const string& file);
#endif

#ifdef NEFORCE_SUPPORT_HIREDIS
    static db_config for_redis(const string& db);
#endif
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_DATABASE_DB_CONFIG_HPP__
