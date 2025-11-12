#ifndef MSTL_DB_CONFIG_HPP__
#define MSTL_DB_CONFIG_HPP__
#include "MSTL/core/string.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(DatabaseTypeCastError, DatabaseError, "Database Type Cast Failed.")
MSTL_ERROR_BUILD_FINAL_CLASS(DatabasePreparedStmtError, DatabaseError, "Database Prepared Statement Operations Error.")


enum class DB_TYPE {
#ifdef MSTL_SUPPORT_MYSQL__
    MYSQL = 1,
#endif
#ifdef MSTL_SUPPORT_SQLITE3__
    SQLITE3,
#endif
#ifdef MSTL_SUPPORT_REDIS__
    REDIS
#endif
};

struct MSTL_API db_config {
    string username{};
    string password{};
    string database{};
    string host = "127.0.0.1";
    uint16_t port = 0;
    string charset{};

#ifdef MSTL_SUPPORT_MYSQL__
    static db_config for_mysql(const string& db);
#endif

#ifdef MSTL_SUPPORT_SQLITE3__
    static db_config for_sqlite(const string& file);
#endif

#ifdef MSTL_SUPPORT_REDIS__
    static db_config for_redis(const string& db);
#endif
};

MSTL_END_NAMESPACE__
#endif // MSTL_DB_CONFIG_HPP__
